#ifndef __MV_COM_STRUCT_H__
#define __MV_COM_STRUCT_H__

#include "com_define.h"

#define GET_ALL                                 0xFF
#define ID_UNKNOWN                              0x7F

#define MAX_NUM_ADAPTERS                        2

#ifndef _OS_BIOS
/* The following MAX number are to support ALL products. */
#define MAX_HD_SUPPORTED_API                    128
#define MAX_EXPANDER_SUPPORTED_API              16
#define MAX_PM_SUPPORTED_API                    8
#define MAX_LD_SUPPORTED_API                    32
#define MAX_BLOCK_SUPPORTED_API                 512
#define MAX_BLOCK_PER_HD_SUPPORTED_API          16
#endif /* _OS_BIOS */

#define MAX_BGA_RATE                            0xFA
#define MAX_MEDIAPATROL_RATE                    0xFF

#ifndef MV_GUID_SIZE
#define MV_GUID_SIZE                            8
#endif /* MV_GUID_SIZE */

#define LD_MAX_NAME_LENGTH                      16

#define CACHE_WRITEBACK_ENABLE                  0
#define CACHE_WRITETHRU_ENABLE                  1
#define CACHE_ADAPTIVE_ENABLE                   2
#define CACHE_WRITE_POLICY_FILTER               (CACHE_WRITEBACK_ENABLE | \
						 CACHE_WRITETHRU_ENABLE | \
						 CACHE_ADAPTIVE_ENABLE)
#define CACHE_LOOKAHEAD_ENABLE                  MV_BIT(2)

//The Flags of Eroor Handling
#define EH_READ_VERIFY_REQ_ERROR		MV_BIT(0)
#define EH_WRITE_REQ_ERROR			MV_BIT(1)
#define EH_WRITE_RECOVERY_ERROR			MV_BIT(2)
#define EH_MEDIA_ERROR				MV_BIT(3)
#define EH_TIMEOUT_ERROR			MV_BIT(4)

#define CONSISTENCYCHECK_ONLY                   0
#define CONSISTENCYCHECK_FIX                    1

#define INIT_QUICK                              0    //Just initialize first part size of LD
#define INIT_FULLFOREGROUND                     1    //Initialize full LD size
#define INIT_FULLBACKGROUND                     2    //Initialize full LD size background
#define INIT_NONE                               3

#define INIT_QUICK_WITHOUT_EVENT				0xf	 // Used for QUICK INIT but set cdb[5]to 0xf so driver won't send event. 

#define BGA_CONTROL_START                       0
#define BGA_CONTROL_RESTART                     1
#define BGA_CONTROL_PAUSE                       2
#define BGA_CONTROL_RESUME                      3
#define BGA_CONTROL_ABORT                       4
#define BGA_CONTROL_COMPLETE                    5
#define BGA_CONTROL_IN_PROCESS                  6
#define BGA_CONTROL_TERMINATE_IMMEDIATE         7
#define BGA_CONTROL_AUTO_PAUSE                  8

#define LD_STATUS_FUNCTIONAL                    0
#define LD_STATUS_DEGRADE                       1
#define LD_STATUS_DELETED                       2
#define LD_STATUS_MISSING                       3 /* LD missing in system. */
#define LD_STATUS_OFFLINE                       4
#define LD_STATUS_PARTIALLYOPTIMAL              5 /* r6 w/ 2 pd, 1 hd drops */
#define LD_STATUS_INVALID                       0xFF

#define LD_BGA_NONE                             0
#define LD_BGA_REBUILD                          MV_BIT(0)
#define LD_BGA_CONSISTENCY_FIX                  MV_BIT(1)
#define LD_BGA_CONSISTENCY_CHECK                MV_BIT(2)
#define LD_BGA_INIT_QUICK                       MV_BIT(3)
#define LD_BGA_INIT_BACK                        MV_BIT(4)
#define LD_BGA_MIGRATION                        MV_BIT(5)
#define LD_BGA_INIT_FORE                        MV_BIT(6)                     

#define LD_BGA_STATE_NONE                       0
#define LD_BGA_STATE_RUNNING                    1
#define LD_BGA_STATE_ABORTED                    2
#define LD_BGA_STATE_PAUSED                     3
#define LD_BGA_STATE_AUTOPAUSED                 4
#define LD_BGA_STATE_DDF_PENDING                MV_BIT(7)

#define LD_MODE_RAID0                           0x0
#define LD_MODE_RAID1                           0x1
#define LD_MODE_RAID5                           0x5
#define LD_MODE_RAID6                           0x6
#define LD_MODE_JBOD                            0x0f
#define LD_MODE_RAID10                          0x10
#define LD_MODE_RAID1E                          0x11
#define LD_MODE_RAID50                          0x50
#define LD_MODE_RAID60                          0x60
#define LD_MODE_UNKNOWN							0xFF

#define HD_WIPE_MDD                             0
#define HD_WIPE_FORCE                           1

#define ROUNDING_SCHEME_NONE                    0     /* no rounding */
#define ROUNDING_SCHEME_1GB                     1     /* 1 GB rounding */
#define ROUNDING_SCHEME_10GB                    2     /* 10 GB rounding */

#define DEVICE_TYPE_NONE                        0
#define DEVICE_TYPE_HD                          1		//  DT_DIRECT_ACCESS_BLOCK
#define DEVICE_TYPE_PM                          2
#define DEVICE_TYPE_EXPANDER                    3		// DT_EXPANDER
#define DEVICE_TYPE_TAPE						4		// DT_SEQ_ACCESS
#define DEVICE_TYPE_PRINTER						5		// DT_PRINTER
#define DEVICE_TYPE_PROCESSOR					6		// DT_PROCESSOR
#define DEVICE_TYPE_WRITE_ONCE					7 		// DT_WRITE_ONCE
#define DEVICE_TYPE_CD_DVD						8		// DT_CD_DVD
#define DEVICE_TYPE_OPTICAL_MEMORY				9 		// DT_OPTICAL_MEMORY
#define DEVICE_TYPE_MEDIA_CHANGER				10		// DT_MEDIA_CHANGER
#define DEVICE_TYPE_ENCLOSURE					11		// DT_ENCLOSURE
#define DEVICE_TYPE_PORT                        0xFF	// DT_STORAGE_ARRAY_CTRL

#define HD_STATUS_FREE                          MV_BIT(0)
#define HD_STATUS_ASSIGNED                      MV_BIT(1)
#define HD_STATUS_SPARE                         MV_BIT(2)
#define HD_STATUS_OFFLINE                       MV_BIT(3)
#define HD_STATUS_SMARTCHECKING                 MV_BIT(4)
#define HD_STATUS_MP                            MV_BIT(5)

#define HD_BGA_STATE_NONE                       LD_BGA_STATE_NONE
#define HD_BGA_STATE_RUNNING                    LD_BGA_STATE_RUNNING
#define HD_BGA_STATE_ABORTED                    LD_BGA_STATE_ABORTED
#define HD_BGA_STATE_PAUSED                     LD_BGA_STATE_PAUSED
#define HD_BGA_STATE_AUTOPAUSED                 LD_BGA_STATE_AUTOPAUSED

#define HD_BGA_TYPE_NONE						0
#define HD_BGA_TYPE_MP							1
#define HD_BGA_TYPE_DATASCRUB					2

#define GLOBAL_SPARE_DISK                       MV_BIT(2)

#define PD_DDF_VALID                            MV_BIT(0)
#define PD_DISK_VALID                           MV_BIT(1)
#define PD_DDF_CLEAN                            MV_BIT(2)
#define PD_NEED_UPDATE                          MV_BIT(3)
#define PD_MBR_VALID                            MV_BIT(4)
#define PD_NEED_FLUSH                           MV_BIT(5)
#define PD_CLEAR_MBR                            MV_BIT(6)
#if ERROR_HANDLING_SUPPORT
#define PD_RCT_NEED_UPDATE             			MV_BIT(7)
#endif

#define PD_STATE_ONLINE                         MV_BIT(0)
#define PD_STATE_FAILED                         MV_BIT(1)
#define PD_STATE_REBUILDING                     MV_BIT(2)
#define PD_STATE_TRANSITION                     MV_BIT(3)
#define PD_STATE_SMART_ERROR                    MV_BIT(4)
#define PD_STATE_READ_ERROR                     MV_BIT(5)
#define PD_STATE_MISSING                        MV_BIT(6)

#define HD_STATUS_SETONLINE                     0
#define HD_STATUS_SETOFFLINE                    1
#define HD_STATUS_INVALID                       0xFF

// Definition used for old driver.
#define HD_TYPE_SATA                            MV_BIT(0)
#define HD_TYPE_PATA                            MV_BIT(1)
#define HD_TYPE_SAS                             MV_BIT(2)
#define HD_TYPE_ATAPI                           MV_BIT(3)
#define HD_TYPE_TAPE                            MV_BIT(4)
#define HD_TYPE_SES                             MV_BIT(5)

// PD's Protocol/Connection type (used by new driver)
#define DC_ATA		MV_BIT(0)
#define DC_SCSI		MV_BIT(1)
#define DC_SERIAL	MV_BIT(2)
#define DC_PARALLEL	MV_BIT(3)
#define DC_ATAPI	MV_BIT(4)		// used by core driver to prepare FIS

// PD's Device type defined in SCSI-III specification (used by new driver)
#define DT_DIRECT_ACCESS_BLOCK	0x00
#define DT_SEQ_ACCESS			0x01
#define DT_PRINTER				0x02
#define DT_PROCESSOR			0x03
#define DT_WRITE_ONCE			0x04
#define DT_CD_DVD				0x05
#define DT_OPTICAL_MEMORY		0x07
#define DT_MEDIA_CHANGER		0x08
#define DT_STORAGE_ARRAY_CTRL	0x0C
#define DT_ENCLOSURE			0x0D
// The following are defined by Marvell
#define DT_EXPANDER				0x20
#define DT_PM					0x21



#define HD_FEATURE_NCQ                          MV_BIT(0)	/* Capability */
#define HD_FEATURE_TCQ                          MV_BIT(1)
#define HD_FEATURE_1_5G                         MV_BIT(2)
#define HD_FEATURE_3G                           MV_BIT(3)
#define HD_FEATURE_WRITE_CACHE                  MV_BIT(4)
#define HD_FEATURE_48BITS                       MV_BIT(5)
#define HD_FEATURE_SMART                        MV_BIT(6)

#define HD_SPEED_1_5G							1		// current 
#define HD_SPEED_3G								2

#define EXP_SSP                                 MV_BIT(0)
#define EXP_STP                                 MV_BIT(1)
#define EXP_SMP                                 MV_BIT(2)

#define HD_DMA_NONE                             0
#define HD_DMA_1                                1
#define HD_DMA_2                                2
#define HD_DMA_3                                3
#define HD_DMA_4                                4
#define HD_DMA_5                                5
#define HD_DMA_6                                6
#define HD_DMA_7                                7
#define HD_DMA_8                                8
#define HD_DMA_9                                9

#define HD_PIO_NONE                             0
#define HD_PIO_1                                1
#define HD_PIO_2                                2
#define HD_PIO_3                                3
#define HD_PIO_4                                4
#define HD_PIO_5                                5

#define HD_XCQ_OFF                              0
#define HD_NCQ_ON                               1
#define HD_TCQ_ON                               2

#define SECTOR_LENGTH                           512
#define SECTOR_WRITE                            0
#define SECTOR_READ                             1

#define DBG_LD2HD                               0
#define DBG_HD2LD                               1

#define DRIVER_LENGTH                           1024*16
#define FLASH_DOWNLOAD                          0xf0
#define FLASH_UPLOAD                            0xf
#define    FLASH_TYPE_CONFIG                    0
#define    FLASH_TYPE_BIN                       1
#define    FLASH_TYPE_BIOS                      2
#define    FLASH_TYPE_FIRMWARE                  3

#define BLOCK_INVALID                           0
#define BLOCK_VALID                             MV_BIT(0)
#define BLOCK_ASSIGNED                          MV_BIT(1)

#ifdef _OS_BIOS
#define FREE_BLOCK(Flags)       (Flags&(BLOCK_VALID) == Flags)
#define ASSIGN_BLOCK(Flags)     (Flags&(BLOCK_VALID|BLOCK_ASSIGNED) == Flags)
#define INVALID_BLOCK(Flags)    (Flags&(BLOCK_VALID|BLOCK_ASSIGNED) == 0) 
#endif /* _OS_BIOS */

/* Target device type */
#define TARGET_TYPE_LD                    0
#define TARGET_TYPE_FREE_PD               1

/*
#define BLOCK_STATUS_NORMAL                     0
#define BLOCK_STATUS_REBUILDING                 MV_BIT(0)
#define BLOCK_STATUS_CONSISTENTCHECKING         MV_BIT(1)
#define BLOCK_STATUS_INITIALIZING               MV_BIT(2)
#define BLOCK_STATUS_MIGRATING                  MV_BIT(3)
#define BLOCK_STATUS_OFFLINE                    MV_BIT(4)
*/
#define MAX_WIDEPORT_PHYS                       8

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _Link_Endpoint 
{
	MV_U16      DevID;
	MV_U8       DevType;         /* Refer to DEVICE_TYPE_xxx, (additional
					type like EDGE_EXPANDER and 
					FANOUT_EXPANDER might be added). */
	MV_U8       PhyCnt;          /* Number of PHYs for this endpoint.  
					Greater than 1 if it is wide port. */
	MV_U8       PhyID[MAX_WIDEPORT_PHYS];    /* Assuming wide port has 
						    max of 8 PHYs. */
	MV_U8       SAS_Address[8];  /* Filled with 0 if not SAS device. */
	MV_U8       Reserved1[8];
} Link_Endpoint, * PLink_Endpoint;

typedef struct _Link_Entity 
{
	Link_Endpoint    Parent;
	MV_U8            Reserved[8];
	Link_Endpoint    Self;
} Link_Entity,  *PLink_Entity;

typedef struct _Version_Info
{
	MV_U32        VerMajor;
	MV_U32        VerMinor;
	MV_U32        VerOEM;
	MV_U32        VerBuild;
}Version_Info, *PVersion_Info;

#define BASE_ADDRESS_MAX_NUM                    6

#define SUPPORT_LD_MODE_RAID0                   MV_BIT(0)
#define SUPPORT_LD_MODE_RAID1                   MV_BIT(1)
#define SUPPORT_LD_MODE_RAID10                  MV_BIT(2)
#define SUPPORT_LD_MODE_RAID1E                  MV_BIT(3)
#define SUPPORT_LD_MODE_RAID5                   MV_BIT(4)
#define SUPPORT_LD_MODE_RAID6                   MV_BIT(5)
#define SUPPORT_LD_MODE_RAID50                  MV_BIT(6)
#define SUPPORT_LD_MODE_JBOD                    MV_BIT(7)

#define FEATURE_BGA_REBUILD_SUPPORT             MV_BIT(0)
#define FEATURE_BGA_BKINIT_SUPPORT				MV_BIT(1)
#define FEATURE_BGA_SYNC_SUPPORT				MV_BIT(2)
#define FEATURE_BGA_MIGRATION_SUPPORT           MV_BIT(3)
#define FEATURE_BGA_MEDIAPATROL_SUPPORT         MV_BIT(4)

typedef struct _Adapter_Info 
{
	Version_Info    DriverVersion;
	Version_Info    BIOSVersion;
	MV_U64          Reserved1[2];     /* Reserve for firmware */

	MV_U32          SystemIOBusNumber;
	MV_U32          SlotNumber;
	MV_U32          InterruptLevel;
	MV_U32          InterruptVector;
    
	MV_U16          VenID;
	MV_U16          SubVenID;
	MV_U16          DevID;
	MV_U16          SubDevID;
    
	MV_U8           PortCount;        /* How many ports, like 4 ports,  
					     or 4S1P. */
	MV_U8           PortSupportType;  /* Like SATA port, SAS port, 
					     PATA port, use MV_BIT */
	MV_U8           Features;         /* Feature bits.  See FEATURE_XXX.
					     If corresponding bit is set, 
					     that feature is supported. */
	MV_BOOLEAN      AlarmSupport;
	MV_U8           RevisionID;		/* Chip revision */
	MV_U8           Reserved2[11];
    
	MV_U8           MaxTotalBlocks;    
	MV_U8           MaxBlockPerPD;
	MV_U8           MaxHD;
	MV_U8           MaxExpander;
	MV_U8           MaxPM;
	MV_U8           MaxLogicalDrive;
	MV_U16          LogicalDriverMode;

	MV_U8           WWN[8];            /* For future VDS use. */
} Adapter_Info, *PAdapter_Info;

typedef struct _Adapter_Config {
	MV_BOOLEAN      AlarmOn;
	MV_BOOLEAN      AutoRebuildOn;
	MV_U8           BGARate;
	MV_BOOLEAN      PollSMARTStatus;
	MV_U8           MediaPatrolRate;
	MV_U8           Reserved[3];
} Adapter_Config, *PAdapter_Config;

typedef struct _HD_Info
{
	Link_Entity     Link;             /* Including self DevID & DevType */
	MV_U8           AdapterID;
	MV_U8           Status;           /* Refer to HD_STATUS_XXX */
	MV_U8           HDType;           /* HD_Type_xxx, replaced by new driver with ConnectionType & DeviceType */
	MV_U8           PIOMode;		  /* Max PIO mode */
	MV_U8           MDMAMode;		  /* Max MDMA mode */
	MV_U8           UDMAMode;		  /* Max UDMA mode */
	MV_U8           ConnectionType;	  /* DC_XXX, ConnectionType & DeviceType in new driver to replace HDType above */
	MV_U8           DeviceType;	      /* DT_XXX */

	MV_U32          FeatureSupport;   /* Support 1.5G, 3G, TCQ, NCQ, and
					     etc, MV_BIT related */
	MV_U8           Model[40];
	MV_U8           SerialNo[20];
	MV_U8           FWVersion[8];
	MV_U64          Size;             /* unit: 1KB */
	MV_U8           WWN[8];          /* ATA/ATAPI-8 has such definitions 
					     for the identify buffer */
	MV_U8           CurrentPIOMode;		/* Current PIO mode */
	MV_U8           CurrentMDMAMode;	/* Current MDMA mode */
	MV_U8           CurrentUDMAMode;	/* Current UDMA mode */
	MV_U8			Reserved3[5];
//	MV_U32			FeatureEnable;		

	MV_U8           Reserved4[80];
}HD_Info, *PHD_Info;

typedef struct _HD_MBR_Info
{
	MV_U8           HDCount;
	MV_U8           Reserved[7];
	MV_U16          HDIDs[MAX_HD_SUPPORTED_API];    
	MV_BOOLEAN      hasMBR[MAX_HD_SUPPORTED_API];
} HD_MBR_Info, *PHD_MBR_Info;


typedef struct _HD_FreeSpaceInfo
{
	MV_U16          ID;               /* ID should be unique*/
	MV_U8           AdapterID;
	MV_U8           Reserved[4];
	MV_BOOLEAN      isFixed;

	MV_U64          Size;             /* unit: 1KB */
}HD_FreeSpaceInfo, *PHD_FreeSpaceInfo;

typedef struct _HD_Block_Info
{
	MV_U16          ID;               /* ID in the HD_Info*/
	MV_U8           Type;             /* Refer to DEVICE_TYPE_xxx */
	MV_U8           Reserved1[5];

	/* Free is 0xff */
	MV_U16          BlockIDs[MAX_BLOCK_PER_HD_SUPPORTED_API];  
}HD_Block_Info, *PHD_Block_Info;

typedef struct _Exp_Info
{
	Link_Entity       Link;            /* Including self DevID & DevType */
	MV_U8             AdapterID;
	MV_BOOLEAN        Configuring;      
	MV_BOOLEAN        RouteTableConfigurable;
	MV_U8             PhyCount;
	MV_U16            ExpChangeCount;
	MV_U16            MaxRouteIndexes;
	MV_U8             VendorID[8+1];
	MV_U8             ProductID[16+1];
	MV_U8             ProductRev[4+1];
	MV_U8             ComponentVendorID[8+1];
	MV_U16            ComponentID;
	MV_U8             ComponentRevisionID;
	MV_U8             Reserved1[17];
}Exp_Info, * PExp_Info;

typedef  struct _PM_Info{
	Link_Entity       Link;           /* Including self DevID & DevType */
	MV_U8             AdapterID;
	MV_U8             ProductRevision;
	MV_U8             PMSpecRevision; /* 10 means 1.0, 11 means 1.1 */
	MV_U8             NumberOfPorts;
	MV_U16            VendorId;
	MV_U16            DeviceId;
	MV_U8             Reserved1[8];
}PM_Info, *PPM_Info;

typedef struct _HD_CONFIG
{
	MV_BOOLEAN        WriteCacheOn;   /* 1: enable write cache */
	MV_BOOLEAN        SMARTOn;        /* 1: enable S.M.A.R.T */
	MV_BOOLEAN        Online;         /* 1: to set HD online */
	MV_U8             DriveSpeed;	  // For SATA & SAS.  HD_SPEED_1_5G, HD_SPEED_3G etc
	MV_U8             Reserved[2];
	MV_U16            HDID;
}HD_Config, *PHD_Config;

typedef struct  _HD_STATUS
{
	MV_BOOLEAN        SmartThresholdExceeded;        
	MV_U8             Reserved[1];
	MV_U16            HDID;
}HD_Status, *PHD_Status;

typedef struct  _SPARE_STATUS
{
	MV_U16            HDID;
	MV_U16            LDID;
	MV_U8             Status;         /* HD_STATUS_SPARE */
	MV_U8             Reserved[3];
}Spare_Status, *PSpare_Status;

typedef struct  _BSL{
	MV_U64            LBA;            /* Bad sector LBA for the HD. */

	MV_U32            Count;          /* How many serial bad sectors */
	MV_BOOLEAN        Flag;           /* Fake bad sector or not. */
	MV_U8             Reserved[3];
}BSL,*PBSL;

typedef struct _BLOCK_INFO
{
	MV_U16            ID;
	MV_U16            HDID;           /* ID in the HD_Info */
	MV_U16            Flags;          /* Refer to BLOCK_XXX definition */
	MV_U16            LDID;           /* Belong to which LD */

	MV_U8             Status;         /* Refer to BLOCK_STATUS_XXX*/
	MV_U8             Reserved[3];
	MV_U32            ReservedSpaceForMigration;	/* Space reserved for migration */

	MV_U64            StartLBA;       /* unit: 512 bytes */
	MV_U64            Size;           /* unit: 512 bytes, including ReservedSpaceForMigration */
}Block_Info, *PBlock_Info;

typedef struct _LD_Info
{
	MV_U16            ID;
	MV_U8             Status;         /* Refer to LD_STATUS_xxx */
	MV_U8             BGAStatus;      /* Refer to LD_BGA_STATE_xxx */
	MV_U16            StripeBlockSize;/* unit: 512 bytes */
	MV_U8             RaidMode;            
	MV_U8             HDCount;

	MV_U8             CacheMode;      /* Default is CacheMode_Default, 
					     see above */
	MV_U8             LD_GUID[MV_GUID_SIZE];
	MV_U8             SectorCoefficient; /* (sector size) 1=>512 (default)
						, 2=>1024, 4=>2048, 8=>4096 */
	MV_U8             AdapterID;
	MV_U8             Reserved[5];

	MV_U64            Size;           /* LD size, unit: 512 bytes */

	MV_U8             Name[LD_MAX_NAME_LENGTH];

	MV_U16            BlockIDs[MAX_HD_SUPPORTED_API];        /* 32 */
/* 
 * According to BLOCK ID, to get the related HD ID, then WMRU can 
 * draw the related graph like above.
 */
	MV_U8             SubLDCount;     /* for raid 10, 50,60 */
	MV_U8             NumParityDisk;  /* For RAID 6. */
	MV_U8             Reserved1[6];
}LD_Info, *PLD_Info;

typedef struct _Create_LD_Param
{
	MV_U8             RaidMode;
	MV_U8             HDCount;
	MV_U8             RoundingScheme; /* please refer to the definitions
					     of  ROUNDING_SCHEME_XXX. */
	MV_U8             SubLDCount;     /* for raid 10,50,60 */
	MV_U16            StripeBlockSize;/*In sectors unit: 1KB */
	MV_U8             NumParityDisk;  /* For RAID 6. */
	MV_U8             CachePolicy;    /* please refer to the definitions
					     of CACHEMODE_XXXX. */

	MV_U8             InitializationOption;/* please refer to the 
						 definitions of INIT_XXXX. */
	MV_U8             SectorCoefficient; /* (sector size) 1=>512 
						(default), 2=>1024, 4=>2048,
						8=>4096 */
	MV_U16            LDID;               /* ID of the LD to be migrated 
						 or expanded */
	MV_U8             Reserved2[4];

	MV_U16            HDIDs[MAX_HD_SUPPORTED_API];    /* 32 */
	MV_U8             Name[LD_MAX_NAME_LENGTH];

	MV_U64            Size;           /* size of LD in sectors */
} Create_LD_Param, *PCreate_LD_Param;

typedef struct _LD_STATUS
{
	MV_U8            Status;          /* Refer to LD_STATUS_xxx */
	MV_U8            Bga;             /* Refer to LD_BGA_xxx */
	MV_U16           BgaPercentage;   /* xx% */
	MV_U8            BgaState;        /* Refer to LD_BGA_STATE_xxx */
	MV_U8            Reserved[1];
	MV_U16           LDID;
} LD_Status, *PLD_Status;

typedef struct    _LD_Config
{
	MV_U8            CacheMode;        /* See definition 4.4.1 
					      CacheMode_xxx */
	MV_U8            Reserved1;        
	MV_BOOLEAN       AutoRebuildOn;    /* 1- AutoRebuild On */
	MV_U8            Status;
	MV_U16           LDID;
	MV_U8            Reserved2[2];

	MV_U8            Name[LD_MAX_NAME_LENGTH];
}LD_Config, * PLD_Config;

// Giving TargetID and LUN, returns it Type and DeviceID.  If returned Type or DeviceID is 0xFF, not found.
typedef struct    _TargetLunType
{
	MV_U8            AdapterID;
	MV_U8            TargetID;       
	MV_U8            Lun;        
	MV_U8            Type;		// TARGET_TYPE_LD or TARGET_TYPE_FREE_PD
	MV_U16           DeviceID;	// LD ID or PD ID depends on Type  
	MV_U8            Reserved[30];
}TargetLunType, * PTargetLunType;

typedef struct _HD_MPSTATUS
{
	MV_U16            HDID;
	MV_U16            LoopCount;      /* loop count */
	MV_U16            ErrorCount;     /* error detected during media patrol */
	MV_U16            Percentage;     /* xx% */
	MV_U8             Status;         /* Refer to HD_BGA_STATE_xxx */
	MV_U8             Type;
	MV_U8             Reserved[50];  
}HD_MPStatus, *PHD_MPStatus;

typedef struct _HD_BGA_STATUS
{
	MV_U16            HDID;
	MV_U16            Percentage;     /* xx% */
	MV_U8             Bga;             /* Refer to HD_BGA_TYPE_xxx */
	MV_U8             Status;          /* Refer to HD_STATUS_xxx */
	MV_U8             BgaStatus;         /* Refer to HD_BGA_STATE_xxx */
	MV_U8             Reserved[1];  
}HD_BGA_Status, *PHD_BGA_Status;

typedef struct _RCT_Record{
	MV_LBA lba;
	MV_U32 sec;	//sector count
	MV_U8   flag;
	MV_U8   rev[3];
}RCT_Record, *PRCT_Record;

typedef struct _DBG_DATA
{
	MV_U64            LBA;
	MV_U64            Size;
	MV_U8             Data[SECTOR_LENGTH];
}DBG_Data, *PDBG_Data;

typedef struct _DBG_HD
{
	MV_U64            LBA;
	MV_U16            HDID;
	MV_BOOLEAN        isUsed;
	MV_U8             Reserved[5];
}DBG_HD;

typedef struct _DBG_MAP
{
	MV_U64            LBA;
	MV_U16            LDID;
	MV_BOOLEAN        isUsed;
	MV_U8             Reserved[5];
	DBG_HD            HDs[MAX_HD_SUPPORTED_API];
}DBG_Map, *PDBG_Map;

#ifdef CACHE_MODULE_SUPPORT
typedef struct _LD_CACHE_STATUS
{
	MV_BOOLEAN       IsOnline;
	MV_U8            CachePolicy;
	MV_U16           StripeUnitSize;
	MV_U32           StripeSize;
	MV_U8            Sector_Coefficient;
	MV_U8            RAID_Level;
	MV_U8            Reserved[6];
	MV_LBA	         MAX_LBA;
#ifdef _DDR_BBU_ENABLE
	MV_U8				LD_GUID[MV_GUID_SIZE];
#endif
}
LD_CACHE_STATUS, *PLD_CACHE_STATUS;
#endif /* CACHE_MODULE_SUPPORT */

#define MAX_PASS_THRU_DATA_BUFFER_SIZE (SECTOR_LENGTH+128)

typedef struct {
	// We put Data_Buffer[] at the very beginning of this structure because SCSI commander did so.
	MV_U8			Data_Buffer[MAX_PASS_THRU_DATA_BUFFER_SIZE];  // set by driver if read, by application if write
	MV_U8			Reserved1[128];
    MV_U32			Data_Length;	// set by driver if read, by application if write 
	MV_U16			DevId;		//	PD ID (used by application only)
	MV_U8			CDB_Type;	// define a CDB type for each CDB category (used by application only)
	MV_U8			Reserved2;
	MV_U32			lba;
	MV_U8			Reserved3[64];
} PassThrough_Config, * PPassThorugh_Config;


#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif /*  __MV_COM_STRUCT_H__ */
