#include "com_define.h"
#include "com_tag.h"
#include "com_dbg.h"

MV_VOID Tag_Init( PTag_Stack pTagStack, MV_U16 size )
{
	MV_U16 i;
	
	MV_DASSERT( size == pTagStack->Size );
	
	pTagStack->Top = size;
	pTagStack->TagStackType = FILO_TAG; 
	pTagStack->PtrOut = 0;
	for ( i=0; i<size; i++ )
	{
		pTagStack->Stack[i] = size-1-i;
	}
}

MV_VOID Tag_Init_FIFO( PTag_Stack pTagStack, MV_U16 size )
{
	MV_U16 i;
	
	MV_DASSERT( size == pTagStack->Size );
	
	pTagStack->Top = size;
	pTagStack->TagStackType = FIFO_TAG; 
	pTagStack->PtrOut = 0;
	for ( i=0; i<size; i++ )
	{
			pTagStack->Stack[i] = i;
	}
}

MV_U16 Tag_GetOne(PTag_Stack pTagStack)
{
	MV_U16 nTag;

	MV_DASSERT( pTagStack->Top>0 );
	if(pTagStack->TagStackType==FIFO_TAG)
	{
		nTag = pTagStack->Stack[pTagStack->PtrOut++];
		if(pTagStack->PtrOut>=pTagStack->Size)
			pTagStack->PtrOut=0;
		pTagStack->Top--;
		return nTag;
	}
	else
		return pTagStack->Stack[--pTagStack->Top];
}

MV_VOID Tag_ReleaseOne(PTag_Stack pTagStack, MV_U16 tag)
{
	MV_DASSERT( pTagStack->Top<pTagStack->Size );
	if(pTagStack->TagStackType==FIFO_TAG)
	{
		pTagStack->Stack[(pTagStack->PtrOut+pTagStack->Top)%pTagStack->Size] = tag;
		pTagStack->Top++;
	}
	else
		pTagStack->Stack[pTagStack->Top++] = tag;
}

MV_BOOLEAN Tag_IsEmpty(PTag_Stack pTagStack)
{
	if ( pTagStack->Top==0 )
	{
		return MV_TRUE;
	}
	return MV_FALSE;
}

