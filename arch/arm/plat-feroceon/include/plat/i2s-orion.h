/*
 * include/asm-arm/plat-orion/i2s-orion.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_PLAT_ORION_I2S_ORION_H
#define __ASM_PLAT_ORION_I2S_ORION_H

#include <linux/mbus.h>

struct orion_i2s_platform_data {
	struct mbus_dram_target_info	*dram;
	int	spdif_rec;
	int	spdif_play;
	int	i2s_rec;
	int	i2s_play;
};


#endif
