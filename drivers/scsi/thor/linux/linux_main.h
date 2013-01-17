#ifndef _LINUX_MAIN_H
#define _LINUX_MAIN_H

#include "hba_header.h"

#define __mv_get_ext_from_host(phost) \
          (((struct hba_extension **) (phost)->hostdata)[0])

/* for communication with OS/SCSI mid layer only */
enum {
#ifdef RAID_DRIVER
	MV_MAX_REQUEST_DEPTH     = MAX_REQUEST_NUMBER_PERFORMANCE - 2,
	MV_MAX_IO                = MAX_REQUEST_NUMBER_PERFORMANCE,
#else /* RAID_DRIVER */
	MV_MAX_REQUEST_DEPTH     = MAX_CORE_REQUEST_NUMBER_PERFORMANCE - 2,
	MV_MAX_IO                = MAX_CORE_REQUEST_NUMBER_PERFORMANCE,
#endif /* RAID_DRIVER */
	MV_MAX_REQUEST_PER_LUN   = 32,
	MV_MAX_SG_ENTRY          = 32,

	MV_SHT_USE_CLUSTERING    = DISABLE_CLUSTERING,
	MV_SHT_EMULATED          = 0,
	MV_SHT_THIS_ID           = -1,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#define mv_scmd_host(cmd)    cmd->device->host
#define mv_scmd_channel(cmd) cmd->device->channel
#define mv_scmd_target(cmd)  cmd->device->id
#define mv_scmd_lun(cmd)     cmd->device->lun
#else
#define mv_scmd_host(cmd)    cmd->host
#define mv_scmd_channel(cmd) cmd->channel
#define mv_scmd_target(cmd)  cmd->target
#define mv_scmd_lun(cmd)     cmd->lun
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)	
#define MV_DMA_BIT_MASK_64 DMA_64BIT_MASK
#define MV_DMA_BIT_MASK_32 DMA_32BIT_MASK
#else
#define MV_DMA_BIT_MASK_64 DMA_BIT_MASK(64)
#define MV_DMA_BIT_MASK_32 DMA_BIT_MASK(32)
#endif

#define LO_BUSADDR(x) ((MV_U32)(x))
#define HI_BUSADDR(x) (sizeof(BUS_ADDRESS)>4? (u64)(x) >> 32 : 0)

#endif /*_LINUX_MAIN_H*/

