/*
 * Synology Kirkwood NAS Board GPIO Setup
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
#ifdef CONFIG_SYNO_MV88F6281

#include <linux/gpio.h>
#include <linux/synobios.h>
#include "boardEnv/mvBoardEnvSpec.h"
#include "ctrlEnv/sys/mvSysSata.h"
#include "mvOs.h"

#define SYNO_KW_DS211 0x0
#define SYNO_KW_DS111 0x1
#define SYNO_KW_DS411 0x2
#define SYNO_KW_DS211P 0x3
#define SYNO_KW_DS212 0x5 
#define SYNO_KW_RS411 0x8 
#define SYNO_KW_RS212 0x9 
#define SYNO_KW_DS112j 0x0 
#define SYNO_KW_DS212w 0xc
#define SYNO_KW_DS412j 0xd

#define GPIO_UNDEF				0xFF

#define SATAHC_LED_CONFIG_REG	(SATA_REG_BASE | 0x2C)
#define SATAHC_LED_ACT          0x0
#define SATAHC_LED_ACT_PRESENT  0x4

/* copied from synobios.h */
#define DISK_LED_OFF			0
#define DISK_LED_GREEN_SOLID	1
#define DISK_LED_ORANGE_SOLID	2
#define DISK_LED_ORANGE_BLINK	3
#define DISK_LED_GREEN_BLINK    4

#define SYNO_LED_OFF		0
#define SYNO_LED_ON			1
#define SYNO_LED_BLINKING	2

#ifdef  MY_ABC_HERE
extern char gszSynoHWVersion[];
#endif

typedef struct __tag_SYNO_KW_HDD_PM_GPIO {
	u8 hdd1_pm;
	u8 hdd2_pm;
	u8 hdd3_pm;
	u8 hdd4_pm;
} SYNO_KW_HDD_PM_GPIO;

typedef struct __tag_SYNO_KW_FAN_GPIO {
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan_fail;
	u8 fan_fail_2;
	u8 fan_fail_3;
} SYNO_KW_FAN_GPIO;

typedef struct __tag_SYNO_KW_MODEL_GPIO {
	u8 model_id_0;
	u8 model_id_1;
	u8 model_id_2;
	u8 model_id_3;
} SYNO_KW_MODEL_GPIO;

typedef struct __tag_SYNO_KW_EXT_HDD_LED_GPIO {
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
} SYNO_KW_EXT_HDD_LED_GPIO;

typedef struct __tag_SYNO_KW_MULTI_BAY_GPIO {
	u8 inter_lock;
}SYNO_KW_MULTI_BAY_GPIO;

typedef struct __tag_SYNO_KW_SOC_HDD_LED_GPIO {
	u8 hdd2_fail_led;
	u8 hdd1_fail_led;
}SYNO_KW_SOC_HDD_LED_GPIO;

typedef struct __tag_SYNO_KW_RACK_GPIO {
	u8 buzzer_mute_req;
	u8 buzzer_mute_ack;
	u8 rps1_on;
	u8 rps2_on;
}SYNO_KW_RACK_GPIO;

typedef struct __tag_SYNO_KW_STATUS_LED_GPIO {
	u8 alarm_led;
	u8 power_led;
} SYNO_KW_STATUS_LED_GPIO;

typedef struct __tag_SYNO_KW_GENERIC_GPIO {
	SYNO_KW_EXT_HDD_LED_GPIO	ext_sata_led;
	SYNO_KW_SOC_HDD_LED_GPIO	soc_sata_led;
	SYNO_KW_MODEL_GPIO			model;
	SYNO_KW_FAN_GPIO			fan;
	SYNO_KW_HDD_PM_GPIO			hdd_pm;
	SYNO_KW_RACK_GPIO			rack;
	SYNO_KW_MULTI_BAY_GPIO		multi_bay;
	SYNO_KW_STATUS_LED_GPIO		status;
}SYNO_KW_GENERIC_GPIO;

static SYNO_KW_GENERIC_GPIO generic_gpio;

unsigned int Syno6282ModelIDGet(SYNO_KW_GENERIC_GPIO *pGpio)
{
	/*
	 * In 6702 platform, there is only 1 bit model ID
	 * 0x0 ds112j
	 * 0x1 Eagle
	 */
	 if( mvBoardIdGet() == SYNO_6702_1BAY_ID)
		 return (gpio_get_value(pGpio->model.model_id_0)?1:0);
	 else{ 

	/**
	 * 0x0 DS211
	 * 0x1 DS111
	 * 0x2 DS411
	 * 0x3 DS211p
	 * 0x4 DS411Slim
	 * 0x5 DS212
	 * 0x8 RS411, RS812
	 * 0x9 RS212
	 */
	return  (((gpio_get_value(pGpio->model.model_id_0) ? 1 : 0) << 3) | 
			 ((gpio_get_value(pGpio->model.model_id_1) ? 1 : 0) << 2) | 
			 ((gpio_get_value(pGpio->model.model_id_2) ? 1 : 0) << 1) | 
			 ((gpio_get_value(pGpio->model.model_id_3) ? 1 : 0) << 0));
	 }
}

int
SYNO_MV6281_GPIO_PIN(int pin, int *pValue, int isWrite)
{
	int ret = -1;

	if (!pValue)
		goto END;

	if (1 == isWrite)
		gpio_set_value(pin, *pValue);
	else
		*pValue = gpio_get_value(pin);

	ret = 0;
END:
	return 0;
}

extern MV_STATUS mvGppBlinkEn(MV_U32 group, MV_U32 mask, MV_U32 value);

int
SYNO_MV6281_GPIO_BLINK(int pin, int blink)
{
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (blink)
		mvGppBlinkEn(grp, mask, mask);
	else
		mvGppBlinkEn(grp, mask, 0);
	return 0;
}

int
SYNO_CTRL_INTERNAL_HDD_LED_SET(int index, int status)
{
	int ret = -1;
	int fail_led;

#ifdef MY_ABC_HERE
	extern long g_internal_hd_num;

	if ( 1 >= g_internal_hd_num ) {
		return 0;
	}
#endif

	WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd1_fail_led);
	WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd2_fail_led);

	MV_REG_WRITE(SATAHC_LED_CONFIG_REG, SATAHC_LED_ACT_PRESENT);

	//note: hd led is active low
	if ( DISK_LED_OFF == status ) {
		fail_led = 1;
	} else if ( DISK_LED_GREEN_SOLID == status ) {
		fail_led = 1;
	} else if ( DISK_LED_ORANGE_SOLID == status ||
		DISK_LED_ORANGE_BLINK == status ) {
		fail_led = 0;
	} else {
		printk("Wrong HDD led status [%d]\n", status);
		goto END;
	}

	switch (index) {
		case 1:
			gpio_set_value(generic_gpio.soc_sata_led.hdd1_fail_led, fail_led);
			break;
		case 2:
			gpio_set_value(generic_gpio.soc_sata_led.hdd2_fail_led, fail_led);
			break;
		default:
			printk("Wrong HDD number [%d]\n", index);
			goto END;
	}

	ret = 0;
END:
	return ret;
}

int
SYNO_CTRL_EXT_CHIP_HDD_LED_SET(int index, int status)
{
	int ret = -1;
	int pin1 = 0, pin2 = 0, bit1 = 0, bit2 = 0;

	bit1 = ( status >> 0 ) & 0x1;
	bit2 = ( status >> 1 ) & 0x1;

	switch (index) {
	case 1:
		pin1 = generic_gpio.ext_sata_led.hdd1_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd1_led_1;
		break;
	case 2:
		pin1 = generic_gpio.ext_sata_led.hdd2_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd2_led_1;
		break;
	case 3:
		pin1 = generic_gpio.ext_sata_led.hdd3_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd3_led_1;
		break;
	case 4:
		pin1 = generic_gpio.ext_sata_led.hdd4_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd4_led_1;
		break;
	case 5:
		if (generic_gpio.ext_sata_led.hdd5_led_0 == GPIO_UNDEF ||
			generic_gpio.ext_sata_led.hdd5_led_1 == GPIO_UNDEF) {
			//some 4 bay model don't contain such gpio.
			ret = 0;
			goto END;
		}
		pin1 = generic_gpio.ext_sata_led.hdd5_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd5_led_1;
		break;
	case 6:
		//for esata
		ret = 0;
		goto END;
	default:
		printk("Wrong HDD number [%d]\n", index);
		goto END;
	}

	WARN_ON(pin1 == GPIO_UNDEF);
	WARN_ON(pin2 == GPIO_UNDEF);

	gpio_set_value(pin1, bit1);
	gpio_set_value(pin2, bit2);

    ret = 0;
END:
    return ret;
}

int
SYNO_CTRL_USB_HDD_LED_SET(int status)
{
	int pin1 = GPIO_UNDEF, pin2 = GPIO_UNDEF, 
		bit1 = 0, bit2 = 0, 
		blink1 = 0, blink2 = 0;

	pin1 = generic_gpio.ext_sata_led.hdd1_led_0;
	pin2 = generic_gpio.ext_sata_led.hdd1_led_1;

	WARN_ON(pin1 == GPIO_UNDEF);
	WARN_ON(pin2 == GPIO_UNDEF);

	switch (status) {
	case DISK_LED_OFF:
		bit1 = 0;
		bit2 = 0;
		blink1 = 0;
		blink2 = 0;
		break;
	case DISK_LED_GREEN_SOLID:
		bit1 = 0;
		bit2 = 1;
		blink1 = 0;
		blink2 = 0;
		break;
	case DISK_LED_ORANGE_SOLID:
		bit1 = 1;
		bit2 = 0;
		blink1 = 0;
		blink2 = 0;
		break;
	case DISK_LED_ORANGE_BLINK:
		bit1 = 1;
		bit2 = 0;
		blink1 = 1;
		blink2 = 0;
		break;
	case DISK_LED_GREEN_BLINK:
		bit1 = 0;
		bit2 = 1;
		blink1 = 0;
		blink2 = 1;
		break;
	default:
		printk("Wrong disk led set.\n");
		break;
	}

	gpio_set_value(pin1, bit1);
	gpio_set_value(pin2, bit2);
	SYNO_MV6281_GPIO_BLINK(pin1, blink1);
	SYNO_MV6281_GPIO_BLINK(pin2, blink2);

	return 0;
}

int SYNO_CTRL_POWER_LED_SET(int status)
{
	int pin = GPIO_UNDEF, bit = 0, blink = 0;

	pin = generic_gpio.status.power_led;

	WARN_ON(pin == GPIO_UNDEF);

	switch (status) {
	case SYNO_LED_OFF:
		blink = 0;
		bit = 1;
		break;
	case SYNO_LED_ON:
		blink = 0;
		bit = 0;
		break;
	case SYNO_LED_BLINKING:
		blink = 1;
		bit = 0;
		break;
	}

	gpio_set_value(pin, bit);
	SYNO_MV6281_GPIO_BLINK(pin, blink);

	return 0;
}

int SYNO_CTRL_HDD_POWERON(int index, int value)
{
	int ret = -1;

	switch (index) {
	case 1:
		WARN_ON(GPIO_UNDEF == generic_gpio.hdd_pm.hdd1_pm);
		gpio_set_value(generic_gpio.hdd_pm.hdd1_pm, value);
		break;
	case 2:
		WARN_ON(GPIO_UNDEF == generic_gpio.hdd_pm.hdd2_pm);
		gpio_set_value(generic_gpio.hdd_pm.hdd2_pm, value);
		break;
	case 3:
		WARN_ON(GPIO_UNDEF == generic_gpio.hdd_pm.hdd3_pm);
		gpio_set_value(generic_gpio.hdd_pm.hdd3_pm, value);
		break;
	case 4:
		WARN_ON(GPIO_UNDEF == generic_gpio.hdd_pm.hdd4_pm);
		gpio_set_value(generic_gpio.hdd_pm.hdd4_pm, value);
		break;
	default:
		goto END;
	}

	ret = 0;
END:
	return ret;
}

int SYNO_CTRL_FAN_PERSISTER(int index, int status, int isWrite)
{
	int ret = 0;
	u8 pin = GPIO_UNDEF;

	switch (index) {
	case 1:
		pin = generic_gpio.fan.fan_1;
		break;
	case 2:
		pin = generic_gpio.fan.fan_2;
		break;
	case 3:
		pin = generic_gpio.fan.fan_3;
		break;
	default:
		ret = -1;
		printk("%s fan not match\n", __FUNCTION__);
		goto END;
	}

	WARN_ON(GPIO_UNDEF == pin);
	gpio_set_value(pin, status);
END:
	return ret;
}

int SYNO_CTRL_FAN_STATUS_GET(int index, int *pValue)
{
	int ret = 0;

	switch (index) {
		case 1:
			WARN_ON(GPIO_UNDEF == generic_gpio.fan.fan_fail);
			*pValue = gpio_get_value(generic_gpio.fan.fan_fail);
			break;
		case 2:
			WARN_ON(GPIO_UNDEF == generic_gpio.fan.fan_fail_2);
			*pValue = gpio_get_value(generic_gpio.fan.fan_fail_2);
			break;
		case 3:
			WARN_ON(GPIO_UNDEF == generic_gpio.fan.fan_fail_3);
			*pValue = gpio_get_value(generic_gpio.fan.fan_fail_3);
			break;
		default:
			WARN_ON(1);
			break;
	}

	if(*pValue)
		*pValue = 0;
	else
		*pValue = 1;

	return ret;
}

int SYNO_CTRL_ALARM_LED_SET(int status)
{
	WARN_ON(GPIO_UNDEF == generic_gpio.status.alarm_led);

	gpio_set_value(generic_gpio.status.alarm_led, status);
	return 0;
}

int SYNO_CTRL_BACKPLANE_STATUS_GET(int *pStatus)
{
	WARN_ON(GPIO_UNDEF == generic_gpio.multi_bay.inter_lock);

	*pStatus = gpio_get_value(generic_gpio.multi_bay.inter_lock);
	return 0;
}

int SYNO_CTRL_BUZZER_CLEARED_GET(int *pValue)
{
	int tempVal = 0;

	WARN_ON(GPIO_UNDEF == generic_gpio.rack.buzzer_mute_req);

	tempVal = gpio_get_value(generic_gpio.rack.buzzer_mute_req);
	if ( tempVal ) {
		*pValue = 0;
	} else {
		*pValue = 1;
		tempVal = 1;
	}

	return 0;
}

MV_U8 SYNOKirkwoodIsBoardNeedPowerUpHDD(MV_U32 disk_id) {
	u8 ret = 0;
	MV_U32 boardId = mvBoardIdGet();

	switch(boardId) {
	case SYNO_DS109_ID:
		if ((0 == strncmp(HW_DS212jv10, gszSynoHWVersion, strlen(HW_DS212jv10)) || 0 == strncmp(HW_DS212jv20, gszSynoHWVersion, strlen(HW_DS212jv20))) && 2 >= disk_id)
			ret = 1;
		else if (2 == disk_id)
			ret = 1;
		break;
	case SYNO_DS212_ID:
		if ((0 == strncmp(HW_DS112, gszSynoHWVersion, strlen(HW_DS112)) || 0 == strncmp(HW_DS112pv10, gszSynoHWVersion, strlen(HW_DS112pv10))) && 1 == disk_id ) {
			ret = 1;
		} else if (2 >= disk_id ) {
			ret = 1;
		}
		break;
	case SYNO_DS211_ID:
		if (2 == disk_id &&
			SYNO_KW_DS111 != Syno6282ModelIDGet(&generic_gpio))
			ret = 1;
		break;
	case SYNO_DS411slim_ID: 
	case SYNO_DS411_ID:
		if ( 2 <= disk_id && SYNO_KW_DS411 == Syno6282ModelIDGet(&generic_gpio))
			ret = 1;
		else if (4 >= disk_id && SYNO_KW_DS412j == Syno6282ModelIDGet(&generic_gpio)) {
			ret = 1;
		}
		break;
	case SYNO_6702_1BAY_ID:
		if (1==disk_id)
			ret = 1;
		break;
	default:
		break;
	}

	return ret;
}

EXPORT_SYMBOL(SYNOKirkwoodIsBoardNeedPowerUpHDD);
EXPORT_SYMBOL(SYNO_MV6281_GPIO_PIN);
EXPORT_SYMBOL(SYNO_MV6281_GPIO_BLINK);
EXPORT_SYMBOL(SYNO_CTRL_INTERNAL_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_EXT_CHIP_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_USB_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_POWER_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_HDD_POWERON);
EXPORT_SYMBOL(SYNO_CTRL_FAN_PERSISTER);
EXPORT_SYMBOL(SYNO_CTRL_FAN_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_ALARM_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_BACKPLANE_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_BUZZER_CLEARED_GET);

/*
Pin			Mode	Signal select and definition	output only		Pull-up/pull-down
MPP0		0x2		SPI signal
MPP[1:3]	0x2		SPI signal						V
MPP4		0x0		GPIO (2bay is inter lock)
MPP5		0x0		GPIO							V
MPP6		0x1		SYSRST_OUTn
MPP7		0x0		GPIO							V
MPP[8:9]	0x1		TWSI
MPP10		0x3		UART0							V
MPP11		0x3		UART0
MPP12		0x0		GPIO							V
MPP[13:14]	0x3		UART1
MPP15		0x0		FAN_150											Default 1 Hi, Low=off
MPP16		0x0		FAN_100											Default 1 Hi, Low=off
MPP17		0x0		FAN_33											Default 0 Low, Hi=ON
MPP[18:19]	0x0		GPIO							V
MPP20		0x5		SATA port 1 ACT LED								See 8.3 for detail
MPP21		0x5		SATA port 0 ACT LED								See 8.3 for detail
MPP22		0x0		SATA port 1 PRESENT LED			
MPP23		0x0		SATA port 0 PRESENT LED			
MPP[24:27]	0x0		GPIO
MPP[28:29]	0x0		GPIO Model ID 0,1
MPP[30:32]	0x0		GPIO
MPP33		0x0		GPIO							V
MPP34		0x0		GPO  (2 bay HDD2 enable)		V
MPP35		0x0		FAN SENSE										Hi=Fail, pulse=running
MPP[36:45]	0x0		GPIO
MPP[46:47]	0x0		GPIO Model ID 2,3
MPP[48:49]	0x0		GPIO
*/
static void 
KW_6282_211_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	SYNO_KW_GENERIC_GPIO gpio_211 = {
		.ext_sata_led = {
							.hdd1_led_0 = GPIO_UNDEF,
							.hdd1_led_1 = GPIO_UNDEF,
							.hdd2_led_0 = GPIO_UNDEF,
							.hdd2_led_1 = GPIO_UNDEF,
							.hdd3_led_0 = GPIO_UNDEF,
							.hdd3_led_1 = GPIO_UNDEF,
							.hdd4_led_0 = GPIO_UNDEF,
							.hdd4_led_1 = GPIO_UNDEF,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led = {
							.hdd2_fail_led = 22,
							.hdd1_fail_led = 23,
						},
		.model		  = {
							.model_id_0 = 28,
							.model_id_1 = 29,
							.model_id_2 = 46,
							.model_id_3 = 47,
						},
		.fan		  = {
							.fan_1 = 15,
							.fan_2 = 16,
							.fan_3 = 17,
							.fan_fail = 35,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = 34,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = GPIO_UNDEF,
							.buzzer_mute_ack = GPIO_UNDEF,
							.rps1_on = GPIO_UNDEF,
							.rps2_on = GPIO_UNDEF,
						},
		.multi_bay	  = {
							.inter_lock = 4,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = GPIO_UNDEF,
						},
	};

	if (SYNO_KW_DS111 == Syno6282ModelIDGet(&gpio_211)) {
		gpio_211.multi_bay.inter_lock = GPIO_UNDEF;
		gpio_211.hdd_pm.hdd2_pm = GPIO_UNDEF;
		printk("Apply DS 111 GPIO\n");
	} else if (SYNO_KW_DS211P == Syno6282ModelIDGet(&gpio_211)) {
		gpio_211.fan.fan_1 = GPIO_UNDEF;
		gpio_211.fan.fan_2 = GPIO_UNDEF;
		gpio_211.fan.fan_3 = GPIO_UNDEF;
		printk("Apply DS 211+ GPIO\n");
	} else if (SYNO_KW_DS212 == Syno6282ModelIDGet(&gpio_211) ||
		SYNO_KW_DS212w == Syno6282ModelIDGet(&gpio_211)) {
		gpio_211.fan.fan_1 = GPIO_UNDEF;
		gpio_211.fan.fan_2 = GPIO_UNDEF;
		gpio_211.fan.fan_3 = GPIO_UNDEF;
		gpio_211.hdd_pm.hdd1_pm =30 ,
		printk("Apply DS 212 GPIO\n");
	}

	*global_gpio = gpio_211;
}

static void 
KW_6282_112_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	KW_6282_211_GPIO_init(global_gpio);
	global_gpio->hdd_pm.hdd1_pm = 30;
}

static void
KW_6282_112p_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	KW_6282_211_GPIO_init(global_gpio);
	global_gpio->hdd_pm.hdd1_pm = 30;
}

/************************
*   DS411slim
*************************/
/*
Pin         Mode    Signal select and definition    output only     Pull-up/pull-down
MPP0        0x2     SPI signal
MPP[1:3]    0x2     SPI signal                      V
MPP4        0x0     Inter lock
MPP5        0x0     GPIO                            V
MPP6        0x1     SYSRST_OUTn
MPP7        0x0     GPIO                            V
MPP[8:9]    0x1     TWSI
MPP10       0x3     UART0                           V
MPP11       0x3     UART0
MPP12       0x0     GPIO                            V
MPP[13:14]  0x3     UART1
MPP15       0x0     FAN_150                                         Default 1 Hi, Low=off
MPP16       0x0     FAN_100                                         Default 1 Hi, Low=off
MPP17       0x0     FAN_33                                          Default 0 Low, Hi=ON
MPP[18:19]  0x0     GPIO                            V
MPP[20:23]	0x3     GE1 TXD
MPP[24:27]  0x3     GE1 RXD
MPP[28:29]  0x0     GPIO Model ID 0,1
MPP30       0x3     GE1_RXCTL
MPP31       0x3     GE1_RXCLK
MPP32       0x3     GE1_TCLKOUT
MPP33       0x3     GE1_TXCTL                       V
MPP34       0x0     GPO  (2 bay HDD2 enable) /Buzzer off      V
MPP35       0x0     FAN SENSE                                       Hi=Fail, pulse=running
MPP36       0x0     GPIO Disk LED A0
MPP37       0x0     GPIO Disk LED A1
MPP38       0x0     GPIO Disk LED B0
MPP39       0x0     GPIO Disk LED B1
MPP40       0x0     GPIO Disk LED C0
MPP41       0x0     GPIO Disk LED C1
MPP42       0x0     GPIO Disk LED D0
MPP43       0x0     GPIO Disk LED D1
MPP44       0x0     GPIO HDD3 enable/ Fan Sense 2
MPP45       0x0     GPIO HDD4 enable/ Fan sense 3
MPP[46:47]  0x0     GPIO Model ID 2,3
MPP[48:49]  0x0     GPIO
*/
static void 
KW_6282_DS_4BAY_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	SYNO_KW_GENERIC_GPIO gpio_ds_6282_4bay = {
		.ext_sata_led = {
							.hdd1_led_0 = 36,
							.hdd1_led_1 = 37,
							.hdd2_led_0 = 38,
							.hdd2_led_1 = 39,
							.hdd3_led_0 = 40,
							.hdd3_led_1 = 41,
							.hdd4_led_0 = 42,
							.hdd4_led_1 = 43,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
						},
		.model		  = {
							.model_id_0 = 28,
							.model_id_1 = 29,
							.model_id_2 = 46,
							.model_id_3 = 47,
						},
		.fan		  = {
							.fan_1 = 15,
							.fan_2 = 16,
							.fan_3 = 17,
							.fan_fail = 35,
							.fan_fail_2 = 48,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = GPIO_UNDEF,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = GPIO_UNDEF,
							.buzzer_mute_ack = GPIO_UNDEF,
							.rps1_on = GPIO_UNDEF,
							.rps2_on = GPIO_UNDEF,
						},
		.multi_bay	  = {
							.inter_lock = 4,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = GPIO_UNDEF,
						},
	};

	//DS412j uses the same Model ID with DS411, and their difference is HDD1 power GPIO only
	if (SYNO_KW_DS411 == Syno6282ModelIDGet(&gpio_ds_6282_4bay)) {
		gpio_ds_6282_4bay.hdd_pm.hdd2_pm = 34;
		gpio_ds_6282_4bay.hdd_pm.hdd3_pm = 44;
		gpio_ds_6282_4bay.hdd_pm.hdd4_pm = 45;
		printk("Apply DS 411 GPIO\n");
	}

	if (SYNO_KW_DS412j == Syno6282ModelIDGet(&gpio_ds_6282_4bay)){
		gpio_ds_6282_4bay.hdd_pm.hdd1_pm = 30;
		gpio_ds_6282_4bay.hdd_pm.hdd2_pm = 34;
		gpio_ds_6282_4bay.hdd_pm.hdd3_pm = 44;
		gpio_ds_6282_4bay.hdd_pm.hdd4_pm = 45;
		printk("Apply DS 412j GPIO\n");
	}

	*global_gpio = gpio_ds_6282_4bay;
}

static void 
KW_6282_RS411_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	SYNO_KW_GENERIC_GPIO gpio_rs411 = {
		.ext_sata_led = {
							.hdd1_led_0 = 36,
							.hdd1_led_1 = 37,
							.hdd2_led_0 = 38,
							.hdd2_led_1 = 39,
							.hdd3_led_0 = 40,
							.hdd3_led_1 = 41,
							.hdd4_led_0 = 42,
							.hdd4_led_1 = 43,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
						},
		.model		  = {
							.model_id_0 = 28,
							.model_id_1 = 29,
							.model_id_2 = 46,
							.model_id_3 = 47,
						},
		.fan		  = {
							.fan_1 = 15,
							.fan_2 = 16,
							.fan_3 = 17,
							.fan_fail = 35,
							.fan_fail_2 = 45,
							.fan_fail_3 = 44,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = GPIO_UNDEF,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = 34,
							.buzzer_mute_ack = GPIO_UNDEF,
							.rps1_on = GPIO_UNDEF,
							.rps2_on = GPIO_UNDEF,
						},
		.multi_bay	  = {
							.inter_lock = 4,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = GPIO_UNDEF,
						},
	};

	if (SYNO_KW_RS212 == Syno6282ModelIDGet(&gpio_rs411)) {
		gpio_rs411.ext_sata_led.hdd1_led_0 = 38;
		gpio_rs411.ext_sata_led.hdd1_led_1 = 39;
		gpio_rs411.ext_sata_led.hdd2_led_0 = 36;
		gpio_rs411.ext_sata_led.hdd2_led_1 = 37;
		gpio_rs411.fan.fan_fail_2 = 44;
		gpio_rs411.fan.fan_fail_3 = 45;
		printk("Apply RS212 GPIO\n");
	}

	*global_gpio = gpio_rs411;
}
/*
Pin         Mode    Signal select and definition    Input/output    Pull-up/pull-down
MPP[0:3]    0x2 SPI signal
MPP[4:7]    0x0 GPIO
MPP[8:9]    0x1 TWSI
MPP[10:11]  0x3 UART0
MPP12       0x0 GPIO
MPP[13:14]  0x3 UART1
MPP[15:19]  0x0 GPIO
MPP[20]     0x5 SATA port 1 ACT LED     output      See 8.3 for detail
MPP[21]     0x5 SATA port 0 ACT LED     output      See 8.3 for detail
MPP[22]     0x0 SATA port 1 FAULT LED       output
MPP[23]     0x0 SATA port 0 FAULT LED       output
MPP[24:35]  0x0 GPIO
MPP31       0x0 HDD2 enable (2 bay only)    output      Default Low, Hi=ON
MPP32       0x0 FAN_182             output      Default Low, Low=off
MPP33       0x0 FAN_150             output      Default Low, Low=off
MPP34       0x0 FAN_100             output      Default Hi, Low=off
MPP35       0x0 FAN SENSE           input       Hi=Fail, pulse=running
MPP[36:44]  0x4 I2S
MPP[45:49]  0x0 GPIO
*/
static void 
KW_6281_109_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	/* 109, 110j, 210j, 209, 210j */
	SYNO_KW_GENERIC_GPIO gpio_109 = {
		.ext_sata_led = {
							.hdd1_led_0 = GPIO_UNDEF,
							.hdd1_led_1 = GPIO_UNDEF,
							.hdd2_led_0 = GPIO_UNDEF,
							.hdd2_led_1 = GPIO_UNDEF,
							.hdd3_led_0 = GPIO_UNDEF,
							.hdd3_led_1 = GPIO_UNDEF,
							.hdd4_led_0 = GPIO_UNDEF,
							.hdd4_led_1 = GPIO_UNDEF,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led = {
							.hdd2_fail_led = 22,
							.hdd1_fail_led = 23,
						},
		.model		  = {
							.model_id_0 = GPIO_UNDEF,
							.model_id_1 = GPIO_UNDEF,
							.model_id_2 = GPIO_UNDEF,
							.model_id_3 = GPIO_UNDEF,
						},
		.fan		  = {
							.fan_1 = 32,
							.fan_2 = 33,
							.fan_3 = 34,
							.fan_fail = 35,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = 31,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = GPIO_UNDEF,
							.buzzer_mute_ack = GPIO_UNDEF,
							.rps1_on = GPIO_UNDEF,
							.rps2_on = GPIO_UNDEF,
						},
		.multi_bay	  = {
							.inter_lock = GPIO_UNDEF,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_109;
}

static void 
KW_6281_212j_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	KW_6281_109_GPIO_init(global_gpio);
	global_gpio->hdd_pm.hdd1_pm = 29;
}


/*
Pin             Mode    Signal definition       Input/output    Pull-up/pull-down
MPP[0:3]        0x2     SPI signal
MPP[4:7]        0x0     GPIO
MPP[8:9]        0x1     TWSI
MPP[10:11]      0x3     UART0
MPP12           0x0     GPIO
MPP[13:14]      0x3     UART1
MPP[15:19]      0x0     GPIO
MPP[20:21]      0x0     DISKA0/A1               output          00=off (default), 01=Green,10=Amber,
MPP[22:23]      0x0     DISKB0/B1               output
MPP[24:25]      0x0     DISKC0/C1               output
MPP[26:27]      0x0     DISKD0/D1               output
MPP[28:30]      0x0     GPIO
MPP31           0x0     BP-LOCK-OUT             input           Low=good, Hi=NG
MPP32           0x0     FAN_150 (409slim)       output          Hi=ON, Low=off
MPP33           0x0     FAN_120                 output          Default low, Low=off
MPP34           0x0     FAN_100                 output          Default low, Low=off
MPP35           0x0     FAN SENSE               input           Hi=Fail, pulse=running
MPP[36:44]      0x4     I2S
MPP[45:49]      0x0     GPIO
*/
static void 
KW_6281_409slim_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	SYNO_KW_GENERIC_GPIO gpio_slim = {
		.ext_sata_led = {
							.hdd1_led_0 = 20,
							.hdd1_led_1 = 21,
							.hdd2_led_0 = 22,
							.hdd2_led_1 = 23,
							.hdd3_led_0 = 24,
							.hdd3_led_1 = 25,
							.hdd4_led_0 = 26,
							.hdd4_led_1 = 27,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
						},
		.model		  = {
							.model_id_0 = GPIO_UNDEF,
							.model_id_1 = GPIO_UNDEF,
							.model_id_2 = GPIO_UNDEF,
							.model_id_3 = GPIO_UNDEF,
						},
		.fan		  = {
							.fan_1 = 32,
							.fan_2 = 33,
							.fan_3 = 34,
							.fan_fail = 35,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = GPIO_UNDEF,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = GPIO_UNDEF,
							.buzzer_mute_ack = GPIO_UNDEF,
							.rps1_on = GPIO_UNDEF,
							.rps2_on = GPIO_UNDEF,
						},
		.multi_bay	  = {
							.inter_lock = 31,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_slim;
}

/*
Pin         Mode    Signal definition   Input/output    Pull-up/pull-down
MPP[0:3]    0x2     SPI signal
MPP[4:7]    0x0     GPIO
MPP[8:9]    0x1     TWSI
MPP[10:11]  0x3     UART0
MPP12       0x0     ALERM_LED           output
MPP[13:14]  0x3     UART1
MPP[15:17]  0x0     FAN 150, FAN 120, FAN 100   output
MPP[18]     0x0     FAN sense           input
MPP[19]     0x0     INTER-LOCK          input
MPP[20:23]  0x0     RGMII G2 TX[0:3]    output
MPP[24:27]  0x0     RGMII G2 RX[0:3]    input
MPP[28:29]  0x0     MODEL_ID            input           MODEL_ID:
														00: DS409, DS410j
														01: DS509
														10:RS409
														11:RS409RP
MPP[30:31]  0x0     RGMII TX_CTL, RGMII TX_CLK  output
MPP[32:33]  0x0     RGMII RX_CTL, RGMII RX_CLK  input
MPP34       0x3     ACT internal SATA port 2    output
MPP35       0x3     ACT internal SATA port 1    output
MPP[36:45]  0x0     DISK LED STATE      output
MPP47       0x0     BUZZER_MUTE_REQ     input
MPP46       0x0     BUZZER_MUTE_ACK     output
MPP48       0x0     RPS1_ON             input
MPP49       0x0     RPS2_ON             input
*/
static void 
KW_6281_409_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	SYNO_KW_GENERIC_GPIO gpio_409 = {
		.ext_sata_led = {
							.hdd1_led_0 = 36,
							.hdd1_led_1 = 37,
							.hdd2_led_0 = 38,
							.hdd2_led_1 = 39,
							.hdd3_led_0 = 40,
							.hdd3_led_1 = 41,
							.hdd4_led_0 = 42,
							.hdd4_led_1 = 43,
							.hdd5_led_0 = 44,
							.hdd5_led_1 = 45,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
						},
		.model		  = {
							.model_id_0 = 28,
							.model_id_1 = 29,
							.model_id_2 = GPIO_UNDEF,
							.model_id_3 = GPIO_UNDEF,
						},
		.fan		  = {
							.fan_1 = 15,
							.fan_2 = 16,
							.fan_3 = 17,
							.fan_fail = 18,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = GPIO_UNDEF,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = 47,
							.buzzer_mute_ack = 46,
							.rps1_on = 48,
							.rps2_on = 49,
						},
		.multi_bay	  = {
							.inter_lock = 19,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = 12,
						},
	};

	*global_gpio = gpio_409;
}

/*
 * Pin          Mode    Signal select and definition    Input/output    Pull-up/pull-down
 * MPP[0:3]     0x2     SPI signal
 * MPP[4]       0x0     GPIO
 * MPP[5]       0x0     GPO                             output
 * MPP[6]       0x1     SYSRST_OUTn                     output
 * MPP[7]       0x0     GPO                             output
 * MPP[8:9]     0x1     TWSI
 * MPP[10:11]   0x3     UART0
 * MPP[12]      0x0     GPIO
 * MPP[13:14]   0x3     UART1
 * MPP[15:17]   0x0     GPIO
 * MPP[18:19]   0x0     GPO                             output
 * MPP[35]      0x0     GPIO
 * MPP[36]      0x0     USB LED                         output      Default Hi, Low=USB CONNECT
 * MPP[37]      0x0     STATUS LED                      output      Hi=power on, Low=power off
 * MPP[38]      0x0     RESET BUTTON                    input       Active Low
 * MPP[39]      0x0     POWER BUTTON                    input       Active Low
 * MPP[40:44]   0x0     GPIO
 */
static void
KW_6180_011_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
    SYNO_KW_GENERIC_GPIO gpio_011 = {
        .ext_sata_led = {
                            .hdd1_led_0 = 40,
                            .hdd1_led_1 = 36,
                            .hdd2_led_0 = GPIO_UNDEF,
                            .hdd2_led_1 = GPIO_UNDEF,
                            .hdd3_led_0 = GPIO_UNDEF,
                            .hdd3_led_1 = GPIO_UNDEF,
                            .hdd4_led_0 = GPIO_UNDEF,
                            .hdd4_led_1 = GPIO_UNDEF,
                            .hdd5_led_0 = GPIO_UNDEF,
                            .hdd5_led_1 = GPIO_UNDEF,
                        },
        .soc_sata_led = {
                            .hdd2_fail_led = GPIO_UNDEF,
                            .hdd1_fail_led = GPIO_UNDEF,
                        },
        .model        = {
                            .model_id_0 = GPIO_UNDEF,
                            .model_id_1 = GPIO_UNDEF,
                            .model_id_2 = GPIO_UNDEF,
                            .model_id_3 = GPIO_UNDEF,
                        },
        .fan          = {
                            .fan_1 = GPIO_UNDEF,
                            .fan_2 = GPIO_UNDEF,
                            .fan_3 = GPIO_UNDEF,
                            .fan_fail = GPIO_UNDEF,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
                        },
        .hdd_pm       = {
                            .hdd1_pm = GPIO_UNDEF,
                            .hdd2_pm = GPIO_UNDEF,
                            .hdd3_pm = GPIO_UNDEF,
                            .hdd4_pm = GPIO_UNDEF,
                        },
        .rack         = {
                            .buzzer_mute_req = GPIO_UNDEF,
                            .buzzer_mute_ack = GPIO_UNDEF,
                            .rps1_on = GPIO_UNDEF,
                            .rps2_on = GPIO_UNDEF,
                        },
        .multi_bay    = {
                            .inter_lock = GPIO_UNDEF,
                        },
        .status       = {
                            .power_led = 37,
                            .alarm_led = GPIO_UNDEF,
                        },
    };

    *global_gpio = gpio_011;
}

/*
 *   Marvell 88f6702 1 BAY, used in DS112j, Eagle
 *
 *  Pin         Mode    Signal select and definition    Input/output    Pull-up/pull-down
 *  MPP[0:3]    0x2         SPI signal                      out             x000
 *  MPP[4]      0x2         UART0 RXD                       in
 *  MPP[5]      0x2         UART0 TXD                       out             1
 *  MPP[6]      0x1         SYSRST_OUTn                     out
 *  MPP[7]      0x0         Fan speed low                   out             1
 *  MPP[8]      0x1         TW_SDA                          in/out
 *  MPP[9]      0x1         TW_SCK                          in/out
 *  MPP[10]     0x5         SATA1_LEDn                      out             1
 *  MPP[11]     0x5         SATA0 LEDn                      out
 *  MPP[12]     0x0         HDD_PWR_EN_1                    out             1
 *  MPP[13]     0x0         SATA port 1 FAULTn LED          out
 *  MPP[14]     0x3         Reserved
 *  MPP[15]     0x3         UART0 TXD                       out
 *  MPP[16]     0x3         UART0 RXD                       in
 *  MPP[17]     0x0         SATA port 0 FAULTn LED          out
 *  MPP[18]     0x0         Fan speed middle                out             1
 *  MPP[19]     0x0         Fan speed high                  out             1
 *  MPP[20:27]  0x0         Reserved
 *  MPP[28]     0x0         Model ID                        in
 */

static void 
KW_6702_1BAY_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	/* 112j */
	SYNO_KW_GENERIC_GPIO gpio_6702_1bay = {
		.ext_sata_led = {
							.hdd1_led_0 = GPIO_UNDEF,
							.hdd1_led_1 = GPIO_UNDEF,
							.hdd2_led_0 = GPIO_UNDEF,
							.hdd2_led_1 = GPIO_UNDEF,
							.hdd3_led_0 = GPIO_UNDEF,
							.hdd3_led_1 = GPIO_UNDEF,
							.hdd4_led_0 = GPIO_UNDEF,
							.hdd4_led_1 = GPIO_UNDEF,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led = {
							.hdd2_fail_led = 13,
							.hdd1_fail_led = 17,
						},
		.model		  = {
							.model_id_0 = 28,
							.model_id_1 = GPIO_UNDEF,
							.model_id_2 = GPIO_UNDEF,
							.model_id_3 = GPIO_UNDEF,
						},
		.fan		  = {
							.fan_1 = 7,
							.fan_2 = 18,
							.fan_3 = 19,
							.fan_fail = 35,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = 12,
							.hdd2_pm = GPIO_UNDEF,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = GPIO_UNDEF,
							.buzzer_mute_ack = GPIO_UNDEF,
							.rps1_on = GPIO_UNDEF,
							.rps2_on = GPIO_UNDEF,
						},
		.multi_bay	  = {
							.inter_lock = GPIO_UNDEF,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_6702_1bay;
}

static void
KW_default_GPIO_init(SYNO_KW_GENERIC_GPIO *global_gpio)
{
	SYNO_KW_GENERIC_GPIO gpio_default = {
		.ext_sata_led = {
							.hdd1_led_0 = GPIO_UNDEF,
							.hdd1_led_1 = GPIO_UNDEF,
							.hdd2_led_0 = GPIO_UNDEF,
							.hdd2_led_1 = GPIO_UNDEF,
							.hdd3_led_0 = GPIO_UNDEF,
							.hdd3_led_1 = GPIO_UNDEF,
							.hdd4_led_0 = GPIO_UNDEF,
							.hdd4_led_1 = GPIO_UNDEF,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
						},
		.model		  = {
							.model_id_0 = GPIO_UNDEF,
							.model_id_1 = GPIO_UNDEF,
							.model_id_2 = GPIO_UNDEF,
							.model_id_3 = GPIO_UNDEF,
						},
		.fan		  = {
							.fan_1 = GPIO_UNDEF,
							.fan_2 = GPIO_UNDEF,
							.fan_3 = GPIO_UNDEF,
							.fan_fail = GPIO_UNDEF,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = GPIO_UNDEF,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = GPIO_UNDEF,
							.buzzer_mute_ack = GPIO_UNDEF,
							.rps1_on = GPIO_UNDEF,
							.rps2_on = GPIO_UNDEF,
						},
		.multi_bay	  = {
							.inter_lock = GPIO_UNDEF,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_default;
}

void synology_gpio_init(void)
{
	MV_U32 boardId = mvBoardIdGet();

	switch(boardId) {
	case SYNO_DS109_ID:
		printk("Synology 6281 1, 2 bay GPIO Init\n");
		if (0 == strncmp(HW_DS212jv10, gszSynoHWVersion, strlen(HW_DS212jv10)) || 0 == strncmp(HW_DS212jv20, gszSynoHWVersion, strlen(HW_DS212jv20))) {
			KW_6281_212j_GPIO_init(&generic_gpio);
		} else {
			KW_6281_109_GPIO_init(&generic_gpio);
		}
		break;
	case SYNO_DS211_ID:
		KW_6282_211_GPIO_init(&generic_gpio);
		printk("Synology 6282 1, 2 bay GPIO Init\n");
		break;
	case SYNO_DS411slim_ID:
		KW_6282_DS_4BAY_GPIO_init(&generic_gpio);
		printk("Synology 6282 DS411slim GPIO Init\n");
		break;
	case SYNO_RS_6282_ID:
		printk("Synology 6282 RS411 / RS812 GPIO Init\n");
		KW_6282_RS411_GPIO_init(&generic_gpio);
		break;
	case SYNO_DS411_ID:
		KW_6282_DS_4BAY_GPIO_init(&generic_gpio);
		printk("Synology 6282 DS411 GPIO Init\n");
		break;
	case SYNO_DS409slim_ID:
		printk("Synology 6281 slim GPIO Init\n");
		KW_6281_409slim_GPIO_init(&generic_gpio);
		break;
	case SYNO_DS409_ID:
		printk("Synology 6281 4 bay GPIO Init\n");
		KW_6281_409_GPIO_init(&generic_gpio);
		break;
	case SYNO_DS011_ID:
		printk("Synology 6180 GPIO Init\n");
		KW_6180_011_GPIO_init(&generic_gpio);
		break;
	case SYNO_DS212_ID:
		if (0 == strncmp(HW_DS112, gszSynoHWVersion, strlen(HW_DS112))) {
			KW_6282_112_GPIO_init(&generic_gpio);
			printk("Synology 6282 DS112 GPIO Init\n");
		} else if (0 == strncmp(HW_DS112pv10, gszSynoHWVersion, strlen(HW_DS112pv10))) {
			KW_6282_112p_GPIO_init(&generic_gpio);
			printk("Synology 6282 DS112+ GPIO Init\n");
		} else {
			KW_6282_211_GPIO_init(&generic_gpio);
			printk("Synology 6282 1, 2 bay GPIO Init\n");
		}
		break;
	case SYNO_6702_1BAY_ID: //ds112j
		KW_6702_1BAY_GPIO_init(&generic_gpio);
		printk("Synology 6702 1 bay GPIO Init\n");
		break;
	default:
		printk("%s BoardID not match\n", __FUNCTION__);
		KW_default_GPIO_init(&generic_gpio);
		break;
	}
}
#endif /* CONFIG_SYNO_MV88F6281 */
