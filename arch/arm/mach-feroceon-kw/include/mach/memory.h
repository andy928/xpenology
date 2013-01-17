/*
 * arch/arm/mach-feroceon-kw/include/mach/memory.h
 */

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#define PHYS_OFFSET		UL(0x00000000)

/* Override the ARM default */
#ifdef CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE

#if (CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE == 0)
#undef CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE
#define CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE 2
#endif

#define CONSISTENT_DMA_SIZE \
        (((CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE + 1) & ~1) * 1024 * 1024)
#endif

#endif
