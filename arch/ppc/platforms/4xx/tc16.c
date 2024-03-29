/*
 * arch/ppc/platforms/4xx/ash.c
 *
 * Support for the IBM NP405H ash eval board
 *
 * Author: Armin Kuster <akuster@mvista.com>
 *
 * 2001-2002 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/pagemap.h>
#include <linux/pci.h>

#include <asm/machdep.h>
#include <asm/pci-bridge.h>
#include <asm/io.h>
#include <asm/ocp.h>
#include <asm/ibm_ocp_pci.h>
#include <asm/todc.h>
#include <asm/ppcboot.h>


/*
 * This is a horrible kludge, we eventually need to abstract this
 * generic PHY stuff, so the  standard phy mode defines can be
 * easily used from arch code.
 */
#include "../../../../drivers/net/ibm_emac/ibm_emac_phy.h"


#ifdef DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

void *tc16_rtc_base;

/* Some IRQs unique to Walnut.
 * Used by the generic 405 PCI setup functions in ppc4xx_pci.c
 */
int __init
ppc405_map_irq(struct pci_dev *dev, unsigned char idsel, unsigned char pin)
{
	static char pci_irq_table[][4] =
	    /*
	     *      PCI IDSEL/INTPIN->INTLINE
	     *      A       B       C       D
	     */
	{
		{24, 24, 24, 24},	/* IDSEL 1 - PCI slot 1 */
		{25, 25, 25, 25},	/* IDSEL 2 - PCI slot 2 */
		{26, 26, 26, 26},	/* IDSEL 3 - PCI slot 3 */
		{27, 27, 27, 27},	/* IDSEL 4 - PCI slot 4 */
	};

	const long min_idsel = 1, max_idsel = 4, irqs_per_slot = 4;
	return PCI_IRQ_TABLE_LOOKUP;
}

static void __init tc16_set_emacdata(void)
{
	struct ocp_def *def;
	struct ocp_func_emac_data *emacdata;
	int mode = PHY_MODE_RMII;	/* default to RMII for yosemite */


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

void __init
tc16_setup_arch(void)
{
if ( ppc_md.progress ) ppc_md.progress("tc16_arch: start", 0x3eab);
	ppc4xx_setup_arch();

	tc16_set_emacdata();

#ifdef CONFIG_DEBUG_BRINGUP
	int i;
	printk("\n");
	printk("machine\t: %s\n", PPC4xx_MACHINE_NAME);
	printk("\n");
	printk("bi_s_version\t %s\n", bip->bi_s_version);
	printk("bi_r_version\t %s\n", bip->bi_r_version);
	printk("bi_memsize\t 0x%8.8x\t %dMBytes\n", bip->bi_memsize,
	       bip->bi_memsize / (1024 * 1000));
	for (i = 0; i < EMAC_NUMS; i++) {
		printk("bi_enetaddr %d\t %2.2x%2.2x%2.2x-%2.2x%2.2x%2.2x\n", i,
		       bip->bi_enetaddr[i][0], bip->bi_enetaddr[i][1],
		       bip->bi_enetaddr[i][2], bip->bi_enetaddr[i][3],
		       bip->bi_enetaddr[i][4], bip->bi_enetaddr[i][5]);
	}
	printk("bi_pci_enetaddr %d\t %2.2x%2.2x%2.2x-%2.2x%2.2x%2.2x\n", 0,
	       bip->bi_pci_enetaddr[0], bip->bi_pci_enetaddr[1],
	       bip->bi_pci_enetaddr[2], bip->bi_pci_enetaddr[3],
	       bip->bi_pci_enetaddr[4], bip->bi_pci_enetaddr[5]);

	printk("bi_intfreq\t 0x%8.8x\t clock:\t %dMhz\n",
	       bip->bi_intfreq, bip->bi_intfreq / 1000000);

	printk("bi_busfreq\t 0x%8.8x\t plb bus clock:\t %dMHz\n",
	       bip->bi_busfreq, bip->bi_busfreq / 1000000);
	printk("bi_pci_busfreq\t 0x%8.8x\t pci bus clock:\t %dMHz\n",
	       bip->bi_pci_busfreq, bip->bi_pci_busfreq / 1000000);

	printk("\n");


#endif
	unsigned long ebc_cfg;

#ifdef CONFIG_DSP_HANG

	printk ("Enabling TCLS1 PerReady workaround for DSP hangs.\n");

	mtdcr (DCRN_EBCCFGADR, 0x23);
	ebc_cfg = mfdcr (DCRN_EBCCFGDATA);

	ebc_cfg &= ~0x40000000;	/* enable per ready timeouts */
	ebc_cfg |= 0x38000000; /* set timeout to approx. 82 usec */

	mtdcr (DCRN_EBCCFGADR, 0x23);
	mtdcr (DCRN_EBCCFGDATA, ebc_cfg);
ppc_md.setup_arch
#else 
	mtdcr (DCRN_EBCCFGADR, 0x23);
	ebc_cfg = mfdcr (DCRN_EBCCFGDATA);

	printk ("PerReady workaround NOT enabled, please reconfigure kernel if you want that.\n");
#endif
	printk ("EBC Configuration register: %lx\n", ebc_cfg);



	/* RTC step for ash */
	//tc16_rtc_base = (void *) TC16_RTC_VADDR;
	//TODC_INIT(TODC_TYPE_DS1743, tc16_rtc_base, tc16_rtc_base, tc16_rtc_base,
	//	  8);
}

void __init
bios_fixup(struct pci_controller *hose, struct pcil0_regs *pcip)
{
#ifdef CONFIG_PCI	
/*
	 * Expected PCI mapping:
	 *
	 *  PLB addr             PCI memory addr
	 *  ---------------------       ---------------------
	 *  0000'0000 - 7fff'ffff <---  0000'0000 - 7fff'ffff
	 *  8000'0000 - Bfff'ffff --->  8000'0000 - Bfff'ffff
	 *
	 *  PLB addr             PCI io addr
	 *  ---------------------       ---------------------
	 *  e800'0000 - e800'ffff --->  0000'0000 - 0001'0000
	 *
	 * The following code is simplified by assuming that the bootrom
	 * has been well behaved in following this mapping.
	 */

#ifdef DEBUG
	int i;

	printk("ioremap PCLIO_BASE = 0x%x\n", pcip);
	printk("PCI bridge regs before fixup \n");
	for (i = 0; i <= 2; i++) {
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].ma)));
		printk(" pmm%dla\t0x%x\n", i, in_le32(&(pcip->pmm[i].la)));
		printk(" pmm%dpcila\t0x%x\n", i,
		       in_le32(&(pcip->pmm[i].pcila)));
		printk(" pmm%dpciha\t0x%x\n", i,
		       in_le32(&(pcip->pmm[i].pciha)));
	}
	printk(" ptm1ms\t0x%x\n", in_le32(&(pcip->ptm1ms)));
	printk(" ptm1la\t0x%x\n", in_le32(&(pcip->ptm1la)));
	printk(" ptm2ms\t0x%x\n", in_le32(&(pcip->ptm2ms)));
	printk(" ptm2la\t0x%x\n", in_le32(&(pcip->ptm2la)));
	for (bar = PCI_BASE_ADDRESS_1; bar <= PCI_BASE_ADDRESS_2; bar += 4) {
		early_read_config_dword(hose, hose->first_busno,
					PCI_FUNC(hose->first_busno), bar,
					&bar_response);
		DBG("BUS %d, device %d, Function %d bar 0x%8.8x is 0x%8.8x\n",
		    hose->first_busno, PCI_SLOT(hose->first_busno),
		    PCI_FUNC(hose->first_busno), bar, bar_response);
	}

#endif
	if (ppc_md.progress)
		ppc_md.progress("bios_fixup(): enter", 0x800);

	/* added for IBM boot rom version 1.15 bios bar changes  -AK */

	/* Disable region first */
	out_le32((void *) &(pcip->pmm[0].ma), 0x00000000);
	/* PLB starting addr, PCI: 0x80000000 */
	out_le32((void *) &(pcip->pmm[0].la), 0x80000000);
	/* PCI start addr, 0x80000000 */
	out_le32((void *) &(pcip->pmm[0].pcila), PPC405_PCI_MEM_BASE);
	/* 512MB range of PLB to PCI */
	out_le32((void *) &(pcip->pmm[0].pciha), 0x00000000);
	/* Enable no pre-fetch, enable region */
	out_le32((void *) &(pcip->pmm[0].ma), ((0xffffffff -
						(PPC405_PCI_UPPER_MEM -
						 PPC405_PCI_MEM_BASE)) | 0x01));

	/* Disable region one */
	out_le32((void *) &(pcip->pmm[1].ma), 0x00000000);
	out_le32((void *) &(pcip->pmm[1].la), 0x00000000);
	out_le32((void *) &(pcip->pmm[1].pcila), 0x00000000);
	out_le32((void *) &(pcip->pmm[1].pciha), 0x00000000);
	out_le32((void *) &(pcip->pmm[1].ma), 0x00000000);

	/* Disable region two */
	out_le32((void *) &(pcip->pmm[2].ma), 0x00000000);
	out_le32((void *) &(pcip->pmm[2].la), 0x00000000);
	out_le32((void *) &(pcip->pmm[2].pcila), 0x00000000);
	out_le32((void *) &(pcip->pmm[2].pciha), 0x00000000);
	out_le32((void *) &(pcip->pmm[2].ma), 0x00000000);

	/* Enable PTM1 and PTM2, mapped to PLB address 0. */

	out_le32((void *) &(pcip->ptm1la), 0x00000000);
	out_le32((void *) &(pcip->ptm1ms), 0x00000001);
	out_le32((void *) &(pcip->ptm2la), 0x00000000);
	out_le32((void *) &(pcip->ptm2ms), 0x00000001);

	/* Write zero to PTM1 BAR. */

	early_write_config_dword(hose, hose->first_busno,
				 PCI_FUNC(hose->first_busno),
				 PCI_BASE_ADDRESS_1,
				 0x00000000);

	/* Disable PTM2 (unused) */

	out_le32((void *) &(pcip->ptm2la), 0x00000000);
	out_le32((void *) &(pcip->ptm2ms), 0x00000000);

	/* end work arround */
	if (ppc_md.progress)
		ppc_md.progress("bios_fixup(): done", 0x800);

#ifdef DEBUG
	printk("PCI bridge regs after fixup \n");
	for (i = 0; i <= 2; i++) {
		printk(" pmm%dma\t0x%x\n", i, in_le32(&(pcip->pmm[i].ma)));
		printk(" pmm%dla\t0x%x\n", i, in_le32(&(pcip->pmm[i].la)));
		printk(" pmm%dpcila\t0x%x\n", i,
		       in_le32(&(pcip->pmm[i].pcila)));
		printk(" pmm%dpciha\t0x%x\n", i,
		       in_le32(&(pcip->pmm[i].pciha)));
	}
	printk(" ptm1ms\t0x%x\n", in_le32(&(pcip->ptm1ms)));
	printk(" ptm1la\t0x%x\n", in_le32(&(pcip->ptm1la)));
	printk(" ptm2ms\t0x%x\n", in_le32(&(pcip->ptm2ms)));
	printk(" ptm2la\t0x%x\n", in_le32(&(pcip->ptm2la)));

	for (bar = PCI_BASE_ADDRESS_1; bar <= PCI_BASE_ADDRESS_2; bar += 4) {
		early_read_config_dword(hose, hose->first_busno,
					PCI_FUNC(hose->first_busno), bar,
					&bar_response);
		DBG("BUS %d, device %d, Function %d bar 0x%8.8x is 0x%8.8x\n",
		    hose->first_busno, PCI_SLOT(hose->first_busno),
		    PCI_FUNC(hose->first_busno), bar, bar_response);
	}


#endif
#endif
}

void __init
tc16_map_io(void)
{
	ppc4xx_map_io();
//	io_block_mapping(TC6_RTC_VADDR, TC16_RTC_PADDR, TC16_RTC_SIZE, _PAGE_IO);
}

void __init
platform_init(unsigned long r3, unsigned long r4, unsigned long r5,
	      unsigned long r6, unsigned long r7)
{
	ppc4xx_init(r3, r4, r5, r6, r7);

	ppc_md.setup_arch = tc16_setup_arch;
	ppc_md.setup_io_mappings = tc16_map_io;

#ifdef CONFIG_PPC_RTC
	ppc_md.time_init = todc_time_init;
	ppc_md.set_rtc_time = todc_set_rtc_time;
	ppc_md.get_rtc_time = todc_get_rtc_time;
	ppc_md.nvram_read_val = todc_direct_read_val;
	ppc_md.nvram_write_val = todc_direct_write_val;
#endif
}
