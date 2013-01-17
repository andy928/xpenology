/*
 *  linux/arch/arm/mach-dove/clcd.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/amba/bus.h>
#include <linux/amba/kmi.h>
#include <linux/mm.h>

//#include <asm/hardware.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <linux/param.h>		/* HZ */
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <video/dovefb.h>
#include <video/dovefbreg.h>
#ifdef CONFIG_FB_DOVE_DCON
#include <video/dovedcon.h>
#include <mach/dove_bl.h>
#endif
#include "gpp/mvGppRegs.h"
#include <ctrlEnv/mvCtrlEnvRegs.h>

unsigned int lcd0_enable;
module_param(lcd0_enable, uint, 0);
MODULE_PARM_DESC(lcd0_enable, "set to 1 to enable LCD0 output.");

#if defined(CONFIG_FB_DOVE_CLCD_FLAREON_GV) || \
    defined(CONFIG_FB_DOVE_CLCD_FLAREON_GV_MODULE)
	#ifndef CONFIG_ARCH_DOVENB_ON_TAHOE_AXI
	#define	LCD_BASE_PHY_ADDR		0x80400000
	#define	LCD1_BASE_PHY_ADDR		0x80410000
	#define	DCON_BASE_PHY_ADDR		0x80420000
	#else
	#define	LCD_BASE_PHY_ADDR		0xB0020000
	#define	LCD1_BASE_PHY_ADDR		0xB0010000
	#define	DCON_BASE_PHY_ADDR		0xB0030000
	#endif
#elif defined(CONFIG_ARCH_DOVE)
	#define	LCD_BASE_PHY_ADDR		(DOVE_LCD1_PHYS_BASE)
	#define	LCD1_BASE_PHY_ADDR		(DOVE_LCD2_PHYS_BASE)
	#define	DCON_BASE_PHY_ADDR		(DOVE_LCD_DCON_PHYS_BASE)
#elif defined(CONFIG_ARCH_FEROCEON_KW)
	#define	LCD_BASE_PHY_ADDR		(INTER_REGS_BASE | 0xC0000)
#else
	#ifdef CONFIG_ARCH_TAHOE_AXI
	#define LCD_BASE_PHY_ADDR		0x1C010000
	#define LCD1_BASE_PHY_ADDR		0x1C020000
	#else
	#define	LCD_BASE_PHY_ADDR		0x70000000
	#define	LCD1_BASE_PHY_ADDR		0x70010000
	#define	DCON_BASE_PHY_ADDR		0x70020000
	#define	IRE_BASE_PHY_ADDR		0x70021000

	#endif /* CONFIG_ARCH_TAHOE */
#endif

/*
 * LCD IRQ Line
 */
#ifndef IRQ_DOVE_LCD0
#define IRQ_DOVE_LCD1		10
#define IRQ_DOVE_LCD0		10
#define IRQ_DOVE_LCD_DCON	10
#endif

/*
 * Default mode database.
 */
static struct fb_videomode video_modes[] = {
	[0] = {			/* 640x480@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 640, 
	.yres		= 480,

	.right_margin	= 16,
	.hsync_len	= 96,
	.left_margin	= 48,

	.lower_margin	= 11,
	.vsync_len	= 2,
	.upper_margin	= 31,
	.sync		= 0,
	},
	[1] = {			/* 640x480@72 */
	.pixclock	= 0,
	.refresh	= 72,
	.xres		= 640, 
	.yres		= 480,

	.right_margin	= 24,
	.hsync_len	= 40,
	.left_margin	= 128,

	.lower_margin	= 9,
	.vsync_len	= 3,
	.upper_margin	= 28,
	.sync		= 0,
	},
	[2] = {			/* 640x480@75 */
	.pixclock	= 0,
	.refresh	= 75,
	.xres		= 640, 
	.yres		= 480,

	.right_margin	= 16,
	.hsync_len	= 96,
	.left_margin	= 48,

	.lower_margin	= 11,
	.vsync_len	= 2,
	.upper_margin	= 32,
	.sync		= 0,
	},
	[3] = {			/* 640x480@85 */
	.pixclock	= 0,
	.refresh	= 85,
	.xres		= 640, 
	.yres		= 480,

	.right_margin	= 32,
	.hsync_len	= 48,
	.left_margin	= 112,

	.lower_margin	= 1,
	.vsync_len	= 3,
	.upper_margin	= 25,
	.sync		= 0,
	},
	[4] = {			/* 800x600@56 */
	.pixclock	= 0,
	.refresh	= 56,
	.xres		= 800, 
	.yres		= 600,

	.right_margin	= 32,
	.hsync_len	= 128,
	.left_margin	= 128,

	.lower_margin	= 1,
	.vsync_len	= 4,
	.upper_margin	= 14,
	.sync		= 0,
	},
	[5] = {			/* 800x600@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 800,
	.yres		= 600,

	.right_margin	= 40,
	.hsync_len	= 128,
	.left_margin	= 88,

	.lower_margin	= 1,
	.vsync_len	= 4,
	.upper_margin	= 23,
	.sync		= 0,
	},
	[6] = {			/* 800x600@72 */
	.pixclock	= 0,
	.refresh	= 72,
	.xres		= 800,
	.yres		= 600,

	.right_margin	= 56,
	.hsync_len	= 120,
	.left_margin	= 64,

	.lower_margin	= 37,
	.vsync_len	= 6,
	.upper_margin	= 23,
	.sync		= 0,
	},
	[7] = {			/* 800x600@75 */
	.pixclock	= 0,
	.refresh	= 75,
	.xres		= 800,
	.yres		= 600,

	.right_margin	= 16,
	.hsync_len	= 80,
	.left_margin	= 160,

	.lower_margin	= 1,
	.vsync_len	= 2,
	.upper_margin	= 21,
	.sync		= 0,
	},
	[8] = {			/* 800x600@85 */
	.pixclock	= 0,
	.refresh	= 85,
	.xres		= 800,
	.yres		= 600,

	.right_margin	= 32,
	.hsync_len	= 64,
	.left_margin	= 152,

	.lower_margin	= 1,
	.vsync_len	= 3,
	.upper_margin	= 27,
	.sync		= 0,
	},
	[9] = {			/* 1024x600@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 1024,
	.yres		= 600,

	.right_margin	= 38,
	.hsync_len	= 100,
	.left_margin	= 38,
	.right_margin	= 38,

	.lower_margin	= 8,
	.vsync_len	= 4,
	.upper_margin	= 8,
	.sync		= 0,
	},
	[10] = {			/* 1024x768@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 1024,
	.yres		= 768,

	.right_margin	= 24,
	.hsync_len	= 136,
	.left_margin	= 160,

	.lower_margin	= 3,
	.vsync_len	= 6,
	.upper_margin	= 29,
	.sync		= 0,
	},
	[11] = {			/* 1024x768@70 */
	.pixclock	= 0,
	.refresh	= 70,
	.xres		= 1024,
	.yres		= 768,

	.right_margin	= 24,
	.hsync_len	= 136,
	.left_margin	= 144,

	.lower_margin	= 3,
	.vsync_len	= 6,
	.upper_margin	= 29,
	.sync		= 0,
	},
	[12] = {			/* 1024x768@75 */
	.pixclock	= 0,
	.refresh	= 75,
	.xres		= 1024,
	.yres		= 768,

	.right_margin	= 16,
	.hsync_len	= 96,
	.left_margin	= 176,

	.lower_margin	= 1,
	.vsync_len	= 3,
	.upper_margin	= 28,
	.sync		= 0,
	},
	[12] = {			/* 1024x768@85 */
	.pixclock	= 0,
	.refresh	= 85,
	.xres		= 1024,
	.yres		= 768,

	.right_margin	= 48,
	.hsync_len	= 96,
	.left_margin	= 208,

	.lower_margin	= 1,
	.vsync_len	= 3,
	.upper_margin	= 36,
	.sync		= 0,
	},
	[13] = {			/* 1280x720@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 1280, /* 1328 */
	.yres		= 720,  /* 816 */

	.hsync_len	= 40,
	.left_margin	= 220,
	.right_margin	= 110,

	.vsync_len	= 5,
	.upper_margin	= 20,
	.lower_margin	= 5,
	.sync		= 0,
	},
	[14] = {			/* 1280x1024@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 1280,
	.yres		= 1024,

	.right_margin	= 48,
	.hsync_len	= 112,
	.left_margin	= 248,

	.lower_margin	= 1,
	.vsync_len	= 3,
	.upper_margin	= 38,
	.sync		= 0,
	},
	[15] = {			/* 1366x768@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 1366,
	.yres		= 768,

	.right_margin	= 72,
	.hsync_len	= 144,
	.left_margin	= 216,

	.lower_margin	= 1,
	.vsync_len	= 3,
	.upper_margin	= 23,
	.sync		= 0,
	},
#if 1
	[16] = {			/* 1650x1050@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 1650,
	.yres		= 1050,

	.right_margin	= 104,
	.hsync_len	= 184,
	.left_margin	= 288,

	.lower_margin	= 1,
	.vsync_len	= 3,
	.upper_margin	= 33,
	.sync		= 0,
	},
	[17] = {			/* 1920x1080@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 1920,
	.yres		= 1080,

	.right_margin	= 88,
	.hsync_len	= 44,
	.left_margin	= 148,

	.lower_margin	= 4,
	.vsync_len	= 5,
	.upper_margin	= 36,
	.sync		= 0,
	},
	[18] = {			/* 1920x1200@60 */
	.pixclock	= 0,
	.refresh	= 60,
	.xres		= 1920,
	.yres		= 1200,

	.right_margin	= 128,
	.hsync_len	= 208,
	.left_margin	= 336,

	.lower_margin	= 1,
	.vsync_len	= 3,
	.upper_margin	= 38,
	.sync		= 0,
	},
#endif

};


#if defined(CONFIG_FB_DOVE_CLCD)

static struct resource lcd0_vid_res[] = {
	[0] = {
		.start	= LCD_BASE_PHY_ADDR,
		.end	= LCD_BASE_PHY_ADDR+0x10000,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DOVE_LCD0,
		.end	= IRQ_DOVE_LCD0,
		.flags	= IORESOURCE_IRQ,
	},
};


static struct resource lcd0_res[] = {
	[0] = {
		.start	= LCD_BASE_PHY_ADDR,
		.end	= LCD_BASE_PHY_ADDR+0x10000,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DOVE_LCD0,
		.end	= IRQ_DOVE_LCD0,
		.flags	= IORESOURCE_IRQ,
	},
};


static struct platform_device lcd0_platform_device = {
	.name = "dovefb",
	.id = 0,			/* lcd0 */
	.dev = {
		.coherent_dma_mask = ~0,
//		.platform_data = &lcd0_dmi,
	},
	.num_resources	= ARRAY_SIZE(lcd0_res),
	.resource	= lcd0_res,
};

static struct platform_device lcd0_vid_platform_device = {
	.name = "dovefb_ovly",
	.id = 0,			/* lcd0 */
	.dev = {
		.coherent_dma_mask = ~0,
//		.platform_data = &lcd0_vid_dmi,
	},
	.num_resources	= ARRAY_SIZE(lcd0_vid_res),
	.resource	= lcd0_vid_res,
};
#endif /* CONFIG_FB_DOVE_CLCD */

#ifdef CONFIG_BACKLIGHT_DOVE
static struct resource backlight_res[] = {
	[0] = {
		.start	= LCD_BASE_PHY_ADDR,
		.end	= LCD_BASE_PHY_ADDR+0x1C8,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device backlight_platform_device = {
	.name = "dove-bl",
	.id = 0,
	.num_resources	= ARRAY_SIZE(backlight_res),
	.resource	= backlight_res,
};

#endif /* CONFIG_FB_DOVE_DCON */

/*****************************************************************************
 * I2C buses - adc, hdmi
 ****************************************************************************/
static struct i2c_board_info __initdata i2c_adi9889[] = {
	{
		I2C_BOARD_INFO("adi9889_i2c", 0x3D),
	},
	{
		I2C_BOARD_INFO("adi9889_edid_i2c", 0x3F),
	},
};

static struct i2c_board_info __initdata i2c_ths8200[] = {
	{
		I2C_BOARD_INFO("ths8200_i2c", 0x21),
	},
};



int clcd_platform_init(struct dovefb_mach_info *lcd0_dmi_data,
		       struct dovefb_mach_info *lcd0_vid_dmi_data,
		       struct dovebl_platform_data *backlight_data)
{
	u32 total_x, total_y, i;
	u64 div_result;

	if (lcd0_enable != 1)
		return 0;
	for (i = 0; i < ARRAY_SIZE(video_modes); i++) {
		total_x = video_modes[i].xres + video_modes[i].hsync_len +
			video_modes[i].left_margin +
			video_modes[i].right_margin;
		total_y = video_modes[i].yres + video_modes[i].vsync_len +
			video_modes[i].upper_margin +
			video_modes[i].lower_margin;
		div_result = 1000000000000ll;
		do_div(div_result,
			(total_x * total_y * video_modes[i].refresh));
		video_modes[i].pixclock	= div_result;
	}

	/*
	 * Because DCON depends on lcd0 & lcd1 clk. Here we
	 * try to reorder the load sequence. Fix me when h/w
	 * changes.
	 */
#ifdef CONFIG_FB_DOVE_CLCD
	/* lcd0 */
	if (lcd0_enable && lcd0_dmi_data && lcd0_vid_dmi_data) {

		lcd0_vid_dmi_data->modes = video_modes;
		lcd0_vid_dmi_data->num_modes = ARRAY_SIZE(video_modes);
		lcd0_vid_platform_device.dev.platform_data = lcd0_vid_dmi_data;

		lcd0_dmi_data->modes = video_modes;
		lcd0_dmi_data->num_modes = ARRAY_SIZE(video_modes);
		lcd0_platform_device.dev.platform_data = lcd0_dmi_data;
		platform_device_register(&lcd0_vid_platform_device);
		platform_device_register(&lcd0_platform_device);
	} else {
		printk(KERN_INFO "LCD 0 disabled.\n");
	}
#endif

#ifdef CONFIG_BACKLIGHT_DOVE
	if (lcd0_enable) {
		backlight_platform_device.dev.platform_data = backlight_data;
		platform_device_register(&backlight_platform_device);
	}
#endif
	i2c_register_board_info(0, i2c_adi9889, ARRAY_SIZE(i2c_adi9889));
	i2c_register_board_info(0, i2c_ths8200, ARRAY_SIZE(i2c_ths8200));

	return 0;
}

