#ifndef COM_DEFINE_H
#define COM_DEFINE_H

/*
 *  This file defines Marvell OS independent primary data type for all OS.
 *
 *  We have macros to differentiate different CPU and OS.
 *
 *  CPU definitions:
 *  _CPU_X86_16B  
 *  Specify 16bit x86 platform, this is used for BIOS and DOS utility.
 *  _CPU_X86_32B
 *  Specify 32bit x86 platform, this is used for most OS drivers.
 *  _CPU_IA_64B
 *  Specify 64bit IA64 platform, this is used for IA64 OS drivers.
 *  _CPU_AMD_64B
 *  Specify 64bit AMD64 platform, this is used for AMD64 OS drivers.
 *
 *  OS definitions:
 *  _OS_WINDOWS
 *  _OS_LINUX
 *  _OS_FREEBSD
 *  _OS_BIOS
 *  __QNXNTO__
 */
 

#include "mv_os.h"

#if !defined(IN)
#   define IN
#endif

#if !defined(OUT)
#   define OUT
#endif

#if defined(_OS_LINUX) || defined(__QNXNTO__)
#   define    BUFFER_PACKED               __attribute__((packed))
#elif defined(_OS_WINDOWS)
#   define    BUFFER_PACKED
#elif defined(_OS_BIOS)
#   define    BUFFER_PACKED
#endif /* defined(_OS_LINUX) || defined(__QNXNTO__) */

#define MV_BIT(x)                         (1L << (x))

#if !defined(NULL)
#   define NULL 0
#endif  /* NULL */

#define MV_TRUE                           1
#define MV_FALSE                          0

typedef unsigned char  MV_BOOLEAN, *MV_PBOOLEAN;
typedef unsigned char  MV_U8, *MV_PU8;
typedef signed char  MV_I8, *MV_PI8;

typedef unsigned short  MV_U16, *MV_PU16;
typedef signed short  MV_I16, *MV_PI16;

typedef void    MV_VOID, *MV_PVOID;

#ifdef _OS_BIOS
    typedef MV_U8 GEN_FAR*  MV_LPU8;
    typedef MV_I8 GEN_FAR*  MV_LPI8;
    typedef MV_U16 GEN_FAR* MV_LPU16;
    typedef MV_I16 GEN_FAR* MV_LPI16;

    typedef MV_U32 GEN_FAR* MV_LPU32;
    typedef MV_I32 GEN_FAR* MV_LPI32;
    typedef void GEN_FAR*   MV_LPVOID;
#else
    typedef void            *MV_LPVOID;
#endif /* _OS_BIOS */

/* Pre-define segment in C code*/
#if defined(_OS_BIOS)
#   define BASEATTR         __based(__segname("_CODE")) 
#   define BASEATTRData     __based(__segname("_CODE")) 
#else
#   define BASEATTR 
#endif /* _OS_BIOS */

#ifdef DEBUG_COM_SPECIAL
	#define MV_DUMP_COM_SPECIAL(pString)  						{bDbgPrintStr(pString);bCOMEnter();}
	#define MV_DUMP32_COM_SPECIAL(pString, value) 				bDbgPrintStr_U32(pString, value)
	#define MV_DUMP16_COM_SPECIAL(pString, value)  				bDbgPrintStr_U16(pString, value)
	#define MV_DUMP32_COM_SPECIAL3(pString, value1, value2)  	bDbgPrintStr_U32_3(pString, value1, value2)
	#define MV_DUMP8_COM_SPECIAL(pString, value)  				bDbgPrintStr_U8(pString, value)	
	#define MV_HALTKEY_SPECIAL									waitForKeystroke()

#else
	#define MV_DUMP_COM_SPECIAL(pString)  						
	#define MV_DUMP32_COM_SPECIAL(pString, value) 				
	#define MV_DUMP16_COM_SPECIAL(pString, value)  				
	#define MV_DUMP32_COM_SPECIAL3(pString, value1, value2)  	
	#define MV_DUMP8_COM_SPECIAL(pString, value)  				
	#define MV_HALTKEY_SPECIAL									
#endif
/* For debug version only */
#ifdef DEBUG_BIOS
 #ifdef DEBUG_SHOW_ALL
	#define MV_DUMP32(_x_) 		{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMP16(_x_)  	{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMP8(_x_)  		{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPC32(_x_)  	{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPC16(_x_)  	{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPC8(_x_)  	{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPE32(_x_) 	{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPE16(_x_) 	{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPE8(_x_)  	{mvDebugDumpU8(_x_);bCOMEnter();}
 	#define MV_DUMP_COM(pString)  						{bDbgPrintStr(pString);bCOMEnter();}
	#define MV_DUMP32_COM(pString, value) 				bDbgPrintStr_U32(pString, value)
	#define MV_DUMP16_COM(pString, value)  				bDbgPrintStr_U16(pString, value)
	#define MV_DUMP32_COM3(pString, value1, value2)  	bDbgPrintStr_U32_3(pString, value1, value2)
	#define MV_DUMP8_COM(pString, value)  				bDbgPrintStr_U8(pString, value)
 #else
	#define MV_DUMP32(_x_) 		//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMP16(_x_)  	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMP8(_x_)  		//{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPC32(_x_)  	//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPC16(_x_)  	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPC8(_x_)  	//{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPE32(_x_) 	//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPE16(_x_) 	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPE8(_x_)  	//{mvDebugDumpU8(_x_);bCOMEnter();}
 	#define MV_DUMP_COM(pString)  						//{bDbgPrintStr(pString);bCOMEnter();}
	#define MV_DUMP32_COM(pString, value) 				//bDbgPrintStr_U32(pString, value)
	#define MV_DUMP16_COM(pString, value)  				//bDbgPrintStr_U16(pString, value)
	#define MV_DUMP32_COM3(pString, value1, value2)  	//bDbgPrintStr_U32_3(pString, value1, value2)
	#define MV_DUMP8_COM(pString, value)  				//bDbgPrintStr_U8(pString, value)
 
 #endif
 
	#define MV_BIOSDEBUG(pString, value)				bDbgPrintStr_U32(pString, value)				
	#define MV_HALTKEY			waitForKeystroke()
	#define MV_ENTERLINE		//mvChangLine()
#else
	#define MV_DUMP32(_x_) 		//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMP16(_x_)  	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMP8(_x_)  		//{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPC32(_x_)  	//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPC16(_x_)  	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPC8(_x_)  	//{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPE32(_x_) 	//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPE16(_x_) 	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPE8(_x_)  	//{mvDebugDumpU8(_x_);bCOMEnter();}
 	#define MV_DUMP_COM(pString)  						//{bDbgPrintStr(pString);bCOMEnter();}
	#define MV_DUMP32_COM(pString, value) 				//bDbgPrintStr_U32(pString, value)
	#define MV_DUMP16_COM(pString, value)  				//bDbgPrintStr_U16(pString, value)
	#define MV_DUMP32_COM3(pString, value1, value2)  	//bDbgPrintStr_U32_3(pString, value1, value2)
	#define MV_DUMP8_COM(pString, value)  				//bDbgPrintStr_U8(pString, value)

	#define MV_BIOSDEBUG(pString, value)
	#define MV_HALTKEY
	#define MV_ENTERLINE
	
#endif

#if defined(_OS_LINUX) || defined(__QNXNTO__)
    /* unsigned/signed long is 64bit for AMD64, so use unsigned int instead */
    typedef unsigned int MV_U32, *MV_PU32;
    typedef   signed int MV_I32, *MV_PI32;
    typedef unsigned long MV_ULONG, *MV_PULONG;
    typedef   signed long MV_ILONG, *MV_PILONG;
#else
    /* unsigned/signed long is 32bit for x86, IA64 and AMD64 */
    typedef unsigned long MV_U32, *MV_PU32;
    typedef   signed long MV_I32, *MV_PI32;
#endif /*  defined(_OS_LINUX) || defined(__QNXNTO__) */

#if defined(_OS_WINDOWS)
    typedef unsigned __int64 _MV_U64;
    typedef   signed __int64 _MV_I64;
#elif defined(_OS_LINUX) || defined(__QNXNTO__)
    typedef unsigned long long _MV_U64;
    typedef   signed long long _MV_I64;
#elif defined(_OS_FREEBSD)
#   error "No Where"
#else
#   error "No Where"
#endif /* _OS_WINDOWS */

typedef _MV_U64 BUS_ADDRESS;

#ifdef _OS_LINUX
#   if defined(__KCONF_64BIT__)
#      define _SUPPORT_64_BIT
#   else
#      ifdef _SUPPORT_64_BIT
#         error Error 64_BIT CPU Macro
#      endif
#   endif /* defined(__KCONF_64BIT__) */
#elif defined(_OS_BIOS) || defined(__QNXNTO__)
#   undef  _SUPPORT_64_BIT
#else
#   define _SUPPORT_64_BIT
#endif /* _OS_LINUX */

/*
 * Primary Data Type
 */
#if defined(_OS_WINDOWS)
    typedef union {
        struct {
            MV_U32 low;
            MV_U32 high;
        } parts;
        _MV_U64 value;
    } MV_U64, *PMV_U64, *MV_PU64;
#elif defined(_OS_LINUX) || defined(__QNXNTO__)
    typedef union {
        struct {
#   if defined (__MV_LITTLE_ENDIAN__)
            MV_U32 low;
            MV_U32 high;
#   elif defined (__MV_BIG_ENDIAN__)
            MV_U32 high;
            MV_U32 low;
#   else
#           error "undefined endianness"
#   endif /* __MV_LITTLE_ENDIAN__ */
        } parts;
        _MV_U64 value;
    } MV_U64, *PMV_U64, *MV_PU64;
#else
/* BIOS compiler doesn't support 64 bit data structure. */
    typedef union {
        struct {
             MV_U32 low;
             MV_U32 high;
        };
        struct {
             MV_U32 value;
             MV_U32 value1;
        };
    } _MV_U64,MV_U64, *MV_PU64, *PMV_U64;
#endif /* defined(_OS_LINUX) || defined(_OS_WINDOWS) || defined(__QNXNTO__)*/

/* PTR_INTEGER is necessary to convert between pointer and integer. */
#if defined(_SUPPORT_64_BIT)
   typedef _MV_U64 MV_PTR_INTEGER;
#else
   typedef MV_U32 MV_PTR_INTEGER;
#endif /* defined(_SUPPORT_64_BIT) */

/* LBA is the logical block access */
typedef MV_U64 MV_LBA;

#if defined(_CPU_16B)
    typedef MV_U32 MV_PHYSICAL_ADDR;
#else
    typedef MV_U64 MV_PHYSICAL_ADDR;
#endif /* defined(_CPU_16B) */

#if defined (_OS_WINDOWS)
typedef PVOID MV_FILE_HANDLE;
#elif defined(_OS_LINUX) || defined(__QNXNTO__)
typedef MV_I32 MV_FILE_HANDLE;
#endif

/* Product device id */
#define VENDOR_ID					0x11AB

#define DEVICE_ID_THORLITE_2S1P				0x6121
#define DEVICE_ID_THORLITE_0S1P				0x6101
#define DEVICE_ID_THORLITE_1S1P				0x6111
#define DEVICE_ID_THOR_4S1P				0x6141
#define DEVICE_ID_THOR_4S1P_NEW				0x6145
/* Revision ID starts from B1 */
#define DEVICE_ID_THORLITE_2S1P_WITH_FLASH              0x6122

 /* Odin lite version */
#define DEVICE_ID_6320					0x6320
#define DEVICE_ID_6340					0x6340 

/* mule-board */
#define DEVICE_ID_6440					0x6440

/* Non-RAID Odin */
#define DEVICE_ID_6445					0x6445

/* mule-board */
#define DEVICE_ID_6480					0x6480
#define DEVICE_ID_UNKNOWN				0xFFFF


/* OS_LINUX depedent definition*/

#if defined(_OS_LINUX) || defined(__QNXNTO__)
/* os-dependent data structures */
#define OSSW_DECLARE_SPINLOCK(x)  spinlock_t x
#define OSSW_DECLARE_TIMER(x)   struct timer_list x
#define MV_DECLARE_TIMER(x)   struct timer_list x
/* expect pointers */
#define OSSW_INIT_SPIN_LOCK(plock) spin_lock_init(plock)

#define OSSW_SPIN_LOCK_IRQ(plock)            \
             do {                            \
	              spin_lock_irq(plock);  \
             } while (0)

#define OSSW_SPIN_UNLOCK_IRQ(plock)           \
             do {                             \
	              spin_unlock_irq(plock); \
             } while (0)

#define OSSW_SPIN_LOCK_IRQSAVE(plock, flag)           \
             do {                                     \
	             spin_lock_irqsave(plock, flag);  \
             } while (0)

#define OSSW_SPIN_UNLOCK_IRQRESTORE(plock, flag)           \
             do {                                          \
	             spin_unlock_irqrestore(plock, flag);  \
             } while (0)


/* Delayed Execution Services */
#define OSSW_INIT_TIMER(ptimer) init_timer(ptimer)


#else

#define OSSW_DECLARE_SPINLOCK(x)  
#define OSSW_DECLARE_TIMER(x)  
#define MV_DECLARE_TIMER(x)   
/* expect pointers */
#define OSSW_INIT_SPIN_LOCK(plock) 

#define OSSW_SPIN_LOCK_IRQ(plock)           

#define OSSW_SPIN_UNLOCK_IRQ(plock)           

#define OSSW_SPIN_LOCK_IRQSAVE(plock, flag)          

#define OSSW_SPIN_UNLOCK_IRQRESTORE(plock, flag)           

/* Delayed Execution Services */
#define OSSW_INIT_TIMER(ptimer) 

#endif

#endif /* COM_DEFINE_H */
