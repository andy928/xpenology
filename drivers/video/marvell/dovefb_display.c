#include <linux/module.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/smp_lock.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>

#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include <video/dovedcon.h>
#include <video/dovefb_display.h>

/*
 * LCD controll register physical address
 */
static unsigned int lcd_regbase;
//module_param(lcd_regbase, uint, 0xf1810000);
module_param(lcd_regbase, uint, 0);
MODULE_PARM_DESC(lcd_regbase, "DCON register base");

/*
 * 0 means lcd0 = regbase + 0x0, lcd1 = regbase + 0x10000;
 * 1 means lcd0 = regbase + 0x10000, lcd1 = regbase + 0x0;
 */
static unsigned int lcdseq;
module_param(lcdseq, uint, 1);
MODULE_PARM_DESC(lcdseq, "LCD sequence");

extern struct class *fb_class;
static void *lcd0_regbase;
static void *lcd1_regbase;
static void *dcon_regbase;

static int config_dcon(unsigned int settings)
{
	writel(settings, dcon_regbase+DCON_CTRL0);
	return 0;
}

static int config_porta(unsigned int settings)
{
	unsigned int ctrl0;

	ctrl0 = readl(dcon_regbase+DCON_CTRL0);
	ctrl0 &= ~(0x3 << 6);
	ctrl0 |= (settings << 6);
	writel(ctrl0, dcon_regbase+DCON_CTRL0);

	return 0;
}

static int config_portb(unsigned int settings)
{
	unsigned int ctrl0;

	ctrl0 = readl(dcon_regbase+DCON_CTRL0);
	ctrl0 &= ~(0x3 << 8);
	ctrl0 |= (settings << 8);
	writel(ctrl0, dcon_regbase+DCON_CTRL0);

	return 0;
}

/*
 * #define FBIO_SET_DISPLAY_MODE   0
 * #define FBIO_GET_DISPLAY_MODE   1
 * #define FBIO_CONFIG_DCON        2
 * #define FBIO_CONFIG_PORTA       3
 * #define FBIO_CONFIG_PORTB       4
 * #define FBIO_GET_DCON_STATUS    5
 */
static long display_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case FBIO_SET_DISPLAY_MODE:
		/* configure display mode. */
		printk(KERN_INFO "Reserved!! Not implement yet.\n");
		break;
	case FBIO_GET_DISPLAY_MODE:
		/* get display configuration. */
		printk(KERN_INFO "Reserved!! Not implement yet.\n");
		break;
	case FBIO_CONFIG_DCON:
	{
		unsigned int settings;
		/* Get data from user space. */
		if (copy_from_user(&settings, (void *)arg, sizeof(unsigned int)))
			return -EFAULT;
		config_dcon(settings);
		break;
	}
	case FBIO_CONFIG_PORTA:
	{
		unsigned int settings;
		/* Get data from user space. */
		if (copy_from_user(&settings, (void *)arg, sizeof(unsigned int)))
			return -EFAULT;

		config_porta(settings);
		break;
	}
	case FBIO_CONFIG_PORTB:
	{
		unsigned int settings;

		/* Get data from user space. */
		if (copy_from_user(&settings, (void *)arg, sizeof(unsigned int)))
			return -EFAULT;

		config_portb(settings);
		break;
	}
	case FBIO_GET_DCON_STATUS:
	{
		unsigned int dc_status;

		/* get dcon configuration. */
		dc_status = readl(dcon_regbase+DCON_CTRL0);

		printk(KERN_INFO "get dcon config.\n");
		if (copy_to_user((void *)arg, &dc_status, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	default:
		printk(KERN_ERR "Unknown command\n");
	}

	return 0;
}

static const struct file_operations display_fops = {
	.owner =	THIS_MODULE,
	.unlocked_ioctl = display_ioctl,
};


static int __init
dovefb_display_init(void)
{
#ifdef MTL_DEBUG
	uint x;

	printk(KERN_INFO "lcd_regbase = 0x%08x\n", lcd_regbase);
	printk(KERN_INFO "lcdseq = %d, 1^lcdseq = %d\n", lcdseq, 1^lcdseq);
	printk(KERN_INFO "lcdseq = %d, 0^lcdseq = %d\n", lcdseq, 0^lcdseq);
#endif
	printk(KERN_WARNING "dovefb_display_init\n");

	/* register character. */
	if (register_chrdev(30, "display tools", &display_fops))
		printk("unable to get major %d for fb devs\n", 30);

	if (lcd_regbase) {
		/* remap to ctrl registers. */
		lcd0_regbase = ioremap_nocache( lcd_regbase + (0x10000*(0^lcdseq)), (0x10000 - 1));
		lcd1_regbase = ioremap_nocache( lcd_regbase + (0x10000*(1^lcdseq)), (0x10000 - 1));
		dcon_regbase = ioremap_nocache( lcd_regbase + 0x20000,		    (0x10000 - 1));
#ifdef MTL_DEBUG
		x = readl( lcd0_regbase + 0x104 );
		printk(KERN_INFO "debug lcd0 reg 0x104 = 0x%08x\n", x);
		x = readl( lcd1_regbase + 0x108 );
		printk(KERN_INFO "debug lcd0 reg 0x104 = 0x%08x\n", x);
		x = readl( dcon_regbase + 0x000 );
		printk(KERN_INFO "debug dcon reg 0x000 = 0x%08x\n", x);
#endif
	}

	printk(KERN_WARNING "dovefb_display driver init ok.\n");
	return 0;
}

static void __exit
dovefb_display_exit(void)
{
	iounmap(lcd0_regbase);
	iounmap(lcd1_regbase);
	iounmap(dcon_regbase);
	unregister_chrdev(30, "display tools");
	printk(KERN_WARNING "dovefb_display driver unload OK.\n");
}

module_init(dovefb_display_init);
module_exit(dovefb_display_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Display mode driver");

