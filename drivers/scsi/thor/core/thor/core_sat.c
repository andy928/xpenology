#include "mv_include.h"
#include "core_sat.h"
#include "core_inter.h"
#include "core_thor.h"
#include "core_exp.h"
#include "core_sata.h"
#include "core_ata.h"
#include "core_init.h"

#ifdef SUPPORT_ATA_SMART

/***************************************************************************
* SCSI_ATA_CheckCondition
* Purpose: Send Check Condition when request is illegal
*
***************************************************************************/
void SCSI_ATA_CheckCondition(
	IN PMV_Request pReq,
	IN MV_U8 senseKey,
	IN MV_U8 senseCode
	)
{
	 pReq->Scsi_Status = SCSI_STATUS_CHECK_CONDITION;
	 if ( pReq->Sense_Info_Buffer ) {
		((MV_PU8)pReq->Sense_Info_Buffer)[2] = senseKey;
		((MV_PU8)pReq->Sense_Info_Buffer)[7] = 0;		// additional sense length 
		((MV_PU8)pReq->Sense_Info_Buffer)[12] = senseCode; //additional sense code
	 }
}

#define MAX_MODE_PAGE_LENGTH	28
MV_U32 Core_get_mode_page_caching(MV_PU8 pBuf, PDomain_Device pDevice)
{
	pBuf[0] = 0x08;		/* Page Code, PS = 0; */
	pBuf[1] = 0x12;		/* Page Length */
	/* set the WCE and RCD bit based on device identification data */
	if (pDevice->Setting & DEVICE_SETTING_WRITECACHE_ENABLED)
		pBuf[2] |= MV_BIT(2);
	pBuf[3] = 0;	/* Demand read/write retention priority */
	pBuf[4] = 0xff;	/* Disable pre-fetch trnasfer length (4,5) */
	pBuf[5] = 0xff;	/* all anticipatory pre-fetching is disabled */
	pBuf[6] = 0;	/* Minimum pre-fetch (6,7) */
	pBuf[7] = 0;
	pBuf[8] = 0;	/* Maximum pre-fetch (8,9) */
	pBuf[9] = 0x01;
	pBuf[10] = 0;	/* Maximum pre-fetch ceiling (10,11) */
	pBuf[11] = 0x01;
//	pBuf[12] |= MV_BIT(5);	/* How do I know if Read Ahead is enabled or disabled???  */
	pBuf[12] = 0x00;
	pBuf[13] = 0x01;	/* Number of cache segments */
	pBuf[14] = 0xff;	/* Cache segment size (14, 15) */
	pBuf[15] = 0xff;
	return 0x14;	/* Total page length in byte */
}

MV_U32 Core_Fill_ProSpecPortModePage(MV_PU8 pBuf)
{
	static const MV_U8 pro_spec_mode_page[] = {
		0x19,/*Protocol Specific Port mode page*/
		0x01,	/*Page Length*/
		0x08, /*protocol identify*/
		0,
	};
	MV_CopyMemory(pBuf, pro_spec_mode_page, sizeof(pro_spec_mode_page));
	return sizeof(pro_spec_mode_page);	/* Total page length in byte */
}

static const MV_U8 info_except_ctrl_mode_page[] = {
	0x1c,	/* Page Code, PS = 0; */
	0x0a,  /* Page Length */
	0xb7, /* Report Error  */
	0x04, /*set MRIE*/
	0, 0, 0, 0,
	0, 0, 0, 0
};

MV_U32 Core_Fill_InfoExcepCtrlModePage(MV_PU8 pBuf, PDomain_Device pDevice,PMV_Request pReq)
{
	MV_CopyMemory(pBuf, info_except_ctrl_mode_page, sizeof(info_except_ctrl_mode_page));
	
	// Setting Extended Self-test Completion time
	if(!(pDevice->Capacity & DEVICE_CAPACITY_SMART_SUPPORTED))	
	{
		SCSI_ATA_CheckCondition(pReq, SCSI_SK_ILLEGAL_REQUEST, 
				SCSI_ASC_INVALID_FEILD_IN_CDB);
	}
	return sizeof( info_except_ctrl_mode_page);	/* Total page length in byte */
}



MV_U32  Core_Fill_CtrlModePage(MV_PU8 pBuf, PDomain_Device pDevice)
{
	static const MV_U8 ctrl_mode_page[] = {
		0x0a,
		10,
		2,	/* DSENSE=0, GLTSD=1 */
		0,	/* [QAM+QERR may be 1, see 05-359r1] */
		0, 0, 0, 0, 0, 0,
		0x02, 0x4b	/* extended self test time, see 05-359r1 */
	};
	MV_CopyMemory(pBuf, ctrl_mode_page, sizeof(ctrl_mode_page));

	// Setting Extended Self-test Completion time
	if(pDevice->Capacity & DEVICE_CAPACITY_SMART_SELF_TEST_SUPPORTED)
	{
		pBuf[10] = 0;
		pBuf[11] = 30;
	} else {
		pBuf[10] = 0;
		pBuf[11] = 0;
	}
	return sizeof(ctrl_mode_page);
}

MV_U32  Core_Fill_RWErrorRecoveryModePage(MV_PU8 pBuf )
{
	static const MV_U8 rw_recovery_mdpage[] = {
		0x01,
		10,
		(1 << 7),	/* AWRE */
		0,		/* read retry count */
		0, 0, 0, 0,
		0,		/* write retry count */
		0, 0, 0
	};
	MV_CopyMemory(pBuf, rw_recovery_mdpage, sizeof(rw_recovery_mdpage));
	return sizeof(rw_recovery_mdpage);
}

MV_U32 Core_Fill_ErrorCounterLogPage(MV_PU8 pBuf)
{
	static const MV_U8 error_counter_log_pg[] = {
		0x03,	/* DU =0, DS =0, TSD =0, ETC=0, TMC=00, FL=11 */
		0x02, 	/* Parameter length */
		0x00 ,0x00
	};

	pBuf[0] = 0x00;	/* Parameter Code*/
	pBuf[1] = 0x06;/*00,01,02,03,04,05,06 Total uncorrected errors ?*/
       MV_CopyMemory(pBuf+2, error_counter_log_pg, sizeof(error_counter_log_pg));
       return sizeof(error_counter_log_pg)+2;
}

MV_U32 Core_Fill_SelfTestLogPage(MV_PU8 pBuf, MV_U8 pageNum)
{
	static const MV_U8 self_test_results_log_pg[] = {
		0x03,	/* DU =0, DS =0, TSD =0, ETC=0, TMC=00, FL=11 */
		0x10, 	/* Parameter length */
		0x81,   /*SELF-TEST CODE=100,RES=0,SELF-TEST RESULTS=0001*/
		0x08, /*self test number*/
		0x0, 0x0,0x0,0x0, 
		0x0, 0x0,0x0,0x0
	};

	pBuf[0] = 0;	/* Parameter Code */
	pBuf[1] = pageNum+1;
       MV_CopyMemory(pBuf+2, self_test_results_log_pg, sizeof(self_test_results_log_pg));
       return sizeof(self_test_results_log_pg)+2;
}

 MV_U32 Core_Fill_TempLogPage(MV_PU8 pBuf)
{
	static const MV_U8  temp_log_page[] = {
		0x0, 0x0, 
		0x3, 0x2, 0x0, 
		0xFF,/*TEMPERATURE*/
	       0x0, 0x1, 0x3, 0x2, 0x0,
	       0xFF /*REFERENCE TEMPERATURE*/
	};
        MV_CopyMemory(pBuf, temp_log_page, sizeof(temp_log_page));
        return sizeof(temp_log_page);
}

 MV_U32 Core_Fill_InfoExceptLogPage(MV_PU8 pBuf)
{
	static const MV_U8  info_except_log_page[] = {
		0x0, 0x0, /*parameter code*/
		0x3, /*DU=0 OBS=0 TSD=0 ETC=0 TMC=00 FL=11h*/
		0x3,/*parampter length*/
		0x0, 0x0, 
		38, /*MOST RECENT TEMPERATURE READING*/
	};

       MV_CopyMemory(pBuf, info_except_log_page, sizeof(info_except_log_page));
	if (info_except_ctrl_mode_page[2] & 0x4) {	/* TEST bit set */
		pBuf[4] = SCSI_ASC_FAILURE_PREDICTION_THRESHOLD_EXCEEDED;
		pBuf[5] = 0xff;
	}
        return sizeof(info_except_log_page);
}
 
 MV_U32 Core_Fill_StartStopCycleCounterLogPage(MV_PU8 pBuf)
{
	static const MV_U8  start_stop_cycle_counter_log_page[] = {
		0x00, 0x01,0x15,0x06,00,00,20,8,00,12,
		0x00, 0x02,0x16,0x06,00,00,20,8,00,12,
		0x00,0x03,0x17,0x04,00,00,00,10,
	       0x00, 0x04,0x18,0x04,00,00,00,02
	};
        MV_CopyMemory(pBuf, start_stop_cycle_counter_log_page, sizeof(start_stop_cycle_counter_log_page));
        return sizeof(start_stop_cycle_counter_log_page);
}


void  mvScsiLogSenseTranslation(PCore_Driver_Extension pCore,  PMV_Request pReq)
{
	MV_U8 pageCode,subpcode,ppc, sp, pcontrol;
	MV_U32 pageLen = 0, tmpLen = 0,i,n;

	unsigned char ptmpBuf[512];
	MV_U8 *buf = pReq->Data_Buffer;
	
	MV_ZeroMemory(buf, pReq->Data_Transfer_Length);
	MV_ZeroMemory(ptmpBuf, sizeof(ptmpBuf));
	
	ppc = pReq->Cdb[1] & 0x2;
	sp = pReq->Cdb[1] & 0x1;
	if (ppc || sp) {
		pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
		Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
		return ;
	}
	pcontrol = (pReq->Cdb[2] & 0xc0) >> 6;
	pageCode = pReq->Cdb[2] & 0x3f;
	subpcode = pReq->Cdb[3] & 0xff;
	
	ptmpBuf[0] = pageCode;
	if (0 == subpcode) {
		switch (pageCode) {
		case SUPPORTED_LPAGES:/* Supported log pages log page */
			n = 4;
			ptmpBuf[0] = SUPPORTED_LPAGES; /* Page Code */
			ptmpBuf[1] = 0x00; /*subpage Code */
			ptmpBuf[2] = 0x00;/*length MSB*/
			ptmpBuf[n++] = SUPPORTED_LPAGES;		/* this page */
			ptmpBuf[n++] = WRITE_ERROR_COUNTER_LPAGE;		
			ptmpBuf[n++] = READ_ERROR_COUNTER_LPAGE;		
			ptmpBuf[n++] = READ_REVERSE_ERROR_COUNTER_LPAGE;	
			ptmpBuf[n++] = VERIFY_ERROR_COUNTER_LPAGE;		
			ptmpBuf[n++] = SELFTEST_RESULTS_LPAGE;		
			ptmpBuf[n++] = TEMPERATURE_LPAGE;		
			
			ptmpBuf[n++] = STARTSTOP_CYCLE_COUNTER_LPAGE;	
		//	ptmpBuf[n++] = SEAGATE_CACHE_LPAGE;	
		//	ptmpBuf[n++] = SEAGATE_FACTORY_LPAGE;	
												
			ptmpBuf[n++] = IE_LPAGE;	/* Informational exceptions */	
			ptmpBuf[3] = n - 4; /*length LSB*/
			pageLen = ptmpBuf[3] ;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+4));
			MV_CopyMemory(buf, ptmpBuf, tmpLen);
			pReq->Data_Transfer_Length = tmpLen;
			pReq->Scsi_Status = REQ_STATUS_SUCCESS;
			break;
		case WRITE_ERROR_COUNTER_LPAGE:
		case READ_ERROR_COUNTER_LPAGE:
		case READ_REVERSE_ERROR_COUNTER_LPAGE:
		case VERIFY_ERROR_COUNTER_LPAGE:
			if (pageCode == WRITE_ERROR_COUNTER_LPAGE)
				ptmpBuf[0] = WRITE_ERROR_COUNTER_LPAGE; // Page Code
			if (pageCode == READ_ERROR_COUNTER_LPAGE)
				ptmpBuf[0] = READ_ERROR_COUNTER_LPAGE; // Page Code
			if (pageCode == READ_REVERSE_ERROR_COUNTER_LPAGE)
				ptmpBuf[0] = READ_REVERSE_ERROR_COUNTER_LPAGE; // Page Code
			if (pageCode == VERIFY_ERROR_COUNTER_LPAGE)
			ptmpBuf[0] = VERIFY_ERROR_COUNTER_LPAGE; // Page Code
			
			ptmpBuf[1] = 0x00; // subPage Code
			ptmpBuf[2] = 0x00;
			
			// Error Counter  log parameter
			ptmpBuf[3] = Core_Fill_ErrorCounterLogPage(ptmpBuf + 4);
			pageLen = ptmpBuf[3] ;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, pageLen);
			
			MV_CopyMemory(buf, ptmpBuf, tmpLen);
			pReq->Data_Transfer_Length = tmpLen;
			pReq->Scsi_Status = REQ_STATUS_SUCCESS;
			break;	
		case SELFTEST_RESULTS_LPAGE:
			ptmpBuf[0] = SELFTEST_RESULTS_LPAGE; // Page Code
			ptmpBuf[1] = 0x00; // subPage Code
			ptmpBuf[2] = 0x1;
			ptmpBuf[3] = 0x90;

			// Self-test Results log parameter
			for(i=0; i<20; i++)
			{
				pageLen = Core_Fill_SelfTestLogPage(&ptmpBuf[4+(i*20)], i);
			}
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen*20+4));
			
			MV_CopyMemory(buf, ptmpBuf, tmpLen);
			pReq->Data_Transfer_Length = tmpLen;
			pReq->Scsi_Status = REQ_STATUS_SUCCESS;
			break;	
		case TEMPERATURE_LPAGE:	/* Temperature log page */
			ptmpBuf[0] = TEMPERATURE_LPAGE; /* Page Code*/
			ptmpBuf[1] = 0x00; /* subPage Code*/
			ptmpBuf[2] = 0x00;
			ptmpBuf[3] = Core_Fill_TempLogPage(ptmpBuf + 4);
			pageLen = ptmpBuf[3] ;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, pageLen);
			
			MV_CopyMemory(buf, ptmpBuf, tmpLen);
			pReq->Data_Transfer_Length = tmpLen;
			pReq->Scsi_Status = REQ_STATUS_SUCCESS;
			break;	
		case STARTSTOP_CYCLE_COUNTER_LPAGE:	/* Informational exceptions log page */
			ptmpBuf[0] = STARTSTOP_CYCLE_COUNTER_LPAGE; /* Page Code*/
			ptmpBuf[1] = 0x00; /* subPage Code*/
			ptmpBuf[2] = 0x00;
			ptmpBuf[3] = Core_Fill_StartStopCycleCounterLogPage(ptmpBuf + 4);

			pageLen = ptmpBuf[3] ;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, pageLen);
			
			MV_CopyMemory(buf, ptmpBuf, tmpLen);
			pReq->Data_Transfer_Length = tmpLen;
			pReq->Scsi_Status = REQ_STATUS_SUCCESS;
			break;
		case IE_LPAGE:	/* Informational exceptions log page */
			ptmpBuf[0] = IE_LPAGE; /* Page Code*/
			ptmpBuf[1] = 0x00; /* subPage Code*/
			ptmpBuf[2] = 0x00;
			ptmpBuf[3] = Core_Fill_InfoExceptLogPage(ptmpBuf + 4);

			pageLen = ptmpBuf[3] ;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, pageLen);
			
			MV_CopyMemory(buf, ptmpBuf, tmpLen);
			pReq->Data_Transfer_Length = tmpLen;
			pReq->Scsi_Status = REQ_STATUS_SUCCESS;
			break;
		default:
		pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
		Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
		break;

		}
	} else {
		pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
		Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
		return ;
	}
}
static MV_U8 mvScsiModeSelectWceGet(PCore_Driver_Extension pCore, PMV_Request pReq, MV_U32 *WCEisEnabled)
{
    struct  scsi_cmnd *scmd=NULL;
    struct  scatterlist *sg=NULL;
    MV_U8  *req_buf=NULL;
    MV_U32  length= pReq->Data_Transfer_Length;
    MV_U8   offset= 0, cachePageOffset= 0;
    MV_U8   rc= 1;

    /* check for parameter list length error */
    if (length < 4)
        return 1;

    if(NULL == WCEisEnabled)
        return 1;

    *WCEisEnabled= 0;

    scmd=(struct scsi_cmnd *)pReq->Org_Req;
    if(!scmd)
        return 1;

    if(!mv_use_sg(scmd))
        return 1;

    sg = (struct scatterlist *)mv_rq_bf(scmd);
    if(NULL == sg)
        return 1;

    req_buf=(char *)(map_sg_page(sg)+sg->offset);
    if(NULL == req_buf)
        return 1;

    /* check for invalid field in parameter list */
    if (req_buf[0] || (req_buf[1] != 0) || req_buf[2])
    {
        MV_DPRINT(("Mode Select Error: invalid field in parameter \n"));
        goto cleanup;
    }

    if (req_buf[3])
    {
        /* check for invalid field in parameter list */
        if (req_buf[3] != 8)
        {
            MV_DPRINT(("Mode Select Error: wrong size for mode parameter"
                     " block descriptor, BLOCK DESCRIPTOR LENGTH %d\n.",req_buf[3]));
            goto cleanup;
        }

        /* check for parameter list length error */
        if (length < 12)
        {
            goto cleanup;
        }

        /* check for invalid field in parameter list */
        if (req_buf[4] || req_buf[5] || req_buf[6] || req_buf[7] || req_buf[8] || req_buf[9] ||(req_buf[10] != 0x2) || req_buf[11])
        {
            MV_DPRINT(("Mode Select Error: invalid field in parameter block descriptor list.\n"));
            goto cleanup;
        }
    }
    /* skip the mode parameter block descriptor */
    offset = 4 + req_buf[3];

    /* check for available mode pages */
    if (length == offset)
    {
        MV_DPRINT(("Mode Select: no mode pages available\n"));
        goto cleanup;
    }

    /* normalize to SELECT_10 offset */
    if (pReq->Cdb[0]==SCSI_CMD_MODE_SELECT_10)
        offset+= 4;

    while ((offset + 2) < length)
    {
        switch (req_buf[offset] & 0x3f)
        {
            case 0x8:
                if (req_buf[offset + 1] != 0x12)
                {
                    MV_DPRINT(("Mode Select Error: bad length in caching mode page %d\n.",
                             req_buf[offset + 1]));
                    goto cleanup;
                }
                cachePageOffset = offset;
                offset += req_buf[offset + 1] + 2;
            break;
            case 0xa:
                if ((req_buf[offset] != 0xa) || (req_buf[offset+1] != 0xa))
                {
                    MV_DPRINT(("Mode Select Error: invalid field in"
                             " mode control page, list[%x] %x, list[%x] %x\n",
                             offset, req_buf[offset], offset + 1, req_buf[offset+1]));
                    goto cleanup;
                }

                if (req_buf[offset + 3] != MV_BIT(4))
                {
                    MV_DPRINT(("Mode Select Error: invalid field in mode control page, list[%x] %x\n",
                             offset + 3, req_buf[offset + 3]));
                    goto cleanup;
                }

                if (req_buf[offset + 2] || req_buf[offset + 4]  || req_buf[offset + 5]  ||
                    req_buf[offset + 6] || req_buf[offset + 7]  ||  req_buf[offset + 8] ||
                    req_buf[offset + 9] || req_buf[offset + 10] || req_buf[offset + 11])
                {
                    MV_DPRINT(("Mode Select Error: invalid field in"
                             " mode control page, line %d\n", __LINE__));
                    goto cleanup;
                }
                offset += req_buf[offset + 1] + 2;
            break;
            default:
                MV_DPRINT(("Mode Select Error: invalid field in parameter "
                         "list, mode page %d not supported, offset %d\n",
                         req_buf[offset], offset));
                goto cleanup;
        }
    }

    if (length != offset)
    {
        MV_DPRINT(("Mode Select Error: bad length %d\n.", length));
        goto cleanup;
    }

    if (cachePageOffset)
    {
        *WCEisEnabled= (req_buf[cachePageOffset+2] & MV_BIT(2)) ? 1 : 0;
        rc= 0;
    }

cleanup:
    kunmap_atomic(sg, KM_IRQ0);
    return rc;
}

MV_BOOLEAN mvScsiModeSelect(PCore_Driver_Extension pCore, PMV_Request pReq)
{
	MV_U8 ptmpBuf[MAX_MODE_PAGE_LENGTH];
	MV_U32 pageLen = 0, tmpLen = 0;
	PDomain_Device pDevice = NULL;
	MV_U8 portId, deviceId;
	MV_U8 result = 0, cachePageOffset  =0 ,offset = 0;
	MV_U8  wcachebit = 0,smartbit = 0;
	MV_U8 *buf = pReq->Data_Buffer;
	MV_U32  length = pReq->Data_Transfer_Length;
	MV_U8   PF = (pReq->Cdb[1] & MV_BIT(4)) >> 4;
	MV_U8   SP = (pReq->Cdb[1] & MV_BIT(0));
	MV_U32 WCEisEnabled=0;
	if (length == 0) {
    		pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
        	return MV_TRUE;
    	}
   	 if (PF == 0 || SP == 1) {/* Invalid field in CDB,PARAMETER LIST LENGTH ERROR */
		pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
		Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ADSENSE_INVALID_CDB);
		return MV_TRUE;
    	}

	portId = PATA_MapPortId(pReq->Device_Id);
	deviceId = PATA_MapDeviceId(pReq->Device_Id);
	pDevice = &pCore->Ports[portId].Device[deviceId];
	
	if (mvScsiModeSelectWceGet(pCore, pReq, &WCEisEnabled))
	{
		 pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
		Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FIELD_IN_PARAMETER);
		return MV_TRUE;
	}
	MV_ZeroMemory(buf, pReq->Data_Transfer_Length);
	MV_ZeroMemory(ptmpBuf, MAX_MODE_PAGE_LENGTH);
	/* Block Descriptor Length set to 0 - No Block Descriptor */

	if (  pReq->Cdb[0]==SCSI_CMD_MODE_SELECT_6){
		pageLen = Core_get_mode_page_caching((ptmpBuf+4), pDevice);
		ptmpBuf[0] = (MV_U8)(4 + pageLen - 1);	/* Mode data length */
		ptmpBuf[2] = 0x10;
		tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+4));
		offset = 4;
	}
	else if  (pReq->Cdb[0]==SCSI_CMD_MODE_SELECT_10) {	/* Mode Sense 10,select10 */
		pageLen = Core_get_mode_page_caching((ptmpBuf+8), pDevice);
		/* Mode Data Length, it does not include the number of bytes in */
		/* Mode Data Length field */
		tmpLen = 8 + pageLen - 2;
		ptmpBuf[0] = (MV_U8)(((MV_U16)tmpLen) >> 8);
		ptmpBuf[1] = (MV_U8)tmpLen;
		ptmpBuf[2] = 0x00;
		ptmpBuf[3] = 0x10;
		tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+8));
		offset = 8;
	}
	MV_CopyMemory(buf, ptmpBuf, tmpLen);
	pReq->Data_Transfer_Length = tmpLen;

        if ( (buf[offset] & 0x3f) == 0x08)
        {
            if (buf[offset + 1] != 0x12)
            {
              /* Invalid field in parameter list */
                pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
                Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FIELD_IN_PARAMETER);
                return MV_TRUE;
            }
            cachePageOffset = offset;
            if(WCEisEnabled){
            		// enable write cache ,send to hardware
            		pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
			pReq->Cdb[1] = CDB_CORE_MODULE;
			pReq->Cdb[2] = CDB_CORE_ENABLE_WRITE_CACHE;
			return MV_FALSE;
            } else{
            		// disable write cache,send to hardware
            		pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
			pReq->Cdb[1] = CDB_CORE_MODULE;
			pReq->Cdb[2] = CDB_CORE_DISABLE_WRITE_CACHE;
			return MV_FALSE;
            }
        }
	pReq->Scsi_Status = REQ_STATUS_SUCCESS;
	return MV_TRUE;
}

void mvScsiReadDefectData(PCore_Driver_Extension pCore, PMV_Request req)
{
        MV_U8  temp_buf[6];
        MV_U32 length;
        MV_PU8 data_buf = req->Data_Buffer;

        temp_buf[0] = 0;
        temp_buf[1] = req->Cdb[2] & 0x18;
        temp_buf[2] = 0;
        temp_buf[3] = 0;
        temp_buf[4] = 0;
        temp_buf[5] = 0;
        
        length = MV_MIN(6, req->Data_Transfer_Length);

        MV_CopyMemory(req->Data_Buffer, temp_buf, length);
        req->Data_Transfer_Length = length;
        req->Scsi_Status = REQ_STATUS_SUCCESS;
}

MV_U8 check_page_control(PMV_Request pReq,MV_U8 page_control){
	MV_U8 ret=0;
	switch(page_control){
		case 0:/* only support current */
			ret=0;
			break;
		case 3:
			ret=1;
			pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
			Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_SAVING_PARAMETERS_NOT_SUPPORT);
			break;
		case 1:
		case 2:
		default:
			ret=1;
			pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
			Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
			break;
		}
	return ret;
}
void mvScsiModeSense(PCore_Driver_Extension pCore, PMV_Request pReq)
{
	MV_U8 pageCode = pReq->Cdb[2] & 0x3F;		/* Same for mode sense 6 and 10 */
	MV_U8 page_control=pReq->Cdb[2]>>6;
	MV_U8 ptmpBuf[MAX_MODE_PAGE_LENGTH];
	MV_U32 pageLen = 0, tmpLen = 0;
	PDomain_Device pDevice = NULL;
	MV_U8 portId, deviceId;
	MV_U8 *buf = pReq->Data_Buffer;

	portId = PATA_MapPortId(pReq->Device_Id);
	deviceId = PATA_MapDeviceId(pReq->Device_Id);
	pDevice = &pCore->Ports[portId].Device[deviceId];

	MV_ZeroMemory(buf, pReq->Data_Transfer_Length);
	MV_ZeroMemory(ptmpBuf, MAX_MODE_PAGE_LENGTH);
	/* Block Descriptor Length set to 0 - No Block Descriptor */

	switch (pageCode) {
	case 0x3F:		/* Return all pages */
	case 0x08:		/* Caching mode page */
		if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_6) {
			pageLen = Core_get_mode_page_caching((ptmpBuf+4), pDevice);
			ptmpBuf[0] = (MV_U8)(4 + pageLen - 1);	/* Mode data length */
			ptmpBuf[2] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+4));
		}
		else {	/* Mode Sense 10 */
			pageLen = Core_get_mode_page_caching((ptmpBuf+8), pDevice);
			/* Mode Data Length, it does not include the number of bytes in */
			/* Mode Data Length field */
			tmpLen = 8 + pageLen - 2;
			ptmpBuf[0] = (MV_U8)(((MV_U16)tmpLen) >> 8);
			ptmpBuf[1] = (MV_U8)tmpLen;
			ptmpBuf[2] = 0x00;
			ptmpBuf[3] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+8));
		}
		if(check_page_control(pReq,page_control))
			break;
		MV_CopyMemory(buf, ptmpBuf, tmpLen);
		pReq->Data_Transfer_Length = tmpLen;
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
	case 0x19:	/*Protocol Specific Port mode page*/	
		if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_6) {
			pageLen = Core_Fill_ProSpecPortModePage(ptmpBuf+4);
			ptmpBuf[0] = (MV_U8)(4 + pageLen - 1);	/* Mode data length */
			ptmpBuf[2] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+4));
		}else if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_10) {	/* Mode Sense 10 */
			pageLen = Core_Fill_ProSpecPortModePage(ptmpBuf+8);
			/* Mode Data Length, it does not include the number of bytes in */
			/* Mode Data Length field */
			tmpLen = 8 + pageLen - 2;
			ptmpBuf[0] = (MV_U8)(((MV_U16)tmpLen) >> 8);
			ptmpBuf[1] = (MV_U8)tmpLen;
			ptmpBuf[2] = 0x00;
			ptmpBuf[3] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+8));
		}
		if(check_page_control(pReq,page_control))
			break;
		MV_CopyMemory(buf, ptmpBuf, tmpLen);
		pReq->Data_Transfer_Length = tmpLen;
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
	case 0x1c:	/*Informational Exceptions Control mode page*/
		if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_6) {
			pageLen = Core_Fill_InfoExcepCtrlModePage((ptmpBuf+4), pDevice,pReq);
			ptmpBuf[0] = (MV_U8)(4 + pageLen - 1);	/* Mode data length */
			ptmpBuf[2] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+4));
		}else if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_10) {	/* Mode Sense 10 */
			pageLen = Core_Fill_InfoExcepCtrlModePage((ptmpBuf+8), pDevice,pReq);
			/* Mode Data Length, it does not include the number of bytes in */
			/* Mode Data Length field */
			tmpLen = 8 + pageLen - 2;
			ptmpBuf[0] = (MV_U8)(((MV_U16)tmpLen) >> 8);
			ptmpBuf[1] = (MV_U8)tmpLen;
			ptmpBuf[2] = 0x00;
			ptmpBuf[3] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+8));
		}
		if(check_page_control(pReq,page_control))
			break;
		MV_CopyMemory(buf, ptmpBuf, tmpLen);
		pReq->Data_Transfer_Length = tmpLen;
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
	case 0x0a:  /*Control mode page*/		
		if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_6) {
			pageLen = Core_Fill_CtrlModePage((ptmpBuf+4), pDevice);
			ptmpBuf[0] = (MV_U8)(4 + pageLen - 1);	/* Mode data length */
			ptmpBuf[2] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+4));
		}else if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_10) {	/* Mode Sense 10 */
			pageLen = Core_Fill_CtrlModePage((ptmpBuf+8), pDevice);
			/* Mode Data Length, it does not include the number of bytes in */
			/* Mode Data Length field */
			tmpLen = 8 + pageLen - 2;
			ptmpBuf[0] = (MV_U8)(((MV_U16)tmpLen) >> 8);
			ptmpBuf[1] = (MV_U8)tmpLen;
			ptmpBuf[2] = 0x00;
			ptmpBuf[3] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+8));
		}
		if(check_page_control(pReq,page_control))
			break;
		MV_CopyMemory(buf, ptmpBuf, tmpLen);
		pReq->Data_Transfer_Length = tmpLen;
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		break;

	case 0x01: /*RW Error Recovery*/
		if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_6) {
			pageLen = Core_Fill_RWErrorRecoveryModePage(ptmpBuf+4);
			ptmpBuf[0] = (MV_U8)(4 + pageLen - 1);	/* Mode data length */
			ptmpBuf[2] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+4));
		}else if (pReq->Cdb[0]==SCSI_CMD_MODE_SENSE_10) {	/* Mode Sense 10 */
			pageLen = Core_Fill_RWErrorRecoveryModePage(ptmpBuf+8);
			/* Mode Data Length, it does not include the number of bytes in */
			/* Mode Data Length field */
			tmpLen = 8 + pageLen - 2;
			ptmpBuf[0] = (MV_U8)(((MV_U16)tmpLen) >> 8);
			ptmpBuf[1] = (MV_U8)tmpLen;
			ptmpBuf[2] = 0x00;
			ptmpBuf[3] = 0x10;
			tmpLen = MV_MIN(pReq->Data_Transfer_Length, (pageLen+8));
		}
		if(check_page_control(pReq,page_control))
			break;
		MV_CopyMemory(buf, ptmpBuf, tmpLen);
		pReq->Data_Transfer_Length = tmpLen;
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
		
	default:
		pReq->Scsi_Status = REQ_STATUS_HAS_SENSE;
		Core_FillSenseData(pReq, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
		break;
	}
}

void Core_Fill_SendDiagTaskfile( PDomain_Device device,PMV_Request req, PATA_TaskFile taskfile)
{

        if ((req->Cdb[1] & 0x04) &&
                (device->Capacity & 
                        DEVICE_CAPACITY_SMART_SELF_TEST_SUPPORTED) &&
                (device->Setting & DEVICE_SETTING_SMART_ENABLED)) {
		        
                taskfile->LBA_Low = 0x01;
        
        } else if ((req->Cdb[1] & 0x04) &&
                !((device->Capacity & 
                        DEVICE_CAPACITY_SMART_SELF_TEST_SUPPORTED) &&
                (device->Setting & DEVICE_SETTING_SMART_ENABLED))) {
          //      scsi_ata_send_diag_verify_0(root, req);

                taskfile->Device = MV_BIT(6);
                taskfile->LBA_Low = 0;
                taskfile->LBA_Mid = 0;
                taskfile->LBA_High = 0;
                taskfile->Sector_Count = 1;
	        taskfile->LBA_Low_Exp = 0;
                taskfile->LBA_Mid_Exp = 0;
                taskfile->LBA_High_Exp = 0;
                taskfile->Sector_Count_Exp = 0;
                taskfile->Features = 0;
	        taskfile->Control = 0;
	        taskfile->Feature_Exp = 0;

                if (device->Capacity & DEVICE_CAPACITY_48BIT_SUPPORTED) {
                        taskfile->Command = ATA_CMD_VERIFY_EXT;
                } else {
                        taskfile->Command = ATA_CMD_VERIFY;
                }

                return;
        } else {
	        switch ((req->Cdb[1] & 0xE0) >> 5) {
	        case BACKGROUND_SHORT_SELF_TEST:
                        taskfile->LBA_Low = 0x01;
		        break;
	        case BACKGROUND_EXTENDED_SELF_TEST:
                        taskfile->LBA_Low = 0x02;
		        break;
	        case ABORT_BACKGROUND_SELF_TEST:
                        taskfile->LBA_Low = 0x7F;
		        break;
	        case FOREGROUND_SHORT_SELF_TEST:
                        taskfile->LBA_Low = 0x81;
		        break;
	        case FOREGROUND_EXTENDED_SELF_TEST:
                        taskfile->LBA_Low = 0x82;
		        break;
	        default:
		        break;
	        }
        }

        taskfile->Device = MV_BIT(6);
        taskfile->LBA_Mid = 0x4F;
        taskfile->LBA_High = 0xC2;
        taskfile->Features = ATA_CMD_SMART_EXECUTE_OFFLINE;
	 taskfile->Command = ATA_CMD_SMART;

	taskfile->Sector_Count = 0;
	taskfile->Control = 0;
	taskfile->Feature_Exp = 0;
	taskfile->Sector_Count_Exp = 0;
	taskfile->LBA_Low_Exp = 0;
	taskfile->LBA_Mid_Exp = 0;
	taskfile->LBA_High_Exp = 0;
}

#endif //#ifdef SUPPORT_ATA_SMART


