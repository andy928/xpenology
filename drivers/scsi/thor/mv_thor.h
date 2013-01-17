#ifndef __MV_THOR_H__
#define __MV_THOR_H__

/* OEM Account definition */
#define VER_OEM_GENERIC			0
#define VER_OEM_INTEL			1
#define VER_OEM_ASUS			2
#if defined(__MV_LINUX__) || defined(__QNXNTO__)
#	define THOR_DRIVER						1
#	define NEW_LINUX_DRIVER				1
#   define __MM_SE__                       1
#   define __RES_MGMT__                    1
#endif /* __MV_LINUX__ && __QNXNTO__ */

#ifndef __MV_LINUX__
#define DISPATCH_HOOK
/*
 * define USE_NEW_SGTABLE to use the new SG format as defined in
 * "General SG Format" document.
 */
#define USE_NEW_SGTABLE

/* 
 * define USE_SRBEXT_AS_REQ to use Windows SrbExtension to store 
 * the MV_Request with its SG Table. 
 */
#define USE_SRBEXT_AS_REQ

#define FILTER_ASSISTED_COMMON_BUFFER

#endif /* _OS_WINDOWS */

#define USE_NEW_SGTABLE

/* HBA macro definition */
#define MV_MAX_TRANSFER_SIZE	(128*1024)
#define MAX_REQUEST_NUMBER		32
#define MAX_BASE_ADDRESS		6

#define MAX_DEVICE_SUPPORTED	8

/* Core driver macro definition */
#define MAX_SG_ENTRY			34
#define MAX_SG_ENTRY_REDUCED	16
#define MV_MAX_PHYSICAL_BREAK	(MAX_SG_ENTRY - 1)

#ifdef USE_NEW_SGTABLE
#define HW_SG_ENTRY_MAX		(MAX_SG_ENTRY)
#endif

//#define ENABLE_PATA_ERROR_INTERRUPT	//ATAPI???

/* It's dangerous. Never enable it unless we have to. */
#define PRD_SIZE_WORD_ALIGN	

/* 
 * For ATAPI device, we can choose 
 * 1. Enable USE_PIO_FOR_ALL_PACKET_COMMAND
 * 2. Enable USE_DMA_FOR_ALL_PACKET_COMMAND
 * 3. Don't enable either of them. It'll use DMA for read/write. Others will use PIO.
 * So far, USE_DMA_FOR_ALL_PACKET_COMMAND is the best choice.
 */
//#define USE_PIO_FOR_ALL_PACKET_COMMAND
#define USE_DMA_FOR_ALL_PACKET_COMMAND


#define CORE_SUPPORT_API      1
#define CORE_IGNORE_START_STOP_UNIT


//#define CORE_DEBUG_MODE

/* hot plug & PM */
#define SUPPORT_HOT_PLUG	1
#define SUPPORT_PM			1
#define SUPPORT_ERROR_HANDLING		1

#ifndef __MV_LINUX__
#define SUPPORT_CONSOLIDATE			1
#define SUPPORT_TIMER				1
#define SUPPORT_SCSI_PASSTHROUGH  1
#endif /* _OS_WINDOWS */

/* pass through */
#ifdef SUPPORT_SCSI_PASSTHROUGH		
#define SUPPORT_VIRTUAL_DEVICE
#define MV_MAX_TARGET_NUMBER		21		// virtual device 5 port*4 device
#else
#define MV_MAX_TARGET_NUMBER		20		// virtual device 5 port*4 device
#endif
#define MV_MAX_LUN_NUMBER			1
#define MV_MAX_HD_DEVICE_ID		20		// virtual device 5 port*4 device

#define VIRTUAL_DEVICE_ID					(MV_MAX_TARGET_NUMBER - 1) * MV_MAX_LUN_NUMBER

#define SUPPORT_EVENT			  1

/* RAID */
#ifndef __MV_LINUX__
#define PERFORMANCE_WHQL_SWITCH

#define RAID_DRIVER				1
#define SUPPORT_RAID6
#define CACHE_MODULE_SUPPORT
#endif /* _OS_WINDOWS */

#ifdef RAID_DRIVER
#define BGA_SUPPORT				1
#define SOFTWARE_XOR			1
#define SUPPORT_FREE_POLICY		1
//#define SUPPORT_SRL				1
#define SUPPORT_XOR_DWORD		1
//#define SUPPORT_RAID1E				1
#define SUPPORT_DUAL_DDF			0

#ifdef SUPPORT_RAID6
#define USE_MATH_LIBARY
#define SUPPORT_READ_MODIFY_WRITE
#define RAID_USE_64K_SU
#endif
/*
 * define SUPPORT_NON_STRIPE to force RAID not to split RAID0/1
 * access by stripes.
 */
#define SUPPORT_NON_STRIPE
#define ALLOCATE_SENSE_BUFFER		1

//#define SUPPORT_MIGRATION
#define MV_MIGRATION_RESERVED_SPACE_V1	2048	/* 1MB & by sectors */
#define MV_MIGRATION_RESERVED_SPACE_V2	32768 //16MB	// SECTORS
#define MV_MIGRATION_SHIFT_SPACE		2048  // 1MB	// SECTORS
#endif /* RAID_DRIVER */

#if defined(USE_NEW_SGTABLE) && (defined(SOFTWARE_XOR) || defined(CACHE_MODULE_SUPPORT))
#define USE_VIRTUAL_PRD_TABLE           1
#endif

#define ERROR_HANDLING_SUPPORT	0
#define ERROR_SIMULATOR			0 /*1 for error simulator enable, 0 for disable*/

/* Cache */
#ifdef CACHE_MODULE_SUPPORT
#define CACHE_FREE_DISK_ENABLE 1
#endif

#ifndef USE_NEW_SGTABLE
#define SUPPORT_VIRTUAL_AND_PHYSICAL_SG
#endif

/* hardware-dependent definitions */
#define MAX_EXPANDER_SUPPORTED				0     
#define MAX_PM_SUPPORTED					4     
#define MAX_BLOCK_PER_HD_SUPPORTED			8

#define MAX_REQUEST_NUMBER_WHQL				MAX_REQUEST_NUMBER
#define MAX_DEVICE_SUPPORTED_WHQL			8
#define MAX_LD_SUPPORTED_WHQL				8
#define MAX_BLOCK_SUPPORTED_WHQL			32

#define MAX_REQUEST_NUMBER_PERFORMANCE		MAX_REQUEST_NUMBER_WHQL
#define MAX_DEVICE_SUPPORTED_PERFORMANCE	MAX_DEVICE_SUPPORTED_WHQL
#define MAX_LD_SUPPORTED_PERFORMANCE		MAX_LD_SUPPORTED_WHQL
#define MAX_BLOCK_SUPPORTED_PERFORMANCE		MAX_BLOCK_SUPPORTED_WHQL
#define MAX_CORE_REQUEST_NUMBER_PERFORMANCE   32

#define COMMAND_ISSUE_WORKROUND		1

#define SUPPORT_ATA_SMART        1 
#define SUPPORT_ATA_SECURITY_CMD 1
#define HOTPLUG_ISSUE_WORKROUND 1
#define SUPPORT_ATA_POWER_MANAGEMENT   1
//#define SUPPORT_WORKQUEUE             1

#endif /* __MV_THOR_H__ */
