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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <asm/sizes.h>
#include <linux/io.h>
#include <plat/i2s-orion.h>
#include <asm/dma.h>
#include <linux/scatterlist.h>
#include <asm/sizes.h>
#include <mvTypes.h>
#include "mv88fx-pcm.h"
#include "audio/mvAudioRegs.h"
#include "audio/mvAudio.h"
#include "ctrlEnv/sys/mvSysAudio.h"

#ifdef CONFIG_MACH_DOVE_RD_AVNG
#include <linux/gpio.h>
#include <asm/mach-types.h>
#endif


struct mv88fx_i2s_info {
	int		port;
	void __iomem	*base;	/* base address of the host */
	u32		irq_mask;
	u32		playback_cntrl_reg;
	u32		capture_cntrl_reg;
};

static struct mv88fx_i2s_info mv88fx_i2s_info;


static int mv88fx_i2_hw_init(void)
{
	mv88fx_snd_debug("");

	mv88fx_snd_writel(mv88fx_i2s_info.base,
		MV_AUDIO_INT_CAUSE_REG(mv88fx_i2s_info.port), 0xffffffff);
	mv88fx_snd_writel(mv88fx_i2s_info.base,
		MV_AUDIO_INT_MASK_REG(mv88fx_i2s_info.port), 0);
	mv88fx_snd_writel(mv88fx_i2s_info.base,
		MV_AUDIO_SPDIF_REC_INT_CAUSE_MASK_REG(mv88fx_i2s_info.port), 0);

//	if (MV_OK != mvAudioInit(mv88fx_i2s_info.port))
//		return -EIO;
	mvAudioInit();

	/* Disable all playback/recording */
	mv88fx_snd_bitreset(mv88fx_i2s_info.base,
		MV_AUDIO_PLAYBACK_CTRL_REG(mv88fx_i2s_info.port),
		(APCR_PLAY_I2S_ENABLE_MASK | APCR_PLAY_SPDIF_ENABLE_MASK));

	mv88fx_snd_bitreset(mv88fx_i2s_info.base,
		MV_AUDIO_RECORD_CTRL_REG(mv88fx_i2s_info.port),
		(ARCR_RECORD_SPDIF_EN_MASK | ARCR_RECORD_I2S_EN_MASK));

	if (MV_OK != mvSPDIFRecordTclockSet(mv88fx_i2s_info.port)) {
		mv88fx_snd_error("mvSPDIFRecordTclockSet failed");
		return -ENOMEM;
	}
	return 0;
}


static int mv88fx_i2s_snd_hw_playback_set(struct mv88fx_snd_chip *chip,
					  struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mv88fx_snd_stream *audio_stream = runtime->private_data;
	MV_AUDIO_PLAYBACK_CTRL pcm_play_ctrl;
	MV_I2S_PLAYBACK_CTRL i2s_play_ctrl;
	MV_SPDIF_PLAYBACK_CTRL spdif_play_ctrl;
	MV_AUDIO_FREQ_DATA dco_ctrl;

	mv88fx_snd_debug("chip=%p chip->base=%p", chip, chip->base);

	memset(&pcm_play_ctrl, 0, sizeof(pcm_play_ctrl));
	memset(&dco_ctrl, 0, sizeof(dco_ctrl));
	memset(&i2s_play_ctrl, 0, sizeof(i2s_play_ctrl));
	memset(&spdif_play_ctrl, 0, sizeof(spdif_play_ctrl));

	dco_ctrl.offset = chip->dco_ctrl_offst;

	mv88fx_snd_debug("rate: %u	", runtime->rate);
	switch (runtime->rate) {
	case 44100:
		dco_ctrl.baseFreq = AUDIO_FREQ_44_1KH;
		break;
	case 48000:
		dco_ctrl.baseFreq = AUDIO_FREQ_48KH;
		break;
	case 96000:
		dco_ctrl.baseFreq = AUDIO_FREQ_96KH;
		break;
	default:
		mv88fx_snd_error("Requested rate %d is not supported",
				 runtime->rate);
		return -1;
	}

	pcm_play_ctrl.burst = (chip->burst == 128) ? AUDIO_128BYTE_BURST :
	    AUDIO_32BYTE_BURST;

	pcm_play_ctrl.loopBack = chip->loopback;

	if (mv88fx_pcm_is_stereo(runtime)) {
		pcm_play_ctrl.monoMode = AUDIO_PLAY_MONO_OFF;
	} else {

		switch (audio_stream->mono_mode) {
		case MONO_LEFT:
			pcm_play_ctrl.monoMode = AUDIO_PLAY_LEFT_MONO;
			break;
		case MONO_RIGHT:
			pcm_play_ctrl.monoMode = AUDIO_PLAY_RIGHT_MONO;
			break;
		case MONO_BOTH:
		default:
			pcm_play_ctrl.monoMode = AUDIO_PLAY_BOTH_MONO;
			break;
		}
	}

	if (runtime->format == SNDRV_PCM_FORMAT_S16_LE) {
		pcm_play_ctrl.sampleSize = SAMPLE_16BIT;
		i2s_play_ctrl.sampleSize = SAMPLE_16BIT;
	} else if (runtime->format == SNDRV_PCM_FORMAT_S24_LE) {
		pcm_play_ctrl.sampleSize = SAMPLE_24BIT;
		i2s_play_ctrl.sampleSize = SAMPLE_24BIT;

	} else if (runtime->format == SNDRV_PCM_FORMAT_S32_LE) {
		pcm_play_ctrl.sampleSize = SAMPLE_32BIT;
		i2s_play_ctrl.sampleSize = SAMPLE_32BIT;
	} else {
		mv88fx_snd_error("Requested format %d is not supported",
				 runtime->format);
		return -1;
	}

	/* buffer and period sizes in frame */
	pcm_play_ctrl.bufferPhyBase = runtime->dma_addr;
	pcm_play_ctrl.bufferSize =
	    frames_to_bytes(runtime, runtime->buffer_size);
	pcm_play_ctrl.intByteCount =
	    frames_to_bytes(runtime, runtime->period_size);

	/* I2S playback streem stuff */
	/*i2s_play_ctrl.sampleSize = pcm_play_ctrl.sampleSize; */
	i2s_play_ctrl.justification = I2S_JUSTIFIED;
	i2s_play_ctrl.sendLastFrame = 0;

	spdif_play_ctrl.nonPcm = MV_FALSE;

	spdif_play_ctrl.validity = chip->ch_stat_valid;

	if (audio_stream->stat_mem) {
		spdif_play_ctrl.userBitsFromMemory = MV_TRUE;
		spdif_play_ctrl.validityFromMemory = MV_TRUE;
		spdif_play_ctrl.blockStartInternally = MV_FALSE;
	} else {
		spdif_play_ctrl.userBitsFromMemory = MV_FALSE;
		spdif_play_ctrl.validityFromMemory = MV_FALSE;
		spdif_play_ctrl.blockStartInternally = MV_TRUE;
	}

	mv88fx_snd_debug("");
	/* If this is non-PCM sound, mute I2S channel */
	spin_lock_irq(&chip->reg_lock);

	mv88fx_snd_debug("chip=%p chip->base=%p port=%d", chip, chip->base,
			 chip->port);

	if (!(mv88fx_snd_readl(chip->base,
		MV_AUDIO_PLAYBACK_CTRL_REG(chip->port)) &
		(APCR_PLAY_I2S_ENABLE_MASK | APCR_PLAY_SPDIF_ENABLE_MASK))) {

		mv88fx_snd_debug("");

		if (MV_OK != mvAudioDCOCtrlSet(chip->port, &dco_ctrl)) {
			mv88fx_snd_error
			    ("Failed to initialize DCO clock control.");
			return -1;
			mv88fx_snd_debug("");

		}
	}

	mv88fx_snd_debug("");

	if (audio_stream->clock_src == DCO_CLOCK)
		while ((mv88fx_snd_readl(chip->base,
			 MV_AUDIO_SPCR_DCO_STATUS_REG(chip->port)) &
			ASDSR_DCO_LOCK_MASK) == 0)
			;
	else if (audio_stream->clock_src == SPCR_CLOCK)
		while ((mv88fx_snd_readl(chip->base,
			 MV_AUDIO_SPCR_DCO_STATUS_REG(chip->port)) &
			ASDSR_SPCR_LOCK_MASK) == 0)
			;

	mv88fx_snd_debug("");

	if (MV_OK != mvAudioPlaybackControlSet(chip->port, &pcm_play_ctrl)) {
		mv88fx_snd_error("Failed to initialize PCM playback control.");
		return -1;
	}
	mv88fx_snd_debug("");

	if (MV_OK != mvI2SPlaybackCtrlSet(chip->port, &i2s_play_ctrl)) {
		mv88fx_snd_error("Failed to initialize I2S playback control.");
		return -1;
	}
	mv88fx_snd_debug("");

	mvSPDIFPlaybackCtrlSet(chip->port, &spdif_play_ctrl);

	mv88fx_snd_debug("");

	spin_unlock_irq(&chip->reg_lock);

	return 0;
}

static int mv88fx_i2s_snd_hw_capture_set(struct mv88fx_snd_chip *chip,
					 struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mv88fx_snd_stream *audio_stream = runtime->private_data;
	MV_AUDIO_RECORD_CTRL pcm_rec_ctrl;
	MV_I2S_RECORD_CTRL i2s_rec_ctrl;
	MV_AUDIO_FREQ_DATA dco_ctrl;

#ifdef CONFIG_MACH_DOVE_RD_AVNG
	int phoneInDetected;
#endif

	mv88fx_snd_debug("chip=%p chip->base=%p", chip, chip->base);

	dco_ctrl.offset = chip->dco_ctrl_offst;

	switch (runtime->rate) {
	case 44100:
		dco_ctrl.baseFreq = AUDIO_FREQ_44_1KH;
		break;
	case 48000:
		dco_ctrl.baseFreq = AUDIO_FREQ_48KH;
		break;
	case 96000:
		dco_ctrl.baseFreq = AUDIO_FREQ_96KH;
		break;
	default:
		mv88fx_snd_debug("ERROR");
		mv88fx_snd_error("Requested rate %d is not supported",
				 runtime->rate);
		return -1;
	}

	pcm_rec_ctrl.burst = (chip->burst == 128) ? AUDIO_128BYTE_BURST :
	    AUDIO_32BYTE_BURST;

	if (runtime->format == SNDRV_PCM_FORMAT_S16_LE) {
		pcm_rec_ctrl.sampleSize = SAMPLE_16BIT;
	} else if (runtime->format == SNDRV_PCM_FORMAT_S24_LE) {
		pcm_rec_ctrl.sampleSize = SAMPLE_24BIT;
	} else if (runtime->format == SNDRV_PCM_FORMAT_S32_LE) {
		pcm_rec_ctrl.sampleSize = SAMPLE_32BIT;
	} else {
		mv88fx_snd_error("Requested format %d is not supported",
				 runtime->format);
		return -1;
	}

	pcm_rec_ctrl.mono =
	    (mv88fx_pcm_is_stereo(runtime)) ? MV_FALSE : MV_TRUE;

	if (pcm_rec_ctrl.mono) {
		switch (audio_stream->mono_mode) {
		case MONO_LEFT:
			pcm_rec_ctrl.monoChannel = AUDIO_REC_LEFT_MONO;
			break;
		default:
		case MONO_RIGHT:
			pcm_rec_ctrl.monoChannel = AUDIO_REC_RIGHT_MONO;
			break;
		}

	} else
		pcm_rec_ctrl.monoChannel = AUDIO_REC_LEFT_MONO;

	pcm_rec_ctrl.bufferPhyBase = runtime->dma_addr;
	pcm_rec_ctrl.bufferSize =
	    frames_to_bytes(runtime, runtime->buffer_size);
	pcm_rec_ctrl.intByteCount =
	    frames_to_bytes(runtime, runtime->period_size);

	/* I2S record streem stuff */
	i2s_rec_ctrl.sample = pcm_rec_ctrl.sampleSize;
	i2s_rec_ctrl.justf = I2S_JUSTIFIED;

	spin_lock_irq(&chip->reg_lock);

	/* set clock only if record is not enabled */
	if (!(mv88fx_snd_readl(chip->base,
		MV_AUDIO_RECORD_CTRL_REG(chip->port)) &
		(ARCR_RECORD_SPDIF_EN_MASK | ARCR_RECORD_I2S_EN_MASK))) {

		if (MV_OK != mvAudioDCOCtrlSet(chip->port, &dco_ctrl)) {
			mv88fx_snd_debug("ERROR");
			mv88fx_snd_error
			    ("Failed to initialize DCO clock control.");
			return -1;
		}
	}
	mv88fx_snd_debug("");
	if (MV_OK != mvAudioRecordControlSet(chip->port, &pcm_rec_ctrl)) {
		mv88fx_snd_error("Failed to initialize PCM record control.");
		return -1;
	}

	if (MV_OK != mvI2SRecordCntrlSet(chip->port, &i2s_rec_ctrl)) {
		mv88fx_snd_error("Failed to initialize I2S record control.");
		return -1;
	}
	mv88fx_snd_debug("");

#ifdef CONFIG_MACH_DOVE_RD_AVNG
	if (machine_is_dove_rd_avng()) {
		phoneInDetected = gpio_get_value(53);

		if (phoneInDetected < 0)
			mv88fx_snd_error("Failed to detect phone-in.");
		else {
			int input_type;
			mv88fx_snd_error("detect phone-in.");
			if (!phoneInDetected)
				input_type = 2;	/* external MIC */
			else
				input_type = 1;	/* internal MIC */
		}
	}
#endif
	mv88fx_snd_debug("");

	spin_unlock_irq(&chip->reg_lock);

	mv88fx_snd_debug("");

	return 0;

}

static int mv88fx_i2s_capture_trigger(struct snd_pcm_substream *substream,
				      int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mv88fx_snd_stream *audio_stream = runtime->private_data;
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();
	int result = 0;

	mv88fx_snd_debug("substream=%p cmd=%d audio_stream=%p", substream, cmd,
			 audio_stream);

	spin_lock(&chip->reg_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		/* FIXME: should check if busy before */
		mv88fx_snd_debug("");
		/* make sure the dma in pause state */
		mv88fx_snd_bitset(chip->base,
			MV_AUDIO_RECORD_CTRL_REG(chip->port),
			ARCR_RECORD_PAUSE_MASK);

		/* enable interrupt */
		mv88fx_snd_bitset(chip->base, MV_AUDIO_INT_MASK_REG(chip->port),
				  AICR_RECORD_BYTES_INT);

		/* enable dma */
		mv88fx_snd_debug("dig_mode=%x", audio_stream->dig_mode);
		if (audio_stream->dig_mode & I2S) {
			mv88fx_snd_debug("enable dma");
			mv88fx_snd_bitset(chip->base,
					  MV_AUDIO_RECORD_CTRL_REG(chip->port),
					  ARCR_RECORD_I2S_EN_MASK);
		}

		if (audio_stream->dig_mode & SPDIF) {
			mv88fx_snd_debug("enable spdif dma");
			mv88fx_snd_bitset(chip->base,
					  MV_AUDIO_RECORD_CTRL_REG(chip->port),
					  ARCR_RECORD_SPDIF_EN_MASK);
		}

		/* start dma */
		mv88fx_snd_bitreset(chip->base,
			MV_AUDIO_RECORD_CTRL_REG(chip->port),
			ARCR_RECORD_PAUSE_MASK);

		break;
	case SNDRV_PCM_TRIGGER_STOP:

		/* make sure the dma in pause state */
		mv88fx_snd_bitset(chip->base,
			MV_AUDIO_RECORD_CTRL_REG(chip->port),
			ARCR_RECORD_PAUSE_MASK);

		/* disable interrupt */
		mv88fx_snd_bitreset(chip->base,
			MV_AUDIO_INT_MASK_REG(chip->port),
			AICR_RECORD_BYTES_INT);

		/* always stop both I2S and SPDIF */
		mv88fx_snd_bitreset(chip->base,
			MV_AUDIO_RECORD_CTRL_REG(chip->port),
			(ARCR_RECORD_I2S_EN_MASK | ARCR_RECORD_SPDIF_EN_MASK));

		/* FIXME: should check if busy after */

		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		mv88fx_snd_bitset(chip->base,
			MV_AUDIO_RECORD_CTRL_REG(chip->port),
			ARCR_RECORD_PAUSE_MASK);
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		mv88fx_snd_bitreset(chip->base,
			MV_AUDIO_RECORD_CTRL_REG(chip->port),
			ARCR_RECORD_PAUSE_MASK);

		break;
	default:
		result = -EINVAL;
		break;
	}
	spin_unlock(&chip->reg_lock);
	mv88fx_snd_debug("result=%d", result);
	return result;
}

static int mv88fx_i2s_playback_trigger(struct snd_pcm_substream *substream,
				       int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mv88fx_snd_stream *audio_stream = runtime->private_data;
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();
	int result = 0;

	mv88fx_snd_debug("substream=%p cmd=%d audio_stream=%p", substream, cmd,
			 audio_stream);

	spin_lock(&chip->reg_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:

		mv88fx_snd_debug("");
		/* enable interrupt */
		mv88fx_snd_bitset(chip->base, MV_AUDIO_INT_MASK_REG(chip->port),
				  AICR_PLAY_BYTES_INT);

		/* make sure the dma in pause state */
		mv88fx_snd_bitset(chip->base,
			MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
			APCR_PLAY_PAUSE_MASK);

		/* enable dma */
		if ((audio_stream->dig_mode & I2S) && (chip->pcm_mode == PCM))
			mv88fx_snd_bitset(chip->base,
				MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
				APCR_PLAY_I2S_ENABLE_MASK);

		if (audio_stream->dig_mode & SPDIF)
			mv88fx_snd_bitset(chip->base,
				MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
				APCR_PLAY_SPDIF_ENABLE_MASK);

		/* start dma */
		mv88fx_snd_bitreset(chip->base,
				    MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
				    APCR_PLAY_PAUSE_MASK);

		break;
	case SNDRV_PCM_TRIGGER_STOP:

		mv88fx_snd_debug("");

		/* disable interrupt */
		mv88fx_snd_bitreset(chip->base,
			MV_AUDIO_INT_MASK_REG(chip->port),
			AICR_PLAY_BYTES_INT);

		/* make sure the dma in pause state */
		mv88fx_snd_bitset(chip->base,
			MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
			APCR_PLAY_PAUSE_MASK);

		/* always stop both I2S and SPDIF */
		mv88fx_snd_bitreset(chip->base,
				    MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
				    (APCR_PLAY_I2S_ENABLE_MASK |
				     APCR_PLAY_SPDIF_ENABLE_MASK));

		/* check if busy twice */
		while (mv88fx_snd_readl(chip->base,
			MV_AUDIO_PLAYBACK_CTRL_REG(chip->port)) &
		       APCR_PLAY_BUSY_MASK)
			;

		while (mv88fx_snd_readl(chip->base,
			MV_AUDIO_PLAYBACK_CTRL_REG(chip->port)) &
		       APCR_PLAY_BUSY_MASK)
			;

		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		mv88fx_snd_debug("");

		mv88fx_snd_bitset(chip->base,
			MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
			APCR_PLAY_PAUSE_MASK);
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		mv88fx_snd_debug("");

		mv88fx_snd_bitreset(chip->base,
				    MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
				    APCR_PLAY_PAUSE_MASK);

		break;
	default:
		result = -EINVAL;
		break;
	}
	spin_unlock(&chip->reg_lock);
	mv88fx_snd_debug("result=%d", result);
	return result;
}

static int mv88fx_i2s_startup(struct snd_pcm_substream *stream,
				struct snd_soc_dai *dai)
{
	mv88fx_snd_debug("");

	return 0;
}

static void mv88fx_i2s_shutdown(struct snd_pcm_substream *stream,
				struct snd_soc_dai *dai)
{
	mv88fx_snd_debug("");
}

static int mv88fx_i2s_hw_params(struct snd_pcm_substream *stream,
				struct snd_pcm_hw_params *hw_params,
				struct snd_soc_dai *dai)
{
	mv88fx_snd_debug("");

	return 0;
}

static int mv88fx_i2s_hw_free(struct snd_pcm_substream *stream,
				struct snd_soc_dai *dai)
{
	mv88fx_snd_debug("");

	return 0;
}

static int mv88fx_i2s_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();
	mv88fx_snd_debug("");

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return mv88fx_i2s_snd_hw_capture_set(chip, substream);
	else
		return mv88fx_i2s_snd_hw_playback_set(chip, substream);

}

static int mv88fx_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
				struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		return mv88fx_i2s_capture_trigger(substream, cmd);
	else
		return mv88fx_i2s_playback_trigger(substream, cmd);
}

static int mv88fx_i2s_set_sysclk(struct snd_soc_dai *codec_dai,
				 int clk_id, unsigned int freq, int dir)
{
	mv88fx_snd_debug("");

	return 0;
}

static int mv88fx_i2s_set_fmt(struct snd_soc_dai *codec_dai,
			      unsigned int fmt)
{
	mv88fx_snd_debug("");

	return 0;
}

static int mv88fx_i2s_dai_probe(struct platform_device *pdev,
				struct snd_soc_dai *dai)
{
	struct mv88fx_snd_machine_data *machine_data = pdev->dev.platform_data;

	mv88fx_snd_debug("");

	mv88fx_i2s_info.base = machine_data->base;
	mv88fx_i2s_info.port = machine_data->port;


	mv88fx_i2_hw_init();


	return 0;
}

static void mv88fx_i2s_dai_remove(struct platform_device *pdev,
				struct snd_soc_dai *dai)
{
	mv88fx_snd_debug("");

}


static int mv88fx_i2s_suspend(struct snd_soc_dai *cpu_dai)
{
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();

	mv88fx_snd_debug("");

	if (!cpu_dai->active)
		return 0;


	spin_lock(&chip->reg_lock);

	/* save registers */
	mv88fx_i2s_info.irq_mask = mv88fx_snd_readl(chip->base,
			MV_AUDIO_INT_MASK_REG(chip->port));
	mv88fx_i2s_info.capture_cntrl_reg = mv88fx_snd_readl(chip->base,
			MV_AUDIO_RECORD_CTRL_REG(chip->port));
	mv88fx_i2s_info.playback_cntrl_reg = mv88fx_snd_readl(chip->base,
			MV_AUDIO_PLAYBACK_CTRL_REG(chip->port));

	/* clear all interrupts */
	mv88fx_snd_writel(chip->base,
		MV_AUDIO_INT_CAUSE_REG(chip->port), 0xffffffff);
	/* disable all interrupts */
	mv88fx_snd_writel(chip->base,
		MV_AUDIO_INT_MASK_REG(chip->port), 0);
	/* pause dma */
	mv88fx_snd_bitset(chip->base,
		MV_AUDIO_RECORD_CTRL_REG(chip->port), ARCR_RECORD_PAUSE_MASK);
	mv88fx_snd_bitset(chip->base,
		MV_AUDIO_PLAYBACK_CTRL_REG(chip->port), APCR_PLAY_PAUSE_MASK);


	spin_unlock(&chip->reg_lock);

	return 0;
}



static int mv88fx_i2s_resume(struct snd_soc_dai *cpu_dai)
{
	struct mv88fx_snd_chip *chip = mv88fx_pcm_get_chip();

	mv88fx_snd_debug("");

	if (!cpu_dai->active)
		return 0;

	mv88fx_i2_hw_init();

	spin_lock(&chip->reg_lock);

	mv88fx_snd_writel(chip->base,
		MV_AUDIO_INT_CAUSE_REG(chip->port), 0xffffffff);
	/* restore registers */
	mv88fx_snd_writel(chip->base,
		MV_AUDIO_RECORD_CTRL_REG(chip->port),
		mv88fx_i2s_info.capture_cntrl_reg);
	mv88fx_snd_writel(chip->base,
		MV_AUDIO_PLAYBACK_CTRL_REG(chip->port),
		mv88fx_i2s_info.playback_cntrl_reg);
	/* enable interrupts */
	mv88fx_snd_writel(chip->base,
		MV_AUDIO_INT_MASK_REG(chip->port),
		mv88fx_i2s_info.irq_mask);

	spin_unlock(&chip->reg_lock);

	return 0;
}

static struct snd_soc_dai_ops mv88fx_i2s_dai_ops = {
	.startup = mv88fx_i2s_startup,
	.shutdown = mv88fx_i2s_shutdown,
	.hw_params = mv88fx_i2s_hw_params,
	.hw_free = mv88fx_i2s_hw_free,
	.prepare = mv88fx_i2s_prepare,
	.trigger = mv88fx_i2s_trigger,
	.hw_params = mv88fx_i2s_hw_params,
	.set_sysclk = mv88fx_i2s_set_sysclk,
	.set_fmt = mv88fx_i2s_set_fmt,
};

struct snd_soc_dai mv88fx_i2s_dai = {
	.name = "mv88fx-i2s",
	.id = 0,
	.probe = mv88fx_i2s_dai_probe,
	.remove = mv88fx_i2s_dai_remove,
	.suspend = mv88fx_i2s_suspend,
	.resume = mv88fx_i2s_resume,
	.ops = &mv88fx_i2s_dai_ops,
	.capture = {
		    .stream_name = "i2s-capture",
		    .formats = (SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S32_LE),
		    .rate_min = 44100,
		    .rate_max = 96000,
		    .rates = (SNDRV_PCM_RATE_44100 |
			      SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000),
		    .channels_min = 1,
		    .channels_max = 2,
		    },
	.playback = {
		     .stream_name = "i2s-playback",
		     .formats = (SNDRV_PCM_FMTBIT_S16_LE |
				 SNDRV_PCM_FMTBIT_S24_LE |
				 SNDRV_PCM_FMTBIT_S32_LE),
		     .rate_min = 44100,
		     .rate_max = 96000,
		     .rates = (SNDRV_PCM_RATE_44100 |
			       SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000),
		     .channels_min = 1,
		     .channels_max = 2,
		     },

	.runtime = NULL,
	.active = 0,
	.dma_data = NULL,
	.private_data = NULL,
};
EXPORT_SYMBOL_GPL(mv88fx_i2s_dai);





static int mv88fx_i2s_probe(struct platform_device *pdev)
{
	mv88fx_snd_debug("");
	mv88fx_i2s_dai.dev = &pdev->dev;
	return snd_soc_register_dai(&mv88fx_i2s_dai);
}

static int __devexit mv88fx_i2s_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&mv88fx_i2s_dai);
	return 0;
}

static struct platform_driver mv88fx_i2s_driver = {
	.probe = mv88fx_i2s_probe,
	.remove = __devexit_p(mv88fx_i2s_remove),

	.driver = {
		.name = "mv88fx-i2s",
		.owner = THIS_MODULE,
	},
};

static int __init mv88fx_i2s_init(void)
{
	mv88fx_snd_debug("");
	return platform_driver_register(&mv88fx_i2s_driver);
}

static void __exit mv88fx_i2s_exit(void)
{
	mv88fx_snd_debug("");
	platform_driver_unregister(&mv88fx_i2s_driver);
}

module_init(mv88fx_i2s_init);
module_exit(mv88fx_i2s_exit);


/* Module information */
MODULE_AUTHOR("Yuval Elmaliah <eyuval@marvell.com>");
MODULE_DESCRIPTION("mv88fx I2S SoC Interface");
MODULE_LICENSE("GPL");
