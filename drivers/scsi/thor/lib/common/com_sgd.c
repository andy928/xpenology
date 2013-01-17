#include "com_type.h"
#include "com_define.h"
#ifdef USE_NEW_SGTABLE
#include "com_sgd.h"
#include "com_u64.h"
#include "com_dbg.h"
#include "com_util.h"
#include "hba_exp.h"

#if defined(_64BPLATFORM) || defined(_64_SYS_)
#define USES_64B_POINTER
#endif

int sg_iter_walk( 
	IN sgd_t* sgd, 
	IN MV_U32 offset, 
	IN MV_U32 count, 
	IN sgd_visitor_t visitor, 
	IN MV_PVOID context 
	)
{
	sgd_t	sg[2];
	int		sg_cnt = 0;
	MV_U32	sz;

	sgd_getsz(sgd,sz);
	while( sz <= offset )
	{
		offset -= sz;
		MV_ASSERT( !sgd_eot(sgd) );

		sg_cnt++;
		sgd_inc(sgd);
		sgd_getsz(sgd,sz);
	}

	while(1)
	{
		if( sgd->flags & (SGD_REFTBL|SGD_REFSGD) )
		{
			MV_U32 copy_count = sz - offset;
			MV_U32 offRef;
			sgd_tbl_t* refSgdt;
			sgd_t* refSgd;

			sgd_get_reftbl(sgd,refSgdt);
			if( sgd->flags & SGD_REFTBL )
				refSgd = refSgdt->Entry_Ptr;
			else
				refSgd = (sgd_t*) refSgdt;

			sgd_get_refoff(sgd,offRef);

			if( copy_count > count )
				copy_count = count;

			if( !sg_iter_walk(
				refSgd,
				offRef + offset,
				copy_count,
				visitor,
				context ) )
				return 0;
			count -= copy_count;
		}
		else if( sgd->flags & SGD_NEXT_TBL )
		{
			MV_ASSERT( MV_FALSE );	// TODO
		}
		else
		{
			sgd_copy( sg, sgd );

			if( offset )
			{
				sg[0].baseAddr = U64_ADD_U32(sg[0].baseAddr,offset);

				if( sgd->flags & SGD_VP )
				{
					((sgd_vp_t*)sg)->u.vaddr = ((MV_U8*) ((sgd_vp_t*)sg)->u.vaddr) + 
						offset;
				}

				if (sgd->flags & SGD_PCTX) {
					((sgd_pctx_t *)sg)->rsvd += offset;
				}

				sg[0].size -= offset;
			}

			if( sg[0].size > count )
				sg[0].size = count;

			if( !visitor( sg, context ) )
				return 0;

			count -= sg[0].size;
		}

		sg_cnt++;

		if( sgd_eot(sgd) 
			|| count==0 )
		{
			MV_ASSERT( count == 0 );
			break;
		}
		offset = 0;
		sgd_inc(sgd);
		sgd_getsz(sgd,sz);
	}

	return sg_cnt;
}

int sgd_table_walk(
	sgd_tbl_t*		sgdt,
	sgd_visitor_t	visitor,
	MV_PVOID		ctx 
	)
{
	return sg_iter_walk( 
		sgdt->Entry_Ptr,
		0,
		sgdt->Byte_Count,
		visitor,
		ctx );
}

void  sgd_iter_init(
	sgd_iter_t*	iter,
	sgd_t*		sgd,
	MV_U32		offset,
	MV_U32		count 
	)
{
	MV_U32	sz;

	sgd_getsz(sgd,sz);
	while( sz <= offset )
	{
		offset -= sz;
		MV_ASSERT( !sgd_eot(sgd) );
		sgd_inc(sgd);
		sgd_getsz(sgd,sz);
	}

	iter->sgd = sgd;
	iter->offset = offset;
	iter->remainCnt = count;
}

int sgd_iter_get_next(
	sgd_iter_t*	iter,
	sgd_t*		sgd
	)
{
	MV_U32	sz;

	if( iter->remainCnt == 0 )
		return 0;

	sgd_getsz(iter->sgd,sz);
	while( iter->offset >= sz )
	{
		if( sgd_eot(iter->sgd) )
		{
			iter->remainCnt = 0;
			return 0;
		}

		iter->offset -= sz;
		sgd_inc(iter->sgd);
		sgd_getsz(iter->sgd,sz);
	}
again:
	if( iter->sgd->flags & (SGD_REFTBL|SGD_REFSGD) )
	{
		sgd_iter_t	sub_iter;
		sgd_t*		refSgd;
		sgd_tbl_t*	refSgdt;
		MV_U32		sub_cnt = sz - iter->offset;
		MV_U32		offRef;

		if( sub_cnt > iter->remainCnt )
			sub_cnt = iter->remainCnt;

		sgd_get_reftbl(iter->sgd,refSgdt);

		if( iter->sgd->flags & SGD_REFTBL )
			refSgd = refSgdt->Entry_Ptr;
		else
			refSgd = (sgd_t*) refSgdt;

		sgd_get_refoff(iter->sgd,offRef);

		sgd_iter_init( 
			&sub_iter, 
			refSgd,
			offRef + iter->offset,
			sub_cnt );

		if( !sgd_iter_get_next( &sub_iter, sgd ) )
		{
			if( sgd_eot(iter->sgd) )
			{
				iter->remainCnt = 0;
				return 0;
			}
			sgd_inc(iter->sgd);
			iter->offset = 0;
			goto again;
		}
		else if( sgd->flags & SGD_NEXT_TBL )
		{
			MV_ASSERT( MV_FALSE );	// TODO
		}
		else
		{
			sgd_getsz(sgd,sz);
			if( sz > iter->remainCnt )
				sgd_setsz(sgd,iter->remainCnt);

			iter->offset += sz;
			iter->remainCnt -= sz;
		}

		return 1;
	}
	else
	{
		sgd_copy( sgd, iter->sgd );

		sgd->baseAddr = U64_ADD_U32(sgd->baseAddr,iter->offset);

		if( sgd->flags & SGD_VP )
		{
			((sgd_vp_t*)sgd)->u.vaddr = ((MV_U8*) ((sgd_vp_t*)sgd)->u.vaddr) + 
				iter->offset;
		}

		if (sgd->flags & SGD_PCTX) {
			((sgd_pctx_t *)sgd)->rsvd += iter->offset;
		}

		sz -= iter->offset;
		sgd_setsz( sgd, sz );
	}

	if( sz > iter->remainCnt )
	{
		sgd_setsz( sgd, iter->remainCnt );
		sz = iter->remainCnt;
	}

	iter->remainCnt -= sz;

	if( sgd_eot(iter->sgd) 
		|| iter->remainCnt == 0 )
	{
		iter->remainCnt = 0;
		return 1;
	}

	iter->offset = 0;
	sgd_inc(iter->sgd);

	return 1;
}

void sgd_dump(sgd_t* sg, char* prefix)
{
	MV_U32	sz;

	sgd_getsz(sg,sz);

	if( prefix )
	{
#ifndef _OS_LINUX
		MV_PRINT(prefix);
#endif
	}

	if( sg->flags & SGD_VIRTUAL )
	{
		MV_PVOID vaddr, xctx;

		sgd_get_vaddr(sg,vaddr);
		sgd_get_xctx(sg,xctx);

		MV_PRINT( "\tV %p T %p %08x F %08x\n"
			, vaddr
			, xctx
			, sz
			, sg->flags );
	}
	else if( sg->flags & (SGD_REFTBL|SGD_REFSGD) )
	{
		MV_PVOID ref;
		MV_U32	refOff;

		sgd_get_ref(sg,ref);
		sgd_get_refoff(sg,refOff);

		MV_PRINT( "\tR %p O %08x %08x F %08x\n"
			, ref
			, refOff
			, sz
			, sg->flags );
	}
	else if( sg->flags & SGD_NEXT_TBL )
	{
		MV_PVOID nexttbl;
		
		sgd_get_nexttbl(sg, nexttbl);

		MV_PRINT( "\tN %p F %08x\n"
			, nexttbl, sg->flags );

	}
	else if( sg->flags & SGD_VP )
	{
		sgd_vp_t* vp = (sgd_vp_t*) sg;
		MV_PRINT( "\tX %08x_%08x %p F %08x\n"
			, vp->baseAddr.parts.high
			, vp->baseAddr.parts.low
			, vp->u.vaddr
			, sg->flags );
	}
	else if( sg->flags & SGD_VWOXCTX )
	{
		sgd_v_t* vp = (sgd_v_t*) sg;

		MV_PRINT( "\tV %p T %p %08x F %08x\n"
			, vp->u.vaddr
			, (MV_PVOID)0
			, sz
			, sg->flags );
	}
	else if( sg->flags & SGD_PCTX )
	{
		sgd_pctx_t* p = (sgd_pctx_t*) sg;
		MV_PRINT( "\tP %08x_%08x %08x F %08x X %p\n"
			, p->baseAddr.parts.high, p->baseAddr.parts.low, p->size, p->flags
			, p->u.xctx );
	}
	else
	{
		MV_PRINT( "\tP %08x_%08x %08x F %08x\n"
		, sg->baseAddr.parts.high, sg->baseAddr.parts.low, sz, sg->flags );
	}
}

void sgdl_dump(sgd_t* sg, char* prefix )
{
	while(1)
	{
		sgd_dump(sg,prefix);

		if( sg->flags & SGD_REFTBL )
		{
			sgd_tbl_t* tbl;
			sgd_get_reftbl(sg,tbl);
			sgdl_dump( tbl->Entry_Ptr, "R " );
		}
		else if( sg->flags & SGD_REFSGD )
		{
			sgd_t* refsgd;
			sgd_get_refsgd(sg,refsgd);
			sgdl_dump( refsgd, "R " );
		}

		if( sgd_eot(sg) )
			break;
		sgd_inc(sg);
	}
}


void sgdt_dump(sgd_tbl_t *SgTbl, char* prefix)
{
	sgd_t* sg = SgTbl->Entry_Ptr;

	MV_PRINT( "%s %p %u of %u 0x%x bytes\n"
		, prefix ? prefix : " "
		, SgTbl
		, SgTbl->Valid_Entry_Count
		, SgTbl->Max_Entry_Count
		, SgTbl->Byte_Count
		);

	if( !SgTbl->Valid_Entry_Count )
		return;

#if 0
	sgdl_dump(sg, NULL);
#else
	while(1)
	{

		sgd_dump(sg,NULL);
		if( sgd_eot(sg) )
			break;
		sgd_inc(sg);
	}
#endif
}

void sgdt_clear_eot(
	sgd_tbl_t*	sgdt 
	)
{
	if( sgdt->Valid_Entry_Count )
	{
		sgd_t* sgd;
		sgdt_get_lastsgd(sgdt,sgd);

		sgd_clear_eot(sgd);
	}
}

void sgdt_append(
	sgd_tbl_t*	sgdt,
	MV_U32		address,
	MV_U32		addressHigh,
	MV_U32		size
	)
{
	sgd_t* pSGEntry = &sgdt->Entry_Ptr[sgdt->Valid_Entry_Count];

	MV_ASSERT( sgdt->Valid_Entry_Count+1<=sgdt->Max_Entry_Count );

	sgdt_clear_eot(sgdt);

	sgdt->Valid_Entry_Count += 1;
	sgdt->Byte_Count += size;

	pSGEntry->flags = 0;
	pSGEntry->baseAddr.parts.low = address;
	pSGEntry->baseAddr.parts.high = addressHigh;
	pSGEntry->size = size;

	sgd_mark_eot(pSGEntry);
}

void sgdt_append_pctx(
	sgd_tbl_t*	sgdt,
	MV_U32		address,
	MV_U32		addressHigh,
	MV_U32		size,
	MV_PVOID	xctx
	)
{
	sgd_pctx_t* pSGEntry = (sgd_pctx_t*) &sgdt->Entry_Ptr[sgdt->Valid_Entry_Count];

	MV_ASSERT( sgdt->Valid_Entry_Count+2<=sgdt->Max_Entry_Count );

	sgdt_clear_eot(sgdt);

	sgdt->Valid_Entry_Count += 2;
	sgdt->Byte_Count += size;

	pSGEntry->flags = SGD_PCTX | SGD_WIDE | SGD_EOT;
	pSGEntry->baseAddr.parts.low = address;
	pSGEntry->baseAddr.parts.high = addressHigh;
	pSGEntry->size = size;
	pSGEntry->u.xctx = xctx;
	pSGEntry->flagsEx = SGD_X64;
	pSGEntry->rsvd = 0;
}

void sgdt_append_sgd(
	sgd_tbl_t*	sgdt,
	sgd_t*		sgd
	)
{
	sgd_t*	pSGEntry = &sgdt->Entry_Ptr[sgdt->Valid_Entry_Count];
	MV_U8	cnt = 1;
	MV_U32	sgdsz;

	sgd_getsz(sgd,sgdsz);

	if( sgd->flags & SGD_WIDE )
		cnt++;

	MV_ASSERT( sgdt->Valid_Entry_Count+cnt<=sgdt->Max_Entry_Count );

	sgdt_clear_eot(sgdt);
	sgdt->Valid_Entry_Count += cnt;
	sgdt->Byte_Count += sgdsz;

	MV_CopyMemory( pSGEntry, sgd, sizeof(sgd_t) * cnt );

	sgd_mark_eot(pSGEntry);
}

static int sgdt_append_virtual_wo_xctx(
	sgd_tbl_t* sgdt,
	MV_PVOID virtual_address,
	MV_U32 size 
	)
{
	sgd_t* sg = &sgdt->Entry_Ptr[sgdt->Valid_Entry_Count];
	sgd_v_t* vsg = (sgd_v_t*) sg;

	MV_ASSERT( sgdt->Valid_Entry_Count+1<=sgdt->Max_Entry_Count );

	if( sgdt->Valid_Entry_Count + 1 > sgdt->Max_Entry_Count )
		return -1;	// not enough space

	sgdt_clear_eot(sgdt);

	vsg->flags = SGD_EOT | SGD_VWOXCTX;
	vsg->size = size;
	vsg->u.vaddr = virtual_address;

	sgdt->Valid_Entry_Count++;
	sgdt->Byte_Count += size;

	return 0;
}

int sgdt_append_virtual(
	sgd_tbl_t* sgdt,
	MV_PVOID virtual_address,
	MV_PVOID translation_ctx,
	MV_U32 size 
	)
{
	sgd_t* sg;
#ifdef USES_64B_POINTER
	sgd_v64_t* vsg;
#else
	sgd_v32_t* vsg;
#endif

	if( translation_ctx == 0 )
		return sgdt_append_virtual_wo_xctx(sgdt,virtual_address,size);

	sg = &sgdt->Entry_Ptr[sgdt->Valid_Entry_Count];

#ifdef USES_64B_POINTER
	vsg = (sgd_v64_t*) sg;

	MV_ASSERT( sgdt->Valid_Entry_Count+2<=sgdt->Max_Entry_Count );

	if( sgdt->Valid_Entry_Count + 2 > sgdt->Max_Entry_Count )
		return -1;	// not enough space

	sgdt_clear_eot(sgdt);

	vsg->u1.vaddr = virtual_address;
	vsg->u2.xctx = translation_ctx;
	vsg->flags = SGD_WIDE | SGD_VIRTUAL | SGD_EOT;
	vsg->flagsEx = SGD_X64;
	sgdt->Valid_Entry_Count++;
#else	// USES_64B_POINTER
	vsg = (sgd_v32_t*) sg;

	MV_ASSERT( sgdt->Valid_Entry_Count+1<=sgdt->Max_Entry_Count );

	if( sgdt->Valid_Entry_Count + 1 > sgdt->Max_Entry_Count )
		return -1;	// not enough space

	sgdt_clear_eot(sgdt);

	vsg->vaddr = virtual_address;
	vsg->xctx = translation_ctx;
	vsg->flags = SGD_VIRTUAL | SGD_EOT;
#endif	// !USES_64B_POINTER

	vsg->size = size;

	sgdt->Valid_Entry_Count++;
	sgdt->Byte_Count += size;

	return 0;
}

int sgdt_append_vp(
	sgd_tbl_t*	sgdt,
	MV_PVOID	virtual_address,
	MV_U32		size,
	MV_U32		address,
	MV_U32		addressHigh
	)
{
	sgd_vp_t* sg = (sgd_vp_t*) &sgdt->Entry_Ptr[sgdt->Valid_Entry_Count];

	MV_ASSERT( sgdt->Valid_Entry_Count+2<=sgdt->Max_Entry_Count );

	if( sgdt->Valid_Entry_Count + 2 > sgdt->Max_Entry_Count )
		return -1;	// not enough space

	sgdt_clear_eot(sgdt);

	sg->baseAddr.parts.low = address;
	sg->baseAddr.parts.high = addressHigh;
	sg->flags = SGD_VP | SGD_WIDE | SGD_EOT;
	sg->size = size;

	sg->u.vaddr = virtual_address;
	sg->flagsEx = SGD_X64;

	sgdt->Valid_Entry_Count += 2;
	sgdt->Byte_Count += size;

	return 0;
}

int sgdt_append_ref(
	sgd_tbl_t*	sgdt,
	MV_PVOID	ref,
	MV_U32		offset,
	MV_U32		size,
	MV_BOOLEAN	refTbl
	)
{
	sgd_t* sg;

	if( sgdt->Valid_Entry_Count )
	{
		sgdt_get_lastsgd(sgdt,sg);

		if( sg->flags&(SGD_REFTBL|SGD_REFSGD) )
		{
			MV_PVOID lastRef;
			MV_U32 lastOffset;
			
			sgd_get_ref(sg, lastRef);
			sgd_get_refoff(sg, lastOffset);

			if( lastRef == ref
				&& lastOffset + sg->size == offset )
			{
				// contiguous items!
				sg->size += size;
				sgdt->Byte_Count += size;
				return 0;
			}
		}				
	}

	sg = &sgdt->Entry_Ptr[sgdt->Valid_Entry_Count];

	{

#ifdef USES_64B_POINTER
	sgd_ref64_t* rsg = (sgd_ref64_t*) sg;
	MV_ASSERT( sgdt->Valid_Entry_Count+2<=sgdt->Max_Entry_Count );
	if( sgdt->Valid_Entry_Count + 2 > sgdt->Max_Entry_Count )
		return -1;	// not enough space
	sgdt_clear_eot(sgdt);
	rsg->u.ref = ref;
	sgdt->Valid_Entry_Count++;
	rsg->flags = SGD_WIDE | SGD_EOT | (refTbl ? SGD_REFTBL : SGD_REFSGD);
	rsg->flagsEx = SGD_X64;
#else

	sgd_ref32_t* rsg = (sgd_ref32_t*) sg;
	MV_ASSERT( sgdt->Valid_Entry_Count+1<=sgdt->Max_Entry_Count );
	if( sgdt->Valid_Entry_Count + 1 > sgdt->Max_Entry_Count )
		return -1;	// not enough space
	sgdt_clear_eot(sgdt);
	rsg->ref = ref;
	rsg->flags = SGD_EOT | (refTbl ? SGD_REFTBL : SGD_REFSGD);
#endif

	rsg->offset = offset;
	rsg->size = size;

	sgdt->Valid_Entry_Count++;
	sgdt->Byte_Count += size;

	}

	return 0;
}


void
sgdt_copy_partial(
	sgd_tbl_t* sgdt,
	sgd_t**	ppsgd,
	MV_PU32	poff,
	MV_U32	size
	)
{
	MV_U32	sgdsz;
	MV_U32	tmpSize;
	sgd_t	sgd[2];

	while( size )
	{
		sgd_getsz( *ppsgd, sgdsz );
		MV_ASSERT( sgdsz > *poff );

		tmpSize = MV_MIN( size, sgdsz - *poff );

		if( sgdt )
		{
			sgd_copy( sgd, *ppsgd );

			sgd_setsz( sgd, tmpSize );

			if( *poff )
			{
				if( sgd->flags & (SGD_REFTBL|SGD_REFSGD) )
				{
					MV_U32 refoff;
					sgd_get_refoff( sgd, refoff );
					sgd_set_refoff( sgd, refoff+(*poff) );
				}
				else
				{
					sgd->baseAddr = U64_ADD_U32( sgd->baseAddr, (*poff) );
					if( sgd->flags & SGD_VP )
					{
						sgd_vp_t* vp = (sgd_vp_t*) sgd;
						vp->u.vaddr = ((MV_U8*)vp->u.vaddr) + (*poff);
					}

					if (sgd->flags & SGD_PCTX) {
						sgd_pctx_t *pctx = 
							(sgd_pctx_t *)sgd;
						pctx->rsvd += (*poff);
					}
				}
			}

			sgdt_append_sgd( sgdt, sgd );

		}

		if( size == sgdsz - *poff 
			|| tmpSize == sgdsz - *poff )
		{
			sgd_inc( *ppsgd );
			(*poff) = 0;
		}
		else
			(*poff) += tmpSize;

		size -= tmpSize;
	}
}

#ifdef SIMULATOR

int SgVisitor(sgd_t* sg, MV_PVOID ctx)
{
	MV_U32* p = (MV_U32*)ctx;

	sgd_dump( sg, NULL );

	(*p)++;

	return 1;
}

void sgd_test()
{
	sgd_tbl_t SgTbl1 = {0,};
	sgd_tbl_t SgTbl2 = {0,};
	sgd_tbl_t SgTbl3 = {0,};
	sgd_t Entries1[32];
	sgd_t Entries2[32];
	sgd_t Entries3[32];

	SgTbl1.Max_Entry_Count = sizeof(Entries1)/sizeof(Entries1[0]);
	SgTbl1.Entry_Ptr = Entries1;

	SgTbl2.Max_Entry_Count = sizeof(Entries2)/sizeof(Entries2[0]);
	SgTbl2.Entry_Ptr = Entries2;

	SgTbl3.Max_Entry_Count = sizeof(Entries3)/sizeof(Entries3[0]);
	SgTbl3.Entry_Ptr = Entries3;

	int i;

	for( i = 0; i < 32; i++ )
	{
		sgdt_append( &SgTbl2, 0x80000000+i*0x1000, 0x90000000, 0x1000 );
	}

	sgdt_dump( &SgTbl2, " " );

	sgdt_append_reftbl( &SgTbl1, &SgTbl2, 0x3800, 0x1000*10 );
	sgdt_append_virtual( &SgTbl1, (MV_PVOID)0x40000, (MV_PVOID)0x60000, 0x1000*10 );
	sgdt_append_vp( &SgTbl1, (MV_PVOID)0x80000, 0x1000*10, 0x4000, 0 );

	sgdt_dump( &SgTbl1, " " );

	MV_PRINT( "Walking through the table:\n" );

	MV_U32 index = 0;
	sgd_table_walk( &SgTbl1, SgVisitor, &index );

	sgd_iter_t iter;
	sgd_t sg[2];

	sgd_iter_init( &iter, SgTbl1.Entry_Ptr, 0, SgTbl1.Byte_Count );

	MV_PRINT( "Walking through the table in another way:\n" );
	i = 0;
	while( sgd_iter_get_next( &iter, sg ) )
	{
		sgd_dump( sg, NULL );
	}

	sgdt_dump( &SgTbl1, " " );
	sgd_t* sgd = SgTbl1.Entry_Ptr;
	MV_U32 off = 0x1000;
	sgdt_copy_partial( &SgTbl3, &sgd, &off, 0x1000 );
	sgdt_copy_partial( &SgTbl3, &sgd, &off, 0x9000 );
	sgdt_copy_partial( &SgTbl3, &sgd, &off, 0x9000 );
	sgdt_copy_partial( &SgTbl3, &sgd, &off, 0x1000 );
	sgdt_copy_partial( &SgTbl3, &sgd, &off, 0x1000 );
	sgdt_dump( &SgTbl3, " " );
}
#endif

typedef struct _PRDTableWalkCtx
{
	MV_PVOID		pCore;
	sgd_t*			pSg;
	int				avail;
	int				itemCnt;
} PRDTableWalkCtx;

static int PRDTablePrepareVisitor(sgd_t* sg, MV_PVOID _ctx)
{
	PRDTableWalkCtx* ctx = (PRDTableWalkCtx*) _ctx;

	if( !ctx->avail )
		return 0;

	if( sg->flags & (SGD_VIRTUAL|SGD_VWOXCTX) )
	{
		MV_U32 totalSize, thisSize;
		MV_PVOID vaddr;
		MV_PVOID xctx;
		MV_U64 paddr;
#ifdef _OS_LINUX
		MV_ASSERT( 0 );
#endif /* _OS_LINUX */
		sgd_getsz( sg, totalSize );

		if( sg->flags & SGD_VIRTUAL )
		{
			sgd_get_vaddr( sg, vaddr );
			sgd_get_xctx( sg, xctx );
		}
		else
		{
			vaddr = ((sgd_v_t*)sg)->u.vaddr;
			xctx = 0;
		}

		while( 1 )
		{
			thisSize = totalSize;

			if( !HBA_ModuleGetPhysicalAddress(
					ctx->pCore, 
					vaddr,
					xctx,
					&paddr,
					&thisSize ) )
				return 0;

			ctx->avail--;
			ctx->itemCnt++;

			ctx->pSg->flags = 0;
#ifdef ODIN_DRIVER
			ctx->pSg->size = MV_CPU_TO_LE32(thisSize);
#else
			ctx->pSg->size = MV_CPU_TO_LE32(thisSize - 1);
#endif
			ctx->pSg->baseAddr.parts.low = MV_CPU_TO_LE32(paddr.parts.low);
			ctx->pSg->baseAddr.parts.high = MV_CPU_TO_LE32(paddr.parts.high);
			ctx->pSg++;

			totalSize -= thisSize;
			if( totalSize == 0 )
				break;

			if( !ctx->avail )
				return 0;

			vaddr = (MV_PVOID)((MV_PU8) vaddr + thisSize);
		}
	}
	else
	{
		// including SGD_VP/SGD_PCTX
		ctx->avail--;
		ctx->itemCnt++;

		ctx->pSg->flags = 0;
#ifdef ODIN_DRIVER
		ctx->pSg->size = MV_CPU_TO_LE32(sg->size);
#else
		ctx->pSg->size = MV_CPU_TO_LE32(sg->size - 1);
#endif
		ctx->pSg->baseAddr.parts.low = MV_CPU_TO_LE32(sg->baseAddr.parts.low);
		ctx->pSg->baseAddr.parts.high = MV_CPU_TO_LE32(sg->baseAddr.parts.high);
		ctx->pSg++;
	}

	return 1;
}

int sgdt_prepare_hwprd(
	MV_PVOID		pCore,
	sgd_tbl_t*		pSource,
	sgd_t*			pSg,
	int				availSgEntry
	)
{
	PRDTableWalkCtx ctx;

	ctx.pCore = pCore;
	ctx.pSg = pSg;
	ctx.avail = availSgEntry;
	ctx.itemCnt = 0;

	if( !sgd_table_walk( pSource, PRDTablePrepareVisitor, &ctx) )
		return 0;

	return ctx.itemCnt;
}

#endif // USE_NEW_SGTABLE
