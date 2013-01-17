typedef union {
    struct {
        MV_U32 low;
        MV_U32 high;
    } parts;
    MV_U8       b[8];
    MV_U16      w[4];
    MV_U32      d[2];        
} SAS_ADDR, *PSAS_ADDR;

typedef struct _PHY_TUNING {
	MV_U8	AMP:4;						  /* 4 bits,  amplitude  */
	MV_U8	Pre_Emphasis:3;				  /* 3 bits,  pre-emphasis */
	MV_U8	Reserved_1bit_1:1;				  /* 1 bit,   reserved space */
    MV_U8	Drive_En:6;						/* 6 bits,	drive enable */
	MV_U8	Pre_Half_En:1;					/* 1 bit,	Half Pre-emphasis Enable*/
	MV_U8	Reserved_1bit_2:1;				/* 1 bit, 	reserved space */
    MV_U8	Reserved[2];					/* 2 bytes, reserved space */
} PHY_TUNING, *PPHY_TUNING;


/* HBA_FLAG_XX */
#define HBA_FLAG_INT13_ENABLE				MV_BIT(0)	//int 13h enable/disable
#define HBA_FLAG_SILENT_MODE_ENABLE			MV_BIT(1)	//silent mode enable/disable
#define HBA_FLAG_ERROR_STOP					MV_BIT(2)	//if error then stop

#define NVRAM_DATA_MAJOR_VERSION		0
#define NVRAM_DATA_MINOR_VERSION		1

/* 
	HBA_Info_Page is saved in Flash/NVRAM, total 256 bytes.
	The data area is valid only Signature="MRVL".
	If any member fills with 0xFF, the member is invalid.
*/
typedef struct _HBA_Info_Page{
	// Dword 0
	MV_U8     	Signature[4];                 	/* 4 bytes, structure signature,should be "MRVL" at first initial */

	// Dword 1
	MV_U8     	MinorRev;                 		/* 2 bytes, NVRAM data structure version */
	MV_U8		MajorRev;
	MV_U16    	Next_Page;					  	/* 2 bytes, For future data structure expansion, 0 is for NULL pointer and current page is the last page. */

	// Dword 2
	MV_U8     	Major;                   		/* 1 byte,  BIOS major version */
	MV_U8     	Minor;                  		/* 1 byte,	BIOS minor version */
	MV_U8     	OEM_Num;                     	/* 1 byte,  OEM number */
	MV_U8     	Build_Num;                    	/* 1 byte,  Build number */

	// Dword 3
	MV_U8     	Page_Code;					  	/* 1 byte,  eg. 0 for the 1st page  */
	MV_U8     	Max_PHY_Num;				  	/* 1 byte,   maximum PHY number */
	MV_U8		Reserved2[2];

	// Dword 4
	MV_U32     	HBA_Flag;                     	/* 
													4 bytes, should be 0x0000,0000 at first initial
	                                                                HBA flag:  refers to HBA_FLAG_XX
	                                                                bit 0   --- HBA_FLAG_BBS_ENABLE
	                                                                bit 1   --- HBA_FLAG_SILENT_MODE_ENABLE
	                                                                bit 2   --- HBA_FLAG_ERROR_STOP
	                                                                bit 3   --- HBA_FLAG_INT13_ENABLE
	                                                                bit 4   --- HBA_FLAG_ERROR_STOP
	                                                                bit 5   --- HBA_FLAG_ERROR_PASS
	                                                                bit 6   --- HBA_FLAG_SILENT_MODE_ENABLE
	                                           */
	// Dword 5	                                              
	MV_U32     	Boot_Device;					/* 4 bytes, select boot device */
												/* for ata device, it is CRC of the serial number + model number. */
												/* for sas device, it is CRC of sas address */
												/* for VD, it is VD GUI */

	// Dword 6-8											  
	MV_U32     	Reserved3[3];				  	/* 12 bytes, reserved	*/

	// Dword 9-13
	MV_U8     	Serial_Num[20];				  	/* 20 bytes, controller serial number */

	// Dword 14-29
	SAS_ADDR	SAS_Address[8];               /* 64 bytes, SAS address for each port */

	// Dword 30-43
	MV_U8     	Reserved4[56];                  /* 56 bytes, reserve space for future,initial as 0xFF */   

	// Dword 44-45
	MV_U8     	PHY_Rate[8];                  	/* 8 bytes,  0:  1.5G, 1: 3.0G, should be 0x01 at first initial */

	// Dword 46-53
	PHY_TUNING    PHY_Tuning[8];				/* 32 bytes, PHY tuning parameters for each PHY*/

	// Dword 54-62
	MV_U32     	Reserved5[9];                 	/* 9 dword, reserve space for future,initial as 0xFF */

	// Dword 63
	MV_U8     	Reserved6[3];                 	/* 3 bytes, reserve space for future,initial as 0xFF */
	MV_U8     	Check_Sum;                    	/* 1 byte,   checksum for this structure,Satisfy sum of every 8-bit value of this structure */
}HBA_Info_Page, *pHBA_Info_Page;			/* total 256 bytes */

#define FLASH_PARAM_SIZE 	(sizeof(HBA_Info_Page))
#define ODIN_FLASH_SIZE		0x40000  				//.256k bytes
#define PARAM_OFFSET		ODIN_FLASH_SIZE - 0x100 //.255k bytes

MV_U8 mvui_init_param(MV_PVOID This, pHBA_Info_Page pHBAInfo);//get initial data from flash

MV_U8	mvCaculateChecksum(MV_PU8	Address, MV_U32 Size);
MV_U8	mvVerifyChecksum(MV_PU8	Address, MV_U32 Size);


