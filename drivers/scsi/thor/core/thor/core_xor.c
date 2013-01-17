#include "mv_include.h"
#include "core_inter.h"

#ifndef USE_NEW_SGTABLE
#ifdef RAID_DRIVER
#ifdef SOFTWARE_XOR

#ifdef _OS_LINUX

/*
 * Software XOR operations
 */
void mvXORWrite (MV_PVOID This, PMV_XOR_Request pXORReq);
void mvXORCompare (MV_PVOID This, PMV_XOR_Request pXORReq);
void mvXORDMA (MV_PVOID This, PMV_XOR_Request pXORReq);

void Core_ModuleSendXORRequest(MV_PVOID This, PMV_XOR_Request pXORReq)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;

	switch (pXORReq->Request_Type) 
	{
		case XOR_REQUEST_WRITE:
			mvXORWrite (pCore, pXORReq);
			break;
		case XOR_REQUEST_COMPARE:
			mvXORCompare (pCore, pXORReq);
			break;
		case XOR_REQUEST_DMA:
			mvXORDMA (pCore, pXORReq);
			break;
		default:
			pXORReq->Request_Status = XOR_STATUS_INVALID_REQUEST;
			break;
	}
	pXORReq->Completion( pXORReq->Cmd_Initiator, pXORReq );
}

void mvXORInit(
	PMV_SG_Entry	*pSGPtr,
	MV_PU32			SGSizePtr,
	MV_PVOID		*pVirPtr,
	MV_PU32			SGOffPtr,
	PMV_SG_Table	SGListPtr,
	MV_U8			tableCount,
	MV_PU32			minSizePtr)
{
	MV_U8 id;
	for ( id=0; id<tableCount; id++ ) {
		pSGPtr[id] = SGListPtr[id].Entry_Ptr;
		pVirPtr[id] = (MV_PVOID)
			( (MV_PTR_INTEGER)pSGPtr[id]->Base_Address 
			| (MV_PTR_INTEGER)((_MV_U64)pSGPtr[id]->Base_Address_High<<32) );
		SGSizePtr[id] = pSGPtr[id]->Size;
		SGOffPtr[id] = 0;
		if ( *minSizePtr > SGSizePtr[id] ) *minSizePtr=SGSizePtr[id];
	}
}

void mvXORUpdateEntry(
	PMV_SG_Entry	*pSGPtr,
	MV_PU32			SGSizePtr,
	MV_PVOID		*pVirPtr,
	MV_PU32			SGOffPtr,
	MV_U32			finishSize,
	MV_U8			tableCount,
	MV_PU32			minSizePtr)
{
	MV_U8 id;
	for ( id=0; id<tableCount; id++ ) {
		if ( SGSizePtr[id] > finishSize )
			SGSizePtr[id] -= finishSize;
		else {
			pSGPtr[id]++;
			pVirPtr[id] = (MV_PVOID)
					( (MV_PTR_INTEGER)pSGPtr[id]->Base_Address 
					| (MV_PTR_INTEGER)((_MV_U64)pSGPtr[id]->Base_Address_High<<32) );
			SGSizePtr[id] = pSGPtr[id]->Size;
			SGOffPtr[id] = 0;
		}
		if ( *minSizePtr > SGSizePtr[id] ) *minSizePtr=SGSizePtr[id];
	}
}

MV_PVOID mv_get_kvaddr(PMV_SG_Entry sg_entry, MV_PVOID addr)
{
#ifdef _OS_LINUX
	MV_U8 map;
	struct scatterlist *sg = NULL;
	MV_PVOID kvaddr = NULL;
	MV_U16 offset = 0;

	sg = (MV_PVOID)((MV_PTR_INTEGER)sg_entry->Base_Address |
		(MV_PTR_INTEGER)((_MV_U64)sg_entry->Base_Address_High << 32));
	map = sg_entry->Reserved0;
	if (map == 0) return sg;
	MV_ASSERT((sg_entry->Reserved0&0x1) == 1);

	offset = ((sg_entry->Reserved0) >> 16) + sg->offset;
	kvaddr = kmap_atomic(sg->page, KM_IRQ0) + offset;
	return kvaddr;
#endif
#ifdef _OS_WINDOWS
	return addr;
#endif
}

MV_VOID mv_put_kvaddr(PMV_SG_Entry sg_entry, MV_PVOID kvaddr)
{
#ifdef _OS_LINUX
	MV_U8 map;
	struct scatterlist *sg = NULL;
	MV_U16 offset = 0;

	map = sg_entry->Reserved0;
	if (map == 0) return;
	MV_ASSERT((sg_entry->Reserved0&0x1) == 1);

	sg = (MV_PVOID)((MV_PTR_INTEGER)sg_entry->Base_Address |
		(MV_PTR_INTEGER)((_MV_U64)sg_entry->Base_Address_High << 32));
	offset = ((sg_entry->Reserved0) >> 16) + sg->offset;
	flush_dcache_page(sg->page);
	kunmap_atomic(kvaddr - offset, KM_IRQ0);
#endif
}

MV_VOID mv_save_val_8(PMV_SG_Entry sg_entry, MV_U32 sge_off, 
	MV_PU8 kaddr, MV_U8 val)
{
#ifdef _OS_LINUX
	MV_U8 map;
	struct scatterlist *sg = NULL;
	MV_PU8 kvaddr = NULL;
	MV_U16 offset = 0;

	sg = (MV_PVOID)((MV_PTR_INTEGER)sg_entry->Base_Address |
		(MV_PTR_INTEGER)((_MV_U64)sg_entry->Base_Address_High << 32));

	map = sg_entry->Reserved0;
	if (map == 0) {
		MV_ASSERT((sg == (struct scatterlist *)kaddr));
		*(kaddr + sge_off) = val;
		return;
	}

	offset = ((sg_entry->Reserved0) >> 16) + sg->offset + sge_off;
	kvaddr = kmap_atomic(sg->page, KM_IRQ0) + offset;
	*(kvaddr) = val;
	kunmap_atomic(kvaddr - offset, KM_IRQ0);
#endif
#ifdef _OS_WINDOWS
	*(kaddr + sge_off) = val;
#endif
}

#ifdef SUPPORT_XOR_DWORD
static MV_VOID mv_save_val_32(PMV_SG_Entry sg_entry, MV_U32 sge_off, 
	MV_PU32 kaddr, MV_U32 val)
{
#ifdef _OS_LINUX
	MV_U8 map;
	struct scatterlist *sg = NULL;
	MV_PU32 kvaddr = NULL;
	MV_U16 offset = 0;

	sg = (MV_PVOID)((MV_PTR_INTEGER)sg_entry->Base_Address |
		(MV_PTR_INTEGER)((_MV_U64)sg_entry->Base_Address_High << 32));

	map = sg_entry->Reserved0;
	if (map == 0) {
		MV_ASSERT((sg == (struct scatterlist *)kaddr));
		*(kaddr + sge_off) = val;
		return;
	}

	offset = ((sg_entry->Reserved0) >> 16) + sg->offset;
	kvaddr = kmap_atomic(sg->page, KM_IRQ0) + offset;
	kvaddr += sge_off;
	*(kvaddr) = val;
	kvaddr -= sge_off;
	kvaddr = (MV_PU32)((MV_PU8)kvaddr - offset);
	kunmap_atomic(kvaddr, KM_IRQ0);
#endif
#ifdef _OS_WINDOWS
	*(kaddr + sge_off) = val;
#endif
}
#endif /* SUPPORT_XOR_DWORD */

MV_U8 mvXORByte(
	PMV_SG_Entry	*sg_entry,
	MV_PU8			*pSourceVirPtr,
	PMV_XOR_Request	pXORReq,
	MV_U8			tId,
	MV_PU32			sge_off
)
{
	MV_U8 xorResult, sId;
	MV_PU8 addr;

	addr = mv_get_kvaddr(sg_entry[0], pSourceVirPtr[0]);
	xorResult = GF_Multiply(*(addr + sge_off[0]), pXORReq->Coef[tId][0]);
	mv_put_kvaddr(sg_entry[0], addr);

	for ( sId=1; sId<pXORReq->Source_SG_Table_Count; sId++ ) {
		addr = mv_get_kvaddr(sg_entry[sId], pSourceVirPtr[sId]);
		xorResult = GF_Add(xorResult, 
			GF_Multiply(*(addr + sge_off[sId]), pXORReq->Coef[tId][sId]));
		mv_put_kvaddr(sg_entry[sId], addr);
	}
	return xorResult;
}

#ifdef SUPPORT_XOR_DWORD
MV_U32 mvXORDWord(
	PMV_SG_Entry	*sg_entry,
	MV_PU32			*pSourceVirPtr,
	PMV_XOR_Request	pXORReq,
	MV_U8			tId,
	MV_PU32			sge_off
)
{
	MV_U8	sId;
	MV_U32 xorResult;
	MV_PU32 addr = NULL;

	addr = mv_get_kvaddr(sg_entry[0], pSourceVirPtr[0]);
	xorResult = GF_Multiply(*(addr + sge_off[0]), pXORReq->Coef[tId][0]);
	mv_put_kvaddr(sg_entry[0], addr);

	for ( sId=1; sId<pXORReq->Source_SG_Table_Count; sId++ ) {
		addr = mv_get_kvaddr(sg_entry[sId], pSourceVirPtr[sId]);
		xorResult = GF_Add(xorResult,
			GF_Multiply(*(addr + sge_off[sId]), pXORReq->Coef[tId][sId]));
		mv_put_kvaddr(sg_entry[sId], addr);
	}
	return xorResult;
}
#endif /* SUPPORT_XOR_DWORD */

/* The SG Table should have the virtual address instead of the physical address. */
void mvXORWrite(MV_PVOID This, PMV_XOR_Request pXORReq)
{
	PMV_SG_Entry	pSourceSG[XOR_SOURCE_SG_COUNT];
	PMV_SG_Entry	pTargetSG[XOR_TARGET_SG_COUNT];
	MV_U32			sourceSize[XOR_SOURCE_SG_COUNT];
	MV_U32			targetSize[XOR_TARGET_SG_COUNT];
	MV_U32			sourceOff[XOR_SOURCE_SG_COUNT];
	MV_U32			targetOff[XOR_TARGET_SG_COUNT];
	MV_U32 i;
	/* source index and target index. */
	MV_U8 sId,tId;
	MV_U32 size, remainSize, minSize;
#ifdef SUPPORT_XOR_DWORD
	MV_PU32			pSourceVir[XOR_SOURCE_SG_COUNT];
	MV_PU32			pTargetVir[XOR_TARGET_SG_COUNT];
	MV_U32			xorResult, Dword_size;
#else
	MV_PU8			pSourceVir[XOR_SOURCE_SG_COUNT];
	MV_PU8			pTargetVir[XOR_TARGET_SG_COUNT];
	MV_U8			xorResult;
#endif

	/* Initialize these two variables. */
	/* All the SG table should have same Byte_Count */
	remainSize = pXORReq->Source_SG_Table_List[0].Byte_Count;
	minSize = remainSize;
	/* Initialize XOR source */
	mvXORInit(pSourceSG, sourceSize, (MV_PVOID)pSourceVir, sourceOff,
			  pXORReq->Source_SG_Table_List,
			  pXORReq->Source_SG_Table_Count,
			  &minSize);
	/* Initialize XOR target */
	mvXORInit(pTargetSG, targetSize, (MV_PVOID)pTargetVir, targetOff,
			  pXORReq->Target_SG_Table_List,
			  pXORReq->Target_SG_Table_Count,
			  &minSize);

	/* Navigate all the SG table, calculate the target xor value. */
	while ( remainSize>0 ) 
	{
		size = minSize;
#ifdef SUPPORT_XOR_DWORD
		MV_DASSERT( !(size%4) );
		Dword_size = size/4;
		for ( i=0; i<Dword_size; i++ ) 
#else
		for ( i=0; i<size; i++ ) 
#endif
		{
			for ( tId=0; tId<pXORReq->Target_SG_Table_Count; tId++ )
			{
#ifdef SUPPORT_XOR_DWORD
				xorResult = mvXORDWord(pSourceSG, pSourceVir, pXORReq, tId, sourceOff);
				mv_save_val_32(pTargetSG[tId], targetOff[tId], pTargetVir[tId], xorResult);
				targetOff[tId]++;
#else
				xorResult = mvXORByte(pSourceSG, pSourceVir, pXORReq, tId, sourceOff);
				mv_save_val_8(pTargetSG[tId], targetOff[tId], pTargetVir[tId], xorResult);
				targetOff[tId]++;
#endif
			}

			for ( sId=0; sId<pXORReq->Source_SG_Table_Count; sId++ )
				sourceOff[sId]++;
		}

		/* Update entry pointer, size */
		MV_DASSERT( remainSize>=size );
		remainSize -= size;
		minSize = remainSize;
		/* Update XOR source */
		mvXORUpdateEntry(pSourceSG, sourceSize, (MV_PVOID)pSourceVir, sourceOff,
						 size, pXORReq->Source_SG_Table_Count, &minSize);
		/* Update XOR target */
		mvXORUpdateEntry(pTargetSG, targetSize, (MV_PVOID)pTargetVir, targetOff,
						 size, pXORReq->Target_SG_Table_Count, &minSize);
	}

	pXORReq->Request_Status = XOR_STATUS_SUCCESS;
}

/* consolidate compare and write */
void mvXORCompare(MV_PVOID This, PMV_XOR_Request pXORReq)
{
	PMV_SG_Entry	pSourceSG[XOR_SOURCE_SG_COUNT];
	MV_U32			sourceSize[XOR_SOURCE_SG_COUNT];
	MV_U32			sourceOff[XOR_SOURCE_SG_COUNT];
	MV_U32			totalSize, remainSize, minSize, size, i;
	MV_U8			sId;
#ifdef SUPPORT_XOR_DWORD
	MV_PU32			pSourceVir[XOR_SOURCE_SG_COUNT];
	MV_U32			xorResult, Dword_size;
#else
	MV_PU8			pSourceVir[XOR_SOURCE_SG_COUNT];
	MV_U8			xorResult;
#endif

	/* All the SG table should have same Byte_Count */
	totalSize = remainSize = minSize = pXORReq->Source_SG_Table_List[0].Byte_Count;
	mvXORInit(pSourceSG, sourceSize, (MV_PVOID)pSourceVir, sourceOff,
			  pXORReq->Source_SG_Table_List,
			  pXORReq->Source_SG_Table_Count,
			  &minSize);
	while ( remainSize>0 ) {
		size = minSize;
#ifdef SUPPORT_XOR_DWORD
		MV_DASSERT( !(size%4) );
		Dword_size = size/4;
		for ( i=0; i<Dword_size; i++ ) {
			xorResult = mvXORDWord(pSourceSG, pSourceVir, pXORReq, 0, sourceOff);
#else
		for ( i=0; i<size; i++ ) {
			xorResult = mvXORByte(pSourceSG, pSourceVir, pXORReq, 0, sourceOff);
#endif
			if (xorResult != 0) {
				pXORReq->Request_Status = XOR_STATUS_ERROR;
#ifdef SUPPORT_XOR_DWORD
				pXORReq->Error_Offset = totalSize - remainSize + i*4;
#else
				pXORReq->Error_Offset = totalSize - remainSize + i;
#endif
				return;
			}
			for ( sId=0; sId<pXORReq->Source_SG_Table_Count; sId++ )
				sourceOff[sId]++;
		}

		/* Update entry pointer, size */
		MV_DASSERT( remainSize>=size );
		remainSize -= size;
		minSize = remainSize;
		mvXORUpdateEntry(pSourceSG, sourceSize, (MV_PVOID)pSourceVir, sourceOff,
						 size, pXORReq->Source_SG_Table_Count, &minSize);
	}
}

void mvXORDMA(MV_PVOID This, PMV_XOR_Request pXORReq)
{
	MV_ASSERT( MV_FALSE );
}

#endif /* _OS_LINUX */

#ifdef _OS_WINDOWS

/*
 * Software XOR operations
 */

void mvXORWrite (MV_PVOID This, PMV_XOR_Request pXORReq);
void mvXORCompare (MV_PVOID This, PMV_XOR_Request pXORReq);
void mvXORDMA (MV_PVOID This, PMV_XOR_Request pXORReq);

#ifndef SUPPORT_RAID6
MV_U8 mvXORInitArray (
	MV_PVOID This, 
	PMV_XOR_Request pXORReq, 
	PMV_SG_Entry *SG_entry,
	MV_PU32 SG_size,
	MV_PU8 *pSource, 
	MV_PU32 table_size);
#endif

#ifdef SIMULATOR
void _Core_ModuleSendXORRequest(MV_PVOID This, PMV_XOR_Request pXORReq)
#else	/*SIMULATOR*/
void Core_ModuleSendXORRequest(MV_PVOID This, PMV_XOR_Request pXORReq)
#endif	/*!SIMULATOR*/
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;

	switch (pXORReq->Request_Type) 
	{
		case XOR_REQUEST_WRITE:
			mvXORWrite (pCore, pXORReq);
			break;
		case XOR_REQUEST_COMPARE:
			mvXORCompare (pCore, pXORReq);
			break;
		case XOR_REQUEST_DMA:
			mvXORDMA (pCore, pXORReq);
			break;
		default:
			pXORReq->Request_Status = XOR_STATUS_INVALID_REQUEST;
			break;
	}
#ifndef SIMULATOR
	pXORReq->Completion( pXORReq->Cmd_Initiator, pXORReq );
#endif	/*!SIMULATOR*/
}

#ifndef SUPPORT_RAID6
void mvXORWrite (MV_PVOID This, PMV_XOR_Request pXORReq)
{
	MV_ASSERT(MV_FALSE);
}

void mvXORCompare (MV_PVOID This, PMV_XOR_Request pXORReq)
{
	PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;

//	PMV_SG_Entry SG_entry[MAX_SG_ENTRY + 1];		// last element is target
	PMV_SG_Entry SG_entry[2];
//	MV_U32 SG_size[MAX_SG_ENTRY + 1];
	MV_U32 SG_size[2];	// number of source, currently support 2 sources

	MV_U32 min_size;
	MV_U32 table_size, total_byte = 0;
	MV_U8 num_sources, sourceCount;
	MV_U32 sizeCount;
	//PMV_SG_Table current_table;
//	MV_PU8 pSource[MAX_SG_ENTRY];
	MV_PU8 pSource[2];
	MV_U8 xorResult;

	num_sources = pXORReq->Source_SG_Table_Count;
	if (num_sources==0) {
		pXORReq->Request_Status = XOR_STATUS_INVALID_PARAMETER;
		return;
	}

	// init array & error checking
	if ( mvXORInitArray(pCore, pXORReq, SG_entry, SG_size, pSource, &table_size) != num_sources ) 
	{
		pXORReq->Request_Status = XOR_STATUS_INVALID_PARAMETER;
		return;
	}

	while (total_byte < table_size) 
	{
		min_size = SG_size[0];
		for (sourceCount=1; sourceCount<num_sources; sourceCount++) 
		{
			if (min_size > SG_size[sourceCount])
				min_size = SG_size[sourceCount];	
		}

		for (sizeCount=0; sizeCount<min_size; sizeCount++) 
		{
			xorResult = *pSource[0];
			for (sourceCount=1; sourceCount<num_sources; sourceCount++) 
			{
//				*pSource[0] = *pSource[0] ^ *pSource[sourceCount];
				xorResult = xorResult ^ *pSource[sourceCount];
				pSource[sourceCount]++;
			}

//			if (*pSource[0] != 0) 
			if (xorResult != 0)
			{
				pXORReq->Request_Status = XOR_STATUS_ERROR;
				pXORReq->Error_Offset = total_byte + sizeCount;
				return;
			}
			pSource[0]++;
		}
		total_byte += min_size;

		if (total_byte<table_size) {
			// make new size array & increment entry pointer if needed
			for (sourceCount=0; sourceCount<num_sources; sourceCount++) 
			{
				SG_size[sourceCount] -= min_size;
				if (SG_size[sourceCount] == 0)
				{
					SG_entry[sourceCount]++;
					SG_size[sourceCount] = SG_entry[sourceCount]->Size;
					pSource[sourceCount] = raid_physical_to_virtual(pXORReq->Cmd_Initiator,
											  SG_entry[sourceCount]->Base_Address);
				}
			}
		}
	}

	pXORReq->Request_Status = XOR_STATUS_SUCCESS;
}

void mvXORDMA (MV_PVOID This, PMV_XOR_Request pXORReq)
{
	MV_ASSERT(MV_FALSE);
}

MV_U8 mvXORInitArray (
	MV_PVOID This, 
	PMV_XOR_Request pXORReq, 
	PMV_SG_Entry *SG_entry,
	MV_PU32 SG_size,
	MV_PU8 *pSource, 
	MV_PU32 table_size)
{	
	//PCore_Driver_Extension pCore = (PCore_Driver_Extension)This;
	PMV_SG_Table current_table = NULL;
	//MV_U8 j; 
	MV_U8 i = 0;

	*table_size = 0;
	for (i=0; i<pXORReq->Source_SG_Table_Count; i++) {
//	while ( !List_Empty(&pXORReq->Source_SG_Table_List) )
//	{
//		current_table = (PMV_SG_Table)List_GetFirstEntry(&pXORReq->Source_SG_Table_List, MV_XOR_Request, Queue_Pointer);
		current_table = &pXORReq->Source_SG_Table_List[i];
		MV_ASSERT( current_table!=NULL );

		SG_entry[i] = &current_table->Entry_Ptr[0];
		SG_size[i] = current_table->Entry_Ptr[0].Size;
//		pSource[i] = (MV_PU8)SG_entry[i]->Base_Address;	
		pSource[i] = raid_physical_to_virtual(pXORReq->Cmd_Initiator, current_table->Entry_Ptr[0].Base_Address);

//		i++;
	}
	MV_DASSERT( current_table!=NULL );
	if ( current_table ) *table_size = current_table->Byte_Count;
	return i;
}

#else /* SUPPORT_RAID6 */

void mvXORInit(
	PMV_SG_Entry	*pSGPtr,
	MV_PU32			SGSizePtr,
	MV_PVOID		*pVirPtr,
	PMV_SG_Table	SGListPtr,
	MV_U8			tableCount,
	MV_PU32			minSizePtr)
{
	MV_U8 id;
	for ( id=0; id<tableCount; id++ ) {
		pSGPtr[id] = SGListPtr[id].Entry_Ptr;
		pVirPtr[id] = (MV_PVOID)
			( (MV_PTR_INTEGER)pSGPtr[id]->Base_Address 
			| (MV_PTR_INTEGER)pSGPtr[id]->Base_Address_High<<32 );
		SGSizePtr[id] = pSGPtr[id]->Size;
		if ( *minSizePtr > SGSizePtr[id] ) *minSizePtr=SGSizePtr[id];
	}
}

void mvXORUpdateEntry(
	PMV_SG_Entry	*pSGPtr,
	MV_PU32			SGSizePtr,
	MV_PVOID		*pVirPtr,
	MV_U32			finishSize,
	MV_U8			tableCount,
	MV_PU32			minSizePtr)
{
	MV_U8 id;
	for ( id=0; id<tableCount; id++ ) {
		if ( SGSizePtr[id] > finishSize )
			SGSizePtr[id] -= finishSize;
		else {
			pSGPtr[id]++;
			pVirPtr[id] = (MV_PVOID)
					( (MV_PTR_INTEGER)pSGPtr[id]->Base_Address 
					| (MV_PTR_INTEGER)pSGPtr[id]->Base_Address_High<<32 );
			SGSizePtr[id] = pSGPtr[id]->Size;
		}
		if ( *minSizePtr > SGSizePtr[id] ) *minSizePtr=SGSizePtr[id];
	}
}

MV_U8 mvXORByte(
	MV_PU8			*pSourceVirPtr,
	PMV_XOR_Request	pXORReq,
	MV_U8			tId
)
{
	MV_U8 xorResult, sId;

	xorResult = GF_Multiply(*pSourceVirPtr[0], pXORReq->Coef[tId][0]);
	for ( sId=1; sId<pXORReq->Source_SG_Table_Count; sId++ ) {
		xorResult = GF_Add(xorResult,
						   GF_Multiply(*pSourceVirPtr[sId], pXORReq->Coef[tId][sId]));
	}
	return xorResult;
}

#ifdef SUPPORT_XOR_DWORD
MV_U32 mvXORDWord(
	MV_PU32			*pSourceVirPtr,
	PMV_XOR_Request	pXORReq,
	MV_U8			tId
)
{
	MV_U8	sId;
	MV_U32 xorResult;

	xorResult = GF_Multiply(*pSourceVirPtr[0], pXORReq->Coef[tId][0]);
	for ( sId=1; sId<pXORReq->Source_SG_Table_Count; sId++ ) {
		xorResult = GF_Add(xorResult,
						   GF_Multiply(*pSourceVirPtr[sId], pXORReq->Coef[tId][sId]));
	}
	return xorResult;
}
#endif /* SUPPORT_XOR_DWORD */

/* The SG Table should have the virtual address instead of the physical address. */
void mvXORWrite(MV_PVOID This, PMV_XOR_Request pXORReq)
{
	PMV_SG_Entry	pSourceSG[XOR_SOURCE_SG_COUNT];
	PMV_SG_Entry	pTargetSG[XOR_TARGET_SG_COUNT];
	MV_U32			sourceSize[XOR_SOURCE_SG_COUNT];
	MV_U32			targetSize[XOR_TARGET_SG_COUNT];
	MV_U32 i;
	MV_U8 sId,tId;									/* source index and target index. */
	MV_U32 size, remainSize, minSize;
#ifdef SUPPORT_XOR_DWORD
	MV_PU32			pSourceVir[XOR_SOURCE_SG_COUNT];
	MV_PU32			pTargetVir[XOR_TARGET_SG_COUNT];
	MV_U32			xorResult, Dword_size;
#else
	MV_PU8			pSourceVir[XOR_SOURCE_SG_COUNT];
	MV_PU8			pTargetVir[XOR_TARGET_SG_COUNT];
	MV_U8			xorResult;
#endif

	/* Initialize these two variables. */
	remainSize = pXORReq->Source_SG_Table_List[0].Byte_Count;	/* All the SG table should have same Byte_Count */
	minSize = remainSize;
	/* Initialize XOR source */
	mvXORInit(pSourceSG, sourceSize, (MV_PVOID*)pSourceVir,
			  pXORReq->Source_SG_Table_List,
			  pXORReq->Source_SG_Table_Count,
			  &minSize);
	/* Initialize XOR target */
	mvXORInit(pTargetSG, targetSize, (MV_PVOID*)pTargetVir,
			  pXORReq->Target_SG_Table_List,
			  pXORReq->Target_SG_Table_Count,
			  &minSize);

/*
	for ( sId=0; sId<pXORReq->Source_SG_Table_Count; sId++ ) 
	{
		pSourceSG[sId] = pXORReq->Source_SG_Table_List[sId].Entry;
		sourceSize[sId] = pSourceSG[sId]->Size;
		pSourceVir[sId] = (MV_PVOID)
			( (MV_PTR_INTEGER)pSourceSG[sId]->Base_Address 
			| (MV_PTR_INTEGER)pSourceSG[sId]->Base_Address_High<<32 );
		MV_DASSERT( remainSize==pXORReq->Source_SG_Table_List[sId].Byte_Count );
		if ( minSize>sourceSize[sId] ) minSize=sourceSize[sId];
	}

	for ( tId=0; tId<pXORReq->Target_SG_Table_Count; tId++ ) 
	{
		pTargetSG[tId] = pXORReq->Target_SG_Table_List[tId].Entry;
		targetSize[tId] = pTargetSG[tId]->Size;
		pTargetVir[tId] = (MV_PVOID)
			( (MV_PTR_INTEGER)pTargetSG[tId]->Base_Address 
			| (MV_PTR_INTEGER)pTargetSG[tId]->Base_Address_High<<32 );
		MV_DASSERT( remainSize==pXORReq->Target_SG_Table_List[tId].Byte_Count );
		if ( minSize>targetSize[tId] ) minSize=targetSize[tId];
	}
*/

	/* Navigate all the SG table, calculate the target xor value. */
	while ( remainSize>0 ) 
	{
		size = minSize;
#ifdef SUPPORT_XOR_DWORD
		MV_DASSERT( !(size%4) );
		Dword_size = size/4;
		for ( i=0; i<Dword_size; i++ ) 
#else
		for ( i=0; i<size; i++ ) 
#endif
		{
			for ( tId=0; tId<pXORReq->Target_SG_Table_Count; tId++ )
			{
#ifdef SUPPORT_XOR_DWORD
				xorResult = mvXORDWord(pSourceVir, pXORReq, tId);
#else
				xorResult = mvXORByte(pSourceVir, pXORReq, tId);
#endif

/*
				tmp = GF_Multiply(*pSourceVir[0], pXORReq->Coef[tId][0]);

				for ( sId=1; sId<pXORReq->Source_SG_Table_Count; sId++ )
				{
					tmp = GF_Add(tmp,
								GF_Multiply(*pSourceVir[sId], pXORReq->Coef[tId][sId]));
				}
				*pTargetVir[tId] = tmp;
*/
				*pTargetVir[tId] = xorResult;
				pTargetVir[tId]++;
			}

			for ( sId=0; sId<pXORReq->Source_SG_Table_Count; sId++ )
				pSourceVir[sId]++;
		}

		/* Update entry pointer, size */
		MV_DASSERT( remainSize>=size );
		remainSize -= size;
		minSize = remainSize;
		/* Update XOR source */
		mvXORUpdateEntry(pSourceSG, sourceSize, (MV_PVOID*)pSourceVir,
						 size, pXORReq->Source_SG_Table_Count, &minSize);
		/* Update XOR target */
		mvXORUpdateEntry(pTargetSG, targetSize, (MV_PVOID*)pTargetVir,
						 size, pXORReq->Target_SG_Table_Count, &minSize);
/*

		for ( sId=0; sId<pXORReq->Source_SG_Table_Count; sId++ )
		{
			if ( sourceSize[sId]>size )
			{
				sourceSize[sId]-=size;
			}
			else
			{
				pSourceSG[sId]++;
				pSourceVir[sId] = (MV_PVOID)
					( (MV_PTR_INTEGER)pSourceSG[sId]->Base_Address | (MV_PTR_INTEGER)pSourceSG[sId]->Base_Address_High<<32 );
				sourceSize[sId] = pSourceSG[sId]->Size;
			}
			if ( minSize>sourceSize[sId] ) minSize=sourceSize[sId];
		}

		for ( tId=0; tId<pXORReq->Target_SG_Table_Count; tId++ )
		{
			if ( targetSize[tId]>size )
			{
				targetSize[tId]-=size;
			}
			else
			{
				pTargetSG[tId]++;
				pTargetVir[tId] = (MV_PVOID)
					( (MV_PTR_INTEGER)pTargetSG[tId]->Base_Address | (MV_PTR_INTEGER)pTargetSG[tId]->Base_Address_High<<32 );
				targetSize[tId] = pTargetSG[tId]->Size;
			}
			if ( minSize>targetSize[tId] ) minSize=targetSize[tId];
		}
*/
	}

	pXORReq->Request_Status = XOR_STATUS_SUCCESS;
}

//consolidate compare and write
void mvXORCompare (MV_PVOID This, PMV_XOR_Request pXORReq)
{
	PMV_SG_Entry	pSourceSG[XOR_SOURCE_SG_COUNT];
	MV_U32			sourceSize[XOR_SOURCE_SG_COUNT];
	MV_U32			totalSize, remainSize, minSize, size, i;
	MV_U8			sId;
#ifdef SUPPORT_XOR_DWORD
	MV_PU32			pSourceVir[XOR_SOURCE_SG_COUNT];
	MV_U32			xorResult, Dword_size;
#else
	MV_PU8			pSourceVir[XOR_SOURCE_SG_COUNT];
	MV_U8			xorResult;
#endif

	/* All the SG table should have same Byte_Count */
	totalSize = remainSize = minSize = pXORReq->Source_SG_Table_List[0].Byte_Count;
	mvXORInit(pSourceSG, sourceSize, (MV_PVOID*)pSourceVir,
			  pXORReq->Source_SG_Table_List,
			  pXORReq->Source_SG_Table_Count,
			  &minSize);
	while ( remainSize>0 ) {
		size = minSize;
#ifdef SUPPORT_XOR_DWORD
		MV_DASSERT( !(size%4) );
		Dword_size = size/4;
		for ( i=0; i<Dword_size; i++ ) {
			xorResult = mvXORDWord(pSourceVir, pXORReq, 0);
#else
		for ( i=0; i<size; i++ ) {
			xorResult = mvXORByte(pSourceVir, pXORReq, 0);
#endif
			if (xorResult != 0)	{
				pXORReq->Request_Status = XOR_STATUS_ERROR;
#ifdef SUPPORT_XOR_DWORD
				pXORReq->Error_Offset = totalSize - remainSize + i*4;
#else
				pXORReq->Error_Offset = totalSize - remainSize + i;
#endif
				return;
			}
			for ( sId=0; sId<pXORReq->Source_SG_Table_Count; sId++ )
				pSourceVir[sId]++;
		}

		/* Update entry pointer, size */
		MV_DASSERT( remainSize>=size );
		remainSize -= size;
		minSize = remainSize;
		mvXORUpdateEntry(pSourceSG, sourceSize, (MV_PVOID*)pSourceVir,
						 size, pXORReq->Source_SG_Table_Count, &minSize);
	}
}

void mvXORDMA (MV_PVOID This, PMV_XOR_Request pXORReq)
{
	MV_ASSERT( MV_FALSE );
}

#endif /* SUPPORT_RAID6 */

#endif /* _OS_WINDOWS */

#endif /* SOFTWARE_XOR */
#endif /* RAID_DRIVER */
#endif /* USE_NEW_SGTABLE */
