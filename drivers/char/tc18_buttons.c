/*
 *	Pushbutton device driver for TOPCALL TC18 board.
 *
 *	(c) Copyright 2001 Johannes Thoma
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *	
 *	Neither Alan Cox nor CymruNet Ltd. admit liability nor provide 
 *	warranty for any of this software. This material is provided 
 *	"AS-IS" and at no charge.	
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/delay.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/poll.h>

#include "tc18_lcd.h"

/* 240-255 are reserved for local use, see Documentation/devices.txt */

#define BUTTONS_MINOR 244
#define KB_BUF_LEN 16

static volatile unsigned long* gpio0_base;

static struct tc18_buttons { 
	unsigned long mask;               /* GPIO mask */ 
	char event;                       /* char that is read by userland */
        unsigned long last_event;         /* time in jiffies */
        unsigned long events_repeated;    /* time in jiffies */
} tc18_buttons[] = {
	{0x10, 'M', 0, 0},     /* Mode button (GPIO 27) */
	{0x20, 'S', 0, 0},     /* Set button (GPIO 26) */
	{0, 0, 0, 0}};
	 
struct buttons_delay {
	unsigned long sample_period;    /* in jiffies */
	unsigned long repeat_delay;     /* in periods */
	unsigned long repeat_key_slow;  /* in periods */
	unsigned long repeat_key_cnt;   /* in events  */
	unsigned long repeat_key_fast;  /* in periods */
};


struct buttons_delay delay = {
	sample_period:   1, 
	repeat_delay:    400,   
	repeat_key_slow: 440, 
	repeat_key_cnt:  5,
	repeat_key_fast: 40
};


static struct key_buffer { 
	struct semaphore lock;
	int head, tail; 
	char buf[KB_BUF_LEN];
} kb;

static struct timer_list button_timer;
static DECLARE_WAIT_QUEUE_HEAD(button_wait);


static void add_event (struct tc18_buttons *b) 
   /* Writer. This must be called only once .. there's no locking in 
      interrupt .. */
{
	int h;

	if (!b) {
		printk (KERN_INFO "Button driver: strange b is NULL!");
		return;
	}
	h=kb.head+1; 
	if (h>=KB_BUF_LEN) h=0;
		
	if (h==kb.tail) {
		static int msg = 0;
		if (msg++<5) {
			printk (KERN_INFO "Button buffer full, somebody should handle button events\n");
		}
		return;
	}
	kb.buf[kb.head=h] = b->event;
	b->last_event = jiffies;
	b->events_repeated++;

	wake_up_interruptible(&button_wait);
}
	

static char next_event (void) 
{
	int c;

	if (kb.tail == kb.head) return 0;

	down (&kb.lock);
	kb.tail++;
	if (kb.tail>=KB_BUF_LEN) kb.tail=0;
	c = kb.buf[kb.tail];
	up (&kb.lock);

	return c;
}

		

static void sample_buttons (unsigned long data) 
{
	static unsigned long last_sample=0;
	unsigned long this_sample;
        volatile unsigned long *gpio_in=
		(volatile unsigned long *)((char*) gpio0_base+GPIO0_IR);
	int i;
	int logic, my_delay;

	this_sample = *gpio_in;
	if (last_sample!=0) {
		for (i=0;tc18_buttons[i].mask;i++) {
 /* note that a '1' means button is not pressed .. */
			logic = ((last_sample & tc18_buttons[i].mask) == 0) +
			        (((this_sample & tc18_buttons[i].mask) == 0) << 1); 

			switch (logic) {
			case 0: /* no button press/release */
				break;
			case 1: /* button release */
				tc18_buttons[i].events_repeated = 0;
				break;
			case 2: /* button press */
				add_event (&tc18_buttons[i]);
				break;
			case 3: /* button stays pressed */
				my_delay = 
	(tc18_buttons[i].events_repeated==1) ? delay.repeat_delay : 
	((tc18_buttons[i].events_repeated < delay.repeat_key_cnt) ? 
		delay.repeat_key_slow : 
		delay.repeat_key_fast);
				if (jiffies>tc18_buttons[i].last_event+my_delay) {
					add_event (&tc18_buttons[i]);
				}
				break;
			default: 
				BUG();
			}
		}
	}

	last_sample = this_sample; 
        button_timer.expires = jiffies + delay.sample_period;
	add_timer (&button_timer);
}



static unsigned int
buttons_poll(struct file *file, poll_table * wait)
{
        poll_wait(file, &button_wait, wait);

        if (kb.head != kb.tail) {
		return POLLIN | POLLRDNORM;
	}
        return 0;
}

	

static ssize_t buttons_read(struct file *file, char *buf, size_t nbytes, loff_t *ptr)
{
        DECLARE_WAITQUEUE(wait, current);
        ssize_t count = 0;
	char c;

/*        if (ptr != &file->f_pos)
                return -ESPIPE;
*/
        if (nbytes == 0)
                return 0;

try_again: 
	while ((nbytes > 0) && (c=next_event())) {
        	if (copy_to_user(buf, &c, 1)) {
			return -EFAULT;
		}
		nbytes--;
		count++;
		buf++;
	}
	if (count == 0) {
		if (file->f_flags & O_NONBLOCK) {
               		return -EAGAIN;
		}
		if (signal_pending(current)) {
                	return -ERESTARTSYS;
		}

		add_wait_queue(&button_wait, &wait);
	        set_current_state(TASK_INTERRUPTIBLE);
		schedule();
       		if ( current->state != TASK_RUNNING ) {
			printk ("interresting: i am not running ;) ");
       			current->state = TASK_RUNNING;
		}
	        remove_wait_queue(&button_wait, &wait);
	
		goto try_again;
	}

	return count;
}


static int buttons_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int buttons_release(struct inode *inode, struct file *file)
{
	return 0;
}


/*
 *	Kernel Interfaces
 */
 
 
static struct file_operations buttons_fops = {
	owner:		THIS_MODULE,
	llseek:		no_llseek,
	poll:		buttons_poll,
	read:		buttons_read,
	write:		NULL,
	ioctl:		NULL,
	open:		buttons_open,
	release:	buttons_release,
};

static struct miscdevice buttons_miscdev=
{
	BUTTONS_MINOR,
	"push_buttons",
	&buttons_fops
};

 
static void __exit buttons_exit(void)
{
	del_timer (&button_timer);
	
	iounmap(gpio0_base);

	misc_deregister(&buttons_miscdev);
}

 
static int __init buttons_init(void)
{
	int ret;

	gpio0_base = ioremap(GPIO0_BASE, 0x45);
	ret = misc_register(&buttons_miscdev);
	if (ret) {
		printk(KERN_ERR "buttons: can't misc_register on minor=%d\n", BUTTONS_MINOR);
		return ret;
	}
	kb.head = kb.tail = 0;

        init_timer(&button_timer);
        button_timer.expires = jiffies + delay.sample_period;
        button_timer.function = &sample_buttons;
        add_timer(&button_timer);

        init_MUTEX(&kb.lock);

	printk(KERN_INFO "buttons: TOPCALL Buttons driver for TC18\n");

	return 0;
}

module_init(buttons_init);
module_exit(buttons_exit);

