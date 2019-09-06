/*
 * arch/ppc/kernel/ibm440ep_common.c
 *
 * PPC440EP system library
 *
 * Wade Farnsworth <wfarnsworth@mvista.com>
 * Copyright 2004 MontaVista Software, Inc.
 * 
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/ibm44x.h>
#include <asm/mmu.h>
#include <asm/processor.h>
#include <syslib/ibm440ep_common.h>
#include <linux/module.h>

/* DMA functions reserved for DMA to PLB4 (used for USB2.0 Device) */

/*
 *  * Clear DMA Status Register (DMA2P40_SR)
 *   */
void clear_dma2pl4_status(void)
{
	mtdcr(DCRN_DMASR_PLB4, 0xffffffff);
}

/*
 *  * Get the DMA to PLB4 status Register (DMA2P40_SR)
 *   */
int get_dma2pl4_status(void)
{
	return (mfdcr(DCRN_DMASR_PLB4));
}

/*
 *  * Get the DMA to PLB4 ADDRESS DEST (DMA2P40_DA)
 *   */
unsigned long get_dma2pl4_dst_addr(unsigned int dmanr)
{
	unsigned long dst_addr;

	switch (dmanr) {
	case 0:
		dst_addr = mfdcr(DCRN_DMADA0_PLB4);
		break;

	case 1:
		dst_addr = mfdcr(DCRN_DMADA1_PLB4);
		break;

	case 2:
		dst_addr = mfdcr(DCRN_DMADA2_PLB4);
		break;

	case 3:
		dst_addr = mfdcr(DCRN_DMADA3_PLB4);
		break;

	default:
		dst_addr = 0;
		if (dmanr >= MAX_DMA_PLB4_CHANNELS)
			printk("get_dma2pl4_dst_addr: bad channel: %d\n", dmanr);
	}
	return dst_addr;
}

/*
 * Set the source address
 */
void set_src_addr_dma2pl4(unsigned int dmanr, phys_addr_t src_addr)
{
	switch (dmanr) {

	case 0:
		mtdcr(DCRN_DMASAH0_PLB4, (u32)(src_addr >> 32));
		mtdcr(DCRN_DMASA0_PLB4, (u32)src_addr);
		break;

	case 1:
		mtdcr(DCRN_DMASAH1_PLB4, (u32)(src_addr >> 32));
		mtdcr(DCRN_DMASA1_PLB4, (u32)src_addr);
		break;

	case 2:
		mtdcr(DCRN_DMASAH2_PLB4, (u32)(src_addr >> 32));
		mtdcr(DCRN_DMASA2_PLB4, (u32)src_addr);
		break;

	case 3:
		mtdcr(DCRN_DMASAH3_PLB4, (u32)(src_addr >> 32));
		mtdcr(DCRN_DMASA3_PLB4, (u32)src_addr);
		break;

	default:
		if (dmanr >= MAX_DMA_PLB4_CHANNELS)
			printk("set_src_addr_dma2pl4: bad channel: %d\n", dmanr);
	}
}

/*
 * Set the destimation address
 */
void set_dst_addr_dma2pl4(unsigned int dmanr, phys_addr_t dst_addr)
{
	switch (dmanr) {
	case 0:
		mtdcr(DCRN_DMADAH0_PLB4, (u32)(dst_addr >> 32));
		mtdcr(DCRN_DMADA0_PLB4, (u32)dst_addr);
		break;
	case 1:
		mtdcr(DCRN_DMADAH1_PLB4, (u32)(dst_addr >> 32));
		mtdcr(DCRN_DMADA1_PLB4, (u32)dst_addr);
		break;
	case 2:
		mtdcr(DCRN_DMADAH2_PLB4, (u32)(dst_addr >> 32));
		mtdcr(DCRN_DMADA2_PLB4, (u32)dst_addr);
		break;
	case 3:
		mtdcr(DCRN_DMADAH3_PLB4, (u32)(dst_addr >> 32));
		mtdcr(DCRN_DMADA3_PLB4, (u32)dst_addr);
		break;
	default:
		if (dmanr >= MAX_DMA_PLB4_CHANNELS)
			printk("set_dst_addr_dma2pl4: bad channel: %d\n", dmanr);
	}
}

/*
 * Enable the DMA to PLB4 Peripheral to Memory
 */
void enable_dma2pl4_peripheral_to_memory(unsigned int dmanr,
					 phys_addr_t srcAddr,
					 phys_addr_t destAddr,
					 unsigned int count)
{
	unsigned int control = 0x00;

	control |= DMA_CIE_ENABLE_PLB4;         /* Channel Interrupt Enable */
	control |= DMA_TD_PLB4;                 /* Transfers are from peripheral-to-memory */
	control |= DMA_PL_PLB4;                 /* Device located on the OPB */
	control |= DMA_PW_WORD;                 /* Peripheral Width (32 bits) */
	control |= DMA_DAI_PLB4;                /* Destination Address Increment */
	control |= 0x00;                        /* Do not increment Source Address */
	control |= DMA_BUFFER_ENABLED_PLB4;     /* Enable DMA Buffer */
	control |= DMA_MTM_HARDWARE_START_PLB4; /* Transfert mode: Device spaced memory-to-memory */
	control |= 0x00;                        /* Peripheral Setup Cycles:000 */
	control |= DMA_TS_IS_OUTPUT_PLB4;       /* End of transfert Terminal/Count */
	control |= DMA_STOP_AT_TC_PLB4;         /* Stop at TC */
	control |= DMA_PRIORITY_HIGH_PLB4;      /* Channel priority High */

	switch (dmanr) {
	case 0:
		mtdcr(DCRN_DMACR0_PLB4, control);
		break;
	case 1:
		mtdcr(DCRN_DMACR1_PLB4, control);
		break;
	case 2:
		mtdcr(DCRN_DMACR2_PLB4, control);
		break;
	case 3:
		mtdcr(DCRN_DMACR3_PLB4, control);
		break;
	default:
		printk("enable_dma: bad channel: %d\n", dmanr);
	}

	/*
 	 * Clear the CS, TS, RI bits for the channel from DMASR.  This
 	 * has been observed to happen correctly only after the mode and
	 * ETD/DCE bits in DMACRx are set above.  Must do this before
	 * enabling the channel.
	 */
	mtdcr(DCRN_DMASR_PLB4, 0xffffffff);

	/* peripheral to memory */
	set_src_addr_dma2pl4(dmanr, srcAddr);
	set_dst_addr_dma2pl4(dmanr, destAddr);

	count |= DMA_TCIE_ENABLED_PLB4;
	count |= DMA_ETIE_ENABLED_PLB4;
	count |= DMA_EIE_ENABLED_PLB4;
	count |= DMA_BURST_ENABLED_PLB4;
	count |= DMA_BURST_SIZE_8_PLB4;

	/* Set the number of bytes to transfer */
	switch (dmanr) {
	case 0:
		mtdcr(DCRN_DMACT0_PLB4, count);
		break;
	case 1:
		mtdcr(DCRN_DMACT1_PLB4, count);
		break;
	case 2:
		mtdcr(DCRN_DMACT2_PLB4, count);
		break;
	case 3:
		mtdcr(DCRN_DMACT3_PLB4, count);
		break;
	default:
		printk("enable_dma: bad channel: %d\n", dmanr);
	}

	/*
	 * Now enable the channel.
	 */
	control |= DMA_CE_ENABLE_PLB4;

	switch (dmanr) {
	case 0:
		mtdcr(DCRN_DMACR0_PLB4, control);
		break;
	case 1:
		mtdcr(DCRN_DMACR1_PLB4, control);
		break;
	case 2:
		mtdcr(DCRN_DMACR2_PLB4, control);
		break;
	case 3:
		mtdcr(DCRN_DMACR3_PLB4, control);
		break;
	default:
		printk("enable_dma: bad channel: %d\n", dmanr);
	}
}

/*
 *  * Enable the DMA to PLB4 Memory to Peripheral
 *   */
void enable_dma2pl4_memory_to_peripheral(unsigned int dmanr,
					 phys_addr_t srcAddr,
					 phys_addr_t destAddr,
					 unsigned int count)
{
	unsigned int control = 0x00;

	control |= DMA_CIE_ENABLE_PLB4;         /* Channel Interrupt Enable */
	control |= 0x00;                        /* Transfers are from memory_to_peripheral */
	control |= DMA_PL_PLB4;                 /* Device located on the OPB */
	control |= DMA_PW_WORD;                 /* Peripheral Width (32 bits) */
	control |= 0x00;                        /* Do not increment Destination Address */
	control |= DMA_SAI_PLB4;                /* Source Address Increment */
	control |= DMA_BUFFER_ENABLED_PLB4;     /* Enable DMA Buffer */
	control |= DMA_MTM_HARDWARE_START_PLB4; /* Transfert mode: Device spaced memory-to-memory */
	control |= 0x00;                        /* Peripheral Setup Cycles:000 */
	control |= DMA_TS_IS_OUTPUT_PLB4;       /* End of transfert Terminal/Count */
	control |= DMA_STOP_AT_TC_PLB4;         /* Stop at TC */
	control |= DMA_PRIORITY_HIGH_PLB4;      /* Channel priority High */
	switch (dmanr) {
	case 0:
		mtdcr(DCRN_DMACR0_PLB4, control);
		break;
	case 1:
		mtdcr(DCRN_DMACR1_PLB4, control);
		break;
	case 2:
		mtdcr(DCRN_DMACR2_PLB4, control);
		break;
	case 3:
		mtdcr(DCRN_DMACR3_PLB4, control);
		break;
	default:
		printk("enable_dma: bad channel: %d\n", dmanr);
	}
	/*
	 * Clear the CS, TS, RI bits for the channel from DMASR.  This
	 * has been observed to happen correctly only after the mode and
	 * ETD/DCE bits in DMACRx are set above.  Must do this before
	 * enabling the channel.
	 */
	mtdcr(DCRN_DMASR_PLB4, 0xffffffff);

	/* peripheral to memory */
	set_src_addr_dma2pl4(dmanr, srcAddr);
	set_dst_addr_dma2pl4(dmanr, destAddr);
	count |= DMA_TCIE_ENABLED_PLB4;
	count |= DMA_ETIE_ENABLED_PLB4;
	count |= DMA_EIE_ENABLED_PLB4;
	count |= DMA_BURST_ENABLED_PLB4;
	count |= DMA_BURST_SIZE_8_PLB4;

	/* Set the number of bytes to transfer */
	switch (dmanr) {
	case 0:
		mtdcr(DCRN_DMACT0_PLB4, count);
		break;
	case 1:
		mtdcr(DCRN_DMACT1_PLB4, count);
		break;
	case 2:
		mtdcr(DCRN_DMACT2_PLB4, count);
		break;
	case 3:
		mtdcr(DCRN_DMACT3_PLB4, count);
		break;
	default:
		printk("enable_dma: bad channel: %d\n", dmanr);
	}

	/*
	 * Now enable the channel.
	 */
	control |= DMA_CE_ENABLE_PLB4;

	switch (dmanr) {
	case 0:
		mtdcr(DCRN_DMACR0_PLB4, control);
		break;
	case 1:
		mtdcr(DCRN_DMACR1_PLB4, control);
		break;
	case 2:
		mtdcr(DCRN_DMACR2_PLB4, control);
		break;
	case 3:
		mtdcr(DCRN_DMACR3_PLB4, control);
		break;
	default:
		printk("enable_dma: bad channel: %d\n", dmanr);
	}
}

EXPORT_SYMBOL(get_dma2pl4_status);
EXPORT_SYMBOL(clear_dma2pl4_status);
EXPORT_SYMBOL(get_dma2pl4_dst_addr);
EXPORT_SYMBOL(enable_dma2pl4_peripheral_to_memory);
EXPORT_SYMBOL(enable_dma2pl4_memory_to_peripheral);
