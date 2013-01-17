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

#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/mbus.h>
#include <linux/irqreturn.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <asm/dma.h>
#include <linux/io.h>
#include <linux/scatterlist.h>
#include <asm/sizes.h>
#include <mvTypes.h>
#include <plat/i2s-orion.h>
#include "audio/mvAudioRegs.h"
#include "mv88fx-pcm.h"

#include "audio/mvAudio.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"

#ifdef CONFIG_MACH_DOVE_RD_AVNG
#include <linux/gpio.h>
#include <asm/mach-types.h>
#endif

struct mv88fx_snd_chip *mv88fx_pcm_snd_chip;
EXPORT_SYMBOL_GPL(mv88fx_pcm_snd_chip);

static struct snd_pcm_hardware mv88fx_pcm_hardware = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER | SNDRV_PCM_INFO_PAUSE),
	.formats = (SNDRV_PCM_FMTBIT_S16_LE |
		    SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE),
	.rates = (SNDRV_PCM_RATE_44100 |
		  SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000),
	.rate_min = 44100,
	.rate_max = 96000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = (16 * 1024 * 1024),
	.period_bytes_min = MV88FX_SND_MIN_PERIOD_BYTES,
	.period_bytes_max = MV88FX_SND_MAX_PERIOD_BYTES,
	.periods_min = MV88FX_SND_MIN_PERIODS,
	.periods_max = MV88FX_SND_MAX_PERIODS,
	.fifo_size = 0,

};

static void mv88fx_pcm_init_stream(struct mv88fx_snd_chip *chip,
				   struct mv88fx_snd_stream *stream,
				   int stream_id)
{
	memset(stream, 0, sizeof(*stream));
	if (stream_id == SNDRV_PCM_STREAM_PLAYBACK) {
		stream->dig_mode = 0;
		if (chip->pdata->i2s_play)
			stream->dig_mode |= I2S;
		if (chip->pdata->spdif_play)
			stream->dig_mode |= SPDIF;

		stream->mono_mode = MONO_BOTH;
		stream->stat_mem = 0;
		stream->clock_src = DCO_CLOCK;
	} else {
		if (chip->pdata->i2s_rec)
			stream->dig_mode = I2S;
		else if (chip->pdata->spdif_rec)
			stream->dig_mode = SPDIF;

		stream->mono_mode = MONO_LEFT;
		stream->stat_mem = 0;
		stream->clock_src = DCO_CLOCK;
	}
}



static int mv88fx_find_dram_cs(struct mbus_dram_target_info *dram,
			       u32 base, u32 size)
{
	int i;

	mv88fx_snd_debug("base=0x%x size=%u dram=%p", base, size, dram);

	for (i = 0; i < dram->num_cs; i++) {
		struct mbus_dram_window *cs = dram->cs + i;
		/* check if we fit into one memory window only */
		if ((base >= cs->base) &&
		    ((base + size) <= cs->base + cs->size))
			return i;

	}

	mv88fx_snd_error("");
	return -1;
}
static int mv88fx_config_dma_window(struct mv88fx_snd_chip *chip,
				    int dma, struct mbus_dram_target_info *dram)
{
	struct mbus_dram_window *cs;
	struct snd_pcm_substream *substream = chip->pcm->streams[dma].substream;

	int win;

#define WINDOW_CTRL(dma)		(0xA0C - ((dma) << 3))
#define WINDOW_BASE(dma)		(0xA08 - ((dma) << 3))

	mv88fx_snd_debug("");
	/*
	 * we have one window for each dma (playback dma and recording),
	 * confiure it to point to the dram window where the buffer falls in
	 */
	win = mv88fx_find_dram_cs(dram,
				  (u32) substream->dma_buffer.addr,
				  (u32) MV88FX_SND_MAX_PERIODS *
				  MV88FX_SND_MAX_PERIOD_BYTES);
	if (win < 0)
		return -1;

	cs = dram->cs + win;
	writel(((cs->size - 1) & 0xffff0000) |
	       (cs->mbus_attr << 8) |
	       (dram->mbus_dram_target_id << 4) | 1,
	       chip->base + AUDIO_REG_BASE(chip->port) + WINDOW_CTRL(dma));
	writel(cs->base,
	       chip->base + AUDIO_REG_BASE(chip->port) + WINDOW_BASE(dma));


#undef WINDOW_CTRL
#undef WINDOW_BASE

	return win;
}
static int mv88fx_conf_mbus_windows(struct mv88fx_snd_chip *chip,
				    struct mbus_dram_target_info *dram)
{
	mv88fx_snd_debug("");
	/*
	 * we have one window for playback and one for recording
	 */
	if (mv88fx_config_dma_window(chip, SNDRV_PCM_STREAM_PLAYBACK, dram) < 0)
		return -1;

	if (mv88fx_config_dma_window(chip, SNDRV_PCM_STREAM_CAPTURE, dram) < 0)
		return -1;
	return 0;
}


static irqreturn_t mv88fx_pcm_dma_interrupt(int irq, void *dev_id)
{
	struct mv88fx_snd_chip *chip = dev_id;
	struct snd_pcm_substream *play_stream =
	    chip->pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
	struct snd_pcm_substream *capture_stream =
	    chip->pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
	unsigned int status, mask;

	mv88fx_snd_debug("");

	spin_lock(&chip->reg_lock);

	/* read the active interrupt */
	mask = mv88fx_snd_readl(chip->base,
			MV_AUDIO_INT_MASK_REG(chip->port));
	status = mv88fx_snd_readl(chip->base,
			MV_AUDIO_INT_CAUSE_REG(chip->port)) & mask;

	do {

		if (status & ~(AICR_RECORD_BYTES_INT | AICR_PLAY_BYTES_INT)) {
			spin_unlock(&chip->reg_lock);
			snd_BUG();/* FIXME: should enable error interrupts */
			return IRQ_NONE;
		}

		/* acknowledge interrupt */
		mv88fx_snd_writel(chip->base,
			MV_AUDIO_INT_CAUSE_REG(chip->port), status);

		/* This is record event */
		if (status & AICR_RECORD_BYTES_INT) {
			mv88fx_snd_debug("capture");
			spin_unlock(&chip->reg_lock);
			if (capture_stream)
				snd_pcm_period_elapsed(capture_stream);
			spin_lock(&chip->reg_lock);
		}
		/* This is play event */
		if (status & AICR_PLAY_BYTES_INT) {
			mv88fx_snd_debug("playback");
			spin_unlock(&chip->reg_lock);
			if (play_stream)
				snd_pcm_period_elapsed(play_stream);
			spin_lock(&chip->reg_lock);

		}

		/* read the active interrupt */
		mask = mv88fx_snd_readl(chip->base,
				MV_AUDIO_INT_MASK_REG(chip->port));
		status = mv88fx_snd_readl(chip->base,
				MV_AUDIO_INT_CAUSE_REG(chip->port)) & mask;

	} while (status);

	spin_unlock(&chip->reg_lock);
	return IRQ_HANDLED;

}

static int mv88fx_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;

	mv88fx_snd_debug("substream=%p", substream);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);
	return err;

}

static int mv88fx_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_dma_buffer *buf = runtime->dma_buffer_p;

	mv88fx_snd_debug("");

	if (runtime->dma_area == NULL)
		return 0;

	if (buf != &substream->dma_buffer)
		kfree(runtime->dma_buffer_p);

	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int mv88fx_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mv88fx_snd_stream *audio_stream = runtime->private_data;
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();

	mv88fx_snd_debug("substream=%p chip=%p,chip->base=%p audio_stream=%p",
			 substream, chip, chip->base, audio_stream);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		if ((audio_stream->dig_mode == I2S) &&
		    (chip->pcm_mode == NON_PCM)) {
			mv88fx_snd_error("");
			return -1;
		}

	return 0;
}

static int mv88fx_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	mv88fx_snd_debug("");
	return 0;
}

static snd_pcm_uframes_t mv88fx_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_pcm_uframes_t offset;
	ssize_t size;

	mv88fx_snd_debug("");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		size = (ssize_t) mv88fx_snd_readl(chip->base,
		       MV_AUDIO_PLAYBACK_BUFF_BYTE_CNTR_REG(chip->port));
		offset = bytes_to_frames(substream->runtime, size);
	} else {
		size = (ssize_t) mv88fx_snd_readl(chip->base,
			MV_AUDIO_RECORD_BUF_BYTE_CNTR_REG(chip->port));
		offset = bytes_to_frames(substream->runtime, size);
	}
	if (offset >= runtime->buffer_size)
		offset = 0;

	return offset;
}

static int mv88fx_pcm_open(struct snd_pcm_substream *substream)
{
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;

	mv88fx_snd_debug("");

	runtime->private_data = &chip->stream[substream->stream];

	snd_soc_set_runtime_hwparams(substream, &mv88fx_pcm_hardware);

	err = snd_pcm_hw_constraint_minmax(runtime,
					   SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
					   chip->burst * 2,
					   AUDIO_REG_TO_SIZE(APBBCR_SIZE_MAX));
	if (err < 0)
		return err;

	err = snd_pcm_hw_constraint_step(runtime, 0,
					 SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
					 chip->burst);
	if (err < 0)
		return err;

	err = snd_pcm_hw_constraint_step(runtime, 0,
					 SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
					 chip->burst);
	if (err < 0)
		return err;

	err = snd_pcm_hw_constraint_minmax(runtime,
					   SNDRV_PCM_HW_PARAM_PERIODS,
					   MV88FX_SND_MIN_PERIODS,
					   MV88FX_SND_MAX_PERIODS);
	if (err < 0)
		return err;

	err = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (err < 0)
		return err;

	return 0;
}

static void mv88fx_pcm_reset_spdif_status(struct mv88fx_snd_chip *chip,
					  int usr_bits)
{
	int i;

	mv88fx_snd_debug("");

	for (i = 0; i < 4; i++) {
		chip->stream[SNDRV_PCM_STREAM_PLAYBACK].spdif_status[i] = 0;

		mv88fx_snd_writel(chip->base,
				  MV_AUDIO_SPDIF_PLAY_CH_STATUS_LEFT_REG(chip->
									 port,
									 i), 0);
		mv88fx_snd_writel(chip->base,
				  MV_AUDIO_SPDIF_PLAY_CH_STATUS_RIGHT_REG(chip->
									  port,
									  i),
				  0);
		if (usr_bits) {
			mv88fx_snd_writel(chip->base,
					  MV_AUDIO_SPDIF_PLAY_USR_BITS_LEFT_REG
					  (chip->port, i), 0);
			mv88fx_snd_writel(chip->base,
					  MV_AUDIO_SPDIF_PLAY_USR_BITS_RIGHT_REG
					  (chip->port, i), 0);
		}
	}

}

static int mv88fx_pcm_close(struct snd_pcm_substream *substream)
{
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();

	mv88fx_snd_debug("");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		chip->pcm_mode = PCM;
		mv88fx_pcm_reset_spdif_status(chip, 0);
	}
	return 0;
}

static int mv88fx_pcm_mmap(struct snd_pcm_substream *substream,
			   struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	mv88fx_snd_debug("");

	return dma_mmap_coherent(NULL, vma, runtime->dma_area,
				 runtime->dma_addr, runtime->dma_bytes);
}

struct snd_pcm_ops mv88fx_pcm_ops = {
	.open = mv88fx_pcm_open,
	.close = mv88fx_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = mv88fx_pcm_hw_params,
	.hw_free = mv88fx_pcm_hw_free,
	.prepare = mv88fx_pcm_prepare,
	.trigger = mv88fx_pcm_trigger,
	.pointer = mv88fx_pcm_pointer,
	.mmap = mv88fx_pcm_mmap,
};

static int mv88fx_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = MV88FX_SND_MAX_PERIODS * MV88FX_SND_MAX_PERIOD_BYTES;

	mv88fx_snd_debug("");

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_coherent(pcm->card->dev,
				       size, &buf->addr, GFP_KERNEL);
	if (!buf->area) {
		mv88fx_snd_error("unable to allocate");
		return -ENOMEM;
	}
	buf->bytes = size;
	return 0;
}

static struct mv88fx_snd_chip *mv88fx_pcm_init(struct snd_pcm *pcm,
					       struct platform_device *pdev,
					       struct mv88fx_snd_machine_data
					       *machine_data)
{
	struct mv88fx_snd_chip *chip;

	mv88fx_snd_debug("");
	chip = mv88fx_pcm_snd_chip =
	    kzalloc(sizeof(struct mv88fx_snd_chip), GFP_KERNEL);
	if (!chip)
		return NULL;

	chip->base = machine_data->base;

	spin_lock_init(&chip->reg_lock);
	chip->dev = &pdev->dev;
	chip->port = machine_data->port;
	chip->pcm = pcm;
	chip->pdata = machine_data->pdata;
	chip->ch_stat_valid = 1;
	chip->burst = 128;
	chip->loopback = 0;
	chip->dco_ctrl_offst = 0x800;
	chip->irq = machine_data->irq;

	if (request_irq(machine_data->irq, mv88fx_pcm_dma_interrupt,
		0, DRIVER_NAME, (void *)chip)) {
		mv88fx_snd_error("");
		goto error;
	}
	if (chip->pdata->dram != NULL)
		if (mv88fx_conf_mbus_windows(chip, chip->pdata->dram)) {
			mv88fx_snd_error("");
			goto error;

		}


	mv88fx_pcm_init_stream(chip, &chip->stream[SNDRV_PCM_STREAM_PLAYBACK],
			       SNDRV_PCM_STREAM_PLAYBACK);
	mv88fx_pcm_init_stream(chip, &chip->stream[SNDRV_PCM_STREAM_CAPTURE],
			       SNDRV_PCM_STREAM_CAPTURE);
	chip->pcm_mode = PCM;

	mv88fx_pcm_reset_spdif_status(chip, 1);

	return chip;
error:
	free_irq(chip->irq, chip);
	kfree(chip);
	mv88fx_pcm_snd_chip = NULL;
	return NULL;
}

static int mv88fx_pcm_new(struct snd_card *card, struct snd_soc_dai *dai,
			  struct snd_pcm *pcm)
{
	static u64 mv88fx_pcm_dmamask = DMA_BIT_MASK(32);
	int ret = 0;
	struct platform_device *pdev = to_platform_device(card->dev);
	struct mv88fx_snd_machine_data *machine_data = pdev->dev.platform_data;
	struct mv88fx_snd_chip *chip;

	mv88fx_snd_debug("");

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &mv88fx_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (dai->playback.channels_min) {
		ret = mv88fx_pcm_preallocate_dma_buffer(pcm,
					SNDRV_PCM_STREAM_PLAYBACK);
		if (ret) {
			mv88fx_snd_error("");
			goto out;
		}
	}

	if (dai->capture.channels_min) {
		ret = mv88fx_pcm_preallocate_dma_buffer(pcm,
					SNDRV_PCM_STREAM_CAPTURE);
		if (ret) {
			mv88fx_snd_error("");
			goto out;
		}
	}
	chip = mv88fx_pcm_init(pcm, pdev, machine_data);
	if (!chip)
		return -ENOMEM;

	return 0;
out:
	return ret;
}

static void mv88fx_pcm_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	mv88fx_snd_debug("");

	for (stream = 0; stream < ARRAY_SIZE(pcm->streams); stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;
		dma_free_coherent(pcm->card->dev,
				  buf->bytes, buf->area, buf->addr);
		buf->area = NULL;
		buf->addr = 0;
	}
	free_irq(mv88fx_pcm_snd_chip->irq, mv88fx_pcm_snd_chip);
	kfree(mv88fx_pcm_snd_chip);
	mv88fx_pcm_snd_chip = NULL;
}


static int mv88fx_pcm_suspend(struct snd_soc_dai *cpu_dai)
{
	mv88fx_snd_debug("");


	return 0;
}



static int mv88fx_pcm_resume(struct snd_soc_dai *cpu_dai)
{
	mv88fx_snd_debug("");


	return 0;
}



struct snd_soc_platform mv88fx_soc_platform = {
	.name = DRIVER_NAME,
	.pcm_new = mv88fx_pcm_new,
	.pcm_free = mv88fx_pcm_free,
	.suspend  = mv88fx_pcm_suspend,
	.resume   = mv88fx_pcm_resume,
	.pcm_ops = &mv88fx_pcm_ops,
};
EXPORT_SYMBOL_GPL(mv88fx_soc_platform);


static int __init mv88fx_soc_platform_init(void)
{
	return snd_soc_register_platform(&mv88fx_soc_platform);
}
module_init(mv88fx_soc_platform_init);

static void __exit mv88fx_soc_platform_exit(void)
{
	snd_soc_unregister_platform(&mv88fx_soc_platform);
}
module_exit(mv88fx_soc_platform_exit);




MODULE_AUTHOR("Yuval Elmaliah <eyuval@marvell.com>");
MODULE_DESCRIPTION("mv88fx ASoc Platform driver");
MODULE_LICENSE("GPL");
