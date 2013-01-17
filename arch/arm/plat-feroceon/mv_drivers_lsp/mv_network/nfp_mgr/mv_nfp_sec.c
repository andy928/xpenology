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

#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/skbuff.h>
#include <linux/random.h>
#include <asm/scatterlist.h>
#include <linux/spinlock.h>
#include "ctrlEnv/sys/mvSysCesa.h"
#include "cesa/mvCesa.h"
#include "../../../common/mv802_3.h"
#include "eth/nfp/mvNfpSec.h"
#include "mvDebug.h"
#include "eth/mvEth.h"
#include "eth-phy/mvEthPhy.h"
#include "ctrlEnv/sys/mvSysGbe.h"
#include "../mv_ethernet/mv_netdev.h"

#include "cesa/mvMD5.h"
#include "cesa/mvSHA1.h"

#include "cesa/mvCesaRegs.h"
#include "cesa/AES/mvAes.h"
#include "cesa/mvLru.h"

#undef RT_DEBUG
#undef dprintk
#ifdef RT_DEBUG
#define dprintk(a...)    printk(a)
#else
#define dprintk(a...)
#endif

#define MV_NFP_SEC_MAX_SES	100

extern u32 mv_crypto_base_get(void);
extern void eth_check_for_tx_done(void);
extern MV_U8 mvFpPortsGet(MV_IP_HEADER* pIpHdr, MV_U16* pDstPort, MV_U16* pSrcPort);

static struct tasklet_struct nfp_sec_tasklet;
static spinlock_t 	nfp_sec_lock;

int nfp_sec_sent = 0;

static wait_queue_head_t   nfp_sec_waitq;
atomic_t req_count;
static MV_NFP_SEC_CESA_PRIV * req_array[MV_NFP_SEC_REQ_Q_SIZE];
unsigned int req_empty = 0;
unsigned int req_ready = 0;


/* TBD -  should be used from netdev.c */
INLINE void  nfp_sec_pkt_info_free(MV_PKT_INFO* pPktInfo)
{
	MV_BUF_INFO *pBuf = pPktInfo->pFrags;
	mv_eth_priv *src_priv = mv_eth_ports[(int)pPktInfo->ownerId];
	
	if (pBuf->bufAddrShift) {
		pBuf->bufPhysAddr += pBuf->bufAddrShift;
		pBuf->bufVirtPtr += pBuf->bufAddrShift;
		pBuf->bufAddrShift = 0;
	}
/*
	// for debug:	
	mvEthRxDoneTest(pBuf, CONFIG_NET_SKB_HEADROOM);
*/	
	/* Return to the NFP pool */
	mvStackPush(src_priv->rxPool, (MV_U32)pPktInfo);
}

/* TDB - move to h file */
static INLINE MV_VOID mvNfpSecClearRange(MV_U8* addr, MV_U32 size)
{
	MV_U32 i;
	MV_U8  *align;

	align = (MV_U8*)((MV_U32)addr & ~0x1f);
	
	for(i = 0; align <= (addr+size); align += CPU_D_CACHE_LINE_SIZE)
		mvOsCacheLineFlushInv(NULL, align);
}

static INLINE MV_VOID mvNfpSecInvRange(MV_U8* addr, MV_U32 size)
{
	MV_U32 i;
	MV_U8  *align;

	align = (MV_U8*)((MV_U32)addr & ~0x1f);
	
	for(i = 0; align <= (addr+size); align += CPU_D_CACHE_LINE_SIZE)
		mvOsCacheLineInv(NULL, align);
}



static inline void mvNfpSecBuildMac(MV_PKT_INFO *pPktInfo, MV_NFP_SEC_SA_ENTRY* pSAEntry)
{
        MV_802_3_HEADER *pMacHdr;

        pMacHdr = (MV_802_3_HEADER*)((MV_U8 *)(pPktInfo->pFrags[0].bufVirtPtr));
        memcpy(pMacHdr, &pSAEntry->tunnelHdr.dstMac, 12);
        pMacHdr->typeOrLen = 0x08;  /* stands for IP protocol code 16bit swapped */          
	return;	
}

static void inline 
nfp_sec_complete_out(unsigned long data) 
{
	MV_NFP_SEC_CESA_PRIV *nfp_sec_cesa_priv = (MV_NFP_SEC_CESA_PRIV *)data;
	MV_U32            if_out,if_type,if_ppp;
	MV_PKT_INFO       *pPktInfo;
	MV_BUF_INFO       *pBuf;
	mv_eth_priv       *out_priv;
	struct net_device *out_dev;
	int txq = ETH_DEF_TXQ;
	MV_STATUS status;

	if_ppp = 0;
	if_out = nfp_sec_cesa_priv->pSaEntry->tunnelHdr.outIfIndex;
	if_type = fp_mgr_get_if_type(if_out);

#ifdef CONFIG_MV_ETH_NFP_PPP
	if (if_type == MV_FP_IF_PPP)
	{
		if_ppp = if_out;
		if_out = mvFpPppPhyIf(if_ppp);
		if_type = fp_mgr_get_if_type(if_out);
	}
#endif
	if (if_type != MV_FP_IF_INT)
	{
		printk("%s: NFP Tx on ifindex=%d is not supported\n", __FUNCTION__, if_out);
		nfp_sec_cesa_priv->pSaEntry->stats.dropped++;
		nfp_sec_pkt_info_free(nfp_sec_cesa_priv->pPktInfo);
		return;
	}

	out_dev = fp_mgr_get_net_dev(if_out);
	out_priv = MV_ETH_PRIV(out_dev);

	pPktInfo = nfp_sec_cesa_priv->pPktInfo;
	pBuf = pPktInfo->pFrags;
	pBuf->bufVirtPtr -= MV_NFP_SEC_ESP_OFFSET + ETH_MV_HEADER_SIZE;
	pBuf->bufPhysAddr -= MV_NFP_SEC_ESP_OFFSET + ETH_MV_HEADER_SIZE;
	pBuf->dataSize += MV_NFP_SEC_ESP_OFFSET + ETH_MV_HEADER_SIZE;
	pBuf->bufAddrShift += MV_NFP_SEC_ESP_OFFSET + ETH_MV_HEADER_SIZE;

#ifdef CONFIG_MV_ETH_NFP_PPP
	/* PPPoE */
	if (if_ppp)
		mvFpPPPoE(if_ppp, pPktInfo, &out_priv->fpStats);
#endif

#if defined(CONFIG_MV_GATEWAY)
	if(out_priv->isGtw)
	{
		struct mv_vlan_cfg* vlan_cfg = MV_NETDEV_VLAN(out_dev);
		*(unsigned short *)(pBuf->bufVirtPtr) = vlan_cfg->header;
		mvOsCacheLineFlushInv(NULL, pBuf->bufVirtPtr);
	}
    else
#endif
	{
		pBuf->bufPhysAddr += ETH_MV_HEADER_SIZE;
		pBuf->dataSize -= ETH_MV_HEADER_SIZE;
	}

    spin_lock(out_priv->lock);
	status = mvEthPortTx(out_priv->hal_priv, txq, pPktInfo);
	spin_unlock(out_priv->lock);

    if (status != MV_OK)
    {
		printk("%s: mvEthPortTx failed 0x%x\n", __FUNCTION__, status);
#ifdef CONFIG_MV_GATEWAY
		if(!out_priv->isGtw)
			pBuf->bufPhysAddr -= ETH_MV_HEADER_SIZE;
#else
		pBuf->bufPhysAddr -= ETH_MV_HEADER_SIZE;
#endif /* CONFIG_MV_GATEWAY */
	
		nfp_sec_pkt_info_free(pPktInfo);
		out_dev->stats.tx_dropped++;
		return;
	}

	nfp_sec_sent++;
	out_priv->tx_count[txq]++;
	out_dev->stats.tx_packets++;
	out_dev->stats.tx_bytes += pBuf->dataSize;
	ETH_STAT_DBG( out_priv->eth_stat.tx_hal_ok[txq]++);

#if !defined(ETH_TX_DONE_ISR)
  	if (nfp_sec_sent > 32)
    {
		nfp_sec_sent = 0;
		eth_check_for_tx_done();
    }
#endif /* !ETH_TX_DONE_ISR */
}

static void inline 
nfp_sec_complete_in(unsigned long data) 
{
	MV_NFP_SEC_CESA_PRIV * nfp_sec_cesa_priv = (MV_NFP_SEC_CESA_PRIV *)data;
	MV_U8 if_out, proto = 0, *pNewDigest;
	MV_PKT_INFO *pPktInfo = nfp_sec_cesa_priv->pPktInfo;
	MV_BUF_INFO *pBuf = pPktInfo->pFrags;
	MV_NFP_SEC_SA_ENTRY* pSAEntry =  nfp_sec_cesa_priv->pSaEntry;
	mv_eth_priv  *out_priv;
	struct net_device *out_dev = NULL;
	int txq = ETH_DEF_TXQ;
	MV_STATUS status;
	MV_U32 dip, sip, calc_digest_off, size;
	MV_U16 sport = 0, dport = 0;
	MV_IP_HEADER* pIpHdr;
	MV_NFP_SEC_SPD_RULE *pSpd;

	/* align pointers to MAC header */
	size = sizeof(MV_ESP_HEADER) + pSAEntry->ivSize - sizeof(MV_802_3_HEADER);
	pBuf->bufAddrShift -= size;
	pBuf->bufVirtPtr += size;
	pBuf->bufPhysAddr += size;
	pBuf->dataSize -= (size + pSAEntry->digestSize);
	calc_digest_off = pBuf->dataSize;

	/* extract parameters for SPD match search */
	pIpHdr = (MV_IP_HEADER*)(pBuf->bufVirtPtr + sizeof(MV_802_3_HEADER));
#ifdef	MV_NFP_SEC_5TUPLE_KEY_SUPPORT 
	proto = mvFpPortsGet(pIpHdr, &dport, &sport);
#endif
	dip = pIpHdr->dstIP;
    sip = pIpHdr->srcIP;
	
	/* remove crypto padding */
	pBuf->dataSize = sizeof(MV_802_3_HEADER) + MV_16BIT_BE(pIpHdr->totalLength);

	/* check what to do with the packet according to SPD */
	pSpd = mvNfpSecSPDRuleFind(dip, sip, proto, dport, sport, MV_NFP_SEC_RULE_DB_IN);
	if(pSpd == NULL) {
		printk("%s(%d): SPD rule not found\n", __FUNCTION__, __LINE__);
		printk("dip(0x%x) sip(0x%x) proto(0x%x) dport(0x%x) sport(0x%x), dataSize(0x%x)\n",
			dip, sip, proto, dport, sport, pBuf->dataSize);
		pBuf->bufVirtPtr -= ETH_MV_HEADER_SIZE;
		pBuf->bufPhysAddr -= ETH_MV_HEADER_SIZE;
		pBuf->bufAddrShift += ETH_MV_HEADER_SIZE;
		nfp_sec_pkt_info_free(pPktInfo);
		return;
	}

	/* compare digest */
	pNewDigest = pBuf->bufVirtPtr + calc_digest_off;
	if(memcmp(nfp_sec_cesa_priv->orgDigest, pNewDigest, pSAEntry->digestSize)) {
		printk("%s(%d): ERR. original digest is different from calculated digest(size=%d)\n", __FUNCTION__, __LINE__,calc_digest_off);
		pBuf->bufVirtPtr -= ETH_MV_HEADER_SIZE;
		pBuf->bufPhysAddr -= ETH_MV_HEADER_SIZE;
		pBuf->bufAddrShift += ETH_MV_HEADER_SIZE;
		nfp_sec_pkt_info_free(pPktInfo);
		return;
	}

	/*
	 * Hub and spoke vpn. Check 2nd tunnel
	 */
#ifdef CONFIG_MV_ETH_NFP_SEC_HUB
	/* ipheader of the original packet */
	pIpHdr = (MV_IP_HEADER*)(pBuf->bufVirtPtr + sizeof(MV_802_3_HEADER));
	proto = mvFpPortsGet(pIpHdr, &dport, &sport);
	sip = pIpHdr->srcIP;
	dip = pIpHdr->dstIP;

	/* check legitimacy for entring the 2nd tunnel */
	pSpd = mvNfpSecSPDRuleFind(dip, sip, proto, dport, sport, MV_NFP_SEC_RULE_DB_OUT);
	if (pSpd && pSpd->actionType == MV_NFP_SEC_SECURE)
	{
		pBuf->bufVirtPtr -= ETH_MV_HEADER_SIZE;
		pBuf->bufPhysAddr -= ETH_MV_HEADER_SIZE;
		pBuf->dataSize += ETH_MV_HEADER_SIZE;
		pBuf->bufAddrShift += ETH_MV_HEADER_SIZE;

		status = mvNfpSecOutgoing(pPktInfo, pSpd->pSAEntry);
		if (status != MV_OK)
		{
			if (pSpd->pSAEntry)
				pSpd->pSAEntry->stats.dropped++;
			nfp_sec_pkt_info_free(pPktInfo);
			mvOsPrintf("nfp_sec_complete_in: hub-in-spoke status=%x\n", status);
		}

		return;
	}		
#endif

	/* build MAC header */	
	mvNfpSecBuildMac(pPktInfo, pSAEntry);

	/* flush & invalidate MAC,IP & TCP headers */ 
	size = sizeof(MV_802_3_HEADER)+ sizeof(MV_IP_HEADER);
#ifdef MV_NFP_SEC_5TUPLE_KEY_SUPPORT
	size += sizeof(MV_TCP_HEADER);
#endif

	mvNfpSecClearRange(pBuf->bufVirtPtr, size);
	mvNfpSecInvRange(pNewDigest, pSAEntry->digestSize);

	if_out = pSAEntry->tunnelHdr.outIfIndex;
	out_dev = fp_mgr_get_net_dev(if_out);

	if(fp_mgr_get_if_type(if_out) != MV_FP_IF_INT)
	{
		printk("%s: NFP Tx on ifindex=%d is not supported\n", __FUNCTION__, if_out);
		pSAEntry->stats.dropped++;
		nfp_sec_pkt_info_free(pPktInfo);
		return;
	}

	pBuf->bufVirtPtr -= ETH_MV_HEADER_SIZE;
	pBuf->bufPhysAddr -= ETH_MV_HEADER_SIZE;
	pBuf->dataSize += ETH_MV_HEADER_SIZE;
	pBuf->bufAddrShift += ETH_MV_HEADER_SIZE;

	out_priv = MV_ETH_PRIV(out_dev);

#if defined(CONFIG_MV_GATEWAY)
	if(out_priv->isGtw)
	{
		struct mv_vlan_cfg* vlan_cfg = MV_NETDEV_VLAN(out_dev);
		*(unsigned short *)(pBuf->bufVirtPtr) = vlan_cfg->header;
		mvOsCacheLineFlushInv(NULL, pBuf->bufVirtPtr);
	}
    else
#endif
	{
		pBuf->bufPhysAddr += ETH_MV_HEADER_SIZE;
		pBuf->dataSize -= ETH_MV_HEADER_SIZE;
	}

	spin_lock(out_priv->lock);
	status = mvEthPortTx(out_priv->hal_priv, txq, pPktInfo);
	spin_unlock(out_priv->lock);

    if (status != MV_OK)
    {

		printk("%s: mvEthPortTx failed 0x%x\n", __FUNCTION__, status);
#ifdef CONFIG_MV_GATEWAY
        if(!out_priv->isGtw)
			pBuf->bufPhysAddr -= ETH_MV_HEADER_SIZE;
#else
        	pBuf->bufPhysAddr -= ETH_MV_HEADER_SIZE;
#endif /* CONFIG_MV_GATEWAY */

		nfp_sec_pkt_info_free(pPktInfo);
		out_dev->stats.tx_dropped++;
		return;
	}

	nfp_sec_sent++;
	out_priv->tx_count[txq]++;
	out_dev->stats.tx_packets++;
	out_dev->stats.tx_bytes += pBuf->dataSize;
	ETH_STAT_DBG( out_priv->eth_stat.tx_hal_ok[txq]++);
    
#if !defined(ETH_TX_DONE_ISR)
  	if (nfp_sec_sent > 32)
	{
		nfp_sec_sent = 0;
		eth_check_for_tx_done();
    }
#endif /* !ETH_TX_DONE_ISR */
}


/*
 * nfp sec callback. 
 */
void
req_handler(unsigned long dummy)
{
	while(atomic_read(&req_count) != 0) {
		if(req_array[req_ready]->pSaEntry->secOp == MV_NFP_SEC_ENCRYPT)
			nfp_sec_complete_out((unsigned long)req_array[req_ready]);
		else
			nfp_sec_complete_in((unsigned long)req_array[req_ready]);
		req_ready++;
		if(req_ready == MV_NFP_SEC_REQ_Q_SIZE)
			req_ready = 0;
		
		atomic_dec(&req_count);
	}

}

/*
 * nfp sec Interrupt handler routine.
 */
static irqreturn_t
nfp_sec_interrupt_handler(int irq, void *arg)
{
	MV_CESA_RESULT  	result;
	MV_STATUS               status;

	/* clear interrupts */
    	MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG, 0);
#if defined(MV_CESA_CHAIN_MODE_SUPPORT)
	while(1) {
#endif
	/* Get Ready requests */
	status = mvCesaReadyGet(&result);
	if(status != MV_OK)
	{
#if !defined(MV_CESA_CHAIN_MODE_SUPPORT)
		printk("ERROR: Ready get return %d\n",status);
		return IRQ_HANDLED;
#else
			break;
#endif
	}	

	/* handle result */
	// TBD - need to verify return code !!!
	if(atomic_read(&req_count) > (MV_NFP_SEC_REQ_Q_SIZE - 4)){
		// must take sure that no tx_done will happen on the same time.. 
		// maybe should be moved to tasklet to handle.. 
		MV_NFP_SEC_CESA_PRIV *req_priv = (MV_NFP_SEC_CESA_PRIV *)result.pReqPrv;
		MV_PKT_INFO *pkt_info = req_priv->pPktInfo;
		printk("Error: Q request is full - TBD test...\n");
		pkt_info->pFrags->bufVirtPtr -= (MV_NFP_SEC_ESP_OFFSET + ETH_MV_HEADER_SIZE);
		pkt_info->pFrags->bufPhysAddr -= (MV_NFP_SEC_ESP_OFFSET + ETH_MV_HEADER_SIZE);
		nfp_sec_pkt_info_free(pkt_info);
		return IRQ_HANDLED;
	}
	req_array[req_empty] = (MV_NFP_SEC_CESA_PRIV *)result.pReqPrv;
	req_empty++;
	if(req_empty == MV_NFP_SEC_REQ_Q_SIZE)
		req_empty = 0;
	atomic_inc(&req_count);
#if defined(MV_CESA_CHAIN_MODE_SUPPORT)
	}
#endif
	tasklet_schedule(&nfp_sec_tasklet);

	return IRQ_HANDLED;
}


static int nfp_sec_add_rule(int dir, u32 spi, char* sa_mac, char* da_mac, 
	u32 left_peer, u32 right_peer, int if_out, u32 left_sub, u32 right_sub)
{
	MV_NFP_SEC_SPD_RULE spd;
	MV_NFP_SEC_SA_ENTRY sa;
	MV_CESA_OPEN_SESSION os;
	unsigned short 	digest_size = 0;
	int i;
	unsigned int iv_size;
	unsigned char sha1Key[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
                               0x24, 0x68, 0xac, 0xe0, 0x24, 0x68, 0xac, 0xe0,
                               0x13, 0x57, 0x9b, 0xdf};
	unsigned char cryptoKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                                 0x02, 0x46, 0x8a, 0xce, 0x13, 0x57, 0x9b, 0xdf};

	memset(&spd, 0, sizeof(MV_NFP_SEC_SPD_RULE));
	memset(&sa, 0, sizeof(MV_NFP_SEC_SA_ENTRY));
	memset(&os, 0, sizeof(MV_CESA_OPEN_SESSION));

   	os.cryptoAlgorithm = MV_CESA_CRYPTO_AES;
	switch(os.cryptoAlgorithm)
	{
		case MV_CESA_CRYPTO_DES:
			iv_size = MV_CESA_DES_BLOCK_SIZE;
			break;
		case MV_CESA_CRYPTO_3DES:
			iv_size = MV_CESA_3DES_BLOCK_SIZE;
			break;
		
		case MV_CESA_CRYPTO_AES:
			iv_size = MV_CESA_AES_BLOCK_SIZE;
			break;
		default:
			printk("Unknown crypto \n");

	}
   	os.macMode = MV_CESA_MAC_HMAC_SHA1;
   	switch(os.macMode)
   	{
		case MV_CESA_MAC_MD5:
		case MV_CESA_MAC_HMAC_MD5:
			digest_size = MV_CESA_MD5_DIGEST_SIZE;
			break;
	
		case MV_CESA_MAC_SHA1:
		case MV_CESA_MAC_HMAC_SHA1:
			digest_size = 12;//MV_CESA_SHA1_DIGEST_SIZE;
			break;
	
		case MV_CESA_MAC_NULL:
			digest_size = 0;
			break;
		}

	os.cryptoMode = MV_CESA_CRYPTO_CBC;
	if (dir == MV_NFP_SEC_RULE_DB_IN) {
		os.direction = MV_CESA_DIR_DECODE;
		os.operation = MV_CESA_MAC_THEN_CRYPTO;
	}
	else {
		os.direction = MV_CESA_DIR_ENCODE;
		os.operation = MV_CESA_CRYPTO_THEN_MAC;
	}

   	for(i=0; i<sizeof(cryptoKey); i++)
       	os.cryptoKey[i] = cryptoKey[i];

	os.cryptoKeyLength = sizeof(cryptoKey);

	for(i=0; i<sizeof(sha1Key); i++)
       		os.macKey[i] = sha1Key[i];

	os.macKeyLength = sizeof(sha1Key);
	os.digestSize = digest_size;

	if (MV_OK != mvCesaSessionOpen(&os, (short*)&(sa.sid)))
	{
		printk("%s: open session failed\n", __FUNCTION__);
	}

	sa.digestSize = digest_size;
	sa.ivSize = iv_size;
	sa.spi = MV_32BIT_BE(spi);
	sa.tunProt = MV_NFP_SEC_TUNNEL;
	sa.encap = MV_NFP_SEC_ESP;
	sa.seqNum = 0;
	sa.tunnelHdr.sIp = left_peer;
	sa.tunnelHdr.dIp = right_peer;
	sa.tunnelHdr.outIfIndex = if_out;
	sa.lifeTime = 0; 

	if (dir == MV_NFP_SEC_RULE_DB_IN) 
		sa.secOp = MV_NFP_SEC_DECRYPT;
	else
		sa.secOp = MV_NFP_SEC_ENCRYPT;

	for(i=0; i<6; i++)
	{
		sa.tunnelHdr.dstMac[i] = da_mac[i];
		sa.tunnelHdr.srcMac[i] = sa_mac[i];
	}

	spd.pSAEntry = mvNfpSecSAEntrySet(&sa, dir);	
	printk("pSAEntry=%p\n",spd.pSAEntry);
	if (!spd.pSAEntry)
		printk("%s: SA creation failed \n", __FUNCTION__);

	spd.sIp = left_sub;
	spd.dIp = right_sub;

#ifdef	MV_NFP_SEC_5TUPLE_KEY_SUPPORT 
	spd.dstPort = 0x2;       	// dport: 512
	spd.srcPort = 0x3;      	// sport: 768
	spd.proto = MV_IP_PROTO_TCP;
#endif
	spd.actionType = MV_NFP_SEC_SECURE;

	if (!mvNfpSecSPDRuleSet(&spd, dir))
		printk("%s: SPD creation failed \n", __FUNCTION__);
        
	return 0;
}

static void nfp_sec_init_rule(void)
{
	unsigned char 	sa_wan[] = {0, 0, 0, 0, 0x61, 0x92};
	unsigned char 	sa_lan[] = {0, 0, 0, 0, 0x61, 0x93};
	unsigned char 	da_peer_wan[] = {0x0,0x13,0x20,0x8b,0xbc,0x49};
	unsigned char 	da_host_lan[] = {0x0,0x15,0x58,0x2F,0xF0,0x39};
	unsigned char 	da_peer_lan[] = {0x0,0x62,0x81,0x00,0x39,0x01};
	unsigned char 	null_mac[] = {0, 0, 0, 0, 0, 0};

#if 1
	/* eth0 */
	nfp_sec_add_rule(MV_NFP_SEC_RULE_DB_OUT, 0x65060000, sa_wan, da_peer_wan,
					0x0101a8c0, /* left/sip 192.168.1.1 */
					0x25d2050A, /* right/dip 10.5.210.37 */
					2,          /* egress ifindex 1=eth0, 4=ppp0 */
					0x0a010101,	/* left sub */
					0x0a020202);/* right sub */
#else
	/* ppp0 */
	nfp_sec_add_rule(MV_NFP_SEC_RULE_DB_OUT, 0x65060000, sa_wan, da_peer_wan,
					0x05000028, /* left/sip 40.0.0.5  use ppp0 */
					0x25d2050A, /* right/dip 10.5.210.37 */
					4,          /* egress ifindex 1=eth0, 4=ppp0 */
					0x0a010101,	/* left sub */
					0x0a020202);/* right sub */
#endif

#if 1
	/* eth0 */
	nfp_sec_add_rule(MV_NFP_SEC_RULE_DB_IN, 0x65060000, sa_lan, da_host_lan,  
					0x25d2050A, /* left/sip 10.5.210.37 */
					0x0101a8c0, /* right/dip 192.168.1.1 */
					2,          /* egress ifindex 2=eth1 */
					0x0a020202,	/* left sub */
					0x0a010101);/* right sub */
#else
	/* ppp0 */
	nfp_sec_add_rule(MV_NFP_SEC_RULE_DB_IN, 0x65060000, sa_lan, da_host_lan,  
					0x25d2050A, /* left/sip 10.5.210.37 */
					0x05000028, /* right/dip 40.0.0.5 */
					2,          /* egress ifindex 2=eth1 */
					0x0a020202,	/* left sub */
					0x0a010101);/* right sub */
#endif

#ifdef CONFIG_MV_ETH_NFP_SEC_HUB
	/* dummy, will be rerouted to 2nd tunnel on LAN */
	nfp_sec_add_rule(MV_NFP_SEC_RULE_DB_IN, 0x77070000, null_mac, null_mac,
					0x6402a8c0, /* sip/remote/peer */
					0x0102a8c0, /* dip/local/me */
					4,          /* egress ifindex */
					0x0a010101,	/* left sub */
					0x0a020202);/* right sub */

	/* 2nd tunnel on LAN */
	nfp_sec_add_rule(MV_NFP_SEC_RULE_DB_OUT, 0x77070000, sa_lan, da_peer_lan,  
					0x0102a8c0, /* left/sip/me 192.168.2.1 */
					0x6402a8c0, /* right/dip/peer 192.168.2.100 */
					2,          /* egress ifindex */
					0x0a020202,	/* left sub */
					0x0a010101);/* right sub */
#endif
}

static int nfp_sec_init(void)
{
	spin_lock_init( &nfp_sec_lock );
	if( MV_OK != mvCesaInit(MV_NFP_SEC_MAX_SES, MV_NFP_SEC_Q_SIZE, (char *)mv_crypto_base_get(),
				NULL) ) {
            	printk("%s,%d: mvCesaInit Failed. \n", __FILE__, __LINE__);
		return EINVAL;
	}
	/* clear and unmask Int */
	MV_REG_WRITE( MV_CESA_ISR_CAUSE_REG, 0);
	MV_REG_WRITE( MV_CESA_ISR_MASK_REG, MV_CESA_CAUSE_ACC_DMA_MASK);

	tasklet_init(&nfp_sec_tasklet, req_handler, (unsigned long) 0);
	/* register interrupt */
	if( request_irq( CESA_IRQ, nfp_sec_interrupt_handler,
                             (IRQF_DISABLED) , "cesa", NULL) < 0) {
            	printk("%s,%d: cannot assign irq %x\n", __FILE__, __LINE__, CESA_IRQ);
		return EINVAL;
        }

	mvNfpSecInit(10);

	/* add rules manually */
	nfp_sec_init_rule();
    
	/* use for debug */
	/* mvNfpSecDbPrint(); */ 

	/* init waitqueue */
	init_waitqueue_head(&nfp_sec_waitq);
	
	atomic_set(&req_count, 0);

	return 0;
}


module_init(nfp_sec_init);

MODULE_LICENSE("Marvell/GPL");
MODULE_AUTHOR("Ronen Shitrit\Eran Ben Avi");
MODULE_DESCRIPTION("NFP SEC for CESA crypto");
