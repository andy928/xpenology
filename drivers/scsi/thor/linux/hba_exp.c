#include "hba_header.h"
#include "linux_main.h"

#include "hba_exp.h"

/*
 * 
 * Other exposed functions
 *
 */
int __mv_is_mod_all_started(struct mv_adp_desc *adp_desc);
/* 
 * The extension is the calling module extension. 
 *   It can be any module extension. 
 */
void HBA_ModuleStarted(struct mv_mod_desc *mod_desc)
{
	struct hba_extension *hba;
	struct mv_mod_desc *desc;
	MV_DBG(DMSG_KERN, "start HBA_ModuleStarted addr %p.\n",mod_desc);

	desc = mod_desc;
	while (desc->parent)
		desc = desc->parent; /* hba to be the uppermost */
	hba = (struct hba_extension *) desc->extension;

	if (__mv_is_mod_all_started(desc->hba_desc)) {
		MV_DBG(DMSG_HBA, "all modules have been started.\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
		atomic_set(&hba->hba_sync, 0);
#else
		complete(&hba->cmpl);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
		/* We are totally ready for requests handling. */
		hba->State = DRIVER_STATUS_STARTED;

		/* Module 0 is the last module */
		hba->desc->ops->module_notification(hba,
						    EVENT_MODULE_ALL_STARTED,
						    NULL);
	} else {
		/*
		 * hba's start has already been called, so we should not
		 * call it here. (hba is the highest module, it has no parent.)
		 */
		if (mod_desc->parent && mod_desc->parent->parent)
		{
			MV_DBG(DMSG_HBA, "start module %d.....\n",mod_desc->parent->module_id);
			mod_desc->parent->ops->module_start(mod_desc->parent->extension);
		}
	}

}

#ifdef __BIG_ENDIAN
void hba_swap_buf_le16(u16 *buf, unsigned int words)
{

	unsigned int i;

	for (i=0; i < words; i++)
                buf[i] = le16_to_cpu(buf[i]);

}
#else
inline void hba_swap_buf_le16(u16 *buf, unsigned int words) {}
#endif /* __BIG_ENDIAN */

void hba_map_sg_to_buffer(void *preq)
{
	struct scsi_cmnd *scmd =NULL;
	struct scatterlist *sg =NULL;
	PMV_Request        req =NULL;

	req  = (PMV_Request) preq;

	if (REQ_TYPE_OS != req->Req_Type)
		return;

	scmd = (struct scsi_cmnd *) req->Org_Req;
	sg = (struct scatterlist *) mv_rq_bf(scmd);

	
	if (mv_use_sg(scmd)) {
		if (mv_use_sg(scmd) > 1)
			MV_DBG(DMSG_SCSI, 
			       "_MV_ more than 1 sg entry in an inst cmd.\n");
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
		req->Data_Buffer = kmalloc(sg->length, GFP_ATOMIC);
		if (req->Data_Buffer) {
			memset(req->Data_Buffer, 0, sg->length);
		}
#else
		req->Data_Buffer = kzalloc(sg->length, GFP_ATOMIC);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14) */

		req->Data_Transfer_Length = sg->length;
	} else {
	        req->Data_Buffer = mv_rq_bf(scmd);
	}
}

void hba_unmap_sg_to_buffer(void *preq)
{
	void *buf;
	struct scsi_cmnd *scmd = NULL;
	struct scatterlist *sg = NULL;
	PMV_Request        req = NULL;
	unsigned long flags = 0;

	req  = (PMV_Request) preq;

	if (REQ_TYPE_OS != req->Req_Type)
		return;

	scmd = (struct scsi_cmnd *) req->Org_Req;
	sg   = (struct scatterlist *)mv_rq_bf(scmd);

	if (mv_use_sg(scmd)) {
		//MV_WARNON(!irqs_disabled());
		local_irq_save(flags);
		buf = map_sg_page(sg) + sg->offset;
		memcpy(buf, req->Data_Buffer, sg->length);
		kunmap_atomic(buf, KM_IRQ0);
		kfree(req->Data_Buffer);
		local_irq_restore(flags);
	}
}

MV_PVOID HBA_GetModuleExtension(MV_PVOID ext, MV_U32 mod_id)
{
	struct mv_mod_desc *mod_desc=(struct mv_mod_desc *)__ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	MV_ASSERT(mod_id<MAX_MODULE_NUMBER);
	BUG_ON(NULL == mod_desc);
	list_for_each_entry(mod_desc, &hba_desc->online_module_list, 
			    mod_entry)
	{
		BUG_ON(NULL == mod_desc);
		if (mod_desc->status != MV_MOD_GONE) 
		{
			if ((mod_desc->module_id == mod_id) && (mod_desc->extension))
			{
				return mod_desc->extension;
			}
		}
	}

	MV_ASSERT(MV_FALSE);
	return	NULL;
}
#ifdef THOR_DRIVER
void HBA_TimerRoutine(unsigned long DeviceExtension)
{
#ifndef SUPPORT_TIMER
	PHBA_Extension pHBA   = (PHBA_Extension)HBA_GetModuleExtension((MV_PVOID)DeviceExtension, MODULE_HBA);
	PTimer_Module  pTimer = &pHBA->Timer_Module;
	unsigned long  flags;

	MV_DASSERT(pTimer->routine != NULL);
	spin_lock_irqsave(&pHBA->desc->hba_desc->global_lock, flags);
	pTimer->routine(pTimer->context);
	spin_unlock_irqrestore(&pHBA->desc->hba_desc->global_lock, flags);
#endif /* SUPPORT_TIMER */
}


void HBA_RequestTimer(
	MV_PVOID extension,
	MV_U32 millisecond,
	MV_VOID (*routine) (MV_PVOID)
	)
{
	PHBA_Extension pHBA   = (PHBA_Extension)HBA_GetModuleExtension(extension,MODULE_HBA);
	PTimer_Module pTimer = &pHBA->Timer_Module;
	u64 jif_offset;
	
	pTimer->routine = routine;
	pTimer->context = extension;
	del_timer(&pHBA->timer);
	pHBA->timer.function = HBA_TimerRoutine;
	pHBA->timer.data = (unsigned long)extension;
	jif_offset = (u64)(millisecond * HZ);
	do_div(jif_offset, 1000);
	pHBA->timer.expires = jiffies + 1 + jif_offset;
	add_timer(&pHBA->timer);
}

void hba_spin_lock_irq(spinlock_t* plock)
{
	WARN_ON(irqs_disabled());
	spin_lock_irq(plock);                             	
}

void hba_spin_unlock_irq(spinlock_t* plock)
{
	spin_unlock_irq(plock);                             	
}


#endif

#define LO_ADDR(x) ((MV_U32) ((MV_PTR_INTEGER) (x)))
#define HI_ADDR(x) ((MV_U32) (sizeof(void *)>4?((MV_PTR_INTEGER) (x))>>32:0))

MV_BOOLEAN __is_scsi_cmd_simulated(MV_U8 cmd_type)
{
	switch (cmd_type)
	{
	case SCSI_CMD_INQUIRY:
	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_READ_CAPACITY_16:
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
	case SCSI_CMD_TEST_UNIT_READY:
	case SCSI_CMD_REQUEST_SENSE:
	case SCSI_CMD_RESERVE_6:
	case SCSI_CMD_RELEASE_6:
	case SCSI_CMD_REPORT_LUN:
	case SCSI_CMD_MODE_SENSE_6:
	case SCSI_CMD_MODE_SENSE_10:
	case SCSI_CMD_MODE_SELECT_6:
	case SCSI_CMD_MODE_SELECT_10:
#ifdef SUPPORT_ATA_SMART
	case SCSI_CMD_LOG_SENSE:
	case SCSI_CMD_READ_DEFECT_DATA_10:
#endif
#ifdef CORE_SUPPORT_API
	case APICDB0_PD:
#   ifdef SUPPORT_PASS_THROUGH_DIRECT
	case APICDB0_PASS_THRU_CMD:
#   endif /* SUPPORT_PASS_THROUGH_DIRECT */
#   ifdef SUPPORT_CSMI
	case APICDB0_CSMI_CORE:
#   endif /* SUPPORT_CSMI */
#endif /* CORE_SUPPORT_API */
		return MV_TRUE;
	default:
		return MV_FALSE;
	}
}

