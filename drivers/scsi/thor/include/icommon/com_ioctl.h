#ifndef __MV_COM_IOCTL_H__
#define __MV_COM_IOCTL_H__

#if defined (_OS_WINDOWS)
#include <ntddscsi.h>
#elif defined(_OS_LINUX)

#endif /* _OS_WINDOWS */

/* private IOCTL commands */
#define MV_IOCTL_CHECK_DRIVER                                \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x900, METHOD_BUFFERED,                 \
                     FILE_READ_ACCESS | FILE_WRITE_ACCESS)	

/*
 * MV_IOCTL_LEAVING_S0 is a notification when the system is going 
 * to leaving S0. This gives the driver a chance to do some house
 * keeping work before system really going to sleep.
 *
 * The MV_IOCTL_LEAVING_S0 will be translated to APICDB0_ADAPTER/
 * APICDB1_ADAPTER_POWER_STATE_CHANGE and passed down along the
 * module stack. A module shall handle this request if necessary.
 *
 * Upon this request, usually the Cache module shall flush all
 * cached data. And the RAID module shall auto-pause all background
 * activities.
 */
#define MV_IOCTL_LEAVING_S0                                \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x901, METHOD_BUFFERED,                 \
                     FILE_READ_ACCESS | FILE_WRITE_ACCESS)	

/* IOCTL signature */
#define MV_IOCTL_DRIVER_SIGNATURE                "mv61xxsg"
#define MV_IOCTL_DRIVER_SIGNATURE_LENGTH         8

/* IOCTL command status */
#define IOCTL_STATUS_SUCCESS                     0
#define IOCTL_STATUS_INVALID_REQUEST             1
#define IOCTL_STATUS_ERROR                       2

#ifndef _OS_BIOS
#pragma pack(8)
#endif  /* _OS_BIOS */

typedef struct _MV_IOCTL_BUFFER
{
#ifdef _OS_WINDOWS
	SRB_IO_CONTROL Srb_Ctrl;
#endif /* _OS_WINDOWS */
	MV_U8          Data_Buffer[32];
} MV_IOCTL_BUFFER, *PMV_IOCTL_BUFFER;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif /* __MV_COM_IOCTL_H__ */
