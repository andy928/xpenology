/* ;This file is discarded after POST. */
/*******************************************************************************
*
*                   Copyright 2006,MARVELL SEMICONDUCTOR ISRAEL, LTD.
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
*
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES,
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL. (MSIL),  MARVELL TAIWAN, LTD. AND
* SYSKONNECT GMBH.
*
********************************************************************************
* com_nvram.c - File for implementation of the Driver Intermediate Application Layer
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*   mv_include.h
*
* FILE REVISION NUMBER:
*       $Revision: 1.1 $
*******************************************************************************/
#ifdef ODIN_DRIVER
#include "mv_include.h"
#include "core_header.h"
#include "core_helper.h"
#include "core_spi.h"
#include "com_nvram.h"

MV_BOOLEAN mvui_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param)
{
//	MV_U32 					nsize = FLASH_PARAM_SIZE;
	MV_U32 					param_flash_addr=PARAM_OFFSET,i = 0;
//	MV_U16 					my_ds=0;
	PCore_Driver_Extension	pCore;
	AdapterInfo				AI;

	if (!This)
		return MV_FALSE;

	pCore = (PCore_Driver_Extension)This;
	AI.bar[2] = pCore->Base_Address[2];

	if (-1 == OdinSPI_Init(&AI))
		return MV_FALSE;

	/* step 1 read param from flash offset = 0x3FFF00 */
	OdinSPI_ReadBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);

	/* step 2 check the signature first */
	if(pHBA_Info_Param->Signature[0] == 'M'&& \
	    pHBA_Info_Param->Signature[1] == 'R'&& \
	    pHBA_Info_Param->Signature[2] == 'V'&& \
	    pHBA_Info_Param->Signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,FLASH_PARAM_SIZE)))
	{
		if(pHBA_Info_Param->HBA_Flag == 0xFFFFFFFFL)
		{
			pHBA_Info_Param->HBA_Flag = 0;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
		}

		for(i=0;i<8;i++)
		{
			if(pHBA_Info_Param->PHY_Rate[i]>0x1)
				/* phy host link rate */
				pHBA_Info_Param->PHY_Rate[i] = 0x1;

			// validate phy tuning
			//pHBA_Info_Param->PHY_Tuning[i].Reserved[0] = 0;
			//pHBA_Info_Param->PHY_Tuning[i].Reserved[1] = 0;
		}
	}
	else
	{
		MV_FillMemory((MV_PVOID)pHBA_Info_Param, FLASH_PARAM_SIZE, 0xFF);
		pHBA_Info_Param->Signature[0] = 'M';	
		pHBA_Info_Param->Signature[1] = 'R';
	   	pHBA_Info_Param->Signature[2] = 'V';
	    pHBA_Info_Param->Signature[3] = 'L';

		// Set BIOS Version
		pHBA_Info_Param->Minor = NVRAM_DATA_MAJOR_VERSION;
		pHBA_Info_Param->Major = NVRAM_DATA_MINOR_VERSION;
		
		// Set SAS address
		for(i=0;i<MAX_PHYSICAL_PORT_NUMBER;i++)
		{
			pHBA_Info_Param->SAS_Address[i].b[0]=  0x50;
			pHBA_Info_Param->SAS_Address[i].b[1]=  0x05;
			pHBA_Info_Param->SAS_Address[i].b[2]=  0x04;
			pHBA_Info_Param->SAS_Address[i].b[3]=  0x30;
			pHBA_Info_Param->SAS_Address[i].b[4]=  0x11;
			pHBA_Info_Param->SAS_Address[i].b[5]=  0xab;
			pHBA_Info_Param->SAS_Address[i].b[6]=  0x00;
			pHBA_Info_Param->SAS_Address[i].b[7]=  0x00; 
			/*+(MV_U8)i; - All ports' WWN has to be same */
		}
		
		/* init phy link rate */
		for(i=0;i<8;i++)
		{
			/* phy host link rate */
			pHBA_Info_Param->PHY_Rate[i] = 0x1;//Default is 3.0G;
		}

		MV_PRINT("pHBA_Info_Param->HBA_Flag = 0x%x \n",pHBA_Info_Param->HBA_Flag);

		/* init setting flags */
		pHBA_Info_Param->HBA_Flag = 0;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
		/* write to flash and save it now */
		if(OdinSPI_SectErase( &AI, param_flash_addr) != -1)
			MV_PRINT("FLASH ERASE SUCCESS\n");
		else
			MV_PRINT("FLASH ERASE FAILED\n");

		pHBA_Info_Param->Check_Sum = 0;
		pHBA_Info_Param->Check_Sum=mvCaculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));
		/* init the parameter in ram */
		OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);
	}
	return MV_TRUE;
}

MV_U8	mvCaculateChecksum(MV_PU8	Address, MV_U32 Size)
{
		MV_U8 checkSum;
		MV_U32 temp=0;
        checkSum = 0;
        for (temp = 0 ; temp < Size ; temp ++)
        {
                checkSum += Address[temp];
        }
        
        checkSum = (~checkSum) + 1;



		return	checkSum;
}

MV_U8	mvVerifyChecksum(MV_PU8	Address, MV_U32 Size)
{
		MV_U8	checkSum=0;
		MV_U32 	temp=0;
        for (temp = 0 ; temp < Size ; temp ++)
        {
            checkSum += Address[temp];
        }
        
		return	checkSum;
}

#endif
