#include "com_define.h"
#include "com_dbg.h"
#include "com_scsi.h"
#include "com_util.h"
#include "com_u64.h"

#if ERROR_HANDLING_SUPPORT
void MV_ZeroMvRequest(PMV_Request pReq)
{
	PMV_SG_Entry pSGEntry;
	MV_U8 maxEntryCount;
	MV_PVOID pSenseBuffer;
	MV_PVOID pbheh_ctx;
#ifdef __RES_MGMT__
	List_Head list;
#endif
	MV_DASSERT(pReq);
	pSGEntry = pReq->SG_Table.Entry_Ptr;
	maxEntryCount = pReq->SG_Table.Max_Entry_Count;
	pSenseBuffer = pReq->Sense_Info_Buffer;
	pbheh_ctx = pReq->bh_eh_ctx;
#ifdef __RES_MGMT__
	list = pReq->pool_entry;
#endif
	MV_ZeroMemory(pReq, MV_REQUEST_SIZE);
#ifdef __RES_MGMT__
	pReq->pool_entry = list;
#endif
	pReq->SG_Table.Entry_Ptr = pSGEntry;
	pReq->SG_Table.Max_Entry_Count = maxEntryCount;
	pReq->Sense_Info_Buffer = pSenseBuffer;
	pReq->bh_eh_ctx = pbheh_ctx;
}
#else
void MV_ZeroMvRequest(PMV_Request pReq)
{
	PMV_SG_Entry pSGEntry;
	MV_U8 maxEntryCount;
	MV_PVOID pSenseBuffer;
#ifdef __RES_MGMT__
	List_Head list;
#endif

	MV_DASSERT(pReq);
	pSGEntry = pReq->SG_Table.Entry_Ptr;
	maxEntryCount = pReq->SG_Table.Max_Entry_Count;
	pSenseBuffer = pReq->Sense_Info_Buffer;
#ifdef __RES_MGMT__
	list = pReq->pool_entry;
#endif
	MV_ZeroMemory(pReq, MV_REQUEST_SIZE);
#ifdef __RES_MGMT__
	pReq->pool_entry = list;
#endif
	pReq->SG_Table.Entry_Ptr = pSGEntry;
	pReq->SG_Table.Max_Entry_Count = maxEntryCount;
	pReq->Sense_Info_Buffer = pSenseBuffer;
}
#endif

void MV_CopySGTable(PMV_SG_Table pTargetSGTable, PMV_SG_Table pSourceSGTable)
{
	pTargetSGTable->Valid_Entry_Count = pSourceSGTable->Valid_Entry_Count;
	pTargetSGTable->Flag = pSourceSGTable->Flag;
	pTargetSGTable->Byte_Count = pSourceSGTable->Byte_Count;
	MV_CopyMemory(pTargetSGTable->Entry_Ptr, pSourceSGTable->Entry_Ptr, 
					sizeof(MV_SG_Entry)*pTargetSGTable->Valid_Entry_Count);

}

#ifdef _OS_BIOS
MV_U64 CPU_TO_BIG_ENDIAN_64(MV_U64 x)

{
	MV_U64 x1;
	ZeroU64(x1);
	x1.parts.low=CPU_TO_BIG_ENDIAN_32(x.parts.high);
	x1.parts.high=CPU_TO_BIG_ENDIAN_32(x.parts.low);
	return x1;
}
#endif	/* #ifdef _OS_BIOS */

MV_BOOLEAN MV_Equals(
	IN MV_PU8		des,
	IN MV_PU8		src,
	IN MV_U8		len							 
)
{
	MV_U8 i;

	for (i=0; i<len; i++){
		if (*des != *src){
			return MV_FALSE;
		}
		des++;
		src++;
	}
	return MV_TRUE;
}
/*
 * SG Table operation
 */
void SGTable_Init(
	OUT PMV_SG_Table pSGTable,
	IN MV_U8 flag
	)
{
/*	pSGTable->Max_Entry_Count = MAX_SG_ENTRY;  set during module init */
	pSGTable->Valid_Entry_Count = 0;
	pSGTable->Flag = flag;
	pSGTable->Byte_Count = 0;
}

#ifndef USE_NEW_SGTABLE
void SGTable_Append(
	OUT PMV_SG_Table pSGTable,
	MV_U32 address,
	MV_U32 addressHigh,
	MV_U32 size
	)
{
	PMV_SG_Entry pSGEntry;

	pSGEntry = &pSGTable->Entry_Ptr[pSGTable->Valid_Entry_Count];

	MV_ASSERT(pSGTable->Valid_Entry_Count < pSGTable->Max_Entry_Count);
	/* 
	 * Workaround hardware issue:
	 * If the transfer size is odd, some request cannot be finished.
	 * Hopefully the workaround won't damage the system.
	 */
#ifdef PRD_SIZE_WORD_ALIGN
	if ( size%2 ) size++;
#endif

	pSGTable->Valid_Entry_Count += 1;
	pSGTable->Byte_Count += size;

	pSGEntry->Base_Address = address;
	pSGEntry->Base_Address_High = addressHigh;
	pSGEntry->Size = size;
	pSGEntry->Reserved0 = 0;
}
#endif

MV_BOOLEAN SGTable_Available(
	IN PMV_SG_Table pSGTable
	)
{
	return (pSGTable->Valid_Entry_Count < pSGTable->Max_Entry_Count);
}

void MV_InitializeTargetIDTable(
	IN PMV_Target_ID_Map pMapTable
	)
{
	MV_FillMemory((MV_PVOID)pMapTable, sizeof(MV_Target_ID_Map)*MV_MAX_TARGET_NUMBER, 0xFF);
}

MV_U16 MV_MapTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 i;
	for (i=0; i<MV_MAX_TARGET_NUMBER; i++) {
		if (pMapTable[i].Type==0xFF) {	/* not mapped yet */
			pMapTable[i].Device_Id = deviceId;
			pMapTable[i].Type = deviceType;
			break;
		}
	}
	return i;
}

MV_U16 MV_MapToSpecificTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				specificId,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	/* first check if the device can be mapped to the specific ID */
	if (pMapTable[specificId].Type==0xFF) {	/* not used yet */
		pMapTable[specificId].Device_Id = deviceId;
		pMapTable[specificId].Type = deviceType;
		return specificId;
	}
	/* cannot mapped to the specific ID */
	/* just map the device to first available ID */
	return MV_MapTargetID(pMapTable, deviceId, deviceType);
}

MV_U16 MV_RemoveTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 i;
	for (i=0; i<MV_MAX_TARGET_NUMBER; i++) {
		if ( (pMapTable[i].Type==deviceType) && (pMapTable[i].Device_Id==deviceId) ) {
			pMapTable[i].Type = 0xFF;
			pMapTable[i].Device_Id = 0xFFFF;
			break;
		}
	}
	return i;
}

MV_U16 MV_GetMappedID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 mappedID;
	for (mappedID=0; mappedID<MV_MAX_TARGET_NUMBER; mappedID++) {
		if ( (pMapTable[mappedID].Type==deviceType) && (pMapTable[mappedID].Device_Id==deviceId) )
			break;
	}
	if (mappedID >= MV_MAX_TARGET_NUMBER)
		mappedID = 0xFFFF;
	else {
		MV_DASSERT(mappedID < MV_MAX_TARGET_NUMBER);
#ifdef SUPPORT_SCSI_PASSTHROUGH
		if (mappedID == VIRTUAL_DEVICE_ID) {
			/* Device is on LUN 1 */
			mappedID |= 0x0100;
		}
#endif /* SUPPORT_SCSI_PASSTHROUGH */
	}
	return mappedID;
}

void MV_DecodeReadWriteCDB(
	IN MV_PU8 Cdb,
	OUT MV_LBA *pLBA,
	OUT MV_U32 *pSectorCount)
{
	MV_LBA tmpLBA;
	MV_U32 tmpSectorCount;

	if ((!SCSI_IS_READ(Cdb[0])) && 
	    (!SCSI_IS_WRITE(Cdb[0])) && 
	    (!SCSI_IS_VERIFY(Cdb[0])))
		return;

	/* This is READ/WRITE command */
	switch (Cdb[0]) {
	case SCSI_CMD_READ_6:
	case SCSI_CMD_WRITE_6:
		tmpLBA.value = (MV_U32)((((MV_U32)(Cdb[1] & 0x1F))<<16) |
					((MV_U32)Cdb[2]<<8) |
					((MV_U32)Cdb[3]));
		tmpSectorCount = (MV_U32)Cdb[4];
		break;
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_VERIFY_10:
		tmpLBA.value = (MV_U32)(((MV_U32)Cdb[2]<<24) | 
					((MV_U32)Cdb[3]<<16) | 
					((MV_U32)Cdb[4]<<8) | 
					((MV_U32)Cdb[5]));
		tmpSectorCount = ((MV_U32)Cdb[7]<<8) | (MV_U32)Cdb[8];
		break;
	case SCSI_CMD_READ_12:
	case SCSI_CMD_WRITE_12:
		tmpLBA.value = (MV_U32)(((MV_U32)Cdb[2]<<24) | 
					((MV_U32)Cdb[3]<<16) | 
					((MV_U32)Cdb[4]<<8) | 
					((MV_U32)Cdb[5]));
		tmpSectorCount = (MV_U32)(((MV_U32)Cdb[6]<<24) |
					  ((MV_U32)Cdb[7]<<16) |
					  ((MV_U32)Cdb[8]<<8) |
					  ((MV_U32)Cdb[9]));
		break;
	case SCSI_CMD_READ_16:
	case SCSI_CMD_WRITE_16:
	case SCSI_CMD_VERIFY_16:
		tmpLBA.parts.high = (MV_U32)(((MV_U32)Cdb[2]<<24) | 
				       ((MV_U32)Cdb[3]<<16) | 
				       ((MV_U32)Cdb[4]<<8) | 
				       ((MV_U32)Cdb[5]));
		tmpLBA.parts.low = (MV_U32)(((MV_U32)Cdb[6]<<24) |
				      ((MV_U32)Cdb[7]<<16) |
				      ((MV_U32)Cdb[8]<<8) |
				      ((MV_U32)Cdb[9]));

		tmpSectorCount = (MV_U32)(((MV_U32)Cdb[10]<<24) |
					  ((MV_U32)Cdb[11]<<16) |
					  ((MV_U32)Cdb[12]<<8) |
					  ((MV_U32)Cdb[13]));
		break;
	default:
		MV_DPRINT(("Unsupported READ/WRITE command [%x]\n", Cdb[0]));
		U64_SET_VALUE(tmpLBA, 0);
		tmpSectorCount = 0;
	}
	*pLBA = tmpLBA;
	*pSectorCount = tmpSectorCount;
}

#ifndef _OS_BIOS
void MV_DumpRequest(PMV_Request pReq, MV_BOOLEAN detail)
{
	MV_DPRINT(("Device[%d] Cdb[%2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x].\n",
		pReq->Device_Id,
		pReq->Cdb[0],
		pReq->Cdb[1],
		pReq->Cdb[2],
		pReq->Cdb[3],
		pReq->Cdb[4],
		pReq->Cdb[5],
		pReq->Cdb[6],
		pReq->Cdb[7],
		pReq->Cdb[8],
		pReq->Cdb[9],
		pReq->Cdb[10],
		pReq->Cdb[11]
		));

	if ( detail )
	{
		MV_DPRINT(("Scsi_Status=0x%x\n", pReq->Scsi_Status));
		MV_DPRINT(("Tag=0x%x\n", pReq->Tag));
		MV_DPRINT(("Data_Transfer_Length=%d\n", pReq->Data_Transfer_Length));
		MV_DPRINT(("Sense_Info_Buffer_Length=%d\n", pReq->Sense_Info_Buffer_Length));
		MV_DPRINT(("Org_Req : %p\n", pReq->Org_Req));
	}
}

#if defined(SUPPORT_RAID6) && defined(RAID_DRIVER)
void MV_DumpXORRequest(PMV_XOR_Request pXORReq, MV_BOOLEAN detail)
{
	MV_U32 i,j;

	MV_PRINT("MV_XOR_Request: Type=0x%x, Source count=%d, Target count=%d\n",
		pXORReq->Request_Type,
		pXORReq->Source_SG_Table_Count,
		pXORReq->Target_SG_Table_Count
		);

	if ( detail )
	{
		MV_PRINT("Source SG table...\n");
		for ( i=0; i<pXORReq->Source_SG_Table_Count; i++ )
			MV_DumpSGTable(&pXORReq->Source_SG_Table_List[i]);

		MV_PRINT("Target SG table...\n");
		for ( i=0; i<pXORReq->Target_SG_Table_Count; i++ )
			MV_DumpSGTable(&pXORReq->Target_SG_Table_List[i]);

		MV_PRINT("Coefficient...\n");
		for ( i=0; i<pXORReq->Target_SG_Table_Count; i++ ) {
			for ( j=0; j<pXORReq->Source_SG_Table_Count; j++ ) {
				MV_PRINT("[%d,%d]=0x%x\n", i, j, pXORReq->Coef[i][j]);
			}
		}
	}
}
#endif /* SUPPORT_RAID6 */

void MV_DumpSGTable(PMV_SG_Table pSGTable)
{
#ifdef USE_NEW_SGTABLE
	sgdt_dump(pSGTable, " ");
#else
	PMV_SG_Entry pSGEntry;
	MV_U32 i;
	MV_PRINT("SG Table: size(0x%x)\n", pSGTable->Byte_Count);
	for ( i=0; i<pSGTable->Valid_Entry_Count; i++ )
	{
		pSGEntry = &pSGTable->Entry_Ptr[i];
		MV_PRINT("%d: addr(0x%x-0x%x), size(0x%x).\n", 
			i, pSGEntry->Base_Address_High, pSGEntry->Base_Address, pSGEntry->Size);
	}
#endif
}

const char* MV_DumpSenseKey(MV_U8 sense)
{
	switch ( sense )
	{
		case SCSI_SK_NO_SENSE:
			return "SCSI_SK_NO_SENSE";
		case SCSI_SK_RECOVERED_ERROR:
			return "SCSI_SK_RECOVERED_ERROR";
		case SCSI_SK_NOT_READY:
			return "SCSI_SK_NOT_READY";
		case SCSI_SK_MEDIUM_ERROR:
			return "SCSI_SK_MEDIUM_ERROR";
		case SCSI_SK_HARDWARE_ERROR:
			return "SCSI_SK_HARDWARE_ERROR";
		case SCSI_SK_ILLEGAL_REQUEST:
			return "SCSI_SK_ILLEGAL_REQUEST";
		case SCSI_SK_UNIT_ATTENTION:
			return "SCSI_SK_UNIT_ATTENTION";
		case SCSI_SK_DATA_PROTECT:
			return "SCSI_SK_DATA_PROTECT";
		case SCSI_SK_BLANK_CHECK:
			return "SCSI_SK_BLANK_CHECK";
		case SCSI_SK_VENDOR_SPECIFIC:
			return "SCSI_SK_VENDOR_SPECIFIC";
		case SCSI_SK_COPY_ABORTED:
			return "SCSI_SK_COPY_ABORTED";
		case SCSI_SK_ABORTED_COMMAND:
			return "SCSI_SK_ABORTED_COMMAND";
		case SCSI_SK_VOLUME_OVERFLOW:
			return "SCSI_SK_VOLUME_OVERFLOW";
		case SCSI_SK_MISCOMPARE:
			return "SCSI_SK_MISCOMPARE";
		default:
			MV_DPRINT(("Unknown sense key 0x%x.\n", sense));
			return "Unknown sense key";
	}
}
#endif	/* #ifndef _OS_BIOS */

static MV_U32  BASEATTR crc_tab[] = {
        0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
        0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
        0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
        0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
        0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
        0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
        0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
        0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
        0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
        0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
        0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
        0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
        0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
        0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
        0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
        0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
        0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
        0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
        0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
        0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
        0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
        0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
        0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
        0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
        0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
        0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
        0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
        0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
        0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
        0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
        0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
        0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
        0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
        0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
        0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
        0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
        0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
        0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
        0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
        0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
        0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
        0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
        0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
        0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
        0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
        0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
        0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
        0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
        0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
        0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
        0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
        0x2d02ef8dL
};

/* Calculate CRC and generate PD_Reference number */
MV_U32 MV_CRC(
	IN	MV_PU8		pData, 
	IN	MV_U16		len
)
{
    MV_U16 i;
    MV_U32 crc = MV_MAX_U32;

    for (i = 0;  i < len;  i ++) {
		crc = crc_tab[(crc ^ pData[i]) & 0xff] ^ (crc >> 8);
    }
    return MV_CPU_TO_BE32(crc);
}

#ifdef MV_DEBUG
void MV_CHECK_OS_SG_TABLE(
	IN PMV_SG_Table pSGTable
	)
{
#ifndef USE_NEW_SGTABLE
	/* Check whether there are duplicated entries pointed to the same physical address. */
	MV_U32 i,j;
	static MV_BOOLEAN assertSGTable = MV_TRUE;

	if ( assertSGTable ) {
		for ( i=0; i<pSGTable->Valid_Entry_Count; i++ ) {
			for ( j=i+1; j<pSGTable->Valid_Entry_Count; j++ ) {
				MV_DASSERT( pSGTable->Entry_Ptr[i].Base_Address!=pSGTable->Entry_Ptr[j].Base_Address );
			}
		}
	}
#endif
}
#endif /* MV_DEBUG */

#ifdef _OS_LINUX

#if 1//def MV_DEBUG
/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */

void list_add_debug(struct list_head *new_list,
			      struct list_head *prev,
			      struct list_head *next)
{
	if (unlikely(next->prev != prev)) {
		MV_DPRINT(( "list_add corruption. next->prev should be "
			"prev (0x%p), but was 0x%p. (next=0x%p).\n",
			prev, next->prev, next));

		MV_DPRINT(( "prev=0x%p, prev->next=0x%p,  prev->prev=0x%p"
			"next=0x%p, next->next=0x%p, next->prev=0x%p.\n",
			prev, prev->next, prev->prev, next, next->next, next->prev));
#ifdef MV_LINUX_KGDB
		MV_DASSERT(0);
#endif

	}
	if (unlikely(prev->next != next)) {
		MV_DPRINT(( "list_add corruption. prev->next should be "
			"next (0x%p), but was 0x%p. (prev=0x%p).\n",
			next, prev->next, prev));

		MV_DPRINT(( "prev=0x%p, prev->next=0x%p,  prev->prev=0x%p,"
			"next=0x%p, next->next=0x%p, next->prev=0x%p.\n",
			prev, prev->next, prev->prev, next, next->next, next->prev));

#ifdef MV_LINUX_KGDB
		MV_DASSERT(0);
#endif

	}
	next->prev = new_list;
	new_list->next = next;
	new_list->prev = prev;
	prev->next = new_list;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
void List_Del(struct list_head *entry)
{
	if (unlikely(entry->prev->next != entry)) {
		MV_DPRINT(("list_del corruption. prev->next should be %p, "
				"but was %p\n", entry, entry->prev->next));
		MV_DPRINT(( "entry->next=%p, entry->next->next=%p,  entry->next->prev=%p.\n"
				"entry->prev=%p, entry->prev->next=%p, entry->prev->prev=%p.\n",
			entry->prev, entry->next->next, entry->next->prev, entry->prev, entry->prev->next, entry->prev->prev));

#ifdef MV_LINUX_KGDB
		MV_DASSERT(0);
#endif
	}
	if (unlikely(entry->next->prev != entry)) {
		MV_DPRINT(( "list_del corruption. next->prev should be %p, "
				"but was %p\n", entry, entry->next->prev));

		MV_DPRINT(("entry->next=%p, entry->next->next=%p,  entry->next->prev=%p.\n"
				"entry->prev=%p, entry->prev->next=%p, entry->prev->prev=%p.\n",
			entry->next, entry->next->next, entry->next->prev, entry->prev, entry->prev->next, entry->prev->prev));

#ifdef MV_LINUX_KGDB
		MV_DASSERT(0);
#endif
	}

	if(likely(entry->prev->next == entry) && likely(entry->next->prev == entry))
		__list_del(entry->prev, entry->next);
	
	entry->next = NULL;
	entry->prev = NULL;
}

#else

void List_Del(List_Head *entry)
{
#ifdef SUPPORT_REQUEST_TIMER
          BUG_ON(!entry);
          BUG_ON(!entry->prev);
          BUG_ON(!entry->next);
#endif
//	list_del(entry);	//Not use kernel function, which set next,prev not NULL but LIST_POISON1 under DEBUG mode
	__list_del(entry->prev, entry->next);
	entry->next = NULL;	
	entry->prev = NULL;

//	if((entry->prev) || (entry->next))
//	__List_Del(entry->prev, entry->next);
//	entry->next = NULL;
//	entry->prev = NULL;
}


#endif


void List_Add(List_Head *new_one, List_Head *head)
{
          BUG_ON(!head);
          BUG_ON(!head->prev);
          BUG_ON(!head->next);
	
          BUG_ON(!new_one);

#ifdef MV_DEBUG
	list_add_debug(new_one, head, head->next);
#else
	list_add(new_one, head);
#endif
        BUG_ON(!new_one->next);
         BUG_ON(!new_one->prev);
         
}

void List_AddTail(List_Head *new_one, List_Head *head)
{
          BUG_ON(!head);
          BUG_ON(!head->prev);
          BUG_ON(!head->next);
	
          BUG_ON(!new_one);

#ifdef MV_DEBUG
	list_add_debug(new_one, head->prev, head);
#else
	list_add_tail(new_one, head);
#endif
        BUG_ON(!new_one->next);
         BUG_ON(!new_one->prev);
         
}



struct list_head *List_GetFirst(struct list_head *head)
{
        struct list_head * one = NULL;
          BUG_ON(!head);
          BUG_ON(!head->prev);
          BUG_ON(!head->next);

	if (unlikely(head->prev->next != head)) {
		MV_DPRINT(( "List_GetFirst corruption. prev->next should be 0x%p, "
				"but was 0x%p\n", head, head->prev->next));

		MV_DPRINT(( "head=0x%p, head->next=0x%p,  head->prev=0x%p,"
			"head->prev->next=0x%p, head->prev->prev=0x%p.\n",
			head, head->next, head->prev,  head->prev->next, head->prev->prev));

#ifdef MV_LINUX_KGDB
		MV_DASSERT(0);
#endif
	}
	if (unlikely(head->next->prev != head)) {
		MV_DPRINT(( "List_GetFirst corruption. next->prev should be 0x%p, "
				"but was 0x%p\n", head, head->next->prev));

		MV_DPRINT(( "head=%p, head->next=%p,  head->prev=%p,"
			"head->next->next=%p, head->next->prev=%p.\n",
			head, head->next, head->prev, head->next->next, head->next->prev));

#ifdef MV_LINUX_KGDB
		MV_DASSERT(0);
#endif
	}

      	if (list_empty(head))
		return NULL;

        one = head->next;
        BUG_ON(!one->next);
        BUG_ON(!one->prev);
      	List_Del(one);

	if (unlikely(head->prev->next != head)) {
		MV_DPRINT(( "List_GetFirst after del corruption. prev->next should be 0x%p, "
				"but was 0x%p\n", head, head->prev->next));
#ifdef MV_LINUX_KGDB
		MV_DASSERT(0);
#endif
	}
	if (unlikely(head->next->prev != head)) {
		MV_DPRINT(("List_GetFirst after del corruption. next->prev should be 0x%p, "
				"but was 0x%p\n", head, head->next->prev));
#ifdef MV_LINUX_KGDB
		MV_DASSERT(0);
#endif
	}
      	
        return one;
}

struct list_head *List_GetLast(struct list_head *head)
{
        struct list_head * one = NULL;
            BUG_ON(!head);
          BUG_ON(!head->prev);
          BUG_ON(!head->next);
        if (list_empty(head))
		return NULL;

        one = head->prev;
         BUG_ON(!one->next);
        BUG_ON(!one->prev);
        List_Del(one);
        return one;
}



#endif

