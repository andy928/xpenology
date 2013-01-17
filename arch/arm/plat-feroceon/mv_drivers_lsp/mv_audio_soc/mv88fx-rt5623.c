/*
 *
 *	Marvell Orion Alsa SOC Sound driver
 *
 *	Author: Yuval Elmaliah
 *	Author: Ethan Ku
 *	Copyright (C) 2008 Marvell Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/asoundef.h>
#include <asm/mach-types.h>
//#include <asm/arch/hardware.h>
#include "../sound/soc/codecs/rt5623.h"
#include <linux/mbus.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <../arch/arm/mach-dove/common.h>
//#include <asm/plat-orion/i2s-orion.h>
#include "mv88fx-pcm.h"
#include "mv88fx-i2s.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "audio/mvAudioRegs.h"

extern int mvmpp_gpio_get_value(unsigned int);
extern int mvmpp_gpio_set_value(unsigned int,int);

static struct mv88fx_snd_machine_data mv88fx_machine_data;
static struct rt5623_setup_data rt5623_setup;


static int mv88fx_machine_startup(struct snd_pcm_substream *substream)
{
//	struct snd_soc_pcm_runtime *rtd = substream->private_data;
//	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;
//	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
//	int ret;
#if 0
	int phoneInDetected;

//	printk("%s\n",__func__);

	/* check the jack status at stream startup */
	phoneInDetected = mvmpp_gpio_get_value(MPP_PhLine_IN);

	if (phoneInDetected < 0) {
		snd_printk("Failed to detect phone-in.\n");
	} else {

		if (! phoneInDetected) {
			rt5623_setup.mic2_input = 0;
		} else {
			rt5623_setup.mic2_input = 1;
		}
	}
#endif
	return 0;

}


static void mv88fx_machine_shutdown(struct snd_pcm_substream *substream)
{
//	printk("%s\n",__func__);
}



static int mv88fx_machine_hw_params(struct snd_pcm_substream *substream,	
				   struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	unsigned int clk = 0;
	int ret = 0;
	unsigned int format;

//	printk("%s\n",__func__);
	/* set codec DAI configuration */


	switch (params_rate(params)) {
	case 44100:
		clk = 11289600;
		break;
	case 48000:
		clk = 12288000;
		break;
	case 96000:
		clk = 24576000;
		break;
	}

	format = SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF;
	ret = codec_dai->ops->set_fmt(codec_dai, format);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = cpu_dai->ops->set_fmt(cpu_dai, format);
	if (ret < 0)
		return ret;

	/* cpu clock is the mv88fx master clock sent to codec */
	ret = cpu_dai->ops->set_sysclk(cpu_dai, 0, 0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	/* codec system clock is supplied by mv88fx*/
	ret = codec_dai->ops->set_sysclk(codec_dai, 0, clk, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}

static int mv88fx_machine_trigger(struct snd_pcm_substream *substream,int cmd)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if(substream->stream==SNDRV_PCM_STREAM_PLAYBACK)
		{
//			mvmpp_gpio_set_value(MPP_Amp_PwrDn,1);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		if(substream->stream==SNDRV_PCM_STREAM_PLAYBACK)
		{
//			mvmpp_gpio_set_value(MPP_Amp_PwrDn,0);
		}
		break;

	}
	return 0;
}

static int mv88fx_rt5623_init(struct snd_soc_codec *codec)
{
	mv88fx_snd_debug("");
//	mvmpp_gpio_set_value(MPP_Amp_PwrDn,0);
	return 0;
}


/* machine stream operations */
static struct snd_soc_ops mv88fx_rt5623_machine_ops =
{
	.startup = mv88fx_machine_startup,
	.shutdown = mv88fx_machine_shutdown,
	.hw_params = mv88fx_machine_hw_params,
	.trigger = mv88fx_machine_trigger,

};



static struct snd_soc_dai_link mv88fx_dai = {
	.name = "RT5623",
	.stream_name = "RT5623",
	.cpu_dai = &mv88fx_i2s_dai,
	.codec_dai = &rt5623_dai,
	.ops = &mv88fx_rt5623_machine_ops,
	.init = mv88fx_rt5623_init,
};

static int mv88fx_probe(struct platform_device *pdev)
{
	return 0;
}

static int mv88fx_remove(struct platform_device *pdev)
{
	return 0;
}

static struct snd_soc_card dove = {
	.name = "Dove",
	.platform = &mv88fx_soc_platform,
	.probe = mv88fx_probe,
	.remove = mv88fx_remove,

	/* CPU <--> Codec DAI links */
	.dai_link = &mv88fx_dai,
	.num_links = 1,
};

static struct rt5623_setup_data rt5623_setup = {
	.i2c_address = 0x1a,
	.mic2_input = 1,
};


static struct snd_soc_device mv88fx_snd_devdata = {
	.card = &dove,
	.codec_dev = &soc_codec_dev_rt5623,
	.codec_data = &rt5623_setup,
};

static int mv88fx_initalize_machine_data(struct platform_device *pdev)
{
	struct resource *r = NULL;
	int err = 0;

	mv88fx_snd_debug("");

	mv88fx_machine_data.port = pdev->id;
	mv88fx_machine_data.pdata =
		(struct orion_i2s_platform_data *)pdev->dev.platform_data;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r) {
		err = -ENXIO;
		goto error;
	}
	r = request_mem_region(r->start, SZ_16K, DRIVER_NAME);
	if (!r) {
		err = -EBUSY;
		goto error;
	}
	mv88fx_machine_data.res = r;
	mv88fx_machine_data.base = ioremap(r->start, SZ_16K);

	if (!mv88fx_machine_data.base) {
		mv88fx_snd_error("ioremap failed");
		err = -ENOMEM;
		goto error;
	}
	mv88fx_machine_data.base -= MV_AUDIO_REGS_OFFSET(mv88fx_machine_data.port);

	mv88fx_machine_data.irq = platform_get_irq(pdev, 0);
	if (mv88fx_machine_data.irq == NO_IRQ) {
		err = -ENXIO;
		goto error;
	}
#if defined(CONFIG_HAVE_CLK)
	mv88fx_machine_data.clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(mv88fx_machine_data.clk))
		dev_notice(&pdev->dev, "cannot get clkdev\n");
	else
		clk_enable(mv88fx_machine_data.clk);
#endif
	return 0;
error:
	if (mv88fx_machine_data.base) {
		iounmap(mv88fx_machine_data.base);
		mv88fx_machine_data.base = NULL;
	}
	release_mem_region(mv88fx_machine_data.res->start, SZ_16K);
	return err;
}

static int mv88fx_snd_probe(struct platform_device *pdev)
{
	int ret = 0;

	mv88fx_snd_debug("");

	if (mv88fx_initalize_machine_data(pdev) != 0)
		goto error;

	mv88fx_machine_data.snd_dev = platform_device_alloc("soc-audio", -1);
	if (!mv88fx_machine_data.snd_dev) {
		ret = -ENOMEM;
		goto error;
	}

	platform_set_drvdata(mv88fx_machine_data.snd_dev, &mv88fx_snd_devdata);
	mv88fx_snd_devdata.dev = &mv88fx_machine_data.snd_dev->dev;

	mv88fx_machine_data.snd_dev->dev.platform_data = &mv88fx_machine_data;

	ret = platform_device_add(mv88fx_machine_data.snd_dev);

	if(ret)
	{
                platform_device_put(mv88fx_machine_data.snd_dev);
	}

	return ret;
error:
	mv88fx_snd_error("");
#if defined(CONFIG_HAVE_CLK)
	if (!IS_ERR(mv88fx_machine_data.clk)) {
		clk_disable(mv88fx_machine_data.clk);
		clk_put(mv88fx_machine_data.clk);
	}
#endif
	if (mv88fx_machine_data.snd_dev)
		platform_device_unregister(mv88fx_machine_data.snd_dev);
	return ret;

}

static int mv88fx_snd_remove(struct platform_device *dev)
{
	mv88fx_snd_debug("");
#if defined(CONFIG_HAVE_CLK)
	if (!IS_ERR(mv88fx_machine_data.clk)) {
		clk_disable(mv88fx_machine_data.clk);
		clk_put(mv88fx_machine_data.clk);
	}
#endif
	mv88fx_machine_data.snd_dev->dev.platform_data = NULL;
	platform_device_unregister(mv88fx_machine_data.snd_dev);
	release_mem_region(mv88fx_machine_data.res->start, SZ_16K);
	return 0;
}

static struct platform_driver mv88fx_snd_driver = {
	.probe = mv88fx_snd_probe,
	.remove = mv88fx_snd_remove,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		   .name = DRIVER_NAME,
		   },

};

static int __init mv88fx_snd_init(void)
{
	if (!machine_is_dove_rd_avng())
		return -ENODEV;

	mv88fx_snd_debug("");
	return platform_driver_register(&mv88fx_snd_driver);
}

static void __exit mv88fx_snd_exit(void)
{
	mv88fx_snd_debug("");
	platform_driver_unregister(&mv88fx_snd_driver);

}

module_init(mv88fx_snd_init);
module_exit(mv88fx_snd_exit);

/* Module information */
MODULE_AUTHOR("Yuval Elmaliah <eyuval@marvell.com>");
MODULE_AUTHOR("Ethan Ku <eku@marvell.com>");
MODULE_DESCRIPTION("ALSA SoC Dove");
MODULE_LICENSE("GPL");
