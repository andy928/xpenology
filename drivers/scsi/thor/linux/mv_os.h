#if !defined(LINUX_OS_H)
#define LINUX_OS_H

#ifndef LINUX_VERSION_CODE
#   include <linux/version.h>
#endif

#ifndef AUTOCONF_INCLUDED
#   include <linux/config.h>
#endif /* AUTOCONF_INCLUDED */

#include <linux/list.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/reboot.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/completion.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/nmi.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_transport.h>
#include <scsi/scsi_ioctl.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
#include <scsi/scsi_request.h>
#endif

/* OS specific flags */
#define  _OS_LINUX            1
#define _64_BIT_COMPILER      1
#ifdef USE_NEW_SGTABLE
#define USE_NEW_SGVP
#endif /* USE_NEW_SGTABLE */

#ifdef CONFIG_64BIT
#   define __KCONF_64BIT__
#endif /* CONFIG_64BIT */

#if defined(__LITTLE_ENDIAN)
#   define __MV_LITTLE_ENDIAN__  1
#elif defined(__BIG_ENDIAN)
#   define __MV_BIG_ENDIAN__     1
#else
#   error "error in endianness"
#endif 

#if defined(__LITTLE_ENDIAN_BITFIELD)
#   define __MV_LITTLE_ENDIAN_BITFIELD__   1
#elif defined(__BIG_ENDIAN_BITFIELD)
#   define __MV_BIG_ENDIAN_BITFIELD__      1
#else
#   error "error in endianness"
#endif 

#ifdef __MV_DEBUG__
#   define MV_DEBUG
#else
#   ifdef MV_DEBUG
#      undef MV_DEBUG
#   endif /* MV_DEBUG */
#endif /* __MV_DEBUG__ */

#ifndef NULL
#   define NULL 0
#endif

/* Values for T10/04-262r7 */
#ifndef ATA_16 
#	define ATA_16                0x85      /* 16-byte pass-thru */
#endif
#ifndef ATA_12
#	define ATA_12                0xa1      /* 12-byte pass-thru */
#endif



#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
#define PCI_D0 0
#include <linux/suspend.h>
typedef u32 pm_message_t; 

static inline int try_to_freeze(unsigned long refrigerator_flags)
{
        if (unlikely(current->flags & PF_FREEZE)) {
               refrigerator(refrigerator_flags);
                return 1;
        } else
             return 0;
}
#endif

/*
 *
 * Primary Data Type Definition 
 *
 */
#include "com_define.h" 

#define MV_INLINE inline
#define CDB_INQUIRY_EVPD    1 


typedef void (*OSSW_TIMER_FUNCTION)(unsigned long);
typedef  void (*CORE_TIMER_FUNCTION)(void *);
typedef  void (*OS_TIMER_FUNCTION)(void *);

typedef unsigned long OSSW_TIMER_DATA;

/* OS macro definition */
#define MV_MAX_TRANSFER_SECTOR  (MV_MAX_TRANSFER_SIZE >> 9)

/*Driver Version for Command Line Interface Query.*/
#  define VER_MAJOR           1


#   define VER_MINOR        2
#   define VER_BUILD        24

#ifdef __OEM_INTEL__
#   define VER_OEM          VER_OEM_INTEL
#elif defined(__OEM__ASUS__)
#   define VER_OEM          VER_OEM_ASUS
#else
#   define VER_OEM         VER_OEM_GENERIC
#endif /* __OEM_INTEL__ */


#ifdef RAID_DRIVER
#   define VER_TEST         ""
#else
#   define VER_TEST         "N"
#endif /* RAID_DRIVER */
#ifdef ODIN_DRIVER
#define mv_driver_name   "mv64xx"

#define mv_product_name  "ODIN"
#endif

#ifdef THOR_DRIVER
#define mv_driver_name   "mv61xx"

#define mv_product_name  "THOR"
#endif

/* call VER_VAR_TO_STRING */
#define NUM_TO_STRING(num1, num2, num3, num4) #num1"."#num2"."#num3"."#num4
#define VER_VAR_TO_STRING(major, minor, oem, build) NUM_TO_STRING(major, \
								  minor, \
								  oem,   \
								  build)

#define mv_version_linux   VER_VAR_TO_STRING(VER_MAJOR, VER_MINOR,       \
					     VER_OEM, VER_BUILD) VER_TEST

#define CPU_TO_LE_16 cpu_to_le16
#define CPU_TO_LE_32 cpu_to_le32
#define LE_TO_CPU_16 le16_to_cpu
#define LE_TO_CPU_32 le32_to_cpu

#ifndef scsi_to_pci_dma_dir
#define scsi_to_pci_dma_dir(scsi_dir) ((int)(scsi_dir))
#endif

#ifndef TRUE
#define TRUE 	1
#define FALSE	0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
#define map_sg_page(sg)		kmap_atomic(sg->page, KM_IRQ0)	
#define map_sg_page_sec(sg)		kmap_atomic(sg->page, KM_IRQ1)	
#else
#define map_sg_page(sg)		kmap_atomic(sg_page(sg), KM_IRQ0)	
#define map_sg_page_sec(sg)		kmap_atomic(sg_page(sg), KM_IRQ1)	
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
#define mv_use_sg(cmd)	cmd->use_sg
#define mv_rq_bf(cmd)	cmd->request_buffer
#define mv_rq_bf_l(cmd)	cmd->request_bufflen
#else
#define mv_use_sg(cmd)	scsi_sg_count(cmd)
#define mv_rq_bf(cmd)	scsi_sglist(cmd)
#define mv_rq_bf_l(cmd)	scsi_bufflen(cmd)
#endif

#endif /* LINUX_OS_H */
