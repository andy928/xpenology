#ifndef __MV_COM_FLASH_H__
#define __MV_COM_FLASH_H__

#include "com_define.h"

#define DRIVER_LENGTH                      1024*16

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _Flash_DriverData
{
	MV_U16            Size;
	MV_U8             PageNumber;
	MV_BOOLEAN        isLastPage;
	MV_U16            Reserved[2];
	MV_U8             Data[DRIVER_LENGTH];
}
Flash_DriveData, *PFlash_DriveData;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif /* __MV_COM_FLASH_H__ */
