/*
 * IBM PowerPC 405 Watchdog: A Simple Hardware Watchdog Device
 * Based on PowerPC 8xx driver by Scott Anderson which was
 * based on MixCom driver by Gergely Madarasz which was
 * based on Softdog driver by Alan Cox and PC Watchdog driver by Ken Hollis
 *
 * FILE NAME ppc4xx_wdt.c
 *
 *  Author: MontaVista Software, Inc.  <source@mvista.com>
 *          Debbie Chu   <debbie_chu@mvista.com>
 *          Frank Rowand <frank_rowand@mvista.com>
 *
 * Copyright 2000 MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Version 0.1 (00/06/05):
 * Version 0.2 (00/07/12) by Debbie Chu
 * Version 0.3 (unknown) Montavista
 * Version 0.4 (01/09/26) Johannes Thoma
 * 	fixed initialization order in wdt_open() (previously, 
 *        405 did reset immeadently from time to time, 
 *        because wdt interrupt occured before TSR was reset).
 *      moved wdt= kernel command line parsing in here. 
 *
 */

/* 
 * There are three ways to enable the watchdog timer:
 * 1. turn on the watchdog in the bootrom.
 * 2. turn on the watchdog using the boot command line option,
 *    you can specifiy "wdt=<timeout>" on the boot cmdline
 * 3. turn on the watchdog in this routine,
 *    the default timer period is set to 2 minutes.
 */

/* 
 * Module not tested. ??? do it , little effort , since it should work.
 * Test ioctl's ??
 * Implement: read state of what caused last reset. 
 */ 


/* Define this to monitor heartbeat counters. */
#undef HB_DEBUG 
// #define HB_DEBUG 


#define WDT_DEFAULT_PERIOD	120	/* system default 2 minutes */
#define	WDT_MAX_PERIOD		3600UL	/* Max timeout period 60 minutes */

#undef DEBUG_WDT
// #define DEBUG_WDT

#ifdef DEBUG_WDT
  /* 2.684 sec */
#define WDT_HARDWARE_TIMEOUT TCR_WP(WP_2_29) 
#else
  /* 168 ms */
#define WDT_HARDWARE_TIMEOUT TCR_WP(WP_2_25) 
#endif 


  
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/machdep.h>

/* rename to wdt_enabledd */

static int wdt_enabled = 0;
static int wdt_enable_on_boot = 0;

static int ppc4xxwd_opened;
static unsigned long wdt_period = WDT_DEFAULT_PERIOD * HZ;
static unsigned long wdt_heartbeat_count;

/* The data written to TSR is not actual data but a mask. 

   This actually clears the watchdog interrupt status (by writing a 
   1 bit into it) and also resets the enable watchdog bit (which is 
   set when the hardware watchdog timer expires). All other bits are 
   left untouched. 
 */  

#define TRIGGER_HW_WDT mtspr(SPRN_TSR, (TSR_ENW | TSR_WIS))


void ppc4xx_wdt_heartbeat (void)
{
        if ((wdt_heartbeat_count > 0) || ( !wdt_enabled )) {
                if (wdt_heartbeat_count > 0) wdt_heartbeat_count--;
		TRIGGER_HW_WDT;

#ifdef HB_DEBUG
		if ((wdt_heartbeat_count > 0) && 
                    (wdt_heartbeat_count % 100 == 1)) {
			printk ("heartbeat at %d jiffies: %d left until reset.\n", jiffies, wdt_heartbeat_count); 
                }
#endif
        }
        else {
#ifdef DO_MANUAL_WD_RESET
                printk (KERN_CRIT "Watchdog Timer Timed out, system reset!\n");
		ppc4xx_restart (NULL);
#else

  /* After reboot, we want to know if it was the watchdog timer expiring that
     caused the reset, so let it crash .. */
                printk (KERN_CRIT "Watchdog Timer Timed out, system reset!\n");
//                cli ();
		local_irq_disable();
                while (1) ;
#endif
        }
        ppc_md.heartbeat_count = ppc_md.heartbeat_reset;
}


static inline void
ppc4xxwd_update_timer(void)
{
#ifdef HB_DEBUG
	printk ("<3>setting wdt_heartbeat_count to %d.\n", wdt_period);
#endif
	wdt_heartbeat_count = wdt_period; 
}


static int ppc4xxwd_update_period(int period)
 /* Period is in usecs, applies at next heartbeat countdown. We are
    converting from usecs into jiffies in here. */
{
	if (period <= 0) {	
		period = WDT_MAX_PERIOD * 1000000;
	}         

	wdt_period = (period / (1000000/HZ)) + (period % (1000000/HZ) ? 1 : 0);
	ppc4xxwd_update_timer();
	return 0;
}

static int ppc4xxwd_do_enable_wdt (void)
{
	unsigned int tcr_value;

	TRIGGER_HW_WDT;

	/* From here until this function exits, one ppc4xx watchdog 
           interrupt may occur (which we do not take, the timer interrupt
           is used instead, calling the heartbeat routine in this 
           file, see also ppc4xx_setup.c in the arch/ppc/kernel 
           directory). */

	tcr_value = mfspr(SPRN_TCR);

	if ((tcr_value & TCR_WRC_MASK) != TCR_WRC(WRC_SYSTEM)) {

	    /* 
	     * Watchdog reset not enabled yet, enable it.
             *
             * Watchdog timeouts occur every 168 ms (at 200 MHz processor
             * frequency). This means that after 168 ms of no watchdog
             * triggering, the system is reset (unless watchdog exception 
             * is caught, which we do not). 
             */

                mtspr(SPRN_TCR,
		   (mfspr(SPRN_TCR) & ~TCR_WP_MASK & ~TCR_WRC_MASK) |
		   WDT_HARDWARE_TIMEOUT |
                   TCR_WRC(WRC_SYSTEM));
	}

	ppc4xxwd_update_timer();
	wdt_enabled = 1;
	TRIGGER_HW_WDT;

	return 0;
}

/*
 *	Allow only one person to hold it open
 */
static int ppc4xxwd_open(struct inode *inode, struct file *file)
{
	if(test_and_set_bit(0,(void*) &ppc4xxwd_opened)) {
		return -EBUSY;
	}


	return ppc4xxwd_do_enable_wdt ();
}

static int ppc4xxwd_release(struct inode *inode, struct file *file)
{

	clear_bit(0,(void*) &ppc4xxwd_opened);
	return 0;
}


static ssize_t ppc4xxwd_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
	/*  Can't seek (pwrite) on this device  */
	if (ppos != &file->f_pos) {
		return -ESPIPE;
	}

	if(len) {
		ppc4xxwd_update_timer();
		return 1;
	}

	return 0;
}

static int ppc4xxwd_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	unsigned long period;
	int status;
	int state;
        static struct watchdog_info ident = {
		WDIOF_KEEPALIVEPING,
		0,
		"PPC 405 watchdog"
	};
                                        
	switch(cmd) {

		case WDIOC_GETSTATUS:
			status = ppc4xxwd_opened;

			if (copy_to_user((int *)arg, &status, sizeof(int))) {
				return -EFAULT;
			}
			break;

		case WDIOC_GETSUPPORT:
			if (copy_to_user((struct watchdog_info *)arg, &ident, 
			    sizeof(ident))) {
				return -EFAULT;
			}
			break;

		case WDIOC_KEEPALIVE:
	    		ppc4xxwd_update_timer();
			break;

		case WDIOC_SETOPTIONS:
			if(copy_from_user(&state, (int*) arg, sizeof(int)))
                                return -EFAULT;

                        if (state & WDIOS_DISABLECARD) {
				wdt_enabled = 0;
				printk(KERN_NOTICE "Soft watchdog timer is disabled\n");
				break;
			}
                        if (state & WDIOS_ENABLECARD) {
				ppc4xxwd_update_timer();
				wdt_enabled = 1;
				TRIGGER_HW_WDT;
				printk(KERN_NOTICE "Soft watchdog timer is enabled\n");
				break;
			}


		case WDIOC_SETTIMEOUT:
			/*
			** set watchdog period (units = microseconds)
			** value of zero means maximum
			**
			** Don't set a watchdog period to a value less than
			** the requested value (period will be rounded up to
			** next available interval the watchdog supports).
			**
			** The software watchdog will expire at some point in
			** the range of (rounded up period) ..
			** (rounded up period + 1 jiffie).  If interrupts are
			** disabled so that the software watchdog is unable to
			** reset the system, then the hardware watchdog will
			** eventually reset the system.
			*/

			if (copy_from_user(&period, (unsigned long *)arg,
			    sizeof(period))) {
			    return -EFAULT;
			}

			/* 
			** This code assumes HZ is 100.  Need to remove that
			** assumption.
			*/

			/*
			** The minimum period of ppc4xxwd_timer is a jiffie,
			** which is 10 msec when HZ is 100.  The units of
			** ppc4xxwd_timer_period is jiffies.
			**
			** The new timer period will be used at the next
			** heartbeat.
			*/

			ppc4xxwd_update_period (period);

			/* fall trough // break;  */
			
		case WDIOC_GETTIMEOUT:
			/* return watchdog period (units = microseconds) */
			period = (wdt_period / HZ) * 1000000;
			if (copy_to_user((unsigned long *)arg, &period, 
			    sizeof(period))) {
				return -EFAULT;
			}
			break;
			
			
		default:
			return -ENOIOCTLCMD;

	}
	return 0;
}

static struct file_operations ppc4xxwd_fops =
{
	owner:		THIS_MODULE,
	write:		ppc4xxwd_write,
	ioctl:		ppc4xxwd_ioctl,
	open:		ppc4xxwd_open,
	release:	ppc4xxwd_release,
};

static struct miscdevice ppc4xxwd_miscdev =
{
	WATCHDOG_MINOR,
	"405_watchdog",
	&ppc4xxwd_fops
};

static int __init ppc4xxwd_init(void)
{
	misc_register(&ppc4xxwd_miscdev);
	printk(KERN_NOTICE "PPC 405 watchdog driver.\n");

	if (wdt_enable_on_boot) {
		if (ppc4xxwd_do_enable_wdt ()!=0) {
		        panic ("Could not enable watchdog!\n");
		} else {
			printk (KERN_NOTICE "Watchdog is enabled with timeout %ld seconds.\n", wdt_period/HZ);
		}
	}

	return 0;
}	

void __exit ppc4xxwd_exit(void)
{
	misc_deregister(&ppc4xxwd_miscdev);
}

module_init(ppc4xxwd_init);
module_exit(ppc4xxwd_exit);


#ifndef MODULE

static int __init ppc4xx_wdt_setup(char *str)
{
	int timeout_in_secs;

/* ???? doesn't work with wdt=0 yet */
printk ("<4>XXX ppc4xx_wdt_setup: str=%s\n", str);

	str++;  
	get_option (&str, &timeout_in_secs);
	ppc4xxwd_update_period (timeout_in_secs * 1000000);	

	wdt_enable_on_boot = 1;
	printk (KERN_NOTICE "Watchdog timeout set to %d.\n", timeout_in_secs);

	return 1;
}

__setup ("wdt", ppc4xx_wdt_setup);

#endif 
	
