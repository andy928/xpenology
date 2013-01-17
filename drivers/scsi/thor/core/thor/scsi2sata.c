#include "mv_include.h"

#include "core_inter.h"

#include "core_sata.h"
#include "core_ata.h"
#include "core_sat.h"

/*
 * Translate SCSI command to SATA FIS
 * The following SCSI command set is the minimum required.
 *		Standard Inquiry
 *		Read Capacity
 *		Test Unit Ready
 *		Start/Stop Unit
 *		Read 10
 *		Write 10
 *		Request Sense
 *		Mode Sense/Select
 */
MV_VOID SCSI_To_FIS(MV_PVOID This, PMV_Request pReq, MV_U8 tag, PATA_TaskFile pTaskFile)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	PDomain_Port pPort = &pCore->Ports[PATA_MapPortId(pReq->Device_Id)];
	PMV_Command_Table pCmdTable = Port_GetCommandTable(pPort, tag);

	PSATA_FIS_REG_H2D pFIS = (PSATA_FIS_REG_H2D)pCmdTable->FIS;
#ifdef SUPPORT_PM
	PDomain_Device pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];
#endif
	/* 
	 * 1. SoftReset is not supported yet.
	 * 2. PM_Port
	 */
	pFIS->FIS_Type = SATA_FIS_TYPE_REG_H2D;

#ifdef SUPPORT_PM
	pFIS->PM_Port = pDevice->PM_Number;
#else
	pFIS->PM_Port = 0;
#endif

	pFIS->C = 1;	/* Update command register rather than devcie control register */
	pFIS->Command = pTaskFile->Command;
	pFIS->Features = pTaskFile->Features;
	pFIS->Device = pTaskFile->Device;
	pFIS->Control = pTaskFile->Control;

	pFIS->LBA_Low = pTaskFile->LBA_Low;
	pFIS->LBA_Mid = pTaskFile->LBA_Mid;
	pFIS->LBA_High = pTaskFile->LBA_High;
	pFIS->Sector_Count = pTaskFile->Sector_Count;

	/* No matter it's 48bit or not, I've set the values. */
	pFIS->LBA_Low_Exp = pTaskFile->LBA_Low_Exp;
	pFIS->LBA_Mid_Exp = pTaskFile->LBA_Mid_Exp;
	pFIS->LBA_High_Exp = pTaskFile->LBA_High_Exp;
	pFIS->Features_Exp = pTaskFile->Feature_Exp;
	pFIS->Sector_Count_Exp = pTaskFile->Sector_Count_Exp;
}

/* Set MV_Request.Cmd_Flag */
MV_BOOLEAN Category_CDB_Type(
	IN PDomain_Device pDevice,
	IN PMV_Request pReq
	)
{
	PDomain_Port pPort = pDevice->PPort;

	switch ( pReq->Cdb[0] )
	{
		case SCSI_CMD_READ_10:
		case SCSI_CMD_READ_16:
			pReq->Cmd_Flag |= CMD_FLAG_DATA_IN;
			
		case SCSI_CMD_WRITE_10:
		case SCSI_CMD_WRITE_16:
		case SCSI_CMD_VERIFY_10:
			/* 
			 * 
			 * CMD_FLAG_DATA_IN
			 * CMD_FLAG_NON_DATA
			 * CMD_FLAG_DMA
			 */
			if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
				pReq->Cmd_Flag |= CMD_FLAG_PACKET;

			if ( pDevice->Capacity&DEVICE_CAPACITY_48BIT_SUPPORTED )
				pReq->Cmd_Flag |= CMD_FLAG_48BIT;

			if ( pDevice->Capacity&DEVICE_CAPACITY_NCQ_SUPPORTED )
			{
				// might be a PM - assert is no longer true
				//MV_DASSERT( pPort->Type==PORT_TYPE_SATA );		
#if 0
				if ( (pReq->Cdb[0]==SCSI_CMD_READ_10)
					|| (pReq->Cdb[0]==SCSI_CMD_WRITE_10) )
#else
				if (SCSI_IS_READ(pReq->Cdb[0]) || SCSI_IS_WRITE(pReq->Cdb[0]))
#endif
				{
					if ( (pPort->Running_Slot==0)
						|| (pPort->Setting&PORT_SETTING_NCQ_RUNNING) )
					{
						/* hardware workaround:
						 * don't do NCQ on silicon image PM */
						if( !((pPort->Setting & PORT_SETTING_PM_EXISTING) && 
							(pPort->PM_Vendor_Id == 0x1095 )) )
						{
							if ( pReq->Scsi_Status!=REQ_STATUS_RETRY )
								pReq->Cmd_Flag |= CMD_FLAG_NCQ;
						}
					}
				}
			}

			break;

		case SCSI_CMD_MARVELL_SPECIFIC:
			{
				/* This request should be for core module */
				if ( pReq->Cdb[1]!=CDB_CORE_MODULE )
					return MV_FALSE;
				switch ( pReq->Cdb[2] )
				{
					case CDB_CORE_IDENTIFY:
					case CDB_CORE_READ_LOG_EXT:
						pReq->Cmd_Flag |= CMD_FLAG_DATA_IN;
						break;
					case CDB_CORE_SET_UDMA_MODE:
					case CDB_CORE_SET_PIO_MODE:
					case CDB_CORE_ENABLE_WRITE_CACHE:
					case CDB_CORE_DISABLE_WRITE_CACHE:
					case CDB_CORE_ENABLE_READ_AHEAD:
					case CDB_CORE_DISABLE_READ_AHEAD:
						pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
						break;
				#ifdef SUPPORT_ATA_SMART
					case CDB_CORE_ATA_IDENTIFY_PACKET_DEVICE:
						pReq->Cmd_Flag |= (CMD_FLAG_DATA_IN | CMD_FLAG_SMART);
						break;
					case CDB_CORE_ENABLE_SMART:
					case CDB_CORE_DISABLE_SMART:
					case CDB_CORE_SMART_RETURN_STATUS:
					case  CDB_CORE_ATA_SMART_AUTO_OFFLINE:
					case CDB_CORE_ATA_SMART_AUTOSAVE:      
					case CDB_CORE_ATA_SMART_IMMEDIATE_OFFLINE:
						pReq->Cmd_Flag |= (CMD_FLAG_NON_DATA | CMD_FLAG_SMART);
						break;
					case CDB_CORE_ATA_SMART_READ_VALUES:
					case CDB_CORE_ATA_SMART_READ_THRESHOLDS:
					case CDB_CORE_ATA_SMART_READ_LOG_SECTOR:
						pReq->Cmd_Flag |= (CMD_FLAG_DATA_IN | CMD_FLAG_SMART);
						break;
					case CDB_CORE_ATA_SMART_WRITE_LOG_SECTOR:
						pReq->Cmd_Flag |= (CMD_FLAG_DATA_OUT | CMD_FLAG_SMART);
						break;
				#endif
					case CDB_CORE_ATA_IDENTIFY:
						pReq->Cmd_Flag |= (CMD_FLAG_DATA_IN | CMD_FLAG_SMART);
						break;
					case CDB_CORE_SHUTDOWN:
						if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
							return MV_FALSE;
						MV_DPRINT(("Shutdown on device %d.\n", pReq->Device_Id));
						pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
						break;
               #ifdef SUPPORT_ATA_POWER_MANAGEMENT
					case CDB_CORE_ATA_SLEEP:
					case CDB_CORE_ATA_IDLE:
					case CDB_CORE_ATA_STANDBY:
					case CDB_CORE_ATA_IDLE_IMMEDIATE:
					case CDB_CORE_ATA_CHECK_POWER_MODE:
					case CDB_CORE_ATA_STANDBY_IMMEDIATE:
                                               pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
                                               break;
               #endif
					case CDB_CORE_OS_SMART_CMD:
						if  ((pReq->Cdb[3] != ATA_CMD_IDENTIFY_ATA) ||
							(pReq->Cdb[3] != ATA_CMD_IDENTIY_ATAPI))
							pReq->Cmd_Flag |= CMD_FLAG_SMART;
						if (pDevice->Device_Type & DEVICE_TYPE_ATAPI)
							pReq->Cmd_Flag |= CMD_FLAG_PACKET;
						break;
					default:
						return MV_FALSE;
				}
				break;
			}
#ifdef SUPPORT_ATA_SECURITY_CMD
			case ATA_16:
			return MV_TRUE;
			break;
#endif
		case SCSI_CMD_START_STOP_UNIT:	
		case SCSI_CMD_SYNCHRONIZE_CACHE_10:
			if ( !(pDevice->Device_Type & DEVICE_TYPE_ATAPI )){
				if ( pDevice->Capacity&DEVICE_CAPACITY_48BIT_SUPPORTED )
					pReq->Cmd_Flag |= CMD_FLAG_48BIT;
				pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
				break;
			}	//We will send this command to CD drive directly if it is ATAPI device.	
		case SCSI_CMD_SND_DIAG:
			pReq->Cmd_Flag |= CMD_FLAG_DATA_OUT;
		case SCSI_CMD_INQUIRY:
		case SCSI_CMD_READ_CAPACITY_10:
		case SCSI_CMD_TEST_UNIT_READY:
		case SCSI_CMD_MODE_SENSE_10:
		case SCSI_CMD_MODE_SELECT_10:
		case SCSI_CMD_PREVENT_MEDIUM_REMOVAL:
		case SCSI_CMD_READ_TOC:
		case SCSI_CMD_REQUEST_SENSE:
		default:
			if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
			{
				pReq->Cmd_Flag |= CMD_FLAG_PACKET;
				break;
			}
			else
			{
				MV_DPRINT(("Error: Unknown request: 0x%x.\n", pReq->Cdb[0]));
				return MV_FALSE;
			}
	}

	return MV_TRUE;
}

static void rw16_taskfile(PMV_Request req, PATA_TaskFile tf, int tag)
{
	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
		
		tf->Features = req->Cdb[13];
		tf->Feature_Exp = req->Cdb[12];

		tf->Sector_Count = tag<<3;
		
		tf->LBA_Low = req->Cdb[9];
		tf->LBA_Mid = req->Cdb[8];
		tf->LBA_High = req->Cdb[7];
		tf->LBA_Low_Exp = req->Cdb[6];
		tf->LBA_Mid_Exp = req->Cdb[5];
		tf->LBA_High_Exp = req->Cdb[4];

		tf->Device = MV_BIT(6);

		if (req->Cdb[0] == SCSI_CMD_READ_16)
			tf->Command = ATA_CMD_READ_FPDMA_QUEUED;
		else if (req->Cdb[0] == SCSI_CMD_WRITE_16)
			tf->Command = ATA_CMD_WRITE_FPDMA_QUEUED;
	} else if (req->Cmd_Flag & CMD_FLAG_48BIT) {
		MV_DASSERT(!(req->Cmd_Flag&CMD_FLAG_NCQ));

		tf->Sector_Count = req->Cdb[13];
		tf->Sector_Count_Exp = req->Cdb[12];

		tf->LBA_Low = req->Cdb[9];
		tf->LBA_Mid = req->Cdb[8];
		tf->LBA_High = req->Cdb[7];
		tf->LBA_Low_Exp = req->Cdb[6];
		tf->LBA_Mid_Exp = req->Cdb[5];
		tf->LBA_High_Exp = req->Cdb[4];

		tf->Device = MV_BIT(6);

		if (req->Cdb[0] == SCSI_CMD_READ_16)
			tf->Command = ATA_CMD_READ_DMA_EXT;
		else if (req->Cdb[0] == SCSI_CMD_WRITE_16)
			tf->Command = ATA_CMD_WRITE_DMA_EXT;
	}
}
#ifdef SUPPORT_ATA_SECURITY_CMD
void scsi_ata_ata_passthru_16_fill_taskfile(MV_Request *req, OUT PATA_TaskFile  taskfile)
{
	taskfile->Command = req->Cdb[14];
	taskfile->Features = req->Cdb[4];
	taskfile->Device = req->Cdb[13];

	taskfile->LBA_Low = req->Cdb[8];
	taskfile->LBA_Mid = req->Cdb[10];
	taskfile->LBA_High = req->Cdb[12];
	taskfile->Sector_Count = req->Cdb[6];
	taskfile->Control = req->Cdb[15];
	if (req->Cdb[1] & 0x01) {
		taskfile->LBA_Low_Exp = req->Cdb[7];
		taskfile->LBA_Mid_Exp = req->Cdb[9];
		taskfile->LBA_High_Exp = req->Cdb[11];
		taskfile->Sector_Count_Exp = req->Cdb[5];
		taskfile->Feature_Exp = req->Cdb[3];
	} else {
		taskfile->LBA_Low_Exp = 0;
		taskfile->LBA_Mid_Exp = 0;
		taskfile->LBA_High_Exp = 0;
		taskfile->Sector_Count_Exp = 0;
		taskfile->Feature_Exp = 0;
	}
}
#endif
MV_BOOLEAN ATA_CDB2TaskFile(
	IN PDomain_Device pDevice,
	IN PMV_Request pReq, 
	IN MV_U8 tag,
	OUT PATA_TaskFile pTaskFile
	)
{
	MV_ZeroMemory(pTaskFile, sizeof(ATA_TaskFile));

	switch ( pReq->Cdb[0] )
	{
		case SCSI_CMD_READ_10:
		case SCSI_CMD_WRITE_10:
			{
				
				/* 
				 * The OS maximum tranfer length is set to 128K.
				 * For ATA_CMD_READ_DMA and ATA_CMD_WRITE_DMA,
				 * the max size they can handle is 256 sectors.
				 * And Sector_Count==0 means 256 sectors.
				 * If OS request max lenght>128K, for 28 bit device, we have to split requests.
				 */
#ifndef _OS_LINUX  //Avoid keneral panic 
				MV_DASSERT( ( (((MV_U16)pReq->Cdb[7])<<8) | (pReq->Cdb[8]) ) <= 256 );
#endif
				/*
				 * 24 bit LBA can express 128GB.
				 * 4 bytes LBA like SCSI_CMD_READ_10 can express 2TB.
				 */
			
				/* Make sure Cmd_Flag has set already. */
				#ifdef SUPPORT_ATA_SECURITY_CMD
				if ( pReq->Cmd_Flag&CMD_FLAG_NCQ && !((pDevice->Setting&DEVICE_SETTING_SECURITY_LOCKED)==0x10))
				#else
				if ( pReq->Cmd_Flag&CMD_FLAG_NCQ)
				#endif
				{
					
					pTaskFile->Features = pReq->Cdb[8];
					pTaskFile->Feature_Exp = pReq->Cdb[7];

					pTaskFile->Sector_Count = tag<<3;
					
					pTaskFile->LBA_Low = pReq->Cdb[5];
					pTaskFile->LBA_Mid = pReq->Cdb[4];
					pTaskFile->LBA_High = pReq->Cdb[3];
					pTaskFile->LBA_Low_Exp = pReq->Cdb[2];
		
					pTaskFile->Device = MV_BIT(6);

					if ( pReq->Cdb[0]==SCSI_CMD_READ_10 )
						pTaskFile->Command = ATA_CMD_READ_FPDMA_QUEUED;
					else if ( pReq->Cdb[0]==SCSI_CMD_WRITE_10 )
						pTaskFile->Command = ATA_CMD_WRITE_FPDMA_QUEUED;
				}
				else if ( pReq->Cmd_Flag&CMD_FLAG_48BIT )
				{
					MV_DASSERT( !(pReq->Cmd_Flag&CMD_FLAG_NCQ) );

					pTaskFile->Sector_Count = pReq->Cdb[8];
					pTaskFile->Sector_Count_Exp = pReq->Cdb[7];

					pTaskFile->LBA_Low = pReq->Cdb[5];
					pTaskFile->LBA_Mid = pReq->Cdb[4];
					pTaskFile->LBA_High = pReq->Cdb[3];
					pTaskFile->LBA_Low_Exp = pReq->Cdb[2];

					pTaskFile->Device = MV_BIT(6);

					if ( pReq->Cdb[0]==SCSI_CMD_READ_10 )
						pTaskFile->Command = ATA_CMD_READ_DMA_EXT;
					else if ( pReq->Cdb[0]==SCSI_CMD_WRITE_10 )
						pTaskFile->Command = ATA_CMD_WRITE_DMA_EXT;
				}
				else
				{
					/* 28 bit DMA */
					pTaskFile->Sector_Count = pReq->Cdb[8];		/* Could be zero */
	
					pTaskFile->LBA_Low = pReq->Cdb[5];
					pTaskFile->LBA_Mid = pReq->Cdb[4];
					pTaskFile->LBA_High = pReq->Cdb[3];
			
					pTaskFile->Device = MV_BIT(6) | (pReq->Cdb[2]&0xF);
					
					MV_DASSERT( (pReq->Cdb[2]&0xF0)==0 );

					if ( pReq->Cdb[0]==SCSI_CMD_READ_10 )
						pTaskFile->Command = ATA_CMD_READ_DMA;
					else if ( pReq->Cdb[0]==SCSI_CMD_WRITE_10 )
						pTaskFile->Command = ATA_CMD_WRITE_DMA;
				}

				break;
			}
#ifdef SUPPORT_ATA_SECURITY_CMD
		case ATA_16:
			scsi_ata_ata_passthru_16_fill_taskfile(pReq,pTaskFile);
			break;
#endif
		case SCSI_CMD_READ_16:
		case SCSI_CMD_WRITE_16:
			rw16_taskfile(pReq, pTaskFile, tag);
			break;

		case SCSI_CMD_VERIFY_10:
			/* 
			 * For verify command, the size may need use two MV_U8, especially Windows.
			 * For 28 bit device, we have to split the request.
			 * For 48 bit device, we use ATA_CMD_VERIFY_EXT.
			 */
			if ( pDevice->Capacity&DEVICE_CAPACITY_48BIT_SUPPORTED )
			{
				pTaskFile->Sector_Count = pReq->Cdb[8];
				pTaskFile->Sector_Count_Exp = pReq->Cdb[7];

				pTaskFile->LBA_Low = pReq->Cdb[5];
				pTaskFile->LBA_Mid = pReq->Cdb[4];
				pTaskFile->LBA_High = pReq->Cdb[3];
				pTaskFile->LBA_Low_Exp = pReq->Cdb[2];

				pTaskFile->Device = MV_BIT(6);

				pTaskFile->Command = ATA_CMD_VERIFY_EXT;
			}
			else
			{
				pTaskFile->Sector_Count = pReq->Cdb[8];
				
				pTaskFile->LBA_Low = pReq->Cdb[5];
				pTaskFile->LBA_Mid = pReq->Cdb[4];
				pTaskFile->LBA_High = pReq->Cdb[3];

				pTaskFile->Device = MV_BIT(6) | (pReq->Cdb[2]&0xF);
				
				MV_DASSERT( (pReq->Cdb[2]&0xF0)==0 );

				pTaskFile->Command = ATA_CMD_VERIFY;				
			}

			break;

		case SCSI_CMD_MARVELL_SPECIFIC:
			{
				/* This request should be for core module */
				if ( pReq->Cdb[1]!=CDB_CORE_MODULE )
					return MV_FALSE;
				
				switch ( pReq->Cdb[2] )
				{
					case CDB_CORE_IDENTIFY:
						pTaskFile->Command = ATA_CMD_IDENTIFY_ATA;
						break;
					case CDB_CORE_SET_UDMA_MODE:
						pTaskFile->Command = ATA_CMD_SET_FEATURES;
						pTaskFile->Features = ATA_CMD_SET_TRANSFER_MODE;
						pTaskFile->Sector_Count = 0x40 | pReq->Cdb[3];
						MV_DASSERT( pReq->Cdb[4]==MV_FALSE );	/* Use UDMA mode */
						break;

					case CDB_CORE_SET_PIO_MODE:
						pTaskFile->Command = ATA_CMD_SET_FEATURES;
						pTaskFile->Features = ATA_CMD_SET_TRANSFER_MODE;
						pTaskFile->Sector_Count = 0x08 | pReq->Cdb[3];
						break;

					case CDB_CORE_ENABLE_WRITE_CACHE:
						pTaskFile->Command = ATA_CMD_SET_FEATURES;
						pTaskFile->Features = ATA_CMD_ENABLE_WRITE_CACHE;
						 pDevice->Setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
						break;
			
					case CDB_CORE_DISABLE_WRITE_CACHE:
						pTaskFile->Command = ATA_CMD_SET_FEATURES;
						pTaskFile->Features = ATA_CMD_DISABLE_WRITE_CACHE;
						 pDevice->Setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
						break;
						
					case CDB_CORE_SHUTDOWN:
						if ( pDevice->Capacity&DEVICE_CAPACITY_48BIT_SUPPORTED )
							pTaskFile->Command = ATA_CMD_FLUSH_EXT;
						else
							pTaskFile->Command = ATA_CMD_FLUSH;
						break;

                      	 #ifdef SUPPORT_ATA_POWER_MANAGEMENT
					case CDB_CORE_ATA_IDLE:
						pTaskFile->Command = 0xe3;
						pTaskFile->Sector_Count = pReq->Cdb[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?4:6];
							break;
					case CDB_CORE_ATA_STANDBY:
						pTaskFile->Command = 0xe2;
							break;
					case CDB_CORE_ATA_IDLE_IMMEDIATE:
						pTaskFile->Command = 0xe1;
							break;
					case CDB_CORE_ATA_STANDBY_IMMEDIATE:
						pTaskFile->Command = 0xe0;
							break;
					case CDB_CORE_ATA_CHECK_POWER_MODE:
						pTaskFile->Command = 0xe5;
							break;
                         	        case CDB_CORE_ATA_SLEEP:
                                               	pTaskFile->Command = 0xe6;
                                               	break;
                         #endif
					case CDB_CORE_ATA_IDENTIFY:
						pTaskFile->Command = 0xec;
						break;

					case CDB_CORE_ENABLE_READ_AHEAD:	
						pTaskFile->Command = ATA_CMD_SET_FEATURES;
						pTaskFile->Features = ATA_CMD_ENABLE_READ_LOOK_AHEAD;
						break;

					case CDB_CORE_DISABLE_READ_AHEAD:
						pTaskFile->Command = ATA_CMD_SET_FEATURES;
						pTaskFile->Features = ATA_CMD_DISABLE_READ_LOOK_AHEAD;
						break;
					
					case CDB_CORE_READ_LOG_EXT:
						pTaskFile->Command = ATA_CMD_READ_LOG_EXT;
						pTaskFile->Sector_Count = 1;	/* Read one sector */
						pTaskFile->LBA_Low = 0x10;		/* Page 10h */
						break;

					case CDB_CORE_OS_SMART_CMD:
						pTaskFile->Command = pReq->Cdb[3];
						pTaskFile->Features = pReq->Cdb[4];
						pTaskFile->LBA_Low = pReq->Cdb[5];
						pTaskFile->LBA_Mid  = pReq->Cdb[6];
						pTaskFile->LBA_High = pReq->Cdb[7];
						pTaskFile->LBA_Low_Exp = pReq->Cdb[9];
						pTaskFile->Sector_Count = pReq->Cdb[8];
						break;
				#ifdef SUPPORT_ATA_SMART
					case CDB_CORE_ATA_IDENTIFY_PACKET_DEVICE:
						pTaskFile->Command = ATA_IDENTIFY_PACKET_DEVICE;
						pTaskFile->Sector_Count = 0x1;
						break;
					case CDB_CORE_ENABLE_SMART:
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_CMD_ENABLE_SMART;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->LBA_Low = 0x1;
						break;
					case CDB_CORE_DISABLE_SMART:
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_CMD_DISABLE_SMART;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->LBA_Low = 0x1;
						break;
					case CDB_CORE_SMART_RETURN_STATUS:
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_CMD_SMART_RETURN_STATUS;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						break;	
					case CDB_CORE_ATA_SMART_READ_VALUES:
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_SMART_READ_VALUES;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->LBA_Low = 0x1;
						break;
					case CDB_CORE_ATA_SMART_READ_THRESHOLDS:
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_SMART_READ_THRESHOLDS;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->LBA_Low = 0x1;
						break;
					case CDB_CORE_ATA_SMART_READ_LOG_SECTOR:
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_SMART_READ_LOG_SECTOR;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->Sector_Count = 0x1;
						pTaskFile->LBA_Low = pReq->Cdb[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?5:8];
						break;
					case CDB_CORE_ATA_SMART_WRITE_LOG_SECTOR :
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_SMART_WRITE_LOG_SECTOR;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->Sector_Count = 0x1;
						pTaskFile->LBA_Low = pReq->Cdb[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?5:8];
						break;
					case  CDB_CORE_ATA_SMART_AUTO_OFFLINE:
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_SMART_AUTO_OFFLINE;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->LBA_Low= pReq->Cdb[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?5:8];
						pTaskFile->Sector_Count = pReq->Cdb[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?4:6];
						break;
					case CDB_CORE_ATA_SMART_AUTOSAVE:    
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_SMART_AUTOSAVE;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->Sector_Count =  pReq->Cdb[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?4 : 6];
						break;
					case CDB_CORE_ATA_SMART_IMMEDIATE_OFFLINE:
						pTaskFile->Command = ATA_CMD_SMART;
						pTaskFile->Features = ATA_SMART_IMMEDIATE_OFFLINE;
						pTaskFile->LBA_Mid = 0x4F;
						pTaskFile->LBA_High = 0xC2;
						pTaskFile->LBA_Low= pReq->Cdb[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?5:8];
						pTaskFile->Sector_Count =  pReq->Cdb[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?4 : 6];
						break;
				#endif /*#ifdef SUPPORT_ATA_SMART*/
										
					default:
						return MV_FALSE;
				}
				break;
			}

		case SCSI_CMD_SYNCHRONIZE_CACHE_10:
			if ( pDevice->Capacity&DEVICE_CAPACITY_48BIT_SUPPORTED )
				pTaskFile->Command = ATA_CMD_FLUSH_EXT;
			else
				pTaskFile->Command = ATA_CMD_FLUSH;
			pTaskFile->Device = MV_BIT(6);
			break;
		case SCSI_CMD_START_STOP_UNIT:
			if (pReq->Cdb[4] & MV_BIT(0))
			{
				pTaskFile->Command = ATA_CMD_SEEK;
				pTaskFile->Device = MV_BIT(6);
			}
			else
			{
				pTaskFile->Command = ATA_CMD_STANDBY_IMMEDIATE;
			}
			break;
		case SCSI_CMD_SND_DIAG:
			Core_Fill_SendDiagTaskfile( pDevice,pReq, pTaskFile);
			break;
		case SCSI_CMD_REQUEST_SENSE:
		case SCSI_CMD_MODE_SELECT_10:
		case SCSI_CMD_MODE_SENSE_10:
			MV_DPRINT(("Error: Unknown request: 0x%x.\n", pReq->Cdb[0]));

		default:
			return MV_FALSE;
	}

	/* 
	 * Attention: Never return before this line if your return is MV_TRUE.
	 * We need set the slave DEV bit here. 
	 */
	if ( pDevice->Is_Slave )		
		pTaskFile->Device |= MV_BIT(4);

	return MV_TRUE;
}

MV_BOOLEAN ATAPI_CDB2TaskFile(
	IN PDomain_Device pDevice,
	IN PMV_Request pReq, 
	OUT PATA_TaskFile pTaskFile
	)
{
	MV_ZeroMemory(pTaskFile, sizeof(ATA_TaskFile));

	/* At the same time, set the command category as well. */
	switch ( pReq->Cdb[0] )
	{
	case SCSI_CMD_MARVELL_SPECIFIC:
		/* This request should be for core module */
		if ( pReq->Cdb[1]!=CDB_CORE_MODULE )
			return MV_FALSE;

		switch ( pReq->Cdb[2] )
		{
		case CDB_CORE_IDENTIFY:
			pTaskFile->Command = ATA_CMD_IDENTIY_ATAPI;
			break;
					
		case CDB_CORE_SET_UDMA_MODE:
			pTaskFile->Command = ATA_CMD_SET_FEATURES;
			pTaskFile->Features = ATA_CMD_SET_TRANSFER_MODE;
			if ( pReq->Cdb[4]==MV_TRUE )
				pTaskFile->Sector_Count = 0x20 | pReq->Cdb[3];	/* MDMA mode */
			else
				pTaskFile->Sector_Count = 0x40 | pReq->Cdb[3];	/* UDMA mode*/

			break;
					
		case CDB_CORE_SET_PIO_MODE:
			pTaskFile->Command = ATA_CMD_SET_FEATURES;
			pTaskFile->Features = ATA_CMD_SET_TRANSFER_MODE;
			pTaskFile->Sector_Count = 0x08 | pReq->Cdb[3];
			break;

		case CDB_CORE_OS_SMART_CMD:
		//Now we don't support SMART on ATAPI device.
		default:
			return MV_FALSE;
		}
		break;

#ifdef _OS_LINUX
	case SCSI_CMD_READ_DISC_INFO:
#endif /* _OS_LINUX */
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_VERIFY_10:
	case SCSI_CMD_INQUIRY:
	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_TEST_UNIT_READY:
	case SCSI_CMD_MODE_SENSE_10:
	case SCSI_CMD_MODE_SELECT_10:
	case SCSI_CMD_PREVENT_MEDIUM_REMOVAL:
	case SCSI_CMD_READ_TOC:
	case SCSI_CMD_START_STOP_UNIT:
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
	case SCSI_CMD_REQUEST_SENSE:
	default:
		/* 
		 * Use packet command 
		 */
		/* Features: DMA, OVL, DMADIR */
#if defined(USE_DMA_FOR_ALL_PACKET_COMMAND)
		if ( !(pReq->Cmd_Flag&CMD_FLAG_NON_DATA) )
		{
			pTaskFile->Features |= MV_BIT(0);
		}
#elif defined(USE_PIO_FOR_ALL_PACKET_COMMAND)
		/* do nothing */
#else
		if ( pReq->Cmd_Flag&CMD_FLAG_DMA )
			pTaskFile->Features |= MV_BIT(0);
#endif
		/* Byte count low and byte count high */
		if ( pReq->Data_Transfer_Length>0xFFFF )
		{
			pTaskFile->LBA_Mid = 0xFF;
			pTaskFile->LBA_High = 0xFF;
		}
		else
		{
			pTaskFile->LBA_Mid = (MV_U8)pReq->Data_Transfer_Length;
			pTaskFile->LBA_High = (MV_U8)(pReq->Data_Transfer_Length>>8);
		}

		pTaskFile->Command = ATA_CMD_PACKET;

		break;
	}

	/* 
	 * Attention: Never return before this line if your return is MV_TRUE.
	 * We need set the slave DEV bit here. 
	 */
	if ( pDevice->Is_Slave )		
		pTaskFile->Device |= MV_BIT(4);

	return MV_TRUE;
}

