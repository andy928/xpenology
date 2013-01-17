/*
 * drivers/mtd/maps/synomap.c
 *
 * MTD mapping driver for Synology boards
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/io.h>
#if !defined(CONFIG_SYNO_MV_COMMON) && !defined(CONFIG_SYNO_MPC85XX_COMMON)
#include <asm-ppc/ppcboot.h>
#else
#include <linux/platform_device.h>
#endif

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include "../mtdcore.h"

#define FLASH_BANK_MAX	1

extern unsigned char __res[];

/* trivial struct to describe partition information */
struct mtd_part_def
{
	int nums;
	unsigned char *type;
	struct mtd_partition* mtd_part;
};

static struct mtd_info* mtd_banks[FLASH_BANK_MAX];
static struct map_info* map_banks[FLASH_BANK_MAX];
static struct mtd_part_def part_banks[FLASH_BANK_MAX];

static unsigned long num_banks;
static unsigned long start_scan_addr;


#if defined(CONFIG_SYNO_MPC85XX_COMMON)
extern struct resource physmap_flash_resource;
/* Partition definition for the first flash bank which is always present. */
static struct mtd_partition synomtd_partitions[] = {
	{
		.name	= "RedBoot",			/* u-boot		*/
		.offset	= 0x003C0000,
		.size	= 0x00040000,		/* 256KB		*/
	},
	{
		.name	= "zImage",			/* linux kernel image	*/
		.offset	= 0x00000000,
		.size	= 0x00200000,		/* 2 MB		*/
	},
	{
		.name	= "rd.gz",			/* ramdisk image*/
		.offset	= 0x00200000,
		.size	= 0x00180000,		/* 1.5 MB		*/
	},
	{
		.name	= "vendor",
		.offset	= 0x00380000,
		.size	= 0x00010000,		/* 64KB		*/
	},
	{
		.name	= "RedBoot Config",
		.offset	= 0x00390000,
		.size	= 0x00020000,		/* 128KB		*/
	},
	{
		.name	= "FIS directory",
		.offset	= 0x003B0000,
		.size	= 0x00010000,		/* 64KB		*/
	},
};
#elif defined(CONFIG_SYNO_MV88F6281)
#ifdef MY_DEF_HERE
extern long gSynoFlashMemorySize;
#endif
extern struct resource physmap_flash_resource;
/* Partition definition for the first flash bank which is always present. */
static struct mtd_partition synomtd_partitions_8M[] = {
	{
		.name   = "RedBoot",            /* u-boot               */
		.offset = 0x00000000,
		.size   = 0x00090000,           /* 576 KB               */
	},
	{
		.name   = "zImage",             /* linux kernel image   */
		.offset = 0x00090000,
		.size   = 0x00300000,           /* 3 MB                 */
	},
	{
		.name   = "rd.gz",              /* ramdisk image        */
		.offset = 0x00390000,
		.size   = 0x00440000,           /* 4.2 MB               */
	},
	{
		.name   = "vendor",             /* vendor specific data */
		.offset = 0x007D0000,
		.size   = 0x00010000,           /* 64KB                 */
	},
	{
		.name   = "RedBoot Config",     /* configs for u-boot   */
		.offset = 0x007E0000,
		.size   = 0x00010000,           /* 64KB                */
	},
	{
		.name   = "FIS directory",      /* flash partition table*/
		.offset = 0x007F0000,
		.size   = 0x00010000,           /* 64KB                 */
	},
};
static struct mtd_partition synomtd_partitions[] = {
	{
		.name   = "RedBoot",            /* u-boot               */
		.offset = 0x00000000,
		.size   = 0x00080000,           /* 512KB                */
	},
	{
		.name   = "zImage",                     /* linux kernel image   */
		.offset = 0x00080000,
		.size   = 0x00200000,           /* 2 MB                                 */
	},
	{
		.name   = "rd.gz",                      /* ramdisk image*/
		.offset = 0x00280000,
		.size   = 0x00140000,           /* 1.2 MB               */
	},
	{
		.name   = "vendor",                     /* vendor specific data */
		.offset = 0x003C0000,
		.size   = 0x00010000,           /* 64KB                                 */
	},
	{
		.name   = "RedBoot Config",     /* configs for u-boot   */
		.offset = 0x003D0000,
		.size   = 0x00020000,           /* 128KB                                */
	},
	{
		.name   = "FIS directory",      /* flash partition table*/
		.offset = 0x003F0000,
		.size   = 0x00010000,           /* 64KB                                 */
	},
};
#else
/* Partition definition for the first flash bank which is always present. */
static struct mtd_partition synomtd_partitions[] = {
	{
		.name	= "RedBoot",		/* u-boot		*/
		.offset	= 0x00300000,
		.size	= 0x00040000,		/* 256KB		*/
	},
	{
		.name	= "zImage",			/* linux kernel image	*/
		.offset	= 0x00000000,
		.size	= 0x00200000,		/* 2 MiB		*/
	},
	{
		.name	= "rd.gz",			/* ramdisk image*/
		.offset	= 0x00200000,
		.size	= 0x00100000,		/* 1 MiB		*/
	},
	{
		.name	= "vendor",
		.offset	= 0x00340000,
		.size	= 0x00010000,		/* 64KB		*/
	},
	{
		.name	= "RedBoot Config",
		.offset	= 0x00350000,
		.size	= 0x00020000,		/* 128KB		*/
	},
	{
		.name	= "FIS directory",
		.offset	= 0x003f0000,
		.size	= 0x00010000,		/* 64KB		*/
	},
};
#endif

static int __init init_synomtd(void)
{
	int idx = 0, ret = 0;
	unsigned long flash_addr, flash_size, mtd_size = 0;
	struct mtd_partition *pMtdPartition = NULL;

#if !defined(CONFIG_SYNO_MV_COMMON) && !defined(CONFIG_SYNO_MPC85XX_COMMON)
	bd_t *bd = (bd_t *)__res;
#endif

	int n;
#ifdef CONFIG_MTD_REDBOOT_PARTS
	char mtdid[4];
	const char *part_probes[] = { "RedBoot", NULL };
#endif

#if defined(CONFIG_SYNO_MV_COMMON) || defined(CONFIG_SYNO_MPC85XX_COMMON)
	flash_addr = physmap_flash_resource.start;
	flash_size = physmap_flash_resource.end - physmap_flash_resource.start + 1;
#else
	flash_addr = bd->bi_flashstart;
	flash_size = bd->bi_flashsize;
#endif
	
	/* request maximum flash size address space */
	start_scan_addr = (unsigned long)ioremap(flash_addr, flash_size);
	if (!start_scan_addr) {
		printk("%s: Failed to ioremap address: 0x%lx\n",
		       __FUNCTION__, flash_addr);
		return -EIO;
	}

	for(idx = 0 ; idx < FLASH_BANK_MAX ; idx++) {
		if (mtd_size >= flash_size)
			break;

		pr_debug("%s: chip probing count %d\n", __FUNCTION__, idx);

		map_banks[idx] =
			(struct map_info *)kmalloc(sizeof(struct map_info),
						   GFP_KERNEL);
		if (map_banks[idx] == NULL) {
			ret = -ENOMEM;
			goto error_mem;
		}
		memset((void *)map_banks[idx], 0, sizeof(struct map_info));
		map_banks[idx]->name = (char *)kmalloc(16, GFP_KERNEL);
		if (map_banks[idx]->name == NULL) {
			ret = -ENOMEM;
			goto error_mem;
		}

		snprintf((char *)map_banks[idx]->name, 16, "SYNOMTD-%d", idx);
		map_banks[idx]->size = flash_size;
		map_banks[idx]->bankwidth = 1;

		simple_map_init(map_banks[idx]);

		map_banks[idx]->virt = (void __iomem *)
			(start_scan_addr + ((idx > 0) ?
			(mtd_banks[idx-1] ? (unsigned long)mtd_banks[idx-1]->size : 0) : 0));
		map_banks[idx]->phys =
			flash_addr + ((idx > 0) ?
			(mtd_banks[idx-1] ? mtd_banks[idx-1]->size : 0) : 0);

		/* start to probe flash chips */
#if defined(CONFIG_SYNO_MV88F6281)
		mtd_banks[idx] = do_map_probe("sflash", map_banks[idx]);
#else
		mtd_banks[idx] = do_map_probe("cfi_probe", map_banks[idx]);
#endif
		if (mtd_banks[idx]) {
			mtd_banks[idx]->owner = THIS_MODULE;
			mtd_size += mtd_banks[idx]->size;
			num_banks++;
			pr_debug("%s: bank %lu, name: %s, size: %llu bytes \n",
				 __FUNCTION__, num_banks,
				 mtd_banks[idx]->name, mtd_banks[idx]->size);
		}
	}

	/* no supported flash chips found */
	if (!num_banks) {
		printk("SYNOMTD: No supported flash chips found!\n");
		ret = -ENXIO;
		goto error_mem;
	}

	for(idx = 0; idx < num_banks ; idx++) {
#ifdef CONFIG_MTD_REDBOOT_PARTS
		sprintf(mtdid, "%d", idx);
		n = parse_mtd_partitions(mtd_banks[idx],
					 part_probes,
					 &part_banks[idx].mtd_part,
					 0);
		pr_debug("%s: %d RedBoot partitions on bank %s\n",
			 __FUNCTION__, n, mtdid);
		if (n > 0) {
			part_banks[idx].type = "RedBoot";
			part_banks[idx].nums = n;
		}
		else
#endif	/* CONFIG_MTD_REDBOOT_PARTS */
		{
			/*
			 * Select static partition definitions
			 */
			pMtdPartition = &synomtd_partitions;
			n = ARRAY_SIZE(synomtd_partitions);
#ifdef MY_DEF_HERE
			if (8 == gSynoFlashMemorySize) {
				pMtdPartition = &synomtd_partitions_8M;
				n = ARRAY_SIZE(synomtd_partitions_8M);
			}
#endif
			part_banks[idx].mtd_part	= pMtdPartition;
			part_banks[idx].type	= "static image bank1";
			part_banks[idx].nums	= n;

			/* update last partition size to cover actual remaining space */
			pMtdPartition[n - 1].size =
				mtd_banks[0]->size - pMtdPartition[n - 1].offset;
		}
		if (part_banks[idx].nums == 0) {
			printk(KERN_NOTICE
			       "SYNOMTD flash bank %d: no partition info "
			       "available, registering whole device\n", idx);
			add_mtd_device(mtd_banks[idx]);
		} else {
			printk(KERN_NOTICE
			       "SYNOMTD flash bank %d: Using %s partition "
			       "definition\n", idx, part_banks[idx].type);
			add_mtd_partitions(mtd_banks[idx],
					   part_banks[idx].mtd_part,
					   part_banks[idx].nums);
		}
	}

	return 0;
error_mem:
	for (idx = 0 ; idx < FLASH_BANK_MAX ; idx++) {
		if (map_banks[idx] != NULL) {
			if (map_banks[idx]->name != NULL) {
				kfree(map_banks[idx]->name);
				map_banks[idx]->name = NULL;
			}
			kfree(map_banks[idx]);
			map_banks[idx] = NULL;
		}
	}

	iounmap((void *)start_scan_addr);

	return ret;
}

static void __exit cleanup_synomtd(void)
{
	unsigned int idx = 0;
	for(idx = 0 ; idx < num_banks ; idx++) {
		/* destroy mtd_info previously allocated */
		if (mtd_banks[idx]) {
			del_mtd_partitions(mtd_banks[idx]);
			map_destroy(mtd_banks[idx]);
		}

		/* release map_info not used anymore */
		kfree(map_banks[idx]->name);
		kfree(map_banks[idx]);
	}

	if (start_scan_addr) {
		iounmap((void *)start_scan_addr);
		start_scan_addr = 0;
	}
}

module_init(init_synomtd);
module_exit(cleanup_synomtd);
