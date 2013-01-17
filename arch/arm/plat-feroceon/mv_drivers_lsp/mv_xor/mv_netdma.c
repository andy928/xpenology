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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/dmaengine.h>

#include "xor/mvXor.h"
#include "ctrlEnv/sys/mvSysXor.h"

#define DUBUG
#undef DEBUG
#ifdef DEBUG
#define DPRINTK(s, args...)  printk("xor: " s, ## args)
#else
#define DPRINTK(s, args...)
#endif

/* 
 * Perform integrity test.
 * Upon completion of xor dma operation, 
 * each descriptor will be checked to success bit.
 */
#define XOR_INTEGRITY
#undef XOR_INTEGRITY

/* 
 * Double invalidate WA
 */
#ifdef CONFIG_MV_SP_I_FTCH_DB_INV
#define XOR_UNMAP
#endif

/* 
 * Perform benchmark
 */
#define XOR_BENCH
#undef XOR_BENCH


#define XOR_STATS
#undef XOR_STATS
#ifdef XOR_STATS
#define STAT_INC(s) 	xor_stats->s++
#define STAT_ADD(s,n) 	xor_stats->s[((n)>>STAT_SHIFT)&(STAT_BYTES-1)]++
#define STAT_SHIFT	7
#define STAT_BYTES	(1 << 9)
#else
#define STAT_INC(s)
#define STAT_ADD(s,n)
#endif
#if defined (CONFIG_MV_XOR_MEMCOPY) || defined (CONFIG_MV_IDMA_MEMCOPY)
#define memcpy asm_memcpy
#endif

#define XOR_BUG(c) if (c) {xor_dump(); BUG();}

#define CHANNELS    		CONFIG_MV_XOR_CHANNELS
#define ENGINES    			(CHANNELS >> 1)
#define XOR_TIMEOUT 		0x8000000
#define XOR_MIN_COPY_CHUNK 	16		/* A0 specification */
#define NET_DESC 			128		/* NET DMA descriptors */
#define DMA_DESC 			2		/* memcpy descriptors */

#define to_dma_channel(dch) container_of(dch, struct xor_dma_channel, common)

enum {
	CHAIN_NETDMA = CHANNELS,
#ifdef CONFIG_MV_XOR_MEMCOPY
	CHAIN_MEMCPY,
#endif
#ifdef CONFIG_MV_XOR_MEMXOR
	CHAIN_MEMXOR,
#endif
#ifdef CONFIG_MV_XOR_COPY_TO_USER
	CHAIN_TO_USR,
#endif
#ifdef CONFIG_MV_XOR_COPY_FROM_USER
	CHAIN_FROM_USR,
#endif
	CHAINS,
};

typedef MV_XOR_DESC xor_desc_t;

struct xor_chain {
	unsigned int 		idx;
	unsigned int		pending;
	xor_desc_t*    		desc;			/* N descriptors 	*/
	dma_addr_t     	 	base;			/* phy address  	*/
	struct xor_channel*	owner;			/* owned by channel	*/
	unsigned int		busy;			/* busy by cpu		*/
	unsigned int		cookie_used;	/* last used 		*/
	unsigned int		cookie_done;	/* last completed 	*/
};

struct xor_channel {
	unsigned int 		idx;
	unsigned int		busy;
	struct xor_chain* 	chain;			/* busy on chain	*/
	unsigned int		pad;
};

#ifdef CONFIG_MV_XOR_NET_DMA
struct xor_dma_device {
	struct dma_device common;
};

struct xor_dma_channel {
	struct dma_chan		common;
	struct xor_chain* 	chain;
	spinlock_t 			lock;
};
#endif

struct xor_net_stats
{
#ifdef XOR_STATS
	unsigned int err_chann_busy;
	unsigned int err_chain_busy;
	unsigned int err_addr;
	unsigned int err_dma;

#ifdef CONFIG_MV_XOR_NET_DMA
	unsigned int netdma_b2p;
	unsigned int netdma_p2p;
	unsigned int netdma_b2b;
	unsigned int netdma_cpu;
	unsigned int netdma_dma;
	unsigned int netdma_complete;
	unsigned int netdma_pending;
	unsigned int netdma_bytes[STAT_BYTES];
#endif
#ifdef CONFIG_MV_XOR_MEMCOPY
	unsigned int memcp;
	unsigned int memcp_dma;
	unsigned int memcp_bytes[STAT_BYTES];
#endif
#ifdef CONFIG_MV_XOR_MEMXOR
	unsigned int memxor;
	unsigned int memxor_dma;
	unsigned int memxor_bytes[STAT_BYTES];
#endif
#ifdef CONFIG_MV_XOR_COPY_TO_USER
	unsigned int to_usr;
	unsigned int to_usr_dma;
	unsigned int to_usr_bytes[STAT_BYTES];		
#endif
#ifdef CONFIG_MV_XOR_COPY_FROM_USER
	unsigned int from_usr;
	unsigned int from_usr_dma;
	unsigned int from_usr_bytes[STAT_BYTES];		
#endif
#endif
};

#ifdef CONFIG_MV_XOR_NET_DMA
static struct xor_dma_device xor_dma_dev;
static struct xor_dma_channel xor_dma_chn[1];
#endif

struct xor_channel* xor_channels;
struct xor_chain* xor_chains;
struct xor_net_stats* xor_stats;

static unsigned int xor_enabled __read_mostly;



/*
 * channel busy test
 */
static inline unsigned int xor_is_active(unsigned int i)
{
	return MV_REG_READ(XOR_ACTIVATION_REG(XOR_UNIT(i),XOR_CHAN(i))) 
			/* & XEXACTR_XESTATUS_MASK */;
}

/*
 * set dma operation mode for channel i
 */
static inline void xor_mode_dma(unsigned int i)
{
	unsigned int mode = MV_REG_READ(XOR_CONFIG_REG(XOR_UNIT(i),XOR_CHAN(i)));
	mode &= ~XEXCR_OPERATION_MODE_MASK;
	mode |= XEXCR_OPERATION_MODE_DMA;
	MV_REG_WRITE(XOR_CONFIG_REG(XOR_UNIT(i), XOR_CHAN(i)), mode);
}

/*
 * set xor operation mode for channel i
 */
static inline void xor_mode_xor(unsigned int i)
{
	unsigned int mode = MV_REG_READ(XOR_CONFIG_REG(XOR_UNIT(i),XOR_CHAN(i)));
	mode &= ~XEXCR_OPERATION_MODE_MASK;
	mode |= XEXCR_OPERATION_MODE_XOR;
	MV_REG_WRITE(XOR_CONFIG_REG(XOR_UNIT(i), XOR_CHAN(i)), mode);
}

/*
 * run dma operation on channel
 */
static inline void xor_dma(unsigned int i, unsigned int base)
{
#ifdef MV_BRIDGE_SYNC_REORDER
	mvOsBridgeReorderWA();
#endif
	MV_REG_WRITE(XOR_NEXT_DESC_PTR_REG(XOR_UNIT(i), XOR_CHAN(i)), base);                    
	MV_REG_WRITE(XOR_ACTIVATION_REG(XOR_UNIT(i), XOR_CHAN(i)), XEXACTR_XESTART_MASK);
}

/*
 * xor engine busy wait
 */
static void xor_dump(void);
#define XOR_CAUSE_DONE_MASK(chan) ((BIT0|BIT1) << (chan * 16))
void xor_wait(unsigned int chan)
{
    unsigned int timeout;
	unsigned int addr = XOR_CAUSE_REG(XOR_UNIT(chan));
	unsigned int mask = XOR_CAUSE_DONE_MASK(XOR_CHAN(chan));

	timeout = XOR_TIMEOUT;
    while(!(MV_REG_READ(addr) & mask))
		XOR_BUG(!timeout--);			

	MV_REG_WRITE(addr, ~mask);

    timeout = XOR_TIMEOUT;
	while(xor_is_active(chan))
		XOR_BUG(!timeout--);
}

/*
 * Second L2 invalidate (WA)
 */
#ifdef XOR_UNMAP
static void xor_chain_unmap(struct xor_chain* chain)
{
	u32 va;
	u32 i = chain->pending;
	xor_desc_t* desc = chain->desc;
	unsigned long flags;

	raw_local_irq_save(flags);

	while(i--) {
		va = __phys_to_virt(desc->phyDestAdd);
		/* invalidate l2 */
		__asm__ __volatile__ ("mcr p15, 1, %0, c15, c11, 4" : : "r" (va));
		__asm__ __volatile__ ("mcr p15, 1, %0, c15, c11, 5" : : "r" (va+desc->byteCnt-1));
		desc++;
	}

	raw_local_irq_restore(flags);
}
#endif
/*
 * Chain integrity test
 */
#ifdef XOR_INTEGRITY
static void xor_integrity(struct xor_chain* chain)
{
	unsigned int i;

	for (i=0; i<chain->pending; i++) {
		mvOsCacheLineInv(0, chain->desc + i);
		if (chain->desc[i].status != 0x40000000) {
			printk("XOR: violation chain[%d] desc=%d status=%x active=%d %x<-%x %d bytes\n", 
				   chain->idx, i, 
				   chain->desc[i].status,
				   xor_is_active(chain->idx),
				   chain->desc[i].phyDestAdd, chain->desc[i].srcAdd0, chain->desc[i].byteCnt);
		}
	}
}
#endif

static void xor_dump(void)
{
	unsigned int i;

	for (i=0; i<CHANNELS; i++) {
		printk(" CHANNEL_ARBITER_REG %08x\n", MV_REG_READ(XOR_CHANNEL_ARBITER_REG(XOR_UNIT(i))));
		printk(" CONFIG_REG          %08x\n", MV_REG_READ(XOR_CONFIG_REG(XOR_UNIT(i),XOR_CHAN(i))));
		printk(" ACTIVATION_REG      %08x\n", MV_REG_READ(XOR_ACTIVATION_REG(XOR_UNIT(i), XOR_CHAN(i))));
		printk(" CAUSE_REG           %08x\n", MV_REG_READ(XOR_CAUSE_REG(XOR_UNIT(i))));
		printk(" MASK_REG            %08x\n", MV_REG_READ(XOR_MASK_REG(XOR_UNIT(i))));
		printk(" ERROR_CAUSE_REG     %08x\n", MV_REG_READ(XOR_ERROR_CAUSE_REG(XOR_UNIT(i))));
		printk(" ERROR_ADDR_REG      %08x\n", MV_REG_READ(XOR_ERROR_ADDR_REG(XOR_UNIT(i))));
		printk(" NEXT_DESC_PTR_REG   %08x\n", MV_REG_READ(XOR_NEXT_DESC_PTR_REG(XOR_UNIT(i),XOR_CHAN(i))));
		printk(" CURR_DESC_PTR_REG   %08x\n", MV_REG_READ(XOR_CURR_DESC_PTR_REG(XOR_UNIT(i),XOR_CHAN(i))));
		printk(" BYTE_COUNT_REG      %08x\n\n", MV_REG_READ(XOR_BYTE_COUNT_REG(XOR_UNIT(i),XOR_CHAN(i))));
	}

	for (i=0; i<CHANNELS; i++) {
		printk("channel[%d] active=0x%x busy=%d chain=%x\n", 
						xor_channels[i].idx,
						xor_is_active(i), 
						xor_channels[i].busy, 
						xor_channels[i].chain ? xor_channels[i].chain->idx : ~0);
	}

	for (i=0; i<CHAINS; i++) {		
		printk("chain[%d] on channel=%8x desc=%x cookies(%x,%x) pending=%d\n", i,
						xor_chains[i].owner ? xor_chains[i].owner->idx : ~0,
						xor_chains[i].base, 
						xor_chains[i].cookie_done,
						xor_chains[i].cookie_used,
						xor_chains[i].pending);
	}
}

static void xor_stat(void)
{	
#ifdef XOR_STATS
		int i;

		printk("XOR errors...........%10u\n", xor_stats->err_dma);
		printk("XOR busy channel.....%10u\n", xor_stats->err_chann_busy); 
		printk("XOR busy chain.......%10u\n", xor_stats->err_chain_busy);
		printk("XOR invalid address..%10u\n", xor_stats->err_addr);
#ifdef CONFIG_MV_XOR_NET_DMA
		printk("\n");
		printk("NETDMA b2p...........%10u\n", xor_stats->netdma_b2p);
		printk("NETDMA p2p...........%10u\n", xor_stats->netdma_p2p);
		printk("NETDMA b2b...........%10u\n", xor_stats->netdma_b2b);
		printk("NETDMA by cpu........%10u\n", xor_stats->netdma_cpu);
		printk("NETDMA by hw.........%10u\n", xor_stats->netdma_dma);
		printk("NETDMA pending.......%10u\n", xor_stats->netdma_pending);
		printk("NETDMA complete......%10u\n", xor_stats->netdma_complete);
#endif
#ifdef CONFIG_MV_XOR_MEMCOPY
		printk("\n");
		printk("MEMCPY total.........%10u\n", xor_stats->memcp);
		printk("MEMCPY by hw.........%10u\n", xor_stats->memcp_dma);
#endif
#ifdef CONFIG_MV_XOR_MEMXOR
		printk("\n");
		printk("MEMXOR total.........%10u\n", xor_stats->memxor);
		printk("MEMXOR by hw.........%10u\n", xor_stats->memxor_dma);
#endif
#ifdef CONFIG_MV_XOR_COPY_TO_USER
		printk("\n");
		printk("TO_USER total.........%10u\n", xor_stats->to_usr);
		printk("TO_USER by hw.........%10u\n", xor_stats->to_usr_dma);
#endif
#ifdef CONFIG_MV_XOR_COPY_FROM_USER
		printk("\n");
		printk("FROM_USER total.........%10u\n", xor_stats->from_usr);
		printk("FROM_USER by hw.........%10u\n", xor_stats->from_usr_dma);
#endif

		printk("BYTES  NETDMA   MEMCPY   MEMXOR   TO_USER  FROM_USER\n");
		for (i=0; i<STAT_BYTES; i++) {
			u32 a=0, b=0, c=0, d=0, e=0;
#ifdef CONFIG_MV_XOR_NET_DMA
			a = xor_stats->netdma_bytes[i];
#endif
#ifdef CONFIG_MV_XOR_MEMCOPY
			b = xor_stats->memcp_bytes[i];
#endif
#ifdef CONFIG_MV_XOR_MEMXOR
			c = xor_stats->memxor_bytes[i];
#endif
#ifdef CONFIG_MV_XOR_COPY_TO_USER
			d = xor_stats->to_usr_bytes[i];
#endif
#ifdef CONFIG_MV_XOR_COPY_FROM_USER
			e = xor_stats->from_usr_bytes[i];
#endif
			if (a || b || c || d || e)
				printk("%5d%8u %8u %8u %8u %8u\n", i<<STAT_SHIFT, a, b, c, d, e);
		}

		memset(xor_stats, 0, sizeof(struct xor_net_stats));
#endif       
}

/*
 * Allocate chain
 */
static inline unsigned int xor_try_chain(struct xor_chain* chain)
{
	unsigned long flags=0;
	local_irq_save(flags);	
	if (chain->busy) {
		local_irq_restore(flags);
		STAT_INC(err_chain_busy);
		return 1;
	}
	chain->busy = 1;
	local_irq_restore(flags);
	return 0;
}

/*
 * Connect channel to chain
 */
static inline void xor_attach(struct xor_chain* chain, struct xor_channel* xch)
{
	xch->chain = chain;
	chain->owner = xch;
}

/*
 * Disconnect channel and chain
 */
static inline void xor_detach(struct xor_chain* chain, struct xor_channel* xch)
{
#ifdef XOR_INTEGRITY
	xor_integrity(chain);
#endif	
#ifdef XOR_UNMAP
	if (chain->pending) 
		xor_chain_unmap(chain);
#endif
	chain->cookie_done = chain->cookie_used;
	chain->pending = 0;
	chain->owner = NULL;
	chain->busy = 0;
	xch->chain = NULL;
}

/*
 * Allocate channel
 */
static struct xor_channel* xor_get(void)
{
    static int rr=0;
	int retry=CHANNELS;
	struct xor_channel* xch = NULL;	
	unsigned long flags=0;

	local_irq_save(flags);

	rr += ENGINES;

	while(retry--) { 
		rr &= (CHANNELS-1);

		if (xor_channels[rr].busy || xor_is_active(rr)) {
			rr++;
			continue;
		}
	
		xch = &xor_channels[rr];
		xch->busy = 1;

		if (xch->chain)
			xor_detach(xch->chain, xch);

		break;
	}

	local_irq_restore(flags);

	if (!xch) 
		STAT_INC(err_chann_busy);

	return xch;
}

static inline void xor_put(struct xor_channel* xch)
{
	xch->busy = 0;
}

/*
 * Fast clean, dsb() required
 * The call is intended for multiple calls and when dsb() on commit endpoint
 */
static inline void dmac_clean_dcache_line(void* addr)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
	__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 1" : : "r" (addr));
	__asm__ __volatile__ ("mcr p15, 1, %0, c15, c9, 1" : : "r" (addr));

#else
	__dma_single_cpu_to_dev(addr, 32, DMA_TO_DEVICE);
#endif
}

#ifdef CONFIG_MV_XOR_NET_DMA
static int xor_alloc_chan_resources(struct dma_chan *dch)
{
	return 1;
}

static void xor_free_chan_resources(struct dma_chan *dch)
{
	printk("xor: free netdma chain[%d] resources\n", dch->chan_id);
}

/*
 *  Start xmit of chain 
 */
static struct xor_chain* xor_xmit(struct xor_chain* chain)
{
	struct xor_channel* xch;
	int i;

	xch = xor_get();
	if (!xch) 
		goto out;

	xor_attach(chain, xch);

	i = chain->pending;
	while(i--)
		dmac_clean_dcache_line(chain->desc + i);
	dsb();

	STAT_INC(netdma_dma);
	xor_dma(xch->idx, chain->base);
	xor_put(xch);

	/*
	 * find free chain,
	 * always succedded since chains >> channels
	 */
	i = CHAIN_NETDMA;
	while(i--) {	
		if (likely(chain->idx < CHAIN_NETDMA))
			chain++;
		else
			chain = xor_chains;

		if (chain->owner)
			continue;
			
		if (chain->busy)
			continue;

        chain->busy = 1;
		break;
	}

out:
	return chain;	
}

static dma_cookie_t xor_memcpy_buf_to_pg(struct dma_chan *chan,
										 struct page *page, unsigned int offset, void *_from, size_t n)
{
	u32 to, from=(u32)_from;
	xor_desc_t* desc;
	struct xor_dma_channel* dch = to_dma_channel(chan);
	struct xor_chain* chain;
	int irq = in_interrupt();
	u32 c;

	STAT_INC(netdma_b2p);
	STAT_ADD(netdma_bytes, n);

	to = (u32)page_address(page) + offset;

	DPRINTK("%s: %p<-%p %d bytes\n", __FUNCTION__, to, from, n);

	if (!virt_addr_valid(to) || !virt_addr_valid(from)) {
		STAT_INC(err_addr);
		goto out;
	}

	/* at least two cache lines or XOR minimum */
	if (n < 96)
		goto out;

	if (to & 31) {
		c = 32 - (to & 31);
		memcpy((void*)to, (void*)from, c);
		to += c;
		from += c;
		n -= c;
	}

	c = (to+n) & 31;
	if (c) {
		n -= c;
		memcpy((void*)to+n, (void*)from+n, c);
	}

	if (n < 32) 
		goto out;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
	dmac_clean_range((void*)from, (void*)from + n);
	dmac_inv_range((void*)to, (void*)to + n);
#else
	dma_cache_maint((void*)from, n, DMA_TO_DEVICE);
	dma_cache_maint((void*)to, n, DMA_FROM_DEVICE);
#endif

	if (!irq)
		spin_lock_bh(&dch->lock);

	chain = dch->chain; 

	XOR_BUG(chain->owner || chain->pending>=NET_DESC);

	desc = chain->desc + chain->pending++;
	desc->srcAdd0 = __pa((void*)from);
	desc->phyDestAdd = __pa((void*)to);	
	desc->byteCnt = n;
	desc->status = BIT31;

	if (chain->pending == 1)
		chain->cookie_done = chan->cookie;

	if (++chan->cookie < 0)
		chan->cookie = 1;

	chain->cookie_used = chan->cookie;

    if (chain->pending > 1)
		dch->chain = xor_xmit(chain);

	if (!irq)
		spin_unlock_bh(&dch->lock);

	return chan->cookie;

out:
	STAT_INC(netdma_cpu);
	memcpy((void*)to, (void*)from, n);
	return chan->cookie;
}

static dma_cookie_t xor_memcpy_pg_to_pg(struct dma_chan *chan,
									struct page *dest_pg, unsigned int dest_off,
									struct page *src_pg, unsigned int src_off, size_t n)
{
	STAT_INC(netdma_p2p);
	return xor_memcpy_buf_to_pg(chan,
								dest_pg, 
								dest_off, 
								(void*)((u32)page_address(src_pg) + src_off), n);
}

static dma_cookie_t xor_memcpy_buf_to_buf(struct dma_chan *chan,void *dest, void *src, size_t n)
{
	/* kernel copies (e.g. smbfs) 
	 * not really tested
	 */
	STAT_INC(netdma_b2b);
	return xor_memcpy_buf_to_pg(chan,
								page_address(dest),
								page_offset(dest),
								src, n);
}

/*
 * cookies compare
 */
static inline int before(__s32 c1, __s32 c2)
{
	/*
	 * Cookies are continuous. 
	 * The distance between cookies should not exceed FFFF,
	 * otherwise is considered as overflow.
	 */
	if (c1 <= c2) 
		return (c2 - c1) <= 0xFFFF;
    else 
		return (c1 - c2) > 0xFFFF;
}

/*
 * Find completed cookie across all chains. 
 * Each one has [done,used] section. If for sections done=used, report max,
 * otherwise, the minimum 'done' is returned.
 */
static /*inline*/ dma_cookie_t xor_get_completed(void)
{
	dma_cookie_t min_cookie = 0;
	dma_cookie_t max_cookie = 0;
	dma_cookie_t c_used, c_done;

	struct xor_chain* chain = xor_chains;
	int i = CHAIN_NETDMA;

	do {
			c_used = chain->cookie_used;
			c_done = chain->cookie_done;

			if (c_used != c_done) {
				if (chain->owner && !xor_is_active(chain->owner->idx)) {
					c_used = c_done = chain->cookie_used;
				}
			}

			if (c_used != c_done) {
			/*
			 * at least one chain is incomplete. 
			 * find minimal cookie which is done across all chains
			 */		
				if (!min_cookie || before(c_done, min_cookie))
					min_cookie = c_done;
			}

			/*
			 * in the case whether all chains are completed, 
			 * find their max cookie
			 */		
			else if (!c_done)  {
				;
			}
			else if (!max_cookie || before(max_cookie, c_done))
					max_cookie = c_done;

			chain++;

	} while(i--);

	return min_cookie ? min_cookie: max_cookie;
}
#ifdef XOR_UNMAP
static void xor_memcpy_cleanup(struct dma_chan *chan, dma_cookie_t cookie)
{
	struct xor_chain* chain = xor_chains;
	struct xor_dma_channel* dch = to_dma_channel(chan);
	int i = CHAIN_NETDMA;

	spin_lock_bh(&dch->lock);

	do {
		if (chain->owner && 
			chain->pending && !xor_is_active(chain->owner->idx)) {
				xor_detach(chain, chain->owner);
		}

		chain++;

	} while(i--);

	spin_unlock_bh(&dch->lock);
}
#endif

static enum dma_status xor_memcpy_complete(struct dma_chan *chan,
										dma_cookie_t cookie, 
										dma_cookie_t *pdone,
										dma_cookie_t *pused)
{
	dma_cookie_t done;
	dma_cookie_t used;
	enum dma_status ret;

	STAT_INC(netdma_complete);

	done = xor_get_completed();
	used = chan->cookie;
	
	if (pdone)
		*pdone = done;
	if (pused)
		*pused = used;

	ret = dma_async_is_complete(cookie, done, used);
#ifdef XOR_UNMAP
	if (ret == DMA_SUCCESS)
		xor_memcpy_cleanup(chan, cookie);
#endif

	return ret;
}

static void xor_memcpy_issue_pending(struct dma_chan *chan)
{
	struct xor_dma_channel* dch = to_dma_channel(chan);
	struct xor_chain* chain;
	unsigned int irq = in_interrupt();

	STAT_INC(netdma_pending);

	chain = dch->chain;
	if (chain->owner)
		return;

	if (!irq)
		spin_lock_bh(&dch->lock);

	chain = dch->chain;

	if (!chain->owner && chain->pending) {
		do {
			dch->chain = xor_xmit(chain);
		} while (dch->chain == chain);
	}
 
	if (!irq)
		spin_unlock_bh(&dch->lock);
}
#endif /* CONFIG_MV_XOR_NET_DMA */

static int xor_proc_write(struct file *file, const char __user *buffer,
			   unsigned long count, void *data)
{
#ifdef XOR_BENCH
	xor_bench(NULL,NULL,0);
#endif
	return count;
}

static int xor_proc_read(char *page, char **start, off_t off, 
						 int count, int *eof, void *data)
{
	xor_dump();
	xor_stat();
	return 0;
}

int __init mv_xor_init(void)
{
		unsigned int i;
		struct proc_dir_entry *proc_e;
		struct xor_chain* chain;
		u32 va;	

		if ((MV_FALSE == mvCtrlPwrClckGet(XOR_UNIT_ID, 0)) || 
		    (MV_FALSE == mvCtrlPwrClckGet(XOR_UNIT_ID, 1)))
		{
			printk("\nWarning at least one XOR port is Powered Off\n");
			return 0;
			
		}
	

#if defined(CONFIG_MV78200) || defined(CONFIG_MV632X)
		if (MV_FALSE == mvSocUnitIsMappedToThisCpu(XOR)) {
			printk(KERN_INFO"XOR engine is not mapped to this CPU\n");
			return -ENODEV;
		}	
#endif

		mvXorInit();

		/* stats */
		va = (u32)kmalloc(32 + (sizeof(struct xor_net_stats)), GFP_KERNEL);
		va += 32 - (va & 31);
		xor_stats = (struct xor_net_stats*)va;
		memset(xor_stats, 0, sizeof(struct xor_net_stats));

		/* channels */
		va = (u32)kmalloc(32 + (sizeof(struct xor_channel) * CHANNELS), GFP_KERNEL);
		va += 32 - (va & 31);
		xor_channels = (struct xor_channel*)va;
		memset(xor_channels, 0, sizeof(struct xor_channel) * CHANNELS);

		/* chains */
		va = (u32)kmalloc(32 + (sizeof(struct xor_chain) * CHAINS), GFP_KERNEL);
		va += 32 - (va & 31);
		xor_chains = (struct xor_chain*)va;
		memset(xor_chains, 0, sizeof(struct xor_chain) * CHAINS);

		/*
		 * init channels
		 */
		for(i=0; i<CHANNELS; i++) {
			xor_channels[i].idx = i;
			xor_channels[i].busy = 0;
			xor_channels[i].chain = NULL;

			xor_mode_dma(i);
		}

		/*
		 * init chains
         */                                 
        i = CHAINS;
		while(i--) {
			u32 sz, va=0;	

			chain = &xor_chains[i];
			
			chain->idx = i;			

			if (i <= CHAIN_NETDMA)
				sz = sizeof(xor_desc_t) * NET_DESC;
			else
				sz = sizeof(xor_desc_t) * DMA_DESC;

			va = (u32)kmalloc(sz+64, GFP_KERNEL);
			va += 64 - (va & 63);
			chain->desc = (void*)va;
			chain->base = __pa(va);

			memset(chain->desc, 0, sz);	
			BUG_ON(63 & (u32)chain->desc);

			/*
			 * Init next pointer
             */
			if (i <= CHAIN_NETDMA) {
				int d = NET_DESC - 1;
				while(d--)
					chain->desc[d].phyNextDescPtr = 
						chain->base + (sizeof(xor_desc_t) * (d+1));
			}

		}
    

#ifdef CONFIG_PROC_FS
		/* FIXME: /proc/sys/dev/ */
		proc_e = create_proc_entry("mvxor", 0666, 0);
        proc_e->read_proc = xor_proc_read;
        proc_e->write_proc = xor_proc_write;
		proc_e->nlink = 1;
#endif

#ifdef CONFIG_MV_XOR_NET_DMA
		memset(&xor_dma_dev, 0, sizeof(struct xor_dma_device));
		memset(xor_dma_chn, 0, sizeof(struct xor_dma_channel));
		INIT_LIST_HEAD(&xor_dma_dev.common.channels);

		xor_dma_dev.common.chancnt = 1;
		xor_dma_chn[0].common.device = &xor_dma_dev.common;
		xor_dma_chn[0].chain = &xor_chains[0];
		xor_dma_chn[0].chain->busy = 1;

		spin_lock_init(&xor_dma_chn[0].lock);
		list_add_tail(&xor_dma_chn[0].common.device_node, 
						  &xor_dma_dev.common.channels);

		xor_dma_dev.common.device_alloc_chan_resources = xor_alloc_chan_resources;
		xor_dma_dev.common.device_free_chan_resources = xor_free_chan_resources;
		xor_dma_dev.common.device_memcpy_buf_to_buf = xor_memcpy_buf_to_buf;
		xor_dma_dev.common.device_memcpy_buf_to_pg = xor_memcpy_buf_to_pg;
		xor_dma_dev.common.device_memcpy_pg_to_pg = xor_memcpy_pg_to_pg;
		xor_dma_dev.common.device_memcpy_complete = xor_memcpy_complete;
		xor_dma_dev.common.device_memcpy_issue_pending = xor_memcpy_issue_pending;

		if (dma_async_device_register(&xor_dma_dev.common)) {
			printk(KERN_ERR" XOR engine cannot be registered as async NET_DMA\n");    
			return -ENODEV;
		}

		printk(KERN_INFO "XOR registered %d NET_DMA over %d channels\n", 
			   xor_dma_dev.common.chancnt, CHANNELS);

#else
		printk(KERN_INFO "XOR registered %d channels\n", CHANNELS);

#endif

#ifdef XOR_UNMAP
		printk(KERN_INFO "XOR 2nd invalidate WA enabled\n");
#endif
		xor_enabled = 1;
		return 0;
}

void __exit mv_xor_exit(void)
{
    return;
}

module_init(mv_xor_init);
module_exit(mv_xor_exit);
MODULE_LICENSE(GPL);


/* = ====================================================================== */
#ifdef CONFIG_MV_XOR_MEMCOPY
void* xor_memcpy(void *_to, const void* _from, __kernel_size_t n)
{
	u32 to = (u32)_to;
	u32 from = (u32)_from;
	u32 sa, da;

	struct xor_chain* chain;
	struct xor_channel *xch;

	if (!xor_enabled)			
		return asm_memcpy((void*)to, (void*)from, n);

	STAT_INC(memcp);
	STAT_ADD(memcp_bytes,n);

	DPRINTK("%s:%x<-%x %d bytes\n", __FUNCTION__, to, from, n);

	if (n < 128)
		goto out;

	if (!virt_addr_valid(to) || !virt_addr_valid(from)) {
		STAT_INC(err_addr);
		goto out;
	}

	da = __pa((void*)to);	
	sa = __pa((void*)from);

	if (((da < sa) && (sa < da+n)) || 
		((sa < da) && (da < sa+n))) 
		return memmove((void*)to, (void*)from, n);
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
	dmac_clean_range((void*)from, (void*)from + n);
	dmac_inv_range((void*)to, (void*)to + n);
#else
	dma_cache_maint((void*)from, n, DMA_TO_DEVICE);
	dma_cache_maint((void*)to, n, DMA_FROM_DEVICE);
#endif

	da = virt_to_phys((void*)to);	
	sa = virt_to_phys((void*)from);

	if (unlikely(!sa || !da)) {
		STAT_INC(err_addr);
		goto out;
	}

	chain = &xor_chains[CHAIN_MEMCPY];
	if (xor_try_chain(chain)) 
		goto out;

	DPRINTK("%s: %x<-%x %d bytes\n", __FUNCTION__, da, sa, n);
	chain->pending = 1;
	chain->desc->srcAdd0 = sa;
	chain->desc->phyDestAdd = da;
	chain->desc->byteCnt = n;
	chain->desc->phyNextDescPtr = 0;
	chain->desc->status = BIT31;

	dmac_clean_dcache_line(chain->desc);
	dsb();

	xch = xor_get();
	if (!xch) 
		goto out2;

	STAT_INC(memcp_dma);
	xor_attach(chain, xch);
	xor_dma(xch->idx, chain->base);

	{
		u32 c;

		da = to & 31;
		sa = from & 31;		
		if (sa || da) {
			c = (sa > da) ? 32 - da : 32 - sa;
			memmove((void*)to, (void*)from, c);
		}

		da = (to+n) & 31;
		sa = (from+n) & 31;
		if (sa || da) {
			c = (da > sa) ? da : sa;
			memmove((void*)(to+n-c), (void*)(from+n-c), c);
		}
	}

	xor_wait(xch->idx);
	xor_detach(chain, xch);
	xor_put(xch);

	return (void*)to;

out2:
	chain->busy = 0;
out:
	return asm_memcpy((void*)to, (void*)from, n);
}
EXPORT_SYMBOL(xor_memcpy);

#endif /* CONFIG_MV_XOR_MEMCOPY */

#ifdef CONFIG_MV_XOR_MEMXOR
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
int xor_memxor(unsigned int count, unsigned int n, void **va)
{
	struct xor_chain* chain;
	struct xor_channel *xch;
	u32* p;

	if (!xor_enabled)
		goto out;

	BUG_ON(count <= 1);

	DPRINTK("%s: %d sources %d bytes\n", __FUNCTION__, count, n);

	chain = &xor_chains[CHAIN_MEMXOR];
	if (xor_try_chain(chain)) 
		goto out;

	chain->pending = 1;
	chain->desc->phyDestAdd = __pa(va[0]);
	chain->desc->byteCnt = n;
	chain->desc->phyNextDescPtr = 0;
	chain->desc->descCommand = (1 << count) - 1;
	chain->desc->status = BIT31;

	p = &chain->desc->srcAdd0;

#ifdef MV_CPU_BE
	p = &chain->desc->srcAdd1;
	if (count & 1) 
		p[count] = __pa(va[count-1]);
#endif

	while(count--) {
		dmac_clean_range(va[count], (void*)va[count] + n);
		p[count] = __pa(va[count]);
	}

	dmac_clean_dcache_line(chain->desc);
	dmac_clean_dcache_line(chain->desc+32);
	dsb();

	xch = xor_get();
	if (!xch) 
		goto out2;

	STAT_ADD(memxor_bytes, n);
	STAT_INC(memxor_dma);

	xor_attach(chain, xch);
	xor_mode_xor(xch->idx);
	xor_dma(xch->idx, chain->base);

	/* could be useful before busywait, because we already have it clean */
	dmac_inv_range(va[0], (void*)va[0] + n);
	xor_wait(xch->idx);

	xor_mode_dma(xch->idx);
	xor_detach(chain, xch);
	wmb();
	xor_put(xch);

	return 0;

out2:
	chain->busy = 0;
out:
	return 1;
}
#else /* LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26) */
int xor_memxor(unsigned int src_count, unsigned int bytes, void *dest, void **srcs)
{
	struct xor_chain* chain;
	struct xor_channel *xch;
	u32* p;

	if (!xor_enabled)
		goto out;

	BUG_ON(src_count < 1);

	DPRINTK("%s: %d sources %d bytes\n", __FUNCTION__, src_count, bytes);

	chain = &xor_chains[CHAIN_MEMXOR];
	if (xor_try_chain(chain)) 
		goto out;

	chain->pending = 1;
	chain->desc->phyDestAdd = __pa(dest);
	chain->desc->byteCnt = bytes;
	chain->desc->phyNextDescPtr = 0;
	chain->desc->descCommand = (1 << (src_count+1)) - 1; /* add dest to the count */
	chain->desc->status = BIT31;

#ifdef MV_CPU_BE
    /* set the first (srcAdd1) desc to be the dest */
	chain->desc->srcAdd1 = chain->desc->phyDestAdd;
	p = &chain->desc->srcAdd0;
	if ((src_count+1) & 1)
		p[src_count] = __pa(srcs[src_count-1]);
#else
	/* set the first desc (srcAdd0) to be the dest */
	chain->desc->srcAdd0 = chain->desc->phyDestAdd;
	p = &chain->desc->srcAdd1;
#endif

	__dma_single_cpu_to_dev(dest, bytes, DMA_TO_DEVICE);
    
	while(src_count--) {
		__dma_single_cpu_to_dev((void*)srcs[src_count], bytes, DMA_TO_DEVICE);
		p[src_count] = __pa(srcs[src_count]);
	}

	dmac_clean_dcache_line(chain->desc);
	dmac_clean_dcache_line(chain->desc+32);
	dsb();

	xch = xor_get();
	if (!xch) 
		goto out2;

	STAT_ADD(memxor_bytes, bytes);
	STAT_INC(memxor_dma);

	xor_attach(chain, xch);
	xor_mode_xor(xch->idx);
	xor_dma(xch->idx, chain->base);

	/* could be useful before busywait, because we already have it clean */
	__dma_single_cpu_to_dev(dest, bytes, DMA_FROM_DEVICE);
	xor_wait(xch->idx);

	xor_mode_dma(xch->idx);
	xor_detach(chain, xch);
	wmb();
	xor_put(xch);

	return 0;

out2:
	chain->busy = 0;
out:
	return 1;
}

#endif
EXPORT_SYMBOL(xor_memxor);
#endif /* CONFIG_MV_XOR_MEMXOR */


#if defined(CONFIG_MV_XOR_COPY_TO_USER) || defined(CONFIG_MV_XOR_COPY_FROM_USER)
/*
 * Obtain pa address from user va
 */
static inline u32 __pa_user(u32 va, int write)
{
    struct page *page;
    struct vm_area_struct *vm;
    struct mm_struct * mm = (va >= TASK_SIZE)? &init_mm : current->mm;
    unsigned int flags;

    if (virt_addr_valid(va)) 
        return __pa(va);

    if (va >= (u32)high_memory)
	    return 0;
    
    flags  = write ? (VM_WRITE | VM_MAYWRITE) : (VM_READ | VM_MAYREAD);

    vm = find_extend_vma(mm, va);
    if (!vm || (vm->vm_flags & (VM_IO|VM_PFNMAP)) || !(flags & vm->vm_flags))
		return 0;

    flags = FOLL_PTE_EXIST | FOLL_TOUCH;
    flags |= (write) ? FOLL_WRITE : 0;
		 
    page = follow_page(vm, va, flags);
    
    if (pfn_valid(page_to_pfn(page))) 
        return ((page_to_pfn(page) << PAGE_SHIFT) | (va & (PAGE_SIZE - 1)));

	return 0;
}
#endif /* COPY_XX_USER */

#ifdef CONFIG_MV_XOR_COPY_TO_USER
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
unsigned long xor_copy_to_user(unsigned long to, unsigned long from, unsigned long n)
{
	u32 sa, da, copy;
	struct xor_chain* chain;
	struct xor_channel* xch;
	unsigned int i = 0;
	unsigned long rc;
	unsigned long flags=0;

	if (unlikely(!xor_enabled))
		goto out;

	STAT_INC(to_usr);
	STAT_ADD(to_usr_bytes, n);

	DPRINTK("%s: %p<-%p %d bytes\n", __FUNCTION__, to, from, n);

	if (!virt_addr_valid(from)) {
		STAT_INC(err_addr);
		goto out;
	}

	if (to & 31) 
		mvOsCacheLineFlushInv(0, to);

	if ((to+n) & 31) 
		mvOsCacheLineFlushInv(0, to+n);

	dmac_clean_range((void*)from, (void*)from + n);
	
	chain = &xor_chains[CHAIN_TO_USR];
	if (xor_try_chain(chain)) 
		goto out;

	local_irq_save(flags);	

	while(n) {
		copy = PAGE_SIZE - (to & ~PAGE_MASK);
		if (copy > n)
			copy = n;

		if (!copy) 
			break;

		if (copy < XOR_MIN_COPY_CHUNK) {
			rc= __arch_copy_to_user((void*)to, (void*)from, copy);
			if (rc)
				goto out2;
		}
		else {

			sa = __pa(from);
			da = __pa_user(to, 1);
	
			if (unlikely(!sa || !da)) {
				STAT_INC(err_addr);
				goto out2;
			}

            DPRINTK("%s: desc[%d] %x<-%x %d bytes\n", __FUNCTION__, i, da, sa, copy);

			dmac_inv_range((void*)to, (void*)to + copy);

			chain->desc[i].srcAdd0 = sa;
			chain->desc[i].phyDestAdd = da;
			chain->desc[i].byteCnt = copy;
			chain->desc[i].phyNextDescPtr = 0;
			chain->desc[i].status = BIT31;
			if (i) 
				chain->desc[i-1].phyNextDescPtr = 
						chain->base + (sizeof(xor_desc_t) * i);
			
			i++;
		}

		from += copy;
		to += copy;
		n -= copy;
	}

	if (i) {       

		while(i--)
			dmac_clean_dcache_line(chain->desc + i);
		dsb();

		xch = xor_get();		
		if (!xch) 
			goto out2;

		xor_attach(chain, xch);

		DPRINTK("%s: ch[%d] chain=%d\n", __FUNCTION__, xch->idx, chain->idx);

		STAT_INC(to_usr_dma);
		xor_dma(xch->idx, chain->base);
		xor_wait(xch->idx);

		xor_detach(chain, xch);
		xor_put(xch);
	}

	local_irq_restore(flags);

	return 0;

out2:
	chain->busy=0;
	local_irq_restore(flags);
out:
	return __arch_copy_to_user((void*)to, (void*)from, n);
}
EXPORT_SYMBOL(xor_copy_to_user);
#else
/* #error "Kernel version >= 2,6,26 does not support XOR copy_to_user" */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26) */
#endif /* CONFIG_MV_XOR_COPY_TO_USER */

#ifdef CONFIG_MV_XOR_COPY_FROM_USER
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
unsigned long xor_copy_from_user(unsigned long to, unsigned long from, unsigned long n)
{
	u32 sa, da, copy;
	struct xor_chain* chain;
	struct xor_channel* xch;
	unsigned int i = 0;
	unsigned long flags=0;
	unsigned long rc;

	if (unlikely(!xor_enabled))
		goto out;

	STAT_INC(from_usr);
	STAT_ADD(from_usr_bytes, n);

	DPRINTK("%s: %p<-%p %d bytes\n", __FUNCTION__, to, from, n);

	if (!virt_addr_valid(to)) {
		STAT_INC(err_addr);
		goto out;
	}

	if (to & 31) 
		mvOsCacheLineFlushInv(0, to);

	if ((to+n) & 31) 
		mvOsCacheLineFlushInv(0, to+n);

	dmac_inv_range((void*)to, (void*)to + n);
	
	chain = &xor_chains[CHAIN_FROM_USR];
	if (xor_try_chain(chain)) 
		goto out;

	local_irq_save(flags);	

	while(n) {
		copy = PAGE_SIZE - (from & ~PAGE_MASK);
		if (copy > n)
			copy = n;

		if (!copy) 
			break;

		if (copy < XOR_MIN_COPY_CHUNK) {
			rc = __arch_copy_from_user((void*)to, (void*)from, copy);
			if (rc)
				goto out2;
		}
		else {

			sa = __pa_user(from, 0);
			da = __pa(to);

			if (unlikely(!sa || !da)) {
				STAT_INC(err_addr);
				goto out2;
			}
            
			DPRINTK("%s: desc[%d] %x<-%x %d bytes\n", __FUNCTION__, i, da, sa, copy);

			dmac_clean_range((void*)from, (void*)from + copy);

			chain->desc[i].srcAdd0 = sa;
			chain->desc[i].phyDestAdd = da;
			chain->desc[i].byteCnt = copy;
			chain->desc[i].phyNextDescPtr = 0;
			chain->desc[i].status = BIT31;
			if (i) 
				chain->desc[i-1].phyNextDescPtr = 
						chain->base + (sizeof(xor_desc_t) * i);
			
			i++;
		}

		from += copy;
		to += copy;
		n -= copy;
	}

	if (i) {       

		while(i--)
			dmac_clean_dcache_line(chain->desc + i);
		dsb();

		xch = xor_get();		
		if (!xch) 
			goto out2;

		xor_attach(chain, xch);

		DPRINTK("%s: ch[%d] chain=%d\n", __FUNCTION__, xch->idx, chain->idx);

		STAT_INC(from_usr_dma);
		xor_dma(xch->idx, chain->base);
		xor_wait(xch->idx);

		xor_detach(chain, xch);
		xor_put(xch);
	}

	local_irq_restore(flags);

	return 0;

out2:
	chain->busy=0;
	local_irq_restore(flags);
out:
	return __arch_copy_from_user((void*)to, (void*)from, n);
}
EXPORT_SYMBOL(xor_copy_from_user);
#else
/* #error "Kernel version >= 2,6,26 does not support XOR copy_from_user" */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26) */
#endif /* CONFIG_MV_XOR_COPY_FROM_USER */


#ifdef XOR_BENCH
void xor_bench(void *to, void *from, unsigned long n)
{
		unsigned long time;
		u32 i;
		u32 t;
		 //	
		 // functions under benchmark, use / to skip.
         //
		char* fn[] = {"/", "asm_memzero", "/", 
						"/xor_memcpy", "/memcpy", 
						"/xor_memxor", "/xor_block", NULL};
		void* va[3];

		for (n=128; n<=0x100000; n*=2) {

			va[0] =  kmalloc(n, GFP_KERNEL);;
			va[1] = kmalloc(n, GFP_KERNEL);;
			va[2] = kmalloc(n, GFP_KERNEL);;

			if (!va[1] || !va[0]) {
				printk("%s: kmalloc failed on %luB\n", __FUNCTION__, n);
				return;
			}

			for (t=0; fn[t]; t++) {
				if (fn[t][0] == '/') 
					continue;

				cond_resched();
				i = 0;
				time = jiffies + msecs_to_jiffies(1000);
	
				while (jiffies <= time) {
					switch (t) {
						case 1: 
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
								dmac_inv_range((void*)va[0], (void*)va[0] + n);
#else
								dma_cache_maint((void*)va[0], n, DMA_FROM_DEVICE);
#endif
								memzero(va[0], n); 
						break;
#ifdef CONFIG_MV_XOR_MEMCOPY
						case 3: xor_memcpy(va[0], va[1], n); 
							break;
						case 4: asm_memcpy(va[0], va[1], n); 
							break;
#else
						case 4: memcpy(va[0], va[1], n); 
							break;
#endif
#ifdef CONFIG_MV_XOR_MEMXOR
						case 5: xor_memxor(3, n ,va); 
							break;
#endif
						case 6: xor_block(3, n ,va); 
							break;
						default:
							printk("%s skipped\n", fn[t]);
							time = 0;
					}
					
					i++;
				}

				printk("%s: %lu8B %8luMBs %s()\n", __FUNCTION__, 
					   n, (i * n) >> 20, fn[t]);

			}

			kfree(va[2]);
			kfree(va[1]);
			kfree(va[0]);
		}
}
#endif

