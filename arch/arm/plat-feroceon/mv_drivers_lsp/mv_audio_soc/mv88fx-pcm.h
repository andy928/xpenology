/*
 *
 *	Marvell Orion Alsa SOC Sound driver
 *
 *	Author: Yuval Elmaliah
 *	Copyright (C) 2008 Marvell Ltd
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 *
 */


#ifndef _MV88FX_PCM_H
#define _MV88FX_PCM_H

#define DRIVER_NAME	"mv88fx_snd"

#ifdef MV88FX_SND_DEBUG
#define mv88fx_snd_debug(format, args...) \
	printk(KERN_DEBUG "%s(%d): "format"\n", __func__, __LINE__, ##args)
#else
#define mv88fx_snd_debug(a...)
#endif
#define mv88fx_snd_error(format, args...) \
	printk(KERN_ERR "%s(%d): "format"\n", __func__, __LINE__, ##args)


struct mv88fx_snd_stream {

unsigned int dig_mode;	/* i2s,spdif,both */

#define I2S		1
#define SPDIF		2
	int mono_mode;	/* both mono, left mono, right mono */
#define MONO_BOTH	0
#define MONO_LEFT	1
#define MONO_RIGHT	2
	int clock_src;

#define DCO_CLOCK	0
#define SPCR_CLOCK	1
#define EXTERN_CLOCK	2
	int stat_mem;	/* Channel status source */

 unsigned int spdif_status[4];	/* SPDIF status */

};


struct mv88fx_snd_chip {
	struct snd_card *card;
	struct snd_pcm *pcm;
	struct device *dev;
	int port;
	struct orion_i2s_platform_data *pdata;	/* platform dara */
	struct mv88fx_snd_stream stream[2];	/* run time values */
	spinlock_t reg_lock;	/* Register access spinlock */
	void __iomem *base;	/* base address of the host */
	int loopback;		/* When Loopback is enabled, playback
				data is looped back to be recorded */
	int ch_stat_valid;	/* Playback SPDIF channel validity bit
				   value when REG selected */
	int burst;		/* DMA Burst Size */
#define SPDIF_MEM_STAT		0
#define SPDIF_REG_STAT		1
	unsigned int dco_ctrl_offst;
	int pcm_mode;		/* pcm, nonpcm */
#define PCM		0
#define NON_PCM		1
	int irq;

};



struct mv88fx_snd_machine_data {
	struct platform_device *snd_dev;
	int port;
	void __iomem *base;	/* base address of the host */
	int irq;
	struct resource *res;	/* resource for IRQ and base */
	struct orion_i2s_platform_data *pdata;	/* platform dara */
#if defined(CONFIG_HAVE_CLK)
	struct clk		*clk;
#endif
};


#define MV88FX_SND_MIN_PERIODS		8
#define MV88FX_SND_MAX_PERIODS		16
#define	MV88FX_SND_MIN_PERIOD_BYTES	0x4000
#define	MV88FX_SND_MAX_PERIOD_BYTES	0x4000


/* read/write registers */
#define mv88fx_snd_writel(base, offs, val)	\
	writel((val), (base + offs))

#define mv88fx_snd_readl(base, offs)	\
	readl(base + offs)

#define mv88fx_snd_bitset(base, offs, bitmask)	\
	writel((readl(base + offs) | (bitmask)),	\
	base + offs)

#define mv88fx_snd_bitreset(base, offs, bitmask) \
	writel((readl(base + offs) & (~(bitmask))), \
	base + offs)




extern struct mv88fx_snd_chip *mv88fx_pcm_snd_chip;

#define mv88fx_pcm_get_chip() mv88fx_pcm_snd_chip
#define mv88fx_pcm_is_stereo(runtime)        ((runtime)->channels != 1)


extern struct snd_soc_platform mv88fx_soc_platform;



#endif
