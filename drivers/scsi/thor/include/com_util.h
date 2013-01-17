#ifndef __MV_COM_UTIL_H__
#define __MV_COM_UTIL_H__

#include "com_define.h"
#include "com_type.h"

#define MV_ZeroMemory(buf, len)           memset(buf, 0, len)
#define MV_FillMemory(buf, len, pattern)  memset(buf, pattern, len)
#define MV_CopyMemory(dest, source, len)  memcpy(dest, source, len)

void MV_ZeroMvRequest(PMV_Request pReq);
void MV_CopySGTable(PMV_SG_Table pTargetSGTable, PMV_SG_Table pSourceSGTable);

MV_BOOLEAN MV_Equals(MV_PU8 des, MV_PU8 src, MV_U8 len);

#ifndef _OS_BIOS
#define	U64_ASSIGN(x,y)				  	((x).value = (y))
#define	U64_ASSIGN_U64(x,y)			  	((x).value = (y).value)
#define	U64_COMP_U64(x,y)			  	((x) == (y).value)
#define U64_COMP_U64_VALUE(x,y)			((x).value == (y).value)
#define U32_ASSIGN_U64(v64, v32)		((v64).value = (v32))
#define	U64_SHIFT_LEFT(v64, v32)		((v64).value=(v64).value << (v32))
#define	U64_SHIFT_RIGHT(v64, v32)		((v64).value=(v64).value >> (v32))
#define U64_ZERO_VALUE(v64)				((v64).value = 0)
#else
#define	U64_ASSIGN(x,y)					((x) = (y))
#define	U64_ASSIGN_U64(x,y)				((x) = (y))
#define	U64_COMP_U64(x,y)			  	(U64_COMPARE_U64(x, y)==0)
#define U64_COMP_U64_VALUE(x,y)			(U64_COMPARE_U64(x, y)==0)
#define U32_ASSIGN_U64(v64, v32)		do { (v64).parts.low = v32; (v64).parts.high = 0; } while(0);
#define	U64_SHIFT_LEFT(v64, v32)		do { (v64).parts.low=(v64).parts.low << v32; v64.parts.high = 0; } while(0);
#define	U64_SHIFT_RIGHT(v64, v32)		do { (v64).parts.low=(v64).parts.low >> v32; v64.parts.high = 0; } while(0);
#define U64_ZERO_VALUE(v64)				do { (v64).parts.low = (v64).parts.high = 0; } while(0);
#endif

#define MV_SWAP_32(x)                             \
           (((MV_U32)((MV_U8)(x)))<<24 |          \
            ((MV_U32)((MV_U8)((x)>>8)))<<16 |     \
            ((MV_U32)((MV_U8)((x)>>16)))<<8 |     \
            ((MV_U32)((MV_U8)((x)>>24))) )
#define MV_SWAP_64(x)                             \
           (((_MV_U64) (MV_SWAP_32((x).parts.low))) << 32 | \
	    MV_SWAP_32((x).parts.high))
#define MV_SWAP_16(x)                             \
           (((MV_U16) ((MV_U8) (x))) << 8 |       \
	    (MV_U16) ((MV_U8) ((x) >> 8)))

#if !defined(_OS_LINUX) && !defined(__QNXNTO__)
#   ifndef __MV_BIG_ENDIAN__

#      define MV_CPU_TO_BE32(x)     MV_SWAP_32(x)
#      define MV_CPU_TO_BE64(x)     MV_SWAP_64(x)
#      define MV_CPU_TO_BE16(x)     MV_SWAP_16(x)
#      define MV_BE16_TO_CPU(x)     MV_SWAP_16(x)
#      define MV_BE32_TO_CPU(x)     MV_SWAP_32(x)
#      define MV_BE64_TO_CPU(x)     MV_SWAP_64(x)

#      define MV_CPU_TO_LE16(x)     x
#      define MV_CPU_TO_LE32(x)     x
#      define MV_CPU_TO_LE64(x)     x
#      define MV_LE16_TO_CPU(x)     x
#      define MV_LE32_TO_CPU(x)     x
#      define MV_LE64_TO_CPU(x)     x

#   else  /* __MV_BIG_ENDIAN__ */

#      define MV_CPU_TO_BE32(x)     x
#      define MV_CPU_TO_BE64(x)     x
#      define MV_CPU_TO_BE16(x)     x
#      define MV_BE16_TO_CPU(x)     x
#      define MV_BE32_TO_CPU(x)     x
#      define MV_BE64_TO_CPU(x)     x

#      define MV_CPU_TO_LE16(x)     MV_SWAP_16(x)
#      define MV_CPU_TO_LE32(x)     MV_SWAP_32(x)
#      define MV_CPU_TO_LE64(x)     MV_SWAP_64(x)
#      define MV_LE16_TO_CPU(x)     MV_SWAP_16(x)
#      define MV_LE32_TO_CPU(x)     MV_SWAP_32(x)
#      define MV_LE64_TO_CPU(x)     MV_SWAP_64(x)

#   endif /* __MV_BIG_ENDIAN__ */

#else /* !_OS_LINUX */

#define MV_CPU_TO_LE16      cpu_to_le16
#define MV_CPU_TO_LE32      cpu_to_le32
#define MV_CPU_TO_LE64(x)   cpu_to_le64((x).value)
#define MV_CPU_TO_BE16      cpu_to_be16
#define MV_CPU_TO_BE32      cpu_to_be32
#define MV_CPU_TO_BE64(x)   cpu_to_be64((x).value)

#define MV_LE16_TO_CPU      le16_to_cpu
#define MV_LE32_TO_CPU      le32_to_cpu
#define MV_LE64_TO_CPU(x)   le64_to_cpu((x).value)
#define MV_BE16_TO_CPU      be16_to_cpu
#define MV_BE32_TO_CPU      be32_to_cpu
#define MV_BE64_TO_CPU(x)   be64_to_cpu((x).value)

#endif /* !_OS_LINUX */

/*
 * big endian bit-field structs that are larger than a single byte 
 * need swapping
 */
#ifdef __MV_BIG_ENDIAN__
#define MV_CPU_TO_LE16_PTR(pu16)        \
   *((MV_PU16)(pu16)) = MV_CPU_TO_LE16(*(MV_PU16) (pu16))
#define MV_CPU_TO_LE32_PTR(pu32)        \
   *((MV_PU32)(pu32)) = MV_CPU_TO_LE32(*(MV_PU32) (pu32))

#define MV_LE16_TO_CPU_PTR(pu16)        \
   *((MV_PU16)(pu16)) = MV_LE16_TO_CPU(*(MV_PU16) (pu16))
#define MV_LE32_TO_CPU_PTR(pu32)        \
   *((MV_PU32)(pu32)) = MV_LE32_TO_CPU(*(MV_PU32) (pu32))
# else  /* __MV_BIG_ENDIAN__ */
#define MV_CPU_TO_LE16_PTR(pu16)        /* Nothing */
#define MV_CPU_TO_LE32_PTR(pu32)        /* Nothing */
#define MV_LE16_TO_CPU_PTR(pu32)
#define MV_LE32_TO_CPU_PTR(pu32)
#endif /* __MV_BIG_ENDIAN__ */

/* definitions - following macro names are used by RAID module 
   must keep consistent */
#define CPU_TO_BIG_ENDIAN_16(x)        MV_CPU_TO_BE16(x)
#define CPU_TO_BIG_ENDIAN_32(x)        MV_CPU_TO_BE32(x)
#define CPU_TO_BIG_ENDIAN_64(x)        MV_CPU_TO_BE64(x)

void SGTable_Init(
    OUT PMV_SG_Table pSGTable,
    IN MV_U8 flag
    );

#ifndef USE_NEW_SGTABLE
void SGTable_Append(
    OUT PMV_SG_Table pSGTable,
    MV_U32 address,
    MV_U32 addressHigh,
    MV_U32 size
    );
#else
#define SGTable_Append sgdt_append
#endif

MV_BOOLEAN SGTable_Available(
    IN PMV_SG_Table pSGTable
    );

void MV_InitializeTargetIDTable(
    IN PMV_Target_ID_Map pMapTable
    );

MV_U16 MV_MapTargetID(
    IN PMV_Target_ID_Map    pMapTable,
    IN MV_U16                deviceId,
    IN MV_U8                deviceType
    );

MV_U16 MV_MapToSpecificTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				specificId,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	);

MV_U16 MV_RemoveTargetID(
    IN PMV_Target_ID_Map    pMapTable,
    IN MV_U16                deviceId,
    IN MV_U8                deviceType
    );

MV_U16 MV_GetMappedID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	);

void MV_DecodeReadWriteCDB(
	IN MV_PU8 Cdb,
	OUT MV_LBA *pLBA,
	OUT MV_U32 *pSectorCount);

#define MV_SetLBAandSectorCount(pReq) do {								\
	MV_DecodeReadWriteCDB(pReq->Cdb, &pReq->LBA, &pReq->Sector_Count);	\
	pReq->Req_Flag |= REQ_FLAG_LBA_VALID;								\
} while (0)

void MV_DumpRequest(PMV_Request pReq, MV_BOOLEAN detail);
#if defined(SUPPORT_RAID6) && defined(RAID_DRIVER)
void MV_DumpXORRequest(PMV_XOR_Request pXORReq, MV_BOOLEAN detail);
#endif /* SUPPORT_RAID6 */
void MV_DumpSGTable(PMV_SG_Table pSGTable);
const char* MV_DumpSenseKey(MV_U8 sense);

MV_U32 MV_CRC(
	IN MV_PU8  pData, 
	IN MV_U16  len
);

#define MV_MOD_ADD(value, mod)                    \
           do {                                   \
              (value)++;                          \
              if ((value) >= (mod))               \
                 (value) = 0;                     \
           } while (0);

#ifdef MV_DEBUG
void MV_CHECK_OS_SG_TABLE(
    IN PMV_SG_Table pSGTable
    );
#endif /* MV_DEBUG */

/* used for endian-ness conversion */
static inline MV_VOID mv_swap_bytes(MV_PVOID buf, MV_U32 len)
{
	MV_U32 i;
	MV_U8  tmp, *p;

	/* we expect len to be in multiples of 2 */
	if (len & 0x1)
		return;

	p = (MV_U8 *) buf;
	for (i = 0; i < len / 2; i++) 
	{
		tmp = p[i];
		p[i] = p[len - i - 1];
		p[len - i - 1] = tmp;
	}
}


#ifdef _OS_LINUX
void List_Add(List_Head *new_one, List_Head *head);
void List_AddTail(List_Head *new_one, List_Head *head);
void List_Del(List_Head *entry);
struct list_head *List_GetFirst(struct list_head *head);
struct list_head *List_GetLast(struct list_head *head);

#endif

#endif /*  __MV_COM_UTIL_H__ */
