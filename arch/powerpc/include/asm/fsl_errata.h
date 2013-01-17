/*
 * Copyright (C) 2007 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Gong Chen, g.chen@freescale.com
 *
 * Description:
 * This file defines the errata macro. This file is part of the Linux kernel
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */


#ifdef __KERNEL__
#ifndef __ASM_FSL_ERRATA_H__
#define __ASM_FSL_ERRATA_H__

#ifdef CONFIG_SYNO_MPC85XX_COMMON

#define SVR_MAJ(svr)    (((svr) >>  4) & 0xF)   /* Major revision field*/
#define SVR_MIN(svr)    (((svr) >>  0) & 0xF)   /* Minor revision field*/

#define MPC8548_ERRATA(maj, min) \
       ((0x80210000 == (0xFFFF0000 & mfspr(SPRN_PVR))) && \
	(SVR_MAJ(mfspr(SPRN_SVR)) <= maj) && (SVR_MIN(mfspr(SPRN_SVR)) <= min))
#else

#define MPC8548_ERRATA(maj, min) (0)

#endif

#endif
#endif


