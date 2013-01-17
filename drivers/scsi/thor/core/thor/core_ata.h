#if !defined(CORE_ATA_H)
#define CORE_ATA_H


/*
 * ATA IDE Command definition 
 */
/* PIO command */
#define ATA_CMD_READ_PIO				0x20
#define ATA_CMD_READ_PIO_EXT			0x24
#define ATA_CMD_READ_PIO_MULTIPLE_EXT 	0x29
#define ATA_CMD_WRITE_PIO				0x30
#define ATA_CMD_WRITE_PIO_EXT			0x34
#define ATA_CMD_WRITE_PIO_MULTIPLE_EXT	0x39

/* DMA read write command */
#define ATA_CMD_READ_DMA				0xC8	/* 28 bit DMA read */
#define ATA_CMD_READ_DMA_QUEUED			0xC7	/* 28 bit TCQ DMA read */
#define ATA_CMD_READ_DMA_EXT			0x25	/* 48 bit DMA read */
#define ATA_CMD_READ_DMA_QUEUED_EXT		0x26	/* 48 bit TCQ DMA read */
#define ATA_CMD_READ_FPDMA_QUEUED		0x60	/* NCQ DMA read: SATA only. Always 48 bit */

#define ATA_CMD_WRITE_DMA				0xCA	
#define ATA_CMD_WRITE_DMA_QUEUED		0xCC
#define ATA_CMD_WRITE_DMA_EXT  			0x35
#define ATA_CMD_WRITE_DMA_QUEUED_EXT	0x36
#define ATA_CMD_WRITE_FPDMA_QUEUED		0x61

/* Identify command */
#define ATA_CMD_IDENTIFY_ATA			0xEC
#define ATA_CMD_IDENTIY_ATAPI			0xA1

#define ATA_CMD_VERIFY					0x40	/* 28 bit read verifty */
#define ATA_CMD_VERIFY_EXT				0x42	/* 48 bit read verify */

#define ATA_CMD_FLUSH					0xE7	/* 28 bit flush */
#define ATA_CMD_FLUSH_EXT				0xEA	/* 48 bit flush */

#define ATA_CMD_PACKET					0xA0
#define ATA_CMD_SMART					0xB0
	#define ATA_CMD_SMART_READ_DATA 					0xD0
	#define ATA_CMD_SMART_ENABLE_ATTRIBUTE_AUTOSAVE	0xD2
	#define ATA_CMD_SMART_EXECUTE_OFFLINE				0xD4
	#define ATA_CMD_SMART_READ_LOG						0xD5
	#define ATA_CMD_SMART_WRITE_LOG 					0xD6
	#define ATA_CMD_ENABLE_SMART				0xD8
	#define ATA_CMD_DISABLE_SMART				0xD9
	#define ATA_CMD_SMART_RETURN_STATUS			0xDA

#define ATA_CMD_SET_FEATURES			0xEF
	#define ATA_CMD_ENABLE_WRITE_CACHE			0x02
	#define ATA_CMD_SET_TRANSFER_MODE			0x03
	#define ATA_CMD_DISABLE_READ_LOOK_AHEAD		0x55
	#define ATA_CMD_DISABLE_WRITE_CACHE			0x82
	#define ATA_CMD_ENABLE_READ_LOOK_AHEAD		0xAA

#define ATA_CMD_STANDBY_IMMEDIATE		0xE0
#define ATA_CMD_SEEK					0x70
#define ATA_CMD_READ_LOG_EXT			0x2F
#ifdef SUPPORT_ATA_SECURITY_CMD

/*	security commmand */
#define ATA_CMD_SEC_PASSWORD			0xF1
#define ATA_CMD_SEC_UNLOCK			0xF2
#define ATA_CMD_SEC_ERASE_PRE			0xF3
#define ATA_CMD_SEC_ERASE_UNIT			0xF4
#define ATA_CMD_SEC_FREEZE_LOCK			0xF5
#define ATA_CMD_SEC_DISABLE_PASSWORD		0xF6

enum _ata_passthru_protocol {
        ATA_PROTOCOL_HARD_RESET                 = 0x00,
        ATA_PROTOCOL_SRST                       = 0x01,
        ATA_PROTOCOL_NON_DATA                   = 0x03,
        ATA_PROTOCOL_PIO_IN                     = 0x04,
        ATA_PROTOCOL_PIO_OUT                    = 0x05,
        ATA_PROTOCOL_DMA                        = 0x06,
        ATA_PROTOCOL_DMA_QUEUED                 = 0x07,
        ATA_PROTOCOL_DEVICE_DIAG                = 0x08,
        ATA_PROTOCOL_DEVICE_RESET               = 0x09,
        ATA_PROTOCOL_UDMA_IN                    = 0x0A,
        ATA_PROTOCOL_UDMA_OUT                   = 0x0B,
        ATA_PROTOCOL_FPDMA                      = 0x0C,
        ATA_PROTOCOL_RTN_INFO                   = 0x0F,
};
#define ATA_CMD_READ_PIO				0x20
#define ATA_CMD_READ_PIO_MULTIPLE                       0xC4
#define ATA_CMD_READ_PIO_EXT			0x24
#define ATA_CMD_READ_PIO_MULTIPLE_EXT 	0x29
#define ATA_CMD_WRITE_PIO				0x30
#define ATA_CMD_WRITE_PIO_MULTIPLE                      0xC5
#define ATA_CMD_WRITE_PIO_EXT			0x34
#define ATA_CMD_WRITE_PIO_MULTIPLE_EXT	0x39
#define ATA_CMD_WRITE_PIO_MULTIPLE_FUA_EXT              0xCE

#define IS_ATA_MULTIPLE_READ_WRITE(x) \
        ((x == ATA_CMD_READ_PIO_MULTIPLE)\
        ||(x == ATA_CMD_READ_PIO_MULTIPLE_EXT)\
        ||(x == ATA_CMD_WRITE_PIO_MULTIPLE)\
        ||(x == ATA_CMD_WRITE_PIO_MULTIPLE_EXT)\
        ||(x == ATA_CMD_WRITE_PIO_MULTIPLE_FUA_EXT))

enum {
	SG_CDB2_TLEN_NODATA	= 0 << 0,
	SG_CDB2_TLEN_FEAT	= 1 << 0,
	SG_CDB2_TLEN_NSECT	= 2 << 0,

	SG_CDB2_TLEN_BYTES	= 0 << 2,
	SG_CDB2_TLEN_SECTORS	= 1 << 2,

	SG_CDB2_TDIR_TO_DEV	= 0 << 3,
	SG_CDB2_TDIR_FROM_DEV	= 1 << 3,

	SG_CDB2_CHECK_COND	= 1 << 5,
};
#define SG_READ			0
#define SG_WRITE		1
#define SG_PIO			0
#define SG_DMA			1

enum {
	/*
	 * These (redundantly) specify the category of the request
	 */
	TASKFILE_CMD_REQ_NODATA	= 0,	/* ide: IDE_DRIVE_TASK_NO_DATA */
	TASKFILE_CMD_REQ_IN	= 2,	/* ide: IDE_DRIVE_TASK_IN */
	TASKFILE_CMD_REQ_OUT	= 3,	/* ide: IDE_DRIVE_TASK_OUT */
	TASKFILE_CMD_REQ_RAW_OUT= 4,	/* ide: IDE_DRIVE_TASK_RAW_WRITE */
	/*
	 * These specify the method of transfer (pio, dma, multi, ..)
	 */
	TASKFILE_DPHASE_NONE	= 0,	/* ide: TASKFILE_IN */
	TASKFILE_DPHASE_PIO_IN	= 1,	/* ide: TASKFILE_IN */
	TASKFILE_DPHASE_PIO_OUT	= 4,	/* ide: TASKFILE_OUT */
};
#define SG_ATA_LBA48		1
#define SG_ATA_PROTO_NON_DATA	( 3 << 1)
#define SG_ATA_PROTO_PIO_IN	( 4 << 1)
#define SG_ATA_PROTO_PIO_OUT	( 5 << 1)
#define SG_ATA_PROTO_DMA	( 6 << 1)
#define SG_ATA_PROTO_UDMA_IN	(11 << 1) /* not yet supported in libata */
#define SG_ATA_PROTO_UDMA_OUT	(12 << 1) /* not yet supported in libata */

#endif



struct _ATA_TaskFile;
typedef struct _ATA_TaskFile ATA_TaskFile, *PATA_TaskFile;

struct _ATA_TaskFile {
	MV_U8	Features;
	MV_U8	Sector_Count;
	MV_U8	LBA_Low;
	MV_U8	LBA_Mid;
	MV_U8	LBA_High;
	MV_U8	Device;
	MV_U8	Command;

	MV_U8	Control;

	/* extension */
	MV_U8	Feature_Exp;
	MV_U8	Sector_Count_Exp;
	MV_U8	LBA_Low_Exp;
	MV_U8	LBA_Mid_Exp;
	MV_U8	LBA_High_Exp;
};

/* ATA device identify frame */
typedef struct _ATA_Identify_Data {
	MV_U16 General_Config;							/*	0	*/
	MV_U16 Obsolete0;								/*	1	*/
	MV_U16 Specific_Config;							/*	2	*/
	MV_U16 Obsolete1;								/*	3	*/
	MV_U16 Retired0[2];								/*	4-5	*/
	MV_U16 Obsolete2;								/*	6	*/
	MV_U16 Reserved0[2];							/*	7-8	*/
	MV_U16 Retired1;								/*	9	*/
	MV_U8 Serial_Number[20];				        /*	10-19	*/
	MV_U16 Retired2[2];								/*	20-21	*/
	MV_U16 Obsolete3;								/*	22	*/
	MV_U8 Firmware_Revision[8];						/*	23-26	*/
	MV_U8 Model_Number[40];							/*	27-46	*/
	MV_U16 Maximum_Block_Transfer;					/*	47	*/
	MV_U16 Reserved1;								/*	48	*/
	MV_U16 Capabilities[2];							/*	49-50	*/
	MV_U16 Obsolete4[2];							/*	51-52	*/
	MV_U16 Fields_Valid;							/*	53	*/
	MV_U16 Obsolete5[5];							/*	54-58	*/
	MV_U16 Current_Multiple_Sector_Setting;			/*	59	*/
	MV_U16 User_Addressable_Sectors[2];				/*	60-61	*/
	MV_U16 ATAPI_DMADIR;							/*	62	*/
	MV_U16 Multiword_DMA_Modes;						/*	63	*/
	MV_U16 PIO_Modes;								/*	64	*/
	MV_U16 Minimum_Multiword_DMA_Cycle_Time;		/*	65	*/
	MV_U16 Recommended_Multiword_DMA_Cycle_Time;	/*	66	*/
	MV_U16 Minimum_PIO_Cycle_Time;					/*	67	*/
	MV_U16 Minimum_PIO_Cycle_Time_IORDY;			/*	68	*/
	MV_U16 Reserved2[2];							/*	69-70	*/
	MV_U16 ATAPI_Reserved[4];						/*	71-74	*/
	MV_U16 Queue_Depth;								/*	75	*/
	MV_U16 SATA_Capabilities;						/*	76	*/
	MV_U16 SATA_Reserved;							/*	77	*/
	MV_U16 SATA_Feature_Supported;					/*	78	*/
	MV_U16 SATA_Feature_Enabled;					/*	79	*/
 	MV_U16 Major_Version;							/*	80	*/
	MV_U16 Minor_Version;							/*	81	*/
	MV_U16 Command_Set_Supported[2];				/*	82-83	*/
	MV_U16 Command_Set_Supported_Extension;			/*	84	*/
	MV_U16 Command_Set_Enabled[2];					/*	85-86	*/
	MV_U16 Command_Set_Default;						/*	87	*/
	MV_U16 UDMA_Modes;								/*	88	*/
	MV_U16 Time_For_Security_Erase;					/*	89	*/
	MV_U16 Time_For_Enhanced_Security_Erase;		/*	90	*/
	MV_U16 Current_Advanced_Power_Manage_Value;		/*	91	*/
	MV_U16 Master_Password_Revision;				/*	92	*/
	MV_U16 Hardware_Reset_Result;					/*	93	*/
	MV_U16 Acoustic_Manage_Value;					/*	94	*/
	MV_U16 Stream_Minimum_Request_Size;				/*	95	*/
	MV_U16 Stream_Transfer_Time_DMA;				/*	96	*/
	MV_U16 Stream_Access_Latency;					/*	97	*/
	MV_U16 Stream_Performance_Granularity[2];		/*	98-99	*/
	MV_U16 Max_LBA[4];								/*	100-103	*/
	MV_U16 Stream_Transfer_Time_PIO;				/*	104	*/	
	MV_U16 Reserved3;								/*	105	*/
	MV_U16 Physical_Logical_Sector_Size;			/*	106	*/
	MV_U16 Delay_Acoustic_Testing;					/*	107	*/
	MV_U16 NAA;										/*	108	*/
	MV_U16 Unique_ID1;								/*	109	*/
	MV_U16 Unique_ID2;								/*	110	*/
	MV_U16 Unique_ID3;								/*	111	*/
	MV_U16 Reserved4[4];							/*	112-115	*/
	MV_U16 Reserved5;								/*	116	*/
	MV_U16 Words_Per_Logical_Sector[2];				/*	117-118	*/
	MV_U16 Reserved6[8];							/*	119-126	*/
	MV_U16 Removable_Media_Status_Notification;		/*	127	*/
	MV_U16 Security_Status;							/*	128	*/
	MV_U16 Vendor_Specific[31];						/*	129-159	*/
	MV_U16 CFA_Power_Mode;							/*	160	*/
	MV_U16 Reserved7[15];							/*	161-175	*/
	MV_U16 Current_Media_Serial_Number[30];			/*	176-205	*/
	MV_U16 Reserved8[49];							/*	206-254	*/
	MV_U16 Integrity_Word;							/*	255	*/
} ATA_Identify_Data, *PATA_Identify_Data;

#define ATA_REGISTER_DATA			0x08
#define ATA_REGISTER_ERROR			0x09
#define ATA_REGISTER_FEATURES		0x09
#define ATA_REGISTER_SECTOR_COUNT	0x0A
#define ATA_REGISTER_LBA_LOW		0x0B
#define ATA_REGISTER_LBA_MID		0x0C
#define ATA_REGISTER_LBA_HIGH		0x0D
#define ATA_REGISTER_DEVICE			0x0E
#define ATA_REGISTER_STATUS			0x0F
#define ATA_REGISTER_COMMAND		0x0F

#define ATA_REGISTER_ALT_STATUS		0x16
#define ATA_REGISTER_DEVICE_CONTROL	0x16

#endif

