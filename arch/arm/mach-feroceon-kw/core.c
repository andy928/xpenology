/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <asm/mach/time.h>
#include <linux/clocksource.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <mach/system.h>

#include <linux/tty.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#ifdef MY_DEF_HERE
#include <linux/ata_platform.h>
#endif
#include <asm/serial.h>
#include <plat/cache-feroceon-l2.h>

#include <mach/serial.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "mvDebug.h"
#include "mvSysHwConfig.h"
#include "pex/mvPexRegs.h"
#include "cntmr/mvCntmr.h"
#include "gpp/mvGpp.h"
#include "plat/gpio.h"

#if defined(CONFIG_MV_INCLUDE_SDIO)
#include "ctrlEnv/sys/mvSysSdmmc.h"
#include <plat/mvsdio.h>
#endif
#if defined(CONFIG_MV_INCLUDE_CESA)
#include "cesa/mvCesa.h"
#endif
#if defined(CONFIG_MV_INCLUDE_AUDIO)
#include <plat/i2s-orion.h>
#endif

#include <plat/orion_wdt.h>

#ifdef CONFIG_FB_DOVE
#include <video/dovefb.h>
#include <video/dovefbreg.h>
#endif

/* I2C */
#include <linux/i2c.h>	
#include <linux/mv643xx_i2c.h>
#include "ctrlEnv/mvCtrlEnvSpec.h"

extern unsigned int irq_int_type[];

/* for debug putstr */
#include <mach/uncompress.h> 
static char arr[256];

#ifdef MV_INCLUDE_EARLY_PRINTK
extern void putstr(const char *ptr);
void mv_early_printk(char *fmt,...)
{
	va_list args;
	va_start(args, fmt);
	vsprintf(arr,fmt,args);
	va_end(args);
	putstr(arr);
}
#endif


extern void __init mv_map_io(void);
extern void __init mv_init_irq(void);
extern struct sys_timer mv_timer;
extern MV_CPU_DEC_WIN* mv_sys_map(void);
#if defined(CONFIG_MV_INCLUDE_CESA)
extern u32 mv_crypto_base_get(void);
#endif
unsigned int support_wait_for_interrupt = 0x1;

u32 mvTclk = 166666667;
u32 mvSysclk = 200000000;
u32 mvIsUsbHost = 1;


u8	mvMacAddr[CONFIG_MV_ETH_PORTS_NUM][6];
u16	mvMtu[CONFIG_MV_ETH_PORTS_NUM] = {0};
extern MV_U32 gBoardId; 
extern unsigned int elf_hwcap;

#ifdef CONFIG_MV_NAND
extern unsigned int mv_nand_ecc;
#endif
 
static int __init parse_tag_mv_uboot(const struct tag *tag)
{
    	unsigned int mvUbootVer = 0;
	int i = 0;
 
	mvUbootVer = tag->u.mv_uboot.uboot_version;
	mvIsUsbHost = tag->u.mv_uboot.isUsbHost;

        printk("Using UBoot passing parameters structure\n");
#ifdef CONFIG_SYNO_MV88F6281
	printk("Sys Clk = %d, Tclk = %d\n",mvSysclk ,mvTclk  );
#endif
  
	gBoardId =  (mvUbootVer & 0xff);
#ifdef CONFIG_SYNO_MV88F6281
	printk("Synology Board ID: %d\n",gBoardId);
#endif
	for (i = 0; i < CONFIG_MV_ETH_PORTS_NUM; i++) {
#if defined (CONFIG_OVERRIDE_ETH_CMDLINE)
		memset(mvMacAddr[i], 0, 6);
		mvMtu[i] = 0;
#else			
		memcpy(mvMacAddr[i], tag->u.mv_uboot.macAddr[i], 6);
		mvMtu[i] = tag->u.mv_uboot.mtu[i];
#endif    	
	}
#ifdef CONFIG_MV_NAND
               /* get NAND ECC type(1-bit or 4-bit) */
               if((mvUbootVer >> 8) >= 0x3040c)
                       mv_nand_ecc = tag->u.mv_uboot.nand_ecc;
               else
                       mv_nand_ecc = 1; /* fallback to 1-bit ECC */
#endif  
	return 0;
}
                                                                                                                             
__tagtable(ATAG_MV_UBOOT, parse_tag_mv_uboot);

#ifdef CONFIG_MV_INCLUDE_CESA
unsigned char*  mv_sram_usage_get(int* sram_size_ptr)
{
    int used_size = 0;

#if defined(CONFIG_MV_CESA)
    used_size = sizeof(MV_CESA_SRAM_MAP);
#endif

    if(sram_size_ptr != NULL)
        *sram_size_ptr = _8K - used_size;

    return (char *)(mv_crypto_base_get() + used_size);
}
#endif


void print_board_info(void)
{
    char name_buff[50];

#ifndef CONFIG_SYNO_MV88F6281
    printk("\n  Marvell Development Board (LSP Version %s)",LSP_VERSION);

    mvBoardNameGet(name_buff);
    printk("-- %s ",name_buff);

    mvCtrlModelRevNameGet(name_buff);
    printk(" Soc: %s",  name_buff);
#if defined(MV_CPU_LE)
	printk(" LE");
#else
	printk(" BE");
#endif
    printk("\n\n");
#endif
    printk(" Detected Tclk %d and SysClk %d \n",mvTclk, mvSysclk);
}

/*****************************************************************************
 * I2C(TWSI)
 ****************************************************************************/

/*Platform devices list*/

static struct mv64xxx_i2c_pdata kw_i2c_pdata = {
       .freq_m         = 8, /* assumes 166 MHz TCLK */
       .freq_n         = 3,
       .timeout        = 1000, /* Default timeout of 1 second */
};

static struct resource kw_i2c0_resources[] = {
       {
               .name   = "i2c base",
               .start  = INTER_REGS_BASE + TWSI_SLAVE_ADDR_REG(0),
               .end    = INTER_REGS_BASE + TWSI_SLAVE_ADDR_REG(0) + 0x20 -1,
               .flags  = IORESOURCE_MEM,
       },
       {
               .name   = "i2c irq",
               .start  = IRQ_TWSI(0),
               .end    = IRQ_TWSI(0),
               .flags  = IORESOURCE_IRQ,
       },
};

static struct platform_device kw_i2c0 = {
       .name           = MV64XXX_I2C_CTLR_NAME,
       .id             = 0,
       .num_resources  = ARRAY_SIZE(kw_i2c0_resources),
       .resource       = kw_i2c0_resources,
       .dev            = {
               .platform_data = &kw_i2c_pdata,
       },
};

static struct resource kw_i2c1_resources[] = {
       {
               .name   = "i2c base",
               .start  = INTER_REGS_BASE + TWSI_SLAVE_ADDR_REG(1),
               .end    = INTER_REGS_BASE + TWSI_SLAVE_ADDR_REG(1) + 0x20 -1,
               .flags  = IORESOURCE_MEM,
       },
       {
               .name   = "i2c irq",
               .start  = IRQ_TWSI(1),
               .end    = IRQ_TWSI(1),
               .flags  = IORESOURCE_IRQ,
       },
};

static struct platform_device kw_i2c1 = {
       .name           = MV64XXX_I2C_CTLR_NAME,
       .id             = 0,
       .num_resources  = ARRAY_SIZE(kw_i2c1_resources),
       .resource       = kw_i2c1_resources,
       .dev            = {
               .platform_data = &kw_i2c_pdata,
       },
};


/*****************************************************************************
 * UART
 ****************************************************************************/
static struct resource mv_uart0_resources[] = {
	{
		.start		= PORT0_BASE,
		.end		= PORT0_BASE + 0xff,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start          = IRQ_UART0,
		.end            = IRQ_UART0,
		.flags          = IORESOURCE_IRQ,
	},
};

static struct resource mv_uart1_resources[] = {
	{
		.start		= PORT1_BASE,
		.end		= PORT1_BASE + 0xff,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start          = IRQ_UART1,
		.end            = IRQ_UART1,
		.flags          = IORESOURCE_IRQ,
	},
};

static struct plat_serial8250_port mv_uart0_data[] = {
	{
		.mapbase	= PORT0_BASE,
		.membase	= (char *)PORT0_BASE,
		.irq		= IRQ_UART0,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
	},
	{ },
};

static struct plat_serial8250_port mv_uart1_data[] = {
	{
		.mapbase	= PORT1_BASE,
		.membase	= (char *)PORT1_BASE,
		.irq		= IRQ_UART1,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
	},
	{ },
};

static struct platform_device mv_uart = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= mv_uart0_data,
	},
#ifdef CONFIG_SYNO_MV88F6281
	.num_resources		= ARRAY_SIZE(mv_uart0_resources),
#else
	.num_resources		= 2, /*ARRAY_SIZE(mv_uart_resources),*/
#endif
	.resource		= mv_uart0_resources,
};

#ifdef CONFIG_SYNO_MV88F6281
static struct platform_device mv_uart1 = {
    .name           = "serial8250",
    .id         = PLAT8250_DEV_PLATFORM1,
    .dev            = {
        .platform_data  = mv_uart1_data,
    },
    .resource       = mv_uart1_resources,
    .num_resources      = ARRAY_SIZE(mv_uart1_resources),
};
#endif

static void serial_initialize(void)
{
	mv_uart0_data[0].uartclk = mv_uart1_data[0].uartclk = mvTclk;
	if((mvBoardIdGet() == DB_88F6280A_BP_ID) || (mvBoardIdGet() == RD_88F6282A_ID))
	{
		mv_uart.dev.platform_data = mv_uart1_data;
		mv_uart.resource = mv_uart1_resources;
	}
	platform_device_register(&mv_uart);
#ifdef CONFIG_SYNO_MV88F6281
	platform_device_register(&mv_uart1);
#endif
}

#ifdef CONFIG_SYNO_MV88F6281
#ifdef MY_ABC_HERE
extern void syno_mv_net_shutdown();
#endif
#define	UART1_REG(x)			(PORT1_BASE + ((UART_##x) << 2))
#define	SET8N1					0x3
#define	SOFTWARE_SHUTDOWN		0x31
#define	SOFTWARE_REBOOT			0x43
static void synology_power_off(void)
{
#ifdef MY_ABC_HERE
	/* platform driver will not shutdown when poweroff */
	syno_mv_net_shutdown();
#endif
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_SHUTDOWN, UART1_REG(TX));
}

static void synology_restart(char mode, const char *cmd)
{
	writel(SET8N1, UART1_REG(LCR));
	writel(SOFTWARE_REBOOT, UART1_REG(TX));

	/* Calls original reset function for models those do not use uP
	 * I.e. USB Station. */
	arm_machine_restart(mode, cmd);
}
#endif /* CONFIG_SYNO_MV88F6281 */

#ifdef CONFIG_MV_INCLUDE_AUDIO

/*****************************************************************************
 * I2S/SPDIF
 ****************************************************************************/
static struct resource mv_i2s_resources[] = {
	[0] = {
		.start	= INTER_REGS_BASE + AUDIO_REG_BASE(0),
		.end	= INTER_REGS_BASE + AUDIO_REG_BASE(0) + SZ_16K -1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_AUDIO_INT,
		.end	= IRQ_AUDIO_INT,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 mv_i2s0_dmamask = 0xFFFFFFFFUL;

static struct orion_i2s_platform_data mv_i2s_plat_data = {
	.dram	= NULL,
	.spdif_rec = 1,
	.spdif_play = 1,
	.i2s_rec = 1,
	.i2s_play = 1,
};


static struct platform_device mv_i2s = {
	.name           = "mv88fx_snd",
	.id             = 0,
	.dev            = {
		.dma_mask = &mv_i2s0_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
		.platform_data	= &mv_i2s_plat_data,
	},
	.num_resources  = ARRAY_SIZE(mv_i2s_resources),
	.resource       = mv_i2s_resources,
};

static struct platform_device mv_mv88fx_i2s = {
	.name           = "mv88fx-i2s",
	.id             = -1,
};

/*****************************************************************************
 * A2D on I2C bus
 ****************************************************************************/
static struct i2c_board_info __initdata i2c_a2d = {
	I2C_BOARD_INFO("i2s_i2c", 0x4A),
};

void __init mv_i2s_init(void)
{
	platform_device_register(&mv_mv88fx_i2s);
	platform_device_register(&mv_i2s);
	return;
}



#endif /* #ifdef CONFIG_MV_INCLUDE_AUDIO */

#if defined(CONFIG_MV_INCLUDE_SDIO)

static struct resource mvsdio_resources[] = {
	[0] = {
		.start	= INTER_REGS_BASE + MV_SDIO_REG_BASE,
		.end	= INTER_REGS_BASE + MV_SDIO_REG_BASE + SZ_1K -1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SDIO_IRQ_NUM,
		.end	= SDIO_IRQ_NUM,
		.flags	= IORESOURCE_IRQ,
	},

};

static u64 mvsdio_dmamask = 0xffffffffUL;

static struct mvsdio_platform_data mvsdio_data = {
	.gpio_write_protect	= 0,
	.gpio_card_detect	= 0,
	.dram			= NULL,
};

static struct platform_device mv_sdio_plat = {
	.name		= "mvsdio",
	.id		= -1,
	.dev		= {
		.dma_mask = &mvsdio_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data	= &mvsdio_data,
	},
	.num_resources	= ARRAY_SIZE(mvsdio_resources),
	.resource	= mvsdio_resources,
};

#ifdef CONFIG_FB_DOVE
/*****************************************************************************
 * LCD
 ****************************************************************************/

/*
 * LCD HW output Red[0] to LDD[0] when set bit [19:16] of reg 0x190
 * to 0x0. Which means HW outputs BGR format default. All platforms
 * uses this controller should enable .panel_rbswap. Unless layout
 * design connects Blue[0] to LDD[0] instead.
 */
static struct dovefb_mach_info kw_lcd0_dmi = {
	.id_gfx			= "GFX Layer 0",
	.id_ovly		= "Video Layer 0",
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.lcd_ref_clk		= 25000000,
#if defined(CONFIG_FB_DOVE_CLCD_DCONB_BYPASS0)
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
#else
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
#endif
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 3,
	.gpio_output_mask	= 3,
	.ddc_i2c_adapter	= 0,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 1,
	.active			= 1,
};

static struct dovefb_mach_info kw_lcd0_vid_dmi = {
	.id_ovly		= "Video Layer 0",
	.pix_fmt		= PIX_FMT_RGB888PACK,
	.io_pin_allocation	= IOPAD_DUMB24,
	.panel_rgb_type		= DUMB24_RGB888_0,
	.panel_rgb_reverse_lanes= 0,
	.gpio_output_data	= 3,
	.gpio_output_mask	= 3,
	.ddc_i2c_adapter	= -1,
	.invert_composite_blank	= 0,
	.invert_pix_val_ena	= 0,
	.invert_pixclock	= 0,
	.invert_vsync		= 0,
	.invert_hsync		= 0,
	.panel_rbswap		= 0,
	.active			= 1,
	.enable_lcd0		= 0,
};

extern unsigned int lcd0_enable;

#endif /* CONFIG_FB_DOVE */

#endif /* #if defined(CONFIG_MV_INCLUDE_SDIO) */

#ifdef CONFIG_MV_ETHERNET
/*****************************************************************************
 * Ethernet
 ****************************************************************************/
static struct platform_device mv88fx_eth = {
	.name		= "mv88fx_eth",
	.id		= 0,
	.num_resources	= 0,
};
#endif

static void __init kirkwood_l2_init(void)
{
#ifdef CONFIG_CACHE_FEROCEON_L2_WRITETHROUGH
	MV_REG_BIT_SET(CPU_L2_CONFIG_REG, 0x10);
	feroceon_l2_init(1);
#else
	MV_REG_BIT_RESET(CPU_L2_CONFIG_REG, 0x10);
	feroceon_l2_init(0);
#endif
}

/*****************************************************************************
 * SoC hwmon Thermal Sensor
 ****************************************************************************/
void __init kw_hwmon_init(void)
{
	platform_device_register_simple("kw-temp", 0, NULL, 0);
}

/*****************************************************************************
 * WATCHDOG
 ****************************************************************************/

/* the orion watchdog device data structure */
static struct orion_wdt_platform_data mv_wdt_data = {
	.tclk		= 0,
};

/* the watchdog device structure */
static struct platform_device mv_wdt_device = {
	.name		= "orion_wdt",
	.id		= -1,
	.dev		= {
		.platform_data	= &mv_wdt_data,
	},
	.num_resources	= 0,
};

/* init the watchdog device */
static void __init mv_wdt_init(void)
{
	mv_wdt_data.tclk = mvTclk;
	platform_device_register(&mv_wdt_device);
}

#ifdef MY_DEF_HERE
/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct mv_sata_platform_data synology_sata_data = {
	.n_ports	= 2,
};

static struct resource kirkwood_sata_resources[] = {
	{
		.name	= "sata base",
		.start	= (INTER_REGS_BASE + SATA_REG_BASE),
		.end	= (INTER_REGS_BASE + SATA_REG_BASE) + 0x5000 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "sata irq",
		.start	= SATA_IRQ_NUM,
		.end	= SATA_IRQ_NUM,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device kirkwood_sata = {
	.name	= "sata_mv",
	.id		= 0,
	.dev	= {
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(kirkwood_sata_resources),
	.resource		= kirkwood_sata_resources,
};

void __init kirkwood_sata_init()
{
	kirkwood_sata.dev.platform_data = &synology_sata_data;
	platform_device_register(&kirkwood_sata);
}
#endif

#ifdef CONFIG_SYNO_MV88F6281
void synology_gpio_init(void);
#endif

static void __init mv_init(void)
{
#ifdef CONFIG_CACHE_FEROCEON_L2
	kirkwood_l2_init();
#endif

        /* init the Board environment */
       	mvBoardEnvInit();

        /* init the controller environment */
        if( mvCtrlEnvInit() ) {
            printk( "Controller env initialization failed.\n" );
            return;
        }

	/* Init the CPU windows setting and the access protection windows. */
	if( mvCpuIfInit(mv_sys_map()) ) {

		printk( "Cpu Interface initialization failed.\n" );
		return;
	}

        if(mvBoardIdGet() == RD_88F6281A_ID) {
		mvBoardHDDPowerControl(1);
		mvBoardFanPowerControl(1);
#if defined(CONFIG_MV_INCLUDE_AUDIO)
		/* SPDIF only */
		mv_i2s_plat_data.i2s_rec = mv_i2s_plat_data.i2s_play = 0;	
#endif
	}


    	/* Init Tclk & SysClk */
    	mvTclk = mvBoardTclkGet();
   	mvSysclk = mvBoardSysClkGet();
	
        support_wait_for_interrupt = 1;
  
#ifdef CONFIG_JTAG_DEBUG
            support_wait_for_interrupt = 0; /*  for Lauterbach */
#endif

	elf_hwcap &= ~HWCAP_JAVA;

   	serial_initialize();

	/* At this point, the CPU windows are configured according to default definitions in mvSysHwConfig.h */
	/* and cpuAddrWinMap table in mvCpuIf.c. Now it's time to change defaults for each platform.         */
	mvCpuIfAddDecShow();

    	print_board_info();
	
	mv_gpio_init();

	/* I2C */
#ifdef CONFIG_FB_DOVE 
	/* LCD uses i2c 1 interface, while rest (like audio) uses i2c 0.
	   Currently the i2c linux driver doesn't support 2 interfaces */
	if(lcd0_enable == 1)
		platform_device_register(&kw_i2c1);
	else
#endif
		platform_device_register(&kw_i2c0);

#if defined(CONFIG_MV_INCLUDE_SDIO)
       if (MV_TRUE == mvCtrlPwrClckGet(SDIO_UNIT_ID, 0)) 
       {
		int irq_detect = mvBoardSDIOGpioPinGet();
		
		if (irq_detect != MV_ERROR) {
			mvsdio_data.gpio_card_detect = mvBoardSDIOGpioPinGet();
			irq_int_type[mvBoardSDIOGpioPinGet()+IRQ_GPP_START] = GPP_IRQ_TYPE_CHANGE_LEVEL;
		}
		if (MV_OK == mvSdmmcWinInit()) {
			mvsdio_data.clock = mvBoardTclkGet();
			platform_device_register(&mv_sdio_plat);
		}
       }

#endif

#if defined(CONFIG_MV_INCLUDE_AUDIO)
       if (MV_TRUE == mvCtrlPwrClckGet(AUDIO_UNIT_ID, 0)) 
       {
		platform_device_register(&mv_mv88fx_i2s);
		platform_device_register(&mv_i2s);
		i2c_register_board_info(0, &i2c_a2d, 1);
       }

#endif

#ifdef CONFIG_MV_ETHERNET
       /* ethernet */
       platform_device_register(&mv88fx_eth);
#endif

       /* WATCHDOG */
	mv_wdt_init();

#ifdef MY_DEF_HERE
	kirkwood_sata_init();
#endif

#ifdef CONFIG_SYNO_MV88F6281
	pm_power_off = synology_power_off;
	arm_pm_restart = synology_restart;
	synology_gpio_init();
#endif

#ifdef CONFIG_SENSORS_FEROCEON_KW
	/* SoC hwmon Thermal Sensor */
	if( mvBoardIdGet() == RD_88F6282A_ID ) {
		kw_hwmon_init();
	}
#endif

#ifdef CONFIG_FB_DOVE
	if (MV_TRUE == mvCtrlPwrClckGet(LCD_UNIT_ID, 0))
	clcd_platform_init(&kw_lcd0_dmi, &kw_lcd0_vid_dmi, NULL);
#endif
    return;
}

#ifdef CONFIG_FB_DOVE_OPTIMIZED_FB_MEM_ALLOC
/*
 * This fixup function is used to reserve memory for the GPU and VPU engines
 * as these drivers require large chunks of consecutive memory.
 */

void __init kw_tag_fixup_mem32(struct machine_desc *mdesc, struct tag *t,
		char **from, struct meminfo *meminfo)
{
	struct tag *last_tag = NULL;
	int total_size = PAGE_ALIGN(DEFAULT_FB_SIZE*4) * 2;
	void **fb_mem = kw_lcd0_dmi.fb_mem;
	unsigned int *fb_mem_size = kw_lcd0_dmi.fb_mem_size;
	unsigned int bank_size;

	for (; t->hdr.size; t = tag_next(t))
		if ((t->hdr.tag == ATAG_MEM) && (t->u.mem.size >= total_size)) {
			if ((last_tag == NULL) ||
			    (t->u.mem.start > last_tag->u.mem.start))
				last_tag = t;
		}

	if (last_tag == NULL) {
		early_printk(KERN_WARNING "No suitable memory tag was found, "
				"required memory %d MB.\n", total_size);
		return;
	}

	/* Resereve memory from last tag for LCD usage.
	** We assume that each tag is a different DRAM CS.
	** Allocate GFX & OVLY memory from different DRAM banks.
	** We assume that each bank is 1/8 of the DRAM.
	*/
	bank_size = last_tag->u.mem.size / 8;
	if((total_size / 2) < bank_size)
		total_size = bank_size * 2;
	fb_mem[0] = (void*)last_tag->u.mem.start + last_tag->u.mem.size - total_size;
	fb_mem[1] = (void*)last_tag->u.mem.start + last_tag->u.mem.size - (total_size / 2);
	last_tag->u.mem.size = 0; //-= total_size;
	fb_mem_size[0] = fb_mem_size[1] = (total_size / 2);
}
#endif
MACHINE_START(SYNOLOGY_6282 ,"Synology 6282 board")
	.boot_params = 0x00000100,
	.map_io = mv_map_io,
	.init_irq = mv_init_irq,
	.timer = &mv_timer,
	.init_machine = mv_init,
MACHINE_END

MACHINE_START(FEROCEON_KW ,"Feroceon-KW")
     /* MAINTAINER("MARVELL") */
    .boot_params = 0x00000100,
    .map_io = mv_map_io,
    .init_irq = mv_init_irq,
    .timer = &mv_timer,
    .init_machine = mv_init,
/* reserve memory for VMETA and GPU */
#ifdef CONFIG_FB_DOVE_OPTIMIZED_FB_MEM_ALLOC
    .fixup = kw_tag_fixup_mem32,
#endif
MACHINE_END

