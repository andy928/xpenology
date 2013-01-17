#if !defined(COMMON_DEBUG_H)
#define COMMON_DEBUG_H

/* 
 *	Marvell Debug Interface
 * 
 *	MACRO
 *		MV_DEBUG is defined in debug version not in release version.
 *	
 *	Debug funtions:
 *		MV_PRINT:	print string in release and debug build.
 *		MV_DPRINT:	print string in debug build.
 *		MV_TRACE:	print string including file name, line number in release and debug build.
 *		MV_DTRACE:	print string including file name, line number in debug build.
 *		MV_ASSERT:	assert in release and debug build.
 *		MV_DASSERT: assert in debug build.
 */

#include "com_define.h"
/*
 *
 * Debug funtions
 *
 */

/* For both debug and release version */
#if defined(SIMULATOR)
#   include <assert.h>
#   define MV_PRINT printf
#   define MV_ASSERT assert
#   define MV_TRACE(_x_)                                   \
              do {	                                   \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);  \
                 MV_PRINT _x_;                             \
	      } while (0)
#elif defined(_OS_WINDOWS)
/* for VISTA */
#   if ((_WIN32_WINNT >= 0x0600) && defined(MV_DEBUG))
       ULONG _cdecl MV_PRINT(char* format, ...);
#   else
#      define MV_PRINT                      DbgPrint
#   endif /* ((_WIN32_WINNT >= 0x0600) && defined(MV_DEBUG)) */

#   if (defined(_CPU_IA_64B) || defined(_CPU_AMD_64B))
#      if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#         define NTAPI __stdcall
#      else
#         define NTAPI
#      endif /* (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) */
	
       void NTAPI DbgBreakPoint(void);
#      define MV_ASSERT(_condition_)    \
                 do { if (!(_condition_)) DbgBreakPoint(); } while(0)
#   else
#      define MV_ASSERT(_condition_)    \
                 do { if (!(_condition_)) {__asm int 3}; } while(0)
#   endif /*  (defined(_CPU_IA_64B) || defined(_CPU_AMD_64B)) */

#   define MV_TRACE(_x_)                                        \
              do {                                              \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);       \
                 MV_PRINT _x_;                                  \
              } while (0)

#elif defined(_OS_LINUX)
#   define MV_PRINT(format, arg...)	printk("%s %d:" format, __FILE__, __LINE__, ## arg)
#   define MV_ASSERT(_x_)  BUG_ON(!(_x_))

#   define MV_TRACE(_x_)                                        \
              do {                                              \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);       \
                 MV_PRINT _x_;                                  \
           } while(0)
#elif defined(__QNXNTO__)
#   define MV_PRINT      printk("%s: ", mv_full_name), printk
#   define MV_ASSERT(_x_)                                       \
              do {                                              \
		 if (!(_x_))                                    \
                    MV_PRINT("Assert at File %s: Line %d.\n",   \
                             __FILE__, __LINE__);               \
              } while (0)
#   define MV_TRACE(_x_)                                        \
              do {                                              \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);       \
                 printk   _x_;                                  \
           } while(0)
#else /* OTHER OSes */
#   define MV_PRINT(_x_)
#   define MV_ASSERT(_condition_)
#   define MV_TRACE(_x_)
#endif /* _OS_WINDOWS */


/* 
 * Used with MV_DBG macro, see below .
 * Should be useful for Win driver too, so it is placed here.
 *
 */
#define DMSG_POKE        0x0001  /* flag controlling the dump_stack()  */
#define DMSG_KERN        0x0002  /* kernel driver dbg msg */
#define DMSG_SCSI        0x0004  /* SCSI Subsystem dbg msg */
#define DMSG_CORE        0x0008  /* CORE dbg msg */
#define DMSG_HBA         0x0010  /* HBA dbg msg */
#define DMSG_RESER03     0x0020
#define DMSG_FREQ        0x0040  /* msg that'll pop up 100+ times per sec */
#define DMSG_IOCTL       0x0080  /* ioctl msg */ 
#define DMSG_MSG         0x0100  /* plain msg, should be enabled all time */
#define DMSG_SCSI_FREQ   0x0200  /* freq scsi dbg msg */
#define DMSG_RAID        0x0400  /* raid dbg msg */
#define DMSG_PROF        0x0800  /* profiling msg */
#define DMSG_PROF_FREQ   0x1000  /* freq profiling msg */
#define DMSG_RES     	 0x2000
#define DMSG_SAS         0x4000  /* sas msg */
#define DMSG_CORE_EH     0x8000  /* core error handling */
/* For debug version only */
#if defined(MV_DEBUG)
#   ifdef _OS_LINUX
       extern unsigned int mv_dbg_opts;
#      define MV_DBG(x,...)                          \
          do {                                       \
	     if (unlikely(x&mv_dbg_opts)) {          \
                MV_PRINT(__VA_ARGS__);               \
             }                                       \
          } while (0)

#      define MV_POKE()                              \
          do {                                       \
		dump_stack();                        \
          } while (0)

#      define MV_DPRINT(x)                           \
          do {                                       \
	     if (unlikely(DMSG_CORE&mv_dbg_opts))    \
	        MV_PRINT x;                          \
          } while (0)
#   else
#      define MV_PRINT        printk
#      define MV_DPRINT(x)	MV_PRINT x
/* in case drivers for non-linux os go crazy */
#      define MV_DBG(x)         do{}while(0)
#      define MV_POKE()         
#   endif /* _OS_LINUX */

#   define MV_DASSERT	        MV_ASSERT
#   define MV_DTRACE	        MV_DTRACE
#else
#   if defined(__QNXNTO__) || defined(_OS_LINUX)
#      define MV_DBG(x,...)        do{}while(0)
#   else
#      define MV_DBG(x)            do{}while(0)
#   endif /* __QNXNTO__ */
//#   define MV_PRINT(x,...)
#   define MV_POKE()  
#   define MV_DPRINT(x)
#   define MV_DASSERT(x)
#   define MV_DTRACE(x)
#endif /* MV_DEBUG */

MV_BOOLEAN mvLogRegisterModule(MV_U8 moduleId, MV_U32 filterMask, char* name);
MV_BOOLEAN mvLogSetModuleFilter(MV_U8 moduleId, MV_U32 filterMask);
MV_U32 mvLogGetModuleFilter(MV_U8 moduleId);
void mvLogMsg(MV_U8 moduleId, MV_U32 type, char* format, ...);

#endif /* COMMON_DEBUG_H */

