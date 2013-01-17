#include "mv_include.h"
#include "core_inter.h"

#if defined(RAID_DRIVER) && defined(USE_NEW_SGTABLE)
typedef struct _xor_strm_t
{
	sgd_t		sgd[2];
	MV_U32		off;
} xor_strm_t;

static MV_PVOID sgd_kmap(
	PCore_Driver_Extension	pCore,
	sgd_t*					sg
	)
{
#ifdef _OS_WINDOWS
	sgd_pctx_t* pctx = (sgd_pctx_t*) sg;
	MV_PTR_INTEGER addr = (MV_PTR_INTEGER)(pctx->baseAddr.value);

	MV_DASSERT( sg->flags & SGD_PCTX );

	addr &= ~0x80000000L; // just for fun, refer to GenerateSGTable in win_helper.c

	return (MV_PVOID) addr;
#endif /* _OS_WINDOWS */
#ifdef _OS_LINUX
	sgd_pctx_t* pctx = (sgd_pctx_t*)sg;
	struct scatterlist *ksg = (struct scatterlist *)pctx->u.xctx;
	void *kvaddr = NULL;

	MV_DASSERT( sg->flags & SGD_PCTX );
	MV_DASSERT( ksg );

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	kvaddr = page_address(ksg->page);
	if (!kvaddr) 
#endif
		kvaddr = map_sg_page(ksg);
	kvaddr += ksg->offset;
	return kvaddr;
#endif /* _OS_LINUX */
}

static void sgd_kunmap(
	PCore_Driver_Extension	pCore,
	sgd_t*					sg,
	MV_PVOID				mapped_addr
	)
{
#ifdef _OS_WINDOWS
#endif /* _OS_WINDOWS */
#ifdef _OS_LINUX
	sgd_pctx_t* pctx = (sgd_pctx_t*)sg;
	struct scatterlist *ksg = (struct scatterlist *)pctx->u.xctx;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	void *kvaddr = NULL;
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif
	kunmap_atomic(mapped_addr - ksg->offset, KM_IRQ0);
#endif /* _OS_LINUX */
}
static MV_PVOID sgd_kmap_sec(
	PCore_Driver_Extension	pCore,
	sgd_t*					sg
	)
{
#ifdef _OS_WINDOWS
	sgd_pctx_t* pctx = (sgd_pctx_t*) sg;
	MV_PTR_INTEGER addr = (MV_PTR_INTEGER)(pctx->baseAddr.value);

	MV_DASSERT( sg->flags & SGD_PCTX );

	addr &= ~0x80000000L; // refer to GenerateSGTable in win_helper.c

	return (MV_PVOID) addr;
#endif /* _OS_WINDOWS */
#ifdef _OS_LINUX
	sgd_pctx_t* pctx = (sgd_pctx_t*)sg;
	struct scatterlist *ksg = (struct scatterlist *)pctx->u.xctx;
	void *kvaddr = NULL;
	MV_DASSERT( sg->flags & SGD_PCTX );
	MV_DASSERT( ksg );

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	kvaddr = page_address(ksg->page);
	if (!kvaddr) 
#endif
		kvaddr = map_sg_page_sec(ksg);
	kvaddr += ksg->offset;
	return kvaddr;
#endif /* _OS_LINUX */
}

static void sgd_kunmap_sec(
	PCore_Driver_Extension	pCore,
	sgd_t*					sg,
	MV_PVOID				mapped_addr
	)
{
#ifdef _OS_WINDOWS
#endif /* _OS_WINDOWS */
#ifdef _OS_LINUX
	sgd_pctx_t* pctx = (sgd_pctx_t*)sg;
	struct scatterlist *ksg = (struct scatterlist *)pctx->u.xctx;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	void *kvaddr = NULL;
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif
	kunmap_atomic(mapped_addr - ksg->offset, KM_IRQ1);
#endif /* _OS_LINUX */
}

void CopySGs(
	PCore_Driver_Extension	pCore,
	PMV_SG_Table			srctbl,
	PMV_SG_Table			dsttbl
	);

#endif /* RAID_DRIVER && USE_NEW_SGTABLE */

#if defined(RAID_DRIVER) && defined(SOFTWARE_XOR) && defined(USE_NEW_SGTABLE)
/*
 * CompareSGs needs not to be changed for SGD_PCTX since RAID always
 * use memory with known virtual address.
 */
void
CompareSGs( 
	PCore_Driver_Extension	pCore,
	PMV_XOR_Request			pXORReq,
	PMV_SG_Table			srctbl
	)
{
	MV_PU32					pSrc[2] = { NULL, };
	sgd_t					sgd[2];
	sgd_iter_t				sg_iter[2];
	MV_U32					wCount[2];
	MV_BOOLEAN				bFinished = MV_FALSE;
	MV_U8					bIndex;
	MV_U32					offset = 0;
	MV_PVOID				p = NULL;

	MV_ASSERT( srctbl[0].Byte_Count == srctbl[1].Byte_Count );

	for( bIndex = 0; bIndex < 2; bIndex++ )
	{
		sgd_iter_init( &sg_iter[bIndex], srctbl[bIndex].Entry_Ptr, 0, srctbl[bIndex].Byte_Count );

		sgd_iter_get_next( &sg_iter[bIndex], sgd );

		sgd_get_vaddr( sgd, p );
		pSrc[bIndex] = (MV_PU32) p;
		
		sgd_getsz( sgd, wCount[bIndex] );

	}

	while( !bFinished )
	{
		if( *pSrc[0] != *pSrc[1] )
		{
			pXORReq->Request_Status = XOR_STATUS_ERROR;
			pXORReq->Error_Offset = offset;
			return;
		}

		offset += sizeof( MV_U32 );

		for( bIndex = 0; bIndex < 2; bIndex++ )
		{
			pSrc[bIndex]++;
			wCount[bIndex] -= sizeof( MV_U32 );
			if( wCount[bIndex] == 0 )
			{
				if( !sgd_iter_get_next( &sg_iter[bIndex], sgd ) )
				{
					bFinished = MV_TRUE;

					/*
					for( bIndex = 0; bIndex < srcCount + dstCount; bIndex++ )
					{
						MV_ASSERT( !sgd_iter_get_next( &sg_iter[bIndex], sgd ) );
					}
					*/
					break;
				}

				sgd_get_vaddr( sgd, p );
				pSrc[bIndex] = (MV_PU32) p;
				
				sgd_getsz( sgd, wCount[bIndex] );
			}
		}
	}
}

#if defined(XOR_AS_GF)
typedef MV_U8 XORUNIT;
#else
typedef MV_U32 XORUNIT;
#endif

void sg_xor(
	PCore_Driver_Extension	pCore,
	xor_strm_t*				strm,
	MV_U8					src_cnt,
	MV_U8					dst_cnt,
	MV_U32					byte_cnt
	)
{
	//XORUNIT	*pSrc, *pDst;
	XORUNIT		*p;
	MV_PVOID 	ptmp;
	int		i;
	XORUNIT	value = 0;
	MV_BOOLEAN	mapped;
	MV_U32	off = 0;
#ifdef _OS_LINUX
	unsigned long flags = 0;
#endif		/* _OS_LINUX */

	while( byte_cnt )
	{
		for( i = 0; i < src_cnt+dst_cnt; i++ )
		{
			mapped = MV_FALSE;
			if( strm[i].sgd[0].flags & SGD_PCTX )
			{
			#ifdef _OS_LINUX
				local_irq_save(flags);
			#endif
				p = (XORUNIT*) sgd_kmap(pCore,strm[i].sgd);
				ptmp = p;
				mapped = MV_TRUE;
	#ifdef _OS_LINUX
				p = (XORUNIT*)(((MV_PU8)p) + 
				              strm[i].sgd[1].size);
	#endif
			}
			else
			{
				ptmp = NULL;
				sgd_get_vaddr( strm[i].sgd, ptmp );
				p = (XORUNIT*) ptmp;
			}

			p = (XORUNIT*) (((MV_PU8) p) + strm[i].off + off);

			if( i == 0 )
				value = *p;
			else if( i >= src_cnt )
				*p = value;
			else
				value ^= *p;

			if( mapped ){
				sgd_kunmap( pCore, strm[i].sgd, ptmp );
			#ifdef _OS_LINUX
			 	local_irq_restore(flags);	
			#endif
			}
		}
		byte_cnt -= sizeof(XORUNIT);
		off += sizeof(XORUNIT);
	}
}

MV_U32 min_of(MV_U32* ele, MV_U32 cnt)
{
	MV_U32 v = *ele++;

	while( --cnt )
	{
		if( *ele < v )
			v = *ele;
		ele++;
	}

	return v;
}

void
XorSGs(
	PCore_Driver_Extension	pCore,
	MV_U8					srcCount,
	MV_U8					dstCount,
	PMV_SG_Table			srctbl,
	PMV_SG_Table			dsttbl,
	XOR_COEF				Coef[XOR_TARGET_SG_COUNT][XOR_SOURCE_SG_COUNT]
	)
{
	xor_strm_t				strm[XOR_SOURCE_SG_COUNT+ XOR_TARGET_SG_COUNT];
	sgd_iter_t				sg_iter[XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT];
	MV_U32					wCount[XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT];
	MV_U8					bIndex;
	MV_BOOLEAN				bFinished = MV_FALSE;
	MV_U32					tmp = srctbl[0].Byte_Count;
	MV_U32					count = 0xffffffffL;

	for( bIndex = 0; bIndex < srcCount; bIndex++ )
	{
		MV_ASSERT( srctbl[bIndex].Byte_Count == tmp );
		sgd_iter_init( &sg_iter[bIndex], srctbl[bIndex].Entry_Ptr, 0, srctbl[bIndex].Byte_Count );
	}

	for( bIndex = 0; bIndex < dstCount; bIndex++ )
	{
		MV_ASSERT( srctbl[bIndex].Byte_Count == tmp );
		sgd_iter_init( &sg_iter[srcCount+bIndex], dsttbl[bIndex].Entry_Ptr, 0, dsttbl[bIndex].Byte_Count );
	}

	for( bIndex = 0; bIndex < srcCount + dstCount; bIndex++ )
	{
		strm[bIndex].off = 0;
		sgd_iter_get_next( &sg_iter[bIndex], strm[bIndex].sgd );		
		sgd_getsz( strm[bIndex].sgd, wCount[bIndex] );

		if( wCount[bIndex] < count )
			count = wCount[bIndex];
	}

	while( !bFinished )
	{

		sg_xor( pCore, strm, srcCount, dstCount, count );

		for( bIndex = 0; bIndex < srcCount + dstCount; bIndex++ )
		{
			wCount[bIndex] -= count;
			if( wCount[bIndex] == 0 )
			{
				if( !sgd_iter_get_next( &sg_iter[bIndex], strm[bIndex].sgd ) )
				{
					bFinished = MV_TRUE;
					break;
				}

				strm[bIndex].off = 0;
				sgd_getsz( strm[bIndex].sgd, wCount[bIndex] );
			}
			else
				strm[bIndex].off += count;
		}
		count = min_of( wCount, srcCount + dstCount );
	}
}



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
			XorSGs( 
				pCore, 
				pXORReq->Source_SG_Table_Count, 
				pXORReq->Target_SG_Table_Count,
				pXORReq->Source_SG_Table_List,
				pXORReq->Target_SG_Table_List,
				pXORReq->Coef );
			break;
		case XOR_REQUEST_COMPARE:
			MV_ASSERT( pXORReq->Source_SG_Table_Count == 2 );
			CompareSGs( 
				pCore, 
				pXORReq,
				pXORReq->Source_SG_Table_List );
			break;
		case XOR_REQUEST_DMA:
			CopySGs(
				pCore,
				pXORReq->Source_SG_Table_List,
				pXORReq->Target_SG_Table_List );
			break;
		default:
			pXORReq->Request_Status = XOR_STATUS_INVALID_REQUEST;
			break;
	}
#ifndef SIMULATOR
	pXORReq->Completion( pXORReq->Cmd_Initiator, pXORReq );
#endif	/*!SIMULATOR*/
}

#endif /* RAID_DRIVER && SOFTWARE_XOR && USE_NEW_SGTABLE */

#if defined(RAID_DRIVER) && defined(USE_NEW_SGTABLE)

static void sg_memcpy(
	PCore_Driver_Extension	pCore,
	xor_strm_t				*strm, /*strm[0]:source, strm[1]:destination*/
	MV_U32					byte_cnt
	)
{
	MV_U32		sz;
	MV_PU32		pSrc[2] = { NULL, NULL };
	MV_BOOLEAN	mapped[2] = { MV_FALSE, MV_FALSE };
	MV_PVOID	p = NULL;
	int			i;
	MV_PVOID 	ptmp = NULL;
#ifdef _OS_LINUX
	unsigned long flags = 0;
	int             enabled = 0;
#endif		/* _OS_LINUX */

	for( i = 0; i < 2; i++ )
	{
		sgd_getsz(strm[i].sgd,sz);
		MV_DASSERT( strm[i].off < sz );
		MV_DASSERT( strm[i].off+byte_cnt <= sz );
	}

#ifdef _OS_LINUX
	if( strm[i].sgd[0].flags & SGD_PCTX ){
		enabled = 1;
		local_irq_save(flags);
	}
#endif		/* _OS_LINUX */

	for( i = 0; i < 2; i++ )
	{
		if( strm[i].sgd[0].flags & SGD_PCTX )
		{
			if(i==0)
				ptmp = (MV_PU32) sgd_kmap(pCore,strm[i].sgd);
			else
				ptmp = (MV_PU32) sgd_kmap_sec(pCore,strm[i].sgd);

			mapped[i] = MV_TRUE;
	#ifdef _OS_LINUX
			pSrc[i] = (MV_PU32)((MV_PU8)ptmp + strm[i].sgd[1].size);
	#else
			pSrc[i] = (MV_PU32)ptmp;
	#endif
		}
		else
		{
			sgd_get_vaddr( strm[i].sgd, p );
			pSrc[i] = (MV_PU32) p;
		}
	}

	MV_CopyMemory( 
		pSrc[1]+strm[1].off/sizeof(MV_U32), 
		pSrc[0]+strm[0].off/sizeof(MV_U32),
		byte_cnt );

	for(i = 0; i < 2; i++ ) {
		if( mapped[i] ) {
			if(i==0)
				sgd_kunmap(pCore,strm[i].sgd, ptmp);
			else
				sgd_kunmap_sec(pCore,strm[i].sgd, ptmp);
		}
	}		

#ifdef _OS_LINUX
	if( enabled == 1 )
		local_irq_restore(flags);
#endif		/* _OS_LINUX */

	return;
}


void CopySGs(
	PCore_Driver_Extension	pCore,
	PMV_SG_Table			srctbl,
	PMV_SG_Table			dsttbl
	)
{
	//MV_PU32					pSrc[2] = { NULL, };
	sgd_iter_t				sg_iter[2];
	MV_U32					wCount[2], count;
	MV_BOOLEAN				bFinished = MV_FALSE;
	MV_U8					bIndex;
	//MV_PVOID				p;
	xor_strm_t				strm[2];

	MV_ASSERT( srctbl->Byte_Count == dsttbl->Byte_Count );

	sgd_iter_init( &sg_iter[0], srctbl->Entry_Ptr, 0, srctbl->Byte_Count );
	sgd_iter_init( &sg_iter[1], dsttbl->Entry_Ptr, 0, dsttbl->Byte_Count );

	for( bIndex = 0; bIndex < 2; bIndex++ )
	{
		strm[bIndex].off = 0;
		sgd_iter_get_next( &sg_iter[bIndex], strm[bIndex].sgd );
		sgd_getsz( strm[bIndex].sgd, wCount[bIndex] );
	}

	while( !bFinished )
	{
		count = MV_MIN( wCount[0], wCount[1] );

		sg_memcpy( 
			pCore,
			strm,
			count
			);

		for( bIndex = 0; bIndex < 2; bIndex++ )
		{
			wCount[bIndex] -= count;
			if( wCount[bIndex] == 0 )
			{
				if( !sgd_iter_get_next( &sg_iter[bIndex], strm[bIndex].sgd ) )
				{
					bFinished = MV_TRUE;
					break;
				}
				sgd_getsz( strm[bIndex].sgd, wCount[bIndex] );
				strm[bIndex].off = 0;
			}
			else
				strm[bIndex].off += count;
		}
	}
}

#endif
