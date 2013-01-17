#include "mv_include.h"

#include "core_exp.h"

#include "core_init.h"
#include "core_inter.h"

#include "core_thor.h"

#include "core_sata.h"

static void Device_IssueIdentify(PDomain_Port pPort, PDomain_Device pDevice);
static void Device_IssueSetUDMAMode(PDomain_Port pPort, PDomain_Device pDevice);
static void Device_IssueSetPIOMode(PDomain_Port pPort, PDomain_Device pDevice);
static void Device_EnableWriteCache(PDomain_Port pPort, PDomain_Device pDevice);
static void Device_EnableReadAhead(PDomain_Port pPort, PDomain_Device pDevice);

extern void Core_HandleWaitingList(PCore_Driver_Extension pCore);

static MV_BOOLEAN mvChannelStateMachine(
	PCore_Driver_Extension pCore,
	PDomain_Port pPort
	);

MV_BOOLEAN mvDeviceStateMachine(
	PCore_Driver_Extension pCore,
	PDomain_Device pDevice
	);

#ifdef COMMAND_ISSUE_WORKROUND
MV_U8 mv_core_reset_port(PDomain_Port pPort);
void mv_core_dump_reg(PDomain_Port pPort);
void mv_core_reset_command_in_timer(PDomain_Port pPort);
void mvHandleDevicePlugin_BH(MV_PVOID  ext);
void  mv_core_reset_command(PDomain_Port pPort);
void mv_core_init_reset_para(PDomain_Port pPort);
#endif


#ifdef SUPPORT_HOT_PLUG
void Device_SoftReset(PDomain_Port pPort, PDomain_Device pDevice)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;

	pDevice->State = DEVICE_STATE_RESET_DONE;
	mvDeviceStateMachine (pCore, pDevice);
}
#endif	/* #ifdef SUPPORT_HOT_PLUG */ 

PMV_Request GetInternalReqFromPool( PCore_Driver_Extension pCore )
{
	if( !List_Empty(&pCore->Internal_Req_List) )
		return ((PMV_Request)List_GetFirstEntry(&pCore->Internal_Req_List, MV_Request, Queue_Pointer));
	else
		return NULL;
}

void ReleaseInternalReqToPool( PCore_Driver_Extension pCore, PMV_Request pReq)
{
	MV_ZeroMvRequest(pReq);
	List_AddTail( &pReq->Queue_Pointer, &pCore->Internal_Req_List );	
}

/*
 * Initialize this port including possible device hard or soft reset.
 */

#ifdef SUPPORT_PM
void mvPMDevReWrReg(
	PDomain_Port pPort, 
	MV_U8 read, 
	MV_U8 PMreg, 
	MV_U32 regVal, 
	MV_U8 PMport, 
	MV_BOOLEAN control)
{
	/* Always use the last slot for PM */
	/* This will only be a problem if two PM requests are sent at the same time */
	/* using the last slot, which should not happen in current driver, */
	/* since our PM operations are done by polling. */
	MV_U16 tag = MAX_SLOT_NUMBER - 1;		/* always use the last slot */
	PMV_Command_Header header = SATA_GetCommandHeader(pPort, tag);
	PMV_Command_Table pCmdTable = Port_GetCommandTable(pPort, tag);
	PSATA_FIS_REG_H2D pFIS = (PSATA_FIS_REG_H2D)pCmdTable->FIS;
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 old_stat, loop=5000;

	mvDisableIntr(portMmio, old_stat);

	MV_ZeroMemory(header, sizeof(MV_Command_Header));
	MV_ZeroMemory(pCmdTable, sizeof(MV_Command_Table));
	
	header->FIS_Length = FIS_REG_H2D_SIZE_IN_DWORD;
	header->PM_Port = control? 0xF : PMport;
	
	*((MV_U16 *) header) = CPU_TO_LE_16( *((MV_U16 *) header) );
	header->Table_Address = CPU_TO_LE_32(pPort->Cmd_Table_DMA.parts.low + SATA_CMD_TABLE_SIZE*tag);
	header->Table_Address_High = CPU_TO_LE_32(pPort->Cmd_Table_DMA.parts.high);
	
	pFIS->FIS_Type = SATA_FIS_TYPE_REG_H2D;
	pFIS->PM_Port = control? 0xF : PMport;
	pFIS->Command =  (read)?MV_ATA_COMMAND_PM_READ_REG : MV_ATA_COMMAND_PM_WRITE_REG;
	pFIS->Features = PMreg;
	pFIS->Device = PMport;
	pFIS->C = 1;
	
	if (!read)
	{
		pFIS->LBA_Low =  (MV_U8)((regVal & 0xff00) >> 8);
		pFIS->LBA_Mid = (MV_U8)((regVal & 0xff0000) >> 16);
		pFIS->LBA_High = (MV_U8)((regVal & 0xff000000) >> 24) ;
		pFIS->Sector_Count = (MV_U8)(regVal & 0xff);
	}

	MV_DASSERT( (MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE)&(1<<tag))==0 );
	
	MV_REG_WRITE_DWORD(portMmio, PORT_CMD_ISSUE, 1<<tag);
	MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE);	/* flush */
	
	//temp = MV_REG_READ_DWORD(portMmio, PORT_SCR_ERR);
	//MV_REG_WRITE_DWORD(portMmio, PORT_SCR_ERR, temp);
	//temp = MV_REG_READ_DWORD(portMmio, PORT_IRQ_STAT);
	//MV_REG_WRITE_DWORD(portMmio, PORT_IRQ_STAT, temp);
	//MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));

	// make sure CI is cleared before moving on
	loop = 5000;
	while(loop > 0) {
		if( (MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE)&(1<<tag)) == 0 )
			break;
		if( (MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT)& 0xf) != 0x3)
			break;
		HBA_SleepMillisecond(pCore, 10);
		loop--;
	}

	if ( (MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE)&(1<<tag)) != 0 )
		MV_DPRINT(("read/write PM register: CI not cleared!\n"));

	mvEnableIntr(portMmio, old_stat);
	
}
static MV_U8 mvGetSataDeviceType(PDomain_Port pPort)
{
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 tmp;

	tmp = MV_REG_READ_DWORD(portMmio, PORT_SIG);

	if(tmp == 0x96690101)
	{
		MV_DPRINT(("Port Multiplier detected.\n"));
		return PORT_TYPE_PM;
	}

	return 0;
}
#endif /* SUPPORT_PM */


MV_BOOLEAN SATA_DoSoftReset(PDomain_Port pPort, MV_U8 PMPort)
{
	MV_U16 tag = Tag_GetOne(&pPort->Tag_Pool);
	PMV_Command_Header header = SATA_GetCommandHeader(pPort, tag);
	PMV_Command_Table pCmdTable = Port_GetCommandTable(pPort, tag);
	PSATA_FIS_REG_H2D pFIS = (PSATA_FIS_REG_H2D)pCmdTable->FIS;
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 old_stat;
	MV_U32 temp = 0, count = 0;
	MV_U8 reset = 1;

	if( PMPort == 0xF )
		MV_DASSERT( tag == 0 );

	mvDisableIntr(portMmio, old_stat);

	do
	{
		MV_ZeroMemory(header, sizeof(MV_Command_Header));
		MV_ZeroMemory(pCmdTable, sizeof(MV_Command_Table));
	
		header->FIS_Length = FIS_REG_H2D_SIZE_IN_DWORD;
		header->Reset = (reset)?1:0;
		header->PM_Port = PMPort;
		
		*((MV_U16 *) header) = CPU_TO_LE_16( *((MV_U16 *) header) );
		header->Table_Address = CPU_TO_LE_32(pPort->Cmd_Table_DMA.parts.low + SATA_CMD_TABLE_SIZE*tag);
		header->Table_Address_High = CPU_TO_LE_32(pPort->Cmd_Table_DMA.parts.high);
	
		pFIS->FIS_Type = SATA_FIS_TYPE_REG_H2D;
		pFIS->PM_Port = PMPort;
//		pFIS->Device = 0x40;
		pFIS->Control = (reset)?MV_BIT(2):0;

		MV_REG_WRITE_DWORD(portMmio, PORT_CMD_ISSUE, 1<<tag);
		MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE);	/* flush */

		HBA_SleepMillisecond(pCore, 2);


		reset = reset ^ 1; /*SRST CLEAR*/

		count = 0;
		// make sure CI is cleared before moving on
		do {
			temp = MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE) & (1<<tag);
			count++;
			HBA_SleepMillisecond(pCore, 10);
		} while (temp != 0 && count < 1000);

	}while(reset==0);
	
	Tag_ReleaseOne(&pPort->Tag_Pool, tag);
	mvEnableIntr(portMmio, old_stat);
#if 0//def MV_DEBUG
	if(pPort->command_callback && pPort->reset_hba_times < MAX_RESET_TIMES){
		pPort->Hot_Plug_Timer = 0;
		pPort->command_callback = mvHandleDevicePlugin_BH;
		SmallTimer_AddRequest(pPort, 10, mv_core_reset_command_in_timer, pPort, NULL);
		pCore->resetting_command = MV_TRUE;
		return MV_FALSE;
	}

#endif

	if (temp != 0)
	{
		MV_PRINT("Softrest:CI can not be clean[0x%x] on port[%d].\n",MV_REG_READ_DWORD( portMmio, PORT_CMD ), pPort->Id);
		mv_core_dump_reg(pPort);
		if(pPort->reset_hba_times > MAX_RESET_TIMES){
			MV_DPRINT(("Port resetted more than %d times, shut down the port.\n",MAX_RESET_TIMES));
			return MV_FALSE;
		}
		pPort->Hot_Plug_Timer = 0;
		pPort->command_callback = mvHandleDevicePlugin_BH;
		pCore->resetting_command = MV_TRUE;
		SmallTimer_AddRequest(pPort, 10, mv_core_reset_command_in_timer, pPort, NULL);
		return MV_FALSE;
	}

	return MV_TRUE;
}

#ifdef SUPPORT_PM
MV_BOOLEAN SATA_SoftResetDevice(PDomain_Port pPort, MV_U8 portNum)
{
#ifndef _OS_LINUX
	PCore_Driver_Extension pCore = pPort->Core_Extension;
#endif
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 status1, status2, loop = 20;

	if (! (SATA_DoSoftReset(pPort, portNum)) )
		return MV_FALSE;

	while(loop>0)
	{
		status1 = MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT) & 0xf;
		status2 = MV_REG_READ_DWORD(portMmio, PORT_TFDATA) & 0xff;
		if (((status1 & 0xf) == 0x3) && ((status2 & 0x80) == 0))
		{
			MV_DPRINT(("loop = %x\n", loop));
			pPort->Type = mvGetSataDeviceType( pPort );
			return MV_TRUE;
		}
		HBA_SleepMicrosecond(pCore, 1000);
		loop--;
	}
	MV_DPRINT(("Did not detect device after soft reset\n"));
	return MV_FALSE;
}

MV_BOOLEAN SATA_SoftResetPMDevice(PDomain_Port pPort, MV_U8 portNum) 
{
#ifndef _OS_LINUX
	PCore_Driver_Extension pCore = pPort->Core_Extension;
#endif
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 status, loop = 5000;
//	MV_U32 PMPort;

	
	if (! (SATA_DoSoftReset(pPort, portNum)) )
		return MV_FALSE;

	
	while(loop>0)
	{
		status = MV_REG_READ_DWORD(portMmio, PORT_TFDATA) & 0xff;
		/*PMPort = (MV_REG_READ_DWORD(portMmio, PORT_TFDATA) & 0xf00000) >> 20;*/

		if ( ((status & 0x80) == 0)/* && (PMPort == portNum)*/ )
		{

			MV_DPRINT(("loop = %x\n", loop));
			return MV_TRUE;
		}
		HBA_SleepMicrosecond(pCore, 1000);
		
		loop--;
	}

	MV_DPRINT(("Did not detect device after soft reset\n"));
	return MV_FALSE;
}

MV_BOOLEAN PMPortDeviceDetected(PDomain_Port pPort, MV_U8 portNum)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 read_result, temp;
	MV_BOOLEAN valid;
	MV_U32 loop = 100;

	while(loop>0)
	{
		/*Detect the sata device*/
		valid = MV_FALSE;
		do
		{
			mvPMDevReWrReg(pPort, MV_Read_Reg, MV_SATA_PSCR_SSTATUS_REG_NUM, 0, portNum, MV_TRUE);
			temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);

			if (((temp >> 16) & 0xF0) == 0xF0)
				valid = MV_TRUE;
              
			read_result = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
		} while (valid == MV_FALSE);

		if( (read_result & 0xFFF) == 0x123 || (read_result & 0xFFF) == 0x113 )
		{
			MV_DPRINT(("the device detected on PM port %d, Port %d ", portNum, pPort->Id));

			/* clears X bit in SError */
			mvPMDevReWrReg( pPort, MV_Write_Reg, MV_SATA_PSCR_SERROR_REG_NUM, 0xFFFFFFFF, portNum, MV_TRUE);
			return MV_TRUE;
		}
		HBA_SleepMillisecond(pCore, 1);
		loop--;
	}
	return MV_FALSE;
}
#endif /* SUPPORT_PM */

void SATA_InitPMPort (PDomain_Port pPort, MV_U8 portNum)
{
	PDomain_Device pDevice = &pPort->Device[portNum];
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 signature, tmp;
#ifdef SUPPORT_ERROR_HANDLING
	#ifdef RAID_DRIVER
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_RAID);	
	#else
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_HBA);
	#endif
	struct mod_notif_param param;
#endif
	if( PMPortDeviceDetected(pPort, portNum) ) 
	{
		/*
		This delay is for SiliconImage PM
		especiall ATAPI device
		*/
		HBA_SleepMillisecond(pCore, 50);
		if ( SATA_SoftResetPMDevice(pPort, portNum) )
		{
			signature = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_PM_FIS_0);

			if ( signature==0xEB140101 )				/* ATAPI signature */	
				pDevice->Device_Type |= DEVICE_TYPE_ATAPI;			
			else
				MV_DASSERT( signature==0x00000101 );	/* ATA signature */
	
			pDevice->Internal_Req = GetInternalReqFromPool(pCore);
			if( pDevice->Internal_Req == NULL )
			{
				MV_DPRINT(("ERROR: Unable to get an internal request buffer\n"));
				// can't initialize without internal buffer - just set this disk down
				pDevice->Status = DEVICE_STATUS_NO_DEVICE;
				pDevice->State = DEVICE_STATE_INIT_DONE;
			}
			else 
			{
				pDevice->Status = DEVICE_STATUS_EXISTING|DEVICE_STATUS_FUNCTIONAL;
				pDevice->State = DEVICE_STATE_RESET_DONE;
				pDevice->PM_Number = portNum;
				pPort->Device_Number++;
			}
		}
		else 
		{
			MV_DPRINT(("soft reset failed on PM port %d\n", portNum));

#ifdef SUPPORT_ERROR_HANDLING
			if( pDevice->Status & DEVICE_STATUS_FUNCTIONAL )
			{
				MV_ASSERT( pDevice->Internal_Req!=NULL );
				pCore->Total_Device_Count--;
				ReleaseInternalReqToPool( pCore, pDevice->Internal_Req );
				pDevice->Internal_Req = NULL;
				param.lo = pDevice->Id;
				#ifdef RAID_DRIVER
				RAID_ModuleNotification(pUpperLayer, EVENT_DEVICE_REMOVAL, &param);
				#else
				HBA_ModuleNotification(pUpperLayer, 
						       EVENT_DEVICE_REMOVAL, 
						       &param);
				#endif
			}
#endif
			pDevice->Status = DEVICE_STATUS_EXISTING;
			pDevice->State = DEVICE_STATE_INIT_DONE;
			pDevice->Need_Notify = MV_FALSE;
			
			/* toggle the start bit in cmd register */
			tmp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
			MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp & ~MV_BIT(0));
			MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp | MV_BIT(0));
			HBA_SleepMillisecond( pCore, 100 );
		}
	}
	else
		pDevice->Need_Notify = MV_FALSE;
}

void SATA_InitPM (PDomain_Port pPort)
{
	PDomain_Device pDevice;
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 numPMPorts, temp;
	MV_U8 i;
	MV_BOOLEAN valid;

	pPort->Setting |= PORT_SETTING_PM_EXISTING;
	pPort->Setting |= PORT_SETTING_PM_FUNCTIONAL;
	
	/* fill in various information about the PM */

	/* check how many ports the PM has */
	valid = MV_FALSE;
	do
	{
		mvPMDevReWrReg(pPort, MV_Read_Reg, MV_SATA_GSCR_INFO_REG_NUM, 0, 0xF, MV_TRUE);
		temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);

		if (((temp >> 16) & 0xF0) == 0xF0)
			valid = MV_TRUE;
          
		numPMPorts = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 ) & 0xF;		
	} while (valid == MV_FALSE);
	
	if ( numPMPorts > MAX_DEVICE_PER_PORT )
		numPMPorts = MAX_DEVICE_PER_PORT;
	else if ( numPMPorts < MAX_DEVICE_PER_PORT )
	{
		for( i=(MV_U8)numPMPorts; i<MAX_DEVICE_PER_PORT; i++ )
		{
			pPort->Device[i].Status = DEVICE_STATUS_NO_DEVICE;
			pPort->Device[i].State = DEVICE_STATE_INIT_DONE;
		}
	}
	pPort->PM_Num_Ports = (MV_U8)numPMPorts;

	/* vendor ID & device ID */
	valid = MV_FALSE;
	do
	{
		mvPMDevReWrReg( pPort, MV_Read_Reg, MV_SATA_GSCR_ID_REG_NUM, 0, 0xF, MV_TRUE );
		temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);

		if (((temp >> 16) & 0xF0) == 0xF0)
			valid = MV_TRUE;
          
		temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
	} while (valid == MV_FALSE);
	
	pPort->PM_Vendor_Id = (MV_U16)temp;
	pPort->PM_Device_Id = (MV_U16)(temp >> 16);

	/* product & spec revisions */
	valid = MV_FALSE;
	do
	{
		mvPMDevReWrReg( pPort, MV_Read_Reg, MV_SATA_GSCR_REVISION_REG_NUM, 0, 0xF, MV_TRUE );
		temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);

		if (((temp >> 16) & 0xF0) == 0xF0)
			valid = MV_TRUE;
          
		temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
	} while (valid == MV_FALSE);
	
	pPort->PM_Product_Revision = (MV_U8)((temp & 0xFF00) >> 8);
	if ( temp & MV_BIT(3) )
		pPort->PM_Spec_Revision = 12;
	else if ( temp & MV_BIT(2) )
		pPort->PM_Spec_Revision = 11;
	else if ( temp & MV_BIT(1) )
		pPort->PM_Spec_Revision = 10;
	else
		pPort->PM_Spec_Revision = 0;

	/* enable asychronous notification bit for hot plug */
	valid = MV_FALSE;
	do
	{
		mvPMDevReWrReg( pPort, MV_Read_Reg, MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM, 0, 0xF, MV_TRUE );
		temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);

		if (((temp >> 16) & 0xF0) == 0xF0)
			valid = MV_TRUE;
          
		temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
	} while (valid == MV_FALSE);
	
	mvPMDevReWrReg( pPort, MV_Write_Reg, MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM, 
					temp | MV_BIT(3), 0xF, MV_TRUE );

	/* enable N & X bit in SError for hot plug */
	valid = MV_FALSE;
	do
	{
		mvPMDevReWrReg( pPort, MV_Read_Reg, MV_SATA_GSCR_ERROR_ENABLE_REG_NUM, 0, 0xF, MV_TRUE );
		temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);

		if (((temp >> 16) & 0xF0) == 0xF0)
			valid = MV_TRUE;
          
		temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
	} while (valid == MV_FALSE);
	
	mvPMDevReWrReg( pPort, MV_Write_Reg, MV_SATA_GSCR_ERROR_ENABLE_REG_NUM, 
					MV_BIT(16) | MV_BIT(26), 0xF, MV_TRUE );

	for( i=0; i<numPMPorts; i++ )
	{
		pDevice = &pPort->Device[i];
		pDevice->Status = DEVICE_STATUS_NO_DEVICE;
		pDevice->State = DEVICE_STATE_INIT_DONE;

	
		/*
		When enter hibernation, do not enable device port again
		for saving hibernation time
		*/
		if(!pCore->Is_Dump){
			/*enable the device port*/
			mvPMDevReWrReg(pPort, MV_Write_Reg, MV_SATA_PSCR_SCONTROL_REG_NUM, 0x01, i, MV_TRUE);	
			HBA_SleepMillisecond(pCore, 1);
			mvPMDevReWrReg(pPort, MV_Write_Reg, MV_SATA_PSCR_SCONTROL_REG_NUM, 0x00, i, MV_TRUE);
			HBA_SleepMillisecond(pCore, 1);
		}
	
		SATA_InitPMPort( pPort, i );
	}

	/* Wait for each port to finish setting flags before starting state machine*/
	for( i=0; i<numPMPorts; i++ )
	{
		pDevice = &pPort->Device[i];
		if( pDevice->Status & DEVICE_STATUS_FUNCTIONAL )
			mvDeviceStateMachine( pCore, pDevice );
	}

	if( pPort->Device_Number == 0 )
		mvDeviceStateMachine( pCore, &pPort->Device[0] );
}

MV_BOOLEAN SATA_PortSoftReset( PCore_Driver_Extension pCore, PDomain_Port pPort )
{
	PDomain_Device pDevice = &pPort->Device[0];
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 tmp;
	MV_U8 i;

	if (! (SATA_SoftResetDevice(pPort, 0xF)) )
	{
		/* toggle the start bit in cmd register */
		tmp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
		MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp & ~MV_BIT(0));
		MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp | MV_BIT(0));
		HBA_SleepMillisecond( pCore, 100 );

		if( (pPort->Type != PORT_TYPE_PM) && (pDevice->Status & DEVICE_STATUS_FUNCTIONAL) 
#ifdef COMMAND_ISSUE_WORKROUND
			&& !mv_core_check_is_reseeting(pCore)
#endif
			){
			SATA_PortReportNoDevice( pCore, pPort );		
		}
		/* had trouble detecting device on this port, so we report existing
		   but not functional */
		pDevice->Status = DEVICE_STATUS_EXISTING;
		pDevice->State = DEVICE_STATE_INIT_DONE;

		/* set the rest of the device on this port */
		for (i=1; i<MAX_DEVICE_PER_PORT; i++)
		{	
			pDevice = &pPort->Device[i];
			pDevice->Status = DEVICE_STATUS_NO_DEVICE;
			pDevice->State = DEVICE_STATE_INIT_DONE;
		}
				
		mvDeviceStateMachine(pCore, pDevice);
		return MV_FALSE;
	}

	/* toggle the start bit in cmd register to make sure hardware
	   is clean after soft reset */
	tmp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp & ~MV_BIT(0));
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp | MV_BIT(0));
	HBA_SleepMillisecond( pCore, 100 );
	
	return MV_TRUE;
}

void SATA_PortReportNoDevice (PCore_Driver_Extension pCore, PDomain_Port pPort)
{
	PDomain_Device pDevice;
	MV_U8 temp, i;
#ifdef RAID_DRIVER
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_RAID);
#else
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_HBA);
#endif /* RAID_DRIVER */
	struct mod_notif_param param;

	mvRemovePortWaitingList( pCore, pPort->Id );

	/* if PM - clear all the device attached to the port */
	if( pPort->Type == PORT_TYPE_PM )
		temp = MAX_DEVICE_PER_PORT-1;
	else
		temp = 0;
		
	for( i=0; i<=temp; i++ )
	{
		pDevice = &pPort->Device[i];

		if( pDevice->Status & DEVICE_STATUS_FUNCTIONAL )
		{
			if( pDevice->Internal_Req != NULL )
			{
				pCore->Total_Device_Count--;
				ReleaseInternalReqToPool( pCore, pDevice->Internal_Req );
				pDevice->Internal_Req = NULL;
			}
			param.lo = pDevice->Id;
#ifdef RAID_DRIVER
			RAID_ModuleNotification(pUpperLayer, EVENT_DEVICE_REMOVAL, &param);
#else
			HBA_ModuleNotification(pUpperLayer, 
					       EVENT_DEVICE_REMOVAL, 
					       &param);
#endif /* RAID_DRIVER */
			pPort->Device_Number--;
		}
		
		pDevice->Status = DEVICE_STATUS_NO_DEVICE;
		pDevice->State = DEVICE_STATE_INIT_DONE;
	}
}

#ifdef SUPPORT_PM
void mvHandlePMUnplug (PCore_Driver_Extension pCore, PDomain_Device pDevice);
#endif

void SATA_PortReset(
	PDomain_Port pPort,
	MV_BOOLEAN hardReset
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PDomain_Device pDevice = &pPort->Device[0];
	MV_U32 signature;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 tmp, old_stat;
	MV_U8 i;
	MV_U8 skip = MV_FALSE;

	/* No running commands at this moment */
//	MV_ASSERT( pPort->Running_Slot==0 );
//	MV_ASSERT( pPort->Port_State==PORT_STATE_IDLE );

	pPort->Device_Number = 0;

	MV_DPRINT(("Enter SATA_PortReset.\n"));
	/* If we already reached the max number of devices supported,
	   disregard the rest */

	{
#ifdef SUPPORT_PM
		/*The timeout retry count more than CORE_MAX_RESET_COUNT*/
		if( pPort->Type == PORT_TYPE_PM ){
			MV_BOOLEAN RetryCountOverflow = MV_FALSE;
			for( i=0; i<MAX_DEVICE_PER_PORT; i++ )
			{
				if((pPort->Device[i].Reset_Count > CORE_MAX_RESET_COUNT)
				    && (pPort->Device[i].Status & DEVICE_STATUS_FUNCTIONAL )){
					mvHandlePMUnplug(pCore, &pPort->Device[i]);
					MV_DPRINT(("Remove device %d on port %d is gone\n",i ,pPort->Id));
					RetryCountOverflow = MV_TRUE;
				}
			}
			if(RetryCountOverflow)
				return;
		}
		else
#endif
		{
			if((pDevice->Reset_Count > CORE_MAX_RESET_COUNT)
			    &&  (pDevice->Status & DEVICE_STATUS_FUNCTIONAL)){
				SATA_PortReportNoDevice( pCore, pPort );
				MV_DPRINT(("Remove port %d\n",pPort->Id));
				return;
			}
		}
	}

	
	if( pCore->Total_Device_Count >= MAX_DEVICE_SUPPORTED )
	{
		for( i=0; i<MAX_DEVICE_PER_PORT; i++ )
		{
			pPort->Device[i].State = DEVICE_STATE_INIT_DONE;
			pPort->Device[i].Status = DEVICE_STATUS_NO_DEVICE;
		}
		MV_DPRINT(("We have too many devices %d.\n", pCore->Total_Device_Count));
		return;
	}

	if ( hardReset )
	{

			MV_U32 SControl,CounterForHardReset = 1000;
			SControl = MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);
			SControl &= ~0x0000000F;
			SControl |= 0x01;
			MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
			HBA_SleepMillisecond(pCore, 1);
			SControl &= ~0x0000000F;
			SControl |= 0x00;
			MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
			while(((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT)&0x0F) != 0x03) && CounterForHardReset>0){
				HBA_SleepMillisecond(pCore, 1);
				CounterForHardReset--;
			}

	}

#ifdef FORCE_1_5_G
	/* It'll trigger OOB. Looks like PATA hardware reset. 
	 * Downgrade 3G to 1.5G
	 * If Port Multiplier is attached, only the PM is downgraded. */
	{
		SStatus = MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT);
		if ( (SStatus&0xF0)==0x20 )
		{
			/* 3G */
			SControl = MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);
			SControl &= ~0x000000FF;
			SControl |= 0x11;
			MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
            HBA_SleepMillisecond(pCore, 2);
			SControl &= ~0x000000FF;
			SControl |= 0x10;
			MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
			HBA_SleepMillisecond(pCore, 2);
		}
	}
#endif

	if( !SATA_PortDeviceDetected(pPort) )
	{
#if defined(SUPPORT_ERROR_HANDLING)
		if( pPort->Setting & PORT_SETTING_PM_FUNCTIONAL )
		{
			pPort->Setting &= ~PORT_SETTING_PM_FUNCTIONAL;
			pPort->Setting &= ~PORT_SETTING_PM_EXISTING;
			MV_DPRINT(("PM on port %d is gone\n", pPort->Id));
			SATA_PortReportNoDevice( pCore, pPort );
		}
		else if( pDevice->Status & DEVICE_STATUS_FUNCTIONAL ) 
		{
			MV_DPRINT(("device on port %d is gone\n", pPort->Id));
			SATA_PortReportNoDevice( pCore, pPort );
		}
#endif

		// fixed: have to set each device individually - or hot plug will have problem
		for (i=0; i<MAX_DEVICE_PER_PORT; i++)
		{	
			pDevice = &pPort->Device[i];
			pDevice->State = DEVICE_STATE_INIT_DONE;
		}

		mvDeviceStateMachine(pCore, pDevice);
		return;
	}

	MV_DPRINT(("find device on port %d.\n", pPort->Id));
	if( !SATA_PortDeviceReady(pPort) )
	{	

#if defined(SUPPORT_ERROR_HANDLING)
		if( pPort->Setting & PORT_SETTING_PM_FUNCTIONAL ) 
		{
			pPort->Setting &= ~PORT_SETTING_PM_FUNCTIONAL;
			MV_DPRINT(("PM on port %d is non-functional\n", pPort->Id));
			SATA_PortReportNoDevice( pCore, pPort );
		}
		else if( pDevice->Status & DEVICE_STATUS_FUNCTIONAL )
		{
			MV_DPRINT(("device on port %d is non-functional\n", pPort->Id));
			SATA_PortReportNoDevice( pCore, pPort );
			pDevice->Status = DEVICE_STATUS_EXISTING;		
		}
#endif
		for (i=0; i<MAX_DEVICE_PER_PORT; i++)
		{	
			pDevice = &pPort->Device[i];
			pDevice->State = DEVICE_STATE_INIT_DONE;
		}
				
		mvDeviceStateMachine(pCore, pDevice);
		return;
	}
	MV_DPRINT(("find device ready on port %d.\n", pPort->Id));


#ifdef SUPPORT_PM
	/* link error work around */
	mvDisableIntr( portMmio, old_stat );
	MV_REG_WRITE_DWORD( pPort->Mmio_Base, PORT_VSR_ADDR, 0x5 );
	tmp = MV_REG_READ_DWORD( pPort->Mmio_Base, PORT_VSR_DATA );
	MV_REG_WRITE_DWORD( pPort->Mmio_Base, PORT_VSR_DATA, tmp | MV_BIT(26));
	HBA_SleepMillisecond( pCore, 1 );
	mvEnableIntr( portMmio, old_stat );
	if(!pCore->Is_Dump)
		HBA_SleepMillisecond(pCore, 1000);

	if ( (pCore->State!=CORE_STATE_STARTED) &&
		 (pCore->Flag_Fastboot_Skip & FLAG_SKIP_PM) )
		 skip = MV_TRUE;

	if(!skip) /*Not skip in running time*/
	{
		MV_DPRINT(("SATA_PortReset not skipped\n"));

		/* Always turn the PM bit on - otherwise won't work! */
		tmp = MV_REG_READ_DWORD(portMmio, PORT_CMD);					
		MV_REG_WRITE_DWORD(portMmio, PORT_CMD, tmp | MV_BIT(17));
		tmp=MV_REG_READ_DWORD(portMmio, PORT_CMD);	/* flush */


		if(!pCore->Is_Dump) {
			HBA_SleepMillisecond(pCore, 2000);
			//hba_msleep(2000);
		}

		if (! (SATA_PortSoftReset( pCore, pPort )) )
		{
#if defined(SUPPORT_ERROR_HANDLING) || defined(_OS_LINUX)
			if( pPort->Setting & PORT_SETTING_PM_FUNCTIONAL ) 
			{
				pPort->Setting &= ~PORT_SETTING_PM_FUNCTIONAL;
				SATA_PortReportNoDevice( pCore, pPort );
			}
#endif
			return;
		}
	} 
	else {	
		MV_DPRINT(("SATA_PortReset is skipped.\n"));
	}

	if( pPort->Type == PORT_TYPE_PM ) 
	{
		SATA_InitPM( pPort );
	} 
	else
#endif	
	{

#ifdef SUPPORT_PM
	/* not a PM - turn off the PM bit in command register */
	tmp = MV_REG_READ_DWORD(portMmio, PORT_CMD);					
	MV_REG_WRITE_DWORD(portMmio, PORT_CMD, tmp & (~MV_BIT(17)));
	tmp=MV_REG_READ_DWORD(portMmio, PORT_CMD);	/* flush */
#endif

	signature = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_SIG);



	if ( signature==0xEB140101 )				/* ATAPI signature */
	{		
		pDevice->Device_Type |= DEVICE_TYPE_ATAPI;
	}
	else
	{
		MV_DASSERT( signature==0x00000101 );	/* ATA signature */
	}

	/* set the rest of the devices on this port */
	for ( i=0; i<MAX_DEVICE_PER_PORT; i++ )
	{
		pPort->Device[i].Status = DEVICE_STATUS_NO_DEVICE;
		pPort->Device[i].State = DEVICE_STATE_INIT_DONE;
	}
	
	/* Device is ready */
	pPort->Device[0].Internal_Req = GetInternalReqFromPool(pCore);
	if( pPort->Device[0].Internal_Req == NULL )
	{
		MV_DPRINT(("ERROR: Unable to get an internal request buffer\n"));
		// can't initialize without internal buffer - just set this disk down
		//pDevice->Status = DEVICE_STATUS_NO_DEVICE;
		//pDevice->State = DEVICE_STATE_INIT_DONE;
	}
	else 
	{
		pPort->Device[0].Status = DEVICE_STATUS_EXISTING|DEVICE_STATUS_FUNCTIONAL;
		pPort->Device[0].State = DEVICE_STATE_RESET_DONE;
		pPort->Device_Number = 1;	/* We have one device here. */
	}

	mvDeviceStateMachine(pCore, &pPort->Device[0]);/*This is single drive.*/
	}
}

MV_VOID PATA_MakeControllerCommandBlock(
	MV_PU16 pControllerCmd,
	MV_U8 address, 
	MV_U8 data, 
	MV_BOOLEAN master, 
	MV_BOOLEAN write
	)
{
	*pControllerCmd = 0L;

	if ( write )
		*((MV_PU8)pControllerCmd) = data;
	*pControllerCmd |= (address<<8);
	if ( !master )
		*pControllerCmd |= MV_BIT(13);
	if ( !write )
		*pControllerCmd |= MV_BIT(14);
}

/* Poll the ATA register using enhanced mode. Exp register and Data is not included. */
MV_BOOLEAN PATA_PollControllerCommand(
	PDomain_Port pPort, 
	MV_U8 slot,
	MV_U8 registerAddress,
	MV_U8 registerData,
	MV_BOOLEAN master,
	MV_BOOLEAN write,
	PATA_TaskFile pTaskFile
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_LPVOID port_mmio = pPort->Mmio_Base;
	/* Cannot use Is_Slave to judge whether it's a master device. The flag may be not ready yet. */

	PMV_PATA_Command_Header header = PATA_GetCommandHeader(pPort, slot);
	PMV_Command_Table pCmdTable = Port_GetCommandTable(pPort, slot);
	MV_PU16 pCmdTableU16 = (MV_PU16)pCmdTable;
	MV_U32 loop = 1000, i;
	MV_U32 temp = 0;

	/* Always use the first slot */
	MV_ASSERT( (pPort->Running_Slot&(1L<<slot))==0 );
	MV_ZeroMemory( pTaskFile, sizeof(ATA_TaskFile) );
	MV_ZeroMemory(header, sizeof(MV_PATA_Command_Header));

	/* Command list */
	header->Controller_Command = 1;
	header->PIO_Sector_Count = 1;	/* How many command */

	if ( !master )
		header->Is_Slave = 1;

	header->Table_Address = pPort->Cmd_Table_DMA.parts.low;
	header->Table_Address_High = pPort->Cmd_Table_DMA.parts.high;

	PATA_MakeControllerCommandBlock(pCmdTableU16++, registerAddress, registerData, master, write);
	
	MV_REG_WRITE_DWORD(port_mmio, PORT_CMD_ISSUE, MV_BIT(0));
	MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE);	/* flush */

	/* Loop command issue to check whether it's finished. Hardware won't trigger interrupt. */
	while ( loop>0 )
	{
		/* check interrupt */
		temp = MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE);

		if ( temp ==0 )	/* It's done. */
		{
			/* Anyway it's still better to clear the interrupt. */
			temp = MV_REG_READ_DWORD(port_mmio, PORT_IRQ_STAT);
			MV_REG_WRITE_DWORD(port_mmio, PORT_IRQ_STAT, temp);
			MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));

			if ( master )
			{
				for (i=0; i<1000; i++)/*Workaround for WD PATA disk*/
				{
					temp = MV_REG_READ_DWORD(port_mmio, PORT_MASTER_TF0);
					if(temp == 0x7F) break;
					if((temp & 0x80) == 0) break;
					HBA_SleepMillisecond(pCore, 1);
				}

				pTaskFile->Command = (MV_U8)temp;
				pTaskFile->Device = (MV_U8)(temp>>8);
				pTaskFile->Features = (MV_U8)(temp>>24);
				temp = MV_REG_READ_DWORD(port_mmio, PORT_MASTER_TF1);
				pTaskFile->LBA_Low = (MV_U8)(temp>>8);
				pTaskFile->Sector_Count = (MV_U8)(temp>>24);
				temp = MV_REG_READ_DWORD(port_mmio, PORT_MASTER_TF2);
				pTaskFile->LBA_High = (MV_U8)(temp>>8);
				pTaskFile->LBA_Mid = (MV_U8)(temp>>24);
			}
			else
			{
				for (i=0; i<1000; i++)/*Workaround for WD PATA disk*/
				{
					temp = MV_REG_READ_DWORD(port_mmio, PORT_SLAVE_TF0);
					if(temp == 0x7F) break;
					if((temp & 0x80) == 0) break;
					HBA_SleepMillisecond(pCore, 1);
				}

				pTaskFile->Command = (MV_U8)temp;
				pTaskFile->Device = (MV_U8)(temp>>8);
				pTaskFile->Features = (MV_U8)(temp>>24);
				temp = MV_REG_READ_DWORD(port_mmio, PORT_SLAVE_TF1);
				pTaskFile->LBA_Low = (MV_U8)(temp>>8);
				pTaskFile->Sector_Count = (MV_U8)(temp>>24);
				temp = MV_REG_READ_DWORD(port_mmio, PORT_SLAVE_TF2);
				pTaskFile->LBA_High = (MV_U8)(temp>>8);
				pTaskFile->LBA_Mid = (MV_U8)(temp>>24);
			}
			return MV_TRUE;
		}
		HBA_SleepMillisecond(pCore, 1);
		loop--;
	}

	/* If this command is not completed and hardware is not cleared, we'll have trouble. */
	MV_DASSERT( MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE)==0 );

	return MV_FALSE;
}

MV_BOOLEAN PATA_PortDeviceWaitForBusy(
	PDomain_Port pPort, 
	MV_BOOLEAN master
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	ATA_TaskFile taskFile;
	/* PATA_PollControllerCommand will poll for 1 millisecond each time, so total 5 seconds ,1 second=1000 millisecond*/
	MV_U32 retry = 5000;	
	do {
		if ( master ) 
			PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE, 0xA0, master, MV_TRUE, &taskFile);
		else
			PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE, 0xB0, master, MV_TRUE, &taskFile);
		HBA_SleepMillisecond(pCore, 1);
		retry--;
	} while ( (taskFile.Command&MV_BIT(7)) && (retry>0) );

#if 1
	if ( taskFile.Command&MV_BIT(7) )
	{
		MV_DPRINT(("Port %d %s is busy retry=%d.\n", 
			pPort->Id, master?"master":"slave", (5000-retry)));
	}
	else
	{
		MV_DPRINT(("Port %d %s is not busy retry=%d.\n", 
			pPort->Id, master?"master":"slave", (5000-retry)));
	}
#endif

	return ( !(taskFile.Command&MV_BIT(7)) );
}

MV_BOOLEAN PATA_PortDeviceDetected(PDomain_Port pPort, MV_BOOLEAN master, MV_BOOLEAN * isATAPI)
{
	ATA_TaskFile taskFile;

	if ( master ) 
		PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE, 0xA0, master, MV_TRUE, &taskFile);
	else
		PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE, 0xB0, master, MV_TRUE, &taskFile);

#if 0
	MV_DPRINT(("PATA_PortDeviceDetected: Sector_Count=0x%x, LBA_Low=0x%x, LBA_Mid=0x%x, LBA_High=0x%x.\n",
		taskFile.Sector_Count, taskFile.LBA_Low, taskFile.LBA_Mid, taskFile.LBA_High));
#endif

	if ( //(taskFile.Sector_Count==0x01) && 
		(taskFile.LBA_Low==0x01)
		&& (taskFile.LBA_Mid==0x14)
		&& (taskFile.LBA_High==0xEB) )
	{
		/* ATAPI signature found */
		*isATAPI = MV_TRUE;
		return MV_TRUE;
	}

	if ( (taskFile.Sector_Count==0x01)
		&& (taskFile.LBA_Low==0x01)
		&& (taskFile.LBA_Mid==0x00)
		&& (taskFile.LBA_High==0x00) )
	{
		
		// if status is 0, conclude that drive is not present
		if ( taskFile.Command == 0)
			return MV_FALSE;

		/* ATA signature found */
		*isATAPI = MV_FALSE;
		return MV_TRUE;
	}
	
	return MV_FALSE;
}

MV_BOOLEAN PATA_PortDeviceReady(PDomain_Port pPort, MV_BOOLEAN master, MV_BOOLEAN * isATAPI)
{
	ATA_TaskFile taskFile;

	if ( master ) 
		PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE, 0xA0, master, MV_TRUE, &taskFile);
	else
		PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE, 0xB0, master, MV_TRUE, &taskFile);
	
#if 0
	MV_DPRINT(("PATA_PortDeviceReady: Sector_Count=0x%x, LBA_Low=0x%x, LBA_Mid=0x%x, LBA_High=0x%x.\n",
		taskFile.Sector_Count, taskFile.LBA_Low, taskFile.LBA_Mid, taskFile.LBA_High));
#endif

	if ( /* (taskFile.Sector_Count==0x01) && */
		(taskFile.LBA_Low==0x01)
		&& (taskFile.LBA_Mid==0x14)
		&& (taskFile.LBA_High==0xEB) )
	{
		/* ATAPI device */
		*isATAPI = MV_TRUE;



		return MV_TRUE;	/* ATAPI is always ready. */
	}

	if ( (taskFile.Sector_Count==0x01)
		&& (taskFile.LBA_Low==0x01)
		&& (taskFile.LBA_Mid==0x00)
		&& (taskFile.LBA_High==0x00) )
	{
		/* ATA device */
		*isATAPI = MV_FALSE;
		if ( (taskFile.Command&0x50)==0x50 )
			return MV_TRUE;
		else
            return MV_FALSE;



	}

	return MV_FALSE;
}

void PATA_PortReset(
	PDomain_Port pPort,
	MV_BOOLEAN hardReset
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PDomain_Device pDevice = NULL;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_BOOLEAN temp, isMaster;
	ATA_TaskFile taskFile;
	MV_U8 i;
	MV_U32 registerValue;
	MV_U32 retry;
	MV_U8  skip = MV_FALSE;
	MV_BOOLEAN working[2];	/* Check whether the master/slave device is functional. */
	MV_BOOLEAN isATAPI[2];	/* Check whether it's ATAPI device. */
	MV_BOOLEAN unplug[2];
#ifdef SUPPORT_ERROR_HANDLING	
#ifdef RAID_DRIVER
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_RAID);	
#else
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_HBA);
#endif /* RAID_DRIVER */
	struct mod_notif_param param;
#endif /* SUPPORT_ERROR_HANDLING */

	/* No running commands at this moment */
	MV_ASSERT( pPort->Running_Slot==0 );
	MV_ASSERT( pPort->Port_State==PORT_STATE_IDLE );

#ifdef MV_DEBUG
	{
		MV_U8 i;
		for ( i=0; i<MAX_SLOT_NUMBER; i++ )
		{
			MV_DASSERT(pPort->Running_Req[i]==NULL);
		}
	}
#endif
		
	/* If we already reached the max number of devices supported,
	   disregard the rest */
	if( pCore->Total_Device_Count >= MAX_DEVICE_SUPPORTED )
	{
		for( i=0; i<MAX_DEVICE_PER_PORT; i++ )
		{
			pPort->Device[i].State = DEVICE_STATE_INIT_DONE;
			pPort->Device[i].Status = DEVICE_STATUS_NO_DEVICE;
		}
		return;
	}

	unplug[0]=MV_FALSE;
	unplug[1]=MV_FALSE;
	pPort->Device_Number = 0;
	/*
	 * For PATA device, reset signal is shared between master and slave.
	 * So both hard reset and soft reset are port based, not device based.
	 */
	if ( hardReset )
	{
#if 1
		int loop = 0;
		registerValue = MV_REG_READ_DWORD(portMmio, PORT_CMD);
		MV_DASSERT( !(registerValue&PORT_CMD_PATA_HARD_RESET) );

		registerValue |= PORT_CMD_PATA_HARD_RESET;
		MV_REG_WRITE_DWORD(portMmio, PORT_CMD, registerValue);

		do {
			loop++;
			registerValue = MV_REG_READ_DWORD(portMmio, PORT_CMD);
		} while ( registerValue&PORT_CMD_PATA_HARD_RESET && loop < 5000);

		HBA_SleepMillisecond(pCore, 2500);
		//hba_msleep(2500);
		MV_DASSERT( MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE)==0 );
#endif
	}


	if ( (pCore->State!=CORE_STATE_STARTED) &&
		 (pCore->Flag_Fastboot_Skip & FLAG_SKIP_PATA_DEVICE) )
		 skip = MV_TRUE;

	if (skip)
	{
		for (i=0; i<MAX_DEVICE_PER_PORT; i++)
		{
			pDevice = &pPort->Device[i];
			pDevice->Status = DEVICE_STATUS_NO_DEVICE;
			pDevice->State = DEVICE_STATE_INIT_DONE;
		}
	}
	else
	{
#if 1
		/* Do soft reset. Soft reset is port based, not device based. */
		pDevice = &pPort->Device[0];
		PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE_CONTROL, MV_BIT(2), MV_TRUE, MV_TRUE, &taskFile);
		HBA_SleepMicrosecond(pCore, 10);	/* At least 5 microseconds. */
		//hba_msleep(10);

		pDevice = &pPort->Device[0];
		PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE_CONTROL, 0, MV_TRUE, MV_TRUE, &taskFile);
		HBA_SleepMillisecond(pCore, 5);		/* At least 2 millisecond. */
		//hba_msleep(5);
#endif

		isMaster = MV_TRUE;

		for ( i=2; i<MAX_DEVICE_PER_PORT; i++ )
		{
			pDevice = &pPort->Device[i];
			pDevice->Status = DEVICE_STATUS_NO_DEVICE;
			pDevice->State = DEVICE_STATE_INIT_DONE;
		}

		/* 
		 * Check master and slave devices. Master is at device[0], Slave is at device [1].
		 */
		/* Slave/Master device. Detect first. After it's totally done, we can send request to the devices. */
		for ( i=0; i<2; i++ )
		{
			pDevice = NULL;/* Shouldn't use pDevice here. */

			/* Wait for busy after the reset */
			temp = PATA_PortDeviceWaitForBusy(pPort, isMaster);

			/* 
			* Suppose after waiting for 5 seconds for the BSY signal, we only need check the signature once.
			* But I found one ATAPI device BSY is clear right away.
			* But the first time we read the signature, it's all 0x7F. 
			* Only after a while, it will return the correct value.
			*/
			if ( temp )
			{
				/*extend retry times,20 times,sometimes PATA disk is still not ready*/
				retry = 200;
				do {
					temp = PATA_PortDeviceDetected(pPort, isMaster, &isATAPI[i]);
					temp &= PATA_PortDeviceReady(pPort, isMaster, &isATAPI[i]);
					retry--;
					HBA_SleepMillisecond(pCore, 1);
					//hba_msleep(1);
				} while ( (retry>0)&&(!temp) );

				if ( !temp )
				{
					if ( isMaster ) 
						PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE, 0xA0, isMaster, MV_TRUE, &taskFile);
					else
						PATA_PollControllerCommand(pPort, 0, ATA_REGISTER_DEVICE, 0xB0, isMaster, MV_TRUE, &taskFile);

					MV_DPRINT(("PATA task file: Command=0x%x, Sector_Count=0x%x, LBA_Low=0x%x, LBA_Mid=0x%x, LBA_High=0x%x retry=%d.\n",
						taskFile.Command, taskFile.Sector_Count, taskFile.LBA_Low, taskFile.LBA_Mid, taskFile.LBA_High, (200-retry)));
				}
				else
				{
					MV_DPRINT(("PATA is detected and ready after retry= %d.\n", (200-retry)));
				}
			}

			working[i] = temp;
			isMaster = MV_FALSE;

			pDevice = &pPort->Device[i];
			if ( isATAPI[i] ) 
				pDevice->Device_Type |= DEVICE_TYPE_ATAPI;
			else
				pDevice->Device_Type &= ~DEVICE_TYPE_ATAPI;
			pDevice->Is_Slave = (i==0)?MV_FALSE:MV_TRUE;

			/* 
			 * If the device has been reset for too many times, 
			 * just set down this disk. It's better to set 
			 * MEDIA ERROR to the timeout request. 
			 */
			if ( pDevice->Reset_Count>CORE_MAX_PATA_RESET_COUNT )
				working[i] = MV_FALSE;

			if ( !working[i] ) 
			{
				if ( pDevice->Status&DEVICE_STATUS_FUNCTIONAL )
				{
					pDevice->Status = DEVICE_STATUS_NO_DEVICE;
					MV_DPRINT(("Port %d %s is gone.\n", pPort->Id, pDevice->Is_Slave?"slave":"master"));
					unplug[i] = MV_TRUE;
				}
				else
				{
					pDevice->Status = DEVICE_STATUS_NO_DEVICE;
					MV_DPRINT(("Port %d %s not ready.\n", pPort->Id, pDevice->Is_Slave?"slave":"master"));
				}
				pDevice->State = DEVICE_STATE_INIT_DONE;
			}
			else
			{

				MV_DPRINT(("Port %d %s ready.\n", pPort->Id, pDevice->Is_Slave?"slave":"master"));
				
				pDevice->Internal_Req = GetInternalReqFromPool(pCore);
				if( pDevice->Internal_Req == NULL )
				{
					MV_DPRINT(("ERROR: Unable to get an internal request buffer\n"));
					// can't initialize without internal buffer - just set this disk down
					pDevice->Status = DEVICE_STATUS_NO_DEVICE;
					pDevice->State = DEVICE_STATE_INIT_DONE;
				}
				else 
				{
					pDevice->Status = DEVICE_STATUS_EXISTING|DEVICE_STATUS_FUNCTIONAL;
					pPort->Device_Number++;
				}
			}
		}


		/* Set Device State for all devices first */
		for ( i=0; i<MAX_DEVICE_PER_PORT; i++ )
		{
			pDevice = &pPort->Device[i];
			if ( pDevice->Status & DEVICE_STATUS_FUNCTIONAL )
			{
				pDevice->State = DEVICE_STATE_RESET_DONE;
				/* Don't start mvDeviceStateMachine now. 
				 * It may trigger other devices to send DMA request before resetting is done. */
			}
		}

		/* After all the flags are set, we can do some related to the state machine and waiting list. */
		for ( i=0; i<2; i++ )
		{
			pDevice = &pPort->Device[i];
			if ( unplug[i] ) 
			{
				if( pDevice->Internal_Req!=NULL )
				{
					ReleaseInternalReqToPool( pCore, pDevice->Internal_Req );
					pDevice->Internal_Req = NULL;
				}

				mvRemoveDeviceWaitingList( pCore, pDevice->Id, MV_TRUE );

			#ifdef SUPPORT_ERROR_HANDLING	
				param.lo = pDevice->Id;
				#ifdef RAID_DRIVER
					RAID_ModuleNotification(pUpperLayer, EVENT_DEVICE_REMOVAL, &param);
				#else
					HBA_ModuleNotification(pUpperLayer, EVENT_DEVICE_REMOVAL, &param);
				#endif
			#endif
			} 
		}

		/* Then run the status machine.*/
		/* Cable detection edit: We don't run the master state machine yet, if there is
		   a slave attached */ 
		for ( i=0; i<MAX_DEVICE_PER_PORT; i++ )
		{
			pDevice = &pPort->Device[i];
			if ( pDevice->Status & DEVICE_STATUS_FUNCTIONAL ) 
			{
				if (i != 0)
					mvDeviceStateMachine(pCore, pDevice);
				else
				{
					if (!(pPort->Device[1].Status & DEVICE_STATUS_FUNCTIONAL))
						mvDeviceStateMachine(pCore, pDevice);
				}
			}
		}
	}

	if ( pPort->Device_Number==0 )
	{
		/* Just use the first device to make the ball roll. */
		
		mvDeviceStateMachine(pCore, &pPort->Device[0]);
	}

}

static MV_BOOLEAN mvChannelStateMachine(
	PCore_Driver_Extension pCore,
	PDomain_Port pPort
	)
{
	MV_U8 i;
	MV_U8 portState;
	PDomain_Device pDevice;
	PDomain_Port pOrgPort = pPort;
	#ifdef RAID_DRIVER
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_RAID);	
	#else
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_HBA);
	#endif

	MV_DPRINT(("Start mvChannelStateMachine.\n"));

	if ( pPort==NULL )
		portState = PORT_STATE_IDLE;
	else {
		portState = pPort->Port_State;
	}

	//Each step: if fail like no device, should go to the end.
	/* Channel state machine */
	switch ( portState )
	{
		case PORT_STATE_IDLE:
			/* To do reset */
			for( i=0; i<pCore->Port_Num; i++ )
			{
				pPort = &pCore->Ports[i];
				MV_DPRINT(("Port %d start mvChannelStateMachine in PORT_STATE_IDLE.\n", pPort->Id));
				MV_DASSERT( pPort->Port_State==PORT_STATE_IDLE );
				if ( pPort->Type==PORT_TYPE_PATA ){
					if(pCore->Is_Dump)
						PATA_PortReset( pPort, MV_FALSE );
					else
						PATA_PortReset( pPort, MV_TRUE );
				}
				else
					SATA_PortReset( pPort, MV_FALSE );
			}
			break;

		/* 
		 * Each port will call mvDeviceStateMachine for its devices. 
		 * When all the devices for that port are done, will call mvChannelStateMachine.
		 */

		case PORT_STATE_INIT_DONE:

			MV_DPRINT(("Discovery port[%d] is finished.\n",pPort->Id));

			/* Check whether all the ports are done. */
			for ( i=0; i<pCore->Port_Num; i++ )
			{
				pPort = &pCore->Ports[i];
				if ( pPort->Port_State != PORT_STATE_INIT_DONE )
					return MV_TRUE;
			}

			MV_DPRINT(("Discovery procedure is finished.\n"));
			MV_DPRINT(("Check need_reset=0x%x,pCore->State=0x%x.\n",pCore->Need_Reset,pCore->State));

			/* Discovery procedure is finished. */
			if(pCore->Need_Reset == 0)
			{
				if ( pCore->State==CORE_STATE_IDLE )
				{
					pCore->State = CORE_STATE_STARTED;
					core_start_cmpl_notify(pCore);	/* The first time initialization */
				}
				else
				{
					/* check which device on this port needs to be reported */
					for (i=0; i<MAX_DEVICE_PER_PORT; i++)
					{
						pDevice = &pOrgPort->Device[i];
						if ( pDevice->Need_Notify )
						{
							struct mod_notif_param param;
							param.lo = pDevice->Id;
#ifdef RAID_DRIVER
							RAID_ModuleNotification(pUpperLayer, EVENT_DEVICE_ARRIVAL, &param);
#else
							HBA_ModuleNotification(pUpperLayer, EVENT_DEVICE_ARRIVAL, &param);
#endif
							pDevice->Need_Notify = MV_FALSE;	
						}
					}
				}
			}
			else
			{
				pCore->Need_Reset = 0;
				pCore->Resetting = 0;

			}
			/* Begin to handle request again. */
			Core_HandleWaitingList(pCore);
			break;
	}

	return MV_TRUE;
}

MV_BOOLEAN mvDeviceStateMachine(
	PCore_Driver_Extension pCore,
	PDomain_Device pDevice
	)
{
	MV_U8 i;
	PDomain_Port pPort = pDevice->PPort;


	switch ( pDevice->State )
	{
		case DEVICE_STATE_RESET_DONE:
			MV_DPRINT(("Device %d DEVICE_STATE_RESET_DONE.\n", pDevice->Id));

			/* To do identify */
			Device_IssueIdentify(pDevice->PPort, pDevice); 
			break;

		case DEVICE_STATE_IDENTIFY_DONE:
			MV_DPRINT(("Device %d DEVICE_STATE_IDENTIFY_DONE.\n", pDevice->Id));

			/* To do set UDMA mode */
			Device_IssueSetPIOMode(pDevice->PPort, pDevice);
			break;

		case DEVICE_STATE_SET_UDMA_DONE:
			MV_DPRINT(("Device %d DEVICE_STATE_SET_UDMA_DONE.\n", pDevice->Id));

			/* To do set PIO mode */
			Device_EnableWriteCache(pDevice->PPort, pDevice);
			break;

		case DEVICE_STATE_SET_PIO_DONE:
			MV_DPRINT(("Device %d DEVICE_STATE_SET_PIO_DONE.\n", pDevice->Id));

			/* To do enable write cache */
			Device_IssueSetUDMAMode(pDevice->PPort, pDevice);
			break;

		case DEVICE_STATE_ENABLE_WRITE_CACHE_DONE:
			MV_DPRINT(("Device %d DEVICE_STATE_ENABLE_WRITE_CACHE_DONE.\n", pDevice->Id));

            /* To do enable read ahead */
			Device_EnableReadAhead( pDevice->PPort, pDevice );
			break;

		case DEVICE_STATE_ENABLE_READ_AHEAD_DONE:
			MV_DPRINT(("Device %d DEVICE_STATE_ENABLE_READ_AHEAD_DONE.\n", pDevice->Id));

	pDevice->State = DEVICE_STATE_INIT_DONE;
	pCore->Total_Device_Count++;

#ifdef COMMAND_ISSUE_WORKROUND
	if (mv_core_check_is_reseeting(pCore)) {
		MV_DPRINT(("Find disk after reset HBA on port[%d].\n",pPort->Id));
		mv_core_init_reset_para(pPort);
	}
#endif

        	/* No break here. */

		case DEVICE_STATE_INIT_DONE:
			MV_DPRINT(("Device %d DEVICE_STATE_INIT_DONE.\n", pDevice->Id));
			/* Check whether all devices attached to this port are done. */
			for ( i=0; i<MAX_DEVICE_PER_PORT; i++ )
			{
				if ( pPort->Device[i].State!=DEVICE_STATE_INIT_DONE )
					return MV_TRUE;
			}
			pPort->Port_State = PORT_STATE_INIT_DONE;
			mvChannelStateMachine(pCore, pDevice->PPort);
			break;

		default:
			break;
	}

	return MV_TRUE;
}

/* 
 * Global controller reset 
 */
MV_BOOLEAN
ResetController(PCore_Driver_Extension pCore)
{
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_U32 tmp;
	MV_BOOLEAN ret = MV_TRUE;

/* #if (VER_OEM==VER_OEM_ASUS) */
	MV_U8 i=0;
/* #endif */

	/*
	If enter hibernation, do not do PHY reset to save time
	*/
	if (pCore->Is_Dump)
		return MV_TRUE;

	/* Reset controller */
	tmp = MV_REG_READ_DWORD(mmio, HOST_CTL);
	if ((tmp & HOST_RESET) == 0) {

/* #if (VER_OEM==VER_OEM_ASUS) */
		if(pCore->VS_Reg_Saved!=VS_REG_SIG)
		{
			for ( i=0; i<pCore->SATA_Port_Num; i++ )
			{
				Domain_Port *port;
				port = &pCore->Ports[i];
				MV_REG_WRITE_DWORD(port->Mmio_Base, PORT_VSR_ADDR, 0xc);
				port->VS_RegC= MV_REG_READ_DWORD(port->Mmio_Base, PORT_VSR_DATA);
				pCore->VS_Reg_Saved=VS_REG_SIG;
			}
		}
/* #endif */
		MV_REG_WRITE_DWORD(mmio, HOST_CTL, tmp|HOST_RESET);
		MV_REG_READ_DWORD(mmio, HOST_CTL); /* flush */
	}

	/* Reset must complete within 1 second, or the hardware should be considered fried. */
	HBA_SleepMillisecond(pCore, 1000);
	//hba_msleep(1000);

	tmp = MV_REG_READ_DWORD(mmio, HOST_CTL);
	if (tmp & HOST_RESET) {
		MV_DASSERT(MV_FALSE);	
		ret = MV_FALSE;
	}


/* #if (VER_OEM==VER_OEM_ASUS) */
	if(pCore->VS_Reg_Saved==VS_REG_SIG)
	{
		for ( i=0; i<pCore->SATA_Port_Num; i++ )
		{
			Domain_Port *port;
			port = &pCore->Ports[i];
			MV_REG_WRITE_DWORD(port->Mmio_Base, PORT_VSR_ADDR, 0xc);
			MV_REG_WRITE_DWORD(port->Mmio_Base, PORT_VSR_DATA, port->VS_RegC);
		}
	}
	/* link error work around */
	for ( i=0; i<pCore->SATA_Port_Num; i++ )
	{
		MV_U32 tmp, old_stat;
		Domain_Port *port;
		port = &pCore->Ports[i];

		mvDisableIntr( port->Mmio_Base, old_stat );
		MV_REG_WRITE_DWORD( port->Mmio_Base, PORT_VSR_ADDR, 0x5 );
		tmp = MV_REG_READ_DWORD( port->Mmio_Base, PORT_VSR_DATA );
		MV_REG_WRITE_DWORD( port->Mmio_Base, PORT_VSR_DATA, tmp | MV_BIT(26));
		HBA_SleepMillisecond( pCore, 1 );
		mvEnableIntr( port->Mmio_Base, old_stat );
	}

/* #endif */
	return ret;
}

void PATA_ResetPort(PCore_Driver_Extension pCore, MV_U8 portId)
{
	PDomain_Port pPort = &pCore->Ports[portId];
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 tmp;

	/* Make sure port is not active. If yes, stop the port. */
	tmp = MV_REG_READ_DWORD(portMmio, PORT_CMD);
	/* For ACHI, four bits are avaiable. For 614x, PORT_CMD_FIS_ON is reserved. */
	if (tmp & (PORT_CMD_PATA_LIST_ON | PORT_CMD_PATA_START)) {
		tmp &= ~(PORT_CMD_PATA_LIST_ON | PORT_CMD_PATA_START);
		MV_REG_WRITE_DWORD(portMmio, PORT_CMD, tmp);
		MV_REG_READ_DWORD(portMmio, PORT_CMD); /* flush */

		/* spec says 500 msecs for each bit, so
			* this is slightly incorrect.
			*/
		HBA_SleepMillisecond(pCore, 500);
	}

	/* Clear error register if any */

	/* Ack any pending irq events for this port */
	tmp = MV_REG_READ_DWORD(portMmio, PORT_IRQ_STAT)&0xF;
	if (tmp)
		MV_REG_WRITE_DWORD(portMmio, PORT_IRQ_STAT, tmp);
	/* Ack pending irq in the host interrupt status register */
	MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, 1 << portId);

	/* set irq mask (enables interrupts) */
#ifdef ENABLE_PATA_ERROR_INTERRUPT
	MV_REG_WRITE_DWORD(portMmio, PORT_IRQ_MASK, DEF_PORT_PATA_IRQ);
#else
	/* 
	 * Workaround
	 * If PATA device has a error, even the error bit in the interrupt register is cleared.
	 * Internal hardware will trigger one more(OS has no idea).
	 * So because there is interrupt bit not cleared, the next command won't be issued.
	 */
	MV_REG_WRITE_DWORD(portMmio, PORT_IRQ_MASK, MV_BIT(2)|MV_BIT(0));
#endif
}

void SATA_ResetPort(PCore_Driver_Extension pCore, MV_U8 portId)
{
	PDomain_Port pPort = &pCore->Ports[portId];
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 tmp, j;

	/* Make sure port is not active. If yes, stop the port. */
	tmp = MV_REG_READ_DWORD(portMmio, PORT_CMD);
	/* For ACHI, four bits are avaiable. For 614x, PORT_CMD_FIS_ON is reserved. */
	if (tmp & (PORT_CMD_LIST_ON | PORT_CMD_FIS_ON |
			PORT_CMD_FIS_RX | PORT_CMD_START)) {
		tmp &= ~(PORT_CMD_LIST_ON | PORT_CMD_FIS_ON |
				PORT_CMD_FIS_RX | PORT_CMD_START);
		MV_REG_WRITE_DWORD(portMmio, PORT_CMD, tmp);
		MV_REG_READ_DWORD(portMmio, PORT_CMD); /* flush */

		/* spec says 500 msecs for each bit, so
			* this is slightly incorrect.
			*/
		HBA_SleepMillisecond(pCore, 500);
		//hba_msleep(500);
	}

	//PORT_CMD enable bit(5): PIO command will issue PIO setup interrupt bit. 
	// Only after clear the PIO setup interrupt bit, the hardware will issue the PIO done interrupt bit.
	// Maybe in this case, we needn't enable PIO setup interrupt bit but for some others we should.

	#ifdef AHCI
	/* For 614x, it's reserved. */
	MV_REG_WRITE_DWORD(portMmio, PORT_CMD, PORT_CMD_SPIN_UP);
	#endif

	/* Wait for SATA DET(Device Detection) */
	j = 0;
	while (j < 100) {
		HBA_SleepMillisecond(pCore, 10);
		//hba_msleep(10);
		tmp = MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT);
		if ((tmp & 0xf) == 0x3)
			break;
		j++;
	}

	
	/* Clear SATA error */
	tmp = MV_REG_READ_DWORD(portMmio, PORT_SCR_ERR);
	MV_REG_WRITE_DWORD(portMmio, PORT_SCR_ERR, tmp);

	/* Ack any pending irq events for this port */
	tmp = MV_REG_READ_DWORD(portMmio, PORT_IRQ_STAT);
	if (tmp)
		MV_REG_WRITE_DWORD(portMmio, PORT_IRQ_STAT, tmp);
	/* Ack pending irq in the host interrupt status register */
	MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, 1 << portId);

	/* set irq mask (enables interrupts) */
	MV_REG_WRITE_DWORD(portMmio, PORT_IRQ_MASK, DEF_PORT_IRQ);


	/* FIFO controller workaround for 6121-B0B1B2, 6111-B0B1, and 6145-A0 */
	if ( 
		( (pCore->Device_Id==DEVICE_ID_THORLITE_2S1P)&&(pCore->Revision_Id>=0xB0) )
		||
		( (pCore->Device_Id==DEVICE_ID_THORLITE_2S1P_WITH_FLASH)&&(pCore->Revision_Id>=0xB0) )
		||
		( (pCore->Device_Id==DEVICE_ID_THORLITE_1S1P)&&(pCore->Revision_Id>=0xB0) )	
		||
		( (pCore->Device_Id==DEVICE_ID_THOR_4S1P_NEW)&&(pCore->Revision_Id>=0xA0) )
	)
	{
		tmp = (MV_REG_READ_DWORD( portMmio, PORT_FIFO_CTL ) & 0xFFFFF0FF ) | 0x500;
		MV_REG_WRITE_DWORD( portMmio, PORT_FIFO_CTL, tmp);
		MV_REG_READ_DWORD( portMmio, PORT_FIFO_CTL);		/* flush */
	}
}
/*
 * It's equivalent to ahci_host_init and ahci_port_start
 */
void InitChip(PCore_Driver_Extension pCore)
{
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_U8 i;
	PDomain_Port pPort;
	MV_U32 tmp;

	pCore->Capacity = MV_REG_READ_DWORD(mmio, HOST_CAP);

	/* 
	 * For 614x, enable enhanced mode for PATA and interrupt. 
	 * For AHCI, enable AHCI.
	 */
	tmp = MV_REG_READ_DWORD(mmio, HOST_CTL);
	MV_REG_WRITE_DWORD(mmio, HOST_CTL, (MV_U32)(tmp | HOST_IRQ_EN | HOST_MVL_EN));
	tmp = MV_REG_READ_DWORD(mmio, HOST_CTL);

	/* Ports implemented: enable ports */
	pCore->Port_Map = MV_REG_READ_DWORD(mmio, HOST_PORTS_IMPL);
	tmp = MV_REG_READ_DWORD(mmio, HOST_CAP);
	//MV_DASSERT( pCore->Port_Num == ((tmp & 0x1f) + 1) );

	/* Initialize ports */
	for ( i = 0; i<pCore->Port_Num; i++) {
		pPort = &pCore->Ports[i];
		/* make sure port is not active */
		if ( pPort->Type==PORT_TYPE_PATA )
			PATA_ResetPort(pCore, i);
		else
			SATA_ResetPort(pCore, i);
	}


	/* Initialize port, set uncached memory pointer. */
	for ( i = 0; i<pCore->Port_Num; i++) {
		pPort = &pCore->Ports[i];

		/* Set the sata port register */
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_LST_ADDR_HI, pPort->Cmd_List_DMA.parts.high);
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_LST_ADDR, pPort->Cmd_List_DMA.parts.low);
		MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_LST_ADDR);

		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_FIS_ADDR_HI, pPort->RX_FIS_DMA.parts.high);
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_FIS_ADDR, pPort->RX_FIS_DMA.parts.low);
		MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_FIS_ADDR);

		/* AHCI is different with Thor */
		#ifdef AHCI
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_CMD, 
			PORT_CMD_ICC_ACTIVE | PORT_CMD_FIS_RX |	PORT_CMD_POWER_ON | PORT_CMD_SPIN_UP | PORT_CMD_START );
		#else
		if ( pPort->Type==PORT_TYPE_PATA )
		{	/* 12<<24: Bit 24-28: Indicates ATAPI command CDB length in bytes */
			MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_CMD, (12L<<24) | PORT_CMD_PATA_INTERRUPT | PORT_CMD_PATA_START );
		}
		else
		{
			/* 
			 * Workaround: Don't enable PORT_CMD_FIS_RX otherwise system will hang.
			 */
			MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_CMD, PORT_CMD_START );
		}
		#endif
	}

	//MV_DPRINT("HostCtrl=0x%x,HostIntStatus=0x%x\n",MV_REG_READ_DWORD(mmio, HOST_CTL),MV_REG_READ_DWORD(mmio, HOST_IRQ_STAT));

}

MV_BOOLEAN mvAdapterStateMachine(
	IN OUT MV_PVOID This,
	IN MV_PVOID temp
	)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;

	switch (pCore->Adapter_State)
	{
		case ADAPTER_INITIALIZING:
			MV_DPRINT(("start mvAdapterStateMachine.\n"));
			if(ResetController(pCore) == MV_FALSE) 
				return MV_FALSE;

			InitChip(pCore);
			pCore->Adapter_State = ADAPTER_READY;
#ifdef SUPPORT_TIMER
			Timer_AddRequest( pCore, 2, mvAdapterStateMachine, pCore, NULL );
#else
			HBA_RequestTimer(pCore, 2000, (MV_VOID(*)(MV_PVOID))mvAdapterStateMachine);
#endif

			break;

		case ADAPTER_READY:
			MV_DPRINT(("start mvChannelStateMachine in timer.\n"));
			mvChannelStateMachine(pCore, NULL);
			break;

		default:
			break;
	}
	return MV_TRUE;
}

void Device_ParseIdentifyData(
	IN PDomain_Device pDevice,
	IN PATA_Identify_Data pATAIdentify
	);

void Core_InternalReqCallback(
	 IN PCore_Driver_Extension pCore,
	 IN PMV_Request pReq
	 )
{
	PDomain_Port pPort; 
	PDomain_Device pDevice;
	PATA_Identify_Data pATAIdentify;
	MV_U8 portId, deviceId;
	MV_U32 tmp;

	portId = PATA_MapPortId(pReq->Device_Id);
	deviceId = PATA_MapDeviceId(pReq->Device_Id);

	pPort = &pCore->Ports[portId];
	pDevice = &pPort->Device[deviceId];

	//It's possible that CDB_CORE_READ_LOG_EXT returns error and come here
	//because we send CDB_CORE_READ_LOG_EXT no matter NCQ is running or not.
	//if ( pReq->Cdb[2]!=CDB_CORE_READ_LOG_EXT ) 
	if (( pReq->Cdb[2]!=CDB_CORE_READ_LOG_EXT ) && ( pReq->Cdb[2]!=CDB_CORE_ENABLE_WRITE_CACHE )&&( pReq->Cdb[2]!=CDB_CORE_ENABLE_READ_AHEAD ))
	{
		if( pReq->Scsi_Status != REQ_STATUS_SUCCESS )
		{
			/* request didn't finish correctly - we set device to existing
			   and finish state machine */
			pDevice->Status = DEVICE_STATUS_EXISTING;
			pDevice->State = DEVICE_STATE_INIT_DONE;
			mvDeviceStateMachine(pCore, pDevice);
			return;
		}
	}

	pATAIdentify = (PATA_Identify_Data)pPort->Device[deviceId].Scratch_Buffer;
	/* Handle internal request like identify */
	MV_DASSERT( pReq->Cdb[0]==SCSI_CMD_MARVELL_SPECIFIC );
	MV_DASSERT( pReq->Cdb[1]==CDB_CORE_MODULE );
	MV_ASSERT( portId < MAX_PORT_NUMBER );

	if ( pReq->Cdb[2]==CDB_CORE_IDENTIFY )
	{
#ifdef _OS_LINUX
		hba_swap_buf_le16((MV_PU16) pATAIdentify, 
				  sizeof(ATA_Identify_Data)/sizeof(MV_U16));
#endif /* _OS_LINUX  */
		Device_ParseIdentifyData(pDevice, pATAIdentify);
		if(pDevice->State != DEVICE_STATE_RESET_DONE){
			return;
			}
		//MV_ASSERT( pDevice->State == DEVICE_STATE_RESET_DONE );
		pDevice->State = DEVICE_STATE_IDENTIFY_DONE;

		/* Detect PATA Cable:
		 *** if there are master and slave, only detect after slave 
			 has finished identify 
		 *** if there is only master, detect after master has finished
			 identify 
		 */
		if (pPort->Type == PORT_TYPE_PATA)
		{
			if ((pDevice->Is_Slave) ||
				(!(pDevice->Is_Slave) && !(pPort->Device[1].Status & DEVICE_STATUS_FUNCTIONAL)))
			{
				tmp = MV_IO_READ_DWORD(pCore->Base_Address[4], 0);
				if( tmp & MV_BIT(8) )	{//40 pin cable
					pPort->PATA_cable_type = MV_40_PIN_CABLE;
				}else {//80 pin cable
					pPort->PATA_cable_type = MV_80_PIN_CABLE;
				}

				/* Cable is detected, now we start state machine for master,
				   if needed */
				if (pDevice->Is_Slave && (pPort->Device[0].Status & DEVICE_STATUS_FUNCTIONAL))
				{
					mvDeviceStateMachine(pCore, &pPort->Device[0]);
				}
			}
		}

		mvDeviceStateMachine(pCore, pDevice);
		return;
	}
	else if ( pReq->Cdb[2]==CDB_CORE_SET_UDMA_MODE )
	{
		pDevice->State = DEVICE_STATE_SET_UDMA_DONE;
		mvDeviceStateMachine(pCore, pDevice);
	}
	else if ( pReq->Cdb[2]==CDB_CORE_SET_PIO_MODE )
	{
		pDevice->State = DEVICE_STATE_SET_PIO_DONE;
		mvDeviceStateMachine(pCore, pDevice);
	}
	else if ( pReq->Cdb[2]==CDB_CORE_ENABLE_WRITE_CACHE )
	{
		pDevice->State = DEVICE_STATE_ENABLE_WRITE_CACHE_DONE;
		mvDeviceStateMachine(pCore, pDevice);
	}
	else if ( pReq->Cdb[2]==CDB_CORE_ENABLE_READ_AHEAD )
	{
		pDevice->State = DEVICE_STATE_ENABLE_READ_AHEAD_DONE;
		mvDeviceStateMachine(pCore, pDevice);
	}
	else if ( pReq->Cdb[2]==CDB_CORE_READ_LOG_EXT )
	{
		/* Do nothing. Just use this command to clear outstanding IO during error handling. */
		MV_PRINT("Read Log Ext is finished on device 0x%x.\n", pDevice->Id);
	}
}

static void Device_IssueIdentify(
	IN PDomain_Port pPort,
	IN PDomain_Device pDevice
	)
{
	PMV_Request pReq = pDevice->Internal_Req;
	PMV_SG_Table pSGTable = &pReq->SG_Table;

	MV_ZeroMvRequest(pReq);

	/* Prepare identify ATA task */
	pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	pReq->Cdb[1] = CDB_CORE_MODULE;
	pReq->Cdb[2] = CDB_CORE_IDENTIFY;
	pReq->Device_Id = pDevice->Id;

	//pReq->Req_Flag;
	pReq->Cmd_Initiator = pPort->Core_Extension;
	pReq->Data_Transfer_Length = sizeof(ATA_Identify_Data);
	pReq->Data_Buffer = pDevice->Scratch_Buffer;
	pReq->Completion = (void(*)(MV_PVOID,PMV_Request))Core_InternalReqCallback;
	MV_DASSERT( SATA_SCRATCH_BUFFER_SIZE>=sizeof(ATA_Identify_Data) );

	/* Make SG table */
	SGTable_Init(pSGTable, 0);
	SGTable_Append(pSGTable, 
				pDevice->Scratch_Buffer_DMA.parts.low, 
				pDevice->Scratch_Buffer_DMA.parts.high,
				pReq->Data_Transfer_Length
				); 
	MV_DASSERT( pReq->Data_Transfer_Length%2==0 );


	/* Send this internal request */
	Core_ModuleSendRequest(pPort->Core_Extension, pReq);
}

void Device_IssueReadLogExt(
	IN PDomain_Port pPort,
	IN PDomain_Device pDevice
	)
{
	PMV_Request pReq = pDevice->Internal_Req;
	PMV_SG_Table pSGTable = &pReq->SG_Table;

	MV_ZeroMvRequest(pReq);
	MV_PRINT("Device_IssueReadLogExt on device 0x%x.\n", pDevice->Id);

	//Disable NCQ after we found NCQ error.
	pDevice->Capacity &= ~(DEVICE_CAPACITY_NCQ_SUPPORTED);

	/* We support READ LOG EXT command with log page of 10h. */
	pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	pReq->Cdb[1] = CDB_CORE_MODULE;
	pReq->Cdb[2] = CDB_CORE_READ_LOG_EXT;
	pReq->Device_Id = pDevice->Id;

	//pReq->Req_Flag;
	pReq->Cmd_Initiator = pPort->Core_Extension;
	pReq->Data_Transfer_Length = SATA_SCRATCH_BUFFER_SIZE;
	pReq->Data_Buffer = pDevice->Scratch_Buffer;
	pReq->Completion = (void(*)(MV_PVOID,PMV_Request))Core_InternalReqCallback;
	MV_DASSERT( SATA_SCRATCH_BUFFER_SIZE>=sizeof(ATA_Identify_Data) );

	/* Make SG table */
	SGTable_Init(pSGTable, 0);
	SGTable_Append(pSGTable, 
				pDevice->Scratch_Buffer_DMA.parts.low, 
				pDevice->Scratch_Buffer_DMA.parts.high,
				pReq->Data_Transfer_Length
				); 
	MV_DASSERT( pReq->Data_Transfer_Length%2==0 );


	/* Send this internal request */
	Core_ModuleSendRequest(pPort->Core_Extension, pReq);
}

static MV_VOID mvAta2HostString(IN MV_U16 *source,
                                OUT MV_U16 *target,
                                IN MV_U32 wordsCount
                               )
{
    MV_U32 i;
    for (i=0 ; i < wordsCount; i++)
    {
        target[i] = (source[i] >> 8) | ((source[i] & 0xff) << 8);
        target[i] = MV_LE16_TO_CPU(target[i]);
    }
}

void Device_ParseIdentifyData(
	IN PDomain_Device pDevice,
	IN PATA_Identify_Data pATAIdentify
	)
{
	PDomain_Port pPort = pDevice->PPort;
	MV_U8 i;
	MV_U32 temp;

	/* Get serial number, firmware revision and model number. */
	
	MV_CopyMemory(pDevice->Serial_Number, pATAIdentify->Serial_Number, 20);
	MV_CopyMemory(pDevice->Firmware_Revision, pATAIdentify->Firmware_Revision, 8);

	MV_CopyMemory(pDevice->Model_Number, pATAIdentify->Model_Number, 40);
	mvAta2HostString((MV_U16 *)pDevice->Serial_Number, (MV_U16 *)pDevice->Serial_Number, 10);
	mvAta2HostString((MV_U16 *)pDevice->Firmware_Revision, (MV_U16 *)pDevice->Firmware_Revision, 4);

	mvAta2HostString((MV_U16 *)pDevice->Model_Number, (MV_U16 *)pDevice->Model_Number, 20);

	/* Capacity: 48 bit LBA, smart, write cache and NCQ */
	pDevice->Capacity = 0;
	pDevice->Setting = 0;
	if ( pATAIdentify->Command_Set_Supported[1] & MV_BIT(10) )
	{
		MV_DPRINT(("Device: %d 48 bit supported.\n", pDevice->Id));

		pDevice->Capacity |= DEVICE_CAPACITY_48BIT_SUPPORTED;
	}
	else
	{
		MV_DPRINT(("Device: %d 48 bit not supported.\n", pDevice->Id));
	}

	if ( pATAIdentify->Command_Set_Supported[0] & MV_BIT(0) ) 
	{
		pDevice->Capacity |= DEVICE_CAPACITY_SMART_SUPPORTED;
		if ( pATAIdentify->Command_Set_Enabled[0] & MV_BIT(0) )
		{
			pDevice->Setting |= DEVICE_SETTING_SMART_ENABLED;
		}
		if ( pATAIdentify->Command_Set_Supported_Extension & MV_BIT(0) )
		{
			pDevice->Capacity |= DEVICE_CAPACITY_SMART_SELF_TEST_SUPPORTED;
		}
	}	
	if ( pATAIdentify->Command_Set_Supported[0] & MV_BIT(5) ) 
	{
		pDevice->Capacity |= DEVICE_CAPACITY_WRITECACHE_SUPPORTED;
		if ( pATAIdentify->Command_Set_Enabled[0] & MV_BIT(5) )
		{
			pDevice->Setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
		}
	}
	if ( pATAIdentify->Command_Set_Supported[0] & MV_BIT(6) ) 
	{
		pDevice->Capacity |= DEVICE_CAPACITY_READ_LOOK_AHEAD_SUPPORTED;
		if ( pATAIdentify->Command_Set_Enabled[0] & MV_BIT(6) )
		{
			pDevice->Setting |= DEVICE_SETTING_READ_LOOK_AHEAD;
		}
	}	
#ifdef SUPPORT_ATA_SECURITY_CMD
	if ( pATAIdentify->Security_Status & MV_BIT(0) ) 
	{
		pDevice->Capacity |= DEVICE_CAPACITY_SECURITY_SUPPORTED;
		if (pATAIdentify->Security_Status & MV_BIT(2) )
		{
			MV_DPRINT(("securiy: locked\n"));
			pDevice->Setting |= DEVICE_SETTING_SECURITY_LOCKED;
		}
	}
#endif
	if ( pATAIdentify->SATA_Capabilities & MV_BIT(8) )
	{

		if (pDevice->Capacity & DEVICE_CAPACITY_48BIT_SUPPORTED)
			pDevice->Capacity |= DEVICE_CAPACITY_NCQ_SUPPORTED;

	}
	if ( pATAIdentify->Command_Set_Supported_Extension & MV_BIT(5) )
	{
		if ( pATAIdentify->Command_Set_Default & MV_BIT(5) )
			pDevice->Capacity |= DEVICE_CAPACITY_READLOGEXT_SUPPORTED;
	}

	temp = MV_REG_READ_DWORD( pPort->Mmio_Base, PORT_SCR_STAT );
	if ( ((temp >> 4) & 0xF) == 1 )
		pDevice->Capacity |= DEVICE_CAPACITY_RATE_1_5G;
	else if ( ((temp >> 4) & 0xF) == 2 )
		pDevice->Capacity |= DEVICE_CAPACITY_RATE_3G;

	/* Disk size */
	if ( pDevice->Capacity&DEVICE_CAPACITY_48BIT_SUPPORTED )
	{
		pDevice->Max_LBA.parts.low = *((MV_PU32)&pATAIdentify->Max_LBA[0]);
		pDevice->Max_LBA.parts.high = *((MV_PU32)&pATAIdentify->Max_LBA[2]);
	}else 
	{
		pDevice->Max_LBA.parts.low = *((MV_PU32)&pATAIdentify->User_Addressable_Sectors[0]);
		pDevice->Max_LBA.parts.high = 0;
	}
	/* For SATA drive, MAX LBA in identify data is actually disk size */
	pDevice->Max_LBA = U64_SUBTRACT_U32(pDevice->Max_LBA, 1);

	/* PIO, MDMA and UDMA mode */	
   	if ( ( pATAIdentify->Fields_Valid&MV_BIT(1) )
		&& ( pATAIdentify->PIO_Modes&0x0F ) )	
	{
       	if ( (MV_U8)pATAIdentify->PIO_Modes>=0x2 )
		  	pDevice->PIO_Mode = 0x04; 
		else
	  		pDevice->PIO_Mode = 0x03; 
	}
    else
	{
       	pDevice->PIO_Mode = 0x02;
	}

	pDevice->MDMA_Mode = 0xFF;
	if ( pATAIdentify->Multiword_DMA_Modes & MV_BIT(2) )
		pDevice->MDMA_Mode = 2;
	else if ( pATAIdentify->Multiword_DMA_Modes & MV_BIT(1) )
		pDevice->MDMA_Mode = 1;
	else if ( pATAIdentify->Multiword_DMA_Modes & MV_BIT(0) )
		pDevice->MDMA_Mode = 0;

	pDevice->UDMA_Mode = 0xFF;
    if ( pATAIdentify->Fields_Valid&MV_BIT(2) )
	{
		for ( i=0; i<7; i++ )
		{
			if ( pATAIdentify->UDMA_Modes & MV_BIT(i) )
				pDevice->UDMA_Mode = i;	
		}
	}	

	/* CRC identify buffer to get the U32 GUID. */
	pDevice->WWN = MV_CRC((MV_PU8)pATAIdentify, sizeof(ATA_Identify_Data));

}

static void Device_IssueSetMDMAMode(
	IN PDomain_Port pPort,
	IN PDomain_Device pDevice
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PMV_Request pReq = pDevice->Internal_Req;
    MV_U32 temp;
	MV_U32 offset;
	MV_LPVOID base;
	MV_BOOLEAN memoryIO=MV_FALSE;
	MV_U8 mode = pDevice->MDMA_Mode;

	/* Only if the Device doesn't support UDMA, we'll use MDMA mode. */
	MV_DASSERT( pDevice->UDMA_Mode==0xFF );
	/* Is that possible that one device doesn't support either UDMA and MDMA? */
	MV_DASSERT( (pDevice->MDMA_Mode<=2) );
	MV_ZeroMvRequest(pReq);

	/* Set controller timing register for PATA port before set the device MDMA mode. */
	if ( pPort->Type==PORT_TYPE_PATA )
	{
		if ( pCore->Device_Id==DEVICE_ID_THORLITE_2S1P 
			|| pCore->Device_Id==DEVICE_ID_THORLITE_2S1P_WITH_FLASH
			|| pCore->Device_Id==DEVICE_ID_THORLITE_0S1P )
		{
			if ( pCore->Revision_Id==0xA0 )
			{
				/* Thorlite A0 */
				temp = MV_IO_READ_DWORD(pCore->Base_Address[4], 0);
				temp &= 0xFFFF00FF;
				temp |= 0x0000A800;
				MV_IO_WRITE_DWORD(pCore->Base_Address[4], 0, temp);

				if ( !pDevice->Is_Slave ) 
					offset = 0x10;
				else
					offset = 0x14;
				base = pCore->Base_Address[4];
				memoryIO = MV_FALSE;
			}
			else
			{
				/* Thorlite B0 */
				MV_DASSERT( (pCore->Revision_Id==0xB0)||(pCore->Revision_Id==0xB1)||(pCore->Revision_Id==0xB2) );
				if ( !pDevice->Is_Slave )
					offset = 0xA0;
				else
					offset = 0xA4;
				base = pCore->Base_Address[5];
				memoryIO = MV_TRUE;
			}
		} 
		else
		{
			MV_DASSERT( (pCore->Device_Id==DEVICE_ID_THOR_4S1P)||(pCore->Device_Id==DEVICE_ID_THOR_4S1P_NEW) );
			if ( pCore->Revision_Id==0x00	/* A0 */
				|| pCore->Revision_Id==0x01	/* A1 */
				|| pCore->Revision_Id==0x10	/* B0 and C0 */	)
			{
				/* Thor A0-C0 */
				if ( !pDevice->Is_Slave )
					offset = 0x08;
				else
					offset = 0x0c;
				base = pCore->Base_Address[4];
				memoryIO = MV_FALSE;
			}
			else
			{
				/* Thor D0 = Thor New A0 */
				MV_DASSERT( (pCore->Revision_Id==0xA0) || (pCore->Revision_Id==0xA1) ||
							(pCore->Revision_Id==0xA2) );
				if ( !pDevice->Is_Slave )
					offset = 0xA0;
				else
					offset = 0xA4;
				base = pCore->Base_Address[5];
				memoryIO = MV_TRUE;
			}
		}

		if ( !memoryIO )
		{
			temp = MV_IO_READ_DWORD(base, offset);
			temp &= 0xFFFFFF3F;
			temp |= ((MV_U32)mode)<<6;
			temp |= 0x100;		/* Enable MDAM */
			MV_IO_WRITE_DWORD(base, offset, temp);
		}
		else
		{
			temp = MV_REG_READ_DWORD(base, offset);
			temp &= 0xFFFFFF3F;
			temp |= ((MV_U32)mode)<<6;
			temp |= 0x100;		/* Enable MDAM */
			MV_REG_WRITE_DWORD(base, offset, temp);
		}
	}

	//pDevice->MDMA_Mode = mode;
	pDevice->Current_MDMA = mode;
	
	/* Prepare set UDMA mode task */
	pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	pReq->Cdb[1] = CDB_CORE_MODULE;
	pReq->Cdb[2] = CDB_CORE_SET_UDMA_MODE;
	pReq->Cdb[3] = mode;
	/* Means we are setting MDMA mode. I still use CDB_CORE_SET_UDMA_MODE because I don't want to change the state machine. */
	pReq->Cdb[4] = MV_TRUE;
	pReq->Device_Id = pDevice->Id;
	//pReq->Req_Flag;
	pReq->Cmd_Initiator = pPort->Core_Extension;
	pReq->Data_Transfer_Length = 0;
	pReq->Data_Buffer = NULL;
	pReq->Completion = (void(*)(MV_PVOID,PMV_Request))Core_InternalReqCallback;


	/* Send this internal request */
	Core_ModuleSendRequest(pPort->Core_Extension, pReq);
}

static void Device_IssueSetUDMAMode(
	IN PDomain_Port pPort,
	IN PDomain_Device pDevice
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PMV_Request pReq = pDevice->Internal_Req;
    MV_U32 temp;
	MV_U32 offset;
	MV_LPVOID base;
	MV_BOOLEAN memoryIO=MV_FALSE;
	MV_U8 mode = pDevice->UDMA_Mode;
	
	if ( pDevice->UDMA_Mode==0xFF )
	{
		Device_IssueSetMDMAMode(pPort, pDevice);
		return;
	}

	//GT 10/19/2006 11:07AM
	//Check PATA port cable if 40_pin or 80_pin
	if ( pPort->Type==PORT_TYPE_PATA )
	{
#ifdef PATA_CABLE_DETECTION
		if (pPort->PATA_cable_type==MV_40_PIN_CABLE){
			if ( mode>2 ) 
				mode = 2;
			pDevice->UDMA_Mode = mode;
		}
#else
		temp = MV_IO_READ_DWORD(pCore->Base_Address[4], 0);
		if( temp & MV_BIT(8) )	//40_pin cable
		{
			if ( mode>2 ) 
				mode = 2;
			pDevice->UDMA_Mode = mode;
		}
#endif
	}	
	
	/* Hardware team required us to downgrade UDMA mode to zero. */
	//if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
	//	mode = 0;
	//if ( mode>=5 ) mode = 4; //???

	MV_ZeroMvRequest(pReq);

	//if ( (pCore->Device_Id!=DEVICE_ID_THOR_4S1P_NEW) && (pCore->Revision_Id!=0xB0) && (pCore->Revision_Id!=0xB1) && (pCore->Revision_Id!=0xB2) )
	if ( (pCore->Device_Id!=DEVICE_ID_THOR_4S1P_NEW) && (pCore->Revision_Id<0xB0) )
	{
		/* Degrade ATAPI device UDMA mode always to 2. */
		if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
		{
			if ( mode>2 ) mode = 2;
		}
		else
		{
			/*
			* Workaround:
			* Thor lite A0 has problem with Hitesh(IBM) HDD under UDMA 5
			* And it has problem with any HDD under UDMA 6
			* So we degrade the HDD mode to 5 and ignore Hitesh HDD for now.
			*/
			if ( mode>5 ) mode = 5;
		}
	}

	/* 
	 * Set controller timing register for PATA port before set the device UDMA mode.
	 * Thorlite A0:	To enable timing programming, BAR 4 offset x0, write a800
	 *				To set values, BAR 4 offset x10, x14
	 * Thorlite B0:	BAR 5 offset xa0, xa4
	 * Thor A0~C0:	BAR 4 offset x8, xc
	 * Thor D0(=Thor New A0):BAR 5 offset xa0, xa4 ( Same as Thorlite B0 )
	 */
	if ( pPort->Type==PORT_TYPE_PATA )
	{
		if ( pCore->Device_Id==DEVICE_ID_THORLITE_2S1P 
			|| pCore->Device_Id==DEVICE_ID_THORLITE_2S1P_WITH_FLASH
			|| pCore->Device_Id==DEVICE_ID_THORLITE_0S1P )
		{
			if ( pCore->Revision_Id==0xA0 )
			{
				/* Thorlite A0 */
				temp = MV_IO_READ_DWORD(pCore->Base_Address[4], 0);
				temp &= 0xFFFF00FF;
				temp |= 0x0000A800;
				MV_IO_WRITE_DWORD(pCore->Base_Address[4], 0, temp);

				if ( !pDevice->Is_Slave ) 
					offset = 0x10;
				else
					offset = 0x14;
				base = pCore->Base_Address[4];
				memoryIO = MV_FALSE;
			}
			else
			{
				/* Thorlite B0 */
				MV_DASSERT( (pCore->Revision_Id==0xB0)||(pCore->Revision_Id==0xB1)||(pCore->Revision_Id==0xB2) );
				if ( !pDevice->Is_Slave )
					offset = 0xA0;
				else
					offset = 0xA4;
				base = pCore->Base_Address[5];
				memoryIO = MV_TRUE;
			}
		} 
		else
		{
			MV_DASSERT( (pCore->Device_Id==DEVICE_ID_THOR_4S1P)||(pCore->Device_Id==DEVICE_ID_THOR_4S1P_NEW) );
			if ( pCore->Revision_Id==0x00	/* A0 */
				|| pCore->Revision_Id==0x01	/* A1 */
				|| pCore->Revision_Id==0x10	/* B0 and C0 */	)
			{
				/* Thor A0-C0 */
				if ( !pDevice->Is_Slave )
					offset = 0x08;
				else
					offset = 0x0c;
				base = pCore->Base_Address[4];
				memoryIO = MV_FALSE;
			}
			else
			{
				/* Thor D0 = Thor New A0 */
				MV_DASSERT( (pCore->Revision_Id==0xA0) || (pCore->Revision_Id==0xA1) ||
							(pCore->Revision_Id==0xA2) );
				if ( !pDevice->Is_Slave )
					offset = 0xA0;
				else
					offset = 0xA4;
				base = pCore->Base_Address[5];
				memoryIO = MV_TRUE;
			}
		}

		if ( !memoryIO )
		{
			temp = MV_IO_READ_DWORD(base, offset);
			temp &= 0xFFFFFFF8;
			temp |= (MV_U32)mode;
			MV_IO_WRITE_DWORD(base, offset, temp);
		}
		else
		{
			temp = MV_REG_READ_DWORD(base, offset);
			temp &= 0xFFFFFFF8;
			temp |= (MV_U32)mode;
			MV_REG_WRITE_DWORD(base, offset, temp);
		}
	}

	//pDevice->UDMA_Mode = mode;
	pDevice->Current_UDMA = mode;

	/* Prepare set UDMA mode task */
	pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	pReq->Cdb[1] = CDB_CORE_MODULE;
	pReq->Cdb[2] = CDB_CORE_SET_UDMA_MODE;
	pReq->Cdb[3] = mode;
	/* Not setting MDMA but UDMA mode. */
	pReq->Cdb[4] = MV_FALSE;
	pReq->Device_Id = pDevice->Id;
	//pReq->Req_Flag;
	pReq->Cmd_Initiator = pPort->Core_Extension;
	pReq->Data_Transfer_Length = 0;
	pReq->Data_Buffer = NULL;
	pReq->Completion = (void(*)(MV_PVOID,PMV_Request))Core_InternalReqCallback;


	/* Send this internal request */
	Core_ModuleSendRequest(pPort->Core_Extension, pReq);
}

static void Device_IssueSetPIOMode(
	IN PDomain_Port pPort,
	IN PDomain_Device pDevice
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PMV_Request pReq = pDevice->Internal_Req;
    MV_U32 temp;
	MV_U32 offset;
	MV_LPVOID base;
	MV_BOOLEAN memoryIO=MV_FALSE;
	MV_U8 mode = pDevice->PIO_Mode;
	
	MV_ZeroMvRequest(pReq);

	/* Hardware team required us to downgrade PIO mode to zero. */
	if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
		mode = 0;


	/* 
	 * Set controller timing register for PATA port before set the device UDMA mode.
	 * Thorlite A0:	To enable timing programming, BAR 4 offset x0, write a800
	 *				To set values, BAR 4 offset x10, x14
	 * Thorlite B0:	BAR 5 offset xa0, xa4
	 * Thor A0~C0:	BAR 4 offset x8, xc
	 * Thor D0:		BAR 5 offset xa0, xa4 ( Same as Thorlite B0 )
	 */
	if ( pPort->Type==PORT_TYPE_PATA )
	{

		if ( pCore->Device_Id==DEVICE_ID_THORLITE_2S1P 
			|| pCore->Device_Id==DEVICE_ID_THORLITE_2S1P_WITH_FLASH
			|| pCore->Device_Id==DEVICE_ID_THORLITE_0S1P )
		{
			if ( pCore->Revision_Id==0xA0 )
			{
				/* Thorlite A0 */
				temp = MV_IO_READ_DWORD(pCore->Base_Address[4], 0);
				temp &= 0xFFFF00FF;
				temp |= 0x0000A800;
				MV_IO_WRITE_DWORD(pCore->Base_Address[4], 0, temp);

				if ( !pDevice->Is_Slave ) 
					offset = 0x10;
				else
					offset = 0x14;
				base = pCore->Base_Address[4];
				memoryIO = MV_FALSE;
			}
			else
			{
				/* Thorlite B0 */
				MV_DASSERT( (pCore->Revision_Id==0xB0)||(pCore->Revision_Id==0xB1)||(pCore->Revision_Id==0xB2) );
				if ( !pDevice->Is_Slave )
					offset = 0xA0;
				else
					offset = 0xA4;
				base = pCore->Base_Address[5];
				memoryIO = MV_TRUE;
			}
		} 
		else
		{

			MV_DASSERT( (pCore->Device_Id==DEVICE_ID_THOR_4S1P)||(pCore->Device_Id==DEVICE_ID_THOR_4S1P_NEW) );
			if ( pCore->Revision_Id==0x00	/* A0 */
				|| pCore->Revision_Id==0x01	/* A1 */
				|| pCore->Revision_Id==0x10	/* B0 and C0 */	)
			{
				/* Thor A0-C0 */
				if ( !pDevice->Is_Slave )
					offset = 0x08;
				else
					offset = 0x0c;
				base = pCore->Base_Address[4];
				memoryIO = MV_FALSE;
			}
			else
			{
				/* Thor D0 = Thor New A0 */
				MV_DASSERT( (pCore->Revision_Id==0xA0) || (pCore->Revision_Id==0xA1) ||
							(pCore->Revision_Id==0xA2) );
				if ( !pDevice->Is_Slave )
					offset = 0xA0;
				else
					offset = 0xA4;
				base = pCore->Base_Address[5];
				memoryIO = MV_TRUE;
			}
		}

		if ( !memoryIO )
		{
			temp = MV_IO_READ_DWORD(base, offset);
			temp &= 0xFFFFFFC7;
			temp |= ((MV_U32)mode<<3);
			MV_IO_WRITE_DWORD(base, offset, temp);
		}
		else
		{
			temp = MV_REG_READ_DWORD(base, offset);
			temp &= 0xFFFFFFC7;
			temp |= ((MV_U32)mode<<3);
			MV_REG_WRITE_DWORD(base, offset, temp);
		}
	}

	//pDevice->PIO_Mode = mode;
	pDevice->Current_PIO = mode;
	
	/* Prepare set PIO mode task */
	pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	pReq->Cdb[1] = CDB_CORE_MODULE;
	pReq->Cdb[2] = CDB_CORE_SET_PIO_MODE;
	pReq->Cdb[3] = mode;
	pReq->Device_Id = pDevice->Id;
	//pReq->Req_Flag;
	pReq->Cmd_Initiator = pPort->Core_Extension;
	pReq->Data_Transfer_Length = 0;
	pReq->Data_Buffer = NULL;
	pReq->Completion = (void(*)(MV_PVOID,PMV_Request))Core_InternalReqCallback;

	/* Send this internal request */
	Core_ModuleSendRequest(pPort->Core_Extension, pReq);

}

static void Device_EnableWriteCache(
	IN PDomain_Port pPort,
	IN PDomain_Device pDevice
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PMV_Request pReq = pDevice->Internal_Req;

	MV_ZeroMvRequest(pReq);

	/* Prepare enable write cache command */
	pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	pReq->Cdb[1] = CDB_CORE_MODULE;
	pReq->Cdb[2] = CDB_CORE_ENABLE_WRITE_CACHE;
	pReq->Device_Id = pDevice->Id;
	//pReq->Req_Flag;
	pReq->Cmd_Initiator = pPort->Core_Extension;
	pReq->Data_Transfer_Length = 0;
	pReq->Data_Buffer = NULL;
	pReq->Completion = (void(*)(MV_PVOID,PMV_Request))Core_InternalReqCallback;

	/* skip if this is ATAPI device */
	if( pDevice->Device_Type & DEVICE_TYPE_ATAPI )
	{
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		pReq->Completion(pCore, pReq);
	}
	else
	{

		/* Send this internal request */
		Core_ModuleSendRequest(pPort->Core_Extension, pReq);
	}
}

static void Device_EnableReadAhead(
	IN PDomain_Port pPort,
	IN PDomain_Device pDevice
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PMV_Request pReq = pDevice->Internal_Req;

	MV_ZeroMvRequest(pReq);

	/* Prepare enable read ahead command */
	pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	pReq->Cdb[1] = CDB_CORE_MODULE;
	pReq->Cdb[2] = CDB_CORE_ENABLE_READ_AHEAD;
	pReq->Device_Id = pDevice->Id;
	//pReq->Req_Flag;
	pReq->Cmd_Initiator = pPort->Core_Extension;
	pReq->Data_Transfer_Length = 0;
	pReq->Data_Buffer = NULL;
	pReq->Completion = (void(*)(MV_PVOID,PMV_Request))Core_InternalReqCallback;

	/* skip if this is ATAPI device */
	if( pDevice->Device_Type & DEVICE_TYPE_ATAPI )
	{
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		pReq->Completion(pCore, pReq);
	}
	else
	{
		/* Send this internal request */
		Core_ModuleSendRequest(pPort->Core_Extension, pReq);
	}
}

