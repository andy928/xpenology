#include "com_define.h"
#include "com_type.h"
#include "com_dbg.h"

MV_BOOLEAN mvLogRegisterModule(MV_U8 moduleId, MV_U32 filterMask, char* name)
{
	return MV_TRUE;
}

void mvLogMsg(MV_U8 moduleId, MV_U32 type, char* format, ...)
{
	va_list args;
	static char szMessageBuffer[1024];

	va_start(args, format);
	vsprintf(szMessageBuffer, format, args);
	va_end(args);
	MV_DPRINT((szMessageBuffer));
}
#if (defined(_OS_WINDOWS) && (_WIN32_WINNT >= 0x0600) && defined(MV_DEBUG) )
ULONG _cdecl MV_PRINT(char* format, ...)
{
	va_list args;
	static char szMessageBuffer[1024];
	ULONG result;

	va_start(args, format);
	vsprintf(szMessageBuffer, format, args);
	result = DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, szMessageBuffer);
	va_end(args);
	return result;
}
#endif
