#ifndef _ASM_POWERPC_SETUP_H
#define _ASM_POWERPC_SETUP_H

#include <asm-generic/setup.h>

#ifdef CONFIG_SYNO_MPC85XX_COMMON
#define SYNO_CPLD_BASE    0xF8000000    // CS1
#define SYNO_CPLD_SIZE    0x10
#endif
#ifndef __ASSEMBLY__
extern void ppc_printk_progress(char *s, unsigned short hex);
#endif

#endif	/* _ASM_POWERPC_SETUP_H */
