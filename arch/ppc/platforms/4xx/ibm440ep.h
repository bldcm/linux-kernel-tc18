/*
 * arch/ppc/platforms/4xx/ibm440ep.h
 *
 * PPC440EP definitions
 *
 * Wade Farnsworth <wfarnsworth@mvista.com>
 *
 * Copyright 2002 Roland Dreier
 * Copyright 2004 MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifdef __KERNEL__
#ifndef __PPC_PLATFORMS_IBM440EP_H
#define __PPC_PLATFORMS_IBM440EP_H

#include <linux/config.h>

/* GPT */
#define PPC440EP_GPT_ADDR               0x0EF600000
#define PPC440EP_GPT_SIZE               0x200
#define GPT_NUMS                        1

/* UART */
#define PPC440EP_UART0_ADDR             0x0EF600300
#define PPC440EP_UART1_ADDR             0x0EF600400
#define PPC440EP_UART2_ADDR             0x0EF600500
#define PPC440EP_UART3_ADDR             0x0EF600600
#define PPC440EP_UART_SIZE              0x08
#define UART_NUMS                       4

/* EMAC */
#define PPC440EP_EMAC0_ADDR             0x0EF600E00
#define PPC440EP_EMAC1_ADDR             0x0EF600F00
#define PPC440EP_EMAC_SIZE              0x100
#define EMAC_NUMS                       2

/* EMAC IRQ's */
#define BL_MAC_WOL      61      /* WOL */
#define BL_MAC_WOL1     63      /* WOL */
#define BL_MAL_SERR     32      /* MAL SERR */
#define BL_MAL_TXDE     33      /* MAL TXDE */
#define BL_MAL_RXDE     34      /* MAL RXDE */
#define BL_MAL_TXEOB    10      /* MAL TX EOB */
#define BL_MAL_RXEOB    11      /* MAL RX EOB */
#define BL_MAC_ETH0     60      /* MAC */
#define BL_MAC_ETH1     62      /* MAC */

/* ZMII */
#define PPC440EP_ZMII_ADDR              0x0EF600D00
#define PPC440EP_ZMII_SIZE              0x10
#define ZMII_NUMS                       1

/* IIC */
#define PPC440EP_IIC0_ADDR              0x0EF600700
#define PPC440EP_IIC1_ADDR              0x0EF600800
#define PPC440EP_IIC_SIZE               0x20
#define IIC0_IRQ                        2
#define IIC1_IRQ                        7
#define IIC_NUMS                        2

/* SPI */
#define PPC440EP_SPI_ADDR               0x0EF600900
#define PPC440EP_SPI_SIZE               0x06
#define SPI_NUMS                        1

/* GPIO */
#define PPC440EP_GPIO0_ADDR             0x0EF600B00
#define PPC440EP_GPIO1_ADDR             0x0EF600C00
#define PPC440EP_GPIO_SIZE              0x80
#define GPIO_NUMS                       2

/* USB1HOST */
#define PPC440EP_USB1HOST_ADDR          0x0EF601000
#define PPC440EP_USB1HOST_SIZE          0x80
#define USB1HOST_IRQ                    40
#define USB1HOST_NUMS                   1

/* USB 1.1 Host constants for usb-ocp-ohci.c */
#define USB0_IRQ                        USB1HOST_IRQ
#define USB0_BASE                       PPC440EP_USB1HOST_ADDR
#define USB0_SIZE                       PPC440EP_USB1HOST_SIZE
#define USB0_EXTENT                     4096

/* NDFC Registers */
#define PPC440EP_NDFC_REG_BASE          0x090000000
#define PPC440EP_NDFC_REG_SIZE          0x2000

/* Clock and Power Management */
#define IBM_CPM_IIC0		0x80000000	/* IIC interface */
#define IBM_CPM_IIC1		0x40000000	/* IIC interface */
#define IBM_CPM_PCI		0x20000000	/* PCI bridge */
#define IBM_CPM_USB1H		0x08000000	/* USB 1.1 Host */
#define IBM_CPM_FPU		0x04000000	/* floating point unit */
#define IBM_CPM_CPU		0x02000000	/* processor core */
#define IBM_CPM_DMA		0x01000000	/* DMA controller */
#define IBM_CPM_BGO		0x00800000	/* PLB to OPB bus arbiter */
#define IBM_CPM_BGI		0x00400000	/* OPB to PLB bridge */
#define IBM_CPM_EBC		0x00200000	/* External Bus Controller */
#define IBM_CPM_EBM		0x00100000	/* Ext Bus Master Interface */
#define IBM_CPM_DMC		0x00080000	/* SDRAM peripheral controller */
#define IBM_CPM_PLB4		0x00040000	/* PLB4 bus arbiter */
#define IBM_CPM_PLB4x3		0x00020000	/* PLB4 to PLB3 bridge controller */
#define IBM_CPM_PLB3x4		0x00010000	/* PLB3 to PLB4 bridge controller */
#define IBM_CPM_PLB3		0x00008000	/* PLB3 bus arbiter */
#define IBM_CPM_PPM		0x00002000	/* PLB Performance Monitor */
#define IBM_CPM_UIC1		0x00001000	/* Universal Interrupt Controller */
#define IBM_CPM_GPIO0		0x00000800	/* General Purpose IO (??) */
#define IBM_CPM_GPT		0x00000400	/* General Purpose Timers  */
#define IBM_CPM_UART0		0x00000200	/* serial port 0 */
#define IBM_CPM_UART1		0x00000100	/* serial port 1 */
#define IBM_CPM_UIC0		0x00000080	/* Universal Interrupt Controller */
#define IBM_CPM_TMRCLK		0x00000040	/* CPU timers */
#define IBM_CPM_EMAC0		0x00000020	/* ethernet port 0 */
#define IBM_CPM_EMAC1		0x00000010	/* ethernet port 1 */
#define IBM_CPM_UART2		0x00000008	/* serial port 2 */
#define IBM_CPM_UART3		0x00000004	/* serial port 3 */
#define IBM_CPM_USB2D		0x00000002	/* USB 2.0 Device */
#define IBM_CPM_USB2H		0x00000001	/* USB 2.0 Host */

#define DFLT_IBM4xx_PM		~(IBM_CPM_UIC0 | IBM_CPM_UIC1 | IBM_CPM_CPU \
				| IBM_CPM_EBC | IBM_CPM_BGO | IBM_CPM_FPU \
				| IBM_CPM_EBM | IBM_CPM_PLB4 | IBM_CPM_3x4 \
				| IBM_CPM_PLB3 | IBM_CPM_PLB4x3 \
				| IBM_CPM_EMAC0 | IBM_CPM_TMRCLK \
				| IBM_CPM_DMA | IBM_CPM_PCI | IBM_CPM_EMAC1)
/*
 * Serial port defines
 */
#define RS_TABLE_SIZE	4

#include <asm/ibm44x.h>
#include <syslib/ibm440ep_common.h>

/*
 * DCRs (the common ones will be defined in ibm44x.h)
 */

/* Base DCR address values for all peripheral cores in the 440EP */

#define CPR0_DCR_BASE           0x00C  /* Clock and Power Reset */
#define SDR0_DCR_BASE           0x00E  /* chip control registers */

/* DMA */

#define MAX_DMA_PLB4_CHANNELS   4

/* Base DCRNs */
#define DCRN_DMA0_PLB4_BASE            0x300      /* DMA to PL4 Channel 0 */
#define DCRN_DMA1_PLB4_BASE            0x308      /* DMA to PL4 Channel 1 */
#define DCRN_DMA2_PLB4_BASE            0x310      /* DMA to PL4 Channel 2 */
#define DCRN_DMA3_PLB4_BASE            0x318      /* DMA to PL4 Channel 3 */
#define DCRN_DMASR_PLB4_BASE            0x320      /* DMA to PL4 status Register */

#define DCRN_DMACR0_PLB4       (DCRN_DMA0_PLB4_BASE + 0x0)     /* DMA Channel Control 0 */
#define DCRN_DMACT0_PLB4        (DCRN_DMA0_PLB4_BASE + 0x1)     /* DMA Count 0 */
#define DCRN_DMASAH0_PLB4       (DCRN_DMA0_PLB4_BASE + 0x2)    /* DMA Src Addr High 0 */
#define DCRN_DMASA0_PLB4        (DCRN_DMA0_PLB4_BASE + 0x3)    /* DMA Src Addr Low 0 */
#define DCRN_DMADAH0_PLB4       (DCRN_DMA0_PLB4_BASE + 0x4)    /* DMA Dest Addr High 0 */
#define DCRN_DMADA0_PLB4       (DCRN_DMA0_PLB4_BASE + 0x5)     /* DMA Dest Addr Low 0 */
#define DCRN_ASGH0_PLB4                (DCRN_DMA0_PLB4_BASE + 0x6)     /* DMA SG Desc Addr High 0 */
#define DCRN_ASG0_PLB4         (DCRN_DMA0_PLB4_BASE + 0x7)     /* DMA SG Desc Addr Low 0 */

#define DCRN_DMACR1_PLB4       (DCRN_DMA1_PLB4_BASE + 0x0)     /* DMA Channel Control 1 */
#define DCRN_DMACT1_PLB4       (DCRN_DMA1_PLB4_BASE + 0x1)     /* DMA Count 1 */
#define DCRN_DMASAH1_PLB4      (DCRN_DMA1_PLB4_BASE + 0x2)     /* DMA Src Addr High 1 */
#define DCRN_DMASA1_PLB4       (DCRN_DMA1_PLB4_BASE + 0x3)     /* DMA Src Addr Low 1 */
#define DCRN_DMADAH1_PLB4      (DCRN_DMA1_PLB4_BASE + 0x4)     /* DMA Dest Addr High 1 */
#define DCRN_DMADA1_PLB4       (DCRN_DMA1_PLB4_BASE + 0x5)     /* DMA Dest Addr Low 1 */
#define DCRN_ASGH1_PLB4                (DCRN_DMA1_PLB4_BASE + 0x6)     /* DMA SG Desc Addr High 1 */
#define DCRN_ASG1_PLB4         (DCRN_DMA1_PLB4_BASE + 0x7)     /* DMA SG Desc Addr Low 1 */

#define DCRN_DMACR2_PLB4       (DCRN_DMA2_PLB4_BASE + 0x0)     /* DMA Channel Control 2 */
#define DCRN_DMACT2_PLB4       (DCRN_DMA2_PLB4_BASE + 0x1)     /* DMA Count 2 */
#define DCRN_DMASAH2_PLB4      (DCRN_DMA2_PLB4_BASE + 0x2)     /* DMA Src Addr High 2 */
#define DCRN_DMASA2_PLB4       (DCRN_DMA2_PLB4_BASE + 0x3)     /* DMA Src Addr Low 2 */
#define DCRN_DMADAH2_PLB4      (DCRN_DMA2_PLB4_BASE + 0x4)     /* DMA Dest Addr High 2 */
#define DCRN_DMADA2_PLB4       (DCRN_DMA2_PLB4_BASE + 0x5)     /* DMA Dest Addr Low 2 */
#define DCRN_ASGH2_PLB4                (DCRN_DMA2_PLB4_BASE + 0x6)     /* DMA SG Desc Addr High 2 */
#define DCRN_ASG2_PLB4         (DCRN_DMA2_PLB4_BASE + 0x7)     /* DMA SG Desc Addr Low 2 */

#define DCRN_DMACR3_PLB4       (DCRN_DMA3_PLB4_BASE + 0x0)     /* DMA Channel Control 3 */
#define DCRN_DMACT3_PLB4       (DCRN_DMA3_PLB4_BASE + 0x1)     /* DMA Count 3 */
#define DCRN_DMASAH3_PLB4      (DCRN_DMA3_PLB4_BASE + 0x2)     /* DMA Src Addr High 3 */
#define DCRN_DMASA3_PLB4       (DCRN_DMA3_PLB4_BASE + 0x3)     /* DMA Src Addr Low 3 */
#define DCRN_DMADAH3_PLB4      (DCRN_DMA3_PLB4_BASE + 0x4)     /* DMA Dest Addr High 3 */
#define DCRN_DMADA3_PLB4       (DCRN_DMA3_PLB4_BASE + 0x5)     /* DMA Dest Addr Low 3 */
#define DCRN_ASGH3_PLB4                (DCRN_DMA3_PLB4_BASE + 0x6)     /* DMA SG Desc Addr High 3 */
#define DCRN_ASG3_PLB4         (DCRN_DMA3_PLB4_BASE + 0x7)     /* DMA SG Desc Addr Low 3 */

#define DCRN_DMASR_PLB4                (DCRN_DMASR_PLB4_BASE + 0x0)    /* DMA Status Register */
#define DCRN_ASGC_PLB4         (DCRN_DMASR_PLB4_BASE + 0x3)    /* DMA Scatter/Gather Command */
#define DCRN_SLP_PLB4          (DCRN_DMASR_PLB4_BASE + 0x5)    /* DMA Sleep Register */
#define DCRN_POL_PLB4          (DCRN_DMASR_PLB4_BASE + 0x6)    /* DMA Polarity Register */

#define DMA_CE_ENABLE_PLB4           0x80000000
#define DMA_CIE_ENABLE_PLB4          0x40000000
#define DMA_TD_PLB4                  0x20000000
#define DMA_PL_PLB4                  0x10000000
#define DMA_PW_WORD                  0x04000000
#define DMA_DAI_PLB4                 0x01000000
#define DMA_SAI_PLB4                 0x00800000
#define DMA_BUFFER_ENABLED_PLB4      0x00400000
#define DMA_MTM_HARDWARE_START_PLB4  0x00300000
#define DMA_TS_IS_OUTPUT_PLB4        0x00000100
#define DMA_STOP_AT_TC_PLB4          0x00000080
#define DMA_PRIORITY_HIGH_PLB4       0x00000060

#define DMA_TCIE_ENABLED_PLB4        0x20000000
#define DMA_ETIE_ENABLED_PLB4        0x10000000
#define DMA_EIE_ENABLED_PLB4         0x08000000
#define DMA_BURST_ENABLED_PLB4       0x00800000
#define DMA_BURST_SIZE_8_PLB4        0x00400000

/* SDR0 */
#define SDR0_USB         0x0320  /* Selection of USB2.0 and USB1.1 Device */

#endif /* __PPC_PLATFORMS_IBM440EP_H */
#endif /* __KERNEL__ */
