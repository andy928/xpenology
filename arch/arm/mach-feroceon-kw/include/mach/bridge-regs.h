/*
 * arch/arm/mach-feroceon-kw/include/mach/bridge-regs.h
 *
 * Mbus-L to Mbus Bridge Registers
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_BRIDGE_REGS_H
#define __ASM_ARCH_BRIDGE_REGS_H


#include "ctrlEnv/sys/mvCpuIfRegs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"


#define FEROCEON_REGS_VIRT_BASE	(INTER_REGS_BASE | MAX_AHB_TO_MBUS_REG_BASE)

#define CPU_CONTROL			(FEROCEON_REGS_VIRT_BASE | CPU_CTRL_STAT_REG)
#define TIMER_VIRT_BASE		(FEROCEON_REGS_VIRT_BASE | CNTMR_BASE)
#define RSTOUTn_MASK			(FEROCEON_REGS_VIRT_BASE | CPU_RSTOUTN_MASK_REG)
#define BRIDGE_CAUSE			(FEROCEON_REGS_VIRT_BASE | CPU_AHB_MBUS_CAUSE_INT_REG)

#define WDT_RESET_OUT_EN		0x00000002
#define WDT_INT_REQ			0x0008


#endif
