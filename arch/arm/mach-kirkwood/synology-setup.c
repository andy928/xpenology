/*
 * Synology Kirkwood NAS Board Setup
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

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mtd/physmap.h>
#include <linux/spi/flash.h>
#include <linux/spi/spi.h>
#include <linux/spi/orion_spi.h>
#include <linux/mv643xx_eth.h>
#include <linux/ata_platform.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/timex.h>
#include <linux/serial_reg.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <mach/kirkwood.h>
#include <mach/synology.h>
#include "common.h"
#include "mpp.h"

/****************************************************************************
 * Layout as used by :
 *  0x00000000-0x00080000 : "U-Boot"
 *  0x00080000-0x00280000 : "Kernel"
 *  0x00280000-0x003c0000 : "RootFS"
 *  0x003c0000-0x003d0000 : "Vendor"
 *  0x003d0000-0x003f0000 : "U-Boot Config"
 *  0x003f0000-0x00400000 : "FIS directory"
 *
 ***************************************************************************/
static struct mtd_partition syno_6281_partitions[] = {
	{
		.name   = "RedBoot",		/* u-boot		*/
		.offset = 0x00000000,
		.size   = 0x00080000,		/* 512KB		*/
	},
	{
		.name   = "zImage",			/* linux kernel image	*/
		.offset = 0x00080000,
		.size   = 0x00200000,		/* 2 MB			*/
	},
	{
		.name   = "rd.gz",			/* ramdisk image	*/
		.offset = 0x00280000,
		.size   = 0x00140000,		/* 1.2 MB		*/
	},
	{
		.name   = "vendor",			/* vendor specific data */
		.offset = 0x003C0000,
		.size   = 0x00010000,		/* 64KB                 */
	},
	{
		.name   = "RedBoot Config",	/* configs for u-boot   */
		.offset = 0x003D0000,
		.size   = 0x00020000,		/* 128KB                */
	},
	{
		.name   = "FIS directory",	/* flash partition table*/
		.offset = 0x003F0000,
		.size   = 0x00010000,		/* 64KB                 */
	},
};

/* Not specify type ley it autoprobe the spi flash moduel */
static const struct flash_platform_data synology_6281_flash = {
	.name		= "spi_flash",
	.parts		= syno_6281_partitions,
	.nr_parts	= ARRAY_SIZE(syno_6281_partitions),
};

static struct spi_board_info __initdata synology_spi_slave_info[] = {
	{
		.modalias	= "m25p80",
		.platform_data	= &synology_6281_flash,
		.irq		= -1,
		.max_speed_hz	= 20000000,
		.bus_num	= 0,
		.chip_select	= 0,
	},
};

static struct mv643xx_eth_platform_data synology_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR(8),
};

static struct mv643xx_eth_platform_data synology_ge01_data = {
	.phy_addr   = MV643XX_ETH_PHY_ADDR(9),
};

static struct mv_sata_platform_data synology_sata_data = {
	.n_ports	= 2,
};

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
static unsigned int synology_109_mpp_config[] __initdata = {
	MPP0_SPI_SCn,		MPP1_SPI_MOSI,		MPP2_SPI_SCK,				MPP3_SPI_MISO,
	MPP4_GPIO,			MPP5_GPO,			MPP6_GPIO,					MPP7_GPO,
	MPP8_TW_SDA,		MPP9_TW_SCK,		MPP10_UART0_TXD,			MPP11_UART0_RXD,
	MPP12_GPO,			MPP13_UART1_TXD,	MPP14_UART1_RXD,			MPP15_GPIO,
	MPP16_GPIO,			MPP17_GPIO,			MPP18_GPO,					MPP19_GPO,
	MPP20_SATA1_ACTn,	MPP21_SATA0_ACTn,	MPP22_GPIO,					MPP23_GPIO,
	MPP24_GPIO,			MPP25_GPIO,			MPP26_GPIO,					MPP27_GPIO,
	MPP28_GPIO,			MPP29_GPIO,			MPP30_GPIO,					MPP31_GPIO,
	MPP32_GPIO,			MPP33_GPIO,			MPP34_GPIO,					MPP35_GPIO,
	MPP36_AUDIO_SPDIFI,	MPP37_AUDIO_SPDIFO,	MPP38_AUDIO_SPDIFRMLCLK,	MPP39_AUDIO_I2SBCLK,
	MPP40_AUDIO_I2SDO,	MPP41_AUDIO_I2SLRC,	MPP42_AUDIO_I2SMCLK,		MPP43_AUDIO_I2SDI,
	MPP44_AUDIO_EXTCLK,	MPP45_GPIO,			MPP46_GPIO,					MPP47_GPIO,
	MPP48_GPIO,			MPP49_GPIO,			0
};

static SYNO_109_GPIO gpio_109 = {
	.hdd2_fail_led = 22,
	.hdd1_fail_led = 23,
	.hdd2_power = 31,
	.fan_1 = 32,
	.fan_2 = 33,
	.fan_3 = 34,
	.fan1_fail = 35,
};

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
static unsigned int synology_409slim_mpp_config[] __initdata = {
	MPP0_SPI_SCn,		MPP1_SPI_MOSI,		MPP2_SPI_SCK,				MPP3_SPI_MISO,
	MPP4_GPIO,			MPP5_GPO,			MPP6_GPIO,					MPP7_GPO,
	MPP8_TW_SDA,		MPP9_TW_SCK,		MPP10_UART0_TXD,			MPP11_UART0_RXD,
	MPP12_GPO,			MPP13_UART1_TXD,	MPP14_UART1_RXD,			MPP15_GPIO,
	MPP16_GPIO,			MPP17_GPIO,			MPP18_GPO,					MPP19_GPO,
	MPP20_GPIO,			MPP21_GPIO,			MPP22_GPIO,					MPP23_GPIO,
	MPP24_GPIO,			MPP25_GPIO,			MPP26_GPIO,					MPP27_GPIO,
	MPP28_GPIO,			MPP29_GPIO,			MPP30_GPIO,					MPP31_GPIO,
	MPP32_GPIO,			MPP33_GPIO,			MPP34_GPIO,					MPP35_GPIO,
	MPP36_AUDIO_SPDIFI,	MPP37_AUDIO_SPDIFO,	MPP38_AUDIO_SPDIFRMLCLK,	MPP39_AUDIO_I2SBCLK,
	MPP40_AUDIO_I2SDO,	MPP41_AUDIO_I2SLRC,	MPP42_AUDIO_I2SMCLK,		MPP43_AUDIO_I2SDI,
	MPP44_AUDIO_EXTCLK,	MPP45_GPIO,			MPP46_GPIO,					MPP47_GPIO,
	MPP48_GPIO,			MPP49_GPIO,			0
};

static SYNO_409slim_GPIO gpio_slim = {
	.hdd1_led_0 = 20,
	.hdd1_led_1 = 21,
	.hdd2_led_0 = 22,
	.hdd2_led_1 = 23,
	.hdd3_led_0 = 24,
	.hdd3_led_1 = 25,
	.hdd4_led_0 = 26,
	.hdd4_led_1 = 27,
	.bp_lock_out = 31,
	.fan_1 = 32,
	.fan_2 = 33,
	.fan_3 = 34,
	.fan1_fail = 35,
};

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
static unsigned int synology_4bay_mpp_config[] __initdata = {
	MPP0_SPI_SCn,		MPP1_SPI_MOSI,		MPP2_SPI_SCK,				MPP3_SPI_MISO,
	MPP4_GPIO,			MPP5_GPO,			MPP6_GPIO,					MPP7_GPO,
	MPP8_TW_SDA,		MPP9_TW_SCK,		MPP10_UART0_TXD,			MPP11_UART0_RXD,
	MPP12_GPO,			MPP13_UART1_TXD,	MPP14_UART1_RXD,			MPP15_GPIO,
	MPP16_GPIO,			MPP17_GPIO,			MPP18_GPO,					MPP19_GPO,
	MPP20_GE1_0,		MPP21_GE1_1,		MPP22_GE1_2,				MPP23_GE1_3,
	MPP24_GE1_4,		MPP25_GE1_5,		MPP26_GE1_6,				MPP27_GE1_7,
	MPP28_GPIO,			MPP29_GPIO,			MPP30_GE1_10,				MPP31_GE1_11,
	MPP32_GE1_12,		MPP33_GE1_13,		MPP34_SATA1_ACTn,			MPP35_SATA0_ACTn,
	MPP36_GPIO,			MPP37_GPIO,			MPP38_GPIO,					MPP39_GPIO,
	MPP40_GPIO,			MPP41_GPIO,			MPP42_GPIO,					MPP43_GPIO,
	MPP44_GPIO,			MPP45_GPIO,			MPP46_GPIO,					MPP47_GPIO,
	MPP48_GPIO,			MPP49_GPIO,			0
};

static SYNO_409_GPIO gpio_409 = {
	.alarm_led = 12,
	.fan_1 = 15,
	.fan_2 = 16,
	.fan_3 = 17,
	.fan_sense = 18,
	.inter_lock = 19,
	.model_id_0 = 28,
	.model_id_1 = 29,
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
	.buzzer_mute_req = 47,
	.buzzer_mute_ack = 46,
	.rps1_on = 48,
	.rps2_on = 49,
};

static u32 gBoardId;
static SYNO_6281_FAN_GPIO fan_gpio;
static SYNO_6281_HDD_LED_GPIO hdd_led_gpio;

static int __init parse_tag_mv_uboot(const struct tag *tag)
{
	gBoardId = (tag->u.mv_uboot.uboot_version & 0xff);
	return 0;
}
__tagtable(ATAG_MV_UBOOT, parse_tag_mv_uboot);

/*****************************************************************************
 * Syonology specific power off method via UART1-attached PIC
 ****************************************************************************/

#define UART1_REG(x)	(UART1_VIRT_BASE + ((UART_##x) << 2))
#define SET8N1                  0x3
#define SOFTWARE_SHUTDOWN       0x31
#define SOFTWARE_REBOOT         0x43
static void synology_power_off(void)
{
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_SHUTDOWN, UART1_REG(TX));
}

static void synology_restart(char mode, const char *cmd)
{
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_REBOOT, UART1_REG(TX));
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

	writel(SATAHC_LED_ACT_PRESENT, SATAHC_LED_CONFIG_REG);

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
			gpio_set_value(gpio_109.hdd1_fail_led, fail_led);
			break;
		case 2:
			gpio_set_value(gpio_109.hdd2_fail_led, fail_led);
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
		pin1 = hdd_led_gpio.hdd1_led_0;
		pin2 = hdd_led_gpio.hdd1_led_1;
		break;
	case 2:
		pin1 = hdd_led_gpio.hdd2_led_0;
		pin2 = hdd_led_gpio.hdd2_led_1;
		break;
	case 3:
		pin1 = hdd_led_gpio.hdd3_led_0;
		pin2 = hdd_led_gpio.hdd3_led_1;
		break;
	case 4:
		pin1 = hdd_led_gpio.hdd4_led_0;
		pin2 = hdd_led_gpio.hdd4_led_1;
		break;
	case 5:
		if (SYNO_DS409slim_ID == gBoardId) {
			//for esata
			ret = 0;
			goto END;
		}
		pin1 = hdd_led_gpio.hdd5_led_0;
		pin2 = hdd_led_gpio.hdd5_led_1;
		break;
	case 6:
		//for esata
		ret = 0;
		goto END;
	default:
		printk("Wrong HDD number [%d]\n", index);
		goto END;
	}

	gpio_set_value(pin1, bit1);
	gpio_set_value(pin2, bit2);

    ret = 0;
END:
    return ret;
}

int SYNO_CTRL_HDD_POWERON(int index, int value)
{
	int ret = -1;

	switch (index) {
	case 2:
		gpio_set_value(gpio_109.hdd2_power, value);
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

	switch (index) {
	case 1:
		gpio_set_value(fan_gpio.fan_1, status);
		break;
	case 2:
		gpio_set_value(fan_gpio.fan_2, status);
		break;
	case 3:
		gpio_set_value(fan_gpio.fan_3, status);
		break;
	default:
		ret = -1;
		printk("%s BoardID not match\n", __FUNCTION__);
		break;
	}

	return ret;
}

int SYNO_CTRL_FAN_STATUS_GET(int index, int *pValue)
{
	int ret = 0;

	*pValue = gpio_get_value(fan_gpio.fan_fail);

	if(*pValue)
		*pValue = 0;
	else
		*pValue = 1;

	return ret;
}

int SYNO_CTRL_ALARM_LED_SET(int status)
{
	gpio_set_value(gpio_409.alarm_led, status);
	return 0;
}

int SYNO_CTRL_BACKPLANE_STATUS_GET(int *pStatus)
{
	*pStatus = gpio_get_value(gpio_409.inter_lock);
	return 0;
}

int SYNO_CTRL_BUZZER_CLEARED_GET(int *pValue)
{
	int tempVal = 0;

	tempVal = gpio_get_value(gpio_409.buzzer_mute_req);
	if ( tempVal ) {
		*pValue = 0;
	} else {
		*pValue = 1;
		tempVal = 1;
		gpio_set_value(gpio_409.buzzer_mute_req, tempVal);
	}

	return 0;
}

u8 SYNOKirkwoodIsBoardNeedPowerUpHDD(u32 disk_id) {
	u8 ret = 0;

	switch(gBoardId) {
	case SYNO_DS109_ID:
		if (1 == disk_id)
		ret = 1;
		break;
	default:
		break;
	}

	return ret;
}

EXPORT_SYMBOL(SYNOKirkwoodIsBoardNeedPowerUpHDD);
EXPORT_SYMBOL(SYNO_MV6281_GPIO_PIN);
EXPORT_SYMBOL(SYNO_CTRL_INTERNAL_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_EXT_CHIP_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_HDD_POWERON);
EXPORT_SYMBOL(SYNO_CTRL_FAN_PERSISTER);
EXPORT_SYMBOL(SYNO_CTRL_FAN_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_ALARM_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_BACKPLANE_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_BUZZER_CLEARED_GET);

static void __init synology_init(void)
{
	kirkwood_init();

	kirkwood_spi_init();
	spi_register_board_info(synology_spi_slave_info,
							ARRAY_SIZE(synology_spi_slave_info));
	kirkwood_i2c_init();
	kirkwood_ehci_init();
	kirkwood_ge00_init(&synology_ge00_data);
	kirkwood_sata_init(&synology_sata_data);
	kirkwood_uart0_init();
	kirkwood_uart1_init();
	
	switch(gBoardId) {
	case SYNO_DS109_ID:
		printk("Synology 1,2 bay NAS\n");
		kirkwood_mpp_conf(synology_109_mpp_config);

		fan_gpio.fan_1 = gpio_109.fan_1;
		fan_gpio.fan_2 = gpio_109.fan_2;
		fan_gpio.fan_3 = gpio_109.fan_3;
		fan_gpio.fan_fail = gpio_109.fan1_fail;
		break;
	case SYNO_DS409slim_ID:
		printk("Synology 409slim\n");
		kirkwood_mpp_conf(synology_409slim_mpp_config);
		fan_gpio.fan_1 = gpio_slim.fan_1;
		fan_gpio.fan_2 = gpio_slim.fan_2;
		fan_gpio.fan_3 = gpio_slim.fan_3;
		fan_gpio.fan_fail = gpio_slim.fan1_fail;

		hdd_led_gpio.hdd1_led_0 = gpio_slim.hdd1_led_0;
		hdd_led_gpio.hdd1_led_1 = gpio_slim.hdd1_led_1;
		hdd_led_gpio.hdd2_led_0 = gpio_slim.hdd2_led_0;
		hdd_led_gpio.hdd2_led_1 = gpio_slim.hdd2_led_1;
		hdd_led_gpio.hdd3_led_0 = gpio_slim.hdd3_led_0;
		hdd_led_gpio.hdd3_led_1 = gpio_slim.hdd3_led_1;
		hdd_led_gpio.hdd4_led_0 = gpio_slim.hdd4_led_0;
		hdd_led_gpio.hdd4_led_1 = gpio_slim.hdd4_led_1;
		break;
	case SYNO_DS409_ID:
		printk("Synology 4 bay NAS\n");
		kirkwood_mpp_conf(synology_4bay_mpp_config);
		fan_gpio.fan_1 = gpio_409.fan_1;
		fan_gpio.fan_2 = gpio_409.fan_2;
		fan_gpio.fan_3 = gpio_409.fan_3;
		fan_gpio.fan_fail = gpio_409.fan_sense;

		hdd_led_gpio.hdd1_led_0 = gpio_409.hdd1_led_0;
		hdd_led_gpio.hdd1_led_1 = gpio_409.hdd1_led_1;
		hdd_led_gpio.hdd2_led_0 = gpio_409.hdd2_led_0;
		hdd_led_gpio.hdd2_led_1 = gpio_409.hdd2_led_1;
		hdd_led_gpio.hdd3_led_0 = gpio_409.hdd3_led_0;
		hdd_led_gpio.hdd3_led_1 = gpio_409.hdd3_led_1;
		hdd_led_gpio.hdd4_led_0 = gpio_409.hdd4_led_0;
		hdd_led_gpio.hdd4_led_1 = gpio_409.hdd4_led_1;
		hdd_led_gpio.hdd5_led_0 = gpio_409.hdd5_led_0;
		hdd_led_gpio.hdd5_led_1 = gpio_409.hdd5_led_1;

		/* 01: 509, 10 11 rs409(rp) are dual lan models. Need init second NIC */
		if (gpio_get_value(gpio_409.model_id_0) ||
		    gpio_get_value(gpio_409.model_id_1))
			kirkwood_ge01_init(&synology_ge01_data);
		break;
	default:
		printk("%s BoardID not match\n", __FUNCTION__);
		break;
	}

	pm_power_off = synology_power_off;
	arm_pm_restart = synology_restart;
}

static int __init synology_pci_init(void)
{
	u32 reg = readl(POWER_MNG_CTRL_REG);

	if (((reg & PMC_PEXSTOPCLOCK_MASK) != PMC_PEXSTOPCLOCK_STOP)) {
		if (!(readl(PCIE_STATUS) & 0x1)) {
			printk("PCIe link is enable, apply PCIe workaround \n");
			/*
			 * Synology 6281 PCIe link issue workaround.
			 * If we don't reset the PCIe link, the device's register will all be 0xffffffff
			 * after the PCIe device work for some times.
			 *
			 * In DS409slim, 7042 is work as PCIe 4X in 6281. Some board will encounter
			 * this problem. When the register are all 0xffffffff, the system is just like hang.
			 *
			 * Disable PCIe link and wait for that link is really down.
			 * And then enable it again.
			 * Please refer 6281 functional Spec page 447.
			 * PCIe Link control status register bit 4.
			 */
			writel(readl(PCIE_LINK_CTRL) | 0x10, PCIE_LINK_CTRL);

			while (1)
				if (readl(PCIE_STATUS) & 0x1)
					break;

			writel(readl(PCIE_LINK_CTRL) & ~0x10, PCIE_LINK_CTRL);

			while (1)
				if (!readl(PCIE_STATUS) & 0x1) {
					printk("PCIe link enable now\n");
					break;
				}
        }

		if (machine_is_synology()) {
			kirkwood_pcie_init();
		}
	}
	return 0;
}
subsys_initcall(synology_pci_init);

MACHINE_START(SYNOLOGY, "Synology board")
	.phys_io	= KIRKWOOD_REGS_PHYS_BASE,
	.io_pg_offst	= ((KIRKWOOD_REGS_VIRT_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.init_machine	= synology_init,
	.map_io		= kirkwood_map_io,
	.init_irq	= kirkwood_init_irq,
	.timer		= &kirkwood_timer,
MACHINE_END
