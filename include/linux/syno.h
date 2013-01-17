#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif

#ifndef __SYNO_H_
#define __SYNO_H_
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif

#define SYNO_HAVE_KERNEL_VERSION(a,b,c) (LINUX_VERSION_CODE >= KERNEL_VERSION((a),(b),(c)) )
#define SYNO_HAVE_GCC_VERSION(a,b) (__GNUC__ > (a) || (__GNUC__ == (a) && __GNUC_MINOR__ >= (b)))
#define SYNO_HAVE_GLIBC_VERSION(a,b) ( __GLIBC__ > (a) || (__GLIBC__ == (a) && __GLIBC_MINOR__ >= (b)))
#if 1
#define SYNO_USB_FLASH_DEVICE_INDEX 255
#define SYNO_USB_FLASH_DEVICE_NAME  "synoboot"
#define SYNO_USB_FLASH_DEVICE_PATH  "/dev/synoboot"
#if 1
#define IS_SYNO_USBBOOT_ID_VENDOR(VENDOR) (0xF400 == (VENDOR) || 0xF401 == (VENDOR))
#define IS_SYNO_USBBOOT_ID_PRODUCT(PRODUCT) (0xF400 == (PRODUCT) || 0xF401 == (PRODUCT))
#else
#define IS_SYNO_USBBOOT_ID_VENDOR(VENDOR) (0xF400 == (VENDOR))
#define IS_SYNO_USBBOOT_ID_PRODUCT(PRODUCT) (0xF400 == (PRODUCT))
#endif

#endif

#if 1 
#ifdef MY_ABC_HERE
#define SYNO_MAX_SWITCHABLE_NET_DEVICE 8
#define SYNO_NET_DEVICE_ENCODING_LENGTH 6
#endif

#endif

#ifdef MY_ABC_HERE
#define SYNO_YOTAWIMAX_DESC          "SYNO CDC Ethernet Device for YotaKey"
#define SYNO_YOTAWIMAX_ETHERNET_NAME "wm"
#define SYNO_YOTAWIMAX_NET_NOLINK_EVENT (0xffffffff)
#endif

#define USBCOPY_PORT_LOCATION 99
#ifdef MY_ABC_HERE
#define SDCOPY_PORT_LOCATION 98
#endif

#ifdef MY_ABC_HERE
#define CHECKINTERVAL (7UL*HZ)
#endif

#if	defined(MY_ABC_HERE) || defined(SYNO_BADSECTOR_TEST)
#if 1
#define SYNO_MAX_INTERNAL_DISK 19
#else
#define SYNO_MAX_INTERNAL_DISK	15
#endif

#endif

#if 1 && defined(MY_ABC_HERE)
#define SYNO_SATA_PM_DEVICE_GPIO
#endif

#if defined(SYNO_SATA_PM_DEVICE_GPIO) && defined(MY_ABC_HERE)
#if 1 
#define SYNO_MAX_SATA_ID 20
#define SYNO_PWRPIN_ENCODE_LEN 2
#define SYNO_PORT_SIGN 'N'
#define SYNO_PWRPIN_ITEM_LEN 1 + SYNO_PWRPIN_ENCODE_LEN*2
#endif

#endif

#ifdef MY_ABC_HERE
#if defined (F_CLEAR_ARCHIVE) || defined (F_SETSMB_ARCHIVE) || defined (F_SETSMB_HIDDEN) || \
	defined (F_SETSMB_SYSTEM) || defined (F_CLRSMB_ARCHIVE) || defined (F_CLRSMB_HIDDEN) || \
	defined (F_CLRSMB_SYSTEM) || defined (F_CLEAR_S3_ARCHIVE)
#error "Samba archive bit redefine."
#endif

#if 1
#if defined (F_CLRSMB_READONLY) || defined (F_SETSMB_READONLY) || \
	defined (F_CLRACL_INHERIT)  || defined (F_SETACL_INHERIT)  || \
	defined (F_CLRACL_OWNER_IS_GROUP) || defined (F_SETACL_OWNER_IS_GROUP)  || \
	defined (F_SETACL_SUPPORT) || defined (F_SETACL_SUPPORT)
#error "ACL archive bit redefine."
#endif

#endif

#define F_CLEAR_ARCHIVE     513
#define F_SETSMB_ARCHIVE    514
#define F_SETSMB_HIDDEN     515
#define F_SETSMB_SYSTEM     516
#define F_CLRSMB_ARCHIVE    517
#define F_CLRSMB_HIDDEN     518
#define F_CLRSMB_SYSTEM     519
#define F_CLEAR_S3_ARCHIVE  520
#ifdef MY_ABC_HERE
#define F_CLRSMB_READONLY   		521
#define F_SETSMB_READONLY   		522
#define F_CLRACL_INHERIT    		523
#define F_SETACL_INHERIT    		524
#define F_CLRACL_HAS_ACL   			525
#define F_SETACL_HAS_ACL   			526
#define F_CLRACL_SUPPORT   			527
#define F_SETACL_SUPPORT   			528
#define F_CLRACL_OWNER_IS_GROUP   	529
#define F_SETACL_OWNER_IS_GROUP   	530
#endif

#else
#undef 1
#undef 1
#endif

#ifdef MY_ABC_HERE
#define SYNO_SMB_PSTRING_LEN 1024
#endif

#define SYNO_EXT4_SYNC_DALLOC_RETRY  100
#ifdef MY_ABC_HERE
#define MAX_CHANNEL_RETRY       2
#define CHANNEL_RETRY_INTERVAL  (3*HZ)
#endif

#include <linux/syno_user.h>
#include <linux/syno_debug.h>
#define SYNO_NFSD_WRITE_SIZE_MIN 131072
#ifdef MY_ABC_HERE
#define SYNO_SATA_DEVICE_PREFIX	   "sd"
#define SYNO_ISCSI_DEVICE_PREFIX   "isd"
#define SYNO_ISCSI_DEVICE_INDEX    (26 + 25 * 26)    
#endif

#define SYNO_RTL8712_DEBUG_MSG 0
#define SYNO_RTL8192_DEBUG_MSG 0
#if 1
#if defined(MY_ABC_HERE)
#define SYNO_SAS_DISK_NAME
#endif

#if defined(SYNO_SAS_DISK_NAME)
#define SYNO_SAS_USB_DEVICE_PREFIX		"usb"
#define SYNO_SAS_DEVICE_PREFIX			"sas"
#ifdef MY_ABC_HERE
#define SYNO_SAS_ISCSI_DEVICE_PREFIX	"iscsi"
#endif

#endif

#endif

#endif

