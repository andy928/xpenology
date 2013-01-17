/*
 * linux/drivers/video/marvell/dovefb_if.h -- Interface functions for Marvell
 * GFX & Overlay FB drivers.
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 *
 */


int dovefb_gfx_handle_irq(u32 isr, struct dovefb_layer_info *dfli);
int dovefb_gfx_init(struct dovefb_info *info, struct dovefb_mach_info *dmi);
int dovefb_gfx_suspend(struct dovefb_layer_info *dfli, pm_message_t mesg);
int dovefb_gfx_resume(struct dovefb_layer_info *dfli);

int dovefb_ovly_handle_irq(u32 isr, struct dovefb_layer_info *dfli);
int dovefb_ovly_init(struct dovefb_info *info, struct dovefb_mach_info *dmi);
int dovefb_ovly_suspend(struct dovefb_layer_info *dfli, pm_message_t mesg);
int dovefb_ovly_resume(struct dovefb_layer_info *dfli);

int dovefb_determine_best_pix_fmt(struct fb_var_screeninfo *var,
		struct dovefb_layer_info *dfli);
unsigned int dovefb_dump_regs(struct dovefb_info *dfi);
int dovefb_check_var(struct fb_var_screeninfo *var, struct fb_info *fi);
int dovefb_setcolreg(unsigned int regno, unsigned int red, unsigned int green,
		unsigned int blue, unsigned int trans, struct fb_info *fi);
void dovefb_set_pix_fmt(struct fb_var_screeninfo *var, int pix_fmt);
void dovefb_set_mode(struct dovefb_layer_info *dfli,
		struct fb_var_screeninfo *var, struct fb_videomode *mode,
		int pix_fmt, int ystretch);


extern struct fb_ops dovefb_gfx_ops;
extern struct fb_ops dovefb_ovly_ops;


