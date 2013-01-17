/*
*
* Marvell Orion SD\MMC\SDIO driver
*
* Author: Maen Suleiman
* Copyright (C) 2008 Marvell Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/
/*
 * mvsdmmc TODO list:
 *
 * --> Set timeout value according to timeout_ns and timeout_clks,
 *  meanwhile a maximum value is set for the host
 *
 * --> Report errors in mrq->data->stop->error and in mrq->data->error,
 *  meanwhile errors are reported in the command itself since always the
 *  completion of data and AutoCmd12 are done with the originated command.
 *
 *
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/mbus.h>
#endif
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/mmc/host.h>
#include <linux/proc_fs.h>
#include <linux/irq.h>


#include <asm/dma.h>
#include <asm/sizes.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <asm/plat-orion/mvsdmmc-orion.h>
#endif

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "mvsdmmc.h"

#undef MVSDMMC_DEBUG
#undef MVSDMMC_DUMP_REGS_ON_CMD
#define MVSDMMC_DUMP_ALL_REGS_ON_ERROR
#define MVSDMMC_DBG_ERROR

#undef MVSDMMC_DBG_FUNC_ENTRY
#ifdef MVSDMMC_DBG_FUNC_ENTRY
#define mvsdmmc_dbg_enter()	printk(KERN_DEBUG "ENTER <=%s\n",  __func__)
#define mvsdmmc_dbg_exit()	printk(KERN_DEBUG "EXIT  <=%s\n",  __func__)
#else
#define mvsdmmc_dbg_enter()
#define mvsdmmc_dbg_exit()
#endif

#undef MVSDMMC_WARN
#ifdef MVSDMMC_WARN
#define mvsdmmc_warning(host, fmt, arg...) 	\
			dev_printk(KERN_INFO, &host->pdev->dev, fmt, ##arg) 
#else 
#define mvsdmmc_warning(host, a...) 
#endif 
#ifdef MVSDMMC_DEBUG
#define mvsdmmc_debug(host, fmt, arg...) 	\
			dev_printk(KERN_DEBUG, &host->pdev->dev, fmt, ##arg) 
#else 
#define mvsdmmc_debug(host, a...) 
#endif

#if defined(MVSDMMC_DBG_ERROR)
#define mvsdmmc_debug_error(host, fmt, arg...)	\
			  dev_printk(KERN_ERR, &host->pdev->dev, fmt, ##arg) 
#else 
#define mvsdmmc_debug_error(host, a...) 
#endif

#define DRIVER_NAME	"mvsdmmc"


struct mvsdmmc_host_stat {

	int total_requests;
	int copied_data;
	int total_data;
	int unaligned_buf;
	int cache_unaligned_buf;
	int ints;
	int error_ints;
	int card_ints;
	int unfinished_dma;
	int buf_256;
	int cmd_timeout;
	int int_timeout;
	int empty_int;
	int first_int_status;
	int first_error_status;
	int detect_int;
	int first_err_int;	/* request number of first error interrupt */
	int first_unfinished_dma;/* request number of first unfinished dma */ 
};

struct mvsdmmc_host {
	struct mmc_host		*mmc;		/* associated mmc structure */
	struct platform_device	*pdev;		/* platform device */
	spinlock_t		lock;		/* spin lock of the host */
	struct resource		*res;		/* resource for IRQ and base */
	void __iomem		*base;		/* base address of the host
						 *  registers
						 */
	int			irq;		/* host IRQ number */
	int			irq_detect;	/* host IRQ number for
						 * insertion/detection
						 */
	char			*dma_buffer;	/* virtual address for
						temp buffer*/
	dma_addr_t		dma_addr;	/* Physical address for
						remp buffer */
	unsigned int		dma_len;	/* sg fragments number
						 * of the request
						 */
	int			size;		/* Total size of transfer */
	unsigned char		power_mode;	/* power status */
	struct mmc_request	*mrq;		/* current mmc request
						 *  structure
						 */
	struct mmc_command	*cmd;		/* current mmc command
						 * structure
						 */
	struct mmc_data		*data;		/* current mmc data structure */
	unsigned short		intr_status;	/* interrupt status on IRQ */
	unsigned short		intr_en;	/* enabled interrupts
						 * during command- status
						 */
	unsigned short		intr_cmd;	/* enabled interrupts
						 * during command
						 */
	unsigned int		cmd_data;	/* if cmd\data are proccesed */
	#define	MVSDMMC_CMD	0x1
	#define	MVSDMMC_DATA	0x2
	#define	MVSDMMC_CMD12	0x4
	unsigned int		pending_commands;/* commands on process*/
	int			card_present;	/* if card present*/
	struct timer_list	timer;		/* Timer for timeouts */
	unsigned int		copy_buf;	/* copy to host buffer*/
	unsigned int		bad_size;	/* unaligned size */
	struct mvsdmmc_host_stat	stat;	/* host statistics */
};


static int maxfreq = MVSDMMC_CLOCKRATE_MAX; 
static int highspeed = 1;
static int detect = 1;
static int dump_on_error;

static void mvsdmmc_request_done(struct mvsdmmc_host *host);
static void mvsdmmc_power_down(struct mmc_host *mmc);
static void mvsdmmc_power_up(struct mmc_host *mmc);
static void mvsdmmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios);

static inline void mvsdmmc_sg_to_dma(struct mvsdmmc_host *host,
				     struct mmc_data *data);
static inline void mvsdmmc_dma_to_sg(struct mvsdmmc_host *host,
				     struct mmc_data *data);
static irqreturn_t mvsdmmc_irq(int irq, void *dev);
static irqreturn_t mvsdmmc_irq_detect(int irq, void *dev);

#define mvsdmmc_writew(host, offs, val)	\
		writew((val), (host->base + offs))


static inline unsigned short mvsdmmc_readw(struct mvsdmmc_host *host,
					   unsigned short offs)
{
	unsigned short val = readw((host->base + offs));
	return (val);
}

#define mvsdmmc_bitset(host, offs, bitmask)	\
		writew((readw(host->base + offs) | (bitmask)),	\
			host->base + offs)

#define mvsdmmc_bitreset(host, offs, bitmask)	\
		writew((readw(host->base + offs) & (~(bitmask))),	\
			host->base + offs)

int mvsdmmc_read_procmem(char *buf, char **start, off_t offset,
			 int count, int *eof, void *data) 
{

	int len = 0;
	struct mvsdmmc_host *host = (struct mvsdmmc_host *)data;

	len += sprintf(buf+len, "\ntotal requests=%d\n",
		host->stat.total_requests);
	len += sprintf(buf+len, "total data=%d\n",
		host->stat.total_data);
	len += sprintf(buf+len, "copied data=%d\n", host->stat.copied_data);
	len += sprintf(buf+len, "buffers small than 256=%d\n",
		host->stat.buf_256);
	len += sprintf(buf+len, "buffers not word aligned=%d\n",
			 host->stat.unaligned_buf);
	len += sprintf(buf+len, "buffers not cache line aligned=%d\n",
			host->stat.cache_unaligned_buf);
	len += sprintf(buf+len, "total interrupts=%d\n", host->stat.ints);
	len += sprintf(buf+len, "total card ints=%d\n", host->stat.card_ints);
	len += sprintf(buf+len, "total error ints=%d\n", host->stat.error_ints);
	len += sprintf(buf+len, "Request num of first error int=%d\n",
			host->stat.first_err_int);
	len += sprintf(buf+len, "Status of int on first error int=0x%x\n",
			(unsigned int)host->stat.first_int_status);
	len += sprintf(buf+len, "Status of first error int=0x%x\n",
			(unsigned int)host->stat.first_error_status);
	len += sprintf(buf+len, "Empty interrupts=%d\n", host->stat.empty_int);
	len += sprintf(buf+len, "Detect interrupts=%d\n",
				host->stat.detect_int);
	len += sprintf(buf+len, "Unifinished dma=%d\n",
		host->stat.unfinished_dma);
	len += sprintf(buf+len, "Request num of first unfinished dma=%d\n",
			host->stat.first_unfinished_dma);
	len += sprintf(buf+len, "interrupt timeouts=%d\n",
		host->stat.int_timeout);
	len += sprintf(buf+len, "command timeouts=%d\n",
					host->stat.cmd_timeout);

	*eof = 1;

	memset(&host->stat , sizeof(struct mvsdmmc_host_stat), 0);

	return len;

}

static void mvsdmmc_dump_registers(struct mvsdmmc_host *host,
				   unsigned short cmdreg)
{
	unsigned int reg;

	for (reg = SDIO_SYS_ADDR_LOW; reg <= SDIO_AUTO_RSP2; reg += 4) {
		if (reg == SDIO_CMD) {
			mvsdmmc_debug(host, "reg 0x%x = 0x%x \n", reg,
				      (unsigned int)cmdreg);
			continue;
		}

		mvsdmmc_debug(host, "reg 0x%x = 0x%x \n", reg,
			      (unsigned int)mvsdmmc_readw(host, reg));
	}
	for (reg = 0x100; reg <= 0x130; reg += 4) {
		mvsdmmc_debug(host, "reg 0x%x = 0x%x \n", reg,
			      (unsigned int)mvsdmmc_readw(host, reg));
	}
}


static inline u32 mvsdmmc_aligned_size(u32 size)
{
	return ((size + 3) / 4) * 4;
}

/*
 * Tx alignment. This is a workaround for problems with
 * data sizes which are not 4-bytes aligned.
 * Currently this function only works for LSB_FIRST=0.
 */
static void mvsdmmc_align_tx_data(struct mvsdmmc_host *host,
					struct mmc_data *data)
{
	u32 size = data->blocks * data->blksz;
	u32 aligned_size = mvsdmmc_aligned_size(size);
	u8 *ptr = (u8 *)sg_virt_addr(data->sg);
	u8 *end = ptr + aligned_size-4;
	u32 remainder = size%4;
	u8 tmp[4];

	if (!remainder)
		return;

	data->sg->length = aligned_size;
	memcpy(tmp, end, 4);
	memset(end, 0, 4);
	memcpy(end+4-remainder, tmp, remainder);
}


/*
 * Rx alignment. This is a workaround for problems with
 * data sizes which are not 4-bytes aligned.
 * Currently this function only works for LSB_FIRST=0
 */
static void mvsdmmc_align_rx_data(struct mvsdmmc_host *host,
					struct mmc_data *data)
{
	u32 size = data->blocks * data->blksz;
	u32 aligned_size = mvsdmmc_aligned_size(size);
	u8 *ptr = (u8 *)sg_virt_addr(data->sg);
	u8 *end = ptr + aligned_size-4;
	u32 remainder = size%4;
	u8 tmp[4];

	if (!remainder)
		return;

	data->sg->length = aligned_size;
	memcpy(tmp, end, 4);
	memset(end, 0, 4);
	memcpy(end, &tmp[4-remainder], remainder);
}






static void mvsdmmc_stop_clock(struct mvsdmmc_host *host)
{

	mvsdmmc_dbg_enter();
	mvsdmmc_bitset(host, SDIO_XFER_MODE, SDIO_XFER_MODE_STOP_CLK);
	mvsdmmc_dbg_exit();
}

static void __mvsdmmc_enable_irq(struct mvsdmmc_host *host, unsigned int mask)
{
	host->intr_en |= mask;
	mvsdmmc_writew(host, SDIO_NOR_INTR_EN, host->intr_en);
	host->intr_cmd |= mask; /* intr_cmd is zeroed in mvsdmmc_start_cmd */
	if (mask == SDIO_NOR_CARD_INT)
		mvsdmmc_bitset(host, SDIO_XFER_MODE,
			       SDIO_XFER_MODE_INT_CHK_EN);
}
static void mvsdmmc_enable_irq(struct mvsdmmc_host *host, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	__mvsdmmc_enable_irq(host, mask);
	spin_unlock_irqrestore(&host->lock, flags); 
}

static void __mvsdmmc_disable_irq(struct mvsdmmc_host *host, unsigned int mask)
{
	host->intr_en &= ~mask;
	mvsdmmc_writew(host, SDIO_NOR_INTR_EN, host->intr_en);
	if (mask == SDIO_NOR_CARD_INT)
		mvsdmmc_bitreset(host, SDIO_XFER_MODE,
				 SDIO_XFER_MODE_INT_CHK_EN);
}

static void mvsdmmc_disable_irq(struct mvsdmmc_host *host, unsigned int mask)
{
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	__mvsdmmc_disable_irq(host, mask);
	spin_unlock_irqrestore(&host->lock, flags); 
}

static void mvsdmmc_setup_data(struct mvsdmmc_host *host, struct mmc_data *data)
{
	char	*virt_addr = NULL;
	dma_addr_t	phys_addr = 0;
	u32		reg;

	mvsdmmc_dbg_enter();

	host->stat.total_data++;

	/*
	 * Calculate size.
	 */
	host->size = data->blocks * data->blksz;

	host->bad_size = 0;

	host->copy_buf = 0;
	
	if (data->flags & MMC_DATA_READ) {

		if (host->size & 0x3)
			mvsdmmc_align_rx_data(host, data);

		host->dma_len = dma_map_sg(mmc_dev(host->mmc), data->sg,
					   data->sg_len,
					   DMA_FROM_DEVICE);

		BUG_ON(host->dma_len == 0);
		phys_addr = sg_dma_address(data->sg);

		if (host->dma_len > 1) {
			host->copy_buf = 1;
		}
	}

	if (data->flags & MMC_DATA_WRITE) {

		if (host->size & 0x3)
			mvsdmmc_align_tx_data(host, data);

		host->dma_len = dma_map_sg(mmc_dev(host->mmc),
					   data->sg, data->sg_len,
					   DMA_TO_DEVICE);
		BUG_ON(host->dma_len == 0);
		phys_addr = sg_dma_address(data->sg);

		if (host->dma_len > 1)
			host->copy_buf = 1;
	}

	if (phys_addr & 0x3)
		host->copy_buf = 1;

	reg = mvsdmmc_readw(host, SDIO_HOST_CTRL);
	
	if (host->bad_size) {
		reg &= ~SDIO_HOST_CTRL_BIG_ENDIAN;
		host->copy_buf = 1;
	} else {
		reg |= SDIO_HOST_CTRL_BIG_ENDIAN;
	}
	mvsdmmc_writew(host, SDIO_HOST_CTRL, reg);

        if (host->copy_buf) {

			virt_addr = host->dma_buffer;
			phys_addr = host->dma_addr;
	}

	BUG_ON(host->size > MVSDMMC_DMA_SIZE);

	if ((data->flags & MMC_DATA_WRITE) && (host->copy_buf)) {
		/*
		* Transfer data from the SG list to
		* the DMA buffer.
		*/
		BUG_ON(virt_addr == NULL);
		mvsdmmc_sg_to_dma(host, data);
		dma_sync_single_for_device(mmc_dev(host->mmc),
					host->dma_addr,
					mvsdmmc_aligned_size(host->size),
					DMA_TO_DEVICE);

	} else if ((data->flags & MMC_DATA_READ) && (host->copy_buf)) {

		dma_sync_single_for_device(mmc_dev(host->mmc),
					host->dma_addr,
					mvsdmmc_aligned_size(host->size),
					DMA_FROM_DEVICE);
	}

	mvsdmmc_writew(host, SDIO_BLK_COUNT, data->blocks);
	mvsdmmc_writew(host, SDIO_SYS_ADDR_LOW,
		       (0xffff & ((unsigned int)phys_addr)));
	mvsdmmc_writew(host, SDIO_SYS_ADDR_HI,
		       (0xffff & (((unsigned int)phys_addr) >> 16)));

	mvsdmmc_writew(host, SDIO_BLK_SIZE, data->blksz);

	mvsdmmc_dbg_exit();

}

static void mvsdmmc_start_cmd(struct mvsdmmc_host *host,
			      struct mmc_command *cmd)
{

	struct mmc_data		*data = host->mrq->data;
	struct mmc_request	*mrq = cmd->mrq;
	unsigned short cmdreg = 0, xfer = 0;
	unsigned short intr_enable = 0;

	mvsdmmc_dbg_enter();
	BUG_ON(host->cmd_data);
	BUG_ON(host->cmd == NULL);

	/* disable interrupts */
	mvsdmmc_writew(host, SDIO_ERR_INTR_EN, 0);
	/* clear error status */
	mvsdmmc_writew(host, SDIO_ERR_INTR_STATUS,
			mvsdmmc_readw(host, SDIO_ERR_INTR_STATUS));

	BUG_ON(host->intr_en & ~SDIO_NOR_CARD_INT);

	/* reset host->intr_cmd*/
	host->intr_cmd = 0;

	if (cmd->flags != MMC_RSP_NONE) {
		intr_enable |= SDIO_NOR_UNEXP_RSP;
		cmdreg |= SDIO_UNEXPECTED_RESP;
	}
	if (cmd->flags & MMC_RSP_OPCODE)
		cmdreg |= SDIO_CMD_INDX_CHECK;


	if (cmd->flags == MMC_RSP_NONE)
		cmdreg |= SDIO_CMD_RSP_NONE;
	else if (cmd->flags & MMC_RSP_BUSY)
		cmdreg |= SDIO_CMD_RSP_48BUSY;
	else if (cmd->flags & MMC_RSP_136)
		cmdreg |= SDIO_CMD_RSP_136;
	else if (cmd->flags & MMC_RSP_PRESENT)
		cmdreg |= SDIO_CMD_RSP_48;

	if (cmd->flags & MMC_RSP_CRC)
		cmdreg |= (SDIO_CMD_CHECK_CMDCRC);

	if (data) {
		host->data = mrq->data;
		mvsdmmc_setup_data(host, mrq->data);

		/* if multiple blocks and need AutoCMD12*/
		if (data->stop) {
			struct mmc_command *stop = data->stop;
			mvsdmmc_writew(host, SDIO_AUTOCMD12_ARG_LOW,
				       stop->arg & 0xffff);
			mvsdmmc_writew(host, SDIO_AUTOCMD12_ARG_HI,
				       stop->arg >> 16);
			mvsdmmc_writew(host, SDIO_AUTOCMD12_INDEX,
				       (stop->opcode << 8) | 3);
			/* enable autocmd12 interrupt*/
			intr_enable |= SDIO_NOR_AUTOCMD12_DONE;

			xfer |= SDIO_XFER_MODE_AUTO_CMD12;
			host->cmd_data = (MVSDMMC_CMD | MVSDMMC_CMD12);
		} else {
			/* enable data interrupt*/
			intr_enable |= SDIO_NOR_DMA_INI;
			host->cmd_data = (MVSDMMC_CMD|MVSDMMC_DATA);
		}

		/* default values */
		cmdreg |= SDIO_CMD_CHECK_DATACRC16;
		cmdreg |= SDIO_CMD_DATA_PRESENT;
		xfer |= SDIO_XFER_MODE_HW_WR_DATA_EN;

		if (data->flags & MMC_DATA_READ)
			xfer |= SDIO_XFER_MODE_TO_HOST;
		else if (data->flags & MMC_DATA_WRITE)
			xfer &= ~SDIO_XFER_MODE_TO_HOST;
	} else {
		intr_enable |= SDIO_NOR_CMD_DONE;
		cmdreg &= ~SDIO_CMD_DATA_PRESENT;
		host->cmd_data = MVSDMMC_CMD;
	}

	if (host->intr_en & SDIO_NOR_CARD_INT)
		xfer |= SDIO_XFER_MODE_INT_CHK_EN;

	cmdreg |= ((cmd->opcode & 0xff) << 0x8);

	mvsdmmc_writew(host, SDIO_ARG_LOW, cmd->arg & 0xffff);
	mvsdmmc_writew(host, SDIO_ARG_HI, cmd->arg >> 16);
	mvsdmmc_writew(host, SDIO_XFER_MODE, xfer);

	/* start timer */
	mod_timer(&host->timer, jiffies + 5 * HZ);

	__mvsdmmc_enable_irq(host, intr_enable);

	host->pending_commands++;

	/* enable error interrupts*/
	mvsdmmc_writew(host, SDIO_ERR_INTR_EN, 0xffff); 
#if defined(MVSDMMC_DUMP_REGS_ON_CMD)
	mvsdmmc_debug_error(host, "================================>\n");
	mvsdmmc_debug_error(host, "CMD%d Start \n", host->cmd->opcode);
	mvsdmmc_dump_registers(host, cmdreg);
#endif
	mvsdmmc_writew(host, SDIO_CMD, cmdreg);

	BUG_ON(host->intr_en == 0);
	BUG_ON(host->intr_en == SDIO_NOR_UNEXP_RSP);

}


static void mvsdmmc_finish_data(struct mvsdmmc_host *host) {
	char				*virt_addr;
	struct mmc_data	*data = host->data;
	u32		blocks_left;
	unsigned short response[3], resp_indx = 0;


	BUG_ON(data == NULL);
	BUG_ON(!(host->intr_status & SDIO_NOR_AUTOCMD12_DONE) &&
		!(host->intr_status & SDIO_NOR_DMA_INI));

	if (host->intr_status & SDIO_NOR_AUTOCMD12_DONE) {
		host->cmd_data &= ~MVSDMMC_CMD12;
		host->intr_status &= ~(SDIO_NOR_AUTOCMD12_DONE);
	} else if (host->intr_status & SDIO_NOR_DMA_INI) {
		host->cmd_data &= ~MVSDMMC_DATA;
		host->intr_status &= ~(SDIO_NOR_DMA_INI);
	}

	if (!host->copy_buf)
		BUG_ON(host->dma_len == 0);

	if ((data->flags & MMC_DATA_READ) && (host->copy_buf)) {

		virt_addr = host->dma_buffer;
		BUG_ON(virt_addr == NULL);
		dma_sync_single_for_cpu(mmc_dev(host->mmc),
					host->dma_addr,
					host->size,
					DMA_FROM_DEVICE);

		/*
		* Transfer data from DMA buffer to
		* SG list.
		*/
		mvsdmmc_dma_to_sg(host, data);

		mvsdmmc_align_rx_data(host, data);

	} else if ((data->flags & MMC_DATA_WRITE) && (host->copy_buf)) {

		dma_sync_single_for_cpu(mmc_dev(host->mmc),
					host->dma_addr,
					host->size,
					DMA_TO_DEVICE);

	}

	if ((!host->copy_buf) && (data->flags & MMC_DATA_READ))
		dma_unmap_sg(mmc_dev(host->mmc), data->sg, host->dma_len,
			     DMA_FROM_DEVICE);

	if ((!host->copy_buf) && (data->flags & MMC_DATA_WRITE))
		dma_unmap_sg(mmc_dev(host->mmc), data->sg, host->dma_len,
			     DMA_TO_DEVICE);

	/* check how much data was transfered */
	blocks_left = mvsdmmc_readw(host, SDIO_CURR_BLK_LEFT);
	if (blocks_left) {
		u32 bytes_left = mvsdmmc_readw(host, SDIO_CURR_BYTE_LEFT);
		data->bytes_xfered = host->size -
			(((blocks_left - 1) * data->blksz) + bytes_left);
	} else {
		data->bytes_xfered = host->size;
	}

	/* Handle Auto cmd 12 response */
	if (data->stop) {

		for (resp_indx = 0 ; resp_indx < 3; resp_indx++)
			response[resp_indx] =
				mvsdmmc_readw(host, SDIO_AUTO_RSP(resp_indx));

		memset(data->stop->resp, 0, 4 * sizeof(data->stop->resp[0]));

		data->stop->resp[0] = ((response[2] & 0x3f) << (8 - 8)) |
			((response[1] & 0xffff) << (14 - 8)) |
			((response[0] & 0x3ff) << (30 - 8));
		data->stop->resp[1] = ((response[0] & 0xfc00) >> 10);

	}


	if (data->bytes_xfered != host->size) {
		host->stat.unfinished_dma++;

		if (host->stat.unfinished_dma == 1)
			host->stat.first_unfinished_dma =
				host->stat.total_requests;

		mvsdmmc_warning(host, "data transfere not complete\n");
		mvsdmmc_warning(host, "data transfered =%d"
					  "original size= %d\n",
					data->bytes_xfered, host->size);
	}

	host->data = NULL;
}

static void mvsdmmc_finish_cmd(struct mvsdmmc_host *host) {
	unsigned short response[8], resp_indx = 0;
	struct mmc_command	*cmd = host->cmd;

	host->intr_status &= ~(SDIO_NOR_CMD_DONE);
	host->cmd_data &= ~MVSDMMC_CMD;

	for (resp_indx = 0 ; resp_indx < 8; resp_indx++)
		response[resp_indx] = mvsdmmc_readw(host, SDIO_RSP(resp_indx));

	memset(cmd->resp, 0, 4 * sizeof(cmd->resp[0]));

	if (cmd->flags & MMC_RSP_136) {
		cmd->resp[3] = ((response[7] & 0x3fff) << 8)	|
			((response[6] & 0x3ff) << 22);
		cmd->resp[2] = ((response[6] & 0xfc00) >> 10)	|
			((response[5] & 0xffff) << 6)	|
			((response[4] & 0x3ff) << 22);
		cmd->resp[1] = ((response[4] & 0xfc00) >> 10)	|
			((response[3] & 0xffff) << 6)	|
			((response[2] & 0x3ff) << 22);
		cmd->resp[0] = ((response[2] & 0xfc00) >> 10)	|
			((response[1] & 0xffff) << 6)	|
			((response[0] & 0x3ff) << 22);
	} else  if (cmd->flags & MMC_RSP_PRESENT) {
		cmd->resp[0] = ((response[2] & 0x3f) << (8 - 8)) |
			((response[1] & 0xffff) << (14 - 8)) |
			((response[0] & 0x3ff) << (30 - 8));
		cmd->resp[1] = ((response[0] & 0xfc00) >> 10);
	}
}

static unsigned int mvsdmmc_check_error(struct mvsdmmc_host *host) {
	unsigned short		error_status;
	unsigned int		error = 0;

	error_status = mvsdmmc_readw(host, SDIO_NOR_INTR_STATUS);
	/* make sure if there is an error*/
	if (error_status & SDIO_NOR_UNEXP_RSP) {
		error = -EPROTO;
		mvsdmmc_warning(host, "SDIO_NOR_INTR_STATUS(0x%x)=0x%x"
				    "\n", SDIO_NOR_INTR_STATUS, error_status);
	} else if (error_status & SDIO_NOR_ERROR) {
		/* clear error status*/
		mvsdmmc_writew(host, SDIO_NOR_INTR_STATUS, SDIO_NOR_ERROR);

		error_status = mvsdmmc_readw(host, SDIO_ERR_INTR_STATUS);

		mvsdmmc_warning(host, "SDIO_ERR_INTR_STATUS(0x%x)=0x%x"
				    "\n", SDIO_ERR_INTR_STATUS,
				    error_status);

		if (error_status & (SDIO_ERR_CMD_TIMEOUT |
				    SDIO_ERR_DATA_TIMEOUT)) {
			error = -ETIMEDOUT;
		} else {
			error = EILSEQ;
		}
	}

	if (host->cmd)
		host->cmd->error = error;

	return error;
}

static void mvsdmmc_request_done(struct mvsdmmc_host *host) {
	struct mmc_request *mrq = host->mrq;
	struct mmc_command	*cmd = host->cmd;

	if ((cmd->error) && (host->data)) {
		char *virt_addr;
		int i;

		host->intr_status = host->intr_cmd;
		__mvsdmmc_disable_irq(host, host->intr_cmd);
		if (!host->copy_buf)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
			virt_addr = (char *)sg_virt(host->data->sg);
#else
			virt_addr = (char *)sg_virt_addr(host->data->sg);
#endif
		else
			virt_addr = (char *)host->dma_buffer;

		mvsdmmc_warning(host, "error: data size =%d, block size= %d "
				    "block numbers=%d\n",
				    host->size, host->data->blksz,
				    host->data->blocks);

		if (dump_on_error) {
			for (i = 0 ; i < host->size ; i++) {
				if (i % 32 == 0)
					mvsdmmc_warning(host, "\n 0x%04X:",
							    (unsigned int)i);
				mvsdmmc_warning(host, "%02X ", *virt_addr++);
			}
		}
		mvsdmmc_warning(host, "\n");
		mvsdmmc_finish_data(host);
	}

	if ((cmd->error)) {

		/* disable pending interrupts*/
		__mvsdmmc_disable_irq(host, host->intr_cmd);
		/* clear pending interrupts*/
		mvsdmmc_writew(host, SDIO_NOR_INTR_STATUS, host->intr_cmd);

		host->cmd_data = 0;
		host->intr_status = 0;
	}

	host->mrq = NULL;
	host->cmd = NULL;
	host->pending_commands--;

	mmc_request_done(host->mmc, mrq);
}

static irqreturn_t mvsdmmc_irq_detect(int irq, void *dev) 
{
	struct mmc_host *mmc = (struct mmc_host *)dev;
	struct mvsdmmc_host *host = mmc_priv(mmc);

	host->stat.detect_int++;
	mmc_detect_change(mmc, msecs_to_jiffies(100));
	return IRQ_HANDLED;
}

static irqreturn_t mvsdmmc_irq(int irq, void *dev) 
{
	struct mmc_host *mmc = (struct mmc_host *)dev;
	struct mvsdmmc_host *host = mmc_priv(mmc);
	unsigned long flags;
	int handled = IRQ_NONE;
	int cardint = 0;
	int cmd_data_int = 0;
	unsigned int	error;

	mvsdmmc_dbg_enter();
	spin_lock_irqsave(&host->lock, flags);

	host->stat.ints++;

	if (host->pending_commands)
		del_timer(&host->timer);

	/* first make sure if there is an error*/
	error = mvsdmmc_check_error(host);


#if defined(MVSDMMC_DUMP_REGS_ON_CMD)
	if (host->cmd)  {
		mvsdmmc_debug_error(host, "==============================>\n");
		mvsdmmc_debug_error(host, "CMD%d Response \n",
					host->cmd->opcode);
		mvsdmmc_dump_registers(host,
				       mvsdmmc_readw(host, SDIO_CMD));
	}
#endif

	if (error) {

		host->stat.error_ints++;

		if (dump_on_error)
			mvsdmmc_dump_registers(host, mvsdmmc_readw(host,
								   SDIO_CMD));

		if (host->stat.error_ints == 1) {
			host->stat.first_err_int = host->stat.total_requests;
			host->stat.first_int_status =
				mvsdmmc_readw(host, SDIO_NOR_INTR_STATUS);
			host->stat.first_error_status =
				mvsdmmc_readw(host, SDIO_ERR_INTR_STATUS);
		}

		/* clear error interrupts*/
		mvsdmmc_writew(host, SDIO_ERR_INTR_STATUS,
				mvsdmmc_readw(host, SDIO_ERR_INTR_STATUS));

		mvsdmmc_writew(host, SDIO_ERR_INTR_EN, 0);

		mvsdmmc_warning(host, "command,data error \n");
		handled = IRQ_HANDLED;
		goto done;
	}

	/* read interrupts status */
	host->intr_status = mvsdmmc_readw(host, SDIO_NOR_INTR_STATUS);
	/* handle only enabled interrupts*/
	host->intr_status &= mvsdmmc_readw(host, SDIO_NOR_INTR_EN);

	/* disable pending interrupts*/
	__mvsdmmc_disable_irq(host, host->intr_status);
	/* clear pending interrupts*/
	mvsdmmc_writew(host, SDIO_NOR_INTR_STATUS, host->intr_status);

	if (host->intr_status & SDIO_NOR_CARD_INT) {
		host->intr_status &= ~SDIO_NOR_CARD_INT;
		host->stat.card_ints++;
		cardint = 1;
		handled = IRQ_HANDLED;
	}

	if (host->intr_status) {
		cmd_data_int = 1;
		if ((host->intr_status & SDIO_NOR_AUTOCMD12_DONE) ||
			(host->intr_status & SDIO_NOR_DMA_INI)) {

			mvsdmmc_finish_data(host);
			/* if data only interrupt was enable,
			 * then handle command now as well
			 */
			if (!(host->intr_cmd & SDIO_NOR_CMD_DONE))
				mvsdmmc_finish_cmd(host);
		}

		if (host->intr_status & SDIO_NOR_CMD_DONE)
			mvsdmmc_finish_cmd(host);

		handled = IRQ_HANDLED;
	}

	if (cardint)
		mmc_signal_sdio_irq(host->mmc);

	if (handled != IRQ_HANDLED) {
		/* clear interrupts*/
		__mvsdmmc_disable_irq(host, 0xffff);
		dev_printk(KERN_WARNING, &host->pdev->dev , "interrupt not handled!!! int status = 0x%x\n", mvsdmmc_readw(host, SDIO_NOR_INTR_STATUS));

		mvsdmmc_writew(host, SDIO_NOR_INTR_STATUS,
			       mvsdmmc_readw(host, SDIO_NOR_INTR_STATUS));

		if ((host->cmd_data&MVSDMMC_DATA) && (host->data)) {
			host->data->error = ETIMEDOUT;
			host->intr_status = SDIO_NOR_DMA_INI; /* dummy */
			mvsdmmc_finish_data(host);
		}
		if ((host->cmd_data&MVSDMMC_CMD) && (host->cmd)) {
			host->cmd->error = ETIMEDOUT;
			mvsdmmc_finish_cmd(host);
		}

		handled = IRQ_HANDLED;
		host->stat.empty_int++;
	}
done:
	if ((cmd_data_int) ||
	    ((host->pending_commands) && (host->cmd->error))) {
		/* Since we may and may not have SDIO_NOR_UNEXP_RSP interrupt
		 * make sure SDIO_NOR_UNEXP_RSP cleared and disabled before next
		 * command
		 */
		host->intr_status &= ~SDIO_NOR_UNEXP_RSP;
		__mvsdmmc_disable_irq(host, SDIO_NOR_UNEXP_RSP);
		mvsdmmc_bitset(host, SDIO_NOR_INTR_STATUS, SDIO_NOR_UNEXP_RSP);
		mvsdmmc_request_done(host);
	}
	spin_unlock_irqrestore(&host->lock, flags);

	mvsdmmc_dbg_exit();
	return handled;

}

static void mvsdmmc_timeout_timer(unsigned long data)
{
	struct mvsdmmc_host *host;
	irqreturn_t retval;

	host = (struct mvsdmmc_host *)data;

	/* disable interrupts */
	mvsdmmc_writew(host, SDIO_ERR_INTR_EN, 0);

	host->stat.int_timeout++;
	mvsdmmc_debug_error(host, "timeout for cmd%d\n",host->cmd->opcode);

	mvsdmmc_bitset(host, SDIO_XFER_MODE, SDIO_XFER_MODE_STOP_CLK); 
	mvsdmmc_bitset(host, SDIO_HOST_CTRL,
			 SDIO_HOST_CTRL_HI_SPEED_EN);
	mvsdmmc_bitreset(host, SDIO_HOST_CTRL,
			 SDIO_HOST_CTRL_HI_SPEED_EN);
	/* reset */
	mvsdmmc_writew(host, SDIO_SW_RESET, 0x100);
	/* enable the clock*/
	mvsdmmc_bitreset(host, SDIO_XFER_MODE, SDIO_XFER_MODE_STOP_CLK); 

	if (dump_on_error)
		mvsdmmc_dump_registers(host, mvsdmmc_readw(host, SDIO_CMD));

	if (host->pending_commands) {
		mvsdmmc_debug_error(host, "timeout:trying to call interrupt routine\n");
		/* first try to handle any pending interrupts*/
		retval =  mvsdmmc_irq(0, (void *)host->mmc);
	}
}

/* 32bit byte swap. For example 0x11223344 -> 0x44332211                    */
#define MV_BYTE_SWAP_32BIT(X) ((((X)&0xff)<<24) |                       \
                               (((X)&0xff00)<<8) |                      \
                               (((X)&0xff0000)>>8) |                    \
                               (((X)&0xff000000)>>24))

static inline void swap_buf(void *buf, int size) {

	int i, aligned_size = (size & 0x3)?((size & ~0x3) + 4):size;
	u32 temp, *buf32 = (u32*)buf;

	for (i = 0; i < aligned_size; i+=4) {
		temp = *buf32;
		*buf32 = MV_BYTE_SWAP_32BIT((temp));
		buf32++;
	}
}

static inline void mvsdmmc_sg_to_dma(struct mvsdmmc_host *host,
					struct mmc_data *data)
{
	unsigned int len, i;
	struct scatterlist *sg;
	char *dmabuf = host->dma_buffer;
	char *sgbuf;

	sg = data->sg;
	len = data->sg_len;

	for (i = 0; i < len; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
		sgbuf = (char *)sg_virt((&sg[i]));
#else
		sgbuf = (char *)sg_virt_addr((&sg[i]));
#endif

		memcpy(dmabuf, sgbuf, sg[i].length);
		dmabuf += sg[i].length;
	}
	if (host->bad_size)
		swap_buf(host->dma_buffer, host->size);
}

static inline void mvsdmmc_dma_to_sg(struct mvsdmmc_host *host,
					struct mmc_data *data)
{
	unsigned int len, i;
	struct scatterlist *sg;
	char *dmabuf = host->dma_buffer;
	char *sgbuf;

	sg = data->sg;
	len = data->sg_len;

	if (host->bad_size)
		swap_buf(host->dma_buffer, host->size);

	for (i = 0; i < len; i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
		sgbuf = (char *)sg_virt((&sg[i]));
#else
		sgbuf = (char *)sg_virt_addr((&sg[i]));
#endif
		memcpy(sgbuf, dmabuf, sg[i].length);
		dmabuf += sg[i].length;
	}
}


static int mvsdmmc_dma_init(struct mmc_host *mmc)
{
	struct mvsdmmc_host *host = mmc_priv(mmc);

	mvsdmmc_dbg_enter();

	host->dma_buffer = NULL;
	host->dma_addr = (dma_addr_t)NULL;

	/*
	* We need to allocate a special buffer in
	* order for SDIO host to be able to DMA to it.
	*/

	host->dma_buffer = kmalloc(MVSDMMC_DMA_SIZE,
		GFP_NOIO | GFP_DMA | __GFP_REPEAT | __GFP_NOWARN);

	if (!host->dma_buffer)
		goto err;

	/*
	* Translate the address to a physical address.
	*/

	host->dma_addr = dma_map_single(mmc_dev(host->mmc), host->dma_buffer,
		MVSDMMC_DMA_SIZE, DMA_BIDIRECTIONAL);

	/*
	* SDIO host DMA must be aligned on a 4 byte basis.
	*/
	if ((host->dma_addr & 0x3) != 0) {

		mvsdmmc_warning(host, "mvsdmmc: dma alignment error\n");
		goto kfree;
	}



	mvsdmmc_dbg_exit();
	return 0;
kfree:
	/*
	 * If we've gotten here then there is some kind of alignment bug
	 */

	dma_unmap_single(mmc_dev(host->mmc), host->dma_addr,
		MVSDMMC_DMA_SIZE, DMA_BIDIRECTIONAL);
	host->dma_addr = (dma_addr_t)NULL;

	kfree(host->dma_buffer);
	host->dma_buffer = NULL;

	return -ENOMEM;

err:
	printk(KERN_WARNING DRIVER_NAME ": Unable to allocate DMA %d. "
		"Falling back on FIFO.\n", (unsigned int)host->dma_buffer);

	return -ENOMEM;

}


static void mvsdmmc_request(struct mmc_host *mmc, struct mmc_request *mrq) 
{
	struct mvsdmmc_host *host = mmc_priv(mmc);
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	WARN_ON(host->mrq != NULL);

	host->stat.total_requests++;

	BUG_ON(host->pending_commands == 1);

	host->mrq = mrq;
	host->cmd = mrq->cmd;
	host->data = mrq->data;

	mvsdmmc_start_cmd(host, mrq->cmd);

	spin_unlock_irqrestore(&host->lock, flags); 

	mvsdmmc_dbg_exit();

}

static void mvsdmmc_set_clock(struct mmc_host *mmc, unsigned int clock)
{
	struct mvsdmmc_host *host = mmc_priv(mmc);
	unsigned int m;
	mvsdmmc_dbg_enter();

	BUG_ON(clock == 0);

	if(MV_6183_DEV_ID == mvCtrlModelGet())
		m = MVSDMMC_BASE_FAST_CLOCK_ORION/(2*clock) - 1;
	else
		m = MVSDMMC_BASE_FAST_CLOCK_KW/(2*clock) - 1;

	mvsdmmc_debug(host, "mvsdmmc_set_clock: dividor = 0x%x clock=%d\n",
		      m, clock);

	mvsdmmc_writew(host, SDIO_CLK_DIV, m & 0x7ff);

	msleep(10);

	mvsdmmc_dbg_exit();
}

static void mvsdmmc_power_up(struct mmc_host *mmc)
{
	unsigned int reg;
	struct mvsdmmc_host *host = mmc_priv(mmc);


	mvsdmmc_dbg_enter();

	reg = mvsdmmc_readw(host, SDIO_HOST_CTRL);
	/* set sd-mem only*/
	reg &= ~SDIO_HOST_CTRL_CARD_TYPE_MASK;
	reg |= SDIO_HOST_CTRL_CARD_TYPE_MASK;

	/* set big endian */
	reg |= SDIO_HOST_CTRL_BIG_ENDIAN;
	reg &= ~SDIO_HOST_CTRL_LSB_FIRST;

	/* set maximum timeout */
	reg &= ~SDIO_HOST_CTRL_TMOUT_MASK;
	reg |= SDIO_HOST_CTRL_TMOUT_MAX;
	reg |= SDIO_HOST_CTRL_TMOUT_EN;
	mvsdmmc_writew(host, SDIO_HOST_CTRL, reg);

	mvsdmmc_writew(host, SDIO_NOR_STATUS_EN, 0xffff);
	mvsdmmc_writew(host, SDIO_ERR_STATUS_EN, 0xffff);
	mvsdmmc_writew(host, SDIO_NOR_INTR_EN, 0);
	mvsdmmc_writew(host, SDIO_NOR_INTR_STATUS, 0xffff);

	mvsdmmc_bitreset(host, SDIO_HOST_CTRL,
			 SDIO_HOST_CTRL_HI_SPEED_EN);

	/* reset */
	mvsdmmc_writew(host, SDIO_SW_RESET, 0x100);
	msleep(50);


	/* enable the clock*/
	mvsdmmc_bitreset(host, SDIO_XFER_MODE, SDIO_XFER_MODE_STOP_CLK);
	mvsdmmc_dbg_exit();
}
static void mvsdmmc_power_down(struct mmc_host *mmc)
{
	struct mvsdmmc_host *host = mmc_priv(mmc);

	mvsdmmc_dbg_enter();
	/* stop the clock*/
	mvsdmmc_bitset(host, SDIO_XFER_MODE, SDIO_XFER_MODE_STOP_CLK);
	mvsdmmc_writew(host, SDIO_NOR_STATUS_EN, 0);
	mvsdmmc_writew(host, SDIO_ERR_STATUS_EN, 0);
	mvsdmmc_writew(host, SDIO_NOR_INTR_EN, 0);
	mvsdmmc_writew(host, SDIO_NOR_INTR_STATUS, 0xffff);

	mvsdmmc_dbg_exit();
}



static void mvsdmmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct mvsdmmc_host *host = mmc_priv(mmc);
	
	mvsdmmc_dbg_enter();
	
	mvsdmmc_debug(host, "setting clock to %d\n", ios->clock);
	if (ios->clock)
		mvsdmmc_set_clock(mmc, ios->clock);
	else
		mvsdmmc_stop_clock(host);

	if ((ios->power_mode == MMC_POWER_UP) &&
	    (host->power_mode != MMC_POWER_UP)) {

		mvsdmmc_debug(host, "power up\n");
		mvsdmmc_power_up(mmc);
		host->power_mode = MMC_POWER_UP;
	}

	if ((ios->power_mode == MMC_POWER_OFF) &&
	    (host->power_mode != MMC_POWER_OFF)) {

		mvsdmmc_debug(host, " power off\n");
		mvsdmmc_power_down(mmc);
		host->power_mode = MMC_POWER_OFF;
	}

	if ((ios->power_mode == MMC_POWER_ON) &&
	    (host->power_mode != MMC_POWER_ON)) {

		mvsdmmc_debug(host, " power on\n");
		host->power_mode = MMC_POWER_ON;
	}

	if (ios->bus_mode == MMC_BUSMODE_OPENDRAIN) {
		mvsdmmc_debug(host, " set bus mode to opendrain\n");
		mvsdmmc_bitreset(host, SDIO_HOST_CTRL,
				 SDIO_HOST_CTRL_PUSH_PULL_EN);
	} else if (ios->bus_mode == MMC_BUSMODE_PUSHPULL) {

		mvsdmmc_debug(host, " set bus mode to pushpull\n");
		mvsdmmc_bitset(host, SDIO_HOST_CTRL,
				SDIO_HOST_CTRL_PUSH_PULL_EN);
	}

	if (ios->bus_width == MMC_BUS_WIDTH_1) {
		mvsdmmc_debug(host, " set width x1\n");
		mvsdmmc_bitreset(host, SDIO_HOST_CTRL,
				 SDIO_HOST_CTRL_DATA_WIDTH_4_BITS);
	} else if (ios->bus_width == MMC_BUS_WIDTH_4) {
		mvsdmmc_debug(host, " set width x4\n");
		mvsdmmc_bitset(host, SDIO_HOST_CTRL,
			       SDIO_HOST_CTRL_DATA_WIDTH_4_BITS);
	}

	mvsdmmc_dbg_exit();
}

static void mvsdmmc_enable_sdio_irq(struct mmc_host *host, int enable)
{
	struct mvsdmmc_host *mvsdmmc_host = mmc_priv(host);
	if (enable)
		mvsdmmc_enable_irq(mvsdmmc_host, SDIO_NOR_CARD_INT);
	else
		mvsdmmc_disable_irq(mvsdmmc_host, SDIO_NOR_CARD_INT);
}

static const struct mmc_host_ops mvsdmmc_ops = {
	.request	= mvsdmmc_request,
	.get_ro		= NULL,
	.set_ios	= mvsdmmc_set_ios,
	.enable_sdio_irq	= mvsdmmc_enable_sdio_irq,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
static void mv_conf_mbus_windows(struct mvsdmmc_host *host,
				 struct mbus_dram_target_info *dram) {
	int i;

	for (i = 0; i < 4; i++) {
		writel(0, host->base + WINDOW_CTRL(i));
		writel(0, host->base + WINDOW_BASE(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		struct mbus_dram_window *cs = dram->cs + i;
		writel(((cs->size - 1) & 0xffff0000) |
		       (cs->mbus_attr << 8) |
		       (dram->mbus_dram_target_id << 4) | 1,
		       host->base + WINDOW_CTRL(i));
		writel(cs->base, host->base + WINDOW_BASE(i));
	}
}
#endif

static int mvsdmmc_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc = NULL;
	struct mvsdmmc_host *host = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	const struct orion_mvsdmmc_data *mv_platform_data;
#endif
	struct resource *r;
	int ret = 0, irq = NO_IRQ;
	int irq_detect = NO_IRQ;

	mvsdmmc_dbg_enter();

	BUG_ON(pdev == NULL);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r)
		return -ENXIO;

	r = request_mem_region(r->start, SZ_1K, DRIVER_NAME);
	if (!r)
		return -EBUSY;

	irq = platform_get_irq(pdev, 0);

	if (irq == NO_IRQ) {
		ret = -ENXIO;
		goto out;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	mv_platform_data = pdev->dev.platform_data;
#endif

	printk( DRIVER_NAME ": irq =%d start %x\n", irq, r->start);

	if (detect) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
		irq_detect = mv_platform_data->detect_irq;
#else
		irq_detect = platform_get_irq(pdev, 1);
#endif

		if (irq_detect == NO_IRQ) {
			detect = 0;
			printk( DRIVER_NAME ": no IRQ detect\n");
		}
		else
			printk( DRIVER_NAME ": irq_detect=%d\n", irq_detect);
	}


	mmc = mmc_alloc_host(sizeof(struct mvsdmmc_host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto out;
	}

	mmc->ops = &mvsdmmc_ops;
	mmc->f_min = MVSDMMC_CLOCKRATE_MIN;
	mmc->f_max = (unsigned int)maxfreq;

	mmc->max_hw_segs = 1;
	mmc->max_phys_segs = 1;

	mmc->max_req_size = MVSDMMC_DMA_SIZE;
	mmc->max_seg_size = mmc->max_req_size;
	mmc->max_blk_count = MVSDMMC_DMA_SIZE;
	mmc->max_blk_size = 2048;

	host = mmc_priv(mmc);
	BUG_ON(host == NULL);

	host->pdev = pdev;
	host->mmc = mmc;
	host->irq = irq;

	if (detect)
		host->irq_detect = irq_detect;

	host->res = r;
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;

	mmc->caps |= MMC_CAP_4_BIT_DATA;

	if (highspeed) {
		mmc->caps |= MMC_CAP_SD_HIGHSPEED;
		mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
	}

	mmc->caps |= MMC_CAP_SDIO_IRQ;

	spin_lock_init(&host->lock);
	host->power_mode = MMC_POWER_OFF;

	host->base = ioremap(r->start, SZ_4K);

	if (!host->base) {
		ret = -ENOMEM;
		goto out;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	/*
	 * (Re-)program MBUS remapping windows if we are asked to.
	 */
	if (mv_platform_data->dram != NULL)
		mv_conf_mbus_windows(host, mv_platform_data->dram);
#endif

	if (request_irq(host->irq, mvsdmmc_irq,
			0 , DRIVER_NAME, mmc)) {
		mvsdmmc_debug(host, "cannot assign irq %d\n", host->irq);
		ret = -EINVAL;
		host->irq = NO_IRQ;
		goto out;
	}

	if (detect) {
		if (request_irq(host->irq_detect, mvsdmmc_irq_detect,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
				IRQT_RISING | IRQT_FALLING,
#else
				0,
#endif
				DRIVER_NAME, mmc)) {

			mvsdmmc_debug(host, "cannot assign irq %d\n",
				      host->irq_detect);
			ret = -EINVAL;
			host->irq_detect = NO_IRQ;
			goto out;
		}
	} else {
		mmc->caps |= MMC_CAP_NEEDS_POLL;
		host->irq_detect = NO_IRQ;
	}

	platform_set_drvdata(pdev, mmc);
	setup_timer(&host->timer, mvsdmmc_timeout_timer, (unsigned long)host);

	if (mvsdmmc_dma_init(mmc) != 0)
		goto out;

	if (mmc_add_host(mmc) != 0)
		goto out;

	create_proc_read_entry("mvsdmmc", 0 , NULL ,
			       mvsdmmc_read_procmem, host);

	mvsdmmc_dbg_exit();
	return 0;
out:
	if (host) {
		mvsdmmc_disable_irq(host, 0xffff);

		if (host->base) {
			mvsdmmc_stop_clock(host);
			iounmap(host->base);
		}


		if (host->irq != NO_IRQ)
			free_irq(host->irq, host);

		if (detect) {
			if (host->irq_detect != NO_IRQ)
				free_irq(host->irq_detect, host);
		}

		if (host->res)
			release_resource(host->res);
	}

	if (mmc)
		mmc_free_host(mmc);

	return ret;
}

static int mvsdmmc_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);

	if (mmc) {
		struct mvsdmmc_host *host = mmc_priv(mmc);
		mvsdmmc_disable_irq(host, 0xffff);

		cancel_delayed_work(&mmc->detect);

		mmc_remove_host(mmc);

		if (host) {
			mvsdmmc_stop_clock(host);


			if (host->irq != NO_IRQ)
				free_irq(host->irq, mmc);

			if (detect) {
				if (host->irq_detect != NO_IRQ)
					free_irq(host->irq_detect, mmc);
			}

			if (host->base)
				iounmap(host->base);

			if (host->res)
				release_resource(host->res);

			del_timer_sync(&host->timer);
		}
		mmc_free_host(mmc);
	}
	remove_proc_entry("mvsdmmc", NULL);
	return 0;
}

#ifdef CONFIG_PM
static int mvsdmmc_suspend(struct platform_device *dev, pm_message_t state,
			   u32 level)
{
	struct mmc_host *mmc = platform_get_drvdata(dev);
	int ret = 0;

	if (mmc && level == SUSPEND_DISABLE)
		ret = mmc_suspend_host(mmc, state);

	return ret;
}

static int mvsdmmc_resume(struct platform_device *dev, u32 level)
{
	struct mmc_host *mmc = platform_dev_get_drvdata(dev);
	int ret = 0;

	if (mmc && level == RESUME_ENABLE)
		ret = mmc_resume_host(mmc);

	return ret;
}
#else
#define mvsdmmc_suspend	NULL
#define mvsdmmc_resume	NULL
#endif


static struct platform_driver mvsdmmc_driver = {
	.probe		= mvsdmmc_probe,
	.remove		= mvsdmmc_remove,
	.suspend	= mvsdmmc_suspend,
	.resume		= mvsdmmc_resume,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

static int __init mvsdmmc_init(void)
{
	int ret = 0;
	mvsdmmc_dbg_enter();

	if (MV_FALSE == mvCtrlPwrClckGet(SDIO_UNIT_ID, 0)) {
		printk("\nWarning SDIO unit is Powered Off\n");
		return;
	}

	ret = platform_driver_register(&mvsdmmc_driver);

	mvsdmmc_dbg_exit();
	return ret;
}

static void __exit mvsdmmc_exit(void)
{
	mvsdmmc_dbg_enter();
	platform_driver_unregister(&mvsdmmc_driver);
	mvsdmmc_dbg_exit();
}

module_init(mvsdmmc_init);
module_exit(mvsdmmc_exit);

/* maximum frequency used in the driver (default 50MHz) */ 
module_param(maxfreq, int, 0);

/* do we support high speed, default yes*/ 
module_param(highspeed, int, 0);

module_param(dump_on_error, int, 0);

/* support detection removal\insersion (default = 1) */ 
module_param(detect, int, 0);


MODULE_AUTHOR("Maen Suleiman <maen@marvell.com>"); 
MODULE_DESCRIPTION("Marvell SD,SDIO,MMC Host Controller driver"); 
MODULE_LICENSE("GPL");
