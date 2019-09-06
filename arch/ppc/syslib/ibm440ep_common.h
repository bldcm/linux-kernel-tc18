/*
 * arch/ppc/kernel/ibm440ep_common.h
 *
 * PPC440EP system library
 *
 * Wade Farnsworth <wfarnsworth@mvista.com>
 * Copyright 2004 MontaVista Software, Inc.
 * 
 * Eugene Surovegin <eugene.surovegin@zultys.com> or <ebs@ebshome.net>
 * Copyright (c) 2003 Zultys Technologies
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#ifdef __KERNEL__
#ifndef __PPC_SYSLIB_IBM440EP_COMMON_H
#define __PPC_SYSLIB_IBM440EP_COMMON_H

#ifndef __ASSEMBLY__

#include <linux/config.h>
#include <linux/init.h>
#include <syslib/ibm44x_common.h>
#include <asm/mmu.h>

/*
 * Please, refer to the Figure 15.1 in 440EP user manual
 *
 * if internal UART clock is used, ser_clk is ignored
 */
void ibm440ep_get_clocks(struct ibm44x_clocks*, unsigned int sys_clk,
	unsigned int ser_clk) __init;

/*
 * The DMA API are in ibm440ep_common.c
 */
int get_dma2pl4_status(void);
void clear_dma2pl4_status(void);
unsigned long get_dma2pl4_dst_addr(unsigned int dmanr);
void enable_dma2pl4_peripheral_to_memory(unsigned int dmanr,
					 phys_addr_t srcAddr,
					 phys_addr_t destAddr,
					 unsigned int count);
void enable_dma2pl4_memory_to_peripheral(unsigned int dmanr,
		                         phys_addr_t srcAddr,
					 phys_addr_t destAddr,
					 unsigned int count);

#endif /* __ASSEMBLY__ */
#endif /* __PPC_SYSLIB_IBM440EP_COMMON_H */
#endif /* __KERNEL__ */
