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
#ifndef __mv_eth_tool
#define __mv_eth_tool

#include <linux/ethtool.h>

#define MV_ETH_TOOL_AN_TIMEOUT	2000

extern const struct ethtool_ops mv_eth_tool_ops;

int mv_eth_tool_read_mdio
		(struct net_device *netdev, int addr, int reg);
void mv_eth_tool_write_mdio
		(struct net_device *netdev, int addr, int reg, int data);
int mv_eth_tool_restore_settings(struct net_device *netdev);
int mv_eth_tool_get_settings
		(struct net_device *netdev, struct ethtool_cmd *cmd);
int mv_eth_tool_set_settings
		(struct net_device *netdev, struct ethtool_cmd *cmd);
int mv_eth_tool_nway_reset
		(struct net_device *netdev);
int mv_eth_tool_get_regs_len
		(struct net_device *netdev);
void mv_eth_tool_get_regs
		(struct net_device *netdev, struct ethtool_regs *regs, void *p);
u32 mv_eth_tool_get_link
		(struct net_device *netdev);
int mv_eth_tool_get_coalesce
		(struct net_device *netdev, struct ethtool_coalesce *cmd);
int mv_eth_tool_set_coalesce
		(struct net_device *netdev, struct ethtool_coalesce *cmd);
void mv_eth_tool_get_ringparam
		(struct net_device *netdev, struct ethtool_ringparam *ring);
void mv_eth_tool_get_pauseparam
		(struct net_device *netdev, struct ethtool_pauseparam *pause);
int mv_eth_tool_set_pauseparam
		(struct net_device *netdev, struct ethtool_pauseparam *pause);
void mv_eth_tool_get_drvinfo
		(struct net_device *netdev, struct ethtool_drvinfo *info);
u32 mv_eth_tool_get_rx_csum
		(struct net_device *netdev);
int mv_eth_tool_set_rx_csum
		(struct net_device *netdev, uint32_t data);
int mv_eth_tool_set_tx_csum
		(struct net_device *netdev, uint32_t data);
int mv_eth_tool_set_tso
		(struct net_device *netdev, uint32_t data);
int mv_eth_tool_set_ufo
		(struct net_device *netdev, uint32_t data);
void mv_eth_tool_get_strings
		(struct net_device *netdev, uint32_t stringset, uint8_t *data);
int mv_eth_tool_phys_id
		(struct net_device *netdev, u32 data);
int mv_eth_tool_get_stats_count(struct net_device *netdev, int stringset);
void mv_eth_tool_get_ethtool_stats
		(struct net_device *netdev, struct ethtool_stats *stats, uint64_t *data);

#endif /* __mv_eth_tool */
