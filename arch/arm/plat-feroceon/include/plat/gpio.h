/*
 * arch/arm/plat-feroceon/include/plat/gpio.h
 *
 * Marvell Feroceon SoC GPIO handling.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PLAT_GPIO_H
#define __PLAT_GPIO_H

#include <linux/init.h>

#define gpio_get_value  __gpio_get_value
#define gpio_set_value  __gpio_set_value
#define gpio_cansleep   __gpio_cansleep

#define GPIO_INPUT_OK		(1 << 0)
#define GPIO_OUTPUT_OK		(1 << 1)

/* Initialize gpiolib. */
void __init mv_gpio_init(void);

#endif
