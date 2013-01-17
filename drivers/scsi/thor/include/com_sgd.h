#ifndef __MV_COM_SGD_H__
#define __MV_COM_SGD_H__

struct _sgd_tbl_t;
struct _sgd_t;

#define SGD_DOMAIN_MASK	0xF0000000L

#define SGD_EOT			(1L<<27)	/* End of table */
#define SGD_COMPACT		(1L<<26)	/* Compact (12 bytes) SG format, not verified yet */
#define SGD_WIDE		(1L<<25)	/* 32 byte SG format */
#define SGD_X64			(1L<<24)	/* the 2nd part of SGD_WIDE */
#define SGD_NEXT_TBL	(1L<<23)	/* Next SG table format */
#define SGD_VIRTUAL		(1L<<22)	/* Virtual SG format, either 32 or 64 bit is determined during compile time. */
#define SGD_REFTBL		(1L<<21)	/* sg table reference format, either 32 or 64 bit is determined during compile time. */
#define SGD_REFSGD		(1L<<20)	/* sg item reference format */
#define SGD_VP			(1L<<19)	/* virtual and physical, not verified yet */
#define SGD_VWOXCTX		(1L<<18)	/* virtual without translation context */
#define SGD_PCTX		(1L<<17)	/* sgd_pctx_t, 64 bit only */

typedef struct _sg_common_t
{
	MV_U32	dword1;
	MV_U32	dword2;
	MV_U32	flags;	/* SGD_xxx */
	MV_U32	dword3;
} sg_common_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_t
{
	MV_U64	baseAddr;
	MV_U32	flags;
	MV_U32	size;
} sgd_t;

/*---------------------------------------------------------------------------*/

#define GET_COMPACT_SGD_SIZE(sgd)	\
	((((sgd_compact_t*)(sgd))->flags) & 0x3FFFFFL)

#define SET_COMPACT_SGD_SIZE(sgd,sz) do {			\
	(((sgd_compact_t*)(sgd))->flags) &= ~0x3FFFFFL;	\
	(((sgd_compact_t*)(sgd))->flags) |= (sz);		\
} while(0)

#define SIZEOF_COMPACT_SGD	12

typedef struct _sgd_compact_t
{
	MV_U64	baseAddr;
	MV_U32	flags;
} sgd_compact_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_v32_t
{
	MV_PVOID	vaddr;
	MV_PVOID	xctx;
	MV_U32		flags;
	MV_U32		size;
} sgd_v32_t;

/* sgd_v_t defines 32/64 bit virtual sgd without translation context */
typedef struct _sgd_v_t
{
	union {
		MV_PVOID	vaddr;
		MV_U64		dummy;
	} u;
	MV_U32		flags;
	MV_U32		size;
} sgd_v_t;

typedef struct _sgd_v64_t
{
	union {
		MV_PVOID	vaddr;
		MV_U64		dummy;
	} u1;
	MV_U32	flags;
	MV_U32	size;

	union {
		MV_PVOID	xctx;
		MV_U64		dummy;
	} u2;
	MV_U32	flagsEx;
	MV_U32	rsvd;
} sgd_v64_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_ref32_t
{
	MV_PVOID	ref;
	MV_U32		offset;
	MV_U32		flags;
	MV_U32		size;
} sgd_ref32_t;

typedef struct _sgd_ref64_t
{
	union {
		MV_PVOID	ref;
		MV_U64		dummy;
	} u;
	MV_U32	flags;
	MV_U32	size;

	MV_U32	offset;
	MV_U32	rsvd1;
	MV_U32	flagsEx;
	MV_U32	rsvd2;
} sgd_ref64_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_nt_t
{
	union {
		struct _sgd_tbl_t*	next;
		MV_U64	dummy;
	} u;
	MV_U32	flags;
	MV_U32	rsvd;
} sgd_nt_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_vp_t
{
	MV_U64	baseAddr;
	MV_U32	flags;		// SGD_VP | SGD_WIDE
	MV_U32	size;

	union {
		MV_PVOID vaddr;
		MV_U64   dummy;
	} u;
	MV_U32	flagsEx;	// SGD_X64
	MV_U32	rsvd;
} sgd_vp_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_pctx_t
{
	MV_U64	baseAddr;
	MV_U32	flags;		// SGD_PCTX | SGD_WIDE
	MV_U32	size;

	union {
		MV_PVOID xctx;
		MV_U64   dummy;
	} u;
	MV_U32	flagsEx;	// SGD_X64
	MV_U32	rsvd;
} sgd_pctx_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_tbl_t
{
	MV_U8 Max_Entry_Count;
	MV_U8 Valid_Entry_Count;
	MV_U8 Flag;
	MV_U8 Reserved0;
	MV_U32 Byte_Count;
	sgd_t* Entry_Ptr;
} sgd_tbl_t;

#define sgd_table_init(sgdt,maxCnt,entries) do {	\
	MV_ZeroMemory(sgdt,sizeof(sgd_tbl_t));		\
	(sgdt)->Max_Entry_Count = (maxCnt);				\
	(sgdt)->Entry_Ptr = (sgd_t*)(entries);			\
} while(0)

/*---------------------------------------------------------------------------*/

#define sgd_inc(sgd) do {	\
	if( (sgd)->flags & SGD_COMPACT )					\
		sgd = (sgd_t*)(((unsigned char*) (sgd)) + 12);	\
	else if( (sgd)->flags & SGD_WIDE )					\
		sgd = (sgd_t*)(((unsigned char*) (sgd)) + 32);	\
	else sgd = (sgd_t*)(((unsigned char*) (sgd)) + 16);	\
} while(0)

#define sgd_get_vaddr(sgd,v) do {				\
	if( (sgd)->flags & SGD_VIRTUAL ) {			\
		if( (sgd)->flags & SGD_WIDE )			\
			(v) = ((sgd_v64_t*)(sgd))->u1.vaddr;\
		else (v) = ((sgd_v32_t*)(sgd))->vaddr;	\
	}											\
	else if( (sgd)->flags & SGD_VWOXCTX )		\
		(v) = ((sgd_v_t*)sgd)->u.vaddr;			\
	else if( (sgd)->flags & SGD_VP )			\
		(v) = ((sgd_vp_t*)(sgd))->u.vaddr;		\
	else										\
		MV_ASSERT(MV_FALSE);					\
} while(0)

#define sgd_get_xctx(sgd,v) do {	\
	if( (sgd)->flags & SGD_WIDE )	(v) = ((sgd_v64_t*)(sgd))->u2.xctx;	\
	else (v) = ((sgd_v32_t*)(sgd))->xctx;	\
} while(0)

#define sgd_get_ref(sgd,_ref) do {	\
	if( (sgd)->flags & SGD_WIDE ) (_ref) = ((sgd_ref64_t*)(sgd))->u.ref;	\
	else (_ref) = ((sgd_ref32_t*)(sgd))->ref;	\
} while(0)

#define sgd_set_ref(sgd,_ref) do {	\
	if( (sgd)->flags & SGD_WIDE ) ((sgd_ref64_t*)(sgd))->u.ref = (_ref);	\
	else ((sgd_ref32_t*)(sgd))->ref = (_ref);	\
} while(0)

#define sgd_get_reftbl(sgd,reft) do {	\
	if( (sgd)->flags & SGD_WIDE )		\
		(reft) = (sgd_tbl_t*) (((sgd_ref64_t*)(sgd))->u.ref);	\
	else (reft) = (sgd_tbl_t*)(((sgd_ref32_t*)(sgd))->ref);	\
} while(0)

#define sgd_get_refsgd(sgd,reft) do {	\
	if( (sgd)->flags & SGD_WIDE )		\
		(reft) = (sgd_t*) (((sgd_ref64_t*)(sgd))->u.ref);	\
	else (reft) = (sgd_t*)(((sgd_ref32_t*)(sgd))->ref);	\
} while(0)

#define sgd_get_refoff(sgd,off) do {	\
	if( (sgd)->flags & SGD_WIDE )	(off) = ((sgd_ref64_t*)(sgd))->offset;	\
	else (off) = ((sgd_ref32_t*)(sgd))->offset;	\
} while(0)

#define sgd_set_refoff(sgd,off) do {	\
	if( (sgd)->flags & SGD_WIDE )	((sgd_ref64_t*)(sgd))->offset = (off);	\
	else ((sgd_ref32_t*)(sgd))->offset = (off);	\
} while(0)

#define sgd_get_nexttbl(sgd,n) do {	\
	n = ((sgd_nt_t*)(sgd))->u.next;	\
} while(0)

#define sgd_mark_eot(sgd) \
	((sgd)->flags |= SGD_EOT)

#define sgd_clear_eot(sgd) \
	((sgd)->flags &= ~SGD_EOT)

#define sgd_eot(sgd)	\
	((sgd)->flags & SGD_EOT)

#define sgd_copy(sgdDst,sgdSrc) do {	\
	*(sgdDst) = *(sgdSrc);	\
	if( (sgdSrc)->flags & SGD_WIDE )	\
		(sgdDst)[1] = (sgdSrc)[1];	\
} while(0)

#define sgd_getsz(sgd,sz) do {				\
	if( (sgd)->flags & SGD_COMPACT )		\
		(sz) = GET_COMPACT_SGD_SIZE(sgd);	\
	else (sz) = (sgd)->size;				\
} while(0)

#define sgd_setsz(sgd,sz) do {				\
	if( (sgd)->flags & SGD_COMPACT )		\
		SET_COMPACT_SGD_SIZE(sgd,sz);		\
	else (sgd)->size = (sz);				\
} while(0)

#define sgdt_get_lastsgd(sgdt,sgd) do {		\
	(sgd) = &(sgdt)->Entry_Ptr[(sgdt)->Valid_Entry_Count];	\
	(sgd)--;								\
	if( (sgd)->flags & SGD_X64 ) (sgd)--;	\
} while(0)

/*---------------------------------------------------------------------------*/

typedef int (*sgd_visitor_t)(sgd_t* sgd, MV_PVOID pContext);

int sgd_table_walk(
	sgd_tbl_t*		sgdt,
	sgd_visitor_t	visitor,
	MV_PVOID		ctx 
	);

/*---------------------------------------------------------------------------*/

typedef struct _sgd_iter_t
{
	sgd_t*	sgd;		/* current SG */
	MV_U32	offset;		/* offset in the SG */
	MV_U32	remainCnt;
} sgd_iter_t;

void  sgd_iter_init(
	sgd_iter_t*	iter,
	sgd_t*		sgd,
	MV_U32		offset,
	MV_U32		count 
	);

int sgd_iter_get_next(
	sgd_iter_t*	iter,
	sgd_t*		sgd
	);

/*---------------------------------------------------------------------------*/

void sgd_dump(sgd_t* sg, char* prefix);
void sgdt_dump(sgd_tbl_t *SgTbl, char* prefix);

/*---------------------------------------------------------------------------*/

void sgdt_append(
	sgd_tbl_t*	sgdt,
	MV_U32		address,
	MV_U32		addressHigh,
	MV_U32		size
	);

void sgdt_append_pctx(
	sgd_tbl_t*	sgdt,
	MV_U32		address,
	MV_U32		addressHigh,
	MV_U32		size,
	MV_PVOID	xctx
	);

int sgdt_append_virtual(
	sgd_tbl_t* sgdt,
	MV_PVOID virtual_address,
	MV_PVOID translation_ctx,
	MV_U32 size 
	);

int sgdt_append_ref(
	sgd_tbl_t*	sgdt,
	MV_PVOID	ref,
	MV_U32		offset,
	MV_U32		size,
	MV_BOOLEAN	refTbl
	);

int sgdt_append_vp(
	sgd_tbl_t*	sgdt,
	MV_PVOID	virtual_address,
	MV_U32		size,
	MV_U32		address,
	MV_U32		addressHigh
	);

void
sgdt_copy_partial(
	sgd_tbl_t* sgdt,
	sgd_t**	ppsgd,
	MV_PU32	poff,
	MV_U32	size
	);

void sgdt_append_sgd(
	sgd_tbl_t*	sgdt,
	sgd_t*		sgd
	);

#define sgdt_append_reftbl(sgdt,refSgdt,offset,size)	\
	sgdt_append_ref(sgdt,refSgdt,offset,size,MV_TRUE)

#define sgdt_append_refsgd(sgdt,refSgd,offset,size)	\
	sgdt_append_ref(sgdt,refSgd,offset,size,MV_FALSE)

/*---------------------------------------------------------------------------*/

int sgdt_prepare_hwprd(
	MV_PVOID		pCore,
	sgd_tbl_t*		pSource,
	sgd_t*			pSg,
	int				availSgEntry
	);

#endif	/*__MV_COM_SGD_H__*/
