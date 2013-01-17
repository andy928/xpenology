// Copyright (c) 2000-2008 Synology Inc. All rights reserved.
#ifdef MY_ABC_HERE
#include <linux/bio.h>
#include <linux/synobios.h>
#include <linux/synolib.h>

#include <linux/raid/libmd-report.h>
#include "md.h"

int (*funcSYNOSendRaidEvent)(unsigned int, unsigned int, unsigned int, unsigned int) = NULL;

void SynoReportBadSector(sector_t sector, unsigned long rw, 
						 int md_minor, struct block_device *bdev, const char *szFuncName)
{
	char b[BDEVNAME_SIZE];
	int index = SynoSCSIGetDeviceIndex(bdev);

	bdevname(bdev,b);

	if (printk_ratelimit()) {
		printk("%s error, md%d, %s index [%d], sector %llu [%s]\n",
					   rw ? "write" : "read", md_minor, b, index, (unsigned long long)sector, szFuncName);
	}

	if (funcSYNOSendRaidEvent) {
		funcSYNOSendRaidEvent(
			(rw == WRITE) ? MD_SECTOR_WRITE_ERROR : MD_SECTOR_READ_ERROR, 
			md_minor, index, sector);
	}
}

EXPORT_SYMBOL(SynoReportBadSector);

void SynoReportCorrectBadSector(sector_t sector, int md_minor, 
								struct block_device *bdev, const char *szFuncName)
{
	char b[BDEVNAME_SIZE];
	int index = SynoSCSIGetDeviceIndex(bdev);

	bdevname(bdev,b);

	printk("read error corrected, md%d, %s index [%d], sector %llu [%s]\n",
				   md_minor, b, index, (unsigned long long)sector, szFuncName);

	if (funcSYNOSendRaidEvent) {
		funcSYNOSendRaidEvent(MD_SECTOR_REWRITE_OK, md_minor, 
							  index, sector);
	}
}
EXPORT_SYMBOL(SynoReportCorrectBadSector);
EXPORT_SYMBOL(funcSYNOSendRaidEvent);

#ifdef MY_ABC_HERE
sector_t (*funcSYNOLvLgSectorCount)(void *, sector_t) = NULL;
int (*funcSYNOSendAutoRemapRaidEvent)(unsigned int, unsigned long long, unsigned int) = NULL;
int (*funcSYNOSendAutoRemapLVEvent)(const char*, unsigned long long, unsigned int) = NULL;
void SynoAutoRemapReport(struct mddev *mddev, sector_t sector, struct block_device *bdev)
{
	int index = SynoSCSIGetDeviceIndex(bdev);

	if (NULL == mddev->syno_private) {
		if (NULL == funcSYNOSendAutoRemapRaidEvent) {
			printk("Can't reference to function 'SYNOSendAutoRemapRaidEvent'\n");
		} else {
			printk("report md[%d] [%d]th disk auto-remapped sector:[%llu]\n",
				mddev->md_minor, index, (unsigned long long)sector);
			funcSYNOSendAutoRemapRaidEvent(mddev->md_minor, sector, (unsigned int)index);
		}
	} else {
		if (NULL == funcSYNOLvLgSectorCount || NULL == funcSYNOSendAutoRemapLVEvent) {
			printk("Can't reference to function 'funcSYNOLvLgSectorCount' or 'SYNOSendAutoRemapLVEvent'\n");
		} else {
			sector_t lv_sector = funcSYNOLvLgSectorCount(mddev->syno_private, sector);
			printk("report lv:[%s] [%d]th auto-remapped sector:[%llu]\n",
				mddev->lv_name, index, (unsigned long long)lv_sector);
			funcSYNOSendAutoRemapLVEvent(mddev->lv_name, lv_sector, (unsigned int)index);
		}
	}
}

EXPORT_SYMBOL(SynoAutoRemapReport);
EXPORT_SYMBOL(funcSYNOLvLgSectorCount);
EXPORT_SYMBOL(funcSYNOSendAutoRemapRaidEvent);
EXPORT_SYMBOL(funcSYNOSendAutoRemapLVEvent);
#endif
#endif /* MY_ABC_HERE */
