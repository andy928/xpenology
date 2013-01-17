/*
 * Synology Kirkwood NAS Board GPIO define
 *
 * Maintained by:  KueiHuan Chen <khchen@synology.com>
 *
 * Copyright 2009-2010 Synology, Inc.  All rights reserved.
 * Copyright 2009-2010 KueiHuan.Chen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __ARCH_SYNOLOGY_KIRKWOOD_H_
#define __ARCH_SYNOLOGY_KIRKWOOD_H_

#define SYNO_DS409_ID           (0x13)
#define SYNO_DS409slim_ID       (0x14)
#define SYNO_DS109_ID           (0x15)

#define SATAHC_LED_CONFIG_REG	(SATA_VIRT_BASE | 0x2C)
#define SATAHC_LED_ACT          0x0
#define SATAHC_LED_ACT_PRESENT  0x4

#define DISK_LED_OFF        0
#define DISK_LED_GREEN_SOLID    1
#define DISK_LED_ORANGE_SOLID   2
#define DISK_LED_ORANGE_BLINK   3

typedef struct __tag_SYNO_FAN_GPIO {
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan_fail;
} SYNO_6281_FAN_GPIO;

typedef struct __tag_SYNO_6281_HDD_LED_GPIO {
	u8 hdd1_led_0;
	u8 hdd1_led_1;
	u8 hdd2_led_0;
	u8 hdd2_led_1;
	u8 hdd3_led_0;
	u8 hdd3_led_1;
	u8 hdd4_led_0;
	u8 hdd4_led_1;
	u8 hdd5_led_0;
	u8 hdd5_led_1;
} SYNO_6281_HDD_LED_GPIO;

typedef struct __tag_SYNO_109_GPIO {
	u8 hdd2_fail_led;
	u8 hdd1_fail_led;
	u8 hdd2_power;
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan1_fail;
}SYNO_109_GPIO;

typedef struct __tag_SYNO_409_GPIO {
	u8 alarm_led;
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan_sense;
	u8 inter_lock;
	u8 model_id_0;
	u8 model_id_1;
	u8 hdd1_led_0;
	u8 hdd1_led_1;
	u8 hdd2_led_0;
	u8 hdd2_led_1;
	u8 hdd3_led_0;
	u8 hdd3_led_1;
	u8 hdd4_led_0;
	u8 hdd4_led_1;
	u8 hdd5_led_0;
	u8 hdd5_led_1;
	u8 buzzer_mute_req;
	u8 buzzer_mute_ack;
	u8 rps1_on;
	u8 rps2_on;
} SYNO_409_GPIO;

typedef struct __tag_SYNO_409slim_GPIO {
	u8 hdd1_led_0;
	u8 hdd1_led_1;
	u8 hdd2_led_0;
	u8 hdd2_led_1;
	u8 hdd3_led_0;
	u8 hdd3_led_1;
	u8 hdd4_led_0;
	u8 hdd4_led_1;
	u8 bp_lock_out;
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan1_fail;
} SYNO_409slim_GPIO;
#endif
