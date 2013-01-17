#include <Copyright.h>
/********************************************************************************
* debug.c
*
* DESCRIPTION:
*       Debug message display routine
*
* DEPENDENCIES:
*       OS Dependent
*
* FILE REVISION NUMBER:
*       $Revision: 1.2 $
*******************************************************************************/
#include <msApi.h>

#ifdef DEBUG_QD
#ifdef MV_NETBSD
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/stdarg.h>
#elif _VXWORKS
#include "vxWorks.h"
#include "logLib.h"
#include "stdarg.h"
#elif defined(WIN32)
#include "windows.h"
/* #include "wdm.h" */
#elif defined(LINUX)
#include "stdarg.h"
#endif

/*******************************************************************************
* gtDbgPrint
*
* DESCRIPTION:
*       .
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*
* COMMENTS:
*       None
*
*******************************************************************************/
#if defined(_VXWORKS) || defined(WIN32) || defined(LINUX) || defined(MV_NETBSD)
void gtDbgPrint(char* format, ...)
{
    va_list argP;
    char dbgStr[1000] = "";

    va_start(argP, format);

    vsprintf(dbgStr, format, argP);

#ifdef MV_NETBSD
	printf("%s",dbgStr);

#elif _VXWORKS
	printf("%s",dbgStr);
/*	logMsg(dbgStr,0,1,2,3,4,5); */
#elif defined(WIN32)
	printf("%s",dbgStr);
/*	DbgPrint(dbgStr);*/
#elif defined(LINUX)
	printk("%s",dbgStr);
#endif
	return;
}
#else
void gtDbgPrint(char* format, ...)
{
	GT_UNUSED_PARAM(format);
}
#endif
#else /* DEBUG_QD not defined */
void gtDbgPrint(char* format, ...)
{
	GT_UNUSED_PARAM(format);
}
#endif /* DEBUG_QD */

