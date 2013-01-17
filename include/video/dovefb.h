/*
 * linux/include/video/dovefb.h -- Marvell frame buffer for DOVE
 *
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by Green Wan <gwan@marvell.com>
 *
 * Adapted from:  linux/drivers/video/skeletonfb.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 *
 */
#ifndef _DOVEFB_H_
#define _DOVEFB_H_

/* ---------------------------------------------- */
/*              Header Files                      */
/* ---------------------------------------------- */
#include <linux/fb.h>

/* ---------------------------------------------- */
/*              IOCTL Definition                  */
/* ---------------------------------------------- */
#define DOVEFB_IOC_MAGIC			'm'
#define DOVEFB_IOCTL_CONFIG_CURSOR		_IO(DOVEFB_IOC_MAGIC, 0)
#define DOVEFB_IOCTL_DUMP_REGS			_IO(DOVEFB_IOC_MAGIC, 1)
#define DOVEFB_IOCTL_CLEAR_IRQ			_IO(DOVEFB_IOC_MAGIC, 2)

/*
 * There are many video mode supported.
 */
#define DOVEFB_IOCTL_SET_VIDEO_MODE		_IO(DOVEFB_IOC_MAGIC, 3)
#define DOVEFB_IOCTL_GET_VIDEO_MODE		_IO(DOVEFB_IOC_MAGIC, 4)
/* Request a new video buffer from driver. User program needs to free
 * this memory.
 */
#define DOVEFB_IOCTL_CREATE_VID_BUFFER		_IO(DOVEFB_IOC_MAGIC, 5)

/* Configure viewport in driver. */
#define DOVEFB_IOCTL_SET_VIEWPORT_INFO		_IO(DOVEFB_IOC_MAGIC, 6)
#define DOVEFB_IOCTL_GET_VIEWPORT_INFO		_IO(DOVEFB_IOC_MAGIC, 7)

/* Flip the video buffer from user mode. Vide buffer can be separated into:
 * a. Current-used buffer - user program put any data into it. It will be
 *    displayed immediately.
 * b. Requested from driver but not current-used - user programe can put any
 *    data into it. It will be displayed after calling
 *    DOVEFB_IOCTL_FLIP_VID_BUFFER.
 *    User program should free this memory when they don't use it any more.
 * c. User program alloated - user program can allocated a contiguos DMA
 *    buffer to store its video data. And flip it to driver. Notices that
 *    this momory should be free by user programs. Driver won't take care of
 *    this.
 */
#define DOVEFB_IOCTL_FLIP_VID_BUFFER		_IO(DOVEFB_IOC_MAGIC, 8)

/* Get the current buffer information. User program could use it to display
 * anything directly. If developer wants to allocate multiple video layers,
 * try to use DOVEFB_IOCTL_CREATE_VID_BUFFER  to request a brand new video
 * buffer.
 */
#define DOVEFB_IOCTL_GET_BUFF_ADDR		_IO(DOVEFB_IOC_MAGIC, 9)

/* Get/Set offset position of screen */
#define DOVEFB_IOCTL_SET_VID_OFFSET		_IO(DOVEFB_IOC_MAGIC, 10)
#define DOVEFB_IOCTL_GET_VID_OFFSET		_IO(DOVEFB_IOC_MAGIC, 11)

/* Turn on the memory toggle function to improve the frame rate while playing
 * movie.
 */
#define DOVEFB_IOCTL_SET_MEMORY_TOGGLE		_IO(DOVEFB_IOC_MAGIC, 12)

/* Turn on the memory toggle function to improve the frame rate while playing
 * movie.
 */
#define DOVEFB_IOCTL_SET_COLORKEYnALPHA		_IO(DOVEFB_IOC_MAGIC, 13)
#define DOVEFB_IOCTL_GET_COLORKEYnALPHA		_IO(DOVEFB_IOC_MAGIC, 14)
#define DOVEFB_IOCTL_SWITCH_GRA_OVLY		_IO(DOVEFB_IOC_MAGIC, 15)
#define DOVEFB_IOCTL_SWITCH_VID_OVLY		_IO(DOVEFB_IOC_MAGIC, 16)

/* For Vmeta integration */
#define DOVEFB_IOCTL_GET_FREELIST		_IO(DOVEFB_IOC_MAGIC, 17)

/* Wait for vsync happen. */
#define DOVEFB_IOCTL_WAIT_VSYNC			_IO(DOVEFB_IOC_MAGIC, 18)

/* for xv+vmeta/sw decoder w/o memory move. */
#define DOVEFB_IOCTL_GET_FBPA			_IO(DOVEFB_IOC_MAGIC, 19)
#define DOVEFB_IOCTL_GET_FBID			_IO(DOVEFB_IOC_MAGIC, 20)
#define DOVEFB_IOCTL_SET_SRC_MODE		_IO(DOVEFB_IOC_MAGIC, 21)
#define DOVEFB_IOCTL_GET_SRC_MODE		_IO(DOVEFB_IOC_MAGIC, 22)

/* clear framebuffer: Makes resolution or color space changes look nicer */
#define FBIO_CLEAR_FRAMEBUFFER              	_IO(FB_IOC_MAGIC, 19)

/* Global alpha blend controls - Maintaining compatibility with existing
   user programs. */
#define FBIOPUT_VIDEO_ALPHABLEND            0xeb
#define FBIOPUT_GLOBAL_ALPHABLEND           0xe1
#define FBIOPUT_GRAPHIC_ALPHABLEND          0xe2

/* color swapping */
#define FBIOPUT_SWAP_GRAPHIC_RED_BLUE       0xe3
#define FBIOPUT_SWAP_GRAPHIC_U_V            0xe4
#define FBIOPUT_SWAP_GRAPHIC_Y_UV           0xe5
#define FBIOPUT_SWAP_VIDEO_RED_BLUE         0xe6
#define FBIOPUT_SWAP_VIDEO_U_V              0xe7
#define FBIOPUT_SWAP_VIDEO_Y_UV             0xe8

/* colorkey compatibility */
#define FBIOGET_CHROMAKEYS                  0xe9
#define FBIOPUT_CHROMAKEYS                  0xea

#define DOVEFB_VMODE_RGB565			0x100
#define DOVEFB_VMODE_BGR565			0x101
#define DOVEFB_VMODE_RGB1555			0x102
#define DOVEFB_VMODE_BGR1555			0x103
#define DOVEFB_VMODE_RGB888PACK			0x104
#define DOVEFB_VMODE_BGR888PACK			0x105
#define DOVEFB_VMODE_RGB888UNPACK		0x106
#define DOVEFB_VMODE_BGR888UNPACK		0x107
#define DOVEFB_VMODE_RGBA888			0x108
#define DOVEFB_VMODE_BGRA888			0x109

#define	DOVEFB_VMODE_YUV422PACKED		0x0
#define	DOVEFB_VMODE_YUV422PACKED_SWAPUV	0x1
#define	DOVEFB_VMODE_YUV422PACKED_SWAPYUorV	0x2
#define	DOVEFB_VMODE_YUV422PLANAR		0x3
#define	DOVEFB_VMODE_YUV422PLANAR_SWAPUV	0x4
#define	DOVEFB_VMODE_YUV422PLANAR_SWAPYUorV	0x5
#define	DOVEFB_VMODE_YUV420PLANAR		0x6
#define	DOVEFB_VMODE_YUV420PLANAR_SWAPUV	0x7
#define	DOVEFB_VMODE_YUV420PLANAR_SWAPYUorV	0x8

#define	DOVEFB_HWCMODE_1BITMODE			0x0
#define	DOVEFB_HWCMODE_2BITMODE			0x1

#define	DOVEFB_DISABLE_COLORKEY_MODE		0x0
#define	DOVEFB_ENABLE_Y_COLORKEY_MODE		0x1
#define	DOVEFB_ENABLE_U_COLORKEY_MODE		0x2
#define	DOVEFB_ENABLE_V_COLORKEY_MODE		0x4
#define	DOVEFB_ENABLE_RGB_COLORKEY_MODE		0x3
#define	DOVEFB_ENABLE_R_COLORKEY_MODE		0x5
#define	DOVEFB_ENABLE_G_COLORKEY_MODE		0x6
#define	DOVEFB_ENABLE_B_COLORKEY_MODE		0x7

#define DOVEFB_VID_PATH_ALPHA               0x0
#define DOVEFB_GRA_PATH_ALPHA               0x1
#define DOVEFB_CONFIG_ALPHA                 0x2

#define DOVEFB_SYNC_COLORKEY_TO_CHROMA          1
#define DOVEFB_SYNC_CHROMA_TO_COLORKEY          2

/* Compatible to pxa168. */
#define FB_IOCTL_SET_COLORKEYnALPHA            _IO(FB_IOC_MAGIC, 13)
#define FB_IOCTL_GET_COLORKEYnALPHA            _IO(FB_IOC_MAGIC, 14)
#define FB_VID_PATH_ALPHA               0x0
#define FB_GRA_PATH_ALPHA               0x1
#define FB_CONFIG_ALPHA                 0x2

#define FB_SYNC_COLORKEY_TO_CHROMA          1
#define FB_SYNC_CHROMA_TO_COLORKEY          2

#define DOVEFB_FB_NUM		2

/* ---------------------------------------------- */
/*              Data Structure                    */
/* ---------------------------------------------- */
/*
 * The follow structures are used to pass data from
 * user space into the kernel for the creation of
 * overlay surfaces and setting the video mode.
 */

#define DOVEFBVideoMode signed int

struct _sViewPortInfo {
	unsigned short srcWidth;	/* video source size */
	unsigned short srcHeight;
	unsigned short zoomXSize;	/* size after zooming */
	unsigned short zoomYSize;
	unsigned short ycPitch;
	unsigned short uvPitch;
};

struct _sViewPortOffset {
	unsigned short xOffset;		/* position on screen */
	unsigned short yOffset;
};

struct _sVideoBufferAddr {
	unsigned char	frameID;	/* which frame wants */
	unsigned char *startAddr;	/* new buffer (PA) */
	unsigned char *inputData;	/* input buf address (VA) */
	unsigned int length;		/* input data's length */
};

struct dovefb_chroma {
        u_char     mode;
        u_char     y_alpha;
        u_char     y;
        u_char     y1;
        u_char     y2;
        u_char     u_alpha;
        u_char     u;
        u_char     u1;
        u_char     u2;
        u_char     v_alpha;
        u_char     v;
        u_char     v1;
        u_char     v2;
};

struct _sColorKeyNAlpha {
        unsigned int mode;
        unsigned int alphapath;
        unsigned int config;
        unsigned int Y_ColorAlpha;
        unsigned int U_ColorAlpha;
        unsigned int V_ColorAlpha;
};

struct _sOvlySurface {
	DOVEFBVideoMode videoMode;
	struct _sViewPortInfo viewPortInfo;
	struct _sViewPortOffset viewPortOffset;
	struct _sVideoBufferAddr videoBufferAddr;
};

struct _sCursorConfig {
	unsigned char	enable;		/* enable cursor or not */
	unsigned char	mode;		/* 1bit or 2bit mode */
	unsigned int color1;		/* foreground color */
	unsigned int color2;		/* background color */
	unsigned short xoffset;
	unsigned short yoffset;
	unsigned short	width;
	unsigned short height;
	unsigned char *pBuffer;		/* cursor data */
};

#define SHM_NORMAL		0x01
#define SHM_VMETA		0x02
#define SHM_SOFTWARE_MAP	0x04

struct shm_private_info {
	unsigned int method;
	unsigned int fbid;
	unsigned int format;
	unsigned int width;
	unsigned int height;
	unsigned long fb_pa;
};

/* MAX bytes per yuv pixel. */
#define MAX_YUV_PIXEL	2

/* Dumb interface */
#define DOVEFB_PINS_DUMB_24		0
#define DOVEFB_PINS_DUMB_18_SPI		1
#define DOVEFB_PINS_DUMB_18_GPIO	2
#define DOVEFB_PINS_DUMB_16_SPI		3
#define DOVEFB_PINS_DUMB_16_GPIO	4
#define DOVEFB_PINS_DUMB_12_SPI_GPIO	5
#define DOVEFB_PINS_SMART_18_SPI	6
#define DOVEFB_PINS_SMART_16_SPI	7
#define DOVEFB_PINS_SMART_8_SPI_GPIO	8

/* Dumb interface pin allocation */
#define DOVEFB_DUMB_PANEL_RGB565	0
#define DOVEFB_DUMB_PANEL_RGB565_UPPER	1
#define DOVEFB_DUMB_PANEL_RGB666	2
#define DOVEFB_DUMB_PANEL_RGB666_UPPER	3
#define DOVEFB_DUMB_PANEL_RGB444	4
#define DOVEFB_DUMB_PANEL_RGB444_UPPER	5
#define DOVEFB_DUMB_PANEL_RGB888	6

/* Max fb buffer. 1048x2048-32bits */
#define DEFAULT_FB_SIZE	(1024 * 2048 * 4)

/*
 * Buffer pixel format
 * bit0 is for rb swap.
 * bit12 is for Y UorV swap
 */
#define PIX_FMT_RGB565		0
#define PIX_FMT_BGR565		1
#define PIX_FMT_RGB1555		2
#define PIX_FMT_BGR1555		3
#define PIX_FMT_RGB888PACK	4
#define PIX_FMT_BGR888PACK	5
#define PIX_FMT_RGB888UNPACK	6
#define PIX_FMT_BGR888UNPACK	7
#define PIX_FMT_RGBA888		8
#define PIX_FMT_BGRA888		9
#define PIX_FMT_YUV422PACK	10
#define PIX_FMT_YVU422PACK	11
#define PIX_FMT_YUV422PLANAR	12
#define PIX_FMT_YVU422PLANAR	13
#define PIX_FMT_YUV420PLANAR	14
#define PIX_FMT_YVU420PLANAR	15
#define PIX_FMT_PSEUDOCOLOR	20
#define PIX_FMT_UYVY422PACK	(0x1000|PIX_FMT_YUV422PACK)

#ifdef __KERNEL__
#include <linux/interrupt.h>

enum dovefb_type {
	DOVEFB_GFX_PLANE = 0,
	DOVEFB_OVLY_PLANE
};

struct dovefb_layer_info {
	struct device		*dev;
	enum dovefb_type	type;
	struct dovefb_info	*info;
	struct fb_info		*fb_info;

	void			*reg_base;

	unsigned long		new_addr;
	dma_addr_t		fb_start_dma;
	void			*fb_start;
	int			fb_size;
	atomic_t		w_intr;
	wait_queue_head_t	w_intr_wq;
	struct mutex		access_ok;
	struct _sOvlySurface	surface;
	struct _sColorKeyNAlpha ckey_alpha;

	unsigned char		*hwc_buf;
	unsigned int		pseudo_palette[16];
	struct tasklet_struct	tasklet;
	char 			*mode_option;

	int			pix_fmt;
	unsigned		is_blanked:1;
	unsigned		cursor_enabled:1;
	unsigned		cursor_cfg:1;
	unsigned		active:1;
	unsigned		enabled:1;

	/*
	 * 0: DMA mem is from DMA region.
	 * 1: DMA mem is from normal region.
	 */
	unsigned		mem_status:1;

	/*
	 * current frame id for mapping to user.
	 */
	int			cur_fbid;
	int			src_mode;
};

/*
 * Dove LCD controller private state.
 */
struct dovefb_info {
	struct device			*dev;
	int				id;

	void				*reg_base;
	struct dovefb_layer_info	*gfx_plane;
	struct dovefb_layer_info	*vid_plane;

	struct fb_videomode		dft_vmode;
	struct fb_videomode		out_vmode;
	int				fixed_output;

	char 				*mode_option;
	struct clk			*clk;
	int				io_pin_allocation;

	int			pix_fmt;
	unsigned		edid:1;
	unsigned		panel_rbswap:1;
	unsigned		edid_en:1;
};

/*
 * Dove fb machine information
 */
struct dovefb_mach_info {
	char		id_gfx[16];
	char		id_ovly[16];
//KW	unsigned int	sclk_clock;

	int		num_modes;
	struct fb_videomode *modes;

	/* LCD reference clock value.	*/
	unsigned int	lcd_ref_clk;

	/* KW
	** FrameBuffer memory.
	*/
	void		*fb_mem[2];
	unsigned int	fb_mem_size[2];

	/*
	 * Pix_fmt
	 */
	unsigned	pix_fmt;

	/*
	 * I/O pin allocation.
	 */
	unsigned	io_pin_allocation:4;

	/*
	 * I2C bus to read DDC data through. -1 not available
	 */
	int		ddc_i2c_adapter;

	/*
	 * Dumb panel -- assignment of R/G/B component info to the 24
	 * available external data lanes.
	 */
	unsigned	panel_rgb_type:4;
	unsigned	panel_rgb_reverse_lanes:1;

	/*
	 * Dumb panel -- GPIO output data.
	 */
	unsigned	gpio_output_mask:8;
	unsigned	gpio_output_data:8;

	/*
	 * Dumb panel -- configurable output signal polarity.
	 */
	unsigned	invert_composite_blank:1;
	unsigned	invert_pix_val_ena:1;
	unsigned	invert_pixclock:1;
	unsigned	invert_vsync:1;
	unsigned	invert_hsync:1;
	unsigned	panel_rbswap:1;
	unsigned	active:1;
	unsigned	enable_lcd0:1;
};

struct dovebl_platform_data;

int clcd_platform_init(struct dovefb_mach_info *lcd0_dmi_data,
		       struct dovefb_mach_info *lcd0_vid_dmi_data,
		       struct dovebl_platform_data *backlight_data);


#endif /* _KERNEL_ */
#endif /* _DOVEFB_H_ */
