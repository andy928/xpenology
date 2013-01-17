#ifndef __HBA_EXPOSE_H__
#define __HBA_EXPOSE_H__

#ifdef SUPPORT_EVENT
#include "com_event_struct.h"
#include "com_event_define.h"
#include "com_event_define_ext.h"
#endif /* SUPPORT_EVENT */

#include "hba_header.h"
#include "com_mod_mgmt.h"

struct _MV_SCP {
	MV_U16           mapped;
	MV_U16           map_atomic;
	BUS_ADDRESS bus_address;
};

#define MV_SCp(cmd) ((struct _MV_SCP *)(&((struct scsi_cmnd *)cmd)->SCp))

typedef struct _Assigned_Uncached_Memory
{
	MV_PVOID			Virtual_Address;
	MV_PHYSICAL_ADDR	Physical_Address;
	MV_U32				Byte_Size;
	MV_U32				Reserved0;
} Assigned_Uncached_Memory, *PAssigned_Uncached_Memory;

typedef struct _Controller_Infor
{
        MV_PVOID Base_Address;

        MV_U16 Vendor_Id;
        MV_U16 Device_Id;
        MV_U8 Revision_Id;
        MV_U8 Reserved[3];
} Controller_Infor, *PController_Infor;

void HBA_ModuleStarted(struct mv_mod_desc *mod_desc);

#ifdef SUPPORT_ATA_POWER_MANAGEMENT                                                                       
#define IS_ATA_PASS_THROUGH_COMMAND(pReq) \
	((pReq->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) && \
    		(pReq->Cdb[1] == CDB_CORE_MODULE) && (\
			pReq->Cdb[2] == CDB_CORE_ATA_SLEEP || \
			pReq->Cdb[2] == CDB_CORE_ATA_IDLE || \
			pReq->Cdb[2] == CDB_CORE_ATA_STANDBY || \
			pReq->Cdb[2] == CDB_CORE_ATA_IDLE_IMMEDIATE || \
			pReq->Cdb[2] == CDB_CORE_ATA_CHECK_POWER_MODE || \
			pReq->Cdb[2] == CDB_CORE_ATA_STANDBY_IMMEDIATE || \
			pReq->Cdb[2] == CDB_CORE_RESET_DEVICE))
#endif

#define IS_ATA_12_CMD(scmd) \
	((scmd->cmnd[0]==ATA_12)&& \
	 (scmd->cmnd[9] ==0x08||scmd->cmnd[9] ==0xE0||\
	 scmd->cmnd[9] ==0xE1||scmd->cmnd[9] ==0xE2||\
	 scmd->cmnd[9] ==0xE3|| scmd->cmnd[9] ==0xE5||\
	 scmd->cmnd[9] ==0xE6||scmd->cmnd[9] ==0xEC||\
	 scmd->cmnd[9] ==0xA1||scmd->cmnd[9] ==0xB0))


#ifdef SUPPORT_EVENT
/* wrapper for DriverEvent, needed to implement queue */
typedef struct _Driver_Event_Entry
{
	struct list_head Queue_Pointer;
	DriverEvent Event;
} Driver_Event_Entry, *PDriver_Event_Entry;
#endif /* SUPPORT_EVENT */

struct gen_module_desc {
#ifdef __MM_SE__	
/* Must the first */
	struct mv_mod_desc *desc;
#else
	MV_PVOID	reserved;
#endif /* __MM_SE__ */	
};

#define __ext_to_gen(_ext)       ((struct gen_module_desc *) (_ext))

#define HBA_GetNextModuleSendFunction(_ext, _child_ext_p, _func_pp)          \
           {                                                                 \
		   *(_func_pp) = __ext_to_gen(_ext)->desc->child->ops->module_sendrequest; \
		   *(_child_ext_p) = __ext_to_gen(_ext)->desc->child->extension;           \
	   }

#define HBA_GetUpperModuleNotificationFunction(_ext, _parent_ext_p, _func_pp) \
           {                                                                  \
		   *(_func_pp) = __ext_to_gen(_ext)->desc->parent->ops->module_notification; \
		   *(_parent_ext_p) = __ext_to_gen(_ext)->desc->parent->extension;           \
	   }


#define hba_notify_upper_md(ext, eid, param)                 			 	\
   {                                                                     	\
	__ext_to_gen(ext)->desc->parent->ops->module_notification(       		\
                                            __ext_to_gen(ext)->desc->parent->extension, 	\
					    					eid,         				 	\
					    					param);					 	\
   }



#define HBA_GetControllerInfor(_ext, _pinfo)                              \
           {    \
		   (_pinfo)->Base_Address = __ext_to_gen(_ext)->desc->hba_desc->Base_Address; \
		   (_pinfo)->Vendor_Id    = __ext_to_gen(_ext)->desc->hba_desc->vendor;    \
		   (_pinfo)->Device_Id    = __ext_to_gen(_ext)->desc->hba_desc->device;    \
		   (_pinfo)->Revision_Id  = __ext_to_gen(_ext)->desc->hba_desc->Revision_Id;  \
	   }


void hba_swap_buf_le16(u16 *buf, unsigned int words);

/* map bus addr in sg entry into cpu addr (access via. Data_Buffer) */
void hba_map_sg_to_buffer(void *preq);
void hba_unmap_sg_to_buffer(void *preq);

void hba_log_msg(unsigned char *buf, unsigned int len);

int __hba_dump_log(unsigned char *buf);

static inline MV_BOOLEAN
HBA_ModuleGetPhysicalAddress(MV_PVOID Module,
			     MV_PVOID Virtual,
			     MV_PVOID TranslationContext,
			     MV_PU64 PhysicalAddress,
			     MV_PU32 Length)
{
	panic("not supposed to be called.\n");
	return MV_FALSE;
};

void __hba_dump_req_info(unsigned int module, PMV_Request req);
int HBA_GetResource(struct mv_mod_desc *mod_desc,
		    enum Resource_Type type,
		    MV_U32  size,
		    Assigned_Uncached_Memory *dma_res);
MV_PVOID HBA_GetModuleExtension(MV_PVOID ext, MV_U32 mod_id);
void HBA_ModuleNotification(MV_PVOID This, 
			     enum Module_Event event, 
			     struct mod_notif_param *event_param);

#ifdef THOR_DRIVER
void HBA_RequestTimer(
	MV_PVOID extension,
	MV_U32 millisecond,
	MV_VOID (*routine) (MV_PVOID)
	);
void hba_spin_lock_irq(spinlock_t* plock);
void hba_spin_unlock_irq(spinlock_t* plock);

#endif

#ifdef SUPPORT_REQUEST_TIMER
#define 	add_request_timer(pReq, time, func, ctx)			\
			{													\
				hba_add_timer(pReq, time, (OSSW_TIMER_FUNCTION)(func));					\
				(pReq)->err_request_ctx=(MV_PVOID)(ctx);							\
			}	

#define 	remove_request_timer(pReq)			\
			{													\
				hba_remove_timer(pReq);					\
			}	

#endif

MV_BOOLEAN __is_scsi_cmd_simulated(MV_U8 cmd_type);
void HBA_kunmap_sg(void*);
/*set pci Device ID */
MV_U16 SetDeviceID(MV_U32 pad_test);


#endif /* __HBA_EXPOSE_H__ */
