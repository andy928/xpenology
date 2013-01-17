#ifndef  __MV_COM_API_H__
#define  __MV_COM_API_H__

#define MAX_CDB_SIZE                           16  

/* CDB definitions */
#define APICDB0_ADAPTER                        0xF0
#define APICDB0_LD                             0xF1
#define APICDB0_BLOCK                          0xF2
#define APICDB0_PD                             0xF3
#define APICDB0_EVENT                          0xF4
#define APICDB0_DBG                            0xF5
#define APICDB0_FLASH                          0xF6

/* for Adapter */
#define APICDB1_ADAPTER_GETCOUNT               0
#define APICDB1_ADAPTER_GETINFO                (APICDB1_ADAPTER_GETCOUNT + 1)
#define APICDB1_ADAPTER_GETCONFIG              (APICDB1_ADAPTER_GETCOUNT + 2)
#define APICDB1_ADAPTER_SETCONFIG              (APICDB1_ADAPTER_GETCOUNT + 3)
#define APICDB1_ADAPTER_POWER_STATE_CHANGE     (APICDB1_ADAPTER_GETCOUNT + 4)
#define APICDB1_ADAPTER_MAX                    (APICDB1_ADAPTER_GETCOUNT + 5)

/* for LD */
#define APICDB1_LD_CREATE                      0
#define APICDB1_LD_GETMAXSIZE                  (APICDB1_LD_CREATE + 1)
#define APICDB1_LD_GETINFO                     (APICDB1_LD_CREATE + 2)
#define APICDB1_LD_GETTARGETLDINFO             (APICDB1_LD_CREATE + 3)
#define APICDB1_LD_DELETE                      (APICDB1_LD_CREATE + 4)
#define APICDB1_LD_GETSTATUS                   (APICDB1_LD_CREATE + 5)
#define APICDB1_LD_GETCONFIG                   (APICDB1_LD_CREATE + 6)
#define APICDB1_LD_SETCONFIG                   (APICDB1_LD_CREATE + 7)
#define APICDB1_LD_STARTREBUILD                (APICDB1_LD_CREATE + 8)
#define APICDB1_LD_STARTCONSISTENCYCHECK       (APICDB1_LD_CREATE + 9)
#define APICDB1_LD_STARTINIT                   (APICDB1_LD_CREATE + 10)
#define APICDB1_LD_STARTMIGRATION              (APICDB1_LD_CREATE + 11)
#define APICDB1_LD_BGACONTROL                  (APICDB1_LD_CREATE + 12)
#define APICDB1_LD_WIPEMDD                     (APICDB1_LD_CREATE + 13)
#define APICDB1_LD_GETSPARESTATUS              (APICDB1_LD_CREATE + 14)
#define APICDB1_LD_SETGLOBALSPARE              (APICDB1_LD_CREATE + 15)
#define APICDB1_LD_SETLDSPARE                  (APICDB1_LD_CREATE + 16)
#define APICDB1_LD_REMOVESPARE                 (APICDB1_LD_CREATE + 17)
#define APICDB1_LD_HD_SETSTATUS                (APICDB1_LD_CREATE + 18)
#define APICDB1_LD_SHUTDOWN                    (APICDB1_LD_CREATE + 19)
#define APICDB1_LD_HD_FREE_SPACE_INFO          (APICDB1_LD_CREATE + 20)
#define APICDB1_LD_HD_GETMBRINFO               (APICDB1_LD_CREATE + 21)
#define APICDB1_LD_SIZEOF_MIGRATE_TARGET       (APICDB1_LD_CREATE + 22)
#define APICDB1_LD_TARGET_LUN_TYPE			   (APICDB1_LD_CREATE + 23)
#define APICDB1_LD_HD_MPCHECK                  (APICDB1_LD_CREATE + 24)
#define APICDB1_LD_HD_GETMPSTATUS              (APICDB1_LD_CREATE + 25)
#define APICDB1_LD_HD_GET_RCT_COUNT            (APICDB1_LD_CREATE + 26)
#define APICDB1_LD_HD_RCT_REPORT               (APICDB1_LD_CREATE + 27)
#define APICDB1_LD_HD_START_DATASCRUB          (APICDB1_LD_CREATE + 28)	// temp
#define APICDB1_LD_HD_BGACONTROL	           (APICDB1_LD_CREATE + 29)	// temp
#define APICDB1_LD_HD_GETBGASTATUS             (APICDB1_LD_CREATE + 30)	// temp
// Added reserved for future expansion so that MRU/CLI can be backward compatible with drier.
#define APICDB1_LD_RESERVED1				   (APICDB1_LD_CREATE + 31)	
#define APICDB1_LD_RESERVED2				   (APICDB1_LD_CREATE + 32)	
#define APICDB1_LD_RESERVED3				   (APICDB1_LD_CREATE + 33)	
#define APICDB1_LD_RESERVED4				   (APICDB1_LD_CREATE + 34)	
#define APICDB1_LD_RESERVED5				   (APICDB1_LD_CREATE + 35)	

#define APICDB1_LD_MAX                         (APICDB1_LD_CREATE + 36)

/* for PD */
#define APICDB1_PD_GETHD_INFO                  0
#define APICDB1_PD_GETEXPANDER_INFO            (APICDB1_PD_GETHD_INFO + 1)
#define APICDB1_PD_GETPM_INFO                  (APICDB1_PD_GETHD_INFO + 2)
#define APICDB1_PD_GETSETTING                  (APICDB1_PD_GETHD_INFO + 3)
#define APICDB1_PD_SETSETTING                  (APICDB1_PD_GETHD_INFO + 4)
#define APICDB1_PD_BSL_DUMP                    (APICDB1_PD_GETHD_INFO + 5)
#define APICDB1_PD_RESERVED1				   (APICDB1_PD_GETHD_INFO + 6)	// not used
#define APICDB1_PD_RESERVED2				   (APICDB1_PD_GETHD_INFO + 7)	// not used
#define APICDB1_PD_GETSTATUS                   (APICDB1_PD_GETHD_INFO + 8)
#define APICDB1_PD_GETHD_INFO_EXT              (APICDB1_PD_GETHD_INFO + 9)	// APICDB1_PD_GETHD_INFO extension
#define APICDB1_PD_MAX                         (APICDB1_PD_GETHD_INFO + 10)

/* Sub command for APICDB1_PD_SETSETTING */
#define APICDB4_PD_SET_WRITE_CACHE_OFF         0
#define APICDB4_PD_SET_WRITE_CACHE_ON          1
#define APICDB4_PD_SET_SMART_OFF               2
#define APICDB4_PD_SET_SMART_ON                3
#define APICDB4_PD_SMART_RETURN_STATUS         4
#define APICDB4_PD_SET_SPEED_3G				   5
#define APICDB4_PD_SET_SPEED_1_5G			   6

/* for Block */
#define APICDB1_BLOCK_GETINFO                  0
#define APICDB1_BLOCK_HD_BLOCKIDS              (APICDB1_BLOCK_GETINFO + 1)
#define APICDB1_BLOCK_MAX                      (APICDB1_BLOCK_GETINFO + 2)

/* for event */
#define APICDB1_EVENT_GETEVENT                 0
#define APICDB1_EVENT_MAX                      (APICDB1_EVENT_GETEVENT + 1)

/* for DBG */
#define APICDB1_DBG_PDWR                       0
#define APICDB1_DBG_MAP                        (APICDB1_DBG_PDWR + 1)
#define APICDB1_DBG_LDWR					   (APICDB1_DBG_PDWR + 2)
#define APICDB1_DBG_ADD_RCT_ENTRY              (APICDB1_DBG_PDWR + 3)
#define APICDB1_DBG_REMOVE_RCT_ENTRY           (APICDB1_DBG_PDWR + 4)
#define APICDB1_DBG_REMOVE_ALL_RCT             (APICDB1_DBG_PDWR + 5)
#define APICDB1_DBG_MAX                        (APICDB1_DBG_PDWR + 6)

/* for FLASH */
#define APICDB1_FLASH_BIN                      0

#if defined(SUPPORT_CSMI)
/* for SDI(HP CSMI) */
#   define APICDB0_CSMI_CORE                      0xF7
#   define APICDB0_CSMI_RAID                      0xF8
#   define APICDB1_CSMI_GETINFO                   0
#   define APICDB1_CSMI_HD_BLOCKIDS               (APICDB1_BLOCK_GETINFO + 1)
#   define APICDB1_CSMI_MAX                       (APICDB1_BLOCK_GETINFO + 2)

#   define CSMI_DRIVER_NAME                       "mv64xx"
#   define CSMI_DRIVER_DESC                       "64xx:SAS Controller"
#endif

/* for passthru commands
	Cdb[0]: APICDB0_PASS_THRU_CMD_SCSI or APICDB0_PASS_THRU_CMD_ATA
	Cdb[1]: APICDB1 (Data flow)
	Cdb[2]: TargetID MSB
	Cdb[3]: TargetID LSB
	Cdb[4]-Cdb[15]: SCSI/ATA command is embedded here
		SCSI command: SCSI command Cdb bytes is in the same order as the spec
		ATA Command:
			Features = pReq->Cdb[0];
			Sector_Count = pReq->Cdb[1];
			LBA_Low = pReq->Cdb[2];
			LBA_Mid = pReq->Cdb[3];
			LBA_High = pReq->Cdb[4];
			Device = pReq->Cdb[5];
			Command = pReq->Cdb[6];

			if necessary:
			Feature_Exp = pReq->Cdb[7];
			Sector_Count_Exp = pReq->Cdb[8];
			LBA_Low_Exp = pReq->Cdb[9];
			LBA_Mid_Exp = pReq->Cdb[10];
			LBA_High_Exp = pReq->Cdb[11];
*/
#define APICDB0_PASS_THRU_CMD_SCSI			      0xFA
#define APICDB0_PASS_THRU_CMD_ATA				  0xFB

#define APICDB1_SCSI_NON_DATA					  0x00
#define APICDB1_SCSI_PIO_IN						  0x01 // goes with Read Long
#define APICDB1_SCSI_PIO_OUT					  0x02 // goes with Write Long

#define APICDB1_ATA_NON_DATA					  0x00
#define APICDB1_ATA_PIO_IN						  0x01
#define APICDB1_ATA_PIO_OUT						  0x02

#ifdef _OS_LINUX
#ifdef NEW_IO_CONTORL_PATH  
		#define API_IOCTL_DEFAULT_FUN				0x1981
#else
		#define API_IOCTL_DEFAULT_FUN				0x00
#endif
#define	API_IOCTL_GET_VIRTURL_ID				(API_IOCTL_DEFAULT_FUN + 1)
#define	API_IOCTL_GET_HBA_COUNT				(API_IOCTL_DEFAULT_FUN + 2)
#define	API_IOCTL_LOOKUP_DEV					(API_IOCTL_DEFAULT_FUN + 3)
#define API_IOCTL_CHECK_VIRT_DEV                        (API_IOCTL_DEFAULT_FUN + 4)
#define API_IOCTL_MAX                       				(API_IOCTL_DEFAULT_FUN + 5)
#endif


#endif /*  __MV_COM_API_H__ */

