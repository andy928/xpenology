/*
 * linux/drivers/video/dovedcon.c -- Marvell DCON driver for DOVE
 *
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by:
 *	Green Wan <gwan@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#include <video/dovedcon.h>

#define VGA_CHANNEL_DEFAULT	0x90C78
static int dovedcon_enable(struct dovedcon_info *ddi)
{
	unsigned int channel_ctrl;
	unsigned int ctrl0;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

#ifdef CONFIG_FB_DOVE_CLCD_DCONB_BYPASS0
	/* enable lcd0 pass to PortB */
	ctrl0 &= ~(0x3 << 8);
	ctrl0 |= (0x1 << 8);
	ddi->port_b = 1;
#endif
	
	/*
	 * Enable VGA clock, clear it to enable.
	 */
	ctrl0 &= ~(ddi->port_b << 25);	
		
	/*
	 * Enable LCD clock, clear it to enable
	 */
	ctrl0 &= ~(ddi->port_a << 24);	

	/*
	 * Enable LCD Parallel Interface, clear it to enable
	 */
	ctrl0 &= ~(0x1 << 17);	

	writel(ctrl0, ddi->reg_base+DCON_CTRL0);

	/*
	 * Configure VGA data channel and power on them.
	 */
	if (ddi->port_b) {
		channel_ctrl = VGA_CHANNEL_DEFAULT;
		writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);
		writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);
		writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);
	}

	return 0;
}

static int dovedcon_disable(struct dovedcon_info *ddi)
{
	unsigned int channel_ctrl;
	unsigned int ctrl0;

	/*
	 * Power down VGA data channel.
	 */
	channel_ctrl = (0x1 << 22);
	writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);
	writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);
	writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	/*
	 * Disable LCD Parallel Interface, clear it to enable
	 */
	ctrl0 &= (0x1 << 17);	

	/*
	 * Disable LCD clock, clear it to enable
	 */
	ctrl0 |= (0x1 << 24);	

	/*
	 * Disable VGA clock, clear it to enable.
	 */
	ctrl0 |= (0x1 << 25);	
		
	writel(ctrl0, ddi->reg_base+DCON_CTRL0);
	
	return 0;
}


#ifdef CONFIG_PM

/* suspend and resume support */
static int dovedcon_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	printk(KERN_INFO "dovedcon_suspend().\n");

	dovedcon_disable(ddi);
	clk_disable(ddi->clk);

	return 0;
}

static int dovedcon_resume(struct platform_device *pdev)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	printk(KERN_INFO "dovedcon_resume().\n");
	clk_enable(ddi->clk);
	dovedcon_enable(ddi);

	return 0;
}

#endif

static int dovedcon_fb_event_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	struct dovedcon_info *ddi;

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK && event != FB_EVENT_CONBLANK)
		return 0;

	ddi = container_of(self, struct dovedcon_info, fb_notif);

	if (*(int *)evdata->data == FB_BLANK_UNBLANK) {
		printk(KERN_NOTICE "DCON fb callback: unblanking...\n");
		dovedcon_enable(ddi);
	}
	else {
		printk(KERN_NOTICE "DCON fb callback: blank...\n");
		dovedcon_disable(ddi);
	}

	return 0;
}

static int dovedcon_hook_fb_event(struct dovedcon_info *ddi)
{
	memset(&ddi->fb_notif, 0, sizeof(ddi->fb_notif));
	ddi->fb_notif.notifier_call = dovedcon_fb_event_callback;
	return fb_register_client(&ddi->fb_notif);
}

/*
 * dcon sysfs interface implementation
 */
static ssize_t dcon_show_pa_clk(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	struct dovedcon_info *ddi;

	ddi = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ddi->port_a);
}

static ssize_t dcon_ena_pa_clk(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long ena_clk;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &ena_clk);
	if (rc)
		return rc;

	rc = -ENXIO;
	
	if (ddi->port_a != ena_clk) {
		unsigned int ctrl0;

		ddi->port_a = ena_clk;

		/*
		 * Get current configuration of CTRL0
		 */
		ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

		/* enable or disable LCD clk. */
		if (0 == ddi->port_a)
			ctrl0 |= (0x1 << 24);
		else
			ctrl0 &= ~(0x1 << 24);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
	}

	rc = count;

	return rc;
}

static ssize_t dcon_show_pb_clk(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	struct dovedcon_info *ddi;

	ddi = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ddi->port_b);
}

static ssize_t dcon_ena_pb_clk(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long ena_clk;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &ena_clk);
	if (rc)
		return rc;

	if (ddi->port_b != ena_clk) {
		unsigned int ctrl0;

		ddi->port_b = ena_clk;

		/*
		 * Get current configuration of CTRL0
		 */
		ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

		/* enable or disable LCD clk. */
		if (0 == ddi->port_b)
			ctrl0 |= (0x1 << 25);
		else
			ctrl0 &= ~(0x1 << 25);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
	}

	rc = count;

	return rc;
}

static ssize_t dcon_show_pa_mode(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	struct dovedcon_info *ddi;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	return sprintf(buf, "%d\n", (ctrl0 & (0x3 << 6)) >> 6 );
}

static ssize_t dcon_cfg_pa_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	if (value <= 3) {
		ctrl0 &= ~(0x3 << 6);
		ctrl0 |= (value << 6);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
		rc = count;
	}

	return rc;
}

static ssize_t dcon_show_pb_mode(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	struct dovedcon_info *ddi;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	return sprintf(buf, "%d\n", (ctrl0 & (0x3 << 8)) >> 8 );
}

static ssize_t dcon_cfg_pb_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	if (value <= 3) {
		ctrl0 &= ~(0x3 << 8);
		ctrl0 |= (value << 8);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
		rc = count;
	}

	return rc;
}

static ssize_t dcon_show_parallel_ena(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	struct dovedcon_info *ddi;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	return sprintf(buf, "%d\n", (ctrl0 & (0x1 << 17)) ? 0x0:0x1 );
}

static ssize_t dcon_ena_parallel(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int ctrl0;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	/*
	 * Get current configuration of CTRL0
	 */
	ctrl0 = readl(ddi->reg_base+DCON_CTRL0);

	if (value <= 1) {
		if (0 == value)
			ctrl0 |= (0x1 << 17);
		else
			ctrl0 &= ~(0x1 << 17);

		/* Apply setting. */
		writel(ctrl0, ddi->reg_base+DCON_CTRL0);
		rc = count;
	}

	return rc;
}

static ssize_t dcon_show_vga_pwr(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	struct dovedcon_info *ddi;
	unsigned int channel;
	unsigned int result;

	result = 0;
	ddi = dev_get_drvdata(dev);

	/*
	 * Get channel A power status
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);
	result |= (channel & (0x1 << 22)) ? 0x0:0x1;
	
	/*
	 * Get channel B power status
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);
	result |= (channel & (0x1 << 22)) ? 0x0:0x2;
	
	/*
	 * Get channel C power status
	 */
	channel = readl(ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);
	result |= (channel & (0x1 << 22)) ? 0x0:0x4;

	return sprintf(buf, "%d\n", result);
}

static ssize_t dcon_ena_vga_pwr(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct dovedcon_info *ddi;
	unsigned long value;
	unsigned int channel;

	ddi = dev_get_drvdata(dev);
	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;
	channel = 0;

	/*
	 * bit[0] for channel A power status.
	 * bit[1] for channel B power status.
	 * bit[2] for channel C power status.
	 */
	if (value <= 0x7) {
		/* config channel A */
		if (value & 0x1)
			channel = VGA_CHANNEL_DEFAULT;
		else
			channel |= (0x1 << 22);

		/* Apply setting. */
		writel(channel, ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);

		/* config channel B */
		if (value & 0x2)
			channel = VGA_CHANNEL_DEFAULT;
		else
			channel |= (0x1 << 22);

		/* Apply setting. */
		writel(channel, ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);

		/* config channel C */
		if (value & 0x4)
			channel = VGA_CHANNEL_DEFAULT;
		else
			channel |= (0x1 << 22);

		/* Apply setting. */
		writel(channel, ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);

		rc = count;
	}

	return rc;
}

/* Using sysfs to configure DCON function. */
static struct class *dcon_class;
#define DCON_ATTR_NUM 6
static struct device_attribute dcon_device_attributes[DCON_ATTR_NUM+1] = {
	__ATTR(pa_clk_ena, 0644, dcon_show_pa_clk, dcon_ena_pa_clk),
	__ATTR(pb_clk_ena, 0644, dcon_show_pb_clk, dcon_ena_pb_clk),
	__ATTR(pa_mode, 0644, dcon_show_pa_mode, dcon_cfg_pa_mode),
	__ATTR(pb_mode, 0644, dcon_show_pb_mode, dcon_cfg_pb_mode),
	__ATTR(parallel_ena, 0644, dcon_show_parallel_ena, dcon_ena_parallel),
	__ATTR(vga_ch_pwr, 0644, dcon_show_vga_pwr, dcon_ena_vga_pwr),
	__ATTR_NULL,
};

static int __init dcon_class_init(void)
{
	dcon_class = class_create(THIS_MODULE, "dcon");
	if (IS_ERR(dcon_class)) {
		printk(KERN_WARNING "Unable to create dcon class; errno = %ld\n",
				PTR_ERR(dcon_class));
		return PTR_ERR(dcon_class);
	}

	dcon_class->dev_attrs = dcon_device_attributes;
	dcon_class->suspend = NULL;
	dcon_class->resume = NULL;
	return 0;
}

static void dcon_device_release(struct device *dev)
{
	struct dovedcon_info *ddi;

	ddi = dev_get_drvdata(dev);
	kfree(ddi);
}

/* Initialization */
static int __init dovedcon_probe(struct platform_device *pdev)
{
	struct dovedcon_mach_info *ddmi;
	struct dovedcon_info *ddi;
	struct resource *res;

	ddmi = pdev->dev.platform_data;
	if (!ddmi)
		return -EINVAL;

	ddi = kzalloc(sizeof(struct dovedcon_info), GFP_KERNEL);
	if (ddi == NULL)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		kfree(ddi);
		return -EINVAL;
	}

	if (!request_mem_region(res->start, res->end - res->start,
	    "MRVL DCON Regs")) {
		printk(KERN_INFO "Cannot reserve DCON memory mapped"
			" area 0x%lx @ 0x%lx\n",
			(unsigned long)res->start,
			(unsigned long)res->end - res->start);
		kfree(ddi);
		return -ENXIO;
	}

	ddi->reg_base = ioremap_nocache(res->start, res->end - res->start);

	if (!ddi->reg_base) {
		kfree(ddi);
		return -EINVAL;
	}

	platform_set_drvdata(pdev, ddi);
//	dev_set_drvdata(&pdev->dev, ddi);

	//printk(KERN_INFO "DCON: pdev->dev.class=<0x%x>\n", pdev->dev.class);
	//printk(KERN_INFO "DCON: pdev->dev.parent=<0x%x>\n", pdev->dev.parent);
	//printk(KERN_INFO "DCON: pdev->dev.release=<0x%x>\n", pdev->dev.release);
	ddi->clk = clk_get(&pdev->dev, "LCDCLK");
	if (!IS_ERR(ddi->clk))
		clk_enable(ddi->clk);

	ddi->port_a = ddmi->port_a;
	ddi->port_b = ddmi->port_b;

	/* Initialize DCON hardware */
	dovedcon_enable(ddi);

	/*
	 * Register to receive fb blank event.
	 */
	dovedcon_hook_fb_event(ddi);

	dcon_class_init();
	pdev->dev.class = dcon_class;
	pdev->dev.release = dcon_device_release;

	{
		int i;

		for(i = 0; i < DCON_ATTR_NUM; i++)
			if (device_create_file(&pdev->dev,
			    &dcon_device_attributes[i]))
				printk(KERN_ERR
				    "dcon register <%d> sysfs failed\n", i);
	}

	printk(KERN_INFO "dovedcon has been initialized.\n");

	return 0;
}

/*
 *  Cleanup
 */
static int dovedcon_remove(struct platform_device *pdev)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	clk_disable(ddi->clk);
	clk_put(ddi->clk);
	iounmap(ddi->reg_base);
	kfree(ddi);

	return 0;
}

static struct platform_driver dovedcon_pdriver = {
	.probe		= dovedcon_probe,
	.remove		= dovedcon_remove,
#ifdef CONFIG_PM
	.suspend	= dovedcon_suspend,
	.resume		= dovedcon_resume,
#endif /* CONFIG_PM */
	.driver		=	{
		.name	=	"dovedcon",
		.owner	=	THIS_MODULE,
	},
};
static int __init dovedcon_init(void)
{
	return platform_driver_register(&dovedcon_pdriver);
}

static void __exit dovedcon_exit(void)
{
	platform_driver_unregister(&dovedcon_pdriver);
}

module_init(dovedcon_init);
module_exit(dovedcon_exit);

MODULE_AUTHOR("Green Wan <gwan@marvell.com>");
MODULE_DESCRIPTION("DCON driver for Dove LCD unit");
MODULE_LICENSE("GPL");
