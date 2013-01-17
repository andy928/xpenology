#include "mv_include.h"

#include "core_exp.h"
#include "core_inter.h"
#include "com_tag.h"

#include "core_sata.h"
#include "core_ata.h"

#include "core_init.h"

#if defined(_OS_LINUX)
#include "hba_timer.h"
#endif /* _OS_LINUX */

#ifdef CORE_SUPPORT_API
#include "core_api.h"
#endif

#ifdef SOFTWARE_XOR
#include "core_xor.h"
#endif
#include "core_sat.h"

#define FIS_REG_H2D_SIZE_IN_DWORD	5

void sendDummyFIS( PDomain_Port pPort );
void mv_core_put_back_request(PDomain_Port pPort);
MV_U8 mv_core_reset_port(PDomain_Port pPort);
#ifdef COMMAND_ISSUE_WORKROUND
void mv_core_dump_reg(PDomain_Port pPort);
void  mv_core_reset_command(PDomain_Port pPort);
void mv_core_init_reset_para(PDomain_Port pPort);
#endif
extern MV_VOID SCSI_To_FIS(MV_PVOID pCore, PMV_Request pReq, MV_U8 tag, PATA_TaskFile pTaskFile);

extern MV_BOOLEAN Category_CDB_Type(
	IN PDomain_Device pDevice,
	IN PMV_Request pReq
	);

extern MV_BOOLEAN ATAPI_CDB2TaskFile(
	IN PDomain_Device pDevice,
	IN PMV_Request pReq, 
	OUT PATA_TaskFile pTaskFile
	);

extern MV_BOOLEAN ATA_CDB2TaskFile(
	IN PDomain_Device pDevice,
	IN PMV_Request pReq, 
	IN MV_U8 tag,	
	OUT PATA_TaskFile pTaskFile
	);

extern void Device_IssueReadLogExt(
	IN PDomain_Port pPort,
	IN PDomain_Device pDevice
	);

extern MV_BOOLEAN mvDeviceStateMachine(
	PCore_Driver_Extension pCore,
	PDomain_Device pDevice
	);

void CompleteRequest(
	IN PCore_Driver_Extension pCore,
	IN PMV_Request pReq,
	IN PATA_TaskFile taskFiles
	);

void CompleteRequestAndSlot(
	IN PCore_Driver_Extension pCore,
	IN PDomain_Port pPort,
	IN PMV_Request pReq,
	IN PATA_TaskFile taskFiles,
	IN MV_U8 slotId
	);

#if defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX)
void Core_ResetChannel(MV_PVOID Device, MV_PVOID temp);

static MV_VOID __core_req_timeout_handler(MV_PVOID data)
{
	PMV_Request req = (PMV_Request) data;
	PCore_Driver_Extension pcore;
	PDomain_Device dev;
	PHBA_Extension phba;

	if ( NULL == req )
		return;
#ifdef SUPPORT_ATA_SMART
	if(req->Cdb[0]==SCSI_CMD_SND_DIAG) {
	MV_DPRINT(("Command :'Execute SMART self-test routine' fail.\n"));
	hba_remove_timer(req);
	return;
	}
#endif
#ifdef SUPPORT_ATA_SECURITY_CMD
	if(req->Cdb[0]==ATA_16 && req->Cdb[14]==0xf4) {
		MV_PRINT("The command of Security Erase is running\n");
		hba_remove_timer(req);
		hba_add_timer(req,60, __core_req_timeout_handler);
		return;
	}
#endif
	pcore = HBA_GetModuleExtension(req->Cmd_Initiator, MODULE_CORE);
	dev   = &pcore->Ports[PATA_MapPortId(req->Device_Id)].Device[PATA_MapDeviceId(req->Device_Id)];
	phba = HBA_GetModuleExtension(req->Cmd_Initiator, MODULE_HBA);
#ifdef MV_DEBUG
	MV_DPRINT(("Request time out. Resetting channel %d. \n", PATA_MapPortId(req->Device_Id)));	
	MV_DumpRequest(req, 0);
#endif
	hba_spin_lock_irq(&phba->desc->hba_desc->global_lock);
	Core_ResetChannel((MV_PVOID) dev, NULL);
	hba_spin_unlock_irq(&phba->desc->hba_desc->global_lock);
}
#endif /* SUPPORT_ERROR_HANDLING && _OS_LINUX */

#ifdef SUPPORT_SCSI_PASSTHROUGH
// Read TaskFile
void readTaskFiles(IN PDomain_Port pPort, PDomain_Device pDevice, PATA_TaskFile pTaskFiles)
{
	MV_U32 taskFile[3];

	if (pPort->Type==PORT_TYPE_PATA)
	{
		if ( pDevice->Is_Slave )
		{
			taskFile[1] = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_SLAVE_TF1);
			taskFile[2] = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_SLAVE_TF2);
		}
		else
		{
			taskFile[1] = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_MASTER_TF1);
			taskFile[2] = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_MASTER_TF2);
		}

		pTaskFiles->Sector_Count = (MV_U8)((taskFile[1] >> 24) & 0xFF);
		pTaskFiles->Sector_Count_Exp = (MV_U8)((taskFile[1] >> 16) & 0xFF);
		pTaskFiles->LBA_Low = (MV_U8)((taskFile[1] >> 8) & 0xFF);
		pTaskFiles->LBA_Low_Exp = (MV_U8)(taskFile[1] & 0xFF);

		pTaskFiles->LBA_Mid = (MV_U8)((taskFile[2] >> 24) & 0xFF);
		pTaskFiles->LBA_Mid_Exp = (MV_U8)((taskFile[2] >> 16) & 0xFF);
		pTaskFiles->LBA_High = (MV_U8)((taskFile[2] >> 8) & 0xFF);
		pTaskFiles->LBA_High_Exp = (MV_U8)(taskFile[2] & 0xFF);
	}
	else
	{
//		taskFile[0] = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_TFDATA);
		taskFile[1] = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_SIG);
//		taskFile[2] = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_SCR);

		pTaskFiles->Sector_Count = (MV_U8)((taskFile[1]) & 0xFF);
		pTaskFiles->LBA_Low = (MV_U8)((taskFile[1] >> 8) & 0xFF);
		pTaskFiles->LBA_Mid = (MV_U8)((taskFile[1] >> 16) & 0xFF);
		pTaskFiles->LBA_High = (MV_U8)((taskFile[1] >> 24) & 0xFF);

	}
}
#endif


void mv_core_set_running_slot(PDomain_Port pPort, MV_U32 slot_num, PMV_Request pReq)
{

#ifdef MV_DEBUG
	if ( pPort->Running_Slot >> slot_num & MV_BIT(0)  )
	{
		MV_DPRINT(("Why the slot[%d] is used.\n",slot_num));
	}

	if(pPort->Running_Req[slot_num]){
		MV_DPRINT(("Why the running req[%d] is used.\n",slot_num));
		MV_DumpRequest(pPort->Running_Req[slot_num], MV_FALSE);
	}
#endif
	pPort->Running_Slot |= 1<<slot_num;
	pPort->Running_Req[slot_num] = pReq;
}

void mv_core_reset_running_slot(PDomain_Port pPort, MV_U32 slot_num)
{

#ifdef MV_DEBUG
	if ( !(pPort->Running_Slot >> slot_num & MV_BIT(0)) )
	{
		MV_DPRINT(("Why the slot[%d] is not used.\n",slot_num));
	}

	if(!pPort->Running_Req[slot_num]){
		MV_DPRINT(("Why the running req[%d] is not used.\n",slot_num));
		MV_DumpRequest(pPort->Running_Req[slot_num], MV_FALSE);
	}
#endif

	pPort->Running_Req[slot_num] = NULL;
	pPort->Running_Slot &= ~(1L<<slot_num);
	Tag_ReleaseOne(&pPort->Tag_Pool, slot_num);
}



MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo)
{
	MV_U32 size = 0;
	MV_U8 sgEntryCount, tagCount;
	
	/* Extension quota */	
	if ( type==RESOURCE_CACHED_MEMORY )		
	{
		size = ROUNDING(sizeof(Core_Driver_Extension), 8);
	#ifdef SUPPORT_CONSOLIDATE
		if ( maxIo>1 )
		{
			size += ROUNDING(sizeof(Consolidate_Extension), 8);
			size += ROUNDING(sizeof(Consolidate_Device), 8)*MAX_DEVICE_NUMBER;
		}
	#endif

		/* resource for SG Entry and Tag Pool */
		if (maxIo==1) {
			sgEntryCount = MAX_SG_ENTRY_REDUCED;
			tagCount = 1;
		}
		else {
			sgEntryCount = MAX_SG_ENTRY;
			tagCount = MAX_SLOT_NUMBER - 1;		/* leave the last slot for PM hot-plug */
		}

#ifdef _OS_LINUX
#ifdef USE_NEW_SGVP
		sgEntryCount *= 2;
#endif
#endif /* _OS_LINUX */

		size += sizeof(MV_SG_Entry) * sgEntryCount * INTERNAL_REQ_COUNT;
		size += sizeof(MV_Request) * INTERNAL_REQ_COUNT;

		/* tag pool */
		size += ROUNDING(sizeof(MV_U16) * tagCount * MAX_PORT_NUMBER, 8); 

#ifdef SUPPORT_CONSOLIDATE
		/* resource for Consolidate_Extension->Requests[] SG Entry */	
		if ( maxIo>1 )
			size += sizeof(MV_SG_Entry) * sgEntryCount * CONS_MAX_INTERNAL_REQUEST_COUNT;
#endif
		return size;
	}
	
	/* Uncached memory quota */
	if ( type==RESOURCE_UNCACHED_MEMORY )
	{
		/* 
		 * SATA port alignment quota:
		 * Command list and received FIS is 64 byte aligned.
		 * Command table is 128 byte aligned.
		 * Data buffer is 8 byte aligned.
		 * This is different with AHCI.
		 */
		/* 
		 * PATA port alignment quota: Same with SATA.
		 * The only difference is that PATA doesn't have the FIS.
		 */
		MV_DPRINT(("Command List Size = 0x%x.\n", (MV_U32)SATA_CMD_LIST_SIZE));
		MV_DPRINT(("Received FIS Size = 0x%x.\n", (MV_U32)SATA_RX_FIS_SIZE));
		MV_DPRINT(("Command Table Size = 0x%x.\n", (MV_U32)SATA_CMD_TABLE_SIZE));
		MV_ASSERT(SATA_CMD_LIST_SIZE==ROUNDING(SATA_CMD_LIST_SIZE, 64));
		MV_ASSERT(SATA_RX_FIS_SIZE==ROUNDING(SATA_RX_FIS_SIZE, 64));
//		MV_ASSERT(SATA_CMD_TABLE_SIZE==ROUNDING(SATA_CMD_TABLE_SIZE, 128));
		MV_ASSERT(SATA_SCRATCH_BUFFER_SIZE==ROUNDING(SATA_SCRATCH_BUFFER_SIZE, 8));
		if ( maxIo>1 )
		{
			size = 64 + SATA_CMD_LIST_SIZE*MAX_PORT_NUMBER;								/* Command List*/
			size += 64 + SATA_RX_FIS_SIZE*MAX_SATA_PORT_NUMBER;							/* Received FIS */
			size += 128 + SATA_CMD_TABLE_SIZE*MAX_SLOT_NUMBER*MAX_PORT_NUMBER;			/* Command Table */
			size += 8 + SATA_SCRATCH_BUFFER_SIZE*MAX_DEVICE_NUMBER;						/* Buffer for initialization like identify */
		}
		else
		{
		#ifndef HIBERNATION_ROUNTINE
			size = 64 + SATA_CMD_LIST_SIZE*MAX_PORT_NUMBER;
			size += 64 + SATA_RX_FIS_SIZE*MAX_SATA_PORT_NUMBER;
			size += 128 + SATA_CMD_TABLE_SIZE*MAX_PORT_NUMBER;
			size += 8 + SATA_SCRATCH_BUFFER_SIZE*MAX_DEVICE_NUMBER;
		#else
			size = 64 + SATA_CMD_LIST_SIZE;			/* Command List*/
			size += 64 + SATA_RX_FIS_SIZE;			/* Received FIS */
			size += 128 + SATA_CMD_TABLE_SIZE; 		/* Command Table */	
			size += 8 + SATA_SCRATCH_BUFFER_SIZE;	/* Buffer for initialization like identify */
		#endif
		}

		return size;
	}

	return 0;
}

void Core_ModuleInitialize(MV_PVOID ModulePointer, MV_U32 extensionSize, MV_U16 maxIo)
{
#if defined(NEW_LINUX_DRIVER) 
	struct mv_mod_desc *mod_desc = (struct mv_mod_desc *)ModulePointer;
	MV_PVOID		This = (MV_PVOID)mod_desc->extension;
	PCore_Driver_Extension pCore    = (PCore_Driver_Extension) mod_desc->extension;
	MV_U32 size=0;
#else
	MV_PVOID		This=ModulePointer;
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)ModulePointer;
	Controller_Infor controller;
#endif	/* NEW_LINUX_DRIVER*/


	PMV_Request pReq;
	Assigned_Uncached_Memory dmaResource;
	PDomain_Port port;
	MV_PVOID memVir;
	MV_PHYSICAL_ADDR memDMA;
	MV_PTR_INTEGER temp, tmpSG;
	MV_U32 offset, internalReqSize, tagSize;
	MV_U8 i,j, flagSaved, sgEntryCount, tagCount;
	MV_U32 vsr_c[MAX_SATA_PORT_NUMBER];
	MV_U8 vsrSkipPATAPort = 0;
#ifndef _OS_LINUX
	MV_PVOID pTopLayer = HBA_GetModuleExtension(pCore, MODULE_HBA);
#endif	
	flagSaved=pCore->VS_Reg_Saved;

	if(flagSaved==VS_REG_SIG)
	{
		for ( j=0; j<MAX_SATA_PORT_NUMBER; j++ )
		{
			port = &pCore->Ports[j];
			vsr_c[j]=port->VS_RegC;
		}
		/* Save the PATA Port detection skip flag */
		vsrSkipPATAPort = pCore->Flag_Fastboot_Skip & FLAG_SKIP_PATA_PORT;
	}

	/* 
	 * Zero core driver extension. After that, I'll ignore many variables initialization. 
	 */
	MV_ZeroMemory(This, extensionSize);

	if(flagSaved==VS_REG_SIG)
	{
		pCore->VS_Reg_Saved=flagSaved;

		for ( j=0; j<MAX_SATA_PORT_NUMBER; j++ )
		{
			port = &pCore->Ports[j];
			port->VS_RegC=vsr_c[j];
		}
		/* Restore the PATA Port detection skip flag */
		/* Only this flag should survive the S3 */
		/* The others should be kept as default (0) */
		pCore->Flag_Fastboot_Skip = vsrSkipPATAPort;
	}

	pCore->State = CORE_STATE_IDLE;
	/* Set up controller information */
#if defined(__MM_SE__) 
	/* Set up controller information */
	pCore->desc        = mod_desc;
	pCore->Vendor_Id   = mod_desc->hba_desc->vendor;
	pCore->Device_Id   = mod_desc->hba_desc->device;
	pCore->Revision_Id = mod_desc->hba_desc->Revision_Id;	
	MV_DPRINT(("pCore->Device_Id = 0x%x.\n",pCore->Device_Id));
	/*
	pCore->Sub_System_Id =mod_desc->hba_desc->Sub_System_Id;
	pCore->Sub_Vendor_Id =mod_desc->hba_desc->Sub_Vendor_Id;
 	*/
	for ( i=0; i<MAX_BASE_ADDRESS; i++ )
	{
		pCore->Base_Address[i] = mod_desc->hba_desc->Base_Address[i];
		MV_DPRINT(("pCore->Base_Address[%d]=%p.\n", i,pCore->Base_Address[i]));
	}
	pCore->Mmio_Base = mod_desc->hba_desc->Base_Address[MV_PCI_BAR];
 #else
	/* Set up controller information */
	HBA_GetControllerInfor(pCore, &controller);
	pCore->Vendor_Id = controller.Vendor_Id;
	pCore->Device_Id = controller.Device_Id;
	pCore->Revision_Id = controller.Revision_Id;
	for ( i=0; i<MAX_BASE_ADDRESS; i++ )
	{
		pCore->Base_Address[i] = controller.Base_Address[i];
	}
	pCore->Mmio_Base = controller.Base_Address[MV_PCI_BAR];

#endif /*_OS_LINUX*/
	pCore->Adapter_State = ADAPTER_INITIALIZING;
	MV_LIST_HEAD_INIT(&pCore->Waiting_List);
	MV_LIST_HEAD_INIT(&pCore->Internal_Req_List);

	if ( maxIo==1 )
		pCore->Is_Dump = MV_TRUE;
	else
		pCore->Is_Dump = MV_FALSE;

	if (flagSaved!=VS_REG_SIG) {	/* Added for tuning boot up time */
		/* This initialization is during boot up time, but not S3 */
		/* Read registers modified by BIOS to set detection flag */
		if ( (pCore->Device_Id==DEVICE_ID_THOR_4S1P_NEW) ||
			 (pCore->Device_Id==DEVICE_ID_THORLITE_2S1P_WITH_FLASH) ||
			 (pCore->Device_Id==DEVICE_ID_THORLITE_2S1P)) {
			MV_U32 tmpReg = 0;
			/* Read Bit[3] of PCI CNFG offset 60h to get flag for */
			/* PATA port enable/disable (0 - default, need to detect) */
#ifdef _OS_WINDOWS
#undef MV_PCI_READ_CONFIG_DWORD
#define MV_PCI_READ_CONFIG_DWORD(mod_ext, offset, reg) \
                reg = MV_PCI_READ_DWORD(mod_ext, offset)
			MV_PCI_READ_CONFIG_DWORD(pTopLayer, 0x60, tmpReg);
#else
			MV_PCI_READ_CONFIG_DWORD(pCore, 0x60, &tmpReg);
#endif /* _OS_WINDOWS */
			tmpReg &= MV_BIT(3);

			pCore->Flag_Fastboot_Skip |= (tmpReg >> 3);		/* bit 0 */
			/* Read Bit[10], Bit [11] of BAR5 offset A4h to get flag for */
			/* PATA device detection (0 - default, need to detect) and */
			/* PM detection (0 - default, need to detect) */
			tmpReg = MV_REG_READ_DWORD(pCore->Mmio_Base, VENDOR_DETECT) & 
						(VENDOR_DETECT_PATA | VENDOR_DETECT_PM);
			pCore->Flag_Fastboot_Skip |= (tmpReg >> 9);		/* bit 1, 2 */
		}
	}

	if ( (pCore->Device_Id==DEVICE_ID_THORLITE_2S1P)||(pCore->Device_Id==DEVICE_ID_THORLITE_2S1P_WITH_FLASH) )
	{
		pCore->SATA_Port_Num = 2;
		pCore->PATA_Port_Num = 1;
		pCore->Port_Num = 3;
		MV_DPRINT(("DEVICE_ID_THORLITE_2S1P is found.\n"));

	}
	else if ( pCore->Device_Id==DEVICE_ID_THORLITE_0S1P )
	{
		pCore->SATA_Port_Num = 0;
		pCore->PATA_Port_Num = 1;
		pCore->Port_Num = 1;
		MV_DPRINT(("DEVICE_ID_THORLITE_0S1P is found.\n"));

	}
	else
	{
		pCore->SATA_Port_Num = 4;
		pCore->PATA_Port_Num = 1;
		pCore->Port_Num = 5;
		MV_DPRINT(("DEVICE_ID_THOR is found.\n"));

	}

#if /*(VER_OEM==VER_OEM_ASUS) ||*/(VER_OEM==VER_OEM_INTEL)
	pCore->Port_Num -= pCore->PATA_Port_Num;
	pCore->PATA_Port_Num = 0;
#else
	if (pCore->Flag_Fastboot_Skip & FLAG_SKIP_PATA_PORT) {
		pCore->Port_Num -= pCore->PATA_Port_Num;
		pCore->PATA_Port_Num = 0;
	}
#endif

	if (pCore->Is_Dump) {
		sgEntryCount = MAX_SG_ENTRY_REDUCED;
		tagCount = 1;	/* do not support hot-plug when hibernation */
	}
	else {
		sgEntryCount = MAX_SG_ENTRY;
		tagCount = MAX_SLOT_NUMBER - 1;		/* reserve the last slot for PM hot-plug */
	}

#ifdef _OS_LINUX
#ifdef USE_NEW_SGVP
	sgEntryCount *= 2;
#endif
#endif /* _OS_LINUX */

	tmpSG = (MV_PTR_INTEGER)This + ROUNDING(sizeof(Core_Driver_Extension),8);
	temp = 	tmpSG + sizeof(MV_SG_Entry) * sgEntryCount * INTERNAL_REQ_COUNT;

	internalReqSize = MV_REQUEST_SIZE * INTERNAL_REQ_COUNT;
	MV_ASSERT( extensionSize >= ROUNDING(sizeof(Core_Driver_Extension),8) + internalReqSize );
	for ( i=0; i<INTERNAL_REQ_COUNT; i++ )
	{
		pReq = (PMV_Request)temp;
		pReq->SG_Table.Entry_Ptr = (PMV_SG_Entry)tmpSG;
		pReq->SG_Table.Max_Entry_Count = sgEntryCount;
		List_AddTail(&pReq->Queue_Pointer, &pCore->Internal_Req_List);
		tmpSG += sizeof(MV_SG_Entry) * sgEntryCount;
		temp += MV_REQUEST_SIZE;	/* MV_Request is 64bit aligned. */
	}	
//	temp = ROUNDING( (MV_PTR_INTEGER)temp, 8 );		/* Don't round the extension pointer */

	/* tag pool */
	tagSize = sizeof(MV_U16) * tagCount * MAX_PORT_NUMBER;

	for ( i=0; i<MAX_PORT_NUMBER; i++ )
	{
		port = &pCore->Ports[i];
		port->Tag_Pool.Stack = (MV_PU16)temp;
		port->Tag_Pool.Size = tagCount;
		temp += sizeof(MV_U16) * tagCount;
	}

#ifdef SUPPORT_CONSOLIDATE	
	// Allocate resource for Consolidate_Extension->Requests[].
	tmpSG = temp;
	temp = temp + sizeof(MV_SG_Entry) * sgEntryCount * CONS_MAX_INTERNAL_REQUEST_COUNT;

	if ( pCore->Is_Dump )
	{
		pCore->pConsolid_Device = NULL;
		pCore->pConsolid_Extent = NULL;
	}
	else
	{
		MV_ASSERT( extensionSize>=
			( ROUNDING(sizeof(Core_Driver_Extension),8) + internalReqSize + tagSize + ROUNDING(sizeof(Consolidate_Extension),8) + ROUNDING(sizeof(Consolidate_Device),8)*MAX_DEVICE_NUMBER )
			); 
		pCore->pConsolid_Extent = (PConsolidate_Extension)(temp);

		//Initialize some fields for pCore->pConsolid_Extent->Requests[i]
		for (i=0; i<CONS_MAX_INTERNAL_REQUEST_COUNT; i++)
		{
			pReq = &pCore->pConsolid_Extent->Requests[i];

			pReq->SG_Table.Max_Entry_Count = sgEntryCount;
			pReq->SG_Table.Entry_Ptr = (PMV_SG_Entry)tmpSG;
			tmpSG += sizeof(MV_SG_Entry) * sgEntryCount;
		}

		pCore->pConsolid_Device = (PConsolidate_Device)((MV_PTR_INTEGER)pCore->pConsolid_Extent + ROUNDING(sizeof(Consolidate_Extension),8));
	}
#endif

	/* Port_Map and Port_Num will be read from the register */

	/* Init port data structure */
	for ( i=0; i<pCore->Port_Num; i++ )
	{
		port = &pCore->Ports[i];
		
		port->Id = i;
		port->Port_State = PORT_STATE_IDLE;
		port->Core_Extension = pCore;
		port->Mmio_Base = (MV_PU8)pCore->Mmio_Base + 0x100 + (i * 0x80);
		port->Mmio_SCR = (MV_PU8)port->Mmio_Base + PORT_SCR;

		Tag_Init(&port->Tag_Pool, tagCount);

		for (j=0; j<MAX_DEVICE_PER_PORT; j++) 
		{
			port->Device[j].Id = i*MAX_DEVICE_PER_PORT + j;
			port->Device[j].PPort = port;
			port->Device[j].Is_Slave = 0;	/* Which one is the slave will be determined during discovery. */
#if defined(SUPPORT_TIMER) && defined(_OS_WINDOWS)
			port->Device[j].Timer_ID = NO_CURRENT_TIMER;
#endif
			port->Device[j].Reset_Count = 0;
		}

		port->Device_Number = 0;

		// Set function table for each port here.
		if ( i>=pCore->SATA_Port_Num )
			port->Type = PORT_TYPE_PATA;
		else
			port->Type = PORT_TYPE_SATA;

#ifdef COMMAND_ISSUE_WORKROUND
		OSSW_INIT_TIMER(&port->timer);	
		port->reset_hba_times= 0;
		mv_core_init_reset_para(port);
#endif

	}

	/* Get uncached memory */
#if defined(__MM_SE__) 	
	size = pCore->desc->ops->get_res_desc(RESOURCE_UNCACHED_MEMORY, maxIo);
	if (HBA_GetResource(pCore->desc,
			    RESOURCE_UNCACHED_MEMORY,
			    size,
			    &dmaResource))
	{
		//pCore->State = CORE_STATE_ZOMBIE;
		MV_ASSERT(MV_FALSE);
		return;
	}
#else
	/* Get uncached memory */
	HBA_GetResource(pCore, RESOURCE_UNCACHED_MEMORY, &dmaResource);

#endif /*_OS_LINUX*/	
	memVir = dmaResource.Virtual_Address;
	memDMA = dmaResource.Physical_Address;
	
	/* Assign uncached memory for command list (64 byte align) */
	offset = (MV_U32)(ROUNDING(memDMA.value,64)-memDMA.value);
	memDMA.value += offset;
	memVir = (MV_PU8)memVir + offset;
	for ( i=0; i<pCore->Port_Num; i++ )
	{
		port = &pCore->Ports[i];
		port->Cmd_List = memVir;
		port->Cmd_List_DMA = memDMA;
	#ifdef HIBERNATION_ROUNTINE
		if((!pCore->Is_Dump)|| (i==(pCore->Port_Num-1)))
	#endif
		{
			memVir = (MV_PU8)memVir + SATA_CMD_LIST_SIZE;
			memDMA.value += SATA_CMD_LIST_SIZE;
		}
	}

	/* Assign uncached memory for received FIS (64 byte align) */
	offset = (MV_U32)(ROUNDING(memDMA.value,64)-memDMA.value);
	memDMA.value += offset;
	memVir = (MV_PU8)memVir + offset;
	for ( i=0; i<pCore->SATA_Port_Num; i++ )
	{
		port = &pCore->Ports[i];	
		port->RX_FIS = memVir;
		port->RX_FIS_DMA = memDMA;
	#ifdef HIBERNATION_ROUNTINE
		if((!pCore->Is_Dump)|| (i==(pCore->SATA_Port_Num-1)))
	#endif
		{
			memVir = (MV_PU8)memVir + SATA_RX_FIS_SIZE;
			memDMA.value += SATA_RX_FIS_SIZE;
		}
	}

	/* Assign the 32 command tables. (128 byte align) */
	offset = (MV_U32)(ROUNDING(memDMA.value,128)-memDMA.value);
	memDMA.value += offset;
	memVir = (MV_PU8)memVir + offset;
	for ( i=0; i<pCore->Port_Num; i++ )
	{
		port = &pCore->Ports[i];
		port->Cmd_Table = memVir;
		port->Cmd_Table_DMA = memDMA;

		if ( !pCore->Is_Dump )
		{
			memVir = (MV_PU8)memVir + SATA_CMD_TABLE_SIZE * MAX_SLOT_NUMBER;
			memDMA.value += SATA_CMD_TABLE_SIZE * MAX_SLOT_NUMBER;
		}
		else
		{
		#ifdef HIBERNATION_ROUNTINE
			if(i==(pCore->Port_Num-1))
		#endif
			{
				memVir = (MV_PU8)memVir + SATA_CMD_TABLE_SIZE;
				memDMA.value += SATA_CMD_TABLE_SIZE;
			}
		}
	}

	/* Assign the scratch buffer (8 byte align) */
	offset = (MV_U32)(ROUNDING(memDMA.value,8)-memDMA.value);
	memDMA.value += offset;
	memVir = (MV_PU8)memVir + offset;
	for ( i=0; i<pCore->Port_Num; i++ )
	{
		port = &pCore->Ports[i];
		for ( j=0; j<MAX_DEVICE_PER_PORT; j++ )
		{
			port->Device[j].Scratch_Buffer = memVir;
			port->Device[j].Scratch_Buffer_DMA = memDMA;
		
		#ifdef HIBERNATION_ROUNTINE
			if((!pCore->Is_Dump)|| (i==(pCore->Port_Num-1)))
		#endif
			{
				memVir = (MV_PU8)memVir + SATA_SCRATCH_BUFFER_SIZE;
				memDMA.value += SATA_SCRATCH_BUFFER_SIZE;
			}
		}
	}

	/* Let me confirm the following assumption */
	MV_ASSERT( sizeof(SATA_FIS_REG_H2D)==sizeof(MV_U32)*FIS_REG_H2D_SIZE_IN_DWORD );
	MV_ASSERT( sizeof(MV_Command_Table)==0x80+MAX_SG_ENTRY*sizeof(MV_SG_Entry) );
	MV_ASSERT( sizeof(ATA_Identify_Data)==512 ); 

#ifdef SUPPORT_CONSOLIDATE
	if ( !pCore->Is_Dump )
	{
		Consolid_InitializeExtension(This);
		for ( i=0; i<MAX_DEVICE_NUMBER; i++ )
			Consolid_InitializeDevice(This, i);
	}
#endif
}

void Core_ModuleStart(MV_PVOID This)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;

	mvAdapterStateMachine(pCore,NULL);
}


void Core_ModuleShutdown(MV_PVOID This)
{
	/* 
	 * This function is equivalent to ahci_port_stop 
	 */
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	MV_U32 tmp, i;
	MV_LPVOID mmio;
	for ( i=0; i<pCore->Port_Num; i++ )
	{
		mmio = pCore->Ports[i].Mmio_Base;

		tmp = MV_REG_READ_DWORD(mmio, PORT_CMD);
		if ( pCore->Ports[i].Type==PORT_TYPE_SATA )
			tmp &= ~(PORT_CMD_START | PORT_CMD_FIS_RX);
		else
			tmp &= ~PORT_CMD_START;
		MV_REG_WRITE_DWORD(mmio, PORT_CMD, tmp);
		MV_REG_READ_DWORD(mmio, PORT_CMD); /* flush */

		/* 
		 * spec says 500 msecs for each PORT_CMD_{START,FIS_RX} bit, so
		 * this is slightly incorrect.
		 */
		HBA_SleepMillisecond(pCore, 500);
	}

	/* Disable the controller interrupt */
	tmp = MV_REG_READ_DWORD(pCore->Mmio_Base, HOST_CTL);
	tmp &= ~(HOST_IRQ_EN);
	MV_REG_WRITE_DWORD(pCore->Mmio_Base, HOST_CTL, tmp);
}

void Core_ModuleNotification(MV_PVOID This, enum Module_Event event, struct mod_notif_param *param)
{
}

void Core_HandleWaitingList(PCore_Driver_Extension pCore);
void Core_InternalSendRequest(MV_PVOID This, PMV_Request pReq);

void Core_ModuleSendRequest(MV_PVOID This, PMV_Request pReq)
{	
#ifdef SUPPORT_CONSOLIDATE
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	PDomain_Device pDevice;
	MV_U8 portId = PATA_MapPortId(pReq->Device_Id);
	MV_U8 deviceId = PATA_MapDeviceId(pReq->Device_Id);
	
	pDevice = &pCore->Ports[portId].Device[deviceId];
	
	if ( (!(pDevice->Device_Type&DEVICE_TYPE_ATAPI)) && (!pCore->Is_Dump) )
		Consolid_ModuleSendRequest(pCore, pReq);
	else
		Core_InternalSendRequest(pCore, pReq);
#else
	Core_InternalSendRequest(This, pReq);
#endif
}

void Core_InternalSendRequest(MV_PVOID This, PMV_Request pReq)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;

#ifdef SUPPORT_ATA_POWER_MANAGEMENT
	PDomain_Device pdev;
	PHBA_Extension phba;

	pdev = &pCore->Ports[PATA_MapPortId(pReq->Device_Id)].Device[PATA_MapDeviceId(pReq->Device_Id)];
	if((pdev->Status & DEVICE_STATUS_SLEEP)&&(pReq->Req_Type == REQ_TYPE_OS))
	{
		hba_remove_timer(pReq);
		Core_ResetChannel((MV_PVOID) pdev, NULL);
		pdev->Status &= ~DEVICE_STATUS_SLEEP;
	}
#endif
	/* Check whether we can handle this request */
	switch (pReq->Cdb[0])
	{
		case SCSI_CMD_INQUIRY:
		case SCSI_CMD_START_STOP_UNIT:
		case SCSI_CMD_TEST_UNIT_READY:
		case SCSI_CMD_READ_10:
		case SCSI_CMD_WRITE_10:
		case SCSI_CMD_VERIFY_10:
		case SCSI_CMD_READ_CAPACITY_10:
		case SCSI_CMD_REQUEST_SENSE:
		case SCSI_CMD_MODE_SELECT_10:
		case SCSI_CMD_MODE_SENSE_10:
		case SCSI_CMD_MARVELL_SPECIFIC:
		default:
			if ( pReq->Cmd_Initiator==pCore )
			{
				if ( !SCSI_IS_READ(pReq->Cdb[0]) && !SCSI_IS_WRITE(pReq->Cdb[0]) )
				{
					/* Reset request or request sense command. */
					List_Add(&pReq->Queue_Pointer, &pCore->Waiting_List);		/* Add to the header. */
				}
				else
				{
					#ifdef SUPPORT_CONSOLIDATE
					/* Consolidate request */
					MV_DASSERT( !pCore->Is_Dump );
					List_AddTail(&pReq->Queue_Pointer, &pCore->Waiting_List);	/* Append to the tail. */
					#else
					MV_ASSERT(MV_FALSE);
					#endif
				}
			}
			else
			{
				List_AddTail(&pReq->Queue_Pointer, &pCore->Waiting_List);		/* Append to the tail. */
			}
			Core_HandleWaitingList(pCore);
			break;
	}
}

void SATA_PrepareCommandHeader(PDomain_Port pPort, PMV_Request pReq, MV_U8 tag)
{
	MV_PHYSICAL_ADDR table_addr;
	PMV_Command_Header header = NULL;
	PMV_SG_Table pSGTable = &pReq->SG_Table;
#ifdef SUPPORT_PM
	PDomain_Device pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];
#endif
	header = SATA_GetCommandHeader(pPort, tag);
	/* 
	 * Set up the command header.
	 */
	header->FIS_Length = FIS_REG_H2D_SIZE_IN_DWORD;
	header->Packet_Command = (pReq->Cmd_Flag&CMD_FLAG_PACKET)?1:0;
	header->Reset = 0;
	header->NCQ = (pReq->Cmd_Flag&CMD_FLAG_NCQ)?1:0;

#ifdef SUPPORT_PM
	header->PM_Port = pDevice->PM_Number;
#else
	header->PM_Port = 0;
#endif
	*((MV_U16 *) header) = CPU_TO_LE_16( *((MV_U16 *) header) );
	header->PRD_Entry_Count = CPU_TO_LE_16(pSGTable->Valid_Entry_Count);

	table_addr.parts.high = pPort->Cmd_Table_DMA.parts.high;
	table_addr.parts.low = pPort->Cmd_Table_DMA.parts.low + SATA_CMD_TABLE_SIZE*tag;
	if ( table_addr.parts.low<pPort->Cmd_Table_DMA.parts.low ) {
		MV_DPRINT(("Cross 4G boundary.\n"));
		table_addr.parts.high++;
	}


	header->Table_Address = CPU_TO_LE_32(table_addr.parts.low);
	header->Table_Address_High = CPU_TO_LE_32(table_addr.parts.high);
}

void PATA_PrepareCommandHeader(PDomain_Port pPort, PMV_Request pReq, MV_U8 tag)
{
	MV_PHYSICAL_ADDR table_addr;
	PMV_PATA_Command_Header header = NULL;
	PMV_SG_Table pSGTable = &pReq->SG_Table;

	header = PATA_GetCommandHeader(pPort, tag);
	/* 
	 * Set up the command header.
	 * TCQ, Diagnostic_Command, Reset
	 * Table_Address and Table_Address_High are fixed. Needn't set every time.
	 */
	header->PIO_Sector_Count = 0;		/* Only for PIO multiple sector commands */
	header->Controller_Command = 0;
	header->TCQ = 0;
	header->Packet_Command = (pReq->Cmd_Flag&CMD_FLAG_PACKET)?1:0;

#ifdef USE_DMA_FOR_ALL_PACKET_COMMAND
	if ( pReq->Cmd_Flag&CMD_FLAG_PACKET )
	{
		header->DMA = (pReq->Cmd_Flag&CMD_FLAG_NON_DATA)?0:1;
	}
	else
	{
		header->DMA = (pReq->Cmd_Flag&CMD_FLAG_DMA)?1:0;
	}
#elif defined(USE_PIO_FOR_ALL_PACKET_COMMAND)
	if ( pReq->Cmd_Flag&CMD_FLAG_PACKET )
	{
		header->DMA = 0;
	}
	else
	{
		header->DMA = (pReq->Cmd_Flag&CMD_FLAG_DMA)?1:0;
	}
#else	
	header->DMA = (pReq->Cmd_Flag&CMD_FLAG_DMA)?1:0;
#endif

	header->Data_In = (pReq->Cmd_Flag&CMD_FLAG_DATA_IN)?1:0;
	header->Non_Data = (pReq->Cmd_Flag&CMD_FLAG_NON_DATA)?1:0;

	header->PIO_Sector_Command = 0;
	header->Is_48Bit = (pReq->Cmd_Flag&CMD_FLAG_48BIT)?1:0;
	header->Diagnostic_Command = 0;
	header->Reset = 0;

	header->Is_Slave = pPort->Device[PATA_MapDeviceId(pReq->Device_Id)].Is_Slave;

	*((MV_U16 *) header) = CPU_TO_LE_16( *((MV_U16 *) header) );
	header->PRD_Entry_Count = CPU_TO_LE_16(pSGTable->Valid_Entry_Count);

	table_addr.parts.high = pPort->Cmd_Table_DMA.parts.high;
	table_addr.parts.low = pPort->Cmd_Table_DMA.parts.low + SATA_CMD_TABLE_SIZE*tag;
	if ( table_addr.parts.low<pPort->Cmd_Table_DMA.parts.low ) {
		MV_DPRINT(("Cross 4G boundary.\n"));
		table_addr.parts.high++;
	}

	header->Table_Address = CPU_TO_LE_32(table_addr.parts.low);
	header->Table_Address_High = CPU_TO_LE_32(table_addr.parts.high);
}

/*
 * Fill SATA command table
 */
MV_VOID SATA_PrepareCommandTable(
	PDomain_Port pPort, 
	PMV_Request pReq, 
	MV_U8 tag,
	PATA_TaskFile pTaskFile
	)
{
#ifdef USE_NEW_SGTABLE
	PCore_Driver_Extension pCore = pPort->Core_Extension;
#else
	PMV_SG_Entry pSGEntry = NULL;
	MV_U8 i;
#endif
	PMV_Command_Table pCmdTable = Port_GetCommandTable(pPort, tag);

	PMV_SG_Table pSGTable = &pReq->SG_Table;

	/* Step 1: fill the command FIS: MV_Command_Table */
	SCSI_To_FIS(pPort->Core_Extension, pReq, tag, pTaskFile);

	/* Step 2. fill the ATAPI CDB */
	if ( pReq->Cmd_Flag&CMD_FLAG_PACKET )
	{
		MV_CopyMemory(pCmdTable->ATAPI_CDB, pReq->Cdb, MAX_CDB_SIZE);
	}

	/* Step 3: fill the PRD Table if necessary. */
	if ( (pSGTable) && (pSGTable->Valid_Entry_Count) )
	{
#ifdef USE_NEW_SGTABLE
		MV_U16 consumed;
		PMV_Command_Header header = NULL;

		header = SATA_GetCommandHeader(pPort, tag);
		consumed = (MV_U16)sgdt_prepare_hwprd(pCore, pSGTable, pCmdTable->PRD_Entry, HW_SG_ENTRY_MAX);
		if (consumed == 0) {
			/* resource not enough... */
			MV_DPRINT(("Run out of PRD entry.\n"));
			if( pReq->Req_Flag & REQ_FLAG_CONSOLIDATE )
			{
				pReq->Scsi_Status = REQ_STATUS_BUSY;
				Tag_ReleaseOne(&pPort->Tag_Pool, tag);
				return;
			}
			else
			{
				/* check why upper layer send request with too many sg items... */
				MV_DASSERT( MV_FALSE );
			}
		}
		header->PRD_Entry_Count = CPU_TO_LE_16(consumed);
#else /* USE_NEW_SGTABLE */
		if ( (pSGTable) && (pSGTable->Valid_Entry_Count) )
		{
			/* "Transfer Byte Count" in AHCI and 614x PRD table is zero based. */
			for ( i=0; i<pSGTable->Valid_Entry_Count; i++ )
			{
				pSGEntry = &pCmdTable->PRD_Entry[i];
				pSGEntry->Base_Address = CPU_TO_LE_32(pSGTable->Entry_Ptr[i].Base_Address);
				pSGEntry->Base_Address_High = CPU_TO_LE_32(pSGTable->Entry_Ptr[i].Base_Address_High);
				pSGEntry->Size = CPU_TO_LE_32(pSGTable->Entry_Ptr[i].Size-1);
			}
		}
		else
		{	
			MV_DASSERT( !SCSI_IS_READ(pReq->Cdb[0]) && !SCSI_IS_WRITE(pReq->Cdb[0]) );
		}
#endif /* USE_NEW_SGTABLE */
	}
	else
	{	
		MV_DASSERT( !SCSI_IS_READ(pReq->Cdb[0]) && !SCSI_IS_WRITE(pReq->Cdb[0]) );
	}
}

/*
 * Fill the PATA command table
*/
static MV_VOID PATA_PrepareCommandTable(
	PDomain_Port pPort, 
	PMV_Request pReq, 
	MV_U8 tag,
	PATA_TaskFile pTaskFile
	)
{
#ifdef USE_NEW_SGTABLE
	PCore_Driver_Extension pCore = pPort->Core_Extension;
#endif
	PMV_Command_Table pCmdTable = Port_GetCommandTable(pPort, tag);
	PMV_SG_Table pSGTable = &pReq->SG_Table;
	MV_PU8 pU8 = (MV_PU8)pCmdTable;
	MV_U8 device_index;

	device_index = PATA_MapDeviceId(pReq->Device_Id);

	/* Step 1: Fill the command block */
	(*pU8)=pTaskFile->Features; pU8++;
	(*pU8)=pTaskFile->Feature_Exp; pU8++;
	(*pU8)=pTaskFile->Sector_Count; pU8++;
	(*pU8)=pTaskFile->Sector_Count_Exp; pU8++;
	(*pU8)=pTaskFile->LBA_Low; pU8++;
	(*pU8)=pTaskFile->LBA_Low_Exp; pU8++;
	(*pU8)=pTaskFile->LBA_Mid; pU8++;
	(*pU8)=pTaskFile->LBA_Mid_Exp; pU8++;
	(*pU8)=pTaskFile->Command; pU8++;
	(*pU8)=pTaskFile->Device; pU8++;
	(*pU8)=pTaskFile->LBA_High; pU8++;
	(*pU8)=pTaskFile->LBA_High_Exp; pU8++;
	*((MV_PU32)pU8) = 0L;
    
	/* Step 2: Fill the ATAPI CDB */
	if ( pReq->Cmd_Flag&CMD_FLAG_PACKET )
	{
		MV_CopyMemory(pCmdTable->ATAPI_CDB, pReq->Cdb, MAX_CDB_SIZE);
	}

	/* Step 3: Fill the PRD Table if necessary. */
	if ( (pSGTable) && (pSGTable->Valid_Entry_Count) )
	{
#ifdef USE_NEW_SGTABLE
		MV_U16 consumed = (MV_U16) sgdt_prepare_hwprd(pCore, pSGTable, pCmdTable->PRD_Entry, HW_SG_ENTRY_MAX);
		PMV_PATA_Command_Header header = NULL;
		header = PATA_GetCommandHeader(pPort, tag);

		if (consumed == 0) {
			/* resource not enough... */
			MV_DPRINT(("Run out of PRD entry.\n"));
			if( pReq->Req_Flag & REQ_FLAG_CONSOLIDATE )
			{
				pReq->Scsi_Status = REQ_STATUS_BUSY;
				Tag_ReleaseOne(&pPort->Tag_Pool, tag);
				return;
			}
			else
			{
				/* check why upper layer send request with too many sg items... */
				MV_DASSERT( MV_FALSE );
			}
		}
		header->PRD_Entry_Count = CPU_TO_LE_16(consumed);
#else
		MV_U8 i;
		PMV_SG_Entry pSGEntry = NULL;

		/* "Transfer Byte Count" in AHCI and 614x PRD table is zero based. */
		for ( i=0; i<pSGTable->Valid_Entry_Count; i++ )
		{
			pSGEntry = &pCmdTable->PRD_Entry[i];
			pSGEntry->Base_Address = CPU_TO_LE_32(pSGTable->Entry_Ptr[i].Base_Address);
			pSGEntry->Base_Address_High = CPU_TO_LE_32(pSGTable->Entry_Ptr[i].Base_Address_High);
			pSGEntry->Size = CPU_TO_LE_32(pSGTable->Entry_Ptr[i].Size-1);
		}
#endif
	}
	else
	{	
		MV_DASSERT( !SCSI_IS_READ(pReq->Cdb[0]) && !SCSI_IS_WRITE(pReq->Cdb[0]) );
	}

}

void SATA_SendFrame(PDomain_Port pPort, PMV_Request pReq, MV_U8 tag)
{
	MV_LPVOID portMmio = pPort->Mmio_Base;
	#ifdef SUPPORT_ATA_SECURITY_CMD
	PDomain_Device pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];
	#endif
	MV_DASSERT( (pPort->Running_Slot&(1<<tag))==0 );
	MV_DASSERT( pPort->Running_Req[tag]==0 );
	MV_DASSERT( (MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE)&(1<<tag))==0 );
	MV_DASSERT( (MV_REG_READ_DWORD(portMmio, PORT_SCR_ACT)&(1<<tag))==0 );

	mv_core_set_running_slot(pPort, tag, pReq);
	#ifdef SUPPORT_ATA_SECURITY_CMD
	if ( pReq->Cmd_Flag&CMD_FLAG_NCQ && !((pDevice->Setting&DEVICE_SETTING_SECURITY_LOCKED)==0x10) )
	#else
	if ( pReq->Cmd_Flag&CMD_FLAG_NCQ)
	#endif
		pPort->Setting |= PORT_SETTING_NCQ_RUNNING;
	else
		pPort->Setting &= ~PORT_SETTING_NCQ_RUNNING;

	if ( pReq->Scsi_Status==REQ_STATUS_RETRY )
	{
		MV_DPRINT(("Retry request[0x%p] on port[%d]\n",pReq, pPort->Id));
		MV_DumpRequest(pReq, MV_FALSE);
		pPort->Setting |= PORT_SETTING_DURING_RETRY;
	}
	else
	{
		pPort->Setting &= ~PORT_SETTING_DURING_RETRY;
	}

	if ( pPort->Setting&PORT_SETTING_NCQ_RUNNING )
	{
		MV_REG_WRITE_DWORD(portMmio, PORT_SCR_ACT, 1<<tag);
		MV_REG_READ_DWORD(portMmio, PORT_SCR_ACT);	/* flush */
	}

	MV_REG_WRITE_DWORD(portMmio, PORT_CMD_ISSUE, 1<<tag);
	MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE);	/* flush */
}

MV_BOOLEAN Core_WaitingForIdle(MV_PVOID pExtension)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pExtension;
	PDomain_Port pPort = NULL;
	MV_U8 i;

	for ( i=0; i<pCore->Port_Num; i++ )
	{
		pPort = &pCore->Ports[i];

		if ( pPort->Running_Slot!=0 )
			return MV_FALSE;
	}
	
	return MV_TRUE;
}

MV_BOOLEAN ResetController(PCore_Driver_Extension pCore);

void Core_ResetHardware(MV_PVOID pExtension)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pExtension;
	MV_U32 i, j;
	PDomain_Port pPort = NULL;
	PDomain_Device pDevice = NULL;

	/* Re-initialize some variables to make the reset go. */
	pCore->Adapter_State = ADAPTER_INITIALIZING;
	for ( i=0; i<MAX_PORT_NUMBER; i++ )
	{
		pPort = &pCore->Ports[i];
		pPort->Port_State = PORT_STATE_IDLE;
		for ( j=0; j<MAX_DEVICE_PER_PORT; j++ )
		{
			pDevice = &pPort->Device[j];
			pDevice->State = DEVICE_STATE_IDLE;
		}
	}

	/* Go through the mvAdapterStateMachine. */
	if( pCore->Resetting==0 )
	{
		pCore->Resetting = 1;
		if( !mvAdapterStateMachine(pCore,NULL) )
		{
			MV_ASSERT(MV_FALSE);	
		}
	}
	else
	{
		/* I suppose that we only have one chance to call Core_ResetHardware. */
		MV_DASSERT(MV_FALSE);
	}
	
	return;
}

void PATA_LegacyPollSenseData(PCore_Driver_Extension pCore, PMV_Request pReq)
{
	/* 
	 * This sense data says:
	 * Format: Fixed format sense data
	 * Sense key: Hardware error
	 * Sense code and qualifier: 08h 03h LOGICAL UNIT COMMUNICATION CRC ERROR
	 */
	MV_U8 fakeSense[]={0xF0, 0x00, 0x04, 0x00, 0x00, 0x01, 
		0xEA, 0x0A, 0x74, 0x00, 0x00, 0x00, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00};
	MV_U32 size = MV_MIN(sizeof(fakeSense)/sizeof(MV_U8), pReq->Sense_Info_Buffer_Length);

	MV_CopyMemory(pReq->Sense_Info_Buffer, fakeSense, size);

}


void Core_FillSenseData(PMV_Request pReq, MV_U8 senseKey, MV_U8 adSenseCode)
{
	if (pReq->Sense_Info_Buffer != NULL) {
		((MV_PU8)pReq->Sense_Info_Buffer)[0] = 0x70;	/* Current */
		((MV_PU8)pReq->Sense_Info_Buffer)[2] = senseKey;
		((MV_PU8)pReq->Sense_Info_Buffer)[7] = 0;		/* additional sense length */
		((MV_PU8)pReq->Sense_Info_Buffer)[12] = adSenseCode;	/* additional sense code */
	}
}


void mvScsiInquiry(PCore_Driver_Extension pCore, PMV_Request pReq)
{
#ifndef _OS_BIOS
	PDomain_Device pDevice = NULL;
	MV_U8 portId, deviceId;

	portId = PATA_MapPortId(pReq->Device_Id);
	deviceId = PATA_MapDeviceId(pReq->Device_Id);
	if ( (portId>=MAX_PORT_NUMBER)||(pReq->Device_Id >= MAX_DEVICE_NUMBER) ) {
		pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return;
	}
	pDevice = &pCore->Ports[portId].Device[deviceId];
	if ( (pDevice->Status & DEVICE_STATUS_FUNCTIONAL) == 0) {
		/* Device is not functional */
		pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return;
	}

	MV_ZeroMemory(pReq->Data_Buffer, pReq->Data_Transfer_Length);

	if ( pReq->Cdb[1] & CDB_INQUIRY_EVPD )
	{
		MV_U8 MV_INQUIRY_VPD_PAGE0_DATA[6] = {0x00, 0x00, 0x00, 0x02, 0x00, 0x80};
		MV_U8 MV_INQUIRY_VPD_PAGE83_DATA[16] = {
			0x00, 0x83, 0x00, 0x0C, 0x01, 0x02, 0x00, 0x08,
			0x00, 0x50, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00};
		MV_U32 tmpLen = 0;
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;

		/* Shall return the specific page of Vital Production Data */
		switch (pReq->Cdb[2]) {
		case 0x00:	/* Supported VPD pages */
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, 6);
			MV_CopyMemory(pReq->Data_Buffer, MV_INQUIRY_VPD_PAGE0_DATA, tmpLen);
			break;
		case 0x80:	/* Unit Serial Number VPD Page */
			if (pReq->Data_Transfer_Length > 1)
				*(((MV_PU8)(pReq->Data_Buffer)) + 1) = 0x80;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, 4);
			if (tmpLen >= 4) {
				tmpLen = MV_MIN((pReq->Data_Transfer_Length-4), 20);
				MV_CopyMemory(((MV_PU8)(pReq->Data_Buffer)+4), pDevice->Serial_Number, tmpLen);
				*(((MV_PU8)(pReq->Data_Buffer)) + 3) = (MV_U8)tmpLen;
				tmpLen += 4;
			}
			break;
		case 0x83:	/* Device Identification VPD Page */
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, 16);
			MV_CopyMemory(pReq->Data_Buffer, MV_INQUIRY_VPD_PAGE83_DATA, tmpLen);
			break;
		default:
			pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
			Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
			break;
		}
		pReq->Data_Transfer_Length = tmpLen;
	} 
	else
	{
		/* Standard inquiry */
		if (pReq->Cdb[2]!=0) {
			/* PAGE CODE field must be zero when EVPD is zero for a valid request */
			/* sense key as ILLEGAL REQUEST and additional sense code as INVALID FIELD IN CDB */
			pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
			Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
			return;
		}
		else {
			MV_U8 Vendor[9],Product[24], temp[24];
			MV_U8 buff[42];
			MV_U32 i, inquiryLen = 42;

			MV_ZeroMemory(buff, inquiryLen);

			buff[0] = (pDevice->Device_Type&DEVICE_TYPE_ATAPI)?0x5 : 0;
			buff[1] = (pDevice->Device_Type&DEVICE_TYPE_ATAPI)?MV_BIT(7) : 0;    /* Not Removable disk */
			buff[2] = 0x05;    /*claim conformance to SPC-3*/
			buff[3] = 0x12;    /* set RESPONSE DATA FORMAT to 2*/
			buff[4] = 42 - 5;
			buff[6] = 0x0;     /* tagged queuing*/
			buff[7] = 0X13;	

			MV_CopyMemory(temp, pDevice->Model_Number, 24);
			for (i = 0; i < 9; i++)	{
				if (temp[i] == ' ')
					break;
			}
			if (i == 9)	{
				if (((temp[0] == 'I') && (temp[1] == 'C')) ||
					((temp[0] == 'H') && (temp[1] == 'T')) ||
					((temp[0] == 'H') && (temp[1] == 'D')) ||
					((temp[0] == 'D') && (temp[1] == 'K')))
				{ /*Hitachi*/
					Vendor[0] = 'H';
					Vendor[1] = 'i';
					Vendor[2] = 't';
					Vendor[3] = 'a';
					Vendor[4] = 'c';
					Vendor[5] = 'h';
					Vendor[6] = 'i';
					Vendor[7] = ' ';
					Vendor[8] = '\0';
				}
				else if ((temp[0] == 'S') && (temp[1] == 'T'))
				{
					/*Seagate*/
					Vendor[0] = 'S';
					Vendor[1] = 'e';
					Vendor[2] = 'a';
					Vendor[3] = 'g';
					Vendor[4] = 'a';
					Vendor[5] = 't';
					Vendor[6] = 'e';
					Vendor[7] = ' ';
					Vendor[8] = '\0';
				}
				else
				{
					/*Unkown*/
					Vendor[0] = 'A';
					Vendor[1] = 'T';
					Vendor[2] = 'A';
					Vendor[3] = ' ';
					Vendor[4] = ' ';
					Vendor[5] = ' ';
					Vendor[6] = ' ';
					Vendor[7] = ' ';
					Vendor[8] = '\0';
				}
				MV_CopyMemory(Product, temp, 16);
				Product[16] = '\0';
			}
			else {		/* i < 9 */
				MV_U32 j = i;
				MV_CopyMemory(Vendor, temp, j);
				for (; j < 9; j++)
					Vendor[j] = ' ';
				Vendor[8] = '\0';
				for (; i < 24; i++)	{
					if (temp[i] != ' ')
						break;
				}
				MV_CopyMemory(Product, &temp[i], 24 - i);
				Product[16] = '\0';
			}

			MV_CopyMemory(&buff[8], Vendor, 8);
			MV_CopyMemory(&buff[16], Product, 16);
			MV_CopyMemory(&buff[32], pDevice->Firmware_Revision, 4);
			MV_CopyMemory(&buff[36], "MVSATA", 6);	
			
			/*if pReq->Data_Transfer_Length <=36 ,buff[36]+ data miss*/
			MV_CopyMemory( pReq->Data_Buffer, 
							buff, 
							MV_MIN(pReq->Data_Transfer_Length, inquiryLen));
			pReq->Data_Transfer_Length =  MV_MIN(pReq->Data_Transfer_Length, inquiryLen);
			pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		}
	}
#endif	/* #ifndef _OS_BIOS */
}

void mvScsiReportLun(PCore_Driver_Extension pCore, PMV_Request pReq)
{
	MV_U32 allocLen, lunListLen;
	MV_PU8 pBuf = pReq->Data_Buffer;

	allocLen = ((MV_U32)(pReq->Cdb[6] << 24)) |
			   ((MV_U32)(pReq->Cdb[7] << 16)) |
			   ((MV_U32)(pReq->Cdb[8] << 8)) |
			   ((MV_U32)(pReq->Cdb[9]));

	/* allocation length should not less than 16 bytes */
	if (allocLen < 16) {
		pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
		Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
		return;
	}

	MV_ZeroMemory(pBuf, pReq->Data_Transfer_Length);
	/* Only LUN 0 has device */
	lunListLen = 8;
	pBuf[0] = (MV_U8)((lunListLen & 0xFF000000) >> 24);
	pBuf[1] = (MV_U8)((lunListLen & 0x00FF0000) >> 16);
	pBuf[2] = (MV_U8)((lunListLen & 0x0000FF00) >> 8);
	pBuf[3] = (MV_U8)(lunListLen & 0x000000FF);
	pReq->Scsi_Status = REQ_STATUS_SUCCESS;
}

void mvScsiReadCapacity(PCore_Driver_Extension pCore, PMV_Request pReq)
{
	PDomain_Device pDevice = NULL;
	MV_LBA maxLBA;
	MV_U32 blockLength;
	MV_PU32 pU32Buffer;
	MV_U8 portId, deviceId;

	portId = PATA_MapPortId(pReq->Device_Id);
	deviceId = PATA_MapDeviceId(pReq->Device_Id);
#ifndef SECTOR_SIZE
	#define SECTOR_SIZE	512	
#endif

	MV_DASSERT( portId < MAX_PORT_NUMBER );

	if ((pReq->Cdb[8] & MV_BIT(1)) == 0)
	{
		if ( pReq->Cdb[2] || pReq->Cdb[3] || pReq->Cdb[4] || pReq->Cdb[5] )
		{
			pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			return;
		}
	}

	/* 
	 * The disk size as indicated by the ATA spec is the total addressable
	 * sectors on the drive ; while the SCSI translation of the command
	 * should be the last addressable sector.
	 */
	pDevice = &pCore->Ports[portId].Device[deviceId];
//	maxLBA.value = pDevice->Max_LBA.value-1;
	maxLBA = pDevice->Max_LBA;
	blockLength = SECTOR_SIZE;			
	pU32Buffer = (MV_PU32)pReq->Data_Buffer;

	if (maxLBA.parts.high != 0)
		maxLBA.parts.low = 0xFFFFFFFF;

	pU32Buffer[0] = CPU_TO_BIG_ENDIAN_32(maxLBA.parts.low);
	pU32Buffer[1] = CPU_TO_BIG_ENDIAN_32(blockLength);

	pReq->Scsi_Status = REQ_STATUS_SUCCESS;
}

void mvScsiReadCapacity_16(PCore_Driver_Extension pCore, PMV_Request pReq)
{
	PDomain_Device pDevice = NULL;
	MV_U32 blockLength;
	MV_PU32 pU32Buffer;
	MV_U8 portId, deviceId;
	MV_LBA maxLBA;
	
	portId = PATA_MapPortId(pReq->Device_Id);
	deviceId = PATA_MapDeviceId(pReq->Device_Id);
#ifndef SECTOR_SIZE
	#define SECTOR_SIZE	512	
#endif
	MV_DASSERT( portId < MAX_PORT_NUMBER );

	pDevice = &pCore->Ports[portId].Device[deviceId];		
	//maxLBA.value = pDevice->Max_LBA.value-1;;
	maxLBA = pDevice->Max_LBA;
	blockLength = SECTOR_SIZE;
	pU32Buffer = (MV_PU32)pReq->Data_Buffer;
	pU32Buffer[0] = CPU_TO_BIG_ENDIAN_32(maxLBA.parts.high);
	pU32Buffer[1] = CPU_TO_BIG_ENDIAN_32(maxLBA.parts.low);
	pU32Buffer[2] =  CPU_TO_BIG_ENDIAN_32(blockLength);
	pReq->Scsi_Status = REQ_STATUS_SUCCESS;;
}

void Port_Monitor(PDomain_Port pPort);
#if defined(SUPPORT_ERROR_HANDLING)

MV_BOOLEAN Core_IsInternalRequest(PCore_Driver_Extension pCore, PMV_Request pReq)
{
	PDomain_Device pDevice;
	MV_U8 portId = PATA_MapPortId(pReq->Device_Id);
	MV_U8 deviceId = PATA_MapDeviceId(pReq->Device_Id);

	if ( portId>=MAX_PORT_NUMBER )
		return MV_FALSE;
	if ( deviceId>=MAX_DEVICE_PER_PORT )
		return MV_FALSE;

	pDevice = &pCore->Ports[portId].Device[deviceId];
	if ( pReq==pDevice->Internal_Req ) 
		return MV_TRUE;
	else
		return MV_FALSE;
}

void Core_ResetChannel_BH(MV_PVOID ext);

void Core_ResetChannel(MV_PVOID Device, MV_PVOID temp)
{
	PDomain_Device pDevice = (PDomain_Device)Device;
	PDomain_Port pPort = pDevice->PPort;
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PMV_Request pReq;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 tmp;
	MV_U16 i;
	
	//mv_core_dump_reg(pPort);
#ifdef SUPPORT_EVENT
	core_generate_event(pCore, EVT_ID_HD_TIMEOUT, pDevice->Id, SEVERITY_WARNING, 0, NULL);
#endif

	if (pPort->Type==PORT_TYPE_PATA){
		tmp = MV_REG_READ_DWORD( portMmio, PORT_CMD_ISSUE );
		MV_PRINT("CI = 0x%x\n", tmp);
	}
	Port_Monitor( pPort );

	/* toggle the start bit in cmd register */
	tmp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp & ~MV_BIT(0));
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp | MV_BIT(0));
	HBA_SleepMillisecond( pCore, 100 );
	pPort->timer_para = pDevice;
#ifdef COMMAND_ISSUE_WORKROUND
	if((MV_REG_READ_DWORD( portMmio, PORT_CMD ) & PORT_CMD_LIST_ON) || (pDevice->Reset_Count > CORE_MAX_RESET_COUNT))
	{
		pPort->Hot_Plug_Timer = 0;
		pPort->command_callback = Core_ResetChannel_BH;
		pPort->error_state = PORT_ERROR_AT_RUNTIME;
		pCore->Total_Device_Count--;
		mv_core_reset_command(pPort);
		return;
	}
//	mv_core_init_reset_para(pPort);
//	pPort->timer_para = pDevice;
#endif

	Core_ResetChannel_BH(pPort);

}
void Core_ResetChannel_BH(MV_PVOID ext)
{

	PDomain_Port pPort = (PDomain_Port)ext;
	PDomain_Device pDevice = (PDomain_Device)pPort->timer_para;
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PMV_Request pReq;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 tmp;
	MV_U16 i;
	pPort->timer_para = NULL;
	pDevice->Reset_Count++;

	mv_core_put_back_request(pPort);
	
	if ( pPort->Type == PORT_TYPE_PATA )
		PATA_PortReset( pPort, MV_TRUE );
	else
		SATA_PortReset( pPort, MV_TRUE );
	MV_DPRINT(("Finshed Core_ResetChannel_BH on port[%d].\n",pPort->Id));
	//mv_core_init_reset_para(pPort);
	Core_HandleWaitingList(pCore);
}
#endif /* SUPPORT_ERROR_HANDLING || _OS_LINUX */

#define IS_SOFT_RESET_REQ(pReq) \
	((pReq->Cdb[0]==SCSI_CMD_MARVELL_SPECIFIC)&& \
	 (pReq->Cdb[1]==CDB_CORE_MODULE)&& \
	 (pReq->Cdb[2]==CDB_CORE_SOFT_RESET_1 || pReq->Cdb[2]==CDB_CORE_SOFT_RESET_0))

MV_BOOLEAN HandleInstantRequest(PCore_Driver_Extension pCore, PMV_Request pReq)
{
	/* 
	 * Some of the requests can be returned immediately without hardware 
	 * access. 
	 * Handle Inquiry and Read Capacity.
	 * If return MV_TRUE, means the request can be returned to OS now.
	 */
	PDomain_Device pDevice = NULL;
	MV_U8 portId, deviceId;
	MV_U8 ret;
	if(IS_SOFT_RESET_REQ(pReq))
		return MV_FALSE;
	
#ifdef _OS_LINUX
	if(!__is_scsi_cmd_simulated(pReq->Cdb[0])
#ifdef SUPPORT_ATA_POWER_MANAGEMENT
	&& !IS_ATA_PASS_THROUGH_COMMAND(pReq)
#endif
	)
		return	MV_FALSE;
#endif
	if ( pReq->Device_Id != VIRTUAL_DEVICE_ID )
	{
		portId = PATA_MapPortId(pReq->Device_Id);
		deviceId = PATA_MapDeviceId(pReq->Device_Id);
		if ( portId < MAX_PORT_NUMBER )				
			pDevice = &pCore->Ports[portId].Device[deviceId];
	}

	if (pReq->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC && pReq->Cdb[1] == CDB_CORE_MODULE)
	{
		if (pReq->Cdb[2] == CDB_CORE_RESET_DEVICE)
		{
			Core_ResetChannel(pDevice,NULL);
			return MV_TRUE;
		}
	}

	if (pDevice && 
	    (pDevice->Device_Type & DEVICE_TYPE_ATAPI) &&
	    (pDevice->Status & DEVICE_STATUS_FUNCTIONAL))
	{
		return MV_FALSE;
	}

	ret=MV_TRUE;
#ifdef _OS_LINUX
	hba_map_sg_to_buffer(pReq);
#endif /* _OS_LINUX */
	switch ( pReq->Cdb[0] )
	{
	case SCSI_CMD_INQUIRY:
		mvScsiInquiry(pCore, pReq);
		break;
	case SCSI_CMD_MODE_SENSE_6:
	case SCSI_CMD_MODE_SENSE_10:
		mvScsiModeSense(pCore, pReq);
		break;
	case SCSI_CMD_REPORT_LUN:
		mvScsiReportLun(pCore, pReq);
		break;
	case SCSI_CMD_READ_CAPACITY_10:
		mvScsiReadCapacity(pCore, pReq);
		break;
#ifdef _OS_LINUX
	case SCSI_CMD_READ_CAPACITY_16: /* 0x9e SERVICE_ACTION_IN */
		if ((pReq->Cdb[1] & 0x1f)==SCSI_CMD_SAI_READ_CAPACITY_16 /* SAI_READ_CAPACITY_16 */) {
			mvScsiReadCapacity_16(pCore, pReq);
		}
		else
			pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		break;
#endif /* _OS_LINUX */
#ifdef SUPPORT_ATA_SMART
	case SCSI_CMD_MODE_SELECT_6:
	case SCSI_CMD_MODE_SELECT_10:
		ret = mvScsiModeSelect(pCore, pReq);
		break;
	case SCSI_CMD_LOG_SENSE:
		mvScsiLogSenseTranslation( pCore, pReq);
		break;
	case SCSI_CMD_READ_DEFECT_DATA_10:
		mvScsiReadDefectData(pCore, pReq);
		break;
	case SCSI_CMD_FORMAT_UNIT:
		pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
		Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
		break;
#endif
	case SCSI_CMD_REQUEST_SENSE:	/* This is only for Thor hard disk */
	case SCSI_CMD_TEST_UNIT_READY:
	case SCSI_CMD_RESERVE_6:	/* For Thor, just return good status */
	case SCSI_CMD_RELEASE_6:
#ifdef CORE_IGNORE_START_STOP_UNIT
	case SCSI_CMD_START_STOP_UNIT:
#endif
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
#ifdef CORE_SUPPORT_API
	case APICDB0_PD:
		ret=Core_pd_command(pCore, pReq);
		break;
#endif /* CORE_SUPPORT_API */
	default:
		ret = MV_FALSE;
	}
#ifdef _OS_LINUX
	hba_unmap_sg_to_buffer(pReq);
#endif /* _OS_LINUX */

	return ret;
}
#ifdef SUPPORT_ATA_SECURITY_CMD
void scsi_ata_check_condition(MV_Request *req, MV_U8 sense_key,
		MV_U8 sense_code, MV_U8 sense_qualifier)
	{
		 req->Scsi_Status = REQ_STATUS_HAS_SENSE;
		 if (req->Sense_Info_Buffer) {
			 ((MV_PU8)req->Sense_Info_Buffer)[0] = 0x70; /* As SPC-4, set Response Code 
	to 70h, or SCSI layer didn't know to set down error disk */
			((MV_PU8)req->Sense_Info_Buffer)[2] = sense_key;
			/* additional sense length */
			((MV_PU8)req->Sense_Info_Buffer)[7] = 0;
			/* additional sense code */
			((MV_PU8)req->Sense_Info_Buffer)[12] = sense_code;
			/* additional sense code qualifier*/
			((MV_PU8)req->Sense_Info_Buffer)[13] = sense_qualifier;
		 }
	}
u8 mv_ata_pass_through(IN PDomain_Device pDevice,IN PMV_Request req){

	MV_U8 protocol, t_length, t_dir, byte, multi_rw, command;
	MV_U32 length, tx_length = 0, cmd_flag =0;
	MV_PU8 buf_ptr;

	protocol = (req->Cdb[1] >> 1) & 0x0F;
	if(protocol== ATA_PROTOCOL_HARD_RESET || protocol==ATA_PROTOCOL_SRST){
		printk("ATA_16:don't support protocol=0x%x\n",protocol);
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
	multi_rw = (req->Cdb[1] >> 5) & 0x7;
	t_length = req->Cdb[2] & 0x3;
	byte = (req->Cdb[2] >> 2) & 0x01;
	t_dir = (req->Cdb[2] >> 3) & 0x1;

	/* Fix hdparm -A for readahead status, set sect_num */
	if (req->Cdb[0] == ATA_12) {
		command = req->Cdb[9];
		if (command == ATA_CMD_IDENTIFY_ATA && req->Cdb[4] == 0)
		req->Cdb[4] = 1;
	} else {
		command = req->Cdb[14];
		if (req->Cdb[1] & 0x01)
			cmd_flag |= CMD_FLAG_48BIT;
		if (command == ATA_CMD_IDENTIFY_ATA && req->Cdb[6] == 0)
			req->Cdb[6] = 1;
	}
	
	if (multi_rw != 0 && !IS_ATA_MULTIPLE_READ_WRITE(command)) {
			scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
	
		if (t_length == 0)
			cmd_flag |= CMD_FLAG_NON_DATA;
		else {
			if (t_dir == 0)
				cmd_flag |= CMD_FLAG_DATA_OUT;
			else
				cmd_flag |= CMD_FLAG_DATA_IN;
		}
	
		/* Transfer length in bytes */
		if (byte == 0) {
			/* Transfer length defined in Features */
			if (t_length == 0x1) {
				if (req->Cdb[0] == ATA_12)
					tx_length = req->Cdb[3];
				else
					tx_length = req->Cdb[4];
			}
			/* Transfer length defined in Sector Count */
			else if (t_length == 0x2) {
				if (req->Cdb[0] == ATA_12)
					tx_length = req->Cdb[4];
				else
					tx_length = req->Cdb[6];
			}
			/* t_length == 0x3 means use the length defined in the
			nexus transaction */
			else {
				tx_length = req->Data_Transfer_Length;
			}
			/* Transfer length in sectors */
		} else {
			/* Transfer length defined in Features */
			if (t_length == 0x1) {
				if (req->Cdb[0] == ATA_12)
					tx_length = req->Cdb[3] * 512; // fixed jyli
				else
					tx_length = req->Cdb[4] * 512; // fixed jyli
			}
			/* Transfer length defined in Sector Count */
			else if (t_length == 0x2) {
				if (req->Cdb[0] == ATA_12)
					tx_length = req->Cdb[4] * 512; // fixed jyli
				else
					tx_length = req->Cdb[6] * 512; // fixed jyli
			}
			/* t_length == 0x3 means use the length defined in the
			nexus transaction */
			else {
				
				tx_length = req->Data_Transfer_Length;
			}
		}
	switch (protocol) {
	case ATA_PROTOCOL_NON_DATA:
		if (t_length != 0) {
			scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

		break;

	case ATA_PROTOCOL_PIO_IN:
		if (!(cmd_flag & CMD_FLAG_DATA_IN)) {
			scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

		cmd_flag |= CMD_FLAG_PIO;
		break;

	case ATA_PROTOCOL_PIO_OUT:
		if (!(cmd_flag & CMD_FLAG_DATA_OUT)) {
			scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			        SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

		cmd_flag |= CMD_FLAG_PIO;
		break;

	case ATA_PROTOCOL_DMA:
		cmd_flag |= CMD_FLAG_DMA;
		break;
	case ATA_PROTOCOL_DMA_QUEUED:
		cmd_flag |= (CMD_FLAG_DMA | CMD_FLAG_TCQ);
		break;

	case ATA_PROTOCOL_DEVICE_DIAG:
	case ATA_PROTOCOL_DEVICE_RESET:
		/* Do nothing(?) */
		break;

	case ATA_PROTOCOL_UDMA_IN:
		if (!(cmd_flag & CMD_FLAG_DATA_IN)) {
			scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
		cmd_flag |= CMD_FLAG_DMA;
		break;

	case ATA_PROTOCOL_UDMA_OUT:
		if (!(cmd_flag & CMD_FLAG_DATA_OUT)) {
			scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
		cmd_flag |= CMD_FLAG_DMA;
		break;

	case ATA_PROTOCOL_FPDMA:
		cmd_flag |= (CMD_FLAG_DMA | CMD_FLAG_NCQ);
		break;

	case ATA_PROTOCOL_RTN_INFO:
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;

	default:
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB, 0);

		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	 req->Cmd_Flag=cmd_flag;
	return 1;
}
#endif
MV_QUEUE_COMMAND_RESULT
PrepareAndSendCommand(
	IN PCore_Driver_Extension pCore,
	IN PMV_Request pReq
	)
{
	PDomain_Device pDevice = NULL;
	PDomain_Port pPort = NULL;
	MV_BOOLEAN isPATA = MV_FALSE;
	MV_U8 i, tag;
//	MV_U8 count=0;
	ATA_TaskFile taskFile;
	MV_BOOLEAN ret;

	/* Associate this request to the corresponding device and port */
	pDevice = &pCore->Ports[PATA_MapPortId(pReq->Device_Id)].Device[PATA_MapDeviceId(pReq->Device_Id)];
	pPort = pDevice->PPort;

	if ( !(pDevice->Status&DEVICE_STATUS_FUNCTIONAL) )
	{
		pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	/* Set the Cmd_Flag to indicate which type of commmand it is. */
	if ( !Category_CDB_Type(pDevice, pReq) )
	{
		pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		/* Invalid request and can be returned to OS now. */
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	} 
#ifdef SUPPORT_ATA_SECURITY_CMD
	else if(pReq->Cdb[0]==ATA_16)
		{
			ret=mv_ata_pass_through(pDevice,pReq);
			if(ret==MV_QUEUE_COMMAND_RESULT_FINISHED)
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
#endif
	MV_DASSERT( pPort!=NULL );
	if ( pPort->Running_Slot!=0 )	/* Some requests are running. */
	{

#ifdef COMMAND_ISSUE_WORKROUND
		if(mv_core_check_is_reseeting(pCore)){
			MV_DPRINT(("HBA is resetting, wait...\n"));
			MV_DumpRequest(pReq, 0);
			return MV_QUEUE_COMMAND_RESULT_FULL;
		}
#endif

		if ((pDevice->Outstanding_Req >= MAX_SLOT_NUMBER - 2)
			|| (pPort->Running_Slot == 0xFFFFFFFFL)){
			MV_DPRINT(("running slot[0x%x], req=%d is full.\n",pPort->Running_Slot, pDevice->Outstanding_Req));
			return MV_QUEUE_COMMAND_RESULT_FULL;
		}
		
		if (pReq->Cmd_Flag & CMD_FLAG_SMART)
			return MV_QUEUE_COMMAND_RESULT_FULL;
		
		if (	( (pReq->Cmd_Flag&CMD_FLAG_NCQ) && !(pPort->Setting&PORT_SETTING_NCQ_RUNNING) )
				||  ( !(pReq->Cmd_Flag&CMD_FLAG_NCQ) && (pPort->Setting&PORT_SETTING_NCQ_RUNNING) )
				|| (pReq->Scsi_Status==REQ_STATUS_RETRY)
				|| (pPort->Setting&PORT_SETTING_DURING_RETRY)
			)
		{
			return MV_QUEUE_COMMAND_RESULT_FULL;			
		}
	
		if((pReq->Cmd_Flag&CMD_FLAG_NCQ)&&(pPort->Setting&PORT_SETTING_NCQ_RUNNING)&&(pPort->Running_Slot==0xffffffffL))
		{
			MV_PRINT("NCQ TAG is run out\n");
			return MV_QUEUE_COMMAND_RESULT_FULL;			
		}
	
		if((!(pReq->Cmd_Flag&CMD_FLAG_NCQ))&&(!(pPort->Setting&PORT_SETTING_NCQ_RUNNING))&&(pPort->Running_Slot==0xffffffffL))
		{
			MV_PRINT("DMA TAG is run out\n");
			return MV_QUEUE_COMMAND_RESULT_FULL;
		}

		
		/* In order for request sense to immediately follow the error request. */
		if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
			return MV_QUEUE_COMMAND_RESULT_FULL;

		if ((pPort->Type==PORT_TYPE_PATA)&&
			(pPort->Port_State==PORT_STATE_INIT_DONE)){			
			for (i=0; i<2; i++){
				if ((&pPort->Device[i]!=pDevice)&& //check the other device				
				    (pPort->Device[i].Device_Type&DEVICE_TYPE_ATAPI)&&
				    (pPort->Device[i].Status & DEVICE_STATUS_FUNCTIONAL)){					
						return MV_QUEUE_COMMAND_RESULT_FULL;
				}
			}//end of for	
		}
		
		/* One request at a time */
		if ( (pReq->Scsi_Status==REQ_STATUS_RETRY)
			|| (pPort->Setting&PORT_SETTING_DURING_RETRY) 
			)
			return MV_QUEUE_COMMAND_RESULT_FULL;
	}

	if ( Tag_IsEmpty(&pPort->Tag_Pool) )
		return MV_QUEUE_COMMAND_RESULT_FULL;

	isPATA = (pPort->Type==PORT_TYPE_PATA)?1:0;

	/* Get one slot for this request. */
	tag = (MV_U8)Tag_GetOne(&pPort->Tag_Pool);

	if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
		ret = ATAPI_CDB2TaskFile(pDevice, pReq, &taskFile);
	else
		ret = ATA_CDB2TaskFile(pDevice, pReq, tag, &taskFile);
	if ( !ret )
	{
		pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		Tag_ReleaseOne(&pPort->Tag_Pool, tag);
		/* Invalid request and can be returned to OS now. */
		return MV_QUEUE_COMMAND_RESULT_FINISHED;	
	}

	if ( !isPATA )
		SATA_PrepareCommandHeader(pPort, pReq, tag);
	else
		PATA_PrepareCommandHeader(pPort, pReq, tag);


	if ( !isPATA )
		SATA_PrepareCommandTable(pPort, pReq, tag, &taskFile);
	else
		PATA_PrepareCommandTable(pPort, pReq, tag, &taskFile);

	/* in some cases, when preparing command table, a consolidated 
	   request would cause PRD entries to run out. In this case, we
	   return this request to re-try without consolidating */
	/* This is assuming that REQ_STATUS_BUSY is ONLY used for these cases */
	if (pReq->Scsi_Status == REQ_STATUS_BUSY)
		return MV_QUEUE_COMMAND_RESULT_FINISHED;

	SATA_SendFrame(pPort, pReq, tag);
	/* Request is sent to the hardware and not finished yet. */
	return MV_QUEUE_COMMAND_RESULT_SENT;
}

void Core_HandleWaitingList(PCore_Driver_Extension pCore)
{
	PMV_Request pReq = NULL;
	MV_QUEUE_COMMAND_RESULT result;
#ifdef SUPPORT_HOT_PLUG	
	PDomain_Device pDevice;
	MV_U8 portId, deviceId;
#endif	
#if defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX)
	MV_U32 timeout;
#endif /* efined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX) */

	/* Get the request header */
	while ( !List_Empty(&pCore->Waiting_List) )
	{
		pReq = (PMV_Request) List_GetFirstEntry(&pCore->Waiting_List, 
							MV_Request, 
							Queue_Pointer);
		if ( NULL == pReq ) {
			MV_ASSERT(0);
			break;
		}

		if (pReq->Cdb[0]==0xEC){
			MV_DPRINT(("Catch the 0xEC cmd.\n"));
		}

#if defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX)
		pReq->eh_flag = 0;
//		hba_init_timer(pReq);
#endif /* defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX) */

		/* During reset, we still have internal requests need to 
		 *be handled. */

		// Internal request is always at the beginning.
		if ( (pCore->Need_Reset)&&(pReq->Cmd_Initiator!=pCore) ) 
		{
			/* Return the request back. */
			List_Add(&pReq->Queue_Pointer, &pCore->Waiting_List);
			return;
		}
	
#ifdef SUPPORT_HOT_PLUG
		/* hot plug - device is gone, reject this request */
		if ( pReq->Device_Id != VIRTUAL_DEVICE_ID )
		{
			portId = PATA_MapPortId(pReq->Device_Id);
			deviceId = PATA_MapDeviceId(pReq->Device_Id);
			pDevice = &pCore->Ports[portId].Device[deviceId];

			if ( !(pDevice->Status & DEVICE_STATUS_FUNCTIONAL) )
			{
				pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
				CompleteRequest(pCore, pReq, NULL);
				return;
			}

			/* Reset is not done yet. */
			if ( pDevice->State!=DEVICE_STATE_INIT_DONE )
			{
				/* check if it is the reset commands */
				if ( !Core_IsInternalRequest(pCore, pReq) )
				{
					List_Add(&pReq->Queue_Pointer, &pCore->Waiting_List); /* Return the request back. */
					return;
				} 
				else 
				{
					/* 
					 * Cannot be the request sense. 
					 * It's not pushed back. 
					 */
					MV_ASSERT( !SCSI_IS_REQUEST_SENSE(pReq->Cdb[0]) );
				}
			}
		}
#endif /* SUPPORT_HOT_PLUG */

		/* Whether we can handle this request without hardware access? */
		if ( HandleInstantRequest(pCore, pReq) ) 
		{
			CompleteRequest(pCore, pReq, NULL);
			continue;
		}

		/* handle the cmd which data length is > 128k 
		 * We suppose the data length was multiples of 128k first. 
		 * If not, we will still verify multiples of 128k since 
		 * no data transfer.
		 */
		if(pReq->Cdb[0] == SCSI_CMD_VERIFY_10)
		{
			PDomain_Device pDevice = &pCore->Ports[PATA_MapPortId(pReq->Device_Id)].Device[PATA_MapDeviceId(pReq->Device_Id)];
			MV_U32 sectors = SCSI_CDB10_GET_SECTOR(pReq->Cdb);
			
			if((!(pDevice->Capacity&DEVICE_CAPACITY_48BIT_SUPPORTED)) && (sectors > MV_MAX_TRANSFER_SECTOR)){
				MV_ASSERT(!pReq->Splited_Count );
				pReq->Splited_Count = (MV_U8)((sectors + MV_MAX_TRANSFER_SECTOR -1)/MV_MAX_TRANSFER_SECTOR) - 1;
				sectors = MV_MAX_TRANSFER_SECTOR; 
				SCSI_CDB10_SET_SECTOR(pReq->Cdb, sectors);
			}
		}

		result = PrepareAndSendCommand(pCore, pReq);	

		switch ( result )
		{
			case MV_QUEUE_COMMAND_RESULT_FINISHED:
				CompleteRequest(pCore, pReq, NULL);
				break;

			case MV_QUEUE_COMMAND_RESULT_FULL:
				List_Add(&pReq->Queue_Pointer, &pCore->Waiting_List);
				return;

			case MV_QUEUE_COMMAND_RESULT_SENT:
			{
				portId = PATA_MapPortId(pReq->Device_Id);
				deviceId = PATA_MapDeviceId(pReq->Device_Id);
				pDevice = &pCore->Ports[portId].Device[deviceId];
				pDevice->Outstanding_Req++;
#if defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX)
				/*
				 * timeout to 15 secs if the port has just
				 * been reset.
				 */
				if ( pReq->eh_flag ) {
					timeout = HBA_REQ_TIMER_AFTER_RESET;
					pReq->eh_flag = 0;
				} else {
					timeout = HBA_REQ_TIMER;
				}

				if (pDevice->Device_Type & DEVICE_TYPE_ATAPI)
					timeout = timeout * 20 + 5;
			#ifdef SUPPORT_ATA_SMART
				if(pReq->Cdb[0]==SCSI_CMD_SND_DIAG) 
					timeout = timeout * 10;
			#endif
				hba_init_timer(pReq);

				hba_add_timer(pReq, 
					      timeout, 
					      __core_req_timeout_handler);
				/*in captive mode, maybe timeout.*/
			#ifdef SUPPORT_ATA_SMART
				if(pReq->Cdb[0]== SCSI_CMD_MARVELL_SPECIFIC &&
					pReq->Cdb[1]== CDB_CORE_MODULE &&
						pReq->Cdb[2] == CDB_CORE_ATA_SMART_IMMEDIATE_OFFLINE){
							hba_remove_timer(pReq);
				}
			#endif
#elif defined(SUPPORT_ERROR_HANDLING)
#ifdef SUPPORT_TIMER
				/* start timer for error handling */
				if( pDevice->Timer_ID == NO_CURRENT_TIMER )
				{
					// if no timer is running right now	
					if (pDevice->Device_Type&DEVICE_TYPE_ATAPI){
//						MV_DASSERT(pDevice->Outstanding_Req==1);
						if (pReq->Time_Out!=0){							
							pDevice->Timer_ID = Timer_AddRequest( pCore, pReq->Time_Out*2, Core_ResetChannel, pDevice, NULL );
						}else {
							pDevice->Timer_ID = Timer_AddRequest( pCore, REQUEST_TIME_OUT, Core_ResetChannel, pDevice, NULL );
						}
					}else {
						pDevice->Timer_ID = Timer_AddRequest( pCore, REQUEST_TIME_OUT, Core_ResetChannel, pDevice, NULL );
					}				
				}
#endif /* SUPPORT_TIMER */
#endif /* defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX) */
#if 0
				{
					MV_U8 i;
					PMV_Request pTmpRequest = NULL;
					PDomain_Port pPort = pDevice->PPort;
					/* When there is reset command, other commands won't come here. */
					if ( SCSI_IS_READ(pReq->Cdb[0]) || SCSI_IS_WRITE(pReq->Cdb[0]) )
					{
						for ( i=0; i<MAX_SLOT_NUMBER; i++ )
						{
							pTmpRequest = pPort->Running_Req[i];
							if ( pTmpRequest && (pTmpRequest->Device_Id==pReq->Device_Id) ) 
							{
								MV_DASSERT( !SCSI_IS_INTERNAL(pTmpRequest->Cdb[0]) );
							}
						}

					}
				}
#endif /* 0 */
				break;
			}
			default:
				MV_ASSERT(MV_FALSE);
		}
	}
	
#ifdef SUPPORT_CONSOLIDATE
	{
		MV_U8 i,j;
		PDomain_Port pPort;

		if ( pCore->Is_Dump ) return;

		/* 
		* If there is no more request we can do, 
		* force command consolidate to run the holding request. 
		*/
		for ( i=0; i<MAX_PORT_NUMBER; i++ )
		{
			pPort = &pCore->Ports[i];
			for ( j=0; j<MAX_DEVICE_PER_PORT; j++ )
			{
				if ( (pPort->Device[j].Status&DEVICE_STATUS_FUNCTIONAL)
					&& (pPort->Device[j].Outstanding_Req==0) )
				{
					Consolid_PushSendRequest(pCore, i*MAX_DEVICE_PER_PORT+j);
				}
			}
		}
	}
#endif /* SUPPORT_CONSOLIDATE */
}

/*
 * Interrupt service routine and related funtion
 * We can split this function to two functions. 
 * One is used to check and clear interrupt, called in ISR. 
 * The other is used in DPC.
 */
void SATA_PortHandleInterrupt(
	IN PCore_Driver_Extension pCore,
	IN PDomain_Port pPort
	);
void PATA_PortHandleInterrupt(
	IN PCore_Driver_Extension pCore,
	IN PDomain_Port pPort
	);
void SATA_HandleSerialError(
	IN PDomain_Port pPort,
	IN MV_U32 serialError
	);
void SATA_HandleHotplugInterrupt(
	IN PDomain_Port pPort,
	IN MV_U32 intStatus
	);

MV_BOOLEAN Core_InterruptServiceRoutine(MV_PVOID This)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	MV_U32	irqStatus;
	MV_U8 i;
	PDomain_Port pPort = NULL;

	/* Get interrupt status */
	irqStatus = MV_REG_READ_DWORD(pCore->Mmio_Base, HOST_IRQ_STAT);
	irqStatus &= pCore->Port_Map;
#ifndef  SUPPORT_TASKLET
	if (!irqStatus) {
		return MV_FALSE;
	}

	for ( i=0; i<pCore->Port_Num; i++ ) 
	{
		/* no interrupt for this port. */
		if (!(irqStatus&(1<<i)))
			continue;

		pPort = &pCore->Ports[i];
		if ( pPort->Type==PORT_TYPE_PATA )
			PATA_PortHandleInterrupt(pCore, pPort);
		else
			SATA_PortHandleInterrupt(pCore, pPort);
	}

	/* If we need to do hard reset. And the controller is idle now. */
	if ((pCore->Need_Reset) && (!pCore->Resetting))
	{
		if (Core_WaitingForIdle(pCore))
			Core_ResetHardware(pCore);
	}

	Core_HandleWaitingList(pCore);
#else
	pCore->Saved_ISR_Status=irqStatus;
#endif
	return MV_TRUE;
}

#ifdef  SUPPORT_TASKLET
MV_BOOLEAN Core_HandleServiceRoutine(MV_PVOID This)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	MV_U8 i;
	PDomain_Port pPort = NULL;
	for ( i=0; i<pCore->Port_Num; i++ ) 
	{
		/* no interrupt for this port. */
		if (!(pCore->Saved_ISR_Status&(1<<i)))
			continue;

		pPort = &pCore->Ports[i];
		if ( pPort->Type==PORT_TYPE_PATA )
			PATA_PortHandleInterrupt(pCore, pPort);
		else
			SATA_PortHandleInterrupt(pCore, pPort);
	}

	/* If we need to do hard reset. And the controller is idle now. */
	if ((pCore->Need_Reset) && (!pCore->Resetting))
	{
		if (Core_WaitingForIdle(pCore))
			Core_ResetHardware(pCore);
	}

	Core_HandleWaitingList(pCore);

	return MV_TRUE;
}
#endif

void SATA_HandleSerialError(
	IN PDomain_Port pPort,
	IN MV_U32 serialError
	)
{
	MV_DPRINT(("Error: port=%d  Serial error=0x%x.\n", pPort->Id, serialError));
}

void SATA_ResetPort(PCore_Driver_Extension pCore, MV_U8 portId);

#ifdef COMMAND_ISSUE_WORKROUND

void SATA_ResetPort(PCore_Driver_Extension pCore, MV_U8 portId);
MV_BOOLEAN ResetController(PCore_Driver_Extension pCore);
void InitChip(PCore_Driver_Extension pCore);
void mvHandleDevicePlugin_BH(MV_PVOID ext);

#define CORE_ERROR_HANDLE_RESET_DEVICE		1
#define CORE_ERROR_HANDLE_RESET_PORT			2
#define CORE_ERROR_HANDLE_RESET_HBA			3


MV_U8 mv_core_reset_port(PDomain_Port pPort)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pPort->Core_Extension;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_U32 temp=0,count=0;
	MV_U32 old_stat;
	MV_U32 issue_reg=0;
	MV_U8 ret=MV_TRUE;

	//mvDisableIntr(portMmio, old_stat);

	/* Toggle should before we clear the channel interrupt status but not the global interrupt. */
	MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));
#if 1
	{
		MV_U16 counter;
		{
			/*Doing PHY reset*/
			MV_U32 SControl = MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);
			SControl &= ~0x000000FF;
#ifdef FORCE_1_5_G
			SControl |= 0x11;
#else
			SControl |= 0x21;
#endif
			MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
			MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);	/* flush */
			HBA_SleepMillisecond(pCore, 2);
//			hba_msleep(2);

			SControl &= ~0x0000000F;
			MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
			MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);	/* flush */
			HBA_SleepMillisecond(pCore, 10);
//			hba_msleep(10);
		}

		/*Waiting PHY Ready*/
		counter = 200;
		while(((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT) & 0x0f) != 0x03) && (counter > 0)) {
			HBA_SleepMillisecond(pCore, 10);
			//hba_msleep(10);
			counter --;
		}

		if (counter > 0) {
			/*Some HD update status too slow, so the workaround to wait HD ready*/
			counter = 200;
			while (((MV_REG_READ_DWORD(portMmio, PORT_TFDATA) & 0xff) != 0x50) && (counter > 0)) {
				HBA_SleepMillisecond(pCore, 10);
				//hba_msleep(10);
				counter --;
			}
		}
	}
#endif

	/* Always turn the PM bit on - otherwise won't work! */
	temp = MV_REG_READ_DWORD(portMmio, PORT_CMD);
	MV_REG_WRITE_DWORD(portMmio, PORT_CMD, temp | MV_BIT(17));
	temp=MV_REG_READ_DWORD(portMmio, PORT_CMD);	/* flush */
#if 1
	/*Clear Busy bit and error bit, do spin-up device, power on device*/
	temp = MV_REG_READ_DWORD(portMmio, PORT_CMD);					
	MV_REG_WRITE_DWORD(portMmio, PORT_CMD, temp | MV_BIT(3) | MV_BIT(2) | MV_BIT(1));
	temp=MV_REG_READ_DWORD(portMmio, PORT_CMD);	/* flush */
#endif
	/* hardware workaround - send dummy FIS first to clear FIFO */
	temp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp & ~PORT_CMD_START);
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp |PORT_CMD_START);
	Tag_Init( &pPort->Tag_Pool, pPort->Tag_Pool.Size );
	//sendDummyFIS( pPort );

	// start command handling on this port
	temp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp & ~PORT_CMD_START);
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp | PORT_CMD_START);
	HBA_SleepMillisecond(pCore, 2000);
	//hba_msleep(2000);
	// reset the tag stack - to guarantee soft reset is issued at slot 0
	Tag_Init( &pPort->Tag_Pool, pPort->Tag_Pool.Size );

	// make sure CI is cleared before moving on
	issue_reg = MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE);
	while (issue_reg != 0 && count < 10) {
		count++;
		temp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
		MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp & ~PORT_CMD_START);
		MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp | PORT_CMD_START);
		HBA_SleepMillisecond(pCore, 1000);
		//hba_msleep(1000);
		issue_reg = MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE);
	}
	
	//mvEnableIntr(portMmio, old_stat);

	temp = MV_REG_READ_DWORD(portMmio, PORT_IRQ_STAT);
	if(temp){
		MV_DPRINT(("port reset but port status can not be clean[0x%x] on port[%d].\n",MV_REG_READ_DWORD( portMmio, PORT_IRQ_STAT), pPort->Id));
		MV_REG_WRITE_DWORD(portMmio, PORT_IRQ_STAT, temp);
	}

	if(MV_REG_READ_DWORD( portMmio, PORT_CMD) & PORT_CMD_LIST_ON){
		MV_PRINT("port reset but command running can not be clean[0x%x] on port[%d].\n",MV_REG_READ_DWORD( portMmio, PORT_CMD_ISSUE), pPort->Id);
		ret = MV_FALSE;
	}

	if (issue_reg != 0)
	{
		MV_PRINT("port reset but CI can not be clean[0x%x] on port[%d].\n",MV_REG_READ_DWORD( portMmio, PORT_CMD_ISSUE), pPort->Id);
		ret = MV_FALSE;
	}	

	return ret;
}


MV_U8 mv_core_reset_hba(PDomain_Port pPort)
{

	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pPort->Core_Extension;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_U32 tmp;
	pCore->error_handle_state = CORE_ERROR_HANDLE_RESET_HBA;
	MV_REG_WRITE_DWORD(mmio, HOST_CTL, 0);
	MV_REG_WRITE_DWORD(portMmio, PORT_IRQ_MASK, 0);

	//Reset port command start.
	tmp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp & ~PORT_CMD_START);
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, tmp | PORT_CMD_START);
	HBA_SleepMillisecond(pCore, 500);
	//hba_msleep(500);

	if(ResetController(pCore) == MV_FALSE) {
		MV_DPRINT(("Reset controller failed."));
		return MV_FALSE;

	}
	InitChip(pCore);


	MV_DPRINT(("Finished reset hba for port[%d], CMD=0x%x.\n",pPort->Id, MV_REG_READ_DWORD(portMmio, PORT_CMD)));
	//mv_core_dump_reg(pPort);
	return MV_TRUE;
}

void mv_core_reset_command_in_timer(PDomain_Port pPort)
{
	unsigned long flags;
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pPort->Core_Extension;
	MV_DPRINT(("start handle reset command in timer for port[%d].\n",pPort->Id));
	spin_lock_irqsave(&pCore->desc->hba_desc->global_lock, flags);
	mv_core_reset_command(pPort);
	spin_unlock_irqrestore(&pCore->desc->hba_desc->global_lock, flags);

}

MV_U8 mv_core_check_is_reseeting(MV_PVOID core_ext)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)core_ext;
	return	pCore->resetting_command;
}


void mv_core_init_reset_para(PDomain_Port pPort)
{

	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pPort->Core_Extension;
	pCore->resetting_command = MV_FALSE;
	pPort->command_callback = NULL;
	pPort->timer_para = NULL;
	pPort->Hot_Plug_Timer = 0;
	pPort->find_disk = MV_FALSE;
//	pPort->reset_cmd_times= 0;
	
}
void mv_core_put_back_request(PDomain_Port pPort)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pPort->Core_Extension;
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 i,j;
	PMV_Request pReq = NULL;
	PDomain_Device pDevice=NULL;	
	/* put all the running requests back into waiting list */
	for ( i=0; i<MAX_SLOT_NUMBER; i++ )
	{
		pReq = pPort->Running_Req[i];
		if (pReq) {
			/*
			 * If this channel has multiple devices, pReq is 
			 * not the internal request of pDevice
			 */
			if ( !Core_IsInternalRequest(pCore, pReq) )
			{
				List_AddTail(&pReq->Queue_Pointer, &pCore->Waiting_List);
			}
			else 
			{
				/* Can be reset command or request sense command */
				if ( SCSI_IS_REQUEST_SENSE(pReq->Cdb[0]) )
				{
					MV_ASSERT( pReq->Org_Req!=NULL );
					if ( pReq->Org_Req )
						List_AddTail( &((PMV_Request)pReq->Org_Req)->Queue_Pointer, &pCore->Waiting_List);
				}
			}
			
			hba_remove_timer(pReq);
			pReq->eh_flag = 1;
			mv_core_reset_running_slot(pPort, i);
		}
	}
	MV_DASSERT(pPort->Running_Slot == 0);
	
	/* reset device related variables */
	for ( i=0; i<MAX_DEVICE_PER_PORT; i++ )
	{
		pDevice = &pPort->Device[i];
		
//		pDevice->Device_Type = 0;
//		pDevice->Need_Notify = MV_FALSE;
#ifdef SUPPORT_TIMER 
		if( pDevice->Timer_ID != NO_CURRENT_TIMER )
		{
			Timer_CancelRequest( pCore, pDevice->Timer_ID );
			pDevice->Timer_ID = NO_CURRENT_TIMER;
		}
#endif /* SUPPORT_TIMER */
		pDevice->Outstanding_Req = 0;
		
		/*
		 * Go through the waiting list. If there is some reset 
		 * request, remove that request. 
		 */
		mvRemoveDeviceWaitingList(pCore, pDevice->Id, MV_FALSE);
	}

	// reset the tag stack - to guarantee soft reset is issued at slot 0
	Tag_Init( &pPort->Tag_Pool, pPort->Tag_Pool.Size );

	for( i=0; i<MAX_DEVICE_PER_PORT; i++ )
	{
		if( (pPort->Device[i].Status & DEVICE_STATUS_FUNCTIONAL) && 
			(pPort->Device[i].Internal_Req != NULL) )
		{
			if (pCore->Total_Device_Count && (pPort->Port_State == PORT_STATE_INIT_DONE))
				pCore->Total_Device_Count--;
			ReleaseInternalReqToPool( pCore, pPort->Device[i].Internal_Req );
			pPort->Device[i].Internal_Req = NULL;
		}
	}
}


#define MAX_WAIT_TIMES	3
void  mv_core_reset_command(PDomain_Port pPort)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pPort->Core_Extension;
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 tmp;
	MV_U32 port_id, slot_id;
	PMV_Request pReq = NULL;
	PDomain_Port pCheckPort;

	Timer_CancelRequest(pPort, NULL);
	MV_DPRINT(("Reset HBA times=%d, Hot_Plug_Timer=%d.\n",pPort->reset_hba_times, pPort->Hot_Plug_Timer));

	/* check disk exist */
	if ((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT) & 0xf) == 0)
		return;

	pPort->Hot_Plug_Timer++;

	pCore->resetting_command = MV_TRUE;
	for(port_id=0;port_id<MAX_PORT_NUMBER;port_id++){
		pCheckPort = &pCore->Ports[port_id];
		if((pCheckPort != pPort) && pCheckPort->Running_Slot){
			if(pPort->Hot_Plug_Timer < (MAX_WAIT_TIMES -1)){
				MV_DPRINT(("Wait port[%d] running request[0x%x].\n",pCheckPort->Id, pCheckPort->Running_Slot));
				Timer_AddRequest(pPort, 1, mv_core_reset_command_in_timer, pPort, NULL);
				return;
			} else {
				MV_DPRINT(("Abort port[%d] running request[0x%x].\n",pCheckPort->Id, pCheckPort->Running_Slot));
				mv_core_put_back_request(pCheckPort);
			}
		}
	}


	if(pPort->Hot_Plug_Timer > MAX_WAIT_TIMES){
		MV_DPRINT(("ERROR: Wait too long time for command running bit, failed detecting port[%d].\n",pPort->Id));
		mv_core_init_reset_para(pPort);
		return;
	}

	pPort->reset_hba_times++;
	if(pPort->reset_hba_times > MAX_RESET_TIMES)
	{
		MV_DPRINT(("Has reset command on port [%d] more than %d.\n",pPort->Id, MAX_RESET_TIMES));
		mv_core_init_reset_para(pPort);
		return;
	}
	
	pPort->Port_State = PORT_STATE_IDLE;
	mv_core_reset_hba(pPort);
	MV_DPRINT(("Re-enable AHCI mode on port[%d].\n",pPort->Id));
	if(pPort->command_callback){
		MV_DPRINT(("reset hba callback on port[%d].\n",pPort->Id));
		pPort->command_callback( pPort);
	}
	return;
}

void mv_core_dump_reg(PDomain_Port pPort)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pPort->Core_Extension;
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_DPRINT(("Global status=0x%x.\n", MV_REG_READ_DWORD(mmio, HOST_IRQ_STAT)));
	MV_DPRINT(("Global control=0x%x.\n", MV_REG_READ_DWORD(mmio, HOST_CTL)));
	MV_DPRINT(("Port[%d] command =0x%x.\n",pPort->Id,  MV_REG_READ_DWORD( portMmio, PORT_CMD )));
	MV_DPRINT(("Port[%d] command issue =0x%x.\n",pPort->Id,  MV_REG_READ_DWORD( portMmio, PORT_CMD_ISSUE)));
	MV_DPRINT(("Port[%d] irq status=0x%x.\n",pPort->Id,  MV_REG_READ_DWORD(portMmio, PORT_IRQ_STAT)));
}
#endif	//COMMAND_ISSUE_WORKROUND

#ifdef SUPPORT_HOT_PLUG
void Device_SoftReset(PDomain_Port pPort, PDomain_Device pDevice);

void mvRemoveDeviceWaitingList( MV_PVOID This, MV_U16 deviceId, MV_BOOLEAN returnOSRequest )
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	PMV_Request pReq = NULL;
	List_Head *pPos;
	List_Head remove_List;
	MV_U8 count = 0, myCount=0, i;
	PDomain_Device pDevice;
	MV_U8 portNum = PATA_MapPortId(deviceId);
	MV_U8 deviceNum = PATA_MapDeviceId(deviceId);
	pDevice = &pCore->Ports[portNum].Device[deviceNum];

	LIST_FOR_EACH(pPos, &pCore->Waiting_List) {
		count++;
	}

	if (count!=0){
		MV_LIST_HEAD_INIT(&remove_List);
	}

	/* 
	 * If returnOSRequest is MV_FALSE, actually we just remove the 
	 * internal reset command. 
	 */
	while ( count>0 )
	{
		pReq = (PMV_Request)List_GetFirstEntry(&pCore->Waiting_List, MV_Request, Queue_Pointer);

		if ( pReq->Device_Id==deviceId )
		{
			if ( !Core_IsInternalRequest(pCore, pReq) )
			{
				if ( returnOSRequest ) {
					pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
					List_AddTail(&pReq->Queue_Pointer, &remove_List);
					myCount++;
				} else {
					List_AddTail(&pReq->Queue_Pointer, &pCore->Waiting_List);
				}
			}
			else 
			{
				/* Reset command or request sense */
				if ( SCSI_IS_REQUEST_SENSE(pReq->Cdb[0]) )
				{
					MV_ASSERT( pReq->Org_Req!=NULL );
					pReq = (PMV_Request)pReq->Org_Req;
					if ( pReq ) {
						if ( returnOSRequest ) {
							pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
							List_AddTail(&pReq->Queue_Pointer, &remove_List);
							myCount++;
						} else {
							List_AddTail(&pReq->Queue_Pointer, &pCore->Waiting_List);
						}
					}
				} else {
					/* Reset command is removed. */
				}
			}
		}
		else
		{
			List_AddTail(&pReq->Queue_Pointer, &pCore->Waiting_List);
		}
		count--;
	}//end of while

	for (i=0; i<myCount; i++){
		pReq = (PMV_Request)List_GetFirstEntry(&remove_List, MV_Request, Queue_Pointer);
		MV_DASSERT(pReq && (pReq->Scsi_Status==REQ_STATUS_NO_DEVICE));
		CompleteRequest(pCore, pReq, NULL);
	}//end of for
}

void mvRemovePortWaitingList( MV_PVOID This, MV_U8 portId )
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	PMV_Request pReq;
	List_Head *pPos;
	List_Head remove_List;
	MV_U8 count = 0, myCount=0, i;

	LIST_FOR_EACH(pPos, &pCore->Waiting_List) {
		count++;
	}

	if (count!=0){
		MV_LIST_HEAD_INIT(&remove_List);
	}

	while ( count>0 )
	{
		pReq = (PMV_Request)List_GetFirstEntry(&pCore->Waiting_List, MV_Request, Queue_Pointer);
		if ( PATA_MapPortId(pReq->Device_Id) == portId )
		{
			if ( pReq->Cmd_Initiator==pCore ) {
				if ( SCSI_IS_READ(pReq->Cdb[0]) || SCSI_IS_WRITE(pReq->Cdb[0]) ) {
					/* Command consolidate, should return */
					pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
					List_AddTail(&pReq->Queue_Pointer, &remove_List);
					myCount++;
				} else if ( SCSI_IS_REQUEST_SENSE(pReq->Cdb[0]) ) {
					/* Request sense */
					MV_ASSERT( pReq->Org_Req!=NULL );
					pReq = (PMV_Request)pReq->Org_Req;
					if ( pReq ) {
						pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
						List_AddTail(&pReq->Queue_Pointer, &remove_List);
						myCount++;
					} 
				} else {
					/* Reset command. Ignore. */
				}
			} else {
				pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
				List_AddTail(&pReq->Queue_Pointer, &remove_List);
				myCount++;
			}
		}
		else
		{
			List_AddTail(&pReq->Queue_Pointer, &pCore->Waiting_List);
		}
		count--;
	}//end of while

	for (i=0; i<myCount; i++){
		pReq = (PMV_Request)List_GetFirstEntry(&remove_List, MV_Request, Queue_Pointer);
		MV_DASSERT(pReq && (pReq->Scsi_Status==REQ_STATUS_NO_DEVICE));
		CompleteRequest(pCore, pReq, NULL);
	}//end of for

}
#ifdef HOTPLUG_ISSUE_WORKROUND
void mvHandleDeviceUnplugReset (MV_PVOID pport, MV_PVOID temp);
#endif
void mvHandleDeviceUnplug (PCore_Driver_Extension pCore, PDomain_Port pPort)
{
	PMV_Request pReq;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U8 i;
	MV_U32 temp;

	#ifdef HOTPLUG_ISSUE_WORKROUND
	PDomain_Device pDevice = &pPort->Device[0];
    	MV_U32 SControl = 0;
	#endif
#ifdef COMMAND_ISSUE_WORKROUND
	MV_DPRINT(("Check port[%d] running slot[0x%x].\n",pPort->Id, pPort->Running_Slot));
	Timer_CancelRequest(pPort, NULL);
#endif
	if( !SATA_PortDeviceDetected(pPort) )
	{
		/* clear the start bit in cmd register, 
		   stop the controller from handling anymore requests */
		temp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
		MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp & ~PORT_CMD_START);

		/* Device is gone. Return the Running_Req */
		for ( i=0; i<MAX_SLOT_NUMBER; i++ )
		{
			pReq =  pPort->Running_Req[i];
			if ( pReq !=NULL )
			{
				pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
				CompleteRequestAndSlot(pCore, pPort, pReq, NULL, i);
			}
		}

		if( pPort->Type == PORT_TYPE_PM )
		{
			pPort->Setting &= ~PORT_SETTING_PM_FUNCTIONAL;
			pPort->Setting &= ~PORT_SETTING_PM_EXISTING;
		}

		SATA_PortReportNoDevice( pCore, pPort );
		#ifdef HOTPLUG_ISSUE_WORKROUND
		if( pPort->Type != PORT_TYPE_PM ){
			
			mvDisableIntr(portMmio, pPort->old_stat);
			
			SControl = MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);
			SControl &= ~0x0000000F;
			SControl |= 0x4;    // Disable PHY
			MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
			MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);	/* flush */
			HBA_SleepMillisecond(pCore, 10);
		
			MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
			MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);	/* flush */
			HBA_SleepMillisecond(pCore, 10);
		
			pDevice->Status = DEVICE_STATUS_UNPLUG;
			MV_DPRINT(("######### Device UNPLUG on PORT irq_mask=0x%x#########\n",pPort->old_stat));
		
			Timer_AddRequest( pPort, 8, mvHandleDeviceUnplugReset, pPort, NULL);
			}
		#endif
	}
	else
	{
		MV_DPRINT(("===== Not detect that device is out =====\n"));
	}
}

void sendDummyFIS( PDomain_Port pPort )
{
	MV_U16 tag = Tag_GetOne(&pPort->Tag_Pool);
	PMV_Command_Header header = SATA_GetCommandHeader(pPort, tag);
	PMV_Command_Table pCmdTable = Port_GetCommandTable(pPort, tag);
	PSATA_FIS_REG_H2D pFIS = (PSATA_FIS_REG_H2D)pCmdTable->FIS;
#if 1//ndef _OS_LINUX
	PCore_Driver_Extension pCore = pPort->Core_Extension;
#endif
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U32 old_stat;
	MV_U32 temp=0, count=0;
	MV_DASSERT( tag == 0 );

	mvDisableIntr(portMmio, old_stat);

	MV_ZeroMemory(header, sizeof(MV_Command_Header));
	MV_ZeroMemory(pCmdTable, sizeof(MV_Command_Table));
	
	header->FIS_Length = 0;
	header->Reset = 0;
	header->PM_Port = 0xE;
	
	header->Table_Address = pPort->Cmd_Table_DMA.parts.low + SATA_CMD_TABLE_SIZE*tag;
	header->Table_Address_High = pPort->Cmd_Table_DMA.parts.high;
	
	pFIS->FIS_Type = SATA_FIS_TYPE_REG_H2D;
	pFIS->PM_Port = 0;
	pFIS->Control = 0;

	MV_REG_WRITE_DWORD(portMmio, PORT_CMD_ISSUE, 1<<tag);
	MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE);	/* flush */

	HBA_SleepMicrosecond(pCore, 10);
	//hba_msleep(10);

	// make sure CI is cleared before moving on
	do {
		temp = MV_REG_READ_DWORD(portMmio, PORT_CMD_ISSUE) & (1<<tag);
		count++;
		HBA_SleepMillisecond(pCore, 10);
		//hba_msleep(10);
	} while (temp != 0 && count < 1000);

	Tag_ReleaseOne(&pPort->Tag_Pool, tag);
	mvEnableIntr(portMmio, old_stat);

	if (temp != 0)
	{
//		MV_DPRINT(("DummyFIS:CI can not be clean[0x%x] on port[%d].\n",MV_REG_READ_DWORD( portMmio, PORT_CMD_ISSUE), pPort->Id));
	}	
}




void mvHandleDevicePlugin (PCore_Driver_Extension pCore, PDomain_Port pPort)
{
	PDomain_Device pDevice = &pPort->Device[0];
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U8 i;
	MV_U32 temp=0;
	MV_DPRINT(("Find plug in device on port[%d].\n",pPort->Id));

	if( pCore->Total_Device_Count >= MAX_DEVICE_SUPPORTED ){
		MV_DPRINT(("has many device[%d].\n", pCore->Total_Device_Count ));
		return;
	}
#ifdef COMMAND_ISSUE_WORKROUND
	pPort->reset_hba_times = 0;
	mv_core_init_reset_para(pPort);
#endif
	#ifdef HOTPLUG_ISSUE_WORKROUND
	if ( pDevice->Status == DEVICE_STATUS_UNPLUG )
	{
	  	MV_DPRINT(("######## Cancel hot plug INT #########"));
	    return;
	}
	#endif
	// start command handling on this port
	mv_core_reset_port(pPort);
	// reset the tag stack - to guarantee soft reset is issued at slot 0
	Tag_Init( &pPort->Tag_Pool, pPort->Tag_Pool.Size );

#ifdef COMMAND_ISSUE_WORKROUND
	pPort->error_state = PORT_ERROR_AT_PLUGIN;
	if(MV_REG_READ_DWORD( portMmio, PORT_CMD) & PORT_CMD_LIST_ON)
	{
		MV_PRINT("Find command running BIT is set on port[%d], reset HBA in timer handler.\n",pPort->Id);
		pPort->Hot_Plug_Timer = 0;
		pPort->command_callback = mvHandleDevicePlugin_BH;
		mv_core_reset_command(pPort);
		return;
	}
#endif
	mvHandleDevicePlugin_BH(pPort);
}

void mvHandleDevicePlugin_BH(MV_PVOID  ext)
{
	PDomain_Port pPort = (PDomain_Port)ext;
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)pPort->Core_Extension;
	PDomain_Device pDevice = &pPort->Device[0];
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U8 i;
	MV_U32 temp;
	MV_U32 tmp, old_stat;

	// do software reset
	MV_DPRINT(("Detected device plug-in, doing soft reset\n"));

	if (! (SATA_PortSoftReset( pCore, pPort )) ){
		goto start_waiting_command;
	}
	#ifdef HOTPLUG_ISSUE_WORKROUND
	if ( pDevice->Status == DEVICE_STATUS_UNPLUG )
	{
	   MV_DPRINT(("######## Cancel hot plug  BH #########"));
	    return;
	}
	#endif
	if( pPort->Type == PORT_TYPE_PM ) 
	{
		/* need to send notifications for all of these devices */
		for (i=0; i<MAX_DEVICE_PER_PORT; i++)
		{
			pDevice = &pPort->Device[i];
			pDevice->Id = (pPort->Id)*MAX_DEVICE_PER_PORT + i;
			pDevice->Need_Notify = MV_TRUE;
			pDevice->State = DEVICE_STATE_IDLE;
			pDevice->Device_Type = 0;
			pDevice->Reset_Count = 0;
		}

		/*SATA_InitPM( pPort );*/
		SATA_PortReset( pPort, MV_TRUE);
	} 
	else
	{
		/* not a PM - turn off the PM bit in command register */
		temp = MV_REG_READ_DWORD(portMmio, PORT_CMD);					
		MV_REG_WRITE_DWORD(portMmio, PORT_CMD, temp & (~MV_BIT(17)));
		temp=MV_REG_READ_DWORD(portMmio, PORT_CMD);	/* flush */

		if( SATA_PortDeviceDetected(pPort) )
		{
			if ( SATA_PortDeviceReady(pPort) )
			{
				MV_U32 signature;

				signature = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_SIG);
				if ( signature==0xEB140101 )				/* ATAPI signature */
					pDevice->Device_Type |= DEVICE_TYPE_ATAPI;
				else 
				#ifdef HOTPLUG_ISSUE_WORKROUND
					if(signature==0x00000101)
				#endif
					{
						MV_DASSERT( signature==0x00000101 );	/* ATA signature */
                    				pDevice->Device_Type &= ~DEVICE_TYPE_ATAPI;
					}
				#ifdef HOTPLUG_ISSUE_WORKROUND
					else{
						SATA_PortReportNoDevice(pCore,pPort);
						return;
						}
				#endif

				pDevice->Internal_Req = GetInternalReqFromPool(pCore);
				if( pDevice->Internal_Req == NULL )
				{
					MV_DPRINT(("ERROR: Unable to get an internal request buffer\n"));
					/* can't initialize without internal buffer - just set this disk down */
					pDevice->Status = DEVICE_STATUS_NO_DEVICE;
					pDevice->State = DEVICE_STATE_INIT_DONE;
					goto start_waiting_command;
				}
				else 
				{
					{
						pDevice->Status = DEVICE_STATUS_EXISTING|DEVICE_STATUS_FUNCTIONAL;
						pDevice->State = DEVICE_STATE_RESET_DONE;
						pDevice->Id = (pPort->Id)*MAX_DEVICE_PER_PORT;
						if(pPort->error_state == PORT_ERROR_AT_PLUGIN){
							pDevice->Need_Notify = MV_TRUE;
							pPort->Device_Number++;
						}
						pDevice->Reset_Count = 0;
					}
				}
				
				mvDeviceStateMachine (pCore, pDevice);
			}
		}
	}

start_waiting_command:
	MV_DPRINT(("Finshed mvHandleDevicePlugin_BH on port[%d].\n",pPort->Id));
	Core_HandleWaitingList(pCore);
	
}

#ifdef SUPPORT_PM
void mvHandlePMUnplug (PCore_Driver_Extension pCore, PDomain_Device pDevice)
{
	PMV_Request pReq;
	PDomain_Port pPort = pDevice->PPort;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	List_Head *pPos;
	MV_U8 i, count;
	MV_U32 temp, cmdIssue;
	MV_BOOLEAN valid;
	#ifdef RAID_DRIVER
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_RAID);	
	#else
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(pCore, MODULE_HBA);
	#endif

	pDevice->Status = DEVICE_STATUS_NO_DEVICE;
	pPort->Device_Number--;

	cmdIssue = MV_REG_READ_DWORD( portMmio, PORT_CMD_ISSUE );

	/* toggle the start bit in cmd register */
	temp = MV_REG_READ_DWORD( portMmio, PORT_CMD );
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp & ~MV_BIT(0));
	MV_REG_WRITE_DWORD( portMmio, PORT_CMD, temp | MV_BIT(0));
	HBA_SleepMillisecond( pCore, 100 );
	//hba_msleep(100);

	/* check for requests that are not finished, clear the port,
	 * then re-send again */
	for ( i=0; i<MAX_SLOT_NUMBER; i++ )
	{
		pReq = pPort->Running_Req[i];

		if( pReq != NULL )
		{
			if( pReq->Device_Id == pDevice->Id )
			{
				pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
				CompleteRequestAndSlot(pCore, pPort, pReq, NULL, i);
			}
			else if ( cmdIssue & (1<<i) )
			{
				if( PrepareAndSendCommand( pCore, pReq ) == MV_QUEUE_COMMAND_RESULT_SENT )
				{
#ifdef SUPPORT_ERROR_HANDLING
#ifdef SUPPORT_TIMER
					/* start timer for error handling */
					if( pDevice->Timer_ID == NO_CURRENT_TIMER )
					{
						// if no timer is running right now
						pDevice->Timer_ID = Timer_AddRequest( pCore, REQUEST_TIME_OUT, Core_ResetChannel, pDevice, NULL );
					}
#endif /* SUPPORT_TIMER */
#endif /* SUPPORT_ERROR_HANDLING */
					pDevice->Outstanding_Req++;
				}
				else
					MV_DASSERT(MV_FALSE);		// shouldn't happens
			}
		}
	}

	count = 0;
	LIST_FOR_EACH(pPos, &pCore->Waiting_List) {
		count++;
	}
	while ( count>0 )
	{
		pReq = (PMV_Request)List_GetFirstEntry(&pCore->Waiting_List, MV_Request, Queue_Pointer);

		if ( pReq->Device_Id == pDevice->Id )
		{
			pReq->Scsi_Status = REQ_STATUS_NO_DEVICE;
			CompleteRequest(pCore, pReq, NULL);
		}
		else
		{
			List_AddTail(&pReq->Queue_Pointer, &pCore->Waiting_List);
		}
		count--;
	}

	/* clear x bit */
	valid = MV_FALSE;
	do
	{
		mvPMDevReWrReg( pPort, MV_Read_Reg, MV_SATA_PSCR_SERROR_REG_NUM, 0, pDevice->PM_Number, MV_TRUE );
		temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);

		if (((temp >> 16) & 0xF0) == 0xF0)
			valid = MV_TRUE;
          
		temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
	} while (valid == MV_FALSE);

	mvPMDevReWrReg( pPort, MV_Write_Reg, MV_SATA_PSCR_SERROR_REG_NUM, temp, pDevice->PM_Number, MV_TRUE);

	if( pDevice->Internal_Req != NULL )
	{
		pCore->Total_Device_Count--;
		ReleaseInternalReqToPool( pCore, pDevice->Internal_Req );
		pDevice->Internal_Req = NULL;
	}

	{
		struct mod_notif_param param;
		param.lo = pDevice->Id;
#ifdef RAID_DRIVER
		RAID_ModuleNotification(pUpperLayer, EVENT_DEVICE_REMOVAL, 
					&param);
#else
		HBA_ModuleNotification(pUpperLayer, 
				       EVENT_DEVICE_REMOVAL, 
				       &param);
#endif /* RAID_DRIVER */
	}
}

extern MV_BOOLEAN mvDeviceStateMachine(
	PCore_Driver_Extension pCore,
	PDomain_Device pDevice
	);

void mvHandlePMPlugin (PCore_Driver_Extension pCore, PDomain_Device pDevice)
{
	PDomain_Port pPort = pDevice->PPort;

	if( pCore->Total_Device_Count >= MAX_DEVICE_SUPPORTED )
		return;

	pDevice->Need_Notify = MV_TRUE;
	pDevice->Device_Type = 0;
	HBA_SleepMillisecond(pCore, 1000);
	SATA_InitPMPort( pPort, pDevice->PM_Number );
	mvDeviceStateMachine(pCore, pDevice);
}
#endif	/* #ifdef SUPPORT_PM */
#endif	/* #ifdef SUPPORT_HOT_PLUG */

void sata_hotplug(MV_PVOID data,MV_U32 intStatus)
{
#ifdef SUPPORT_HOT_PLUG
	PDomain_Port pPort = (PDomain_Port)data;
	PDomain_Device pDevice = &pPort->Device[0];
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	MV_U8 i, plugout=0, plugin=0;
	MV_U32 temp;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_BOOLEAN valid;
	MV_U32 hotPlugDevice = intStatus & PORT_IRQ_PHYRDY;
	MV_U32 hotPlugPM = (intStatus & PORT_IRQ_ASYNC_NOTIF) || (intStatus & PORT_IRQ_SDB_FIS);

	intStatus &= ~(PORT_IRQ_D2H_REG_FIS|PORT_IRQ_SDB_FIS|PORT_IRQ_PIO_DONE);
#ifdef _OS_LINUX
	/*fix Thor - Linux non-raid driver - hotplug 
	Thor-Lite spec define bit25,26 as plug-out/plug-in irq status, Thor didn't define, but also will set these 
	two bits when doing hot-plug. So clear them here if they're set.
	If power is not stable, hot-plug will result to resetting port, meantime,PORT_IRQ_SIGNATURE_FIS will be set,
	so also need clear it if it is set.*/
	intStatus &= ~(PORT_IRQ_SIGNATURE_FIS|(1L<<26)|(1L<<25));
#endif
	/* if a hard drive or a PM is plugged in/out of the controller */
	if( hotPlugDevice )
	{
		intStatus &= ~PORT_IRQ_PHYRDY;
		/* use Phy status to determine if this is a plug in/plug out */
		//hba_msleep(500);
		HBA_SleepMillisecond(pCore, 500);
		if ((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT) & 0xf) == 0)
			plugout = MV_TRUE;
		else
			plugin = MV_TRUE;

		/* following are special cases, so we take care of these first */
		if( plugout )
		{
			if ( (pPort->Type != PORT_TYPE_PM ) && (pDevice->Status & DEVICE_STATUS_EXISTING) &&
			     !(pDevice->Status & DEVICE_STATUS_FUNCTIONAL) )
			{
				/* a bad drive was unplugged */
				pDevice->Status = DEVICE_STATUS_NO_DEVICE;
				MV_DPRINT(("bad drive was unplugged\n"));
			}

			if ( (pPort->Setting & PORT_SETTING_PM_EXISTING) && 
			     !(pPort->Setting & PORT_SETTING_PM_FUNCTIONAL) )
			{
				/* a bad PM was unplugged */
				pPort->Setting &= ~PORT_SETTING_PM_EXISTING;
				MV_DPRINT(("bad PM was unplugged\n"));
			}

			mvHandleDeviceUnplug( pCore, pPort );
			return;
		}
		
		if ( ((pPort->Type == PORT_TYPE_PM) && (pPort->Setting & PORT_SETTING_PM_FUNCTIONAL)) ||
		     ((pPort->Type != PORT_TYPE_PM) && (pDevice->Status & DEVICE_STATUS_FUNCTIONAL)) 
			)
		{
			if( plugout ){
				mvHandleDeviceUnplug( pCore, pPort );
				return;
			}
		}
		else
		{
			if( plugin ){
				mvHandleDevicePlugin( pCore, pPort );
				return;
			}
		}
	}
				
	/* if a drive was plugged in/out of a PM */
	if ( hotPlugPM ) 
	{
		intStatus &= ~PORT_IRQ_ASYNC_NOTIF;
		intStatus &= ~PORT_IRQ_SDB_FIS;

		valid = MV_FALSE;
		do
		{
			mvPMDevReWrReg( pPort, MV_Read_Reg, MV_SATA_GSCR_ERROR_REG_NUM, 0, 0xF, MV_TRUE );
			temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);
			if ((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT)& 0xf) != 0x3)
			{
				mvHandleDeviceUnplug( pCore, pPort );
				MV_DPRINT(("PM Lost connection 1\n"));
				return;
			}

			if (((temp >> 16) & 0xF0) == 0xF0)
				valid = MV_TRUE;
              
			temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
		} while (valid == MV_FALSE);

		if (temp == 0)
			return;

		// better solution???
		for (i=0; i<MAX_DEVICE_PER_PORT; i++)	
		{
			if( temp & MV_BIT(i) )
			{
				pDevice = &pPort->Device[i];
				pDevice->PM_Number = i;
				break;
			}
		}
		
		/* make sure it's a hot plug SDB */
		valid = MV_FALSE;
		do
		{
			mvPMDevReWrReg( pPort, MV_Read_Reg, MV_SATA_PSCR_SERROR_REG_NUM, 0, pDevice->PM_Number, MV_TRUE );
			temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);
			if ((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT)& 0xf) != 0x3)
			{
				mvHandleDeviceUnplug( pCore, pPort );
				MV_DPRINT(("PM Lost connection 2\n"));
				return;
			}

			if (((temp >> 16) & 0xF0) == 0xF0)
				valid = MV_TRUE;
              
			temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
		} while (valid == MV_FALSE);

		if( !( (temp & MV_BIT(16)) || (temp & MV_BIT(26)) ) )
			return;

		/* check phy status to determine plug in/plug out */
		HBA_SleepMillisecond(pCore, 500);
		//hba_msleep(500);

		valid = MV_FALSE;
		do
		{
			mvPMDevReWrReg(pPort, MV_Read_Reg, MV_SATA_PSCR_SSTATUS_REG_NUM, 0, pDevice->PM_Number, MV_TRUE);
			temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);
			if ((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT)& 0xf) != 0x3)
			{
				mvHandleDeviceUnplug( pCore, pPort );
				MV_DPRINT(("PM Lost connection 3\n"));
				return;
			}

			if (((temp >> 16) & 0xF0) == 0xF0)
				valid = MV_TRUE;
              
			temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
		} while (valid == MV_FALSE);

		if( (temp & 0xF) == 0 )
		{
			plugout = MV_TRUE;
			MV_DPRINT(("PM port %d plugout\n", pDevice->PM_Number));
		}
		else
		{
			if ((temp & 0xF) == 3)
			{
			plugin = MV_TRUE;
				MV_DPRINT(("PM port %d plugin\n", pDevice->PM_Number));
			}
			else
			{
				/*
				On Sil3726, we'll see this condition.
				Solution:
				1. Unplug this unstable device.
				2. Reset Channel, detect current devices to update status
				*/
				MV_DPRINT(("PM device connection not established, reset channel\n"));

				mvHandlePMUnplug(pCore, pDevice);
				Core_ResetChannel(pDevice,NULL);
				return;
			}
		}

		if ( plugout && (pDevice->Status & DEVICE_STATUS_EXISTING) &&
			 !(pDevice->Status & DEVICE_STATUS_FUNCTIONAL) )
		{
			// a bad drive was unplugged
			pDevice->Status = DEVICE_STATUS_NO_DEVICE;

			/* clear x bit */
			valid = MV_FALSE;
			do
			{
				mvPMDevReWrReg( pPort, MV_Read_Reg, MV_SATA_PSCR_SERROR_REG_NUM, 0, pDevice->PM_Number, MV_TRUE );
				temp = MV_REG_READ_DWORD( portMmio, PORT_TFDATA);
				if ((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT)& 0xf) != 0x3)
				{
//					mvHandleDeviceUnplug( pCore, pPort );
					MV_DPRINT(("PM Lost connection 5\n"));
					return;
				}

				if (((temp >> 16) & 0xF0) == 0xF0)
					valid = MV_TRUE;
	              
				temp = MV_REG_READ_DWORD( portMmio, PORT_PM_FIS_0 );
			} while (valid == MV_FALSE);

			mvPMDevReWrReg( pPort, MV_Write_Reg, MV_SATA_PSCR_SERROR_REG_NUM, temp, pDevice->PM_Number, MV_TRUE);

			MV_DPRINT(("bad drive was unplugged\n"));
			mvHandlePMUnplug(pCore, pDevice);
			return;
		}

		if( pDevice->Status & DEVICE_STATUS_FUNCTIONAL )
		{
			if (plugout)
				mvHandlePMUnplug(pCore, pDevice);
		}
		else
		{
			if (plugin)
				mvHandlePMPlugin( pCore, pDevice );
		}
	}
#endif /* SUPPORT_HOT_PLUG */
}
#ifdef HOTPLUG_ISSUE_WORKROUND
void mvHandleDeviceUnplugReset (MV_PVOID pport, MV_PVOID ptemp)
{
	
	PDomain_Port pPort = (PDomain_Port)pport;
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	PMV_Request pReq;
	MV_LPVOID portMmio = pPort->Mmio_Base;
	MV_U8 i;
	MV_U32 temp, temp2;
	PDomain_Device pDevice = &pPort->Device[0];


	MV_U32 SControl = MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);
	SControl &= ~0x0000000F;    //Enable PHY
	mvEnableIntr(portMmio, pPort->old_stat);
	MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
	MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);	/* flush */
	HBA_SleepMillisecond(pCore, 10);

	MV_REG_WRITE_DWORD(portMmio, PORT_SCR_CTL, SControl);
	MV_REG_READ_DWORD(portMmio, PORT_SCR_CTL);	/* flush */
	HBA_SleepMillisecond(pCore, 10);

    	pDevice->Status = DEVICE_STATUS_NO_DEVICE;
	MV_DPRINT(("######### ReEnable PHY&INT by Unplug Timer #########\n"));

}
#endif
void SATA_HandleHotplugInterrupt(
	IN PDomain_Port pPort,
	IN MV_U32 intStatus
	)
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
	MV_U32 hotPlugDevice = intStatus & PORT_IRQ_PHYRDY;
	MV_U32 hotPlugPM = (intStatus & PORT_IRQ_ASYNC_NOTIF) | (intStatus & PORT_IRQ_SDB_FIS);
	struct mod_notif_param event_param;

	#ifdef HOTPLUG_ISSUE_WORKROUND
	MV_LPVOID portMmio = pPort->Mmio_Base;
	PDomain_Device pDevice = &pPort->Device[0];
	MV_U32 plugout=0,plugin=0;
	#endif
	if (hotPlugDevice | hotPlugPM) {
		event_param.p_param = pPort;
		event_param.event_id = intStatus;
		#ifdef HOTPLUG_ISSUE_WORKROUND
		HBA_SleepMillisecond(pCore, 500);
		if ((MV_REG_READ_DWORD(portMmio, PORT_SCR_STAT) & 0xf) == 0)
			plugout= MV_TRUE;
		else
			plugin= MV_TRUE;
		
		/* following are special cases, so we take care of these first */
		if( plugout )
		{
			
			if ( (pPort->Type != PORT_TYPE_PM ) && (pDevice->Status & DEVICE_STATUS_EXISTING) &&
			     !(pDevice->Status & DEVICE_STATUS_FUNCTIONAL) )
			{
				/* a bad drive was unplugged */
				pDevice->Status = DEVICE_STATUS_NO_DEVICE;
				MV_DPRINT(("bad drive was unplugged\n"));
			}

			if ( (pPort->Setting & PORT_SETTING_PM_EXISTING) && 
			     !(pPort->Setting & PORT_SETTING_PM_FUNCTIONAL) )
			{
				/* a bad PM was unplugged */
				pPort->Setting &= ~PORT_SETTING_PM_EXISTING;
				MV_DPRINT(("bad PM was unplugged\n"));
			}

			mvHandleDeviceUnplug( pCore, pPort );
			return;
		}
		
		if ( ((pPort->Type == PORT_TYPE_PM) && (pPort->Setting & PORT_SETTING_PM_FUNCTIONAL)) ||
		     ((pPort->Type != PORT_TYPE_PM) && (pDevice->Status & DEVICE_STATUS_FUNCTIONAL)) 
			)
		{
			if(plugout ){
				mvHandleDeviceUnplug( pCore, pPort );
				return;
			}
		}	
		if(plugin)
		#endif
			HBA_ModuleNotification(pCore, EVENT_HOT_PLUG, &event_param);
	}

	if (intStatus) {
		MV_DPRINT(("Error: port=%d intStatus=0x%x.\n", pPort->Id, intStatus));
		/* clear global before channel */
		MV_REG_WRITE_DWORD(pCore->Mmio_Base, HOST_IRQ_STAT, (1L<<pPort->Id));
		intStatus = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_IRQ_STAT);
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_IRQ_STAT, intStatus);
	}
}

void mvCompleteSlots( PDomain_Port pPort, MV_U32 completeSlot, PATA_TaskFile taskFiles )
{
	PCore_Driver_Extension pCore = pPort->Core_Extension;
#ifdef MV_DEBUG
	MV_LPVOID port_mmio = pPort->Mmio_Base;
#endif
	PDomain_Device pDevice;
	PMV_Request pReq = NULL, pOrgReq = NULL;
	MV_U8 slotId;

	/* Complete finished commands. All of them are finished successfully.
	 * There are three situations code will come here.
	 * 1. No error for both NCQ and Non-NCQ.
	 * 2. Under NCQ, some requests are completed successfully. At lease one is not.
	 *	For the error command, by specification, SActive isn't cleared.
	 * 3. Under non-NCQ, since no interrupt coalescing, no succesful request. 
	 *  Hardware will return one request is completed. But software clears it above. */

	for ( slotId=0; slotId<MAX_SLOT_NUMBER; slotId++ )
	{
		if ( !(completeSlot&(1L<<slotId)) )
			continue;

		MV_DASSERT( (MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE)&(1<<slotId))==0 );
		MV_DASSERT( (MV_REG_READ_DWORD(port_mmio, PORT_SCR_ACT)&(1<<slotId))==0 );

		completeSlot &= ~(1L<<slotId);
				
		/* This slot is finished. */
		pReq = pPort->Running_Req[slotId];
		MV_DASSERT( pReq );
		pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];

		if ( pReq->Scsi_Status==REQ_STATUS_RETRY )
		{
			MV_PRINT("Retried request[0x%p] is finished on port[%d]\n", pReq, pPort->Id);
			MV_DumpRequest(pReq, MV_FALSE);
		}
	
		if ( Core_IsInternalRequest(pCore, pReq)&&(pReq->Org_Req) )
		{
			/* This internal request is used to request sense. */
			MV_ASSERT( pDevice->Device_Type&DEVICE_TYPE_ATAPI );
			pOrgReq = pReq->Org_Req;
			/* Copy sense from the scratch buffer to the request sense buffer. */
			MV_CopyMemory(
					pOrgReq->Sense_Info_Buffer,
					pReq->Data_Buffer,
					MV_MIN(pOrgReq->Sense_Info_Buffer_Length, pReq->Data_Transfer_Length)
					);
			pOrgReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
#if defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX)
			/* remove internal req's timer */
			hba_remove_timer(pReq);
#endif
			pReq = pOrgReq;
		}
		else
		{
			pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		}

		CompleteRequestAndSlot(pCore, pPort, pReq, taskFiles, slotId);

		if ( completeSlot==0 )
			break;
	}
}

void SATA_PortHandleInterrupt(
	IN PCore_Driver_Extension pCore,
	IN PDomain_Port pPort
	)
{
	PDomain_Device pDevice = &pPort->Device[0];
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_LPVOID port_mmio = pPort->Mmio_Base;
	MV_U32 orgIntStatus, intStatus, serialError, commandIssue, serialActive, temp;
	PMV_Request pReq = NULL, pOrgReq = NULL;
	MV_U32 completeSlot = 0;
	MV_U16 slotId;
	MV_U8 i,j;
	MV_BOOLEAN hasError = MV_FALSE, finalError = MV_FALSE,reset_port=MV_FALSE;
	MV_U32 errorSlot = 0;
	ATA_TaskFile	taskFiles;
#ifdef MV_DEBUG
	MV_U32 orgSerialError, orgCommandIssue, orgSerialActive, orgCompleteSlot, orgRunningSlot;
#endif

#ifdef SUPPORT_SCSI_PASSTHROUGH
	readTaskFiles(pPort, pDevice, &taskFiles);
#endif


	/* Read port interrupt status register */
	orgIntStatus = MV_REG_READ_DWORD(port_mmio, PORT_IRQ_STAT);
	intStatus = orgIntStatus;

	if ( pPort->Setting&PORT_SETTING_NCQ_RUNNING )
	{
		serialActive = MV_REG_READ_DWORD(port_mmio, PORT_SCR_ACT);
		completeSlot =  (~serialActive) & pPort->Running_Slot;
	}
	else
	{
		commandIssue = MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE);
		completeSlot = (~commandIssue) & pPort->Running_Slot;
	}

#ifdef MV_DEBUG
	orgCommandIssue = commandIssue;
	orgSerialActive = serialActive;
	orgCompleteSlot = completeSlot;
	orgRunningSlot = pPort->Running_Slot;
#endif

	intStatus &= ~(PORT_IRQ_D2H_REG_FIS|PORT_IRQ_SDB_FIS|PORT_IRQ_PIO_DONE);	/* Used to check request is done. */
	intStatus &= ~(PORT_IRQ_DMAS_FIS|PORT_IRQ_PIOS_FIS);						/* Needn't care. */

	/* Error handling */
	if ( 
			(intStatus&PORT_IRQ_TF_ERR)
		||	(intStatus&PORT_IRQ_LINK_RECEIVE_ERROR)
		||	(intStatus&PORT_IRQ_LINK_TRANSMIT_ERROR)
		)
	{
		MV_DPRINT(("Interrupt Error: 0x%x orgIntStatus: 0x%x completeSlot=0x%x on port[%d].\n", 
			intStatus, orgIntStatus, completeSlot, pPort->Id));
		mv_core_dump_reg( pPort);
		//if (intStatus&PORT_IRQ_TF_ERR)
		{
			/* Don't do error handling when receive link error. 
			 * Wait until we got the Task File Error */

			/* read serial error only when there is error */
			serialError = MV_REG_READ_DWORD(port_mmio, PORT_SCR_ERR);
			MV_REG_WRITE_DWORD(port_mmio, PORT_SCR_ERR, serialError);

			/* Handle serial error interrupt */
			if ( serialError )
			{
				SATA_HandleSerialError(pPort, serialError); 
			}

#ifdef MV_DEBUG
			orgSerialError = serialError;
#endif

			/* read errorSlot only when there is error */
			errorSlot = MV_REG_READ_DWORD(port_mmio, PORT_CMD);

			hasError = MV_TRUE;
			errorSlot = (errorSlot>>8)&0x1F;

			if ( pPort->Setting&PORT_SETTING_DURING_RETRY ){
				pPort->Setting &= ~PORT_SETTING_DURING_RETRY;
				MV_DPRINT(("Start reset port[%d], reset time=%d.\n",pPort->Id, pPort->reset_hba_times));
				if(pPort->reset_hba_times < MAX_RESET_TIMES)
					reset_port = MV_TRUE;
				else
					finalError = MV_TRUE;
					
			}
			else
			{
				/* if the error request is any internal requests, we don't retry 
				 *     1) read log ext - don't retry
				 *	   2) any initialization requests such as identify - buffer
				 *		  will conflict when we try to send read log ext to retry
				 *	   3) request sense - included in the ATAPI condition below
				 */
				pReq = pPort->Running_Req[errorSlot];
				//if ( pReq != NULL && Core_IsInternalRequest(pCore, pReq) )
				//	finalError = MV_TRUE;

				/* For ATAPI device, we don't do retry. OS already has done a lot.
				* ATAPI device: one request at a time. */
				if ( completeSlot==((MV_U32)1L<<errorSlot) )
				{
					pReq = pPort->Running_Req[errorSlot];
					MV_ASSERT( pReq!=NULL );
					pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];
					if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI )
						finalError = MV_TRUE;
				} else{
					MV_U32 slotId=0;
					for ( slotId=0; slotId<MAX_SLOT_NUMBER; slotId++ )
					{
						if ( !(completeSlot&(1L<<slotId)) )
							continue;	
						pReq = pPort->Running_Req[slotId];
						MV_ASSERT( pReq!=NULL );
						pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];
						if ( pDevice->Device_Type&DEVICE_TYPE_ATAPI ){
							if(!(MV_REG_READ_DWORD(mmio,PORT_TFDATA)&0x01)){
								hasError = MV_FALSE;
							}
						}else{
							printk("has error is true\n");
							}
					}	 
				}
			#ifdef SUPPORT_ATA_SECURITY_CMD
				if(intStatus&PORT_IRQ_TF_ERR){
					
					if((MV_REG_READ_DWORD(port_mmio, PORT_TFDATA)&0x451) && pReq){
												
						MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));
						MV_REG_WRITE_DWORD(port_mmio, PORT_IRQ_STAT, orgIntStatus);

						intStatus &= ~(PORT_IRQ_TF_ERR|PORT_IRQ_LINK_RECEIVE_ERROR|PORT_IRQ_LINK_TRANSMIT_ERROR);		

						pReq->Scsi_Status=REQ_STATUS_ABORT;
						CompleteRequestAndSlot(pCore, pPort, pReq, &taskFiles, (MV_U8)errorSlot);
						return;
						}
					}
			#endif
			}
		}
		intStatus &= ~(PORT_IRQ_TF_ERR|PORT_IRQ_LINK_RECEIVE_ERROR|PORT_IRQ_LINK_TRANSMIT_ERROR);		
	}

	/* If NO device ,then only handle hotplug Interrupt. */
	if((pPort->Type != PORT_TYPE_PM) && (pDevice->Status & DEVICE_STATUS_NO_DEVICE))
	{
		SATA_HandleHotplugInterrupt(pPort, intStatus);
		return;
	}

	/* Final Error: we give up this error request. Only one request is running. 
	 * And during retry we won't use NCQ command. */
	if ( finalError )
	{
		MV_DPRINT(("Final error, abort port[%d],running slot=0x%x.\n", pPort->Id,pPort->Running_Slot));
		MV_ASSERT( !(pPort->Setting&PORT_SETTING_NCQ_RUNNING) );
//		MV_DASSERT( completeSlot==((MV_U32)1L<<errorSlot) );
		MV_DASSERT( pPort->Running_Slot==completeSlot );

		/* clear global before channel */
		MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));
		MV_REG_WRITE_DWORD(port_mmio, PORT_IRQ_STAT, orgIntStatus);


		/* This is the failed request. */
		pReq = pPort->Running_Req[errorSlot];
		if((!errorSlot)&&(!pReq)&&(pPort->Setting&PORT_SETTING_DURING_RETRY)){
			for(errorSlot=0;errorSlot<32;errorSlot++){
				if((pPort->Running_Slot>>errorSlot)&0x01)
					break;
			}
			pReq = pPort->Running_Req[errorSlot];
		}
	
		MV_ASSERT( pReq!=NULL );
		pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];

		if( Core_IsInternalRequest(pCore, pReq) )
		{
			if( pReq->Org_Req )
			{
				/* This internal request is used to request sense. */
				MV_ASSERT( pDevice->Device_Type&DEVICE_TYPE_ATAPI );
				pOrgReq = pReq->Org_Req;
				pOrgReq->Scsi_Status = REQ_STATUS_ERROR;
#ifdef _OS_LINUX
				/* remove internal req's timer */
				hba_remove_timer(pReq);
#endif
				pReq = pOrgReq;
			}
			else if( pReq->Cdb[2] == CDB_CORE_READ_LOG_EXT )
			{
				pReq->Scsi_Status = REQ_STATUS_ERROR;
			}
			else
			{
				/* This internal request is initialization request like identify */
				MV_DASSERT( pDevice->State != DEVICE_STATE_INIT_DONE );
				pReq->Scsi_Status = REQ_STATUS_ERROR;
			}
		}
		else
		{
			if ( pReq->Cmd_Flag&CMD_FLAG_PACKET )
			{
				pReq->Scsi_Status = REQ_STATUS_REQUEST_SENSE;
			}
			else
			{
				MV_DPRINT(("Finally SATA error for Req 0x%x.\n", pReq->Cdb[0]));
				pReq->Scsi_Status = REQ_STATUS_ERROR;
			}
		}

		CompleteRequestAndSlot(pCore, pPort, pReq, &taskFiles, (MV_U8)errorSlot);

		/* Handle interrupt status register */
		if ( intStatus )
		{
			SATA_HandleHotplugInterrupt(pPort, intStatus);
		}

		if(!(pDevice->Device_Type&DEVICE_TYPE_ATAPI))
			SATA_PortReportNoDevice(pCore, pPort);

		return;
	}


	/* The Second time to hit the error.  */
	if ( reset_port){
		MV_DPRINT(("retry request failed, try reset HBA for port[%d], running slot=0x%x.\n", pPort->Id, pPort->Running_Slot));
//		mv_core_dump_reg(pPort);
		/* Toggle the port start bit to clear up the hardware to prepare for the retry. */
		temp = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_CMD);
		temp &= ~PORT_CMD_START;
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_CMD, temp );
		HBA_SleepMillisecond(pCore, 1);
		temp |= PORT_CMD_START;
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_CMD, temp );
		HBA_SleepMillisecond(pCore, 100);

		/* Toggle should before we clear the channel interrupt status but not the global interrupt. */
		MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));

		MV_DASSERT( MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE)==0 );
		MV_DASSERT( MV_REG_READ_DWORD(port_mmio, PORT_IRQ_STAT)==0 );
		//MV_DASSERT( orgIntStatus == 0 );
		MV_DASSERT( (MV_REG_READ_DWORD(mmio, HOST_IRQ_STAT)&(1L<<pPort->Id))==0 );
		pPort->Hot_Plug_Timer = 0;
		pPort->timer_para = pDevice;
		pPort->command_callback = Core_ResetChannel_BH;
		//Reset the device.
		pCore->Total_Device_Count--;
		pPort->error_state = PORT_ERROR_AT_RUNTIME;
		mv_core_reset_command(pPort);
		return;
	}

	/* The first time to hit the error. Under error condition, figure out all the successful requests. */
	if ( hasError )
	{
		MV_DPRINT(("Has error, reset port[%d], running slot=0x%x.\n", pPort->Id, pPort->Running_Slot));
		MV_ASSERT( !finalError );
		if ( pPort->Setting&PORT_SETTING_NCQ_RUNNING )
		{
			/* For NCQ command, if error happens on one slot.
			 * This slot is not completed. SActive is not cleared. */
		}
		else
		{
			/* For Non-NCQ command, last command is the error command. 
			 * ASIC will stop whenever there is an error.
			 * And we only have one request if there is no interrupt coalescing or NCQ. */
			//MV_DASSERT( completeSlot==((MV_U32)1L<<errorSlot) );

			/* The error command is finished but we clear it to make it to be retried. */
			completeSlot=0;
		}
		/* Now all the completed commands are completed successfully. */

		/* Reset this port to prepare for the retry. At least one request will be retried. */

		MV_ASSERT( finalError==MV_FALSE );

		/* Toggle the port start bit to clear up the hardware to prepare for the retry. */
		temp = MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_CMD);
		temp &= ~PORT_CMD_START;
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_CMD, temp );
		HBA_SleepMillisecond(pCore, 1);
		temp |= PORT_CMD_START;
		MV_REG_WRITE_DWORD(pPort->Mmio_Base, PORT_CMD, temp );
		HBA_SleepMillisecond(pCore, 1);
		/* Toggle should before we clear the channel interrupt status but not the global interrupt. */
		MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));
				
		if(pPort->Type==PORT_TYPE_SATA){
			//MV_DPRINT(("***pPort->Running_Slot=0x%x pDevice.Outstanding_Req=%d***\n", pPort->Running_Slot, pPort->Device[0].Outstanding_Req));
		}

		/* Abort all the others requests and retry. */
		for ( slotId=0; slotId<MAX_SLOT_NUMBER; slotId++ )
		{
			pReq = pPort->Running_Req[slotId];
			if ( !(completeSlot&(1L<<slotId)) && pReq )
			{
				pReq->Cmd_Flag &= 0xFF;	/* Remove NCQ setting. */
				pReq->Scsi_Status = REQ_STATUS_RETRY;

				/* Put requests to the queue head but don't run them. Should run ReadLogExt first. */
				mv_core_reset_running_slot(pPort, slotId);
#ifdef _OS_LINUX
				pReq->eh_flag = 1;
				hba_remove_timer(pReq);
#endif
				List_Add(&pReq->Queue_Pointer, &pCore->Waiting_List);		/* Add to the header. */

				if (pPort->Type==PORT_TYPE_SATA){
					pPort->Device[0].Outstanding_Req--;
				}else {//PM case					
					for (j=0; j<MAX_DEVICE_PER_PORT; j++){
						if (pPort->Device[j].Id == pReq->Device_Id){
							pPort->Device[j].Outstanding_Req--;
							break;
						}
					}//end of for
					MV_DASSERT(j==MAX_DEVICE_PER_PORT);					
				}

				//MV_PRINT("Abort error requests....\n");
				//MV_DumpRequest(pReq, MV_FALSE);
			}
		}

#ifdef SUPPORT_TIMER 
		if(pPort->Type==PORT_TYPE_SATA)
		{
			MV_DASSERT(pPort->Running_Slot==0);
			MV_DASSERT(pPort->Device[0].Outstanding_Req==0);

			if( pPort->Device[0].Timer_ID != NO_CURRENT_TIMER )
			{
				Timer_CancelRequest( pCore, pPort->Device[0].Timer_ID );
				pPort->Device[0].Timer_ID = NO_CURRENT_TIMER;
				
			}		
		}else {//PM case
			MV_DASSERT(pPort->Type==PORT_TYPE_PM);
			MV_DASSERT(pPort->Running_Slot==0);
			for (j=0; j<MAX_DEVICE_PER_PORT; j++){					
				MV_DASSERT(pPort->Device[j].Outstanding_Req==0);
				if( pPort->Device[j].Timer_ID != NO_CURRENT_TIMER )
				{
					Timer_CancelRequest( pCore, pPort->Device[j].Timer_ID );
					pPort->Device[j].Timer_ID = NO_CURRENT_TIMER;
				
				}	
			}
		}
#endif /* SUPPORT_TIMER */

		MV_DASSERT( MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE)==0 );
		MV_DASSERT( MV_REG_READ_DWORD(port_mmio, PORT_IRQ_STAT)==0 );
		//MV_DASSERT( orgIntStatus == 0 );
		MV_DASSERT( (MV_REG_READ_DWORD(mmio, HOST_IRQ_STAT)&(1L<<pPort->Id))==0 );

		/* Send ReadLogExt command to clear the outstanding commands on the device. 
		 * This request will be put to the queue head because it's Cmd_Initiator is Core Driver. 
		 * Consider the port multiplier. */
		for ( i=0; i<MAX_DEVICE_PER_PORT; i++ )
		{
			pDevice = &pPort->Device[i];
			if ( 
				!(pDevice->Device_Type&DEVICE_TYPE_ATAPI)
				&& (pDevice->Capacity&DEVICE_CAPACITY_READLOGEXT_SUPPORTED)
				&& (pPort->Setting&PORT_SETTING_NCQ_RUNNING)
				)
			{
				if(pDevice->Internal_Req){
					Device_IssueReadLogExt(pPort, pDevice);
				} else {
					Core_ResetChannel(pDevice,NULL);
					return;
				}
			}
			else
			{
				Core_HandleWaitingList(pCore);	
			}
		}

		/* Needn't run interrupt_handle_bottom_half except the hot plug.
		 * Toggle start bit will clear all the interrupt. So don't clear interrupt again. 
		 * Otherwise it'll clear Read Log Ext interrupt. 
		 * If Device_IssueReadLogExt is called, needn't run Core_HandleWaitingList. */
		
	}


	/* clear global before channel */
	MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));
	MV_REG_WRITE_DWORD(port_mmio, PORT_IRQ_STAT, orgIntStatus);

	/* handle completed slots */
	if( completeSlot )
		mvCompleteSlots( pPort, completeSlot, &taskFiles );

	/* Handle interrupt status register */
	if ( intStatus )
	{
		SATA_HandleHotplugInterrupt(pPort, intStatus);
	}
}

void PATA_PortHandleInterrupt(
	IN PCore_Driver_Extension pCore,
	IN PDomain_Port pPort
	)
{
	MV_LPVOID mmio = pCore->Mmio_Base;
	MV_LPVOID port_mmio = pPort->Mmio_Base;
	MV_U32 intStatus, orgIntStatus, commandIssue, taskFile=0, stateMachine, portCommand;
	MV_U32 temp;
	PMV_Request pReq = NULL, pOrgReq = NULL;
	MV_U32 completeSlot = 0;
	MV_U16 slotId = 0;
	MV_BOOLEAN hasOneAlready = MV_FALSE;
	MV_BOOLEAN hasError = MV_FALSE, needReset = MV_FALSE;
	PDomain_Device pDevice=NULL;
	ATA_TaskFile	taskFiles;
    	
	/* Read port interrupt status register */
	intStatus = MV_REG_READ_DWORD(port_mmio, PORT_IRQ_STAT);
	orgIntStatus = intStatus;

	/* 
	 * Workaround for PATA non-data command.
	 * PATA non-data command, CI is not ready yet when interrupt is triggered.
	 */
	commandIssue = MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE);
	completeSlot = (~commandIssue) & pPort->Running_Slot;

/* Thor Lite D0 and Thor B0 */
//if ( (pCore->Device_Id!=DEVICE_ID_THOR_4S1P_NEW) && (pCore->Revision_Id!=0xB0) && (pCore->Revision_Id!=0xB1) && (pCore->Revision_Id!=0xB2) )
if ( (pCore->Device_Id!=DEVICE_ID_THOR_4S1P_NEW) && (pCore->Revision_Id<0xB0) )
{
	temp=1000;
	while ( (completeSlot==0) && (temp>0) )
	{
		HBA_SleepMillisecond(pCore, 2);
		commandIssue = MV_REG_READ_DWORD(port_mmio, PORT_CMD_ISSUE);
		completeSlot = (~commandIssue) & pPort->Running_Slot;
		temp--;
	}

	if ( (completeSlot==0)&&(pPort->Running_Slot!=0) )
	{
		MV_DPRINT(("INT but no request completed: 0x%x CI: 0x%x Running: 0x%x\n", 
			intStatus, commandIssue, pPort->Running_Slot));
		/*
		 * Workaround:
		 * If ATAPI read abort happens, got one interrupt but CI is not cleared.
		 */
		stateMachine = MV_REG_READ_DWORD(port_mmio, PORT_INTERNAL_STATE_MACHINE);
		if ( stateMachine==0x60007013 )
		{
            		pCore->Need_Reset = 1;
			needReset = MV_TRUE;

			/* Actually one request is finished. We need figure out which one it is. */
			portCommand = MV_REG_READ_DWORD(port_mmio, PORT_CMD);
			MV_DASSERT( portCommand&MV_BIT(15) );	/* Command is still running */
			portCommand = (portCommand>>8)&0x1F;
			MV_ASSERT( portCommand<MAX_SLOT_NUMBER );
			MV_DPRINT(("Read abort happens on slot %d.\n", portCommand));
			completeSlot |= (1<<portCommand);
		}
	}
}

	/* Handle interrupt status register */
	intStatus &= ~MV_BIT(0); intStatus &= ~MV_BIT(2);
#ifdef ENABLE_PATA_ERROR_INTERRUPT
	hasError = (intStatus!=0) ? MV_TRUE : MV_FALSE;

	/*
	 * Workaround:
	 * If error interrupt bit is set. We cannot clear it.
	 * Try to use PORT_CMD PORT_CMD_PATA_START bit to clear the error interrupt but didn't work.
	 * So we have to disable PATA error interrupt.
	 */
#endif

	/* Complete finished commands */
	for ( slotId=0; slotId<MAX_SLOT_NUMBER; slotId++ )
	{

		if ( !(completeSlot&(1L<<slotId)) )
			continue;

		completeSlot &= ~(1L<<slotId);
		MV_DASSERT( completeSlot==0 );	

		/* This slot is finished. */
		pReq = pPort->Running_Req[slotId];
		MV_DASSERT(pReq);

#if defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX)
		hba_remove_timer(pReq);
#endif /* defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX) */
		mv_core_reset_running_slot(pPort, slotId);
		MV_DASSERT( (MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_CMD_ISSUE)&(1<<slotId))==0 );

		pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];

	#ifndef ENABLE_PATA_ERROR_INTERRUPT
		/* 
 		 * Workaround:
 		 * Sometimes we got error interrupt bit but the status is still 0x50.
		 * In this case, the command is completed without error.
		 * So we have to check the task status to make sure it's really an error or not.
		 */
		HBA_SleepMicrosecond(pCore, 2);
		if ( !pDevice->Is_Slave )
			taskFile = MV_REG_READ_DWORD(port_mmio, PORT_MASTER_TF0);
		else
			taskFile = MV_REG_READ_DWORD(port_mmio, PORT_SLAVE_TF0);


		if ( taskFile&MV_BIT(0) )
		{
			hasError = MV_TRUE;
			MV_DPRINT(("PATA request returns with error 0x%x.\n", taskFile));
		}

		#ifdef MV_DEBUG
		if ( !(taskFile&MV_BIT(0)) && ( intStatus ) )
		{
			MV_DPRINT(("Error interrupt is set but status is 0x50.\n"));
		
		}
		#endif
	#endif

		//if ( (hasError)&&(pCore->Device_Id!=DEVICE_ID_THOR_4S1P_NEW)&&(pCore->Revision_Id!=0xB0)&&(pCore->Revision_Id!=0xB1)&&(pCore->Revision_Id!=0xB2) )
		if ( (hasError)&&(pCore->Device_Id!=DEVICE_ID_THOR_4S1P_NEW)&&(pCore->Revision_Id<0xB0) )
		{

			if ( pDevice->Device_Type==DEVICE_TYPE_ATAPI )
			{
				/*
				 * Workaround: 
				 * Write request if device abort, hardware state machine got wrong.
				 * Need do reset to recover.
				 * If the error register is 0x40, we think the error happens.
			 	 * Suppose this problem only happens on ODD. HDD won't write abort.
 				 */

				taskFile = taskFile>>24;  /* Get the error register */
				if ( taskFile==0x40 )	
				{
					pCore->Need_Reset = 1;
					needReset = MV_TRUE;
				}
			}
		}
		if ( Core_IsInternalRequest(pCore, pReq)&&(pReq->Org_Req) )
		{
			/* This internal request is used to request sense. */
			pOrgReq = pReq->Org_Req;
			if ( hasError )
			{
				MV_ASSERT( hasOneAlready==MV_FALSE );
				hasOneAlready = MV_TRUE;
				pOrgReq->Scsi_Status = REQ_STATUS_ERROR;
			}
			else
			{
				/* Copy sense from the scratch buffer to the request sense buffer. */
				MV_CopyMemory(
						pOrgReq->Sense_Info_Buffer,
						pReq->Data_Buffer,
						MV_MIN(pOrgReq->Sense_Info_Buffer_Length, pReq->Data_Transfer_Length)
						);
				pOrgReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
			}
			pReq = pOrgReq;
		}
		else
	
		{
			if ( hasError )
			{

				MV_ASSERT( hasOneAlready==MV_FALSE );
				hasOneAlready = MV_TRUE;

				if ( needReset )
				{
					/* Get sense data using legacy mode or fake a sense data here. */
					PATA_LegacyPollSenseData(pCore, pReq);
					pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
				}
				else
				{
					if ( pReq->Cmd_Flag&CMD_FLAG_PACKET )
						pReq->Scsi_Status = REQ_STATUS_REQUEST_SENSE;
					else
						pReq->Scsi_Status = REQ_STATUS_ERROR;
				}
			}
			else
			{
				pReq->Scsi_Status = REQ_STATUS_SUCCESS;

			}
		}

#ifdef SUPPORT_SCSI_PASSTHROUGH
		readTaskFiles(pPort, pDevice, &taskFiles);
#endif

		pDevice->Outstanding_Req--;
#ifdef SUPPORT_ERROR_HANDLING
#ifdef SUPPORT_TIMER
		/* request for this device came back, so we cancel the timer */
		Timer_CancelRequest( pCore, pDevice->Timer_ID );
		pDevice->Timer_ID = NO_CURRENT_TIMER;

		/* if there are more outstanding requests, we send a new timer */
		if ( pDevice->Outstanding_Req > 0 )
		{
			pDevice->Timer_ID = Timer_AddRequest( pCore, REQUEST_TIME_OUT, Core_ResetChannel, pDevice, NULL );
		}
#endif /* SUPPORT_TIMER */
#endif /* SUPPORT_ERROR_HANDLING */

		CompleteRequest(pCore, pReq, &taskFiles);  

		if ( completeSlot==0 )
			break;
	}

	/* 
	 * Clear the interrupt. It'll re-start the hardware to handle the next slot. 
	 * I clear the interrupt after I've checked the CI register.
	 * Currently we handle one request everytime in case if there is an error I don't know which one it is.
	 */
	MV_REG_WRITE_DWORD(mmio, HOST_IRQ_STAT, (1L<<pPort->Id));
	MV_REG_WRITE_DWORD(port_mmio, PORT_IRQ_STAT, orgIntStatus);

	
	/* If there is more requests on the slot, we have to push back there request. */
	if ( needReset )
	{
		for ( slotId=0; slotId<MAX_SLOT_NUMBER; slotId++ )
		{
			pReq = pPort->Running_Req[slotId];
			if ( pReq )
			{
				List_Add(&pReq->Queue_Pointer, &pCore->Waiting_List);
				mv_core_reset_running_slot(pPort, slotId);
			}
		}
		MV_DASSERT(pPort->Running_Slot == 0);
	}
}

void Device_MakeRequestSenseRequest(
	IN PCore_Driver_Extension pCore,
	IN PDomain_Device pDevice,
	IN PMV_Request pNewReq,
	IN PMV_Request pOrgReq
	)
{
	PMV_SG_Table pSGTable = &pNewReq->SG_Table;
	//MV_U8 senseSize = SATA_SCRATCH_BUFFER_SIZE;
	MV_U8 senseSize = 18;
	
	MV_ZeroMvRequest(pNewReq);

	pNewReq->Device_Id = pDevice->Id;

	pNewReq->Scsi_Status = REQ_STATUS_PENDING;
	pNewReq->Cmd_Initiator = pCore;

	pNewReq->Data_Transfer_Length = senseSize;
	pNewReq->Data_Buffer = pDevice->Scratch_Buffer;

	pNewReq->Org_Req = pOrgReq;

	pNewReq->Cmd_Flag = CMD_FLAG_DATA_IN;
#ifdef USE_DMA_FOR_ALL_PACKET_COMMAND	
	pNewReq->Cmd_Flag |=CMD_FLAG_DMA;
#endif

	pNewReq->Completion = NULL;

	/* Make the SG table. */
	SGTable_Init(pSGTable, 0);
	SGTable_Append(
		pSGTable, 
		pDevice->Scratch_Buffer_DMA.parts.low,
		pDevice->Scratch_Buffer_DMA.parts.high,
		senseSize
		);
	MV_DASSERT( senseSize%2==0 );

	/* Request Sense request */
	pNewReq->Cdb[0]=SCSI_CMD_REQUEST_SENSE;
	pNewReq->Cdb[4]=senseSize;

	/* Fixed sense data format is 18 bytes. */
	MV_ZeroMemory(pNewReq->Data_Buffer, senseSize);
}

void CompleteRequest(
	IN PCore_Driver_Extension pCore,
	IN PMV_Request pReq,
	IN PATA_TaskFile pTaskFile
	)
{
#ifdef SUPPORT_SCSI_PASSTHROUGH
	PHD_Status pHDStatus;
#endif
	PDomain_Port pPort = &pCore->Ports[PATA_MapPortId(pReq->Device_Id)];
	PDomain_Device pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];

	//Some of the command, we need read the received FIS like smart command.

	if(pReq->Splited_Count)
	{
		MV_DASSERT( pReq->Cdb[0]==SCSI_CMD_VERIFY_10 );
		if(pReq->Scsi_Status == REQ_STATUS_SUCCESS)
		{
			MV_U32 sectors;
			MV_LBA lba;
			
			pReq->Splited_Count--;

			lba.value = SCSI_CDB10_GET_LBA(pReq->Cdb) + MV_MAX_TRANSFER_SECTOR;
			sectors = MV_MAX_TRANSFER_SECTOR;
			SCSI_CDB10_SET_LBA(pReq->Cdb, lba.value);
			SCSI_CDB10_SET_SECTOR(pReq->Cdb, sectors);

			pReq->Scsi_Status = REQ_STATUS_PENDING;

			Core_ModuleSendRequest(pCore, pReq);

			return;
		}
		else
			pReq->Splited_Count = 0;
	}

#if defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX)
	hba_remove_timer(pReq);
#endif /* defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX) */	

	if ( pReq->Scsi_Status==REQ_STATUS_REQUEST_SENSE )
	{
		/* Use the internal request to request sense. */
		Device_MakeRequestSenseRequest(pCore, pDevice, pDevice->Internal_Req, pReq);
		/* pReq is linked to the */
		Core_ModuleSendRequest(pCore, pDevice->Internal_Req);

		return;
	}


#ifdef SUPPORT_SCSI_PASSTHROUGH
	if (pTaskFile != NULL)
	{
		if (pReq->Scsi_Status == REQ_STATUS_SUCCESS)
		{
			if (pReq->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC && pReq->Cdb[1] == CDB_CORE_MODULE)
			{
				if (pReq->Cdb[2] == CDB_CORE_DISABLE_WRITE_CACHE)
					pDevice->Setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
				else if (pReq->Cdb[2] == CDB_CORE_ENABLE_WRITE_CACHE)
					pDevice->Setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
				else if (pReq->Cdb[2] == CDB_CORE_DISABLE_SMART)
					pDevice->Setting &= ~DEVICE_SETTING_SMART_ENABLED;
				else if (pReq->Cdb[2] == CDB_CORE_ENABLE_SMART)
					pDevice->Setting |= DEVICE_SETTING_SMART_ENABLED;
				else if (pReq->Cdb[2] == CDB_CORE_SMART_RETURN_STATUS)
				{
					pHDStatus = (PHD_Status)pReq->Data_Buffer;
					if (pHDStatus == NULL)
					{
#ifdef SUPPORT_EVENT
					if (pTaskFile->LBA_Mid == 0xF4 && pTaskFile->LBA_High == 0x2C)
						core_generate_event( pCore, EVT_ID_HD_SMART_THRESHOLD_OVER, pDevice->Id, SEVERITY_WARNING, 0, NULL );
#endif
					}
					else
					{
						if (pTaskFile->LBA_Mid == 0xF4 && pTaskFile->LBA_High == 0x2C)
							pHDStatus->SmartThresholdExceeded = MV_TRUE;
						else
							pHDStatus->SmartThresholdExceeded = MV_FALSE;
					}
				}
			}


			if (pReq->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC && pReq->Cdb[1] == CDB_CORE_MODULE
			    && pReq->Cdb[2] == CDB_CORE_OS_SMART_CMD)
			{
				pReq->Cdb[6] = pTaskFile->LBA_Mid;
				pReq->Cdb[7] = pTaskFile->LBA_High;
				pReq->Cdb[5] = pTaskFile->LBA_Low;
				pReq->Cdb[9] = pTaskFile->Device;
				pReq->Cdb[8] = pTaskFile->Sector_Count;
			}

		}
		else
		{
			if (pReq->Sense_Info_Buffer != NULL)
				((MV_PU8)pReq->Sense_Info_Buffer)[0] = REQ_STATUS_ERROR;
			pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
#ifdef SUPPORT_EVENT
			if ( (pReq->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
				 (pReq->Cdb[1] == CDB_CORE_MODULE) &&
				 (pReq->Cdb[2] == CDB_CORE_SMART_RETURN_STATUS) )
				core_generate_event(pCore, EVT_ID_HD_SMART_POLLING_FAIL, pDevice->Id, SEVERITY_WARNING,  0,  NULL );
#endif
		}
	}
#endif

	/* Do something if necessary to return back the request. */
	if ( (pReq->Cdb[0]==SCSI_CMD_MARVELL_SPECIFIC) && (pReq->Cdb[1]==CDB_CORE_MODULE) ) 
	{
		if ( pReq->Cdb[2]==CDB_CORE_SHUTDOWN )
		{
			if ( pReq->Device_Id<MAX_DEVICE_NUMBER-1 )
			{
				pReq->Device_Id++;
				pReq->Scsi_Status = REQ_STATUS_PENDING;
				Core_ModuleSendRequest(pCore, pReq);
				return;
			}
			else
			{
				pReq->Scsi_Status = REQ_STATUS_SUCCESS;
			}
		}
	}
	if(pReq->Completion)
		pReq->Completion(pReq->Cmd_Initiator, pReq);
}

void CompleteRequestAndSlot(
	IN PCore_Driver_Extension pCore,
	IN PDomain_Port pPort,
	IN PMV_Request pReq,
	IN PATA_TaskFile pTaskFile,
	IN MV_U8 slotId
	)
{
#ifdef SUPPORT_ERROR_HANDLING
	PDomain_Device pDevice = &pPort->Device[PATA_MapDeviceId(pReq->Device_Id)];
#endif
	mv_core_reset_running_slot(pPort, slotId);
	MV_DASSERT( (MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_CMD_ISSUE)&(1<<slotId))==0 );

	if ( pPort->Type!=PORT_TYPE_PATA )
	{
		MV_DASSERT( (MV_REG_READ_DWORD(pPort->Mmio_Base, PORT_SCR_ACT)&(1<<slotId))==0 );
	}

	pDevice->Outstanding_Req--;
#ifdef SUPPORT_ERROR_HANDLING
#ifdef SUPPORT_TIMER
	/* request for this device came back, so we cancel the timer */
	Timer_CancelRequest( pCore, pDevice->Timer_ID );
	pDevice->Timer_ID = NO_CURRENT_TIMER;

	/* if there are more outstanding requests, we send a new timer */
	if ( pDevice->Outstanding_Req > 0 )
	{
		MV_DASSERT(pPort->Running_Slot!=0);
		pDevice->Timer_ID = Timer_AddRequest( pCore, REQUEST_TIME_OUT, Core_ResetChannel, pDevice, NULL );
	}
#endif /* SUPPORT_TIMER */
#endif /* SUPPORT_ERROR_HANDLING */
	CompleteRequest(pCore, pReq, pTaskFile);
}

void Port_Monitor(PDomain_Port pPort)
{
	MV_U8 i;
	MV_PRINT("Port_Monitor: Running_Slot=0x%x.\n", pPort->Running_Slot);

	for ( i=0; i<MAX_SLOT_NUMBER; i++ )
	{
		if ( pPort->Running_Req[i]!=NULL )
			MV_DumpRequest(pPort->Running_Req[i], MV_FALSE);
	}
}

void Core_ModuleMonitor(MV_PVOID This)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	PMV_Request pReq = NULL;
	PList_Head head = &pCore->Waiting_List;
	PDomain_Port pPort = NULL;
	MV_U8 i;

	MV_PRINT("Core_ModuleMonitor Waiting_List:\n");
	for (pReq = LIST_ENTRY((head)->next, MV_Request, Queue_Pointer);	\
	     &pReq->Queue_Pointer != (head); 	\
	     pReq = LIST_ENTRY(pReq->Queue_Pointer.next, MV_Request, Queue_Pointer))
	{
		MV_DumpRequest(pReq, MV_FALSE);
	}

	for ( i=0; i<pCore->Port_Num; i++ )
	{
		MV_PRINT("Port[%d]:\n", i);
		pPort = &pCore->Ports[i];
		Port_Monitor(pPort);
	}
}

void Core_ModuleReset(MV_PVOID This)
{
	MV_U32 extensionSize = 0; 

	extensionSize = ( ROUNDING(sizeof(Core_Driver_Extension),8)
#ifdef SUPPORT_CONSOLIDATE
					+ ROUNDING(sizeof(Consolidate_Extension),8) + ROUNDING(sizeof(Consolidate_Device),8)*MAX_DEVICE_NUMBER
#endif
					);
			
	/* Re-initialize all the variables even discard all the requests. */
	Core_ModuleInitialize(This, extensionSize, 32);

}
#ifdef __MM_SE__

struct mv_module_ops __core_mod_ops = {
	.module_id              = MODULE_CORE,
	.get_res_desc           = Core_ModuleGetResourceQuota,
	.module_initialize      = Core_ModuleInitialize,
	.module_start           = Core_ModuleStart,
	.module_stop            =  Core_ModuleShutdown,
	.module_notification    = Core_ModuleNotification,
	.module_sendrequest     = Core_ModuleSendRequest,
	.module_reset           =  Core_ModuleReset,
	.module_monitor         = Core_ModuleMonitor,
	.module_service_isr     = Core_InterruptServiceRoutine,
#ifdef RAID_DRIVER
	.module_send_xor_request= Core_ModuleSendXORRequest,
#endif /* RAID_DRIVER */
};

struct mv_module_ops *mv_core_register_module(void)
{
	return &__core_mod_ops;
}

#endif /* _OS_LINUX */

