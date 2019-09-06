/*
 * arch/ppc/platforms/4xx/ash.h
 *
 * Macros, definitions, and data structures specific to the IBM PowerPC
 * Ash eval board.
 *
 * Author: Armin Kuster <akuster@mvista.com>
 *
 * 2000-2002 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifdef __KERNEL__
#ifndef __ASM_ASH_H__
#define __ASM_ASH_H__
#include <platforms/4xx/ibmnp405l.h>
#include <asm/ppcboot.h>

#ifndef __ASSEMBLY__

/*include bd_t from ppcboot.h*/

#if 0
/*
 * Data structure defining board information maintained by the boot
 * ROM on IBM's "Ash" evaluation board. An effort has been made to
 * keep the field names consistent with the 8xx 'bd_t' board info
 * structures.
 */


typedef struct board_info {

	unsigned long   bi_memstart;    /* start of  DRAM memory        */
	unsigned long   bi_memsize;     /* size  of  DRAM memory in bytes */
	unsigned long   bi_flashstart;  /* start of FLASH memory        */
	unsigned long   bi_flashsize;   /* size  of FLASH memory        */
	unsigned long   bi_flashoffset; /* reserved area for startup monitor */
	unsigned long   bi_sramstart;   /* start of  SRAM memory        */
	unsigned long   bi_sramsize;    /* size  of  SRAM memory        */
	unsigned long   bi_bootflags;   /* boot / reboot flag (for LynxOS) */
	unsigned long   bi_ip_addr;     /* IP Address                   */
// ???	unsigned char   bi_enetaddr[6]; /* Ethernet adress              */
	unsigned short  bi_ethspeed;    /* Ethernet speed in Mbps       */
	unsigned long   bi_intfreq;     /* Internal Freq, in MHz        */
	unsigned long   bi_busfreq;     /* Bus Freq, in MHz             */
	unsigned long   bi_baudrate;    /* Console Baudrate     */


	unsigned char	 bi_s_version[4];	/* Version of this structure */
	unsigned char	 bi_r_version[30];	/* Version of the IBM ROM */
	unsigned int	 bi_memsize;		/* DRAM installed, in bytes */
	unsigned char	 bi_enetaddr[4][6];	/* Local Ethernet MAC address */
	unsigned char	 bi_pci_enetaddr[6];
	unsigned int	 bi_intfreq;		/* Processor speed, in Hz */
	unsigned int	 bi_busfreq;		/* PLB Bus speed, in Hz */
	unsigned int	 bi_pci_busfreq;	/* PCI speed in Hz */
} bd_t;
#endif

/* Some 4xx parts use a different timebase frequency from the internal clock.
*/
#define bi_tbfreq bi_intfreq

/* Memory map for the IBM "TC16" NP405L evaluation board.
 */


/* Early initialization address mapping for block_io.
 * Standard 405GP map.
 */
#define PPC4xx_PCI_IO_PADDR	((uint)PPC405_PCI_PHY_IO_BASE)
#define PPC4xx_PCI_IO_VADDR	PPC4xx_PCI_IO_PADDR
#define PPC4xx_PCI_IO_SIZE	((uint)64*1024)
#define PPC4xx_PCI_CFG_PADDR	((uint)PPC405_PCI_CONFIG_ADDR)
#define PPC4xx_PCI_CFG_VADDR	PPC4xx_PCI_CFG_PADDR
#define PPC4xx_PCI_CFG_SIZE	((uint)4*1024)
#define PPC4xx_PCI_LCFG_PADDR	((uint)0xef400000)
#define PPC4xx_PCI_LCFG_VADDR	PPC4xx_PCI_LCFG_PADDR
#define PPC4xx_PCI_LCFG_SIZE	((uint)4*1024)
#define PPC4xx_ONB_IO_PADDR	((uint)0xef600000)
#define PPC4xx_ONB_IO_VADDR	PPC4xx_ONB_IO_PADDR
#define PPC4xx_ONB_IO_SIZE	((uint)4*1024)

#define NR_BOARD_IRQS 32

#ifdef CONFIG_PPC405GP_INTERNAL_CLOCK
#define BASE_BAUD		201600
#else
#define BASE_BAUD		691200
#endif

#define PPC4xx_MACHINE_NAME "IBM NP405L TC16"

extern char pci_irq_table[][4];


#endif /* !__ASSEMBLY__ */
#endif /* __ASM_ASH_H__ */
#endif /* __KERNEL__ */
