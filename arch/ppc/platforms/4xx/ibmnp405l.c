/*
 * arch/ppc/platforms/4xx/ibmnp405l.c
 *
 * Author: Armin Kuster <akuster@mvista.com>
 *
 * 2000-2002 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 * Derived from ibmnp405h.c
*/

#include <linux/config.h>
#include <linux/init.h>
#include <asm/ocp.h>
#include <platforms/4xx/ibmnp405l.h>

static struct ocp_func_emac_data ibmnp405l_emac0_def = {
	.rgmii_idx	= -1,		/* No RGMII */
	.rgmii_mux	= -1,		/* No RGMII */
	.zmii_idx	= 0,		/* ZMII device index */
	.zmii_mux	= 0,		/* ZMII input of this EMAC */
	.mal_idx	= 0,		/* MAL device index */
	.mal_rx_chan	= 0,		/* MAL rx channel number */
	.mal_tx_chan	= 0,		/* MAL tx channel number */
	.wol_irq	= 41,		/* WOL interrupt number */
	.mdio_idx	= -1,		/* No shared MDIO */
	.tah_idx	= -1,		/* No TAH */
};

static struct ocp_func_emac_data ibmnp405l_emac1_def = {
	.rgmii_idx	= -1,		/* No RGMII */
	.rgmii_mux	= -1,		/* No RGMII */
	.zmii_idx	= 0,		/* ZMII device index */
	.zmii_mux	= 1,		/* ZMII input of this EMAC */
	.mal_idx	= 0,		/* MAL device index */
	.mal_rx_chan	= 1,		/* MAL rx channel number */
	.mal_tx_chan	= 1,		/* MAL tx channel number */
	.wol_irq	= 41,		/* WOL interrupt number */
	.mdio_idx	= -1,		/* No shared MDIO */
	.tah_idx	= -1,		/* No TAH */
};

OCP_SYSFS_EMAC_DATA()

static struct ocp_func_mal_data ibmnp405l_mal0_def = {
	.num_tx_chans	= 8,		/* Number of TX channels */
	.num_rx_chans	= 4,		/* Number of RX channels */
	.txeob_irq	= 17,		/* TX End Of Buffer IRQ  */
	.rxeob_irq	= 18,		/* RX End Of Buffer IRQ  */
	.txde_irq	= 46,		/* TX Descriptor Error IRQ */
	.rxde_irq	= 47,		/* RX Descriptor Error IRQ */
	.serr_irq	= 45,		/* MAL System Error IRQ    */
};
static struct ocp_func_mal_data ibmnp405l_mal1_def = {
	.num_tx_chans	= 8,		/* Number of TX channels */
	.num_rx_chans	= 4,		/* Number of RX channels */
	.txeob_irq	= 15,		/* TX End Of Buffer IRQ  */
	.rxeob_irq	= 16,		/* RX End Of Buffer IRQ  */
	.txde_irq	= 43,		/* TX Descriptor Error IRQ */
	.rxde_irq	= 44,		/* RX Descriptor Error IRQ */
	.serr_irq	= 42,		/* MAL System Error IRQ    */
};
OCP_SYSFS_MAL_DATA()

static struct ocp_func_iic_data ibmnp405l_iic0_def = {
	.fast_mode	= 0,		/* Use standad mode (100Khz) */
};
OCP_SYSFS_IIC_DATA()

struct ocp_def core_ocp[] = {
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_OPB,
	  .index	= 0,
	  .paddr	= 0xEF600600,
	  .irq		= OCP_IRQ_NA,
	  .pm		= IBM_CPM_OPB,
	},
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_16550,
	  .index	= 0,
	  .paddr	= UART0_IO_BASE,
	  .irq		= UART0_INT,
	  .pm		= IBM_CPM_UART0
	},
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_16550,
	  .index	= 1,
	  .paddr	= UART1_IO_BASE,
	  .irq		= UART1_INT,
	  .pm		= IBM_CPM_UART1
	},
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_IIC,
	  .paddr	= 0xEF600500,
	  .irq		= 2,
	  .pm		= IBM_CPM_IIC0,
	  .additions	= &ibmnp405l_iic0_def,
	  .show		= &ocp_show_iic_data
	},
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_GPIO,
	  .paddr	= 0xEF600700,
	  .irq		= OCP_IRQ_NA,
	  .pm		= IBM_CPM_GPIO0
	},
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_MAL,
	  .paddr	= OCP_PADDR_NA,
	  .irq		= OCP_IRQ_NA,
	  .pm		= OCP_CPM_NA,
	  .additions	= &ibmnp405l_mal0_def,
	  .show		= &ocp_show_mal_data,
	},
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_EMAC,
	  .index	= 0,
	  .paddr	= EMAC0_BASE,
	  .irq		= 37,
	  .pm		= IBM_CPM_EMAC0,
	  .additions	= &ibmnp405l_emac0_def,
	  .show		= &ocp_show_emac_data,
	},
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_EMAC,
	  .index	= 1,
	  .paddr	= 0xEF600900,
	  .irq		= 38,
	  .pm		= IBM_CPM_EMAC1,
	  .additions	= &ibmnp405l_emac1_def,
	  .show		= &ocp_show_emac_data,
	},
	{ .vendor	= OCP_VENDOR_IBM,
	  .function	= OCP_FUNC_ZMII,
	  .paddr	= 0xEF600C10,
	  .irq		= OCP_IRQ_NA,
	  .pm		= OCP_CPM_NA,
	},
	{ .vendor	= OCP_VENDOR_INVALID
	}
};
