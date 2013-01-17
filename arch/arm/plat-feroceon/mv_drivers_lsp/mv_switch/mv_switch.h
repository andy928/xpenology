/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
*******************************************************************************/
#ifndef MV_SWITCH_MV_SWITCH_H
#define MV_SWITCH_MV_SWITCH_H

#define MV_SWITCH_PHY_ACCESS			1
#define MV_SWITCH_PORT_ACCESS			2
#define MV_SWITCH_GLOBAL_ACCESS			3
#define MV_SWITCH_GLOBAL2_ACCESS		4
#define MV_SWITCH_SMI_ACCESS            5

//extern GT_QD_DEV *qd_dev;

int mv_switch_load(unsigned int switch_ports_mask);
int mv_switch_reg_read(int port, int reg, int type, MV_U16 *value);
int mv_switch_reg_write(int port, int reg, int type, MV_U16 value);

void    mv_switch_stats_print(void);
void    mv_switch_status_print(void);


#endif
