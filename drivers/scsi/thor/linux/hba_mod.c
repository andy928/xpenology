/*
 * module management module - module of the modules
 *
 *
 */
#define IOCTL_TEMPLATE
#ifdef IOCTL_TEMPLATE
#define SECTOR_SIZE 512
#include <linux/hdreg.h>

#endif
#include "hba_header.h"

#include "linux_main.h"
#include "linux_iface.h"

#include "hba_timer.h"
#include "hba_mod.h"

#include "res_mgmt.h"

#include "hba_exp.h"
#include <scsi/scsi_tcq.h>
#include "core_inter.h"
#include "core_sat.h"
#include "core_ata.h"
#include <scsi/scsi_eh.h>

#if 0
static void __dump_hex(const unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (i && !(i % 8))
			printk(YELLOW(" :"));
		if (i && !(i % 16))
			printk("\n");
		printk(" %02X", buf[i]);
	}
	printk("\n");
}
#endif

void __hba_dump_req_info(MV_U32 id, PMV_Request preq)
{
#if 0
	unsigned long lba =0;
	char *buf;
	struct scsi_cmnd *scmd = NULL;
	struct scatterlist *sg = NULL;

	switch (preq->Cdb[0]) {
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
		lba = preq->Cdb[2]<<24 | preq->Cdb[3]<<16 | preq->Cdb[4]<<8 | \
			preq->Cdb[5];
		break;
	case SCSI_CMD_INQUIRY:
		if (REQ_STATUS_SUCCESS != preq->Scsi_Status)
			break;

		scmd = (struct scsi_cmnd *) preq->Org_Req;
		sg   = (struct scatterlist *) mv_rq_bf(scmd);
		
		/* ignored  */
		break;

		buf = map_sg_page(sg) + sg->offset;
		MV_DBG(DMSG_SCSI, BLUE("-- inquiry dump starts -- ")"\n");
		__dump_hex((unsigned char *)buf, mv_rq_bf_l(scmd));
		MV_DBG(DMSG_SCSI, "-- inquiry dump ends -- \n");
		kunmap_atomic(buf, KM_IRQ0);
		break;
	default:
		lba = 0;
		break;
	} 
#if 0
	MV_DBG(DMSG_FREQ,
	       "(%u) req "RED("%p")
	       " dev %d : cmd %2X : lba %lu - %lu : length %d.\n",
	       id, preq, preq->Device_Id, preq->Cdb[0], lba,
	       lba + preq->Data_Transfer_Length/512,
	       preq->Data_Transfer_Length);
#endif
#endif

}

static void generate_sg_table(struct hba_extension *phba,
			      struct scsi_cmnd *scmd,
			      PMV_SG_Table sg_table)
{
	struct scatterlist *sg;
	unsigned int sg_count = 0;
	unsigned int length;
	BUS_ADDRESS busaddr = 0;
	int i;

	//MV_DBG(DMSG_FREQ,"%s : in %s.\n", mv_product_name, __FUNCTION__);

	if (mv_rq_bf_l(scmd) > (mv_scmd_host(scmd)->max_sectors << 9)) {
		MV_DBG(DMSG_SCSI, "ERROR: request length exceeds "
		       "the maximum alowed value.\n");
	}
	
	if (0 == mv_rq_bf_l(scmd))
		return ;

	if (mv_use_sg(scmd)) {
		MV_DBG(DMSG_SCSI_FREQ, "%s : map %d sg entries.\n",
		       mv_product_name, mv_use_sg(scmd));


		sg = (struct scatterlist *) mv_rq_bf(scmd);
		if (MV_SCp(scmd)->mapped == 0){
			#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
			sg_count = pci_map_sg(phba->dev,
					      sg,
					      mv_use_sg(scmd),
						  scsi_to_pci_dma_dir(scmd->sc_data_direction));
			#else
				sg_count = scsi_dma_map(scmd);
			#endif
			if (sg_count != mv_use_sg(scmd)) {
				MV_PRINT("WARNING sg_count(%d) != scmd->use_sg(%d)\n",
					 (unsigned int) sg_count, mv_use_sg(scmd));
			}
			MV_SCp(scmd)->mapped = 1;
		}
		for (i = 0; i < sg_count; i++) {
			busaddr = sg_dma_address(&sg[i]);
			length = sg_dma_len(&sg[i]);
	#ifdef MV_DEBUG
			if(length > 4096)
				MV_DPRINT(("Warning: sg[%i] length %d > 4096\n", i, length));
	#endif
			sgdt_append_pctx(sg_table,
					 LO_BUSADDR(busaddr), 
					 HI_BUSADDR(busaddr),
					 length,
					 sg + i);
		}
	} else {
		MV_DBG(DMSG_SCSI_FREQ, "map kernel addr into bus addr.\n" );
		if (MV_SCp(scmd)->mapped == 0) {
			busaddr = dma_map_single(&phba->dev->dev,
				      mv_rq_bf(scmd),
				       mv_rq_bf_l(scmd),
					   scsi_to_pci_dma_dir(scmd->sc_data_direction));
			MV_SCp(scmd)->bus_address = busaddr;
			MV_SCp(scmd)->mapped = 1;

		}
	#ifdef MV_DEBUG
			if(mv_rq_bf_l(scmd) > 4096)
				MV_DPRINT(("Warning: single sg request_bufflen %d > 4096\n", mv_rq_bf_l(scmd)));
	#endif

		sgdt_append_vp(sg_table, 
			       mv_rq_bf(scmd),
			       mv_rq_bf_l(scmd),
			       LO_BUSADDR(busaddr), 
			       HI_BUSADDR(busaddr));
		
	}
}

void mv_complete_request(struct hba_extension *phba,
				struct scsi_cmnd *scmd,
				PMV_Request pReq)
{
	PMV_Sense_Data  senseBuffer = (PMV_Sense_Data) scmd->sense_buffer;
	
	//MV_POKE();
	__hba_dump_req_info(phba->desc->module_id, pReq);

	if (mv_rq_bf_l(scmd)) {
		MV_DBG(DMSG_SCSI_FREQ, "unmap %d bytes.\n", 
		        mv_rq_bf_l(scmd));

	if (MV_SCp(scmd)->mapped) {
			if (mv_use_sg(scmd)) {
				#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
				pci_unmap_sg(phba->dev,
					     mv_rq_bf(scmd),
					     mv_use_sg(scmd),
					 	scsi_to_pci_dma_dir(scmd->sc_data_direction));
				#else
				scsi_dma_unmap(scmd);
				#endif
			} else {
				dma_unmap_single(&(phba->dev->dev),
						 MV_SCp(scmd)->bus_address,
						 mv_rq_bf_l(scmd),
				     	scsi_to_pci_dma_dir(scmd->sc_data_direction));
			}
		}
	}

	MV_DBG(DMSG_SCSI_FREQ,
	       " pReq->Scsi_Status = %x pcmd = %p.\n", 
	        pReq->Scsi_Status, scmd);

#if 0//def MV_DEBUG
	if((pReq->Scsi_Status != REQ_STATUS_SUCCESS) && (SCSI_IS_READ(pReq->Cdb[0]) || SCSI_IS_WRITE(pReq->Cdb[0])))
		MV_DPRINT(( "Device %d pReq->Scsi_Status = %x cmd = 0x%x.\n", pReq->Device_Id, pReq->Scsi_Status, pReq->Cdb[0]));
#endif

	switch (pReq->Scsi_Status) {
	case REQ_STATUS_SUCCESS:
		scmd->result = 0x00;
		break;
	case REQ_STATUS_MEDIA_ERROR:
		scmd->result = (DID_BAD_TARGET << 16);
		break;
	case REQ_STATUS_BUSY:
		scmd->result = (DID_BUS_BUSY << 16);
		break;
	case REQ_STATUS_NO_DEVICE:
		scmd->result = (DID_NO_CONNECT << 16);
		break;
	case REQ_STATUS_HAS_SENSE:
		/* Sense buffer data is valid already. */
		scmd->result  = (DRIVER_SENSE << 24) | (DID_OK << 16) | CONDITION_GOOD;
		senseBuffer->Valid = 1;

		MV_DBG(DMSG_SCSI, "MV Sense: response %x SK %s length %x "
		       "ASC %x ASCQ %x.\n", ((MV_PU8) senseBuffer)[0],
		       MV_DumpSenseKey(((MV_PU8) senseBuffer)[2]),
		       ((MV_PU8) senseBuffer)[7],
		       ((MV_PU8) senseBuffer)[12],
		       ((MV_PU8) senseBuffer)[13]);
		break;
	default:
		scmd->result = (DRIVER_INVALID | DID_ABORT) << 16;
		break;
	}

	scmd->scsi_done(scmd);
}

#ifdef SUPPORT_ATA_POWER_MANAGEMENT
static inline int is_ata_16_passthrough_for_pm(struct scsi_cmnd * scmd)
{
	switch(scmd->cmnd[0]){
		case ATA_16:
			switch(scmd->cmnd[14]){
			/* exception*/
			#ifndef SUPPORT_ATA_SECURITY_CMD
				case ATA_CMD_IDENTIFY_ATA:
					return 1;
			#endif
				case ATA_CMD_DEV_RESET:
				case ATA_CMD_STANDBYNOW1:
				case ATA_CMD_IDLEIMMEDIATE:
				case ATA_CMD_STANDBY:
				case ATA_CMD_IDLE :
				case ATA_CMD_CHK_POWER:
				case ATA_CMD_SLEEP:
				case ATA_CMD_SMART:
					if(scmd->sc_data_direction == DMA_NONE &&
						scmd->cmnd[4] != 0xd4)
						return 1;
					else
						return 0;
				default:
					return 0;
			}
		default:
			break;
	}
	return 0;
}

static void mv_pm_ata_16_complete_request(struct hba_extension * phba,
		struct scsi_cmnd *scmd,PMV_Request pReq)
{
	PDomain_Device pDevice = NULL;
	MV_U8 portId, deviceId; /*, temp;*/
	PCore_Driver_Extension pCore; 

	unsigned char *sb = scmd->sense_buffer;
	unsigned char *desc = sb + 8;

	pCore = (PCore_Driver_Extension)HBA_GetModuleExtension(phba, MODULE_CORE);

	if ( pReq->Device_Id != VIRTUAL_DEVICE_ID )
	{
		portId = PATA_MapPortId(pReq->Device_Id);
		deviceId = PATA_MapDeviceId(pReq->Device_Id);
		if ( portId < MAX_PORT_NUMBER )				
			pDevice = &pCore->Ports[portId].Device[deviceId];
	}

/* !DMA_NONE */
	if (MV_SCp(scmd)->mapped) {
		if (mv_use_sg(scmd)){
			
			#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
			pci_unmap_sg(phba->dev,mv_rq_bf(scmd),mv_use_sg(scmd),
					 	scsi_to_pci_dma_dir(scmd->sc_data_direction));
			#else
			scsi_dma_unmap(scmd);
			#endif
		}
		else 
			dma_unmap_single(&(phba->dev->dev),MV_SCp(scmd)->bus_address,
						mv_rq_bf_l(scmd),scsi_to_pci_dma_dir(scmd->sc_data_direction));
	}
	

	memset(sb,0,SCSI_SENSE_BUFFERSIZE);
	scmd->result = (DRIVER_SENSE << 24) | SAM_STAT_CHECK_CONDITION; 

	switch(scmd->cmnd[14]) {
		case ATA_CMD_CHK_POWER:
			if(!(pDevice->Status & DEVICE_STATUS_SLEEP))
				desc[5] = 0xff;
			if(pDevice->Status & DEVICE_STATUS_STANDBY)
				desc[5] = 0x0;
			if((pDevice->Status & DEVICE_STATUS_IDLE))
				desc[5] = 0xff;
			break;
		case ATA_CMD_STANDBYNOW1:
			pDevice->Status |= DEVICE_STATUS_STANDBY;
			break;
		case ATA_CMD_IDLEIMMEDIATE:
			pDevice->Status |= DEVICE_STATUS_IDLE;
			break;
		case ATA_CMD_SLEEP:
			pDevice->Status |= DEVICE_STATUS_SLEEP;
			break;
			
		default:
			break;
	}
	
	/* Sense Data is current and format is descriptor */
	sb[0] = 0x72;
	desc[0] = 0x09;
	
	/* set length of additional sense data */
	sb[7] = 14; // 8+14

	desc[1] = 12;
	desc[2] = 0x00;
	desc[3] = 0; // error register

	desc[13] = 0x50; // status register

	if(scmd->cmnd[14] == ATA_CMD_SMART){
		desc[9] = scmd->cmnd[10];
		desc[10] = scmd->cmnd[11];
		desc[11] = scmd->cmnd[12];
	} else {
		desc[12] = scmd->cmnd[13]; //NO_DATA
	}

	scmd->scsi_done(scmd);
}
#ifdef SUPPORT_ATA_SECURITY_CMD
static void mv_ata_16_complete_request(struct hba_extension * phba,
		struct scsi_cmnd *scmd,PMV_Request pReq)
{
	PDomain_Device pDevice = NULL;
	MV_U8 portId, deviceId; /*, temp;*/
	PCore_Driver_Extension pCore; 

	unsigned char *sb = scmd->sense_buffer;
	unsigned char *desc = sb + 8;

	pCore = (PCore_Driver_Extension)HBA_GetModuleExtension(phba, MODULE_CORE);

	if ( pReq->Device_Id != VIRTUAL_DEVICE_ID )
	{
		portId = PATA_MapPortId(pReq->Device_Id);
		deviceId = PATA_MapDeviceId(pReq->Device_Id);
		if ( portId < MAX_PORT_NUMBER )				
			pDevice = &pCore->Ports[portId].Device[deviceId];
	}
	
	if (MV_SCp(scmd)->mapped) {
		if (mv_use_sg(scmd)){
			
			#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
			pci_unmap_sg(phba->dev,mv_rq_bf(scmd),mv_use_sg(scmd),
					 	scsi_to_pci_dma_dir(scmd->sc_data_direction));
			#else
			scsi_dma_unmap(scmd);
			#endif
		}
		else 
			dma_unmap_single(&(phba->dev->dev),MV_SCp(scmd)->bus_address,
						mv_rq_bf_l(scmd),scsi_to_pci_dma_dir(scmd->sc_data_direction));
	}
	
	memset(sb,0,SCSI_SENSE_BUFFERSIZE);
	scmd->result = (DRIVER_SENSE << 24) | SAM_STAT_CHECK_CONDITION; 
	
	/* Sense Data is current and format is descriptor */
	sb[0] = 0x72;
	sb[1]=0x0;
	desc[0] = 0x09;
	/* set length of additional sense data */
	sb[7] = 14; // 8+14

	desc[1] = 12;
	desc[2] = 0x00;
	desc[3] = 0; 		// error register
	desc[13] = 0x04; 	// status register
	
	if(scmd->cmnd[14] == ATA_CMD_SMART){
		desc[9] = scmd->cmnd[10];
		desc[10] = scmd->cmnd[11];
		desc[11] = scmd->cmnd[12];
	} else {
		desc[12] = scmd->cmnd[13]; //NO_DATA
	}
	switch(scmd->cmnd[14]) {
		case ATA_CMD_SEC_UNLOCK:
		case ATA_CMD_SEC_ERASE_UNIT:
			if(pReq->Scsi_Status==REQ_STATUS_SUCCESS){
				if(pDevice->Capacity & DEVICE_CAPACITY_SECURITY_SUPPORTED){
					if(pDevice->Setting & DEVICE_SETTING_SECURITY_LOCKED){
						MV_DPRINT(("securiy: unlocked\n"));
						pDevice->Setting &= ~DEVICE_SETTING_SECURITY_LOCKED;
						}
					}
			}	
			break;
		case ATA_CMD_IDENTIFY_ATA:
			sb[3]=0x1d;
			break;
		case ATA_CMD_SEC_PASSWORD:
		case ATA_CMD_SEC_DISABLE_PASSWORD:
		case ATA_CMD_SEC_ERASE_PRE:
		default:
			break;
	}
	switch(pReq->Scsi_Status) {
		case REQ_STATUS_ABORT:
			sb[1]=SCSI_SK_ABORTED_COMMAND;
			desc[3]=0x04;
			desc[13]=0x51;
			break;
		default:
			break;
		}
	scmd->scsi_done(scmd);
}
#endif
void mv_clear_device_status(struct hba_extension * phba,PMV_Request pReq)
{
	PDomain_Device pDevice = NULL;
	MV_U8 portId, deviceId; /*, temp;*/
	PCore_Driver_Extension pCore; 

	pCore = (PCore_Driver_Extension)HBA_GetModuleExtension(phba, MODULE_CORE);

	if ( pReq->Device_Id != VIRTUAL_DEVICE_ID )
	{
		portId = PATA_MapPortId(pReq->Device_Id);
		deviceId = PATA_MapDeviceId(pReq->Device_Id);
		if ( portId < MAX_PORT_NUMBER )				
			pDevice = &pCore->Ports[portId].Device[deviceId];
		pDevice->Status &= ~(DEVICE_STATUS_IDLE|DEVICE_STATUS_STANDBY);
	}
	return ;
}

#endif
/* This should be the _only_ os request exit point. */
static void hba_req_callback(MV_PVOID This, PMV_Request pReq)
{
	struct hba_extension *phba = (struct hba_extension *)This;
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)pReq->Org_Req;

	/* Return this request to OS. */
#ifdef SUPPORT_ATA_POWER_MANAGEMENT
	if(is_ata_16_passthrough_for_pm(scmd) && 
		pReq->Scsi_Status == REQ_STATUS_SUCCESS)
			mv_pm_ata_16_complete_request(phba,scmd,pReq);
#endif
#ifdef SUPPORT_ATA_SECURITY_CMD
	else if(scmd->cmnd[0]==ATA_16){
		mv_ata_16_complete_request(phba,scmd,pReq);
		}
#endif
#ifdef SUPPORT_ATA_POWER_MANAGEMENT
	else{
		mv_clear_device_status(phba,pReq);
#endif
		mv_complete_request(phba, scmd, pReq);
#ifdef SUPPORT_ATA_POWER_MANAGEMENT
	}
#endif
	phba->Io_Count--;
	res_free_req_to_pool(phba->req_pool, pReq);
}


static int scsi_cmd_to_req_conv(struct hba_extension *phba, 
				struct scsi_cmnd *scmd, 
				PMV_Request pReq)
{
	/*
	 * Set three flags: CMD_FLAG_NON_DATA 
	 *                  CMD_FLAG_DATA_IN 
	 *                  CMD_FLAG_DMA
	 * currently data in/out all go thru DMA
	 */
	pReq->Cmd_Flag = 0;
	switch (scmd->sc_data_direction) {
	case DMA_NONE:
		pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
		break;
	case DMA_FROM_DEVICE:
		pReq->Cmd_Flag |= CMD_FLAG_DATA_IN;
	case DMA_TO_DEVICE:
		pReq->Cmd_Flag |= CMD_FLAG_DMA;
		break;
	case DMA_BIDIRECTIONAL : 
		MV_DBG(DMSG_SCSI, "unexpected DMA_BIDIRECTIONAL.\n"
		       );
		break;
	default:
		break;
	}

	/* handling of specific cmds */
	memset(pReq->Cdb, 0, MAX_CDB_SIZE);
	switch (scmd->cmnd[0]){
#if 0
	case MODE_SELECT:
		pReq->Cdb[0] = MODE_SELECT_10;
		pReq->Cdb[1] = scmd->cmnd[1];
		pReq->Cdb[8] = scmd->cmnd[4];
		break;
#endif
	case READ_6:
		pReq->Cdb[0] = READ_10;
		pReq->Cdb[3] = scmd->cmnd[1]&0x1f;
		pReq->Cdb[4] = scmd->cmnd[2];
		pReq->Cdb[5] = scmd->cmnd[3];
		pReq->Cdb[8] = scmd->cmnd[4];
		pReq->Cdb[9] = scmd->cmnd[5];
		break;

	case WRITE_6:
		pReq->Cdb[0] = WRITE_10;
		pReq->Cdb[3] = scmd->cmnd[1]&0x1f;
		pReq->Cdb[4] = scmd->cmnd[2];
		pReq->Cdb[5] = scmd->cmnd[3];
		pReq->Cdb[8] = scmd->cmnd[4];
		pReq->Cdb[9] = scmd->cmnd[5];
		break;
	/*0xA1==SCSI_CMD_BLANK== ATA_12 */
	case 0xA1:
		if(IS_ATA_12_CMD(scmd))
			pReq->Cmd_Flag |= CMD_FLAG_SMART_ATA_12;
		else{
			memcpy(pReq->Cdb, scmd->cmnd, MAX_CDB_SIZE);
			break;
		}
	case ATA_16:
		pReq->Cmd_Flag |= CMD_FLAG_SMART_ATA_16;
		
		pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		pReq->Cdb[1] = CDB_CORE_MODULE;

		memcpy(&pReq->Cdb[3],&scmd->cmnd[3],13);

		switch(scmd->cmnd[ (pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?9 : 14])
		{
		#ifdef SUPPORT_ATA_POWER_MANAGEMENT
			case 0x08:
				pReq->Cdb[2] = CDB_CORE_RESET_DEVICE;
				break;
			case 0xE0:
				pReq->Cdb[2] = CDB_CORE_ATA_STANDBY_IMMEDIATE;
				break;
			case 0xE1:
				pReq->Cdb[2] = CDB_CORE_ATA_IDLE_IMMEDIATE;
				break;
			case 0xE2:
				pReq->Cdb[2] = CDB_CORE_ATA_STANDBY;
				break;
			case 0xE3:
				pReq->Cdb[2] = CDB_CORE_ATA_IDLE;
				break;
			case 0xE5:
				pReq->Cdb[2] = CDB_CORE_ATA_CHECK_POWER_MODE;
				break;
			case 0xE6:
				pReq->Cdb[2] = CDB_CORE_ATA_SLEEP;
				break;
		#endif
			case 0xEC:
				pReq->Cdb[2] = CDB_CORE_ATA_IDENTIFY;
				break;
		#ifdef SUPPORT_ATA_SMART
			case ATA_IDENTIFY_PACKET_DEVICE:
				pReq->Cdb[2] = CDB_CORE_ATA_IDENTIFY_PACKET_DEVICE;
				break;
			case ATA_SMART_CMD :
				switch(scmd->cmnd[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?3 : 4])
				{
					case ATA_SMART_ENABLE:
						pReq->Cdb[2] = CDB_CORE_ENABLE_SMART;
						break;
					case ATA_SMART_DISABLE:
						pReq->Cdb[2] = CDB_CORE_DISABLE_SMART;
						break;
					case ATA_SMART_STATUS:
						pReq->Cdb[2] = CDB_CORE_SMART_RETURN_STATUS;
						break;
					case ATA_SMART_READ_VALUES:
						pReq->Cdb[2] = CDB_CORE_ATA_SMART_READ_VALUES;
						break;
					case ATA_SMART_READ_THRESHOLDS:
						pReq->Cdb[2] = CDB_CORE_ATA_SMART_READ_THRESHOLDS;
						break;
					case  ATA_SMART_READ_LOG_SECTOR:
						pReq->Cdb[2] = CDB_CORE_ATA_SMART_READ_LOG_SECTOR;
						break;
					case ATA_SMART_WRITE_LOG_SECTOR:
						pReq->Cdb[2] = CDB_CORE_ATA_SMART_WRITE_LOG_SECTOR;
						break;
					case  ATA_SMART_AUTO_OFFLINE:
						pReq->Cdb[2] = CDB_CORE_ATA_SMART_AUTO_OFFLINE;
						break;
					case  ATA_SMART_AUTOSAVE:
						pReq->Cdb[2] = CDB_CORE_ATA_SMART_AUTOSAVE;
						break;	
					case  ATA_SMART_IMMEDIATE_OFFLINE:
						pReq->Cdb[2] = CDB_CORE_ATA_SMART_IMMEDIATE_OFFLINE;
						break;	
					default:
						pReq->Scsi_Status   =REQ_STATUS_INVALID_PARAMETER;
						MV_PRINT("Unsupported ATA-12 or 16  subcommand =0x%x\n", \
						scmd->cmnd[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?3 : 4]);
						return -1;
				}
				break;
			case 0xef:
				switch(scmd->cmnd[4]){
					case 0x2:
						pReq->Cdb[2]=CDB_CORE_ENABLE_WRITE_CACHE ;
					break;
					case 0x82:
						pReq->Cdb[2]=CDB_CORE_DISABLE_WRITE_CACHE;
					break;
					default:
					pReq->Scsi_Status   =REQ_STATUS_INVALID_PARAMETER;
					MV_PRINT("Enable/Disable Write Cache command error: parameter error:%d\n",scmd->cmnd[4]);
					return -1;
				}
			break;
		#endif /*#ifdef SUPPORT_ATA_SMART*/
		#ifdef SUPPORT_ATA_SECURITY_CMD
			case ATA_CMD_SEC_PASSWORD:	
			case ATA_CMD_SEC_UNLOCK:
					if(0){
						int i;
						char *buf=NULL;
						for(i=0;i<16;i++)
							printk(" %d:%x ",i,scmd->cmnd[i]);
						printk("\n");
						buf=NULL;
						buf=bio_data(scmd->request->bio);
						if(buf){
							printk("bio=");
							for(i=0;i<16;i++)
							printk(" %x ",buf[i]);
							printk("\n");
							} else
							printk("bio is NULL\n");
					}
			case ATA_CMD_SEC_ERASE_PRE:
			case ATA_CMD_SEC_ERASE_UNIT:
			case ATA_CMD_SEC_FREEZE_LOCK:
			case ATA_CMD_SEC_DISABLE_PASSWORD:
				pReq->Cmd_Flag &= ~CMD_FLAG_SMART_ATA_16;
				memcpy(&pReq->Cdb[0],&scmd->cmnd[0],16);
				break;
			#endif
			default:
				pReq->Scsi_Status   =REQ_STATUS_INVALID_PARAMETER;
				MV_PRINT("Unsupported ATA-12 or 16 Command=0x%x\n",\
				scmd->cmnd[(pReq->Cmd_Flag & CMD_FLAG_SMART_ATA_12) ?9 : 14]);
				return -1;
		}
		break;
	default:
		memcpy(pReq->Cdb, scmd->cmnd, MAX_CDB_SIZE);
		break;
	}
	
	pReq->Data_Buffer = mv_rq_bf(scmd);
	pReq->Data_Transfer_Length = mv_rq_bf_l(scmd);
	pReq->Sense_Info_Buffer = scmd->sense_buffer;
	pReq->Sense_Info_Buffer_Length = SCSI_SENSE_BUFFERSIZE;

	SGTable_Init(&pReq->SG_Table, 0);
	generate_sg_table(phba, scmd, &pReq->SG_Table);

	pReq->LBA.value = 0;
	pReq->Sector_Count = 0;
	MV_SetLBAandSectorCount(pReq);

	pReq->Req_Type      = REQ_TYPE_OS;
	pReq->Org_Req       = scmd;
	pReq->Tag           = scmd->tag;
	pReq->Scsi_Status   = REQ_STATUS_PENDING;
	pReq->Completion    = hba_req_callback;
	pReq->Cmd_Initiator = phba;
	//pReq->Scsi_Status   = REQ_STATUS_INVALID_REQUEST;
	pReq->Device_Id     = __MAKE_DEV_ID(mv_scmd_target(scmd),
					    mv_scmd_lun(scmd));

	return 0;
}
static void hba_shutdown_req_cb(MV_PVOID this, PMV_Request req)
{
	struct hba_extension *phba = (struct hba_extension *) this;
	#ifdef SUPPORT_REQUEST_TIMER
	if(req!=NULL)
	{
		MV_DBG(DMSG_HBA,"Shutdown HBA timer!\n");
		hba_remove_timer_sync(req);
	}
	#endif
	res_free_req_to_pool(phba->req_pool, req);
	phba->Io_Count--;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->hba_sync, 0);
#else
	complete(&phba->cmpl);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
/* will wait for atomic value atomic to become zero until timed out */
/* return how much 'timeout' is left or 0 if already timed out */
int __hba_wait_for_atomic_timeout(atomic_t *atomic, unsigned long timeout)
{
	unsigned intv = HZ/20; 

	while (timeout) {
		if (0 == atomic_read(atomic))
			break;

		if (timeout < intv)
			intv = timeout;
		set_current_state(TASK_INTERRUPTIBLE);
		timeout -= (intv - schedule_timeout(intv));
	}
	return timeout;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */

#ifdef CACHE_MODULE_SUPPORT
static void _hba_send_shutdown_req(PHBA_Extension phba)
{
	unsigned long flags;
	PMV_Request pReq;

	/*Send MV_REQUEST to do something.*/	
	pReq = res_get_req_from_pool(phba->req_pool);
	if (NULL == pReq) {
		MV_PRINT("cannot allocate memory for req.\n"
		       );
		return;
	}

	pReq->Cmd_Initiator = phba;
	pReq->Org_Req = NULL;
	pReq->Completion = hba_shutdown_req_cb;
	pReq->Req_Type = REQ_TYPE_OS;
	pReq->Cmd_Flag = 0;
	pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
	pReq->Sense_Info_Buffer_Length = 0;  
	pReq->Data_Transfer_Length = 0;
	pReq->Data_Buffer = NULL;
	pReq->Sense_Info_Buffer = NULL;

	pReq->LBA.value = 0;
	pReq->Sector_Count = 0;

	pReq->Scsi_Status = REQ_STATUS_SUCCESS;

	SGTable_Init(&pReq->SG_Table, 0);
	memset(pReq->Context, 0, sizeof(MV_PVOID) * MAX_POSSIBLE_MODULE_NUMBER);

	MV_DPRINT(("send SHUTDOWN request to CACHE.\n"));
	pReq->Cdb[0] = APICDB0_ADAPTER;
	pReq->Cdb[1] = APICDB1_ADAPTER_POWER_STATE_CHANGE;
	pReq->Cdb[2] = 0;

	spin_lock_irqsave(&phba->desc->hba_desc->global_lock, flags);
	phba->Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->hba_sync, 1);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);
	spin_unlock_irqrestore(&phba->desc->hba_desc->global_lock, flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	__hba_wait_for_atomic_timeout(&phba->hba_sync, 10*HZ);
#else
	wait_for_completion_timeout(&phba->cmpl, 10*HZ);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
}
#endif /* CACHE_MODULE_SUPPORT */

void hba_send_shutdown_req(struct hba_extension *phba)
{
	unsigned long flags;
	PMV_Request pReq;
#ifdef CACHE_MODULE_SUPPORT
	_hba_send_shutdown_req(phba);
#endif
	
	pReq = res_get_req_from_pool(phba->req_pool);
	if (NULL == pReq) {
		MV_PRINT("cannot allocate memory for req.\n");
		return;
	}

	pReq->Cmd_Initiator = phba;
	pReq->Org_Req = pReq;
	pReq->Req_Type = REQ_TYPE_INTERNAL;
	pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
	pReq->Completion = hba_shutdown_req_cb;
	
#ifdef RAID_DRIVER
	pReq->Cdb[0] = APICDB0_LD;
	pReq->Cdb[1] = APICDB1_LD_SHUTDOWN;
#else
	pReq->Device_Id = 0;
	pReq->Cmd_Flag = 0;
	pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
	pReq->Sense_Info_Buffer_Length = 0;  
	pReq->Data_Transfer_Length = 0;
	pReq->Data_Buffer = NULL;
	pReq->Sense_Info_Buffer = NULL;
	SGTable_Init(&pReq->SG_Table, 0);
	pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	pReq->Cdb[1] = CDB_CORE_MODULE;
	pReq->Cdb[2] = CDB_CORE_SHUTDOWN;
	pReq->LBA.value = 0;
	pReq->Sector_Count = 0;
	pReq->Scsi_Status = REQ_STATUS_PENDING;
#endif

	spin_lock_irqsave(&phba->desc->hba_desc->global_lock, flags);
	phba->Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->hba_sync, 1);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);
	spin_unlock_irqrestore(&phba->desc->hba_desc->global_lock, flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	__hba_wait_for_atomic_timeout(&phba->hba_sync, 10*HZ);
#else
	wait_for_completion_timeout(&phba->cmpl, 10*HZ);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7)
/*openSUSE 11.1 SLES 11 SLED 11*/
#if ((LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 27))&&(IS_OPENSUSE_SLED_SLES))
static enum blk_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
static enum scsi_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
#else
static enum blk_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
#endif
{
	MV_BOOLEAN ret = MV_TRUE;
#if ((LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 27))&&(IS_OPENSUSE_SLED_SLES))
	return (ret)?BLK_EH_RESET_TIMER:BLK_EH_NOT_HANDLED;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	return (ret)?EH_RESET_TIMER:EH_NOT_HANDLED;
#else
	return (ret)?BLK_EH_RESET_TIMER:BLK_EH_NOT_HANDLED;
#endif
}
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7) */

static inline struct mv_mod_desc *
__get_lowest_module(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;
	
	/* take a random module, and trace through its child */
	p = list_entry(hba_desc->online_module_list.next, 
		       struct mv_mod_desc,
		       mod_entry);

	WARN_ON(NULL == p);
	while (p) {
		if (NULL == p->child)
			break;
		p = p->child;
	}
	return p;
}

static int mv_linux_queue_command(struct scsi_cmnd *scmd, 
				  void (*done) (struct scsi_cmnd *))
{
	struct Scsi_Host *host = mv_scmd_host(scmd);
	struct hba_extension *hba = *((struct hba_extension * *) host->hostdata);
	PMV_Request req;
	unsigned long flags;

	if (done == NULL) {
		MV_PRINT( ": in queuecommand, done function can't be NULL\n");
		return 0;
    	}


#if 1
	MV_DBG(DMSG_SCSI_FREQ,
	       "mv_linux_queue_command %p (%d/%d/%d/%d \
	       Cdb=(%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x))\n", 
	       scmd, host->host_no, mv_scmd_channel(scmd), 
	       mv_scmd_target(scmd), mv_scmd_lun(scmd),
	       *(scmd->cmnd), *(scmd->cmnd+1), 
	       *(scmd->cmnd+2), *(scmd->cmnd+3), 
	        *(scmd->cmnd+4), *(scmd->cmnd+5), 
	        *(scmd->cmnd+6), *(scmd->cmnd+7), 
	        *(scmd->cmnd+8), *(scmd->cmnd+9), 
	        *(scmd->cmnd+10), *(scmd->cmnd+11), 
	        *(scmd->cmnd+12), *(scmd->cmnd+13), 
	        *(scmd->cmnd+14),*(scmd->cmnd+15));
#endif
	scmd->result = 0;
 	scmd->scsi_done = done;
	MV_SCp(scmd)->bus_address = 0;
	MV_SCp(scmd)->mapped = 0;
	MV_SCp(scmd)->map_atomic = 0;

#ifdef COMMAND_ISSUE_WORKROUND
{
	MV_U8 mv_core_check_is_reseeting(MV_PVOID core_ext);
	struct mv_mod_desc *core_desc=__get_lowest_module(hba->desc->hba_desc);
	if(mv_core_check_is_reseeting(core_desc->extension)){
		//MV_DPRINT(("HOST is resetting, wait..\n"));
		return SCSI_MLQUEUE_HOST_BUSY;
	}

}
#endif
	if (mv_scmd_channel(scmd)) {
		scmd->result = DID_BAD_TARGET << 16;
		goto done;
	}

	/* 
	 * Get mv_request resource and translate the scsi_cmnd request 
	 * to mv_request.
	 */
	req = res_get_req_from_pool(hba->req_pool);
	if (req == NULL)
	{
		MV_DPRINT(("No sufficient request.\n"));
		return SCSI_MLQUEUE_HOST_BUSY;
	}
	if (scsi_cmd_to_req_conv(hba, scmd, req)) {
		struct scsi_cmnd *cmd = (struct scsi_cmnd *)req->Org_Req;
		/* 
		 * Even TranslateOSRequest failed, 
		 * it still should set some of the variables to the MV_Request
		 * especially MV_Request.Org_Req and MV_Request.Scsi_Status;
		 */
		if(!cmd || req->Scsi_Status==REQ_STATUS_INVALID_PARAMETER){
			res_free_req_to_pool(hba->req_pool, req);
			//scmd->result = (DRIVER_INVALID | SUGGEST_ABORT) << 24;
			scmd->result |= DID_ABORT << 16;
			scmd->scsi_done(scmd);
			return 0;			
		}	
		MV_DBG(DMSG_HBA,
		       "ERROR - Translation from OS Request failed.\n");
		hba_req_callback(hba, req);
		return 0;
	}

	spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);
	hba->Io_Count++;

	if (hba->State != DRIVER_STATUS_STARTED) {
		MV_ASSERT(0);
		/*if ( hba->State==DRIVER_STATUS_IDLE )
		  {
		  hba->State = DRIVER_STATUS_STARTING;
		  Module_StartAll(module_manage, MODULE_CORE);
		  }*/
	} else {
		hba->desc->ops->module_sendrequest(hba->desc->extension, req);
	}
	spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);

	return 0;
done:
        scmd->scsi_done(scmd);
        return 0;
}

#if 0
static int mv_linux_abort(struct scsi_cmnd *cmd)
{
	struct Scsi_Host *host;
	struct hba_extension *phba;
	int  ret = FAILED;

	MV_PRINT("__MV__ abort command %p.\n", cmd);

	return ret;
}
#endif /* 0 */
#ifdef IOCTL_TEMPLATE
/****************************************************************
*  Name:   mv_ial_ht_ata_cmd
*
*  Description:    handles mv_sata ata IOCTL special drive command (HDIO_DRIVE_CMD)
*
*  Parameters:     scsidev - Device to which we are issuing command
*                  arg     - User provided data for issuing command
*
*  Returns:        0 on success, otherwise of failure.
*
****************************************************************/
static int mv_ial_ht_ata_cmd(struct scsi_device *scsidev, void __user *arg)
{
	int rc = 0;
     	u8 scsi_cmd[MAX_COMMAND_SIZE];
      	u8 args[4] , *argbuf = NULL, *sensebuf = NULL;
      	int argsize = 0;
      	enum dma_data_direction data_dir;
      	int cmd_result;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	struct scsi_request *sreq;
#endif
      	if (arg == NULL)
          	return -EINVAL;
  
      	if (copy_from_user(args, arg, sizeof(args)))
          	return -EFAULT;
      	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sensebuf = kmalloc(SCSI_SENSE_BUFFERSIZE, GFP_NOIO);
	if (sensebuf) {
		memset(sensebuf, 0, SCSI_SENSE_BUFFERSIZE);
	}
#else
	sensebuf = kzalloc(SCSI_SENSE_BUFFERSIZE, GFP_NOIO);
#endif 

      	if (!sensebuf)
          	return -ENOMEM;
  
      	memset(scsi_cmd, 0, sizeof(scsi_cmd));
      	if (args[3]) {
          	argsize = SECTOR_SIZE * args[3];
          	argbuf = kmalloc(argsize, GFP_KERNEL);
          	if (argbuf == NULL) {
              		rc = -ENOMEM;
              		goto error;
     	}
  
     	scsi_cmd[1]  = (4 << 1); /* PIO Data-in */
     	scsi_cmd[2]  = 0x0e;     /* no off.line or cc, read from dev,
                                             block count in sector count field */
      	data_dir = DMA_FROM_DEVICE;
      	} else {
       		scsi_cmd[1]  = (3 << 1); /* Non-data */
       		scsi_cmd[2]  = 0x20;     /* cc but no off.line or data xfer */
      		data_dir = DMA_NONE;
	}
  
	scsi_cmd[0] = ATA_16;
  
   	scsi_cmd[4] = args[2];
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
    	if (args[0] == WIN_SMART) { /* hack -- ide driver does this too... */
#else
	if (args[0] == ATA_CMD_SMART) { /* hack -- ide driver does this too... */	
#endif	
        	scsi_cmd[6]  = args[3];
       		scsi_cmd[8]  = args[1];
     		scsi_cmd[10] = 0x4f;
    		scsi_cmd[12] = 0xc2;
      	} else {
          	scsi_cmd[6]  = args[1];
      	}
      	scsi_cmd[14] = args[0];
      	
      	/* Good values for timeout and retries?  Values below
         	from scsi_ioctl_send_command() for default case... */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sreq = scsi_allocate_request(scsidev, GFP_KERNEL);
	if (!sreq) {
		rc= -EINTR;
		goto free_req;
	}
	sreq->sr_data_direction = data_dir;
	scsi_wait_req(sreq, scsi_cmd, argbuf, argsize, (10*HZ), 5);

	/* 
	 * If there was an error condition, pass the info back to the user. 
	 */
	cmd_result = sreq->sr_result;
	sensebuf = sreq->sr_sense_buffer;
   
#elif LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 29)
      	cmd_result = scsi_execute(scsidev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0,0);
#else
      	cmd_result = scsi_execute(scsidev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0);
#endif

  
      	if (driver_byte(cmd_result) == DRIVER_SENSE) {/* sense data available */
         	u8 *desc = sensebuf + 8;
          	cmd_result &= ~(0xFF<<24); /* DRIVER_SENSE is not an error */
  
          	/* If we set cc then ATA pass-through will cause a
          	* check condition even if no error. Filter that. */
          	if (cmd_result & SAM_STAT_CHECK_CONDITION) {
              	struct scsi_sense_hdr sshdr;
              	scsi_normalize_sense(sensebuf, SCSI_SENSE_BUFFERSIZE,
                                   &sshdr);
              	if (sshdr.sense_key==0 &&
                  	sshdr.asc==0 && sshdr.ascq==0)
                  	cmd_result &= ~SAM_STAT_CHECK_CONDITION;
          	}
  
          	/* Send userspace a few ATA registers (same as drivers/ide) */
          	if (sensebuf[0] == 0x72 &&     /* format is "descriptor" */
              		desc[0] == 0x09 ) {        /* code is "ATA Descriptor" */
              		args[0] = desc[13];    /* status */
              		args[1] = desc[3];     /* error */
              		args[2] = desc[5];     /* sector count (0:7) */
              		if (copy_to_user(arg, args, sizeof(args)))
                  		rc = -EFAULT;
          	}
      	}
  
      	if (cmd_result) {
          	rc = -EIO;
          	goto free_req;
      	}
  
      	if ((argbuf) && copy_to_user(arg + sizeof(args), argbuf, argsize))
          	rc = -EFAULT;
      	
free_req:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	scsi_release_request(sreq);
#endif		
error:
      	if (sensebuf) kfree(sensebuf);
      	if (argbuf) kfree(argbuf);
      	return rc;
}

#ifdef SUPPORT_ATA_SECURITY_CMD
static int check_dma (__u8 ata_op)
{
	switch (ata_op) {
		case ATA_CMD_READ_DMA_EXT:
		case ATA_CMD_READ_FPDMA_QUEUED:
		case ATA_CMD_WRITE_DMA_EXT:
		case ATA_CMD_WRITE_FPDMA_QUEUED:
		case ATA_CMD_READ_DMA:
		case ATA_CMD_WRITE_DMA:
			return SG_DMA;
		default:
			return SG_PIO;
	}
}
unsigned char excute_taskfile(struct scsi_device *dev,ide_task_request_t *req_task,u8 
 rw,char *argbuf,unsigned int buff_size)
{
	int rc = 0;
     	u8 scsi_cmd[MAX_COMMAND_SIZE];
      	u8 *sensebuf = NULL;
      	int argsize=0;
	enum dma_data_direction data_dir;
      	int cmd_result;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	struct scsi_request *sreq;
#endif
      	argsize=buff_size;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sensebuf = kmalloc(SCSI_SENSE_BUFFERSIZE, GFP_NOIO);
	if (sensebuf) {
		memset(sensebuf, 0, SCSI_SENSE_BUFFERSIZE);
	}
#else
	sensebuf = kzalloc(SCSI_SENSE_BUFFERSIZE, GFP_NOIO);
#endif 
      	if (!sensebuf)
          	return -ENOMEM;
  
      	memset(scsi_cmd, 0, sizeof(scsi_cmd));

  	data_dir = DMA_FROM_DEVICE;   // need to fixed
	scsi_cmd[0] = ATA_16;
	scsi_cmd[13] = 0x40;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
      	scsi_cmd[14] = ((task_struct_t *)(&req_task->io_ports))->command;
#else
	scsi_cmd[14] = ((char *)(&req_task->io_ports))[7];
#endif
	if(check_dma(scsi_cmd[14])){
		scsi_cmd[1] = argbuf ? SG_ATA_PROTO_DMA : SG_ATA_PROTO_NON_DATA;
	} else {
		scsi_cmd[1] = argbuf ? (rw ? SG_ATA_PROTO_PIO_OUT : SG_ATA_PROTO_PIO_IN) : SG_ATA_PROTO_NON_DATA;
	}
	scsi_cmd[ 2] = SG_CDB2_CHECK_COND;
	if (argbuf) {
		scsi_cmd[2] |= SG_CDB2_TLEN_NSECT | SG_CDB2_TLEN_SECTORS;
		scsi_cmd[2] |= rw ? SG_CDB2_TDIR_TO_DEV : SG_CDB2_TDIR_FROM_DEV;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sreq = scsi_allocate_request(dev, GFP_KERNEL);
	if (!sreq) {
		rc= -EINTR;
		goto free_req;
	}
	sreq->sr_data_direction = data_dir;
	scsi_wait_req(sreq, scsi_cmd, argbuf, argsize, (10*HZ), 5);

	/* 
	 * If there was an error condition, pass the info back to the user. 
	 */
	cmd_result = sreq->sr_result;
	sensebuf = sreq->sr_sense_buffer;
   
#elif LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 29)
      	cmd_result = scsi_execute(dev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0,0);
#else
      	cmd_result = scsi_execute(dev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0);
#endif
  
      	if (driver_byte(cmd_result) == DRIVER_SENSE) {/* sense data available */
         	u8 *desc = sensebuf + 8;
          	cmd_result &= ~(0xFF<<24); /* DRIVER_SENSE is not an error */
  		
          	/* If we set cc then ATA pass-through will cause a
          	* check condition even if no error. Filter that. */
          	if (cmd_result & SAM_STAT_CHECK_CONDITION) {
              	struct scsi_sense_hdr sshdr;
              	scsi_normalize_sense(sensebuf, SCSI_SENSE_BUFFERSIZE,
                                   &sshdr);
              	if (sshdr.sense_key==0 &&
                  	sshdr.asc==0 && sshdr.ascq==0)
                  	cmd_result &= ~SAM_STAT_CHECK_CONDITION;
          	}
  
      	}
  
      	if (cmd_result) {
          	rc = EIO;
		MV_PRINT("EIO=%d\n",-EIO);
          	goto free_req;
      	}
#if 0
      	if ( copy_to_user(argbuf,sensebuf, argsize))
       		rc = -EFAULT;
#endif
free_req:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	scsi_release_request(sreq);
#endif		
      	if (sensebuf) kfree(sensebuf);
      	return rc;
}
u8 mv_do_taskfile_ioctl(struct scsi_device *dev,void __user *arg){
	ide_task_request_t *req_task=NULL;
	char __user *buf = (char __user *)arg;
	u8 *outbuf	= NULL;
	u8 *inbuf	= NULL;
	int err		= 0;
	int tasksize	= sizeof(ide_task_request_t);
	int taskin	= 0;
	int taskout	= 0;
	int rw = SG_READ;
	
	req_task = kzalloc(tasksize, GFP_KERNEL);
	if (req_task == NULL) return -ENOMEM;
	if (copy_from_user(req_task, buf, tasksize)) {
		kfree(req_task);
		return -EFAULT;
	}

	switch (req_task->req_cmd) {
		case TASKFILE_CMD_REQ_OUT:
		case TASKFILE_CMD_REQ_RAW_OUT:
			rw         = SG_WRITE;
			break;
		case TASKFILE_CMD_REQ_IN:
			break;
	}
	taskout = (int) req_task->out_size;
	taskin  = (int) req_task->in_size;


	if (taskout) {
		int outtotal = tasksize;
		outbuf = kzalloc(taskout, GFP_KERNEL);
		if (outbuf == NULL) {
			err = -ENOMEM;
			goto abort;
		}
		if (copy_from_user(outbuf, buf + outtotal, taskout)) {
			err = -EFAULT;
			goto abort;
		}
	}

	if (taskin) {
		int intotal = tasksize + taskout;
		inbuf = kzalloc(taskin, GFP_KERNEL);
		if (inbuf == NULL) {
			err = -ENOMEM;
			goto abort;
		}
		if (copy_from_user(inbuf, buf + intotal, taskin)) {
			err = -EFAULT;
			goto abort;
		}
	}
	
	switch(req_task->data_phase) {
		case TASKFILE_DPHASE_PIO_OUT:
			err=excute_taskfile(dev,req_task,rw,outbuf,taskout);
		default:
			err = -EFAULT;
			goto abort;
	}
	if (copy_to_user(buf, req_task, tasksize)) {
		err = -EFAULT;
		goto abort;
	}
	if (taskout) {
		int outtotal = tasksize;
		if (copy_to_user(buf + outtotal, outbuf, taskout)) {
			err = -EFAULT;
			goto abort;
		}
	}
	if (taskin) {
		int intotal = tasksize + taskout;
		if (copy_to_user(buf + intotal, inbuf, taskin)) {
			err = -EFAULT;
			goto abort;
		}
	}
abort:
	kfree(req_task);
	kfree(outbuf);
	kfree(inbuf);

	return err;
}
#endif

/****************************************************************
 *  Name:   mv_ial_ht_ioctl
 *
 *  Description:    mv_sata scsi ioctl
 *
 *  Parameters:     scsidev - Device to which we are issuing command
 *                  cmd     - ioctl command
 *                  arg     - User provided data for issuing command
 *
 *  Returns:        0 on success, otherwise of failure.
 *
 ****************************************************************/
static int mv_ial_ht_ioctl(struct scsi_device *scsidev, int cmd, void __user *arg)
{
     	int rc = -ENOTTY;
 
     	/* No idea how this happens.... */
     	if (!scsidev)
         	return -ENXIO;
 
     	if (arg == NULL)
         	return -EINVAL;
 
     	switch (cmd) {
         	case HDIO_DRIVE_CMD:
             		if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
                 		return -EACCES;
 
             		rc =  mv_ial_ht_ata_cmd(scsidev, arg);
             	break;
		#ifdef SUPPORT_ATA_SECURITY_CMD
 		case HDIO_DRIVE_TASKFILE:
			rc=mv_do_taskfile_ioctl(scsidev,arg);
			break;
		#endif
         	default:
             		rc = -ENOTTY;
     	}
 
     	return rc;
}
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
static irqreturn_t mv_intr_handler(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t mv_intr_handler(int irq, void *dev_id)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19) */
{
	/* MV_FALSE should be equal to IRQ_NONE (0) */
	irqreturn_t retval = MV_FALSE;
	unsigned long flags;

	struct hba_extension *hba = (struct hba_extension *) dev_id;
	//MV_DBG(DMSG_FREQ, "__MV__ Enter intr handler.\n");
	
	spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);
	retval = hba->desc->child->ops->module_service_isr(hba->desc->child->extension);
	#ifdef SUPPORT_TASKLET
	if(retval)
		tasklet_schedule(&hba->mv_tasklet);
	#endif

	spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);

	//MV_DBG(DMSG_FREQ, "__MV__ Exit intr handler retval=0x%x.\n ",retval);
	return IRQ_RETVAL(retval);
}

static int mv_linux_reset (struct scsi_cmnd *cmd)
{
	MV_PRINT("__MV__ reset handler %p.\n", cmd);
	return FAILED;
}

static struct scsi_host_template mv_driver_template = {
	.module                      =  THIS_MODULE,
        .name                        =  "Marvell Storage Controller",
        .proc_name                   =  mv_driver_name,
        .proc_info                   =  mv_linux_proc_info,
        .queuecommand                =  mv_linux_queue_command,
#ifdef IOCTL_TEMPLATE
        .ioctl				= mv_ial_ht_ioctl,
#endif
#if 0
        .eh_abort_handler            =  mv_linux_abort,
        .eh_device_reset_handler     =  mv_linux_reset,
        .eh_bus_reset_handler        =  mv_linux_reset,
#endif /* 0 */
        .eh_host_reset_handler       =  mv_linux_reset,
#if  LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7) && \
     LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
        .eh_timed_out                =  mv_linux_timed_out,
#endif
        .can_queue                   =  MV_MAX_REQUEST_DEPTH,
        .this_id                     =  MV_SHT_THIS_ID,
        .max_sectors                 =  MV_MAX_TRANSFER_SECTOR,
        .sg_tablesize                =  MV_MAX_SG_ENTRY,
        .cmd_per_lun                 =  MV_MAX_REQUEST_PER_LUN,
        .use_clustering              =  MV_SHT_USE_CLUSTERING,
	.support_scattered_spinup    =  1,
        .emulated                    =  MV_SHT_EMULATED,
};

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 15)
static struct scsi_transport_template mv_transport_template = {
	.eh_timed_out   =  mv_linux_timed_out,
};
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16) */


/* module management & hba module code */
extern struct mv_module_ops *mv_core_register_module(void);
extern struct mv_module_ops *mv_hba_register_module(void);

#ifdef RAID_DRIVER
extern struct mv_module_ops *mv_raid_register_module(void);
#else
static inline struct mv_module_ops *mv_raid_register_module(void)
{
	return NULL;
}
#endif /* RAID_DRIVER */

#ifdef CACHE_MODULE_SUPPORT
extern struct mv_module_ops *mv_cache_register_module(void);
#else
static inline struct mv_module_ops *mv_cache_register_module(void)
{
	return NULL;
}
#endif /* CACHE_DRIVER */

static LIST_HEAD(mv_online_adapter_list);

int __mv_get_adapter_count(void)
{
	struct mv_adp_desc *p;
	int i = 0;
	
	list_for_each_entry(p, &mv_online_adapter_list, hba_entry)
		i++;
	
	return i;
}

 inline struct mv_adp_desc *__dev_to_desc(struct pci_dev *dev)
{
	struct mv_adp_desc *p;
	
	list_for_each_entry(p, &mv_online_adapter_list, hba_entry)
		if (p->dev == dev)
			return p;
	return NULL;
}

MV_PVOID *mv_get_hba_extension(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;

	list_for_each_entry(p, &hba_desc->online_module_list, mod_entry)
		if (MODULE_HBA == p->module_id)
			return p->extension;
	return NULL;
}

static inline void __mv_release_hba(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc, *p;
	
	list_for_each_entry_safe(mod_desc, 
				 p, 
				 &hba_desc->online_module_list,
				 mod_entry) {
		list_del(&mod_desc->mod_entry);
		vfree(mod_desc);
	}
	
	list_del(&hba_desc->hba_entry);
	vfree(hba_desc);
}

static struct mv_adp_desc *mv_hba_init_modmm(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;
	
	hba_desc = vmalloc(sizeof(struct mv_adp_desc));
	if (NULL == hba_desc) {
		printk("Unable to get memory at hba init.\n");
		return NULL;
	}
	memset(hba_desc, 0, sizeof(struct mv_adp_desc));
	
	INIT_LIST_HEAD(&hba_desc->online_module_list);
	hba_desc->dev = dev;
	list_add(&hba_desc->hba_entry, &mv_online_adapter_list);
	
	return hba_desc;
}

static void mv_hba_release_modmm(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;
	
	hba_desc = __dev_to_desc(dev);
	
	if (hba_desc)
		__mv_release_hba(hba_desc);
	else
		printk("Weired! dev %p unassociated with any desc.\n", dev);
}

static inline struct mv_mod_desc *__alloc_mod_desc(void)
{
	struct mv_mod_desc *desc;

	desc = vmalloc(sizeof(struct mv_mod_desc));
	if (desc)
		memset(desc, 0, sizeof(struct mv_mod_desc));
	return desc;
}

static int register_online_modules(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc, *prev;
	struct mv_module_ops *ops;
	
	/*
	 * iterate through online_module_list manually , from the lowest(CORE)
	 * to the highest layer (HBA)
	 */
	hba_desc->running_mod_num = 0;
	/* CORE */
	ops = mv_core_register_module();
	if (NULL == ops) {
		printk("No core no life.\n");
		return -1;
	}
	mod_desc = __alloc_mod_desc();
	if (NULL == mod_desc)
		goto disaster;

	mod_desc->hba_desc  = hba_desc;
	mod_desc->ops       = ops;
	mod_desc->status    = MV_MOD_REGISTERED;
	mod_desc->module_id = MODULE_CORE;
	mod_desc->child     = NULL;
	list_add(&mod_desc->mod_entry, &hba_desc->online_module_list);
	hba_desc->running_mod_num++;

#ifdef ODIN_DRIVER
	/* when running in non-RAID, both CACHE and RAID must be disabled */
	if(!hba_desc->RunAsNonRAID)
	{
#endif

#ifdef RAID_DRIVER
	/* RAID */
	ops = mv_raid_register_module();
	if (ops) {
		prev = mod_desc;
		mod_desc = __alloc_mod_desc();
		if (NULL == mod_desc)
			goto disaster;

		mod_desc->hba_desc  = hba_desc;
		mod_desc->ops       = ops;
		mod_desc->status    = MV_MOD_REGISTERED;
		mod_desc->module_id = MODULE_RAID;
		mod_desc->child     = prev;
		prev->parent        = mod_desc;
		list_add(&mod_desc->mod_entry, &hba_desc->online_module_list);
		hba_desc->running_mod_num++;
	}
#endif

#ifdef CACHE_MODULE_SUPPORT
	/* CACHE */
	ops = mv_cache_register_module();
	if (ops) {
		prev = mod_desc;
		mod_desc = __alloc_mod_desc();
		if (NULL == mod_desc)
			goto disaster;
		
		mod_desc->hba_desc  = hba_desc;
		mod_desc->ops       = ops;
		mod_desc->status    = MV_MOD_REGISTERED;
		mod_desc->module_id = MODULE_CACHE;
		mod_desc->child     = prev;
		prev->parent        = mod_desc;
		list_add(&mod_desc->mod_entry, &hba_desc->online_module_list);
		hba_desc->running_mod_num++;
	}
#endif

#ifdef ODIN_DRIVER
	}
#endif

	/* HBA */
	prev = mod_desc;
	mod_desc = __alloc_mod_desc();
	if (NULL == mod_desc)
		goto disaster;

	mod_desc->ops = mv_hba_register_module();
	if (NULL == mod_desc->ops) {
		printk("No HBA no life.\n");
		return -1;
	}

	mod_desc->hba_desc  = hba_desc;
	mod_desc->status    = MV_MOD_REGISTERED;
	mod_desc->module_id = MODULE_HBA;
	mod_desc->child     = prev;
	mod_desc->parent    = NULL;
	prev->parent        = mod_desc;
	list_add(&mod_desc->mod_entry, &hba_desc->online_module_list);
	hba_desc->running_mod_num++;
	
	return 0;
disaster:
	return -1;
}


static void __release_consistent_mem(struct mv_mod_res *mod_res,
				     struct pci_dev *dev)
{
	dma_addr_t       dma_addr;
	MV_PHYSICAL_ADDR phy_addr;

	phy_addr = mod_res->bus_addr;
	dma_addr = (dma_addr_t) (phy_addr.parts.low |
				 ((u64) phy_addr.parts.high << 32));
	pci_free_consistent(dev,
			    mod_res->size,
			    mod_res->virt_addr,
			    dma_addr);
}

static int __alloc_consistent_mem(struct mv_mod_res *mod_res,
				  struct pci_dev *dev)
{
	unsigned long size;
	dma_addr_t    dma_addr;
	BUS_ADDRESS   bus_addr;
	MV_PHYSICAL_ADDR phy_addr;

	size = mod_res->size;
	WARN_ON(size != ROUNDING(size, 8));
	size = ROUNDING(size, 8);
	mod_res->virt_addr = (MV_PVOID) pci_alloc_consistent(dev,
							     size,
							     &dma_addr);
	if (NULL == mod_res->virt_addr) {
		MV_DBG(DMSG_HBA, "unable to alloc 0x%lx consistent mem.\n",
		       size);
		return -1;
	}
	memset(mod_res->virt_addr, 0, size);
	bus_addr            = (BUS_ADDRESS) dma_addr;
	phy_addr.parts.low  = LO_BUSADDR(bus_addr);
	phy_addr.parts.high = HI_BUSADDR(bus_addr);
	mod_res->bus_addr   = phy_addr;

	return 0;
}

int HBA_GetResource(struct mv_mod_desc *mod_desc,
		    enum Resource_Type type,
		    MV_U32  size,
		    Assigned_Uncached_Memory *dma_res)
{
	struct mv_mod_res *mod_res;

	mod_res = vmalloc(sizeof(struct mv_mod_res));
	if (NULL == mod_res) {
		printk("unable to allocate memory for resource management.\n");
		return -1;
	}

	memset(mod_res, 0, sizeof(struct mv_mod_res));
	mod_res->size = size;
	mod_res->type = type;
	switch (type) {
	case RESOURCE_UNCACHED_MEMORY :
		if (__alloc_consistent_mem(mod_res, mod_desc->hba_desc->dev)) {
			printk("unable to allocate 0x%x uncached mem.\n", size);
			vfree(mod_res);
			return -1;
		}
		list_add(&mod_res->res_entry, &mod_desc->res_list);
		memset(mod_res->virt_addr, 0, size);
		dma_res->Virtual_Address  = mod_res->virt_addr;
		dma_res->Physical_Address = mod_res->bus_addr;
		dma_res->Byte_Size        = size;
		break;
	case RESOURCE_CACHED_MEMORY :
	default:
		vfree(mod_res);
		printk("unknown resource type %d.\n", type);
		return -1;
	}
	return 0;
}

static void __release_resource(struct mv_adp_desc *hba_desc,
			       struct mv_mod_desc *mod_desc)
{
	struct mv_mod_res *mod_res, *tmp;

	list_for_each_entry_safe(mod_res,
				 tmp,
				 &mod_desc->res_list,
				 res_entry) {
		switch (mod_res->type) {
		case RESOURCE_UNCACHED_MEMORY :
			__release_consistent_mem(mod_res, hba_desc->dev);
			break;
		case RESOURCE_CACHED_MEMORY :
			vfree(mod_res->virt_addr);
			break;
		default:
			MV_DBG(DMSG_HBA, "res type %d unknown.\n",
			       mod_res->type);
			break;
		}
		list_del(&mod_res->res_entry);
		vfree(mod_res);
	}
}

static void __release_module_resource(struct mv_mod_desc *mod_desc)
{
	__release_resource(mod_desc->hba_desc, mod_desc);
}

static int __alloc_module_resource(struct mv_mod_desc *mod_desc,
				   unsigned int max_io)
{
	struct mv_mod_res *mod_res = NULL;
	unsigned int size = 0;
	
	/*
	 * alloc only cached mem at this stage, uncached mem will be alloc'ed 
	 * during mod init.
	 */
	INIT_LIST_HEAD(&mod_desc->res_list);
	mod_res = vmalloc(sizeof(struct mv_mod_res));
	if (NULL == mod_res)
		return -1;
	memset(mod_res, 0, sizeof(sizeof(struct mv_mod_res)));
	mod_desc->res_entry = 1;
	
	size = mod_desc->ops->get_res_desc(RESOURCE_CACHED_MEMORY, max_io);
	if (size) {
		mod_res->virt_addr = vmalloc(size);
		if (NULL == mod_res->virt_addr) {
			vfree(mod_res);
			return -1;
		}
		memset(mod_res->virt_addr, 0, size);
		mod_res->type                = RESOURCE_CACHED_MEMORY;
		mod_res->size                = size;
		mod_desc->extension          = mod_res->virt_addr;
		mod_desc->extension_size     = size;
		list_add(&mod_res->res_entry, &mod_desc->res_list);
	}

	return 0;
}

static void mv_release_module_resource(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;

	list_for_each_entry(mod_desc, &hba_desc->online_module_list, 
			    mod_entry) {
		if (mod_desc->status == MV_MOD_INITED) {
			__release_module_resource(mod_desc);
			mod_desc->status = MV_MOD_REGISTERED;
		}
	}
}

static int mv_alloc_module_resource(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;
	int ret;

	list_for_each_entry(mod_desc, &hba_desc->online_module_list, 
			    mod_entry) {
		ret = __alloc_module_resource(mod_desc, hba_desc->max_io);
		if (ret)
			goto err_out;
		mod_desc->status = MV_MOD_INITED;
	}
	return 0;

err_out:
	MV_DBG(DMSG_HBA, "error %d allocating resource for mod %d.\n",
	       ret, mod_desc->module_id);
	list_for_each_entry(mod_desc, &hba_desc->online_module_list, 
			    mod_entry) {
		if (mod_desc->status == MV_MOD_INITED) {
			__release_module_resource(mod_desc);
			mod_desc->status = MV_MOD_REGISTERED;
		}
	}
	return -1;
}



static inline struct mv_mod_desc *
__get_highest_module(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;
	
	/* take a random module, and trace through its parent */
	p = list_entry(hba_desc->online_module_list.next, 
		       struct mv_mod_desc,
		       mod_entry);

	WARN_ON(NULL == p);
	while (p) {
		if (NULL == p->parent)
			break;
		p = p->parent;
	}
	return p;
}

static void __map_pci_addr(struct pci_dev *dev, MV_PVOID *addr_array)
{
	int i;
	unsigned long addr;
	unsigned long range;

	for (i = 0; i < MAX_BASE_ADDRESS; i++) {
		addr  = pci_resource_start(dev, i);
		range = pci_resource_len(dev, i);

		if (pci_resource_flags(dev, i) & IORESOURCE_MEM)
			addr_array[i] =(MV_PVOID) ioremap(addr, range);
		else if (pci_resource_flags(dev, i) & IORESOURCE_IO)
			addr_array[i] = (MV_PVOID) ioport_map(addr, range);
			

		MV_DBG(DMSG_HBA, "BAR %d : %p.\n", 
		       i, addr_array[i]);
	}
}

static void __unmap_pci_addr(struct pci_dev *dev, MV_PVOID *addr_array)
{
	int i;
	
	for (i = 0; i < MAX_BASE_ADDRESS; i++) 
		if (pci_resource_flags(dev, i) & IORESOURCE_MEM)
                        iounmap(addr_array[i]);
		else if (pci_resource_flags(dev, i) & IORESOURCE_IO)
			ioport_unmap(addr_array[i]);
}

int __mv_is_mod_all_started(struct mv_adp_desc *adp_desc)
{
	struct mv_mod_desc *mod_desc;
	
	mod_desc = __get_lowest_module(adp_desc);

	while (mod_desc) {
		if (MV_MOD_STARTED != mod_desc->status)
			return 0;

		mod_desc = mod_desc->parent;
	}
	return 1;
}

struct hba_extension *__mv_get_ext_from_adp_id(int id)
{
	struct mv_adp_desc *p;

	list_for_each_entry(p, &mv_online_adapter_list, hba_entry)
		if (p->id == id)
			return __get_highest_module(p)->extension;
	
	return NULL;
}

int mv_hba_start(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;
	struct mv_mod_desc *mod_desc;

	hba_desc = __dev_to_desc(dev);
	BUG_ON(NULL == hba_desc);
#if 0
	mod_desc = __get_lowest_module(hba_desc);
	if (NULL == mod_desc)
		return -1;
	MV_DPRINT(("Start lowest module.\n"));

#ifdef THOR_DRIVER
	spin_lock_irq(&hba_desc->global_lock);
#endif

	mod_desc->ops->module_start(mod_desc->extension);

#ifdef THOR_DRIVER
	spin_unlock_irq(&hba_desc->global_lock);
#endif
#endif
	MV_DPRINT(("Start highest module.\n"));
	mod_desc = __get_highest_module(hba_desc);
	if (NULL == mod_desc)
		return -1;

	mod_desc->ops->module_start(mod_desc->extension);

	return 0;
}

static void __hba_module_stop(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;

	mod_desc = __get_highest_module(hba_desc);
	if (NULL == mod_desc)
		return;

	/* starting from highest module, unlike module_start */
	while (mod_desc) {
		if ((MV_MOD_STARTED == mod_desc->status)) {
			mod_desc->ops->module_stop(mod_desc->extension);
			mod_desc->status = MV_MOD_INITED;
		}
		mod_desc = mod_desc->child;
	}
}

/* stop all HBAs if dev == NULL */
void mv_hba_stop(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;

	hba_wait_eh();

	if (dev) {
		hba_desc = __dev_to_desc(dev);
		__hba_module_stop(hba_desc);
	} else {
		list_for_each_entry(hba_desc, &mv_online_adapter_list, hba_entry)
			__hba_module_stop(hba_desc);
	}
}

void mv_hba_release(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;

	hba_desc = __dev_to_desc(dev);
	if (NULL != hba_desc) {
		__unmap_pci_addr(hba_desc->dev, hba_desc->Base_Address);
		mv_release_module_resource(hba_desc);
		mv_hba_release_modmm(hba_desc->dev);
	}
}

int mv_hba_init(struct pci_dev *dev, MV_U32 max_io)
{
	struct mv_adp_desc *hba_desc;
	struct mv_mod_desc *mod_desc;
	int    dbg_ret = 0;
	
	hba_desc = mv_hba_init_modmm(dev);
	if (NULL == hba_desc)
		goto ext_err_init;


	hba_desc->max_io = max_io;
	hba_desc->id     = __mv_get_adapter_count() - 1;
	
	if (pci_read_config_byte(hba_desc->dev, 
				 PCI_REVISION_ID, 
				 &hba_desc->Revision_Id)) {
		MV_PRINT("Failed to get hba's revision id.\n");
		goto ext_err_pci;
	}
	
	hba_desc->vendor = dev->vendor;
	hba_desc->device = dev->device;
	hba_desc->subsystem_vendor = dev->subsystem_vendor;
	hba_desc->subsystem_device = dev->subsystem_device;

	__map_pci_addr(dev, hba_desc->Base_Address);
	
	spin_lock_init(&hba_desc->lock);
	spin_lock_init(&hba_desc->global_lock);

	/* For Odin family, read PCIE configuration space register bit 31:25 [PAD_TEST] (offset=0x60) */
	/* to identify different part */
#ifdef ODIN_DRIVER
	if (hba_desc->device != DEVICE_ID_6480) {
		MV_U32 padTest = 0;
		pci_read_config_dword(dev, 0x60, &padTest);
		hba_desc->device = SetDeviceID(padTest);
		hba_desc->subsystem_device = hba_desc->device;
		if (hba_desc->device == DEVICE_ID_6445)
			hba_desc->RunAsNonRAID = MV_TRUE;

		MV_DBG(DMSG_HBA, "controller device id 0x%x.\n", 
	       hba_desc->device);
		
	}
#endif



	MV_DBG(DMSG_HBA, "HBA ext struct init'ed at %p.\n", 
	        hba_desc);

	if (register_online_modules(hba_desc))
		goto ext_err_modmm;

	if (mv_alloc_module_resource(hba_desc))
		goto ext_err_modmm;

	mod_desc = __get_lowest_module(hba_desc);
	if (NULL == mod_desc)
		goto ext_err_pci;

	while (mod_desc) {
		if (MV_MOD_INITED != mod_desc->status)
			continue;
		mod_desc->ops->module_initialize(mod_desc,
						 mod_desc->extension_size,
						 hba_desc->max_io);
		/* there's no support for sibling module at the moment  */
		mod_desc = mod_desc->parent;
	}
	return 0;

ext_err_pci:
	++dbg_ret;
	mv_release_module_resource(hba_desc);
ext_err_modmm:
	++dbg_ret;
	mv_hba_release_modmm(dev);
ext_err_init:
        ++dbg_ret;
	return dbg_ret;
}

static MV_U32 HBA_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo)
{
	MV_U32 size = 0;

	if (type == RESOURCE_CACHED_MEMORY) {
		/* Fixed memory */
		size = OFFSET_OF(HBA_Extension, Memory_Pool);
		size = ROUNDING(size, 8);

#ifdef SUPPORT_EVENT
		size += sizeof(Driver_Event_Entry) * MAX_EVENTS;
		MV_ASSERT(size == ROUNDING(size, 8));
#endif /* SUPPORT_EVENT */
	}

	return size;
}

#ifdef SUPPORT_TASKLET
MV_BOOLEAN Core_TaskletHandler(MV_PVOID This);
static void run_tasklet(PHBA_Extension   phba)
{
	struct mv_mod_desc *core_desc=__get_lowest_module(phba->desc->hba_desc);
	unsigned long flags;
	MV_PVOID pcore=core_desc->extension;
	spin_lock_irqsave(&phba->desc->hba_desc->global_lock, flags);
	Core_TaskletHandler(pcore);
	spin_unlock_irqrestore(&phba->desc->hba_desc->global_lock, flags);
}
#endif

static void HBA_ModuleInitialize(MV_PVOID this,
				 MV_U32   size,
				 MV_U16   max_io)
{
	struct   mv_mod_desc *mod_desc=(struct   mv_mod_desc *)this;
	PHBA_Extension phba;
	MV_PTR_INTEGER temp;
	MV_U32 i;
	MV_U32 sg_num;

#ifdef SUPPORT_EVENT
	PDriver_Event_Entry pEvent = NULL;
#endif /* SUPPORT_EVENT */
    
	phba     = (PHBA_Extension) mod_desc->extension;
	temp     = (MV_PTR_INTEGER) phba->Memory_Pool;

	WARN_ON(sizeof(MV_Request) != ROUNDING(sizeof(MV_Request), 8));
	/* 
	 * Initialize data structure however following variables have been 
	 * set already.
	 *	Device_Extension
	 *	Is_Dump
	 */
	phba->State    = DRIVER_STATUS_IDLE;
	phba->Io_Count = 0;
	phba->Max_Io   = max_io;
	phba->desc     = mod_desc;

	init_completion(&phba->cmpl);
	temp = ROUNDING(((MV_PTR_INTEGER) temp), 8);

	if (max_io > 1)
		sg_num   = MAX_SG_ENTRY;
	else
		sg_num   = MAX_SG_ENTRY_REDUCED;

	phba->req_pool = (MV_PVOID) res_reserve_req_pool(MODULE_HBA,
							 max_io,
							 sg_num);
	BUG_ON(NULL == phba->req_pool);
#ifdef THOR_DRIVER
	//spin_lock_init(&phba->lock);
	init_timer(&phba->timer);
#endif

#ifdef SUPPORT_TASKLET
	tasklet_init(&phba->mv_tasklet,
			(void (*)(unsigned long))run_tasklet, (unsigned long)phba);
#endif

#ifdef SUPPORT_EVENT
	INIT_LIST_HEAD(&phba->Stored_Events);
	INIT_LIST_HEAD(&phba->Free_Events);
	phba->Num_Stored_Events = 0;
	phba->SequenceNumber = 0;	/* Event sequence number */

	MV_ASSERT(sizeof(Driver_Event_Entry) == 
		  ROUNDING(sizeof(Driver_Event_Entry), 8));
	temp = ROUNDING(((MV_PTR_INTEGER) temp), 8);

	for (i = 0; i < MAX_EVENTS; i++) {
		pEvent = (PDriver_Event_Entry) temp;
		list_add_tail(&pEvent->Queue_Pointer, &phba->Free_Events);
		temp += sizeof(Driver_Event_Entry);
	}
#endif /* SUPPORT_EVENT */

}

static void HBA_ModuleStart(MV_PVOID extension)
{
	struct Scsi_Host *host = NULL;
	struct hba_extension *hba;
	int ret;

	struct mv_adp_desc *adp_desc;
	struct mv_mod_desc *core_desc;

	hba = (struct hba_extension *) extension;
	host = scsi_host_alloc(&mv_driver_template, sizeof(void *));
	if (NULL == host) {
		MV_PRINT("Unable to allocate a scsi host.\n" );
		goto err_out;
	}

	*((MV_PVOID *) host->hostdata) = extension;
	hba->host = host;
	hba->dev  = hba->desc->hba_desc->dev;

	host->irq          = hba->dev->irq;
	host->max_id       = MV_MAX_TARGET_NUMBER;
	host->max_lun      = MV_MAX_LUN_NUMBER;
	host->max_channel  = 0;
	host->max_cmd_len  = 16;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 15)
	host->transportt   = &mv_transport_template;
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16) */

#ifdef USE_MSI
	pci_enable_msi(hba->dev);
#endif /* USE_MSI */

	MV_DBG(DMSG_KERN, "start install request_irq.\n");
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
	ret = request_irq(hba->dev->irq, mv_intr_handler, IRQF_SHARED,
			  mv_driver_name, hba);
#else
	ret = request_irq(hba->dev->irq, mv_intr_handler, SA_SHIRQ,
			  mv_driver_name, hba);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19) */
	if (ret < 0) {
		MV_PRINT("Error upon requesting IRQ %d.\n", 
		       hba->dev->irq);
		goto  err_request_irq;
	}
	MV_DBG(DMSG_KERN, "request_irq has been installed.\n");

	hba->desc->status = MV_MOD_STARTED;

	/* To start CORE layer */
	adp_desc = hba->desc->hba_desc;
	core_desc = __get_lowest_module(adp_desc);
	if (NULL == core_desc) {
		goto err_wait_cmpl;
	}

	core_desc->ops->module_start(core_desc->extension);
	

	HBA_ModuleStarted(hba->desc);

	MV_DBG(DMSG_KERN, "wait_for_completion_timeout.....\n");
	/* wait for MODULE(CORE,RAID,HBA) init */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->hba_sync, 1);
	if (0 == __hba_wait_for_atomic_timeout(&hba->hba_sync, 30 * HZ)) 
		goto err_wait_cmpl;
#else
	if (0 == wait_for_completion_timeout(&hba->cmpl, 30 * HZ)) 
		goto err_wait_cmpl;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */

	hba_house_keeper_run();

	if (scsi_add_host(host, &hba->dev->dev))
		goto err_wait_cmpl;

	MV_DPRINT(("Start scsi_scan_host.\n"));
	
	scsi_scan_host(host);

	if (mv_register_chdev(hba))
		printk("Unable to register character device interface.\n");

	
	MV_DPRINT(("Finished HBA_ModuleStart.\n"));

	return;
err_wait_cmpl:
	printk("Timeout waiting for module start.\n");
	free_irq(hba->dev->irq, hba);
err_request_irq:
	scsi_host_put(host);
	hba->host = NULL;
err_out:
	return;
}

extern void hba_send_shutdown_req(PHBA_Extension phba);
static void HBA_ModuleShutdown(MV_PVOID extension)
{
	PHBA_Extension hba = (PHBA_Extension) extension;

	mv_unregister_chdev(hba);

	if (DRIVER_STATUS_STARTED == hba->State) {
		scsi_remove_host(hba->host);
		scsi_host_put(hba->host);
		hba->host = NULL;

		hba_send_shutdown_req(hba);
		while(hba->Io_Count != 0)
			mdelay(1);
		free_irq(hba->dev->irq, hba);
	}
	res_release_req_pool(hba->req_pool);
	hba->State = DRIVER_STATUS_SHUTDOWN;
}

#ifdef SUPPORT_EVENT
static MV_BOOLEAN add_event(IN MV_PVOID extension,
			    IN MV_U32 eventID,
			    IN MV_U16 deviceID,
			    IN MV_U8 severityLevel,
			    IN MV_U8 param_cnt,
			    IN MV_PVOID params)
{
	struct hba_extension * hba = (struct hba_extension *) extension;
	PDriver_Event_Entry pEvent;
	static MV_U32 sequenceNo = 1;
	if (param_cnt > MAX_EVENT_PARAMS)
		return MV_FALSE;

	if (list_empty(&hba->Free_Events)) {
		/* No free entry, we need to reuse the oldest entry from 
		 * Stored_Events.
		 */
		MV_ASSERT(!list_empty(&hba->Stored_Events));
		MV_ASSERT(hba->Num_Stored_Events == MAX_EVENTS);
		pEvent = List_GetFirstEntry((&hba->Stored_Events), Driver_Event_Entry, Queue_Pointer);
	}
	else
	{
		pEvent = List_GetFirstEntry((&hba->Free_Events), Driver_Event_Entry, Queue_Pointer);
		hba->Num_Stored_Events++;
		MV_ASSERT(hba->Num_Stored_Events <= MAX_EVENTS);
	}

	pEvent->Event.AdapterID  = hba->desc->hba_desc->id;
	pEvent->Event.EventID    = eventID; 
	pEvent->Event.SequenceNo = sequenceNo++;
	pEvent->Event.Severity   = severityLevel;
	pEvent->Event.DeviceID   = deviceID;
//	pEvent->Event.Param_Cnt  = param_cnt;
	pEvent->Event.TimeStamp  = ossw_get_time_in_sec();

	if (param_cnt > 0 && params != NULL)
		MV_CopyMemory( (MV_PVOID)pEvent->Event.Params, (MV_PVOID)params, param_cnt * 4 );

	list_add_tail(&pEvent->Queue_Pointer, &hba->Stored_Events);

	return MV_TRUE;
}

static void get_event(MV_PVOID This, PMV_Request pReq)
{
	struct hba_extension * hba = (struct hba_extension *) This;
	PEventRequest pEventReq = (PEventRequest)pReq->Data_Buffer;
	PDriver_Event_Entry pfirst_event;
	MV_U8 count = 0;

	pEventReq->Count = 0;
	
	if ( hba->Num_Stored_Events > 0 )
	{	
		MV_DASSERT( !list_empty(&hba->Stored_Events) );
		while (!list_empty(&hba->Stored_Events) &&
		       (count < MAX_EVENTS_RETURNED)) {
			pfirst_event = List_GetFirstEntry((&hba->Stored_Events), Driver_Event_Entry, Queue_Pointer);
			MV_CopyMemory(&pEventReq->Events[count], 
				      &pfirst_event->Event, 
				      sizeof(DriverEvent));
			hba->Num_Stored_Events--;
			list_add_tail(&pfirst_event->Queue_Pointer, 
				      &hba->Free_Events );
			count++;
		}
		pEventReq->Count = count;
	}

	pReq->Scsi_Status = REQ_STATUS_SUCCESS;
	return;
}
#else /* SUPPORT_EVENT */
static inline MV_BOOLEAN add_event(IN MV_PVOID extension,
				   IN MV_U32 eventID,
				   IN MV_U16 deviceID,
				   IN MV_U8 severityLevel,
				   IN MV_U8 param_cnt,
				   IN MV_PVOID params) {}

static inline void get_event(MV_PVOID This, PMV_Request pReq) {}
#endif /* SUPPORT_EVENT */


void HBA_ModuleNotification(MV_PVOID This, 
			     enum Module_Event event, 
			     struct mod_notif_param *event_param)
{
	/* "This" passed in is not hba extension, it is caller's extension */
	/* has to find own extension like the implementation in HBA */
	MV_PVOID hba = HBA_GetModuleExtension(This, MODULE_HBA);	
	MV_DPRINT(("Enter HBA_ModuleNotification event %x\n",event));
//	MV_POKE();
	switch (event) {
	case EVENT_LOG_GENERATED:
		add_event(hba,
			  event_param->event_id,
			  event_param->dev_id,
			  event_param->severity_lvl,
			  event_param->param_count,
			  event_param->p_param);
		break;
	case EVENT_DEVICE_REMOVAL:
	case EVENT_DEVICE_ARRIVAL:
		hba_msg_insert(hba,
			       event,
			       (event_param == NULL)?0:event_param->lo);
		break;
	case EVENT_HOT_PLUG:
		hba_msg_insert(event_param->p_param,
			       event,
			       event_param->event_id);
		break;
	default:
		break;
	}
}
#if 1//def SUPPORT_SCSI_PASSTHROUGH

/* helper functions related to HBA_ModuleSendRequest */
static void mvGetAdapterInfo( MV_PVOID This, PMV_Request pReq )
{
	PHBA_Extension hba = (PHBA_Extension)This;
	struct mv_adp_desc *pHBA=hba->desc->hba_desc;
	PAdapter_Info pAdInfo;
#ifdef ODIN_DRIVER
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)HBA_GetModuleExtension(This,MODULE_CORE);
#endif
	/* initialize */
	pAdInfo = (PAdapter_Info)pReq->Data_Buffer;
	MV_ZeroMemory(pAdInfo, sizeof(Adapter_Info));


	pAdInfo->DriverVersion.VerMajor = VER_MAJOR;
	pAdInfo->DriverVersion.VerMinor = VER_MINOR;
	pAdInfo->DriverVersion.VerOEM = VER_OEM;
	pAdInfo->DriverVersion.VerBuild = VER_BUILD;

	pAdInfo->SystemIOBusNumber = pHBA->Adapter_Bus_Number;
	pAdInfo->SlotNumber = pHBA->Adapter_Device_Number;
	pAdInfo->VenID = pHBA->vendor;
	pAdInfo->DevID = pHBA->device;
	pAdInfo->SubDevID = pHBA->subsystem_device;
	pAdInfo->SubVenID = pHBA->subsystem_vendor;
	pAdInfo->RevisionID = pHBA->Revision_Id;

	if ( pHBA->device == DEVICE_ID_THORLITE_2S1P||pHBA->device == DEVICE_ID_THORLITE_2S1P_WITH_FLASH ){
		pAdInfo->PortCount = 3;
	}else if ( pHBA->device == DEVICE_ID_THORLITE_0S1P ){
		pAdInfo->PortCount = 1;
	}else if ( (pHBA->device == DEVICE_ID_6440) || 
			   (pHBA->device == DEVICE_ID_6445) ||
			   (pHBA->device == DEVICE_ID_6340) ){
		pAdInfo->PortCount = 4;
	}else if ( pHBA->device == DEVICE_ID_6480 ){
		pAdInfo->PortCount = 8;	
	}else if ( pHBA->device == DEVICE_ID_6320 ){
		pAdInfo->PortCount = 2;
	}else {
		pAdInfo->PortCount = 5;
	}

//	pAdInfo->MaxHD = pCore->PD_Count_Supported;
#ifdef SIMULATOR
	pAdInfo->MaxHD = MAX_DEVICE_SUPPORTED_PERFORMANCE;
	pAdInfo->MaxExpander = 0;
#elif defined(ODIN_DRIVER)
	pAdInfo->MaxHD = pCore->PD_Count_Supported;
	pAdInfo->MaxExpander = pCore->Expander_Count_Supported;
#else
	pAdInfo->MaxHD = 8;
	pAdInfo->MaxExpander = MAX_EXPANDER_SUPPORTED;
#endif

	pAdInfo->MaxPM = MAX_PM_SUPPORTED;				// 4 for now
#ifdef RAID_DRIVER
	raid_capability(This,  pAdInfo);
#endif	/* RAID_DRIVER */
	pReq->Scsi_Status = REQ_STATUS_SUCCESS;
}

#endif


static void HBA_ModuleSendRequest(MV_PVOID this, PMV_Request req)
{
	PHBA_Extension phba = (PHBA_Extension) this;

	switch (req->Cdb[0])
	{
	case APICDB0_ADAPTER:
		if (req->Cdb[1] == APICDB1_ADAPTER_GETINFO)
			mvGetAdapterInfo(phba, req);
		else
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;

		req->Completion(req->Cmd_Initiator, req);
		break;
	case APICDB0_EVENT:
		if (req->Cdb[1] == APICDB1_EVENT_GETEVENT)
			get_event(phba, req);
		else
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;

		req->Completion(req->Cmd_Initiator, req);
		break;
	default:
		/* submit to child layer */
		phba->desc->child->ops->module_sendrequest(
			phba->desc->child->extension,
			req);
		break;
	}
}

static struct mv_module_ops hba_module_interface = {
	.module_id              = MODULE_HBA,
	.get_res_desc           = HBA_ModuleGetResourceQuota,
	.module_initialize      = HBA_ModuleInitialize,
	.module_start           = HBA_ModuleStart,
	.module_stop            = HBA_ModuleShutdown,
	.module_notification    = HBA_ModuleNotification,
	.module_sendrequest     = HBA_ModuleSendRequest,
};

struct mv_module_ops *mv_hba_register_module(void)
{
	return &hba_module_interface;
}
