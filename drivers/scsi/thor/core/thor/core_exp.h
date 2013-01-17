#if !defined(CORE_EXPOSE_H)
#define CORE_EXPOSE_H

#ifdef __MM_SE__
#define __ext_to_core(_ext)       ((PCore_Driver_Extension) (_ext))

#define core_start_cmpl_notify(_ext)                                      \
           {                                                              \
		   __ext_to_core(_ext)->desc->status = MV_MOD_STARTED;    \
		   HBA_ModuleStarted(__ext_to_core(_ext)->desc);          \
	   }


#define core_generate_event(ext, eid, did, slv, pc, ptr)                  \
   {                                                                      \
	struct mod_notif_param _param = {ptr, 0, 0, eid, did, slv, pc};   \
	__ext_to_core(ext)->desc->parent->ops->module_notification(       \
                                            ext->desc->parent->extension, \
					    EVENT_LOG_GENERATED,          \
					    &_param);                     \
   }


#else /* __MM_SE__ */
#define core_generate_event(ext, eid, did, slv, pc, ptr)                \
   {                                                                    \
       struct mod_notif_param param = {ptr, 0, 0, eid, did, slv, pc};   \
       HBA_ModuleNotification(ext, EVENT_LOG_GENERATED, &param);        \
   }

#define core_start_cmpl_notify(_ext)  	HBA_ModuleStarted(_ext) 
#endif /* __MM_SE__ */


MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo);
void Core_ModuleInitialize(MV_PVOID, MV_U32, MV_U16);
void Core_ModuleStart(MV_PVOID);
void Core_ModuleShutdown(MV_PVOID);
void Core_ModuleNotification(MV_PVOID, enum Module_Event, struct mod_notif_param *);
void Core_ModuleSendRequest(MV_PVOID, PMV_Request);
void Core_ModuleMonitor(MV_PVOID);
void Core_ModuleReset(MV_PVOID pExtension);

MV_BOOLEAN Core_InterruptServiceRoutine(MV_PVOID This);

#ifdef  SUPPORT_TASKLET
MV_BOOLEAN Core_HandleServiceRoutine(MV_PVOID This);
#endif

#ifdef SUPPORT_ERROR_HANDLING
#ifdef _OS_LINUX
#define REQUEST_TIME_OUT			10		// in multiples of TIMER_INTERVAL, see hba_timer.h
#else
#define REQUEST_TIME_OUT			30		// in multiples of TIMER_INTERVAL, see hba_timer.h
#endif
#endif

void mvRemoveDeviceWaitingList( MV_PVOID This, MV_U16 deviceId, MV_BOOLEAN returnOSRequest );
void mvRemovePortWaitingList( MV_PVOID This, MV_U8 portId );
MV_U8 mv_core_check_is_reseeting(MV_PVOID core_ext);
void Core_FillSenseData(PMV_Request pReq, MV_U8 senseKey, MV_U8 adSenseCode);
void RAID_ModuleNotification(MV_PVOID This,
			     enum Module_Event event,
			     struct mod_notif_param *param);

void sata_hotplug(MV_PVOID data,MV_U32 intStatus);
#endif /* CORE_EXPOSE_H */

