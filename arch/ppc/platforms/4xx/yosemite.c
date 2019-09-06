/*
 * arch/ppc/platforms/4xx/yosemite.c
 *
 * Yosemite board specific routines
 *
 * Wade Farnsworth <wfarnsworth@mvista.com>
 * Copyright 2004 MontaVista Software Inc.
 *
 * Copyright 2005 John Otken
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/config.h>
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/initrd.h>
#include <linux/irq.h>
#include <linux/seq_file.h>
#include <linux/root_dev.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/ethtool.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/ocp.h>
#include <asm/pci-bridge.h>
#include <asm/time.h>
#include <asm/todc.h>
#include <asm/bootinfo.h>
#include <asm/ppc4xx_pic.h>
#include <asm/ppcboot.h>

#include <syslib/gen550.h>
#include <syslib/ibm440gx_common.h>
#include <syslib/ibm440ep_common.h>

/*
 * This is a horrible kludge, we eventually need to abstract this
 * generic PHY stuff, so the  standard phy mode defines can be
 * easily used from arch code.
 */
#include "../../../../drivers/net/ibm_emac/ibm_emac_phy.h"

#if defined(CONFIG_BAMBOO)
static const int bamboo = 1;
static const char cpu_name[] = {"440EP"};
static const char brd_name[] = {"Bamboo"};
#elif defined(CONFIG_YELLOWSTONE)
static const int bamboo = 0;
static const char cpu_name[] = {"440GR"};
static const char brd_name[] = {"Yellowstone"};
#else
static const int bamboo = 0;
static const char cpu_name[] = {"440EP"};
static const char brd_name[] = {"Yosemite"};
#endif

bd_t __res;

static struct ibm44x_clocks clocks __initdata;

/*
 * Yosemite external IRQ triggering/polarity settings
 */
unsigned char ppc4xx_uic_ext_irq_cfg[] __initdata = {
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE), /* IRQ0: Ethernet transceiver */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_POSITIVE), /* IRQ1: Expansion connector */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE), /* IRQ2: PCI slot 0 */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE), /* IRQ3: PCI slot 1 */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE), /* IRQ4: PCI slot 2 */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE), /* IRQ5: PCI slot 3 */
	(IRQ_SENSE_EDGE  | IRQ_POLARITY_NEGATIVE), /* IRQ6: SMI pushbutton */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE), /* IRQ7: EXT */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE), /* IRQ8: EXT */
	(IRQ_SENSE_LEVEL | IRQ_POLARITY_NEGATIVE), /* IRQ9: EXT */
};

static void __init
yosemite_calibrate_decr(void)
{
	unsigned int freq;

	if (mfspr(SPRN_CCR1) & CCR1_TCS)
		freq = bamboo ? BAMBOO_TMRCLK : YOSEMITE_TMRCLK;
	else
		freq = clocks.cpu;

	ibm44x_calibrate_decr(freq);
}

static int
yosemite_show_cpuinfo(struct seq_file *m)
{
	seq_printf(m, "vendor\t\t: AMCC\n");
	seq_printf(m, "machine\t\t: PPC %s EVB (%s)\n", cpu_name, brd_name);

	return 0;
}

static inline int
yosemite_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	static char pci_irq_table[][4] =
	/*
	 *	PCI IDSEL/INTPIN->INTLINE
	 * 	   A   B   C   D
	 */
	{
		{ 28, 28, 28, 28 },	/* IDSEL 1 - PCI Slot 0 */
		{ 27, 27, 27, 27 },	/* IDSEL 2 - PCI Slot 1 */
		{ 26, 26, 26, 26 },	/* IDSEL 3 - PCI Slot 2 */
		{ 25, 25, 25, 25 },	/* IDSEL 4 - PCI Slot 3 */
	};

	const long min_idsel = 1, max_idsel = 4, irqs_per_slot = 4;
	return PCI_IRQ_TABLE_LOOKUP;
}

static void __init yosemite_set_emacdata(void)
{
	unsigned char * selection1_base;
	struct ocp_def *def;
	struct ocp_func_emac_data *emacdata;
	u8 selection1_val;
	int mode = PHY_MODE_RMII;	/* default to RMII for yosemite */

	if (bamboo)			/* if bamboo board */
	{
		selection1_base = ioremap64(BAMBOO_FPGA_SELECTION1_REG_ADDR, 16);
		selection1_val = readb(selection1_base);
		iounmap((void *) selection1_base);
		if (BAMBOO_SEL_MII(selection1_val))
		    mode = PHY_MODE_MII;
		else if (BAMBOO_SEL_RMII(selection1_val))
		    mode = PHY_MODE_RMII;
		else 
		    mode = PHY_MODE_SMII;
	}
	
	/* Set mac_addr and phy mode for each EMAC */

	def = ocp_get_one_device(OCP_VENDOR_IBM, OCP_FUNC_EMAC, 0);
	emacdata = def->additions;
	memcpy(emacdata->mac_addr, __res.bi_enetaddr, 6);
	emacdata->phy_mode = mode;

	def = ocp_get_one_device(OCP_VENDOR_IBM, OCP_FUNC_EMAC, 1);
	emacdata = def->additions;
	memcpy(emacdata->mac_addr, __res.bi_enet1addr, 6);
	emacdata->phy_mode = mode;
}

static int
yosemite_exclude_device(unsigned char bus, unsigned char devfn)
{
	return (bus == 0 && devfn == 0);
}

static unsigned long 
yosemite_ptm1_memory(unsigned long in_size)
{
	if (in_size == PPC44x_MEM_SIZE_8M)
		return PPC44x_MEM_SIZE_8M;

	if ((in_size > PPC44x_MEM_SIZE_8M) && (in_size <= PPC44x_MEM_SIZE_16M))
		return PPC44x_MEM_SIZE_16M;

	if ((in_size > PPC44x_MEM_SIZE_16M) && (in_size <= PPC44x_MEM_SIZE_32M))
		return PPC44x_MEM_SIZE_32M;

	if ((in_size > PPC44x_MEM_SIZE_32M) && (in_size <= PPC44x_MEM_SIZE_64M))
		return PPC44x_MEM_SIZE_64M;

	if ((in_size > PPC44x_MEM_SIZE_64M) && (in_size <= PPC44x_MEM_SIZE_128M))
		return PPC44x_MEM_SIZE_128M;

	if ((in_size > PPC44x_MEM_SIZE_128M) && (in_size <= PPC44x_MEM_SIZE_256M))
		return PPC44x_MEM_SIZE_256M;
	if ((in_size > PPC44x_MEM_SIZE_256M) && (in_size <= PPC44x_MEM_SIZE_512M))
		return PPC44x_MEM_SIZE_512M;

	return 0;
}

#define PCI_READW(offset) \
        (readw((void *)((u32)pci_reg_base+offset)))

#define PCI_WRITEW(value, offset) \
	(writew(value, (void *)((u32)pci_reg_base+offset)))
	
#define PCI_WRITEL(value, offset) \
	(writel(value, (void *)((u32)pci_reg_base+offset)))

static void __init
yosemite_setup_pci(void)
{
	void *pci_reg_base;
	unsigned long memory_size;
	memory_size = ppc_md.find_end_of_memory();

	pci_reg_base = ioremap64(YOSEMITE_PCIL0_BASE, YOSEMITE_PCIL0_SIZE);

	/* Enable PCI I/O, Mem, and Busmaster cycles */
	PCI_WRITEW(PCI_READW(PCI_COMMAND) | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER, PCI_COMMAND);

	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM0MA);                             /* Disable region first */
	PCI_WRITEL(YOSEMITE_PCI_PHY_MEM_BASE, YOSEMITE_PCIL0_PMM0LA);       /* PLB starting addr: 0x00000000A0000000 */
	PCI_WRITEL(YOSEMITE_PCI_MEM_BASE, YOSEMITE_PCIL0_PMM0PCILA);        /* PCI start addr, 0xA0000000 (PCI Address) */
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM0PCIHA);
	PCI_WRITEL(((0xffffffff -
			(YOSEMITE_PCI_UPPER_MEM -
			 YOSEMITE_PCI_MEM_BASE)) | 0x01), YOSEMITE_PCIL0_PMM0MA);/* Enable no pre-fetch, enable region */
	/*PCI_WRITEL(0xFC000001, YOSEMITE_PCIL0_PMM0MA);*/                    /* 64 MB, Enable no pre-fetch, enable region */
	
	/* Disable region one */
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM1MA);
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM1LA);
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM1PCILA);
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM1PCIHA);
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM1MA);
	
	/* Disable region two */
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM2MA);
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM2LA);
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM2PCILA);
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM2PCIHA);
	PCI_WRITEL(0, YOSEMITE_PCIL0_PMM2MA);
	
	/* Now configure the PCI->PLB windows, we only use PTM1
	 *
	 * For Inbound flow, set the window size to all available memory
	 * This is required because if size is smaller,
	 * then Eth/PCI DD would fail as PCI card not able to access
	 * the memory allocated by DD.
	 */
	
	PCI_WRITEL(0, YOSEMITE_PCIL0_PTM1MS);             /* disabled region 1 */
	PCI_WRITEL(0, YOSEMITE_PCIL0_PTM1LA);             /* begin of address map */
	memory_size = yosemite_ptm1_memory(memory_size) | 0x00000001;
	PCI_WRITEL(memory_size, YOSEMITE_PCIL0_PTM1MS);   /* Size low + Enabled */
	
	eieio();
	iounmap(pci_reg_base);
}

static void __init
yosemite_setup_hose(void)
{
	unsigned int bar_response, bar;
	struct pci_controller *hose;

	yosemite_setup_pci();

	hose = pcibios_alloc_controller();

	if (!hose)
		return;

	hose->first_busno = 0;
	hose->last_busno = 0xff;

	hose->pci_mem_offset = YOSEMITE_PCI_MEM_OFFSET;

	pci_init_resource(&hose->io_resource,
			YOSEMITE_PCI_LOWER_IO,
			YOSEMITE_PCI_UPPER_IO,
			IORESOURCE_IO,
			"PCI host bridge");

	pci_init_resource(&hose->mem_resources[0],
			YOSEMITE_PCI_LOWER_MEM,
			YOSEMITE_PCI_UPPER_MEM,
			IORESOURCE_MEM,
			"PCI host bridge");

	ppc_md.pci_exclude_device = yosemite_exclude_device;

	hose->io_space.start = YOSEMITE_PCI_LOWER_IO;
	hose->io_space.end = YOSEMITE_PCI_UPPER_IO;
	hose->mem_space.start = YOSEMITE_PCI_LOWER_MEM;
	hose->mem_space.end = YOSEMITE_PCI_UPPER_MEM;
	isa_io_base =
		(unsigned long)ioremap64(YOSEMITE_PCI_IO_BASE, YOSEMITE_PCI_IO_SIZE);
	hose->io_base_virt = (void *)isa_io_base;

	setup_indirect_pci(hose,
			YOSEMITE_PCI_CFGA_PLB32,
			YOSEMITE_PCI_CFGD_PLB32);
	hose->set_cfg_type = 1;

	/* Zero config bars */
	for (bar = PCI_BASE_ADDRESS_1; bar <= PCI_BASE_ADDRESS_2; bar += 4) {
		early_write_config_dword(hose, hose->first_busno,
					 PCI_FUNC(hose->first_busno), bar,
					 0x00000000);
		early_read_config_dword(hose, hose->first_busno,
					PCI_FUNC(hose->first_busno), bar,
					&bar_response);
	}

	hose->last_busno = pciauto_bus_scan(hose, hose->first_busno);

	ppc_md.pci_swizzle = common_swizzle;
	ppc_md.pci_map_irq = yosemite_map_irq;
}

TODC_ALLOC();

static void __init
yosemite_early_serial_map(void)
{
	struct uart_port port;

	/* Setup ioremapped serial port access */
	memset(&port, 0, sizeof(port));
	port.membase = ioremap64(PPC440EP_UART0_ADDR, 8);
	port.irq = 0;
	port.uartclk = clocks.uart0;
	port.regshift = 0;
	port.iotype = SERIAL_IO_MEM;
	port.flags = ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST;
	port.line = 0;

	if (early_serial_setup(&port) != 0) {
		printk("Early serial init of port 0 failed\n");
	}

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	/* Configure debug serial access */
	gen550_init(0, &port);
#endif

	port.membase = ioremap64(PPC440EP_UART1_ADDR, 8);
	port.irq = 1;
	port.uartclk = clocks.uart1;
	port.line = 1;

	if (early_serial_setup(&port) != 0) {
		printk("Early serial init of port 1 failed\n");
	}

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	/* Configure debug serial access */
	gen550_init(1, &port);
#endif

	port.membase = ioremap64(PPC440EP_UART2_ADDR, 8);
	port.irq = 3;
	port.uartclk = clocks.uart2;
	port.line = 2;

	if (early_serial_setup(&port) != 0) {
		printk("Early serial init of port 2 failed\n");
	}

#if defined(CONFIG_SERIAL_TEXT_DEBUG) || defined(CONFIG_KGDB)
	/* Configure debug serial access */
	gen550_init(2, &port);
#endif

	port.membase = ioremap64(PPC440EP_UART3_ADDR, 8);
	port.irq = 4;
	port.uartclk = clocks.uart3;
	port.line = 3;

	if (early_serial_setup(&port) != 0) {
		printk("Early serial init of port 3 failed\n");
	}
}

static void __init
yosemite_setup_arch(void)
{

	yosemite_set_emacdata();
	
	/*
	 * Determine various clocks.
	 * To be completely correct we should get SysClk
	 * from FPGA, because it can be changed by on-board switches
	 * --ebs
	 */
	ibm440gx_get_clocks(&clocks, bamboo ? BAMBOO_SYSCLK : YOSEMITE_SYSCLK, 6 * 1843200);
	ocp_sys_info.opb_bus_freq = clocks.opb;

	if (bamboo)			/* if bamboo board */
	{
		/* Setup TODC access */
		TODC_INIT(TODC_TYPE_DS1743,
			0,
			0,
			ioremap64(YOSEMITE_RTC_ADDR, YOSEMITE_RTC_SIZE),
			8);
	}

	/* init to some ~sane value until calibrate_delay() runs */
        loops_per_jiffy = 50000000/HZ;

	/* Setup PCI host bridge */
	yosemite_setup_hose();

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start)
		ROOT_DEV = Root_RAM0;
	else
#endif
#ifdef CONFIG_ROOT_NFS
		ROOT_DEV = Root_NFS;
#else
		ROOT_DEV = Root_HDA1;
#endif

	yosemite_early_serial_map();

	/* Identify the system */
	printk("AMCC PowerPC %s %s Platform\n", cpu_name, brd_name);
}

void __init platform_init(unsigned long r3, unsigned long r4,
		unsigned long r5, unsigned long r6, unsigned long r7)
{
	parse_bootinfo(find_bootinfo());

	/*
	 * If we were passed in a board information, copy it into the
	 * residual data area.
	 */
	if (r3)
		__res = *(bd_t *)(r3 + KERNELBASE);

#if defined(CONFIG_BLK_DEV_INITRD)
	/*
	 * If the init RAM disk has been configured in, and there's a valid
	 * starting address for it, set it up.
	 */
	if (r4) {
	    initrd_start = r4 + KERNELBASE;
	    initrd_end = r5 + KERNELBASE;
	}
#endif  /* CONFIG_BLK_DEV_INITRD */

	/* Copy the kernel command line arguments to a safe place. */

	if (r6) {
	    *(char *) (r7 + KERNELBASE) = 0;
	    strcpy(cmd_line, (char *) (r6 + KERNELBASE));
	}

	ibm440gx_get_clocks(&clocks, bamboo ? BAMBOO_SYSCLK : YOSEMITE_SYSCLK, 6 * 1843200);
	ocp_sys_info.opb_bus_freq = clocks.opb;

	ibm44x_platform_init();

	ppc_md.setup_arch = yosemite_setup_arch;
	ppc_md.show_cpuinfo = yosemite_show_cpuinfo;
	ppc_md.get_irq = NULL;		/* Set in ppc4xx_pic_init() */

	ppc_md.calibrate_decr = yosemite_calibrate_decr;

	if (bamboo)
	{
		ppc_md.time_init = todc_time_init;
		ppc_md.set_rtc_time = todc_set_rtc_time;
		ppc_md.get_rtc_time = todc_get_rtc_time;

		ppc_md.nvram_read_val = todc_direct_read_val;
		ppc_md.nvram_write_val = todc_direct_write_val;
	}

#ifdef CONFIG_KGDB
	ppc_md.early_serial_map = yosemite_early_serial_map;
#endif

	SDR_WRITE( 0x320, 1 );		/* little-endian usb */
}

