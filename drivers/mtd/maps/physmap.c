/*
 * $Id$
 *
 * Normal mappings of chips in physical memory
 *
 * Copyright (C) 2003 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * 031022 - [jsun] add run-time configure and partition setup
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/config.h>
#include <linux/mtd/partitions.h>



static int init_physmap_single_flash(int i);
static void cleanup_physmap_single_flash(int i);

static struct mtd_info *mymtd[2];

struct map_info physmap_map[] = {{
	.name = "phys_mapped_flash",
	.phys = CONFIG_MTD_PHYSMAP_START,
	.size = CONFIG_MTD_PHYSMAP_LEN,
	.bankwidth = CONFIG_MTD_PHYSMAP_BANKWIDTH,
}
#ifdef CONFIG_MTD_PHYSMAP_FLASH2
,{	.name = "phys_mapped_flash2",
	.phys = CONFIG_MTD_PHYSMAP_START_FLASH2,
	.size = CONFIG_MTD_PHYSMAP_LEN_FLASH2,
	.bankwidth = CONFIG_MTD_PHYSMAP_BANKWIDTH_FLASH2
}
#endif
};

#ifdef CONFIG_MTD_PARTITIONS
static struct mtd_partition *mtd_parts[2];
static int                   mtd_parts_nb[2];

static int num_physmap_partitions[2];
static struct mtd_partition *physmap_partitions[2];

static const char *part_probes[] __initdata = {"cmdlinepart", "RedBoot", NULL};

void physmap_set_partitions(struct mtd_partition *parts, int num_parts)
{
	physmap_partitions[0]=parts;
	num_physmap_partitions[0]=num_parts;
}

void physmap_set_partitions_flash2(struct mtd_partition *parts, int num_parts)
{
	physmap_partitions[1]=parts;
	num_physmap_partitions[1]=num_parts;
}
#endif /* CONFIG_MTD_PARTITIONS */

static int __init init_physmap(void)
{
	int ret;

	ret = init_physmap_single_flash(0);
#ifdef CONFIG_MTD_PHYSMAP_FLASH2
	if (ret != 0)
		return ret;
	ret = init_physmap_single_flash(1);
#endif
	return ret;
}

static int init_physmap_single_flash(int i)
{
	static const char *rom_probe_types[] = { "cfi_probe", "jedec_probe", "map_rom", NULL };
	const char **type;

       	printk(KERN_NOTICE "physmap flash device: %lx at %lx\n", physmap_map[i].size, physmap_map[i].phys);
	physmap_map[i].virt = ioremap(physmap_map[i].phys, physmap_map[i].size);

	if (!physmap_map[i].virt) {
		printk("Failed to ioremap\n");
		return -EIO;
	}

	simple_map_init(&physmap_map[i]);

	mymtd[i] = NULL;
	type = rom_probe_types;
	for(; !mymtd[i] && *type; type++) {
		mymtd[i] = do_map_probe(*type, &physmap_map[i]);
	}
	if (mymtd[i]) {
		mymtd[i]->owner = THIS_MODULE;

#ifdef CONFIG_MTD_PARTITIONS
		mtd_parts_nb[i] = parse_mtd_partitions(mymtd[i], part_probes, 
						    &mtd_parts[i], 0);

		if (mtd_parts_nb[i] > 0)
		{
			add_mtd_partitions (mymtd[i], mtd_parts[i], mtd_parts_nb[i]);
			return 0;
		}

		if (num_physmap_partitions[i] != 0) 
		{
			printk(KERN_NOTICE 
			       "Using physmap partition definition\n");
			add_mtd_partitions (mymtd[i], physmap_partitions[i], num_physmap_partitions[i]);
			return 0;
		}

#endif
		add_mtd_device(mymtd[i]);

		return 0;
	}

	iounmap(physmap_map[i].virt);
	return -ENXIO;
}

static void __exit cleanup_physmap(void)
{
	cleanup_physmap_single_flash(0);
#ifdef CONFIG_MTD_PHYSMAP_FLASH2
	cleanup_physmap_single_flash(1);
#endif
}


static void cleanup_physmap_single_flash(int i)
{
#ifdef CONFIG_MTD_PARTITIONS
	if (mtd_parts_nb[i]) {
		del_mtd_partitions(mymtd[i]);
		kfree(mtd_parts[i]);
	} else if (num_physmap_partitions[i]) {
		del_mtd_partitions(mymtd[i]);
	} else {
		del_mtd_device(mymtd[i]);
	}
#else
	del_mtd_device(mymtd[i]);
#endif
	map_destroy(mymtd[i]);

	iounmap(physmap_map[i].virt);
	physmap_map[i].virt = NULL;
}

module_init(init_physmap);
module_exit(cleanup_physmap);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Woodhouse <dwmw2@infradead.org>");
MODULE_DESCRIPTION("Generic configurable MTD map driver");
