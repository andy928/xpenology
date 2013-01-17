/*
 * arch/arm/plat-feroceon/mv_drivers_lsp/mv_gpio/mv_gpio.c
 *
 * Marvell Feroceon SoC GPIO handling.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/gpio.h>

#include "mvCommon.h"
#include "mvOs.h"
#include "gpp/mvGpp.h"

static DEFINE_SPINLOCK(gpio_lock);

static inline void __set_direction(unsigned pin, int input)
{
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (input)
		mvGppTypeSet(grp, mask, MV_GPP_IN & mask);
	else
		mvGppTypeSet(grp, mask, MV_GPP_OUT & mask);
}

static void __set_level(unsigned pin, int high)
{
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (high)
		mvGppValueSet (grp, mask, mask);
	else
		mvGppValueSet (grp, mask, 0);
}

static inline void __set_blinking(unsigned pin, int blink)
{
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (blink)
		mvGppBlinkEn(grp, mask, mask);
	else
		mvGppBlinkEn(grp, mask, 0);
}

static inline int mv_gpio_is_valid(unsigned pin, int mode)
{
	return true;
}

/*
 * GENERIC_GPIO primitives.
 */
static int mv_gpio_direction_input(struct gpio_chip *chip, unsigned pin)
{
	unsigned long flags;

	if (!mv_gpio_is_valid(pin, GPIO_INPUT_OK))
		return -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	/* Configure GPIO direction. */
	__set_direction(pin, 1);

	spin_unlock_irqrestore(&gpio_lock, flags);

	return 0;
}

static int mv_gpio_get_value(struct gpio_chip *chip, unsigned pin)
{
	u32 val;
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (MV_REG_READ(GPP_DATA_OUT_EN_REG(grp)) & mask)
		val = mvGppValueGet(grp, mask) ^ mvGppPolarityGet(grp, mask);
	else
		val = MV_REG_READ(GPP_DATA_OUT_REG(grp));

	return (val >> (pin & 31)) & 1;
}

static int mv_gpio_direction_output(struct gpio_chip *chip, unsigned pin,
	int value)
{
	unsigned long flags;

	if (!mv_gpio_is_valid(pin, GPIO_OUTPUT_OK))
		return -EINVAL;

	spin_lock_irqsave(&gpio_lock, flags);

	/* Disable blinking. */
	__set_blinking(pin, 0);

	/* Configure GPIO output value. */
	__set_level(pin, value);

	/* Configure GPIO direction. */
	__set_direction(pin, 0);

	spin_unlock_irqrestore(&gpio_lock, flags);

	return 0;
}

static void mv_gpio_set_value(struct gpio_chip *chip, unsigned pin,
	int value)
{
	unsigned long flags;

	spin_lock_irqsave(&gpio_lock, flags);

	/* Configure GPIO output value. */
	__set_level(pin, value);

	spin_unlock_irqrestore(&gpio_lock, flags);
}

static int mv_gpio_request(struct gpio_chip *chip, unsigned pin)
{
	if (mv_gpio_is_valid(pin, GPIO_INPUT_OK) ||
	    mv_gpio_is_valid(pin, GPIO_OUTPUT_OK))
		return 0;
	return -EINVAL;
}

static struct gpio_chip mv_gpiochip = {
	.label			= "mv_gpio",
	.direction_input	= mv_gpio_direction_input,
	.get			= mv_gpio_get_value,
	.direction_output	= mv_gpio_direction_output,
	.set			= mv_gpio_set_value,
	.request		= mv_gpio_request,
	.base			= 0,
	.ngpio			= MV_GPP_MAX_PINS,
	.can_sleep		= 0,
};

void __init mv_gpio_init(void)
{
	gpiochip_add(&mv_gpiochip);
}

