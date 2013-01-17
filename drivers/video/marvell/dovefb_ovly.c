/*
 * linux/drivers/video/dovefb.c -- Marvell DOVE LCD Controller
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by:
 *	Green Wan <gwan@marvell.com>
 *	Shadi Ammouri <shadi@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

/*
 * 1. Adapted from:  linux/drivers/video/skeletonfb.c
 * 2. Merged from: linux/drivers/video/dovefb.c (Lennert Buytenhek)
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/cpufreq.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/console.h>
#include <linux/io.h>
#include <asm/mach-types.h>
#include <asm/irq.h>

#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include "dovefb_if.h"

#define MAX_QUEUE_NUM	60
#define RESET_BUF	0x1
#define FREE_ENTRY	0x2

static int dovefb_ovly_set_par(struct fb_info *fi);
static void set_graphics_start(struct fb_info *fi, int xoffset, int yoffset);
static void set_dma_control0(struct dovefb_layer_info *dfli);
static int wait_for_vsync(struct dovefb_layer_info *dfli);
static void dovefb_do_tasklet(unsigned long data);

static int addFreeBuf(u8 **bufList, u8 *freeBuf);
static void clearFreeBuf(u8 **bufList, int iFlag);
static int dovefb_switch_buff(struct fb_info *fi);
static void collectFreeBuf(u8 **filterList, u8 **freeList, int count);
static u8 *filterBufList[MAX_QUEUE_NUM];
static u8 *freeBufList[MAX_QUEUE_NUM];

static struct _sViewPortInfo gViewPortInfo = {
	.srcWidth = 640,	/* video source size */
	.srcHeight = 480,
	.zoomXSize = 640,	/* size after zooming */
	.zoomYSize = 480,
};

static struct _sViewPortOffset gViewPortOffset = {
	.xOffset = 0,	/* position on screen */
	.yOffset = 0
};


static void dovefb_do_tasklet(unsigned long data)
{
	struct fb_info *fi;

	fi = (struct fb_info *)data;
	dovefb_switch_buff(fi);
}

static int convert_pix_fmt(u32 vmode)
{
	switch (vmode) {
	case DOVEFB_VMODE_YUV422PACKED:
		return PIX_FMT_YUV422PACK;
	case DOVEFB_VMODE_YUV422PACKED_SWAPUV:
		return PIX_FMT_YVU422PACK;
	case DOVEFB_VMODE_YUV422PLANAR:
		return PIX_FMT_YUV422PLANAR;
	case DOVEFB_VMODE_YUV422PLANAR_SWAPUV:
		return PIX_FMT_YVU422PLANAR;
	case DOVEFB_VMODE_YUV420PLANAR:
		return PIX_FMT_YUV420PLANAR;
	case DOVEFB_VMODE_YUV420PLANAR_SWAPUV:
		return PIX_FMT_YVU420PLANAR;
	case DOVEFB_VMODE_YUV422PACKED_SWAPYUorV:
		return PIX_FMT_UYVY422PACK;
	case DOVEFB_VMODE_RGB565:
		return PIX_FMT_RGB565;
	case DOVEFB_VMODE_BGR565:
		return PIX_FMT_BGR565;
	case DOVEFB_VMODE_RGB1555:
		return PIX_FMT_RGB1555;
	case DOVEFB_VMODE_BGR1555:
		return PIX_FMT_BGR1555;
	case DOVEFB_VMODE_RGB888PACK:
		return PIX_FMT_RGB888PACK;
	case DOVEFB_VMODE_BGR888PACK:
		return PIX_FMT_BGR888PACK;
	case DOVEFB_VMODE_RGBA888:
		return PIX_FMT_RGBA888;
	case DOVEFB_VMODE_BGRA888:
		return PIX_FMT_BGRA888;
	case DOVEFB_VMODE_RGB888UNPACK:
	case DOVEFB_VMODE_BGR888UNPACK:
	case DOVEFB_VMODE_YUV422PLANAR_SWAPYUorV:
	case DOVEFB_VMODE_YUV420PLANAR_SWAPYUorV:
	default:
		printk(KERN_ERR "%s: Unknown vmode (%d).\n", __func__, vmode);
		return -1;
	}
}

static u32 dovefb_ovly_create_surface(struct _sOvlySurface *pOvlySurface)
{
	u16 surfaceWidth;
	u16 surfaceHeight;
	u32 surfaceSize;
	DOVEFBVideoMode vmode;
	u8 *surfVABase;
	u8 *surfPABase;

	surfaceWidth = pOvlySurface->viewPortInfo.srcWidth;
	surfaceHeight = pOvlySurface->viewPortInfo.srcHeight;
	vmode	= pOvlySurface->videoMode;

	/* calculate video surface size */
	switch (vmode) {
	case DOVEFB_VMODE_YUV422PACKED:
	case DOVEFB_VMODE_YUV422PACKED_SWAPUV:
	case DOVEFB_VMODE_YUV422PACKED_SWAPYUorV:
	case DOVEFB_VMODE_YUV422PLANAR:
	case DOVEFB_VMODE_YUV422PLANAR_SWAPUV:
	case DOVEFB_VMODE_YUV422PLANAR_SWAPYUorV:
		surfaceSize = surfaceWidth * surfaceHeight * 2;
		break;
	case DOVEFB_VMODE_YUV420PLANAR:
	case DOVEFB_VMODE_YUV420PLANAR_SWAPUV:
	case DOVEFB_VMODE_YUV420PLANAR_SWAPYUorV:
		surfaceSize = surfaceWidth * surfaceHeight * 3/2;
		break;
	default:
		printk(KERN_INFO "Unknown video mode.\n");
		return -ENXIO;
	}

	/* get new video buffer */
	surfVABase = (u_char *)__get_free_pages(GFP_ATOMIC | GFP_DMA,
				get_order(surfaceSize*2));

	if (surfVABase == NULL) {
		printk(KERN_INFO "Unable to allocate surface memory\n");
		return -ENOMEM;
	}

	/* printk(KERN_INFO "\n create surface buffer"
		" = 0x%08x \n", (u32)surfVABase); */

	surfPABase = (u8 *)__pa(surfVABase);

	memset(surfVABase, 0x0, surfaceSize);

	pOvlySurface->videoBufferAddr.startAddr = surfPABase;
	return 0;
}

static u32 dovefb_ovly_set_colorkeyalpha(struct dovefb_layer_info *dfli)
{
	unsigned int rb;
	unsigned int temp;
	unsigned int x, x_ckey;
	struct _sColorKeyNAlpha *color_a = &dfli->ckey_alpha;

	/* reset to 0x0 to disable color key. */
	x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL1) &
		~(CFG_COLOR_KEY_MASK | CFG_ALPHA_MODE_MASK | CFG_ALPHA_MASK);

	/* switch to color key mode */
	switch (color_a->mode) {
	case DOVEFB_DISABLE_COLORKEY_MODE:
		/* do nothing */
		break;
	case DOVEFB_ENABLE_Y_COLORKEY_MODE:
		x |= CFG_COLOR_KEY_MODE(0x1);
		break;
	case DOVEFB_ENABLE_U_COLORKEY_MODE:
		x |= CFG_COLOR_KEY_MODE(0x2);
		break;
	case DOVEFB_ENABLE_V_COLORKEY_MODE:
		x |= CFG_COLOR_KEY_MODE(0x4);
		break;
	case DOVEFB_ENABLE_RGB_COLORKEY_MODE:
		x |= CFG_COLOR_KEY_MODE(0x3);

                /* check whether h/w turn on RB swap. */
                rb = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0);
                if (rb & CFG_DMA_SWAPRB_MASK) {
                        /* exchange r b fields. */
                        temp = color_a->Y_ColorAlpha;
                        color_a->Y_ColorAlpha = color_a->V_ColorAlpha;
                        color_a->V_ColorAlpha = temp;
                }

		break;
	case DOVEFB_ENABLE_R_COLORKEY_MODE:
		x |= CFG_COLOR_KEY_MODE(0x5);
		break;
	case DOVEFB_ENABLE_G_COLORKEY_MODE:
		x |= CFG_COLOR_KEY_MODE(0x6);
	break;
	case DOVEFB_ENABLE_B_COLORKEY_MODE:
		x |= CFG_COLOR_KEY_MODE(0x7);
		break;
	default:
		printk(KERN_INFO "unknown mode");
		return -1;
	}

	/* switch to alpha path selection */
	switch (color_a->alphapath) {
	case DOVEFB_VID_PATH_ALPHA:
		x |= CFG_ALPHA_MODE(0x0);
		break;
	case DOVEFB_GRA_PATH_ALPHA:
		x |= CFG_ALPHA_MODE(0x1);
		break;
	case DOVEFB_CONFIG_ALPHA:
		x |= CFG_ALPHA_MODE(0x2);
		break;
	default:
		printk(KERN_INFO "unknown alpha path");
		return -1;
	}

	/* configure alpha */
	x |= CFG_ALPHA((color_a->config & 0xff));

	/* Have to program new regs to enable color key for new chip. */
	x_ckey = readl(dfli->reg_base + LCD_SPU_DMA_CTRL1);
	writel( x_ckey | (0x1 << 19), dfli->reg_base + 0x84);

	writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL1);
	writel(color_a->Y_ColorAlpha, dfli->reg_base + LCD_SPU_COLORKEY_Y);
	writel(color_a->U_ColorAlpha, dfli->reg_base + LCD_SPU_COLORKEY_U);
	writel(color_a->V_ColorAlpha, dfli->reg_base + LCD_SPU_COLORKEY_V);

	return 0;
}

static int check_surface(struct fb_info *fi,
			DOVEFBVideoMode new_mode,
			struct _sViewPortInfo *new_info,
			struct _sViewPortOffset *new_offset,
			struct _sVideoBufferAddr *new_addr)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct fb_var_screeninfo *var = &fi->var;
	int changed = 0;
	/*
	 * check mode
	 */
	if (new_mode >= 0 && dfli->surface.videoMode != new_mode) {
		dfli->surface.videoMode = new_mode;
		dfli->pix_fmt = convert_pix_fmt(new_mode);
		dovefb_set_pix_fmt(var, dfli->pix_fmt);
		changed = 1;
	}

	/*
	 * check view port settings.
	 */
	if (new_info &&
	    (dfli->surface.viewPortInfo.srcWidth != new_info->srcWidth ||
	    dfli->surface.viewPortInfo.srcHeight != new_info->srcHeight ||
	    dfli->surface.viewPortInfo.zoomXSize != new_info->zoomXSize ||
	    dfli->surface.viewPortInfo.zoomYSize != new_info->zoomYSize)) {
		var->xres = new_info->srcWidth;
		var->yres = new_info->srcHeight;
		var->xres_virtual = new_info->srcWidth;
		var->yres_virtual = new_info->srcHeight;
		dfli->surface.viewPortInfo = *new_info;
		dovefb_set_pix_fmt(var, dfli->pix_fmt);
		changed = 1;
	}

	/*
	 * Check offset
	 */
	if (new_offset &&
	    (dfli->surface.viewPortOffset.xOffset != new_offset->xOffset ||
	    dfli->surface.viewPortOffset.yOffset != new_offset->yOffset)) {
		dfli->surface.viewPortOffset.xOffset = new_offset->xOffset;
		dfli->surface.viewPortOffset.yOffset = new_offset->yOffset;
		changed = 1;
	}

	/*
	 * Check buffer address
	 */
	if (new_addr && new_addr->startAddr &&
	    dfli->new_addr != (unsigned long)new_addr->startAddr) {
		dfli->new_addr = (unsigned long)new_addr->startAddr;
		changed = 1;
	}

	return changed;
}

static int dovefb_ovly_ioctl(struct fb_info *fi, unsigned int cmd,
		unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct dovefb_layer_info *dfli = fi->par;
	u32 x;
	int vmode = 0;
	int gfx_on = 1;
	int vid_on = 1;

	switch (cmd) {
	case DOVEFB_IOCTL_WAIT_VSYNC:
		wait_for_vsync(dfli);
		break;
	case DOVEFB_IOCTL_GET_VIEWPORT_INFO:
		return copy_to_user(argp, &dfli->surface.viewPortInfo,
			sizeof(struct _sViewPortInfo)) ? -EFAULT : 0;
	case DOVEFB_IOCTL_SET_VIEWPORT_INFO:
		mutex_lock(&dfli->access_ok);
		if (copy_from_user(&gViewPortInfo, argp,
				sizeof(gViewPortInfo))) {
			mutex_unlock(&dfli->access_ok);
			return -EFAULT;
		}

		if (check_surface(fi, -1, &gViewPortInfo, 0, 0))
			dovefb_ovly_set_par(fi);

		mutex_unlock(&dfli->access_ok);
		break;
	case DOVEFB_IOCTL_SET_VIDEO_MODE:
		/*
		 * Get data from user space.
		 */
		if (copy_from_user(&vmode, argp, sizeof(vmode)))
			return -EFAULT;

		if (check_surface(fi, vmode, 0, 0, 0))
			dovefb_ovly_set_par(fi);
		break;
	case DOVEFB_IOCTL_GET_VIDEO_MODE:
		return copy_to_user(argp, &dfli->surface.videoMode,
			sizeof(u32)) ? -EFAULT : 0;
	case DOVEFB_IOCTL_CREATE_VID_BUFFER:
	{
		struct _sOvlySurface OvlySurface;

		mutex_lock(&dfli->access_ok);
		if (copy_from_user(&OvlySurface, argp,
				sizeof(struct _sOvlySurface))) {
			mutex_unlock(&dfli->access_ok);
			return -EFAULT;
		}

		/* Request a video buffer. */
		dovefb_ovly_create_surface(&OvlySurface);

		if (copy_to_user(argp, &OvlySurface,
				sizeof(struct _sOvlySurface))) {
			mutex_unlock(&dfli->access_ok);
			return -EFAULT;
		}

		mutex_unlock(&dfli->access_ok);

		break;
	}
	case DOVEFB_IOCTL_FLIP_VID_BUFFER:
	{
		struct _sOvlySurface *surface = 0;
		u8 *start_addr, *input_data, *dst_addr;
		u32 length;
		surface = kmalloc(sizeof(struct _sOvlySurface),
				GFP_KERNEL);

		/* Get user-mode data. */
		if (copy_from_user(surface, argp,
		    sizeof(struct _sOvlySurface))) {
			kfree(surface);
			return -EFAULT;
		}
		mutex_lock(&dfli->access_ok);
		length = surface->videoBufferAddr.length;
		dst_addr = dfli->surface.videoBufferAddr.startAddr;
		start_addr = surface->videoBufferAddr.startAddr;
		input_data = surface->videoBufferAddr.inputData;

		/*
		 * Has DMA addr?
		 */
		if (start_addr &&
		    (!input_data)) {
			if (0 != addFreeBuf(freeBufList, (u8 *)surface)) {
				printk(KERN_INFO "Error: addFreeBuf()\n");
				mutex_unlock(&dfli->access_ok);
				kfree(surface);
				return -EFAULT;
			} else {
				/* printk(KERN_INFO "addFreeBuf(0x%08x) ok.\n",
					start_addr); */
			}
		} else {
			if (check_surface(fi, surface->videoMode,
					&surface->viewPortInfo,
					&surface->viewPortOffset,
					&surface->videoBufferAddr))
				dovefb_ovly_set_par(fi);

			/* copy buffer */
			if (input_data) {
				wait_for_vsync(dfli);
				/* if support hw DMA, replace this. */
				if (copy_from_user(dfli->fb_start,
						   input_data, length)) {
					mutex_unlock(&dfli->access_ok);
					kfree(surface);
					return -EFAULT;
				}
				mutex_unlock(&dfli->access_ok);
				kfree(surface);
				return 0;
			}

			kfree(surface);
#if 0
			/*
			 * Fix me: Currently not implemented yet.
			 * Application allocate a physical contiguous
			 * buffer and pass it into driver. Here is to
			 * update fb's info to new buffer and free
			 * old buffer.
			 */
			if (start_addr) {
				if (dfli->mem_status)
					free_pages(
					    (unsigned long)dfli->fb_start,
					    get_order(dfli->fb_size));
				else
					dma_free_writecombine(dfli->dev,
					    dfli->fb_size,
					    dfli->fb_start,
					    dfli->fb_start_dma);

				dfli->fb_start = __va(start_addr);
				dfli->fb_size = length;
				dfli->fb_start_dma =
				    (dma_addr_t)__pa(dfli->fb_start);
				dfli->mem_status = 1;
				fi->fix.smem_start = dfli->fb_start_dma;
				fi->fix.smem_len = dfli->fb_size;
				fi->screen_base = dfli->fb_start;
				fi->screen_size = dfli->fb_size;
			}
#endif
		}
		mutex_unlock(&dfli->access_ok);
		return 0;
	}
	case DOVEFB_IOCTL_GET_FREELIST:
	{
		mutex_lock(&dfli->access_ok);

		if (copy_to_user(argp, filterBufList,
				MAX_QUEUE_NUM*sizeof(u8 *))) {
			mutex_unlock(&dfli->access_ok);
			return -EFAULT;
		}

		clearFreeBuf(filterBufList, RESET_BUF);

		mutex_unlock(&dfli->access_ok);
		return 0;
	}
	case DOVEFB_IOCTL_GET_BUFF_ADDR:
	{
		return copy_to_user(argp, &dfli->surface.videoBufferAddr,
			sizeof(struct _sVideoBufferAddr)) ? -EFAULT : 0;
	}
	case DOVEFB_IOCTL_SET_VID_OFFSET:
		mutex_lock(&dfli->access_ok);
		if (copy_from_user(&gViewPortOffset, argp,
				sizeof(gViewPortOffset))) {
			mutex_unlock(&dfli->access_ok);
			return -EFAULT;
		}

		if (check_surface(fi, -1, 0, &gViewPortOffset, 0))
			dovefb_ovly_set_par(fi);
		mutex_unlock(&dfli->access_ok);
		break;
	case DOVEFB_IOCTL_GET_VID_OFFSET:
		return copy_to_user(argp, &dfli->surface.viewPortOffset,
			sizeof(struct _sViewPortOffset)) ? -EFAULT : 0;
	case DOVEFB_IOCTL_SET_MEMORY_TOGGLE:
		break;
	case DOVEFB_IOCTL_SET_COLORKEYnALPHA:
		if (copy_from_user(&dfli->ckey_alpha, argp,
		    sizeof(struct _sColorKeyNAlpha)))
			return -EFAULT;

		dovefb_ovly_set_colorkeyalpha(dfli);
		break;
	case DOVEFB_IOCTL_GET_COLORKEYnALPHA:
		if (copy_to_user(argp, &dfli->ckey_alpha,
		    sizeof(struct _sColorKeyNAlpha)))
			return -EFAULT;
		break;
	case DOVEFB_IOCTL_SWITCH_VID_OVLY:
		if (copy_from_user(&vid_on, argp, sizeof(int)))
			return -EFAULT;
		if (0 == vid_on) {
			x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0) &
				~CFG_DMA_ENA_MASK;
			writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
		} else {
			x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0) |
				CFG_DMA_ENA(0x1);
			writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
			/* Enable VID & VSync. */
			x = readl(dfli->reg_base + SPU_IRQ_ENA) |
				DOVEFB_VID_INT_MASK | DOVEFB_VSYNC_INT_MASK;
			writel( x, dfli->reg_base + SPU_IRQ_ENA);
		}
		break;
	case DOVEFB_IOCTL_SWITCH_GRA_OVLY:
		if (copy_from_user(&gfx_on, argp, sizeof(int)))
			return -EFAULT;
		if (0 == gfx_on) {
			x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0) &
				~CFG_GRA_ENA_MASK;
			writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
		} else {
			x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0) |
				CFG_GRA_ENA(0x1);
			writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
		}
		break;
	case DOVEFB_IOCTL_GET_FBID:
		mutex_lock(&dfli->access_ok);
		if (copy_to_user(argp, &dfli->cur_fbid, sizeof(unsigned int))) {
			mutex_unlock(&dfli->access_ok);
			return -EFAULT;
		}
		mutex_unlock(&dfli->access_ok);
		break;
	case DOVEFB_IOCTL_GET_SRC_MODE:
		mutex_lock(&dfli->access_ok);
		if (copy_to_user(argp, &dfli->src_mode, sizeof(int))) {
			mutex_unlock(&dfli->access_ok);
			return -EFAULT;
		}
		mutex_unlock(&dfli->access_ok);
		break;
	case DOVEFB_IOCTL_SET_SRC_MODE:
		mutex_lock(&dfli->access_ok);
		if (copy_from_user(&dfli->src_mode, argp, sizeof(int))) {
			mutex_unlock(&dfli->access_ok);
			return -EFAULT;
		}
		mutex_unlock(&dfli->access_ok);
		break;
	case DOVEFB_IOCTL_GET_FBPA:
		{
		struct shm_private_info info;
		int index;

		if (copy_from_user(&info, argp,
		    sizeof(struct shm_private_info)))
			return -EFAULT;

		/* which frame want to find. */
		index = info.fbid;

		/* calc physical address. */
		info.fb_pa = (unsigned long)(dfli->fb_start_dma+
				(index*info.width*info.height*MAX_YUV_PIXEL));
		if (copy_to_user(argp, &info, sizeof(struct shm_private_info)))
			return -EFAULT;

		break;
		}
	default:
		printk(KERN_INFO "ioctl_ovly(0x%x) No match.\n", cmd);
		break;
	}

	return 0;
}

static void collectFreeBuf(u8 **filterList, u8 **freeList, int count)
{
	int i = 0, j = 0;
	struct _sOvlySurface *srf = 0;
	u8 *ptr;

	if ((count < 1) || !filterList || !freeList)
		return;

	for (i = 0, j = 0; i < count; i++) {

		ptr = freeList[i];

		/* Check freeList's content. */
		if (ptr) {
			for (; j < MAX_QUEUE_NUM; j++) {
				if (!(filterList[j])) {
					/* get surface's ptr. */
					srf = (struct _sOvlySurface *)ptr;

					/*
					 * save ptrs which need
					 * to be freed.
					 */
					filterList[j] =
					    srf->videoBufferAddr.startAddr;

					break;
				}
			}

			if (j >= MAX_QUEUE_NUM)
				break;

			kfree(freeList[i]);
			freeList[i] = 0;
		} else {
			/* till content is null. */
			break;
		}
	}

	freeList[0] = freeList[count];
	freeList[count] = 0;
}

static int addFreeBuf(u8 **ppBufList, u8 *pFreeBuf)
{
	int i = 0 ;
	struct _sOvlySurface *srf0 = (struct _sOvlySurface *)(pFreeBuf);
	struct _sOvlySurface *srf1 = 0;

	/* Error checking */
	if (!srf0)
		return -1;

	for (; i < MAX_QUEUE_NUM; i++) {
		srf1 = (struct _sOvlySurface *)ppBufList[i];

		if (!srf1) {
			/* printk(KERN_INFO "Set pFreeBuf "
				"into %d entry.\n", i);
			*/
			ppBufList[i] = pFreeBuf;
			return 0;
		}

		if (srf1->videoBufferAddr.startAddr ==
		     srf0->videoBufferAddr.startAddr) {
			/* same address, free this request. */
			kfree(pFreeBuf);
			return 0;
		}
	}


	if (i >= MAX_QUEUE_NUM)
	    printk(KERN_INFO "buffer overflow\n");

	return -3;
}

static void clearFreeBuf(u8 **ppBufList, int iFlag)
{
	int i = 0;

	/* Check null pointer. */
	if (!ppBufList)
		return;

	/* free */
	if (FREE_ENTRY & iFlag) {
		for (i = 0; i < MAX_QUEUE_NUM; i++) {
			if (ppBufList && ppBufList[i])
				kfree(ppBufList[i]);
		}
	}

	if (RESET_BUF & iFlag)
		memset(ppBufList, 0, MAX_QUEUE_NUM * sizeof(u8 *));
}

static int dovefb_ovly_open(struct fb_info *fi, int user)
{
	struct dovefb_mach_info *dmi;
	struct dovefb_layer_info *dfli = fi->par;
	struct fb_var_screeninfo *var = &fi->var;

	dmi = dfli->dev->platform_data;
	dfli->new_addr = 0;
	dfli->cur_fbid = 0;
	fi->fix.smem_start = dfli->fb_start_dma;
	fi->fix.smem_len = dfli->fb_size;
	fi->screen_base = dfli->fb_start;
	fi->screen_size = dfli->fb_size;
	memset(dfli->fb_start, 0, dfli->fb_size);

	dfli->pix_fmt = dmi->pix_fmt;
	dfli->surface.videoMode = -1;
	dfli->surface.viewPortInfo.srcWidth = var->xres;
	dfli->surface.viewPortInfo.srcHeight = var->yres;
	dfli->surface.viewPortInfo.zoomXSize = var->xres;
	dfli->surface.viewPortInfo.zoomYSize = var->yres;
	dfli->surface.videoBufferAddr.startAddr = (unsigned char *)fi->fix.smem_start;
	dovefb_set_pix_fmt(var, dfli->pix_fmt);
	dovefb_ovly_set_par(fi);

	if (mutex_is_locked(&dfli->access_ok))
		mutex_unlock(&dfli->access_ok);

	/* clear buffer list. */
	mutex_lock(&dfli->access_ok);
	clearFreeBuf(filterBufList, RESET_BUF);
	clearFreeBuf(freeBufList, RESET_BUF|FREE_ENTRY);
	mutex_unlock(&dfli->access_ok);

	return 0;
}

static int dovefb_release(struct fb_info *fi, int user)
{
	struct dovefb_layer_info *dfli = fi->par;

	/* Disable all interrupts. */
	writel( 0x0, dfli->reg_base + SPU_IRQ_ENA);

	/* clear buffer list. */
	mutex_lock(&dfli->access_ok);
	clearFreeBuf(filterBufList, RESET_BUF);
	clearFreeBuf(freeBufList, RESET_BUF|FREE_ENTRY);
	mutex_unlock(&dfli->access_ok);
	return 0;
}

static int dovefb_switch_buff(struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;
	int i = 0;
	struct _sOvlySurface *pOvlySurface = 0;
	unsigned long startaddr;
	int fbid;

	/*
	 * Find the latest frame.
	 */
	for (i = (MAX_QUEUE_NUM-1); i >= 0; i--) {
		if (freeBufList[i]) {
			pOvlySurface = (struct _sOvlySurface *)freeBufList[i];
			break;
		}
	}

	if (!pOvlySurface) {
		/*printk(KERN_INFO "********Oops: pOvlySurface"
			" is NULL!!!!\n\n");*/
		return -1;
	}

	startaddr = (unsigned long)pOvlySurface->videoBufferAddr.startAddr;
	fbid = (int)pOvlySurface->videoBufferAddr.frameID;
	/*
	 * Got new frame?
	 */
	if (dfli->new_addr != startaddr) {
		/*
		 * Collect expired frame to list.
		 */
		collectFreeBuf(filterBufList, freeBufList, (i));

		/*
		 * Update new surface.
		 */
		if (check_surface(fi, pOvlySurface->videoMode,
				&pOvlySurface->viewPortInfo,
				&pOvlySurface->viewPortOffset,
				&pOvlySurface->videoBufferAddr)) {
			dovefb_ovly_set_par(fi);
			dfli->cur_fbid = fbid;
		}
	}

	return 0;
}



static void set_dma_control0(struct dovefb_layer_info *dfli)
{
	u32 x, x_bk;
	int pix_fmt;

	pix_fmt = dfli->pix_fmt;

	/*
	 * Get reg's current value
	 */
	x_bk = x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL0);
	/*
	 * clear video layer's field
	 */
	x &= 0xef0fff01;

	/*
	 * enable horizontal smooth scaling.
	 */
	x |= 0x1 << 6;

	/*
	 * If we are in a pseudo-color mode, we need to enable
	 * palette lookup.
	 */
	if (pix_fmt == PIX_FMT_PSEUDOCOLOR)
		x |= 0x10000000;

	/*
	 * Configure hardware pixel format.
	 */
	x |= ((pix_fmt & ~0x1000) >> 1) << 20;

	/*
	 * configure video format into HW.
	 * HW default use UYVY as YUV422Packed format
	 *            use YUV as planar format
	 */
	if ((pix_fmt & 0x1000)) {	/* UYVY. */
		x |= 0x00000002;
	} else if ((pix_fmt == 10) || (pix_fmt == 11)) { /* YUYV, YVYU */
		x |= 1				<< 1;
		x |= 1				<< 2;
		x |= (pix_fmt & 1) 		<< 3;
		x |= (dfli->info->panel_rbswap) << 4;
	} else if (pix_fmt >= 12) {	/* YUV Planar */
		x |= 1				<< 1;
		x |= ((pix_fmt & 1) ^ 1)	<< 3;
		x |= dfli->info->panel_rbswap	<< 4;
	} else {	/* RGB, BGR format */
		x |= ((pix_fmt & 1)^(dfli->info->panel_rbswap)) << 4;
	}

#ifdef CONFIG_ARCH_DOVE
	/* Requires fix */
	if (machine_is_videoplug())
		x |= 1 << 4;
#endif

	if (x_bk != x)
		writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL0);
}

static void set_dma_control1(struct dovefb_layer_info *dfli, int sync)
{
	u32 x, x_bk;

	/*
	 * Get current settings.
	 */
	x_bk = x = readl(dfli->reg_base + LCD_SPU_DMA_CTRL1);

	/*
	 * We trigger DMA on the falling edge of vsync if vsync is
	 * active low, or on the rising edge if vsync is active high.
	 */
	if (!(sync & FB_SYNC_VERT_HIGH_ACT))
		x |= 0x08000000;

	if (x_bk != x)
		writel(x, dfli->reg_base + LCD_SPU_DMA_CTRL1);
}

static int wait_for_vsync(struct dovefb_layer_info *dfli)
{
	if (dfli) {
		u32 irq_ena = readl(dfli->reg_base + SPU_IRQ_ENA);
		int rc = 0;

		writel(irq_ena | DOVEFB_VID_INT_MASK | DOVEFB_VSYNC_INT_MASK,
		       dfli->reg_base + SPU_IRQ_ENA);

		rc = wait_event_interruptible_timeout(dfli->w_intr_wq,
						      atomic_read(&dfli->w_intr), 40);

		if ( rc <= 0)
			printk(KERN_ERR "%s: vid wait for vsync timed out, rc %d\n",
			       __func__, rc);

		writel(irq_ena,
		       dfli->reg_base + SPU_IRQ_ENA);
		atomic_set(&dfli->w_intr, 0);
		return 0;
	}

	return 0;
}

static void set_graphics_start(struct fb_info *fi, int xoffset, int yoffset)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct fb_var_screeninfo *var = &fi->var;
	int pixel_offset;
	unsigned long addr;

	pixel_offset = (yoffset * var->xres_virtual) + xoffset;

	if (dfli->new_addr) {
		addr = dfli->new_addr +
			(pixel_offset * (var->bits_per_pixel >> 3));
	} else {
		addr = dfli->fb_start_dma +
			(pixel_offset * (var->bits_per_pixel >> 3));
	}

	writel(addr, dfli->reg_base + LCD_SPU_DMA_START_ADDR_Y0);

	if (dfli->pix_fmt >= 12 && dfli->pix_fmt <= 15)
		addr += var->xres * var->yres;
	writel(addr, dfli->reg_base + LCD_SPU_DMA_START_ADDR_U0);

	if ((dfli->pix_fmt>>1) == 6)
		addr += var->xres * var->yres/2;
	else if ((dfli->pix_fmt>>1) == 7)
		addr += var->xres * var->yres/4;
	writel(addr, dfli->reg_base + LCD_SPU_DMA_START_ADDR_V0);
}

static int dovefb_ovly_pan_display(struct fb_var_screeninfo *var,
    struct fb_info *fi)
{
	set_graphics_start(fi, var->xoffset, var->yoffset);

	return 0;
}

static int dovefb_ovly_set_par(struct fb_info *fi)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct dovefb_info *info = dfli->info;
	struct dovefb_layer_info *gfxli = info->gfx_plane;
	struct fb_var_screeninfo *var = &fi->var;
	int pix_fmt;
	int xzoom, yzoom;
	int xbefzoom;

	/*
	 * Determine which pixel format we're going to use.
	 */
	pix_fmt = dovefb_determine_best_pix_fmt(&fi->var, dfli);
	if (pix_fmt < 0)
		return pix_fmt;
	dfli->pix_fmt = pix_fmt;

	/*
	 * Set RGB bit field info.
	 */
	dovefb_set_pix_fmt(var, pix_fmt);

	/*
	 * Set additional mode info.
	 */
	if (pix_fmt == PIX_FMT_PSEUDOCOLOR)
		fi->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else
		fi->fix.visual = FB_VISUAL_TRUECOLOR;
	fi->fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;

	/*
	 * Configure global panel parameters.
	 */
	set_dma_control0(dfli);
	set_dma_control1(dfli, fi->var.sync);

	/*
	 * Configure graphics DMA parameters.
	 */
	xbefzoom = var->xres/2;
	set_graphics_start(fi, fi->var.xoffset, fi->var.yoffset);

	if ((dfli->pix_fmt >= 0) && (dfli->pix_fmt < 10)) {
		writel((var->xres_virtual * var->bits_per_pixel) >> 3,
			dfli->reg_base + LCD_SPU_DMA_PITCH_YC);
		writel((var->yres << 16) | xbefzoom,
			dfli->reg_base + LCD_SPU_DMA_HPXL_VLN);
		writel((var->yres << 16) | var->xres,
			dfli->reg_base + LCD_SPU_DZM_HPXL_VLN);
	} else {
		if (((dfli->pix_fmt & ~0x1000) >> 1) == 5) {
			writel((var->xres*2),
				dfli->reg_base + LCD_SPU_DMA_PITCH_YC);
			writel(((var->xres) << 16) | (var->xres) ,
				dfli->reg_base + LCD_SPU_DMA_PITCH_UV);
		} else {
			writel((var->xres),
				dfli->reg_base + LCD_SPU_DMA_PITCH_YC);
			writel((var->xres >> 1) << 16 |
				(var->xres >> 1) ,
				dfli->reg_base + LCD_SPU_DMA_PITCH_UV);
		}

		writel((var->yres << 16) | (xbefzoom*2),
			dfli->reg_base + LCD_SPU_DMA_HPXL_VLN);
		if (info->fixed_output == 0) {
			yzoom = dfli->surface.viewPortInfo.zoomYSize;
			xzoom = dfli->surface.viewPortInfo.zoomXSize;
		} else {
			yzoom = dfli->surface.viewPortInfo.zoomYSize *
				info->out_vmode.yres / gfxli->fb_info->var.yres;
			xzoom = dfli->surface.viewPortInfo.zoomXSize *
				info->out_vmode.xres / gfxli->fb_info->var.xres;
		}
		writel((yzoom << 16) | xzoom,
				dfli->reg_base + LCD_SPU_DZM_HPXL_VLN);
	}

	/* update video position offset */
	writel(CFG_DMA_OVSA_VLN(dfli->surface.viewPortOffset.yOffset)|
		dfli->surface.viewPortOffset.xOffset,
		dfli->reg_base + LCD_SPUT_DMA_OVSA_HPXL_VLN);

	return 0;
}


static int dovefb_ovly_fb_sync(struct fb_info *info)
{
	struct dovefb_layer_info *dfli = info->par;

	return wait_for_vsync(dfli);
}


/*
 *  dovefb_handle_irq(two lcd controllers)
 */
int dovefb_ovly_handle_irq(u32 isr, struct dovefb_layer_info *dfli)
{
	/* wake up queue. */
	atomic_set(&dfli->w_intr, 1);
	wake_up(&dfli->w_intr_wq);

	/* add a tasklet. */
	tasklet_schedule(&dfli->tasklet);

	return 1;
}


#ifdef CONFIG_PM
int dovefb_ovly_suspend(struct dovefb_layer_info *dfli, pm_message_t mesg)
{
	struct fb_info *fi = dfli->fb_info;

	if (mesg.event & PM_EVENT_SLEEP)
		fb_set_suspend(fi, 1);

	return 0;
}

int dovefb_ovly_resume(struct dovefb_layer_info *dfli)
{
	struct fb_info *fi = dfli->fb_info;

	dfli->active = 1;

	if (dovefb_ovly_set_par(fi) != 0) {
		printk(KERN_INFO "dovefb_ovly_resume(): Failed in "
				"dovefb_ovly_set_par().\n");
		return -1;
	}

	fb_set_suspend(fi, 0);

	return 0;
}
#endif

int dovefb_ovly_init(struct dovefb_info *info, struct dovefb_mach_info *dmi)
{
	struct dovefb_layer_info *dfli = info->vid_plane;
	struct fb_info *fi = dfli->fb_info;

	dfli->tasklet.next = NULL;
	dfli->tasklet.state = 0;
	atomic_set(&dfli->tasklet.count, 0);
	dfli->tasklet.func = dovefb_do_tasklet;
	dfli->tasklet.data = (unsigned long)fi;
	dfli->surface.videoBufferAddr.startAddr = (unsigned char *)fi->fix.smem_start; 

	init_waitqueue_head(&dfli->w_intr_wq);
	mutex_init(&dfli->access_ok);

	/*
	 * Fill in sane defaults.
	 */
	dovefb_set_mode(dfli, &fi->var, dmi->modes, dmi->pix_fmt, 0);
	dovefb_ovly_set_par(fi);

	/*
	 * Configure default register values.
	 */
	writel(0x00000000, dfli->reg_base + LCD_SPU_DMA_START_ADDR_Y1);
	writel(0x00000000, dfli->reg_base + LCD_SPU_DMA_START_ADDR_U1);
	writel(0x00000000, dfli->reg_base + LCD_SPU_DMA_START_ADDR_V1);
	writel(0x00000000, dfli->reg_base + LCD_SPUT_DMA_OVSA_HPXL_VLN);

	return 0;
}


/* Fix me: Currently, bufferable property can't be enabled correctly. It has
 * to be enabled cacheable. Or write buffer won't act properly. Here
 * we override the attribute when any program wants to map to this
 * buffer. When HW is ready, this function could simply be removed.
 */
static int dovefb_ovly_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	unsigned long off;
	unsigned long start;
	u32 len;

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	off = vma->vm_pgoff << PAGE_SHIFT;

	lock_kernel();

	/* frame buffer memory */
	start = info->fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
	if (off >= len) {
		/* memory mapped io */
		off -= len;
		if (info->var.accel_flags) {
			unlock_kernel();
			return -EINVAL;
		}
		start = info->fix.mmio_start;
		len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.mmio_len);
	}
	unlock_kernel();
	start &= PAGE_MASK;
	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO | VM_RESERVED;

	/* io control needs non-cacheable attribute. */
	if (start == info->fix.mmio_start)
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	else
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			     vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
	return 0;
}

struct fb_ops dovefb_ovly_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= dovefb_ovly_open,
	.fb_release	= dovefb_release,
	.fb_mmap	= dovefb_ovly_mmap,
	.fb_check_var	= dovefb_check_var,
	.fb_set_par	= dovefb_ovly_set_par,
/*	.fb_setcolreg	= dovefb_setcolreg,*/
	.fb_pan_display	= dovefb_ovly_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_sync	= dovefb_ovly_fb_sync,
	.fb_ioctl	= dovefb_ovly_ioctl,
};


MODULE_AUTHOR("Green Wan <gwan@marvell.com>");
MODULE_AUTHOR("Shadi Ammouri <shadi@marvell.com>");
MODULE_DESCRIPTION("Framebuffer driver for Dove");
MODULE_LICENSE("GPL");
