/*
 *	tcls1-rev1.c - mapper for TOPCALL Line Server ONE, Board revision 1
 *      On this board, flash lines are byte reversed. At least they are 
 *      not bit-reversed any more ;) 
 *
 * Copyright (C) 2000 Crossnet Co. <info@crossnet.co.jp>
 *
 * This code is GPL
 *
 * $Id$
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/byteorder.h>
#include <asm/io.h>


/* There's a hardware bug in revision one boards (data lines are
   reversed such that bytes are swapped). The r/w routines take this
   into account .. */

#define WINDOW_ADDR 0x40000000
#define WINDOW_SIZE 0x01000000

/* 
 * MAP DRIVER STUFF
 */

__u8 tcls1_rev1_read8(struct map_info *map, unsigned long ofs)
{
  ofs ^= 1;
  return *(__u8 *)(map->map_priv_1 + ofs);
}

map_word tcls1_rev1_read16(struct map_info *map, unsigned long ofs)
{
  __u16 r;
  map_word val;

  r = *(__u16 *)(map->map_priv_1 + ofs);
  r = le16_to_cpu (r);
	val.x[0]=r;
  return val;
}

__u32 tcls1_rev1_read32(struct map_info *map, unsigned long ofs)
{  // ?? not used .. 
  __u32 r;
  r = *(volatile unsigned int *)(map->map_priv_1 + ofs);
  return r;
  // return *(volatile unsigned int *)(map->map_priv_1 + ofs);
}

void tcls1_rev1_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
// printk ("memcpy from %lx to %p, len %lx\n", from, to, len);
  memcpy(to, (void *)(map->map_priv_1 + from), len);
}

void tcls1_rev1_write8(struct map_info *map, __u8 d, unsigned long adr)
{
  adr ^= 1;
  *(__u8 *)(map->map_priv_1 + adr) = d;
}

void tcls1_rev1_write16(struct map_info *map, map_word d, unsigned long adr)
{
  d.x[0] = cpu_to_le16 (d.x[0]);
  *(__u16 *)(map->map_priv_1 + adr) = d.x[0];
}

void tcls1_rev1_write32(struct map_info *map, __u32 d, unsigned long adr)
{  // ?? not used ..
  *(__u32 *)(map->map_priv_1 + adr) = d;
}

void tcls1_rev1_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
// printk ("memcpy %p to %lx, len %lx\n", from, to, len);
  memcpy((void *)(map->map_priv_1 + to), from, len);
}

struct map_info tcls1_rev1_map = {
	name: "TCLS1-rev1",
	size: WINDOW_SIZE,
	bankwidth: 2,
	//read: tcls1_rev1_read8,
	read: tcls1_rev1_read16,
	//read: tcls1_rev1_read32,
	copy_from: tcls1_rev1_copy_from,
	//write: tcls1_rev1_write8,
	write: tcls1_rev1_write16,
	//write: tcls1_rev1_write32,
	copy_to: tcls1_rev1_copy_to
};


/*
 * MTD 'PARTITIONING' STUFF 
 */
/* ????? depends on what board info says .. 
  ppc405_get_boardinfo (BI_FLASHOFFSET, &x)  and the like  
   ???? Only PPCBoot config data works correctly.
   */ 
static struct mtd_partition tcls1_rev1_partitions[4][3] = {
{    /* 16MBit ST flashes */
	{
		name: "PPCBoot Config data and Kernel (ST 16MBit)",
		size: 0x100000,
		offset: 0x0,
	},
	{
		name: "Reserved for root fs",
		size: 0x100000,
		offset: 0x700000
	},
	{
		name: "Reserved for r/w JFFS filesystem",
		size: 0x200000,
		offset: 0x200000
	}
}, {  /* 32MBit Intel flashes, Top boot sector  */
	{
		name: "PPCBoot Config data (Intel)",
		size: 0x2000,
		offset: 0x7fe000
	},
	{
		name: "Reserved for root fs",
		size: 0x100000,
		offset: 0x100000
	},
	{
		name: "Reserved for r/w JFFS filesystem",
		size: 0x200000,
		offset: 0x200000
	}
}, {    /* 32MBit ST flashes */
	{
		name: "PPCBoot Config data and Kernel (ST 32MBit)",
		size: 0x100000,
		offset: 0x0,
	},
	{
		name: "Reserved for root fs",
		size: 0x200000,
		offset: 0x300000
	},
	{
		name: "Reserved for r/w JFFS filesystem",
		size: 0x300000,
		offset: 0x500000
	}
}, {    /* 64MBit ST flashes */
	{
		name: "PPCBoot Config data and Kernel (ST 64MBit)",
		size: 0x100000,
		offset: 0x0,
	},
	{
		name: "Root filesystem / Application image",
		size: 	0x00200000,
		offset: 0x00100000
	},
	{
		name: "Data: JFFS filesystem",
		size: 	0x00D00000,
		offset: 0x00300000
	}
}};

/* 
 * This is the master MTD device for which all the others are just
 * auto-relocating aliases.
 */
static struct mtd_info *mymtd;

/* These flash chips are known to date for TOPCALL Line Server ONE boards.
   To add some: add constant here, add entry in partition table above and 
   write some mechanism to probe for the flash. Good luck!         -joth */ 

#define ST_16MBIT_FLASH  0
#define INTEL_FLASH      1
#define ST_32MBIT_FLASH  2
#define ST_64MBIT_FLASH  3

int __init init_tcls1_rev1_2000(void)
{
	int flash_type;

	printk(KERN_NOTICE "TOPCALL Line Server ONE (board revision 1) flash mapping: %x at %x\n", WINDOW_SIZE, WINDOW_ADDR);
        tcls1_rev1_map.map_priv_1 = (unsigned long)ioremap(WINDOW_ADDR, WINDOW_SIZE);
        if (!tcls1_rev1_map.map_priv_1) {
                printk("Failed to ioremap\n");
                return -EIO;
        }

	mymtd = do_map_probe("amd_flash", &tcls1_rev1_map);
	if (!mymtd) {
		mymtd = do_map_probe("cfi_probe", &tcls1_rev1_map); 
		flash_type = INTEL_FLASH;
	} else {
		switch (mymtd->size) {
		case 0x00400000: flash_type = ST_16MBIT_FLASH; break;
		case 0x00800000: flash_type = ST_32MBIT_FLASH; break;
		case 0x01000000: flash_type = ST_64MBIT_FLASH; break;
		default: 
			printk ("Warning: ST flash of unknown size (%x) detected. Assuming bottom boot block.\n", mymtd->size);
			flash_type = ST_32MBIT_FLASH; 
		}
	}

	if (mymtd) {
		//mymtd->module = THIS_MODULE;
		return add_mtd_partitions(mymtd, tcls1_rev1_partitions[flash_type], 3);
	} else {
		printk ("map-probe returned NULL\n");
	}
        iounmap((void *)tcls1_rev1_map.map_priv_1);
	return -ENXIO;
}

static void __exit cleanup_tcls1_rev1_2000(void)
{

	if (tcls1_rev1_map.map_priv_1) {
	        iounmap((void *)tcls1_rev1_map.map_priv_1);
		tcls1_rev1_map.map_priv_1 = 0;
	}

	if (mymtd) {
		del_mtd_partitions(mymtd);
		map_destroy(mymtd);
	}
}

module_init(init_tcls1_rev1_2000);
module_exit(cleanup_tcls1_rev1_2000);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Johannes Thoma (TOPCALL, Intl.) <joe@mond.at>");
MODULE_DESCRIPTION("MTD map driver for TOPCALL Line Server ONE, rev1 board");
