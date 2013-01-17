#ifndef __MV_COM_TYPE_H__
#define __MV_COM_TYPE_H__

#include "com_define.h"
#include "com_list.h"

#include "mv_config.h"	// USE_NEW_SGTABLE is defined in mv_config.h

/*
 * Data Structure
 */
#define MAX_CDB_SIZE                            16

struct _MV_Request;
typedef struct _MV_Request MV_Request, *PMV_Request;

#ifdef RAID_DRIVER
typedef struct _MV_XOR_Request MV_XOR_Request, *PMV_XOR_Request;
#endif /* RAID_DRIVER */

#define REQ_STATUS_SUCCESS                      0x0
#define REQ_STATUS_NOT_READY                    0x1
#define REQ_STATUS_MEDIA_ERROR                  0x2
#define REQ_STATUS_BUSY                         0x3
#define REQ_STATUS_INVALID_REQUEST              0x4
#define REQ_STATUS_INVALID_PARAMETER            0x5
#define REQ_STATUS_NO_DEVICE                    0x6
/* Sense data structure is the SCSI "Fixed format sense datat" format. */
#define REQ_STATUS_HAS_SENSE                    0x7
#define REQ_STATUS_ERROR                        0x8
#define REQ_STATUS_ERROR_WITH_SENSE             0x10
/* Request initiator must set the status to REQ_STATUS_PENDING. */
#define REQ_STATUS_PENDING                      0x80
#define REQ_STATUS_RETRY                        0x81
#define REQ_STATUS_REQUEST_SENSE                0x82
#define REQ_STATUS_ABORT                        0x83

/* 
 * Don't change the order here. 
 * Module_StartAll will start from big id to small id. 
 * Make sure module_set setting matches the Module_Id 
 * MODULE_HBA must be the first one. Refer to Module_AssignModuleExtension.
 * And HBA_GetNextModuleSendFunction has an assumption that the next level 
 * has larger ID.
 */
enum Module_Id
{
        MODULE_HBA = 0,
#ifdef CACHE_MODULE_SUPPORT
        MODULE_CACHE,
#endif /*  CACHE_MODULE_SUPPORT */

#ifdef RAID_DRIVER
        MODULE_RAID,
#endif /*  RAID_DRIVER */
        MODULE_CORE,
        MAX_MODULE_NUMBER
};
#define MAX_POSSIBLE_MODULE_NUMBER              MAX_MODULE_NUMBER

#ifdef USE_NEW_SGTABLE
#include "com_sgd.h"

typedef sgd_tbl_t MV_SG_Table, *PMV_SG_Table;
typedef sgd_t MV_SG_Entry, *PMV_SG_Entry;

#else

struct _MV_SG_Table;
typedef struct _MV_SG_Table MV_SG_Table, *PMV_SG_Table;

struct _MV_SG_Entry;
typedef struct _MV_SG_Entry MV_SG_Entry, *PMV_SG_Entry;



/* SG Table and SG Entry */
struct _MV_SG_Entry
{
	MV_U32 Base_Address;
	MV_U32 Base_Address_High;
	MV_U32 Reserved0;
	MV_U32 Size;
};

struct _MV_SG_Table
{
	MV_U8 Max_Entry_Count;
	MV_U8 Valid_Entry_Count;
	MV_U8 Flag;
	MV_U8 Reserved0;
	MV_U32 Byte_Count;
	PMV_SG_Entry Entry_Ptr;
};

#endif

#ifdef SIMULATOR
#define	MV_REQ_COMMON_SCSI	          0
#define	MV_REQ_COMMON_XOR	          1
#endif	/*SIMULATOR*/

/* 
 * MV_Request is the general request type passed through different modules. 
 * Must be 64 bit aligned.
 */

#define DEV_ID_TO_TARGET_ID(_dev_id)    ((MV_U8)((_dev_id) & 0x00FF))
#define DEV_ID_TO_LUN(_dev_id)                ((MV_U8) (((_dev_id) & 0xFF00) >> 8))
#define __MAKE_DEV_ID(_target_id, _lun)   (((MV_U16)(_target_id)) | (((MV_U16)(_lun)) << 8))

/*Work around free disks suporting temporarily*/
#define RESTORE_FREE_DISK_TID 1

typedef void (*MV_ReqCompletion)(MV_PVOID,PMV_Request);

struct _MV_Request {
#ifdef SIMULATOR
	MV_U32 CommonType;	// please keep it as the first field
#endif	/* SIMULATOR */
#ifdef __RES_MGMT__
	List_Head pool_entry;     /* don't bother, res_mgmt use only */
#endif /* __RES_MGMT__ */
	List_Head Queue_Pointer;
	List_Head Complete_Queue_Pointer;

#if defined(_OS_LINUX) || defined(__QNXNTO__)
	List_Head hba_eh_entry;           /* hba's request queue  */
#endif /* _OS_LINUX || __QNXNTO__ */

	MV_U16 Device_Id;

	MV_U16 Req_Flag;                  /* Check the REQ_FLAG definition */
	MV_U8 Scsi_Status;
	MV_U8 Tag;                        /* Request tag */
	MV_U8 Req_Type;                   /* Check the REQ_TYPE definition */
#ifdef _OS_WINDOWS
	MV_U8 Reserved0[1];
#elif defined(SUPPORT_ERROR_HANDLING) && defined(_OS_LINUX)
	MV_U8 eh_flag;    /* mark a req after it is re-inserted into
			   * waiting_list due to error handling.
			   */
#else
	MV_U8 Reserved0[1]; 
#endif /* _OS_WINDOWS */

	MV_PVOID Cmd_Initiator;           /* Which module(extension pointer) 
					     creates this request. */

	MV_U8 Sense_Info_Buffer_Length;
#if RESTORE_FREE_DISK_TID
	MV_U8 org_target_id;
#else
	MV_U8 Reserved1;
#endif
	MV_U16 SlotNo; 
	MV_U32 Data_Transfer_Length; 

	MV_U8 Cdb[MAX_CDB_SIZE]; 
	MV_PVOID Data_Buffer; 
	MV_PVOID Sense_Info_Buffer;

	MV_SG_Table SG_Table;

	MV_PVOID Org_Req;                /* The original request. */ 

	/* Each module should only use Context to store module information. */
	MV_PVOID Context[MAX_POSSIBLE_MODULE_NUMBER];

	MV_PVOID Scratch_Buffer;          /* pointer to the scratch buffer 
										 that this request used */
	MV_PVOID SG_Buffer;
	MV_PVOID pRaid_Request;

	MV_LBA LBA;
	MV_U32 Sector_Count;
	MV_U32 Cmd_Flag;

	MV_U32 Time_Out;                  /* how many seconds we should wait 
					     before treating request as 
					     timed-out */
	MV_U32 Splited_Count;

#if ERROR_HANDLING_SUPPORT
	MV_U8   bh_eh_flag;
	MV_U8   reserved[3];
	MV_PVOID bh_eh_ctx; /*The type should be MV_PVOID*/
#endif /* ERROR_HANDLING_SUPPORT */
#ifdef _OS_LINUX
	MV_DECLARE_TIMER(eh_timeout);
 #ifdef SUPPORT_REQUEST_TIMER
	MV_PVOID	err_request_ctx;
 #endif
#endif /* _OS_LINUX */
#ifdef _OS_LINUX
#ifdef _32_LEGACY_
	MV_U8 dummy[5];
#endif
#endif /* _OS_LINUX */
	MV_ReqCompletion	Completion; /* call back function */
};

#define MV_REQUEST_SIZE                   sizeof(MV_Request)
/*
 * Request flag is the flag for the MV_Request data structure.
 */
#define REQ_FLAG_LBA_VALID                MV_BIT(0)
#define REQ_FLAG_CMD_FLAG_VALID           MV_BIT(1)
#define REQ_FLAG_RETRY                    MV_BIT(2)
#define REQ_FLAG_INTERNAL_SG              MV_BIT(3)
#ifndef USE_NEW_SGTABLE	
#define REQ_FLAG_USE_PHYSICAL_SG          MV_BIT(4)
#define REQ_FLAG_USE_LOGICAL_SG           MV_BIT(5)
#else
/*temporarily reserve bit 4 & 5 until NEW_SG is enabled fixedly for all OS */
#endif
#define REQ_FLAG_FLUSH                    MV_BIT(6)
#define REQ_FLAG_CONSOLIDATE			  MV_BIT(8)
#define REQ_FLAG_NO_CONSOLIDATE           MV_BIT(9)
#define REQ_FLAG_EXTERNAL				  MV_BIT(10)
#define REQ_FLAG_CORE_SUB                 MV_BIT(11)
#define REQ_FLAG_BYPASS_HYBRID            MV_BIT(12)	// hybrid disk simulation

/*
 * Request Type is the type of MV_Request.
 */
enum {
	/* use a value other than 0, and now they're bit-mapped */
	REQ_TYPE_OS       = 0x01,
	REQ_TYPE_RAID     = 0x02,
	REQ_TYPE_CACHE    = 0x04,
	REQ_TYPE_INTERNAL = 0x08,
	REQ_TYPE_SUBLD    = 0x10,
	REQ_TYPE_SUBBGA   = 0x20,
	REQ_TYPE_MP       = 0x40,
};

/*
 * Command flag is the flag for the CDB command itself 
 */
/* The first 16 bit can be determined by the initiator. */
#define CMD_FLAG_NON_DATA                 MV_BIT(0)  /* 1-non data; 
							0-data command */
#define CMD_FLAG_DMA                      MV_BIT(1)  /* 1-DMA */
#define CMD_FLAG_PIO					  MV_BIT(2)  /* 1-PIO */
#define CMD_FLAG_DATA_IN                  MV_BIT(3)  /* 1-host read data */
#define CMD_FLAG_DATA_OUT                 MV_BIT(4)	 /* 1-host write data */
#define CMD_FLAG_SMART                    MV_BIT(5)  /* 1-SMART command;0-non SMART command*/
#define CMD_FLAG_SMART_ATA_12       MV_BIT(6)  /* SMART ATA_12  */
#define CMD_FLAG_SMART_ATA_16       MV_BIT(7)  /* SMART ATA_16; */

/*
 * The last 16 bit only can be set by the target. Only core driver knows 
 * the device characteristic. 
 */
#define CMD_FLAG_NCQ                      MV_BIT(16)
#define CMD_FLAG_TCQ                      MV_BIT(17)
#define CMD_FLAG_48BIT                    MV_BIT(18)
#define CMD_FLAG_PACKET                   MV_BIT(19)  /* ATAPI packet cmd */
#define CMD_FLAG_SCSI_PASS_THRU           MV_BIT(20)
#define CMD_FLAG_ATA_PASS_THRU            MV_BIT(21)

#ifdef RAID_DRIVER
/* XOR request types */
#define    XOR_REQUEST_WRITE              0
#define    XOR_REQUEST_COMPARE            1
#define    XOR_REQUEST_DMA                2

/* XOR request status */
#define XOR_STATUS_SUCCESS                0
#define XOR_STATUS_INVALID_REQUEST        1
#define XOR_STATUS_ERROR                  2
#define XOR_STATUS_INVALID_PARAMETER      3
#define XOR_SOURCE_SG_COUNT               11  
#ifdef RAID6_MULTIPLE_PARITY
#   define XOR_TARGET_SG_COUNT               3   
#else
#   define XOR_TARGET_SG_COUNT               1   
#endif

typedef MV_U8    XOR_COEF, *PXOR_COEF;        /* XOR Coefficient */

struct _MV_XOR_Request {
#ifdef SIMULATOR
	MV_U32 CommonType;	// please keep it as the first field
#endif	/*SIMULATOR*/
	List_Head Queue_Pointer;

	MV_U16 Device_Id;

	MV_U8 Request_Type;                        
	MV_U8 Request_Status;

	MV_U8 Source_SG_Table_Count;        /* how many items in the 
					       SG_Table_List */
	MV_U8 Target_SG_Table_Count;
#ifdef SOFTWARE_XOR
	MV_U8 Reserved[2];
#else
	MV_U16 SlotNo;
#endif /* SOFTWARE_XOR */

	MV_SG_Table Source_SG_Table_List[XOR_SOURCE_SG_COUNT];
	MV_SG_Table Target_SG_Table_List[XOR_TARGET_SG_COUNT];

	
	XOR_COEF    Coef[XOR_TARGET_SG_COUNT][XOR_SOURCE_SG_COUNT];

	MV_U32 Error_Offset;                 /* byte, not sector */
	MV_PVOID Cmd_Initiator;              /* Which module(extension pointer
						) creates this request. */
#ifdef RAID6_HARDWARE_XOR
	MV_PVOID Context[MAX_POSSIBLE_MODULE_NUMBER];
#else
	MV_PVOID Context;
#endif

	MV_PVOID SG_Buffer;
	void (*Completion)(MV_PVOID, PMV_XOR_Request);    /* callback */
};
#endif /* RAID_DRIVER */

typedef struct _MV_Target_ID_Map
{
	MV_U16   Device_Id;
	MV_U8    Type;                    /* 0:LD, 1:Free Disk */
	MV_U8    Reserved;
} MV_Target_ID_Map, *PMV_Target_ID_Map;

/* Resource type */
enum Resource_Type
{
	RESOURCE_CACHED_MEMORY = 0,
#ifdef SUPPORT_DISCARDABLE_MEM
	RESOURCE_DISCARDABLE_MEMORY,
#endif
	RESOURCE_UNCACHED_MEMORY
};

/* Module event type */
enum Module_Event
{
	EVENT_MODULE_ALL_STARTED = 0,
#ifdef CACHE_MODULE_SUPPORT
	EVENT_DEVICE_CACHE_MODE_CHANGED,
#endif /* CACHE_MODULE_SUPPORT */
#ifdef SUPPORT_DISCARDABLE_MEM
	EVENT_SPECIFY_RUNTIME_DEVICE,
	EVENT_DISCARD_RESOURCE,
#endif
	EVENT_DEVICE_ARRIVAL,
	EVENT_DEVICE_REMOVAL,
	EVENT_LOG_GENERATED,
	EVENT_HOT_PLUG,
};

/* Error_Handling_State */
enum EH_State
{
	EH_NONE = 0,
	EH_ABORT_REQUEST,
	EH_LU_RESET,
	EH_DEVICE_RESET,
	EH_PORT_RESET,
	EH_CHIP_RESET,
	EH_SET_DISK_DOWN
};

typedef enum
{
	EH_REQ_NOP = 0,
	EH_REQ_ABORT_REQUEST,
	EH_REQ_HANDLE_TIMEOUT,
	EH_REQ_RESET_BUS,
	EH_REQ_RESET_CHANNEL,
	EH_REQ_RESET_DEVICE,
	EH_REQ_RESET_ADAPTER
}eh_req_type_t;

struct mod_notif_param {
        MV_PVOID  p_param;
        MV_U16    hi;
        MV_U16    lo;

        /* for event processing */
        MV_U32    event_id;
        MV_U16    dev_id;
        MV_U8     severity_lvl;
        MV_U8     param_count;
};

/*
 * Exposed Functions
 */

/*
 *
 * Miscellaneous Definitions
 *
 */
/* Rounding */

/* Packed */

#define MV_MAX(x,y)        (((x) > (y)) ? (x) : (y))
#define MV_MIN(x,y)        (((x) < (y)) ? (x) : (y))

#define MV_MAX_U64(x, y)   ((((x).value) > ((y).value)) ? (x) : (y))
#define MV_MIN_U64(x, y)   ((((x).value) < ((y).value)) ? (x) : (y))

#define MV_MAX_U8          0xFF
#define MV_MAX_U16         0xFFFF
#define MV_MAX_U32         0xFFFFFFFFL

#ifdef _OS_LINUX
#   define ROUNDING_MASK(x, mask)  (((x)+(mask))&~(mask))
#   define ROUNDING(value, align)  ROUNDING_MASK(value,   \
						 (typeof(value)) (align-1))
#   define OFFSET_OF(type, member) offsetof(type, member)
#else
#   define ROUNDING(value, align)  ( ((value)+(align)-1)/(align)*(align) )
#   define OFFSET_OF(type, member)    ((MV_U32)(MV_PTR_INTEGER)&(((type *) 0)->member))
#   define ALIGN ROUNDING
#endif /* _OS_LINUX */


#endif /* __MV_COM_TYPE_H__ */
