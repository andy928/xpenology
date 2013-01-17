#include "com_define.h"

MV_U64 U64_ADD_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value += v32;
#else
	v64.parts.low += v32;
	v64.parts.high = 0;	
#endif
	return v64;
}

MV_U64 U64_SUBTRACT_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value -= v32;
#else
	v64.parts.low -= v32;
	v64.parts.high = 0;	
#endif
	return v64;
}

MV_U64 U64_MULTIPLY_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value *= v32;
#else
	v64.parts.low *= v32;
	v64.parts.high = 0;	
#endif
	return v64;
}

MV_U32 U64_MOD_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _OS_LINUX
	return do_div(v64.value, v32);
#else
	return (MV_U32) (v64.value % v32);
#endif /* _OS_LINUX */
}

MV_U64 U64_DIVIDE_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _OS_LINUX
	do_div(v64.value, v32);
#else
#ifdef _64_BIT_COMPILER
	v64.value /= v32;
#else
	v64.parts.high = 0;	
	v64.parts.low /= v32;
#endif /* _64_BIT_COMPILER */

#endif /* _OS_LINUX */
	return v64;
}

MV_I32 U64_COMPARE_U32(MV_U64 v64, MV_U32 v32)
{
	if (v64.parts.high > 0)
		return 1;
	if (v64.parts.low > v32)
		return 1;
#ifdef _64_BIT_COMPILER
	else if (v64.value == v32)
#else
	else if (v64.parts.low == v32)
#endif
		return 0;
	else
		return -1;
}

MV_U64 U64_ADD_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	v1.value += v2.value;
#else
	v1.parts.low += v2.parts.low;
	v1.parts.high = 0;	
#endif
	return v1;
}

MV_U64 U64_SUBTRACT_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	v1.value -= v2.value;
#else
	v1.parts.low -= v2.parts.low;
	v1.parts.high = 0;	
#endif
	return v1;
}

MV_U32 U64_DIVIDE_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _OS_LINUX
	MV_U32 ret = 0;
	while (v1.value > v2.value) {
		v1.value -= v2.value;
		ret++;
	}
	return ret;
#else
#ifdef _64_BIT_COMPILER
	v1.value /= v2.value;
#else
	v1.parts.high = 0;	
	v1.parts.low /= v2.parts.low;
#endif
	return v1.parts.low;
#endif
}

MV_I32 U64_COMPARE_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	if (v1.value > v2.value)
		return 1;
	else if (v1.value == v2.value)
		return 0;
	else
		return -1;
#else
	if (v1.value > v2.value)
		return 1;
	else if (v1.value == v2.value)
		return 0;
	else
		return -1;

#endif

}

#ifdef _OS_BIOS
MV_U64 ZeroU64(MV_U64 v1)
{
	v1.parts.low = 0;
	v1.parts.high = 0;

	return	v1;
}
#endif /*  _OS_BIOS */

