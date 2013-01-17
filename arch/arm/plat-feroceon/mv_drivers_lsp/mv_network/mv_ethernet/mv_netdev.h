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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/pci.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <net/ip.h>
#include <net/xfrm.h>

#include "mvOs.h"
#include "mvDebug.h"
#include "mvStack.h"
#include "mvSysHwConfig.h"
#include "eth/mvEth.h"
#include "eth-phy/mvEthPhy.h"
#include "ctrlEnv/sys/mvSysGbe.h"
#include "dbg-trace.h"
//#include "msApi.h"

#if defined(CONFIG_MV_ETH_TSO)
#   define ETH_INCLUDE_TSO
#endif /* CONFIG_MV_ETH_TSO */

#if defined(CONFIG_INET_LRO)
  /* LRO support */
#   include <linux/inet_lro.h>
#   define ETH_LRO
#   define ETH_LRO_DESC		64
#endif /* CONFIG_INET_LRO */

#if defined(CONFIG_MV_ETH_UFO)
  /* UFO support */
#   include <linux/udp.h>
#   define ETH_INCLUDE_UFO
#endif

#ifdef CONFIG_MV_ETH_NFP
#   include "eth/nfp/mvNfp.h"
#   include "../nfp_mgr/mv_nfp_mgr.h"
#endif /* CONFIG_MV_ETH_NFP */

#ifdef CONFIG_MV_ETH_TOOL
#include "mv_eth_tool.h"
#endif

#ifdef MY_ABC_HERE
#define MV_PHY_ID_131X 0x01410e90
#endif

/****************************************************** 
 * driver debug control --                            *
 ******************************************************/
/* debug main on/off switch (more in debug control below ) */
#define ETH_DEBUG
#undef ETH_DEBUG

#define ETH_DBG_OFF     0x0000
#define ETH_DBG_RX      0x0001
#define ETH_DBG_TX      0x0002
#define ETH_DBG_RX_FILL 0x0004
#define ETH_DBG_TX_DONE 0x0008
#define ETH_DBG_LOAD    0x0010
#define ETH_DBG_IOCTL   0x0020
#define ETH_DBG_INT     0x0040
#define ETH_DBG_STATS   0x0080
#define ETH_DBG_POLL    0x0100
#define ETH_DBG_GSO		0x0200
#define ETH_DBG_MCAST   0x0400
#define ETH_DBG_VLAN    0x0800
#define ETH_DBG_IGMP    0x1000
#define ETH_DBG_ALL     0xffff

#ifdef ETH_DEBUG
extern u32 eth_dbg;
# define ETH_DBG(FLG, X) if( (eth_dbg & (FLG)) == (FLG) ) printk X
#else
# define ETH_DBG(FLG, X)
#endif


/****************************************************** 
 * driver statistics control --                       *
 ******************************************************/
/* statistics main on/off switch (more in statistics control below ) */

#ifdef CONFIG_MV_ETH_STATS_ERROR
# define ETH_STAT_ERR(CODE) CODE;
#else
# define ETH_STAT_ERR(CODE) 
#endif /* CONFIG_MV_ETH_STATS_ERROR */ 

#ifdef CONFIG_MV_ETH_STATS_INFO
# define ETH_STAT_INFO(CODE) CODE;
#else
# define ETH_STAT_INFO(CODE)
#endif /* CONFIG_MV_ETH_STATS_INFO */

#ifdef CONFIG_MV_ETH_STATS_DEBUG
# define ETH_STAT_DBG(CODE) CODE;
#else
# define ETH_STAT_DBG(CODE)
#endif /* CONFIG_MV_ETH_STATS_DEBUG */

/* rx buffer size */ 
/* 2(HW hdr) + 4(VLAN) + 14(MAC hdr) + 4(CRC) */
#define MV_WRAP             (2 + 4 + ETH_HLEN + 4)  

#define MV_RX_BUF_SIZE(mtu) MV_ALIGN_UP(((mtu) + MV_WRAP), CPU_D_CACHE_LINE_SIZE)

/* Interrupt Cause Masks */
#define ETH_TXQ_MASK		(((1 << CONFIG_MV_ETH_TXQ) - 1) << ETH_CAUSE_TX_BUF_OFFSET) 
#define ETH_LINK_MASK		((1 << ETH_CAUSE_PHY_STATUS_CHANGE_BIT) | (1 << ETH_CAUSE_LINK_STATE_CHANGE_BIT))
#define ETH_RXQ_MASK		(((1 << CONFIG_MV_ETH_RXQ) - 1) << ETH_CAUSE_RX_READY_OFFSET)
#define ETH_RXQ_RES_MASK	(((1 << CONFIG_MV_ETH_RXQ) - 1) << ETH_CAUSE_RX_ERROR_OFFSET)

/* Gigabit Ethernet Port Interrupt Mask and Port Interrupt Mask Extend Registers (PIMR and PIMER) */
#define ETH_PICR_MASK		(BIT1  | ETH_RXQ_MASK | ETH_RXQ_RES_MASK)
/* phy/link-status-change, tx-done-q0 - q7 */
#define ETH_PICER_MASK		(ETH_LINK_MASK | ETH_TXQ_MASK) 

#if defined(CONFIG_MV_GATEWAY) 

#define GTW_MAX_NUM_OF_IFS  5

struct mv_vlan_cfg {
    char            name[IFNAMSIZ];
    unsigned int    ports_mask;
    unsigned int    ports_link;
    unsigned short  vlan_grp_id;
    unsigned short  header;
    unsigned char   macaddr[ETH_ALEN]; 
};

struct mv_gtw_config
{
    int                 vlans_num;
    int                 mtu;
    struct mv_vlan_cfg  vlan_cfg[GTW_MAX_NUM_OF_IFS];
};

extern struct mv_gtw_config    gtw_config;

#else
#define GTW_MAX_NUM_OF_IFS  0
#endif /* CONFIG_MV_GATEWAY */

typedef struct _eth_statistics
{
    /* erorrs */
    u32 skb_alloc_fail;
    u32 tx_timeout, tx_netif_stop;
    u32 tx_done_netif_wake;
    u32 tx_skb_no_headroom;
    u32 rx_pool_empty;

#ifdef CONFIG_MV_ETH_STATS_INFO
    /* Events */
    u32 irq_total, irq_none, irq_while_polling;
    u32 poll_events, poll_complete;
    u32 tx_events;
    u32 tx_done_events;
    u32 timer_events;
    u32 link_events;
#endif /* CONFIG_MV_ETH_STATS_INFO */

#ifdef CONFIG_MV_ETH_STATS_DEBUG
    /* rx stats */
    u32 rx_hal_ok[CONFIG_MV_ETH_RXQ];
    u32 rx_netif_drop;
    u32 rx_dist[CONFIG_MV_ETH_NUM_OF_RX_DESCR+1];
    u32 rx_csum_hw, rx_csum_hw_frags, rx_csum_sw;

    /* skb stats */
    u32 skb_alloc_ok, skb_free_ok;

    /* rx-fill stats */
    u32 rx_fill_ok[CONFIG_MV_ETH_RXQ];

    /* tx stats */
    u32 tx_hal_ok[CONFIG_MV_ETH_TXQ];
    u32 tx_hal_no_resource[MV_ETH_TX_Q_NUM];
    u32 tx_csum_hw, tx_csum_sw;
    u32 tso_stats[64], ufo_stats[64];

    /* tx-done stats */
    u32 tx_done_hal_ok[CONFIG_MV_ETH_TXQ];
    u32 tx_done_dist[CONFIG_MV_ETH_NUM_OF_TX_DESCR*MV_ETH_TX_Q_NUM + 1];

#ifdef ETH_MV_TX_EN
    u32 tx_en_done, tx_en_wait, tx_en_wait_count, tx_en_busy;
#endif /* ETH_MV_TX_EN */

#ifdef CONFIG_MV_ETH_SKB_REUSE
    u32 skb_reuse_rx, skb_reuse_tx, skb_reuse_alloc;
#endif /* CONFIG_MV_ETH_SKB_REUSE */

#ifdef CONFIG_NET_SKB_RECYCLE
	u32 skb_recycle_ok, skb_recycle_full, skb_recycle_rej;
#endif /* CONFIG_NET_SKB_RECYCLE */

#endif /* CONFIG_MV_ETH_STATS_DEBUG */

} eth_statistics;


/* Masks used for pp->flags */
#define MV_ETH_F_TIMER		0x01
#define MV_ETH_F_LINK_UP	0x02
#define MV_ETH_F_FORCED_LINK	0x04	/* port is connected to a Switch with forced link */


typedef struct _mv_eth_priv
{
    int                 port;
    void*               hal_priv;
    spinlock_t*         lock;
    eth_statistics      eth_stat;
    u32                 picr;
    u32                 picer;
    MV_STACK*           txPktInfoPool;
    int                 tx_count[CONFIG_MV_ETH_TXQ];
    struct timer_list   timer;
    unsigned int        flags; /* timer, link up etc. */
    unsigned int        skb_alloc_fail_cnt;
    struct net_device   *net_dev;		/* back reference to the net_device */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    struct napi_struct  napi;
#endif

#if defined(ETH_INCLUDE_TSO) || defined(ETH_INCLUDE_UFO) || defined(CONFIG_MV_GATEWAY)
#  define TX_EXTRA_BUF_SIZE     128

    char**  tx_extra_bufs[CONFIG_MV_ETH_TXQ]; 
    int     tx_extra_buf_idx[CONFIG_MV_ETH_TXQ];
#endif /* ETH_INCLUDE_TSO || ETH_INCLUDE_UFO */

#ifdef ETH_LRO
    struct net_lro_mgr  lro_mgr;
    struct net_lro_desc lro_desc[ETH_LRO_DESC];
    unsigned int 	lro_en;	/* enable */
#endif /* ETH_LRO */

#ifdef CONFIG_MV_ETH_SKB_REUSE
    MV_STACK*       skbReusePool;
#endif /* CONFIG_MV_ETH_SKB_REUSE */

#ifdef CONFIG_MV_ETH_NFP
    MV_FP_STATS     fpStats;
#endif /* CONFIG_MV_ETH_NFP */

    MV_STACK*       rxPool;
    int             rx_buf_size;

#if defined(CONFIG_MV_GATEWAY)
    int             isGtw;
#endif /* CONFIG_MV_GATEWAY */

#ifdef ETH_MV_TX_EN
    MV_BOOL         tx_en;
    int             tx_en_deep;
    MV_BOOL         tx_en_bk;
#endif /* ETH_MV_TX_EN */

#ifdef RX_CSUM_OFFLOAD
    MV_U32	rx_csum_offload;
#endif

#ifdef CONFIG_MV_ETH_TOOL
    int		phy_id;
    __u16	speed_cfg;
    __u8	duplex_cfg;
    __u8 	autoneg_cfg;
#endif /* CONFIG_MV_ETH_TOOL */
    MV_U32	rx_coal_usec;
    MV_U32	tx_coal_usec;

#ifdef MY_ABC_HERE
	MV_U32	phy_chip;
	MV_U32  wol;
#endif
} mv_eth_priv; 

typedef struct _mv_priv 
{
    mv_eth_priv			*giga_priv;
#ifdef CONFIG_MV_GATEWAY
    struct mv_vlan_cfg		*vlan_cfg;	/* reference to entry in net config table */
#endif

} mv_net_priv;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
#define MV_ETH_PRIV(dev)        (((mv_net_priv*)((dev)->priv))->giga_priv)
#define MV_NETDEV_VLAN(dev)     (((mv_net_priv*)((dev)->priv))->vlan_cfg)
#else
#define MV_ETH_PRIV(dev)        (((mv_net_priv*)(netdev_priv(dev)))->giga_priv)
#define MV_NETDEV_VLAN(dev)     (((mv_net_priv*)(netdev_priv(dev)))->vlan_cfg)
#endif
#define MV_NETDEV_STATS(dev)    (&((dev)->stats))
#define MV_GTW_VLAN_TO_GROUP(vid)	((((vid) & 0xf00) >> 8)-1)

extern int     mv_eth_rxq_desc[CONFIG_MV_ETH_RXQ];
extern int     mv_eth_txq_desc[CONFIG_MV_ETH_TXQ];
extern int     mv_eth_rx_desc_total;
extern int     mv_eth_tx_desc_total;

extern int      mv_eth_tx_done_quota;

extern spinlock_t           mii_lock;
extern spinlock_t           nfp_lock;

extern struct net_device**  mv_net_devs;
extern int                  mv_net_devs_num;

extern mv_eth_priv**        mv_eth_ports;
extern int                  mv_eth_ports_num;

#ifdef CONFIG_MV_ETH_SKB_REUSE
extern int mv_eth_skb_reuse_enable;
#endif /* CONFIG_MV_ETH_SKB_REUSE */

#ifdef CONFIG_NET_SKB_RECYCLE
extern int mv_eth_skb_recycle_enable;
#endif /* CONFIG_NET_SKB_RECYCLE */

static INLINE int  mv_eth_tos_to_q_map(unsigned char tos, unsigned char num_of_queues)
{
    int  prio;

    if(tos == 0)
        return 0;

    prio = (tos >> 5);
    if(prio == 0)
        return 1;

    if(prio >= num_of_queues)
        return (num_of_queues - 1);
    
    return prio;
}

static INLINE void mv_eth_save_interrupts(mv_eth_priv *priv)
{
    priv->picr |= MV_REG_READ(ETH_INTR_CAUSE_REG(priv->port)) & ETH_PICR_MASK;
    if(priv->picr & BIT1) 
    {
	    priv->picer |= MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(priv->port)) & ETH_PICER_MASK;
    }
}
/**************************************************************************************************************/

static INLINE void mv_eth_mask_interrupts(mv_eth_priv *priv)
{
    MV_REG_WRITE( ETH_INTR_MASK_REG(priv->port), 0 );
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(priv->port), 0 );
}

/**************************************************************************************************************/

static INLINE void mv_eth_unmask_interrupts(mv_eth_priv *priv)
{
    /* unmask GbE Rx and Tx interrupts */
    MV_REG_WRITE( ETH_INTR_MASK_REG(priv->port), ETH_PICR_MASK);
#ifdef ETH_TX_DONE_ISR
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(priv->port), ETH_PICER_MASK); 
#else
    MV_REG_WRITE( ETH_INTR_MASK_EXT_REG(priv->port), ETH_LINK_MASK); 
#endif /* ETH_TX_DONE_ISR */
}
/**************************************************************************************************************/

static INLINE void mv_eth_clear_saved_interrupts(mv_eth_priv *priv)
{
    /* clear interrupts */
    MV_REG_WRITE( ETH_INTR_CAUSE_REG(priv->port), ~(priv->picr));
    /* clear Tx interrupts */
    MV_REG_WRITE( ETH_INTR_CAUSE_EXT_REG(priv->port), ~(priv->picer));
}
/**************************************************************************************************************/

static INLINE void mv_eth_clear_interrupts(mv_eth_priv *priv)
{
    /* clear interrupts */
    MV_REG_WRITE( ETH_INTR_CAUSE_REG(priv->port), 0 );
    /* clear Tx interrupts */
    MV_REG_WRITE( ETH_INTR_CAUSE_EXT_REG(priv->port), 0 );
}
/**************************************************************************************************************/

/* Function prototypes */

#ifdef CONFIG_MV_GATEWAY
extern int  mv_gtw_start(struct net_device *dev);
extern int  mv_gtw_stop(struct net_device *dev);
extern int  mv_gtw_change_mtu(struct net_device *dev, int mtu);
extern int  mv_gtw_set_mac_addr( struct net_device *dev, void *mac );
extern void mv_gtw_set_multicast_list(struct net_device *dev);

extern int  __init mv_gtw_net_setup(int port);
extern int __init mv_gtw_init_complete(mv_eth_priv* priv);

extern int  mv_gtw_switch_tos_get(int port, unsigned char codepoint);
extern int  mv_gtw_switch_tos_set(int port, unsigned char codepoint, int queue);

extern void mv_gtw_switch_stats(int port);

#ifdef CONFIG_MV_GTW_IGMP
int mv_gtw_igmp_snoop_process(struct sk_buff* skb, unsigned char port, unsigned char vlan);
#endif /* CONFIG_MV_GTW_IGMP */

/* Note: this function currently supports only working with Marvell Header mode */
extern int mv_gtw_dev_offset;
static INLINE struct net_device *mv_gtw_ingress_dev(char* data)
{
	return mv_net_devs[(((data[0] & 0xF0) >> 4) + mv_gtw_dev_offset)];
}

static INLINE char mv_gtw_ingress_port(char* data)
{
    return data[1] & 0x0F;	
}

static INLINE char mv_gtw_ingress_vlan(char* data)
{
    return (data[0] & 0xF0) >> 4;
}

static INLINE void mv_gtw_update_tx_skb(struct net_device *dev, MV_PKT_INFO *pPktInfo)
{
	/* Note: this function currently supports only working with Marvell Header mode */
	struct mv_vlan_cfg  *vlan_cfg = MV_NETDEV_VLAN(dev); 
	struct sk_buff      *skb = (struct sk_buff *)pPktInfo->osInfo;
	MV_BUF_INFO         *p_buf_info_first = pPktInfo->pFrags;
	MV_BUF_INFO         *p_buf_info_last = (pPktInfo->pFrags + pPktInfo->numFrags - 1);

	/* check if we have place inside skb for the header */
	if(skb_headroom(skb) >= ETH_MV_HEADER_SIZE) {
		*(unsigned short *)((skb->data)-ETH_MV_HEADER_SIZE) = vlan_cfg->header;
		pPktInfo->pFrags[0].bufVirtPtr -= ETH_MV_HEADER_SIZE;
		pPktInfo->pFrags[0].dataSize += ETH_MV_HEADER_SIZE;
		pPktInfo->pktSize += ETH_MV_HEADER_SIZE;
	}
	else {
#ifdef CONFIG_MV_ETH_STATS_ERR
            mv_eth_priv *priv = MV_ETH_PRIV(dev);
            priv->eth_stat.tx_skb_no_headroom++;
#endif /* CONFIG_MV_ETH_STATS_ERR */

		/* make room for one cell (safe because the array is big enough) */
		do {
	        	*(p_buf_info_last+1) = *p_buf_info_last;
        		p_buf_info_last--;
		} while(p_buf_info_last >= p_buf_info_first);

		/* the header (safe on stack) */		
		p_buf_info_first->bufVirtPtr = (void*)&vlan_cfg->header;
		p_buf_info_first->dataSize = ETH_MV_HEADER_SIZE;

		/* count the new frags */
		pPktInfo->numFrags += 1;
		pPktInfo->pktSize += ETH_MV_HEADER_SIZE;
	}
}
#endif /* CONFIG_MV_GATEWAY */

extern int  mv_eth_open(struct net_device *dev);
extern int  mv_eth_start(struct net_device *dev);
extern int  mv_eth_stop(struct net_device *dev);
extern int  mv_eth_change_mtu(struct net_device *dev, int mtu);
extern int  mv_eth_set_mac_addr( struct net_device *dev, void *mac );
extern void mv_eth_set_multicast_list(struct net_device *dev);
void mv_eth_rxcoal_set(int port, MV_U32 rx_coal);
void mv_eth_txcoal_set(int port, MV_U32 tx_coal);

int __init                  mv_eth_hal_init(mv_eth_priv *priv, int mtu, u8* mac);
int __init                  mv_eth_priv_init(mv_eth_priv *priv, int port);
struct net_device* __init   mv_eth_netdev_init(mv_eth_priv *priv, int mtu, u8* mac);
void __init                 mv_eth_priv_cleanup(mv_eth_priv *priv);

void	            mv_eth_config_show(void);
void                mv_eth_netdev_print(unsigned int idx);
void                mv_eth_status_print( unsigned int port );
void                mv_eth_stats_print( unsigned int port );
void                mv_eth_set_noqueue(int port, int enable);
void                mv_eth_set_lro(int port, int enable);
void                mv_eth_set_lro_desc(int port, unsigned int value);

MV_STATUS           mv_eth_rx_fill(mv_eth_priv *priv, int pool_size, int mtu);
irqreturn_t         mv_eth_interrupt_handler(int irq , void *dev_id);
int                 mv_eth_start_internals( mv_eth_priv *priv, int mtu );
int                 mv_eth_stop_internals(mv_eth_priv *priv);
int                 mv_eth_down_internals( struct net_device *dev );
int                 mv_eth_change_mtu_internals( struct net_device *dev, int mtu );

void                mv_eth_tos_map_show(int port);
int                 mv_eth_tos_map_set(int port, unsigned char tos, int queue);

void                print_skb(struct sk_buff* skb);

#ifdef CONFIG_MV_ETH_NFP
void                mv_eth_nfp_stats_print(unsigned int port);
#endif /* CONFIG_MV_ETH_NFP */

#ifdef ETH_MV_TX_EN
void                eth_tx_en_config(int port, int value);
#endif /* ETH_MV_TX_EN */

