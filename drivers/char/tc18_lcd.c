/*
 *	LCD device driver for TOPCALL TC18 board.
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
#include <linux/smp_lock.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/delay.h>
#include <linux/init.h>
#include <linux/ctype.h>


/* Do require '[' after ESC */
#define STRICTLY_ANSI

/* 240-255 are reserved for local use, see Documentation/devices.txt */
#define LCD_MINOR 243

#define LCD_BEEPER 0x100000

#define TAB_STOP 4

/* In HZ */
#define SOUND_ON   20
#define SOUND_OFF  20  

 /* eventually it's getting anoying so set a limit .. */
#define MAX_BEEPS  10  


#include "tc18_lcd.h"


static volatile unsigned long* gpio0_base;
static volatile unsigned long* gpio1_base;




static struct lcd_state {
	int x, y;               /* Current cursor position */
	char screen[LCD_HEIGHT][LCD_WIDTH]; /* Contents of LCD */
	int cursor_visible;     /* Nonzero if cursor visible */
	
	char buf[10];           /* Holds ANSI command to process */
	int next;               /* Index into buf */
	enum state { LCD_WRITE, LCD_READ_CMD, LCD_GOT_ESC } state;
                                /* ANSI parser statemachine */

} lcd_state;  /* globals suck */


#if 0 

maybe we need it again ..
static void dump_lcd_state (void)
{
	int i, j;

	lcd_state.buf[9]='\0';
	printk (KERN_ERR "Cursor (%d/%d) %s cmd=%s (next=%d) state=%s\n", 
		lcd_state.x, lcd_state.y, (lcd_state.cursor_visible?"visible":"not visible"), lcd_state.buf, lcd_state.next, (lcd_state.state==LCD_WRITE?"Write":(lcd_state.state==LCD_READ_CMD?"Read cmd":"Got ESC")));
	printk (KERN_ERR);
	for (i=0;i<LCD_HEIGHT;i++) {
	for (j=0;j<LCD_WIDTH;j++) {
		printk ("%c", lcd_state.screen[i][j]);
	}
	printk ("\n" KERN_ERR);
	}
	printk ("\n");
}

#endif

static void outnib (unsigned long nib, unsigned long inst)
      /* Gibt die unteren 4 Bit von 'nib' an das LCD Display aus. */

{
	volatile unsigned long *gpio_out0 = (unsigned long *) ( (char*) gpio0_base + GPIO0_OR);
	volatile unsigned long *gpio_out1 = (unsigned long *) ( (char*) gpio1_base + GPIO0_OR);

	nib &= 0x0f;

	*gpio_out1 = (*gpio_out1 & LCD_MASK1) | DSBL | inst ;
	udelay(1);
	*gpio_out0 = (*gpio_out0 & LCD_MASK0) | nib  ;
	*gpio_out1 = (*gpio_out1 & LCD_MASK1) | ENBL | inst ;
	udelay(1);
	*gpio_out1 = (*gpio_out1 & LCD_MASK1) | DSBL | inst ;
	udelay(1);
}


static void out (unsigned long byte, unsigned long inst)
     /* Schreibt 'byte' in Register 'inst' des LCD Displays */

{
	outnib((byte >> 4) & 0xf,inst);
	outnib(byte & 0xf,inst);
}


static void lcd_init_hw (void)
      /* Initialisiert das LCD Display nach Power-On. */

{
	/* drive LCDEN, LCDRS and D0-D3 low. */
	volatile unsigned long *gpio_out0 = (unsigned long *) ( (char*) gpio0_base + GPIO0_OR);
	volatile unsigned long *gpio_out1 = (unsigned long *) ( (char*) gpio1_base + GPIO0_OR);

	*gpio_out0 &= LCD_MASK0;
	*gpio_out1 &= LCD_MASK1;
	
	outnib(0x03,INST);
	udelay(4200);
	outnib(0x03,INST);
	udelay(110);
	outnib(0x03,INST);
	udelay(10000);

	outnib(0x02,INST);  /* 4 Bit-Bus  mode */
	udelay(10000);

	out(0x28,INST);     /* 4 Bit-Bus, 2-Zeilig, 5x7 dots */
	udelay(50);
	out(0x08,INST);     /* Display off */
	udelay(50);
	out(0x01,INST);     /* Clear Display */
	udelay(1650);
	out(0x06,INST);     /* Cursor Increment mode */
	udelay(50);
	out(0x0c,INST);     /* Display on, Cursor off, Blink off */
//	out(0x0f,INST);     /* Display on, Cursor off, Blink off */
	udelay(50);
}


static int beeps = 0;
static struct timer_list beep_timer;


void beeper (int flag) 
{
	volatile long *or = (long *) ( (char*) gpio0_base + GPIO0_OR);
	if (flag) (*or)|= LCD_BEEPER;	
	else (*or) &= ~LCD_BEEPER; 	
}


void beep_on (unsigned long unused);

void beep_off (unsigned long unused)
{
	beeper (0);
	if (beeps<=0) { 
		printk ("TC18beeper: Strange: beeps <= 0");
		return;
	}
	beeps--;
	if (beeps>0) {
	        beep_timer.expires = jiffies + SOUND_OFF;
		beep_timer.function = &beep_on;
		add_timer(&beep_timer);
	}
}

void beep_on (unsigned long unused)
{
	beeper (1);

	beep_timer.expires = jiffies + SOUND_ON;
	beep_timer.function = &beep_off;
	add_timer(&beep_timer);
}
	
static void beep (void)
{
	if (beeps<MAX_BEEPS) beeps++;
	if (beeps==1) { 
	        beep_timer.expires = jiffies + 2;
		beep_timer.function = &beep_on;
		add_timer(&beep_timer);
	} 
}
	
static int atoi (const char *s)
 /* use simple_strtoul instead .. */
{
	int i=0;
	if (!s) return 0;
	while (*s>='0' && *s<='9') {
		i=(i*10)+(*s)-'0';
		s++;
	}
	return i;
}

static void hide_cursor (void)
{
	out (0x0c, INST);
	udelay (50);
}

static void show_cursor (void)
{
	out (0x0d, INST);
	udelay (50);
}

static void set_cursor (int x, int y)
{
	if (x<0||x>=LCD_WIDTH||y<0||y>=LCD_HEIGHT) return;
	out (x + y*0x40 + 0x80, INST);
	udelay (50);
}

static void print_char (int c)
{
	out (c, DATA);
	udelay (50);
}


void redisplay_everything (void)
{
	int i, j;

	hide_cursor ();
	for (i=0;i<LCD_HEIGHT;i++) {
		set_cursor (0, i);
		for (j=0;j<LCD_WIDTH;j++) {
			print_char (lcd_state.screen[i][j]);
		}
	}
	set_cursor (lcd_state.x, lcd_state.y);
	if (lcd_state.cursor_visible) show_cursor ();
}

	
	
void do_cmd (const char *buf, const char cmd)
{
	int p1, p2; 
	int i, j;

	p1=atoi (buf);
	switch (cmd) {
	case 'J': 
		if (p1==1 || p1==2) {
			for (i=0;i<LCD_WIDTH;++i)
			for (j=0;j<LCD_HEIGHT;++j) {
				lcd_state.screen[j][i]=' ';
			}
			redisplay_everything ();
		}
		break;
	case 'A': 
		if (p1==0) p1=1;
		lcd_state.y-=p1;
		if (lcd_state.y<0) lcd_state.y=0;
		break;	
	case 'B': 
		if (p1==0) p1=1;
		lcd_state.y+=p1;
		if (lcd_state.y>=LCD_HEIGHT) lcd_state.y=LCD_HEIGHT-1;
		break;	
	case 'C': 
		if (p1==0) p1=1;
		lcd_state.x+=p1;
		if (lcd_state.x>=LCD_WIDTH) lcd_state.x=LCD_WIDTH-1;
		break;	
	case 'D': 
		if (p1==0) p1=1;
		lcd_state.x-=p1;
		if (lcd_state.x<0) lcd_state.x=0;
		break;	
	case 'l': 
		if (buf[0]=='?') {
			p1=atoi (buf+1);
			if (p1==25) lcd_state.cursor_visible=0;
			hide_cursor ();
		}
		break;
	case 'h': 
		if (buf[0]=='?') {
			p1=atoi (buf+1);
			if (p1==25) lcd_state.cursor_visible=1;
			show_cursor ();
		}
		break;
	case 'H': 
		for (i=0;buf[i] && buf[i]!=';';i++) ;
		if (buf[i]==';') p2=atoi(buf+i+1);
		else p2=0;
  /* convert ANSI (starting at 1) to C (starting at 0) position */
		if (p1>0) --p1;
		if (p2>0) --p2;
		lcd_state.y=p1; 
		lcd_state.x=p2; 
		break;

	case 'K': /* clear to EOL */
		for (i=lcd_state.x;i<LCD_WIDTH;++i) {
			lcd_state.screen[lcd_state.y][i]=' ';
			print_char (' ');
		}
		set_cursor (lcd_state.x, lcd_state.y);
		break;
	
	default:
		break;	/* ignore unknown commands */
	}
		/* The special commands never cause scrolling. Other
                   cases handled by caller */
	if (lcd_state.y>=LCD_HEIGHT) lcd_state.y=LCD_HEIGHT-1;
}
		
			
			

void do_char (char c)
{
	int old_x, old_y;

	old_x=lcd_state.x;
	old_y=lcd_state.y;
	
	switch (c) {
	case '\e': lcd_state.state=LCD_GOT_ESC; lcd_state.next=0; break;
/* Strange enough, but control characters inbetween escape sequences 
   are interpreted (maybe its just the PuTTY behaviour) */
	case '\b': lcd_state.x--;  break;
	case '\n': lcd_state.y++; // break; \n does CR and LF
	case '\r': lcd_state.x=0; break;
	case '\t': lcd_state.x=lcd_state.x-(lcd_state.x%TAB_STOP)+TAB_STOP; break;
	case '\a': beep (); break;

	default: 
		switch (lcd_state.state) {
		case LCD_GOT_ESC:
#ifdef STRICTLY_ANSI
			lcd_state.state = ((c=='[') ? LCD_READ_CMD : LCD_WRITE);
			break;
#else
			if (c=='[') break;  /* ignore '[' */

				/* else fall thru .. if not STRICTLY_ANSI, 
                           	GOT_ESC is same as READ_CMD.. */
#endif 
		case LCD_READ_CMD: 
			if (lcd_state.next<9) {
				if (isalpha (c)) {
					lcd_state.buf[lcd_state.next]='\0';
					do_cmd (lcd_state.buf, c);
					lcd_state.state=LCD_WRITE;
				} else {
					lcd_state.buf[lcd_state.next]=c;
					lcd_state.next++;
				}  
			} else {     /* ignore cmds longer than that */
                        	if (isalpha (c)) lcd_state.state=LCD_WRITE;
			}

			break;
		
		case LCD_WRITE: 
			lcd_state.screen[lcd_state.y][lcd_state.x]=c;
			lcd_state.x++;
			print_char (c);
			if (lcd_state.x>=LCD_WIDTH) {
   				/* LCD would utilize scroll buffer if printing out of range */
				lcd_state.x=LCD_WIDTH-1;
				set_cursor (lcd_state.x, lcd_state.y);
			}
			return; 

		default:
			break;
		}
	}
	if (lcd_state.x>=LCD_WIDTH) lcd_state.x=LCD_WIDTH-1;
	if (lcd_state.x<0) lcd_state.x=0;

	if (lcd_state.y<0) lcd_state.y=0; 
	if (lcd_state.y>=LCD_HEIGHT) {
  /* Allows any value for y -- if larger than double screen size, screen 
     will be blank. */
		int lines_to_copy, i, j;
		lines_to_copy=LCD_HEIGHT-lcd_state.y+1;
		for (i=0;i<lines_to_copy;++i) {
			for (j=0;j<LCD_WIDTH;++j) {
				lcd_state.screen[i][j]=lcd_state.screen[i+lines_to_copy][j];
			}
		}
		for (;i<LCD_HEIGHT;++i) {
			for (j=0;j<LCD_WIDTH;++j) {
				lcd_state.screen[i][j]=' ';
			}
		}
		lcd_state.y=LCD_HEIGHT-1;
		redisplay_everything ();
	} else {
		if ((lcd_state.x!=old_x) || (lcd_state.y!=old_y)) {
			set_cursor (lcd_state.x, lcd_state.y);
		}
	}
}

static int do_lcd_reset (void)
{
	int i, j;

	lcd_init_hw (); 

	for (i=0;i<LCD_HEIGHT;++i) {
		for (j=0;j<LCD_WIDTH;++j) {
			lcd_state.screen[i][j]=' ';
		}
	}
	lcd_state.cursor_visible=1;
	lcd_state.x=0;
	lcd_state.y=0;
	lcd_state.next=0;
	lcd_state.state=LCD_WRITE;

	redisplay_everything ();
	
	return 0;
}


static int do_init (void)
{
	static int did_init_lcd=0;
	int ret = 0;

	if (!did_init_lcd) {
		ret = do_lcd_reset ();
		if (ret == 0) did_init_lcd = 1;
	}
	return ret;
}

#ifndef MODULE

#if 0 

/**
 *	lcd_setup:
 *	@str: command line string
 *
 *	Prints a bootup message to LCD. 
 */
??? remove if not needed.
 
static int __init lcd_setup(char *str)
{
	return 1;
maybe put boot message here ..
	int ints[4];

	str = get_options (str, ARRAY_SIZE(ints), ints);

	if (ints[0] > 0)
	{
		io = ints[1];
		if(ints[0] > 1)
			irq = ints[2];
	}

	return 1;
}

__setup("lcd=", lcd_setup);

#endif 

#endif /* !MODULE */


static void lcd_print_bootmsg (void)

	/* This function may be called very early, so that message is
         * actually visible before it gets overwritten by something else. 
         */

{
        int i;
        char *buf="\e[?25lBooting TC/Linux\nVersion ";
        char *buf2=UTS_RELEASE;

	do_init ();

        for (i=0; buf[i]; i++) {
               do_char (buf[i]);
        }
        for (i=0; buf2[i] && buf2[i]!='-'; i++) ;
	if (!buf2[i]) goto unknown;
        i++;
        for (; buf2[i] && buf2[i]!='-'; i++) ;
	if (!buf2[i]) goto unknown;
        i++;

        for (; buf2[i]; i++) { 
                do_char (buf2[i]);
        }
	return;

unknown:
	do_char ('?');
	do_char ('?');

}

/*
 *	Kernel methods.
 */
 

static long long lcd_llseek(struct file *file, long long offset, int origin)
{
	return -ESPIPE;
}


/**
 *	lcd_write:
 *	@file: file handle to the lcd device
 *	@buf: buffer to write 
 *	@count: count of bytes
 *	@ppos: pointer to the position to write. No seeks allowed.
 *
 *      Prints string to LCD panel. Special ANSI codes for cursor
 *      movement will be interpreted. 
 */
 
static ssize_t lcd_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int i;
	
	/*  Can't seek (pwrite) on this device  */
/*	if (ppos != &file->f_pos)
		return -ESPIPE; */

	for (i=0; i<count; i++) {
		do_char (buf[i]);
	}
		
	return count;
}

/**
 *	lcd_read:
 *	@file: file handle to the lcd driver
 *	@buf: buffer to write 1 byte into
 *	@count: length of buffer
 *	@ptr: offset (no seek allowed)
 *
 */
 
static ssize_t lcd_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{
	return -EINVAL;
}

/**
 *	lcd_ioctl:
 *	@inode: inode of the device
 *	@file: file handle to the device
 *	@cmd: LCD command
 *	@arg: argument pointer
 *
 *      No ioctls on that device.
 */
 
static int lcd_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	unsigned long arg)
{
	return -ENOIOCTLCMD;
}

/**
 *	lcd_open:
 *	@inode: inode of device
 *	@file: file handle to device
 *
 *      Does nothing .. 
 */
 
static int lcd_open(struct inode *inode, struct file *file)
{
	return 0;
/* inode->i_rdev to check ? */
}

/**
 *	lcd_close:
 *	@inode: inode to board
 *	@file: file handle to board
 *
 *      Does nothing .. 
 */
 
static int lcd_release(struct inode *inode, struct file *file)
{
	return 0;
}


/*
 *	Kernel Interfaces
 */
 
 
static struct file_operations lcd_fops = {
	owner:		THIS_MODULE,
	llseek:		lcd_llseek,
	read:		lcd_read,
	write:		lcd_write,
	ioctl:		lcd_ioctl,
	open:		lcd_open,
	release:	lcd_release,
};

static struct miscdevice lcd_miscdev=
{
	LCD_MINOR,
	"lcd",
	&lcd_fops
};

/**
 *	cleanup_module:
 *
 *	Deregisters the misc device.
 */
 
static void __exit lcd_exit(void)
{
	beeper (0);   /* Ugh .. we would sound forever .. */
        del_timer (&beep_timer);
	
	iounmap(gpio0_base);
	iounmap(gpio1_base);
	
	misc_deregister(&lcd_miscdev);
}

/**
 * 	lcd_init:
 *
 *	(Re-)initializes hardware and registers misc device (major=10) with 
 *      minor LCD_MINOR.
 */
 
static int __init lcd_init(void)
{
	int ret;
	
	gpio0_base = ioremap(GPIO0_BASE, 0x45);
	gpio1_base = ioremap(GPIO1_BASE, 0x45);
	ret=do_init ();
	
	if (ret) {
		printk(KERN_ERR "lcd: can't initialize hardware (errno=%d).\n", -ret);
		return ret;
	}
		
	ret = misc_register(&lcd_miscdev);
	if (ret) {
		printk(KERN_ERR "lcd: can't misc_register on minor=%d\n", LCD_MINOR);
		return ret;
	}
        init_timer(&beep_timer);

	printk(KERN_INFO "lcd: TOPCALL LCD driver for TC18\n");
	lcd_print_bootmsg (); 

	return 0;
}

module_init(lcd_init);
module_exit(lcd_exit);

