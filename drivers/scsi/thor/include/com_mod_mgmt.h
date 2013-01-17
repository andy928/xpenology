#ifndef __MV_MODULE_MGMT__
#define __MV_MODULE_MGMT__

#include "com_define.h"
#include "com_type.h"
#include "com_util.h"

enum {
	/* module status (module_descriptor) */
	MV_MOD_VOID   = 0,
	MV_MOD_UNINIT,
	MV_MOD_REGISTERED,  /* module ops pointer registered */
	MV_MOD_INITED,      /* resource assigned */ 
	MV_MOD_FUNCTIONAL,
	MV_MOD_STARTED,
	MV_MOD_DEINIT,      /* extension released, be gone soon */
	MV_MOD_GONE,
};


/* adapter descriptor */
struct mv_adp_desc {
	struct list_head  hba_entry;
	struct list_head  online_module_list;

	spinlock_t        lock;
	spinlock_t        global_lock;

	/* adapter information */
	MV_U8             Adapter_Bus_Number;
	MV_U8             Adapter_Device_Number;
	MV_U8             Revision_Id;
	MV_U8             id;             /* multi-hba support, start from 0 */
	MV_U8             pad0;
	MV_U8             running_mod_num;/* number of up & running modules */


	MV_U16            vendor;
	MV_U16            device;
	MV_U16            subsystem_vendor;
	MV_U16            subsystem_device;
#ifdef ODIN_DRIVER
	MV_U8				RunAsNonRAID;
#endif
	/* System resource */
	MV_PVOID          Base_Address[ROUNDING(MAX_BASE_ADDRESS, 2)];

	MV_U32            max_io;

#ifdef _OS_LINUX 
	dev_t             dev_no;

	struct pci_dev    *dev;
	MV_PVOID          extension;      /* hba_ext */
#endif /* _OS_LINUX */
};

struct mv_mod_desc;


struct mv_mod_res {
	struct list_head       res_entry;
	MV_PHYSICAL_ADDR       bus_addr;
	MV_PVOID               virt_addr;
	
	MV_U32                 size;
	
	MV_U16                 type;          /* enum Resource_Type */
	MV_U16                 align;
};

typedef struct _Module_Interface
{
	MV_U8      module_id;
	MV_U32     (*get_res_desc)(enum Resource_Type type, MV_U16 maxIo);
	MV_VOID    (*module_initialize)(MV_PVOID extension,
					MV_U32   size,
					MV_U16   max_io);
	MV_VOID    (*module_start)(MV_PVOID extension);
	MV_VOID    (*module_stop)(MV_PVOID extension);
	MV_VOID    (*module_notification)(MV_PVOID extension, 
					  enum Module_Event event, 
					  struct mod_notif_param *param);
	MV_VOID    (*module_sendrequest)(MV_PVOID extension, 
					 PMV_Request pReq);
	MV_VOID    (*module_reset)(MV_PVOID extension);
	MV_VOID    (*module_monitor)(MV_PVOID extension);
	MV_BOOLEAN (*module_service_isr)(MV_PVOID extension);
#ifdef RAID_DRIVER
	MV_VOID    (*module_send_xor_request)(MV_PVOID This, 
					      PMV_XOR_Request pXORReq);
#endif /* RAID_DRIVER */
} Module_Interface, *PModule_Interface;

#define mv_module_ops _Module_Interface

#define mv_set_mod_ops(_ops, _id, _get_res_desc, _init, _start,            \
		       _stop, _send, _reset, _mon, _send_eh, _isr, _xor)   \
           {                                                               \
		   _ops->id                      = id;                     \
		   _ops->get_res_desc            = _get_res_desc;          \
		   _ops->module_initialize       = _init;                  \
		   _ops->module_start            = _start;                 \
		   _ops->module_stop             = _stop;                  \
		   _ops->module_sendrequest      = _send;                  \
		   _ops->module_reset            = _reset;                 \
		   _ops->module_monitor          = _mon;                   \
		   _ops->module_send_eh_request  = _send_eh;               \
		   _ops->module_service_isr      = _isr;                   \
		   _ops->module_send_xor_request = _xor;                   \
	   }


/* module descriptor */
struct mv_mod_desc {
	struct list_head           mod_entry;      /* kept in a list */
	
	struct mv_mod_desc         *parent;
	struct mv_mod_desc         *child;

	MV_U32                     extension_size;
	MV_U8                      status;
	MV_U8                      ref_count;
	MV_U8                      module_id;
	MV_U8                      res_entry;

	MV_PVOID                   extension;      /* module extention */
	struct mv_module_ops       *ops;           /* interface operations */

	struct mv_adp_desc         *hba_desc;
	struct list_head           res_list;
};

#endif /* __MV_MODULE_MGMT__ */
