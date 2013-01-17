/*
 * arch/asm-arm/mach-feroceon-kw/include/mach/gpio.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#include <mach/irqs.h>
#include <plat/gpio.h>
#include <asm-generic/gpio.h>		/* cansleep wrappers */

static inline int gpio_to_irq(int pin)
{
	return pin + IRQ_GPP_START;
}

static inline int irq_to_gpio(int irq)
{
	return irq - IRQ_GPP_START;
}
#endif

