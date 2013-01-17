/*
 *
 *	Marvell Orion Alsa SOC Sound driver
 *
 *	Author: Yuval Elmaliah
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
#include <sound/soc-dai.h>
#include <sound/soc-dapm.h>
#include <sound/asoundef.h>
#include <asm/mach-types.h>
#include "../sound/soc/codecs/cs42l51.h"
#include <linux/mbus.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
//#include <../arch/arm/mach-dove/common.h>
#include <plat/i2s-orion.h>
#include "mv88fx-pcm.h"
#include "mv88fx-i2s.h"
#include "audio/mvAudioRegs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"

static struct mv88fx_snd_machine_data mv88fx_machine_data;

static int mv88fx_snd_spdif_mask_info(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 1;
	return 0;
}

static int mv88fx_snd_spdif_mask_get(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.iec958.status[0] = 0xff;
	ucontrol->value.iec958.status[1] = 0xff;
	ucontrol->value.iec958.status[2] = 0xff;
	ucontrol->value.iec958.status[3] = 0xff;
	return 0;
}

static struct snd_kcontrol_new mv88fx_snd_spdif_mask = {
	.access = SNDRV_CTL_ELEM_ACCESS_READ,
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,
	.name = SNDRV_CTL_NAME_IEC958("", PLAYBACK, CON_MASK),
	.info = mv88fx_snd_spdif_mask_info,
	.get = mv88fx_snd_spdif_mask_get,
};

static int mv88fx_snd_spdif_stream_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	mv88fx_snd_debug("");
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 1;
	return 0;
}

static int mv88fx_snd_spdif_stream_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct mv88fx_snd_chip *chip = snd_kcontrol_chip(kcontrol);
	int i, word;

	mv88fx_snd_debug("");
	spin_lock_irq(&chip->reg_lock);
	for (word = 0; word < 4; word++) {
		chip->stream[SNDRV_PCM_STREAM_PLAYBACK].spdif_status[word] =
		    mv88fx_snd_readl(chip->base,
		     MV_AUDIO_SPDIF_PLAY_CH_STATUS_LEFT_REG(chip->port, word));
		for (i = 0; i < 4; i++)
			ucontrol->value.iec958.status[word + i] =
			    (chip->stream[SNDRV_PCM_STREAM_PLAYBACK].
			     spdif_status[word] >> (i * 8)) & 0xff;

	}
	spin_unlock_irq(&chip->reg_lock);
	return 0;
}

static int mv88fx_snd_spdif_stream_put(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct mv88fx_snd_chip *chip = snd_kcontrol_chip(kcontrol);
	int i, change = 0, word;
	unsigned int val;
	struct mv88fx_snd_stream *pbstream =
		&chip->stream[SNDRV_PCM_STREAM_PLAYBACK];
	int port = chip->port;

	mv88fx_snd_debug("");
	val = 0;
	port = port;
	spin_lock_irq(&chip->reg_lock);
	for (word = 0; word < 4; word++) {
		for (i = 0; i < 4; i++) {
			chip->stream[SNDRV_PCM_STREAM_PLAYBACK].
			    spdif_status[word] |=
			    ucontrol->value.iec958.status[word + i] << (i * 8);
		}
		mv88fx_snd_writel(chip->base,
			MV_AUDIO_SPDIF_PLAY_CH_STATUS_LEFT_REG(port, word),
			pbstream->spdif_status[word]);

		mv88fx_snd_writel(chip->base,
			MV_AUDIO_SPDIF_PLAY_CH_STATUS_RIGHT_REG(port, word),
			pbstream->spdif_status[word]);

	}

	if (pbstream->spdif_status[0] & IEC958_AES0_NONAUDIO)
		chip->pcm_mode = NON_PCM;

	spin_unlock_irq(&chip->reg_lock);
	return change;
}

static struct snd_kcontrol_new mv88fx_snd_spdif_stream = {
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE |
	    SNDRV_CTL_ELEM_ACCESS_INACTIVE,
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,
	.name = SNDRV_CTL_NAME_IEC958("", PLAYBACK, PCM_STREAM),
	.info = mv88fx_snd_spdif_stream_info,
	.get = mv88fx_snd_spdif_stream_get,
	.put = mv88fx_snd_spdif_stream_put
};

static int mv88fx_snd_spdif_default_info(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_info *uinfo)
{
	mv88fx_snd_debug("");
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 1;
	return 0;
}

static int mv88fx_snd_spdif_default_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct mv88fx_snd_chip *chip = snd_kcontrol_chip(kcontrol);
	struct mv88fx_snd_stream *pbstream =
		&chip->stream[SNDRV_PCM_STREAM_PLAYBACK];
	int port = chip->port;
	int i, word;

	mv88fx_snd_debug("");

	port = port;
	spin_lock_irq(&chip->reg_lock);
	for (word = 0; word < 4; word++) {
		pbstream->spdif_status[word] =
		    mv88fx_snd_readl(chip->base,
			MV_AUDIO_SPDIF_PLAY_CH_STATUS_LEFT_REG(port, word));

		for (i = 0; i < 4; i++)
			ucontrol->value.iec958.status[word + i] =
			(pbstream->spdif_status[word] >> (i * 8)) & 0xff;

	}
	spin_unlock_irq(&chip->reg_lock);
	return 0;
}

static int mv88fx_snd_spdif_default_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct mv88fx_snd_chip *chip = snd_kcontrol_chip(kcontrol);
	struct mv88fx_snd_stream *pbstream =
		&chip->stream[SNDRV_PCM_STREAM_PLAYBACK];
	int port = chip->port;
	int i, change = 0, word;
	unsigned int val;

	mv88fx_snd_debug("");

	port = port;
	val = 0;
	spin_lock_irq(&chip->reg_lock);
	for (word = 0; word < 4; word++) {
		for (i = 0; i < 4; i++) {
			pbstream->spdif_status[word] |=
			    ucontrol->value.iec958.status[word + i] << (i * 8);
		}
		mv88fx_snd_writel(chip->base,
			MV_AUDIO_SPDIF_PLAY_CH_STATUS_LEFT_REG(port, word),
			pbstream->spdif_status[word]);
		mv88fx_snd_writel(chip->base,
			MV_AUDIO_SPDIF_PLAY_CH_STATUS_RIGHT_REG(port, word),
			pbstream->spdif_status[word]);

	}
	if (pbstream->spdif_status[0] & IEC958_AES0_NONAUDIO)
		chip->pcm_mode = NON_PCM;


	spin_unlock_irq(&chip->reg_lock);
	return change;
}
static struct snd_kcontrol_new mv88fx_snd_spdif_default = {
	.iface = SNDRV_CTL_ELEM_IFACE_PCM,
	.name = SNDRV_CTL_NAME_IEC958("", PLAYBACK, DEFAULT),
	.info = mv88fx_snd_spdif_default_info,
	.get = mv88fx_snd_spdif_default_get,
	.put = mv88fx_snd_spdif_default_put
};

struct mv88fx_snd_mixer_enum {
	char **names;		/* enum names */
	int *values;		/* values to be updated */
	int count;		/* number of elements */
	void *rec;		/* field to be updated */
};

int mv88fx_snd_mixer_enum_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	struct mv88fx_snd_mixer_enum *mixer_enum =
	    (struct mv88fx_snd_mixer_enum *)kcontrol->private_value;

	mv88fx_snd_debug("");

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = mixer_enum->count;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item =
		    uinfo->value.enumerated.items - 1;

	strcpy(uinfo->value.enumerated.name,
	       mixer_enum->names[uinfo->value.enumerated.item]);

	return 0;
}
int mv88fx_snd_mixer_enum_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct mv88fx_snd_mixer_enum *mixer_enum =
	    (struct mv88fx_snd_mixer_enum *)kcontrol->private_value;
	int i;
	unsigned int val;

	mv88fx_snd_debug("");

	val = *(unsigned int *)mixer_enum->rec;

	for (i = 0; i < mixer_enum->count; i++) {

		if (val == (unsigned int)mixer_enum->values[i]) {
			ucontrol->value.enumerated.item[0] = i;
			break;
		}
	}

	return 0;
}

int mv88fx_snd_mixer_enum_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	unsigned int val, *rec;
	struct mv88fx_snd_mixer_enum *mixer_enum =
	    (struct mv88fx_snd_mixer_enum *)kcontrol->private_value;
	int i;

	mv88fx_snd_debug("");

	rec = (unsigned int *)mixer_enum->rec;
	val = ucontrol->value.enumerated.item[0];

	if (val < 0)
		val = 0;
	if (val > mixer_enum->count)
		val = mixer_enum->count;

	for (i = 0; i < mixer_enum->count; i++) {

		if (val == i) {
			*rec = (unsigned int)mixer_enum->values[i];
			break;
		}
	}

	return 0;
}

#define MV88FX_PCM_MIXER_ENUM(xname, xindex, value)	\
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
  .name = xname, \
  .index = xindex, \
  .info = mv88fx_snd_mixer_enum_info, \
  .get = mv88fx_snd_mixer_enum_get, \
  .put = mv88fx_snd_mixer_enum_put, \
  .private_value = (unsigned long)value,	\
}

char *playback_src_mixer_names[] = { "SPDIF", "I2S", "SPDIF And I2S" };
int playback_src_mixer_values[] = { SPDIF, I2S, (SPDIF | I2S) };

struct mv88fx_snd_mixer_enum playback_src_mixer = {
	.names = playback_src_mixer_names,
	.values = playback_src_mixer_values,
	.count = 3,
};

char *playback_mono_mixer_names[] = { "Mono Both", "Mono Left", "Mono Right" };
int playback_mono_mixer_values[] = { MONO_BOTH, MONO_LEFT, MONO_RIGHT };

struct mv88fx_snd_mixer_enum playback_mono_mixer = {
	.names = playback_mono_mixer_names,
	.values = playback_mono_mixer_values,
	.count = 3,
};

char *capture_src_mixer_names[] = { "SPDIF", "I2S" };
int capture_src_mixer_values[] = { SPDIF, I2S };

struct mv88fx_snd_mixer_enum capture_src_mixer = {
	.names = capture_src_mixer_names,
	.values = capture_src_mixer_values,
	.count = 2,
};

char *capture_mono_mixer_names[] = { "Mono Left", "Mono Right" };
int capture_mono_mixer_values[] = { MONO_LEFT, MONO_RIGHT };

struct mv88fx_snd_mixer_enum capture_mono_mixer = {
	.names = capture_mono_mixer_names,
	.values = capture_mono_mixer_values,
	.count = 2,
};

static struct snd_kcontrol_new mv88fx_snd_mixers[] = {
	MV88FX_PCM_MIXER_ENUM("Playback output type", 0,
			      &playback_src_mixer),

	MV88FX_PCM_MIXER_ENUM("Playback mono type", 0,
			      &playback_mono_mixer),

	MV88FX_PCM_MIXER_ENUM("Capture input Type", 0,
			      &capture_src_mixer),

	MV88FX_PCM_MIXER_ENUM("Capture mono type", 0,
			      &capture_mono_mixer),

};

#define	PLAYBACK_MIX_INDX	0
#define PLAYBACK_MONO_MIX_INDX	1
#define CAPTURE_MIX_INDX	2
#define	CAPTURE_MONO_MIX_INDX	3

static int mv88fx_snd_ctrl_new(struct snd_card *card)
{
	int err = 0;
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();

	playback_src_mixer.rec =
	    &chip->stream[SNDRV_PCM_STREAM_PLAYBACK].dig_mode;
	playback_mono_mixer.rec =
	    &chip->stream[SNDRV_PCM_STREAM_PLAYBACK].mono_mode;

	capture_src_mixer.rec =
	    &chip->stream[SNDRV_PCM_STREAM_CAPTURE].dig_mode;
	capture_mono_mixer.rec =
	    &chip->stream[SNDRV_PCM_STREAM_CAPTURE].mono_mode;

	if (chip->pdata->i2s_play && chip->pdata->spdif_play) {
		err =
		    snd_ctl_add(card,
				snd_ctl_new1(&mv88fx_snd_mixers
					     [PLAYBACK_MIX_INDX], chip));
		if (err < 0)
			return err;
	}

	if (chip->pdata->i2s_play || chip->pdata->spdif_play) {
		err =
		    snd_ctl_add(card,
				snd_ctl_new1(&mv88fx_snd_mixers
					     [PLAYBACK_MONO_MIX_INDX], chip));
		if (err < 0)
			return err;
	}

	if (chip->pdata->i2s_rec && chip->pdata->spdif_rec) {
		err =
		    snd_ctl_add(card,
				snd_ctl_new1(&mv88fx_snd_mixers
					     [CAPTURE_MIX_INDX], chip));
		if (err < 0)
			return err;
	}

	if (chip->pdata->i2s_rec || chip->pdata->spdif_rec) {
		err =
		    snd_ctl_add(card,
				snd_ctl_new1(&mv88fx_snd_mixers
					     [CAPTURE_MONO_MIX_INDX], chip));
		if (err < 0)
			return err;
	}

	if (chip->pdata->spdif_play) {
		err = snd_ctl_add(card, snd_ctl_new1(&mv88fx_snd_spdif_mask,
						     chip));
		if (err < 0)
			return err;
	}
	if (chip->pdata->spdif_play) {
		err = snd_ctl_add(card, snd_ctl_new1(&mv88fx_snd_spdif_default,
						     chip));
		if (err < 0)
			return err;
	}
	if (chip->pdata->spdif_play) {
		err = snd_ctl_add(card, snd_ctl_new1(&mv88fx_snd_spdif_stream,
						     chip));
		if (err < 0)
			return err;
	}

	return err;
}


static int mv88fx_cs42l51_init(struct snd_soc_codec *codec)
{
	mv88fx_snd_debug("");

#ifdef CONFIG_MACH_DOVE_RD_AVNG
	/* Default Gain */
	snd_soc_update_bits(codec, CS42L51_REG_DAC_OUTPUT_CTRL, 0xE0, 0xE0);

#endif

	mv88fx_snd_ctrl_new(codec->card);

	return 0;
}

static struct snd_soc_dai_link mv88fx_dai[] = {
	{
	 .name = "CS42L51",
	 .stream_name = "CS42L51",
	 .cpu_dai = &mv88fx_i2s_dai,
	 .codec_dai = &cs42l51_dai,
	 .init = mv88fx_cs42l51_init,
	 },
};

static int mv88fx_probe(struct platform_device *pdev)
{
	return 0;
}

static int mv88fx_remove(struct platform_device *pdev)
{
	return 0;
}

static struct snd_soc_card mv_i2s = {
	.name = "mv_i2s",
	.platform = &mv88fx_soc_platform,
	.probe = mv88fx_probe,
	.remove = mv88fx_remove,
	/* CPU <--> Codec DAI links */
	.dai_link = mv88fx_dai,
	.num_links = ARRAY_SIZE(mv88fx_dai),
};

struct cs42l51_setup_data mv88fx_codec_setup_data = {
	.i2c_address = 0x4A,
};

static struct snd_soc_device mv88fx_snd_devdata = {
	.card = &mv_i2s,
	.codec_dev = &soc_codec_dev_cs42l51,
	.codec_data = &mv88fx_codec_setup_data,
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
		mv88fx_snd_error("");
		err = -ENXIO;
		goto error;
	}
	r = request_mem_region(r->start, SZ_16K, DRIVER_NAME);
	if (!r) {
		mv88fx_snd_error("");
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
	mv88fx_machine_data.base -= AUDIO_REG_BASE(mv88fx_machine_data.port);

	mv88fx_machine_data.irq = platform_get_irq(pdev, 0);
	if (mv88fx_machine_data.irq == NO_IRQ) {
		mv88fx_snd_error("");
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
	mv88fx_machine_data.snd_dev = platform_device_alloc("soc-audio", 1);
	if (!mv88fx_machine_data.snd_dev) {
		ret = -ENOMEM;
		goto error;
	}

	platform_set_drvdata(mv88fx_machine_data.snd_dev, &mv88fx_snd_devdata);
	mv88fx_snd_devdata.dev = &mv88fx_machine_data.snd_dev->dev;

	mv88fx_machine_data.snd_dev->dev.platform_data = &mv88fx_machine_data;

	ret = platform_device_add(mv88fx_machine_data.snd_dev);

	mv88fx_snd_debug("");

	return 0;
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
#ifdef CONFIG_ARCH_DOVE
	if (!machine_is_dove_db() && !machine_is_dove_db_z0())
		return -ENODEV;
#endif

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
MODULE_DESCRIPTION("ALSA Marvell SoC");
MODULE_LICENSE("GPL");
