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
/*******************************************************************************
* file_name - mvLinuxIalLib.c
*
* DESCRIPTION:
*   implementation for linux IAL lib functions.
*
* DEPENDENCIES:
*   mvLinuxIalLib.h
*   mvLinuxIalHt.h
*
*
*******************************************************************************/

/* includes */
#include <linux/version.h>
#include <linux/kernel.h>
#include <generated/autoconf.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/pci.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    #include <linux/workqueue.h>
#endif

#include "mvLinuxIalLib.h"
#include "mvIALCommon.h"

#ifdef MY_ABC_HERE
extern void ResubmitMvCommand(unsigned long data);
#endif /* MY_ABC_HERE */

#ifndef scsi_to_pci_dma_dir
    #define scsi_to_pci_dma_dir(scsi_dir) ((int)(scsi_dir))
#endif


/* Connect / disconnect timers. */
/* Note that the disconnect timer should be smaller than the SCSI */
/* subsystem timer. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
struct rescan_wrapper
{
    struct work_struct     work;
    IAL_ADAPTER_T       *pAdapter;
    MV_U8                  channelIndex;
    MV_U16                 targetsToRemove;
    MV_U16                 targetsToAdd;
#ifdef MY_ABC_HERE
    MV_U32                 eh_flag;
#endif
};
#endif


static void *mv_ial_lib_prd_allocate(IAL_HOST_T *pHost);

static int mv_ial_lib_add_buffer_to_prd_table(MV_SATA_ADAPTER   *pMvSataAdapter,
                                              MV_SATA_EDMA_PRD_ENTRY *pPRD_table,
                                              int table_size,
                                              int *count, dma_addr_t buf_addr,
                                              unsigned int buf_len, int isEOT);

void release_ata_mem(struct mv_comp_info * pInfo);

dma_addr_t inline pci64_map_page(struct pci_dev *hwdev, void* address,
                                 size_t size, int direction)

{
    dma_addr_t mm = pci_map_page(hwdev, virt_to_page(address),
                                 ((unsigned long)address & ~PAGE_MASK),
                                 size, direction);
    return mm;
}


void inline pci64_unmap_page(struct pci_dev *hwdev, dma_addr_t address,
                             size_t size, int direction)
{
    pci_unmap_page(hwdev, address, size, direction);
}


int mv_ial_lib_prd_init(IAL_HOST_T *pHost)
{
    MV_U8  i;
    MV_U32  boolSize = MV_SATA_SW_QUEUE_SIZE;

    if ((pHost->pAdapter->mvSataAdapter.sataAdapterGeneration >= MV_SATA_GEN_IIE)&&
        (pHost->pAdapter->mvSataAdapter.pciConfigDeviceId != MV_SATA_DEVICE_ID_6082))
    {
        boolSize = MV_SATA_GEN2E_SW_QUEUE_SIZE;
    }
    /*
     * Allocate PRD Pool  -
     * Since the driver supports 64 SG table, then each PRD table can go upto
     * 1KByte (64 entries * 16byte)
     */
    for (i = 0 ; i < boolSize; i++)
    {
        pHost->prdPool[i] = kmalloc ((MV_EDMA_PRD_ENTRY_SIZE * MV_PRD_TABLE_SIZE * 2)
				     + 16, 
                                     GFP_KERNEL);
        if (pHost->prdPool[i] == NULL)
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Could not allocate PRD pool\n",
                     pHost->pAdapter->mvSataAdapter.adapterId, pHost->channelIndex);
            return -1;
        }
        pHost->prdPoolAligned[i] = (void *)(((ulong)(pHost->prdPool[i]) + 15 ) & ~0xf);
    }

    pHost->freePRDsNum = boolSize;

    return 0;
}

static void *mv_ial_lib_prd_allocate(IAL_HOST_T *pHost)
{
    return pHost->prdPoolAligned[--pHost->freePRDsNum];
}

int mv_ial_lib_prd_free(IAL_HOST_T *pHost, int size, dma_addr_t dmaPtr,
                        void *cpuPtr)
{
    pci64_unmap_page(pHost->pAdapter->pcidev,
                     dmaPtr,
                     size * MV_EDMA_PRD_ENTRY_SIZE,
                     PCI_DMA_TODEVICE);

    pHost->prdPoolAligned[pHost->freePRDsNum++] = cpuPtr;
    return 0;
}


int mv_ial_lib_prd_destroy(IAL_HOST_T *pHost)
{
    MV_U8 temp;
    MV_U32  boolSize = MV_SATA_SW_QUEUE_SIZE;

    if ((pHost->pAdapter->mvSataAdapter.sataAdapterGeneration >= MV_SATA_GEN_IIE)&&
        (pHost->pAdapter->mvSataAdapter.pciConfigDeviceId != MV_SATA_DEVICE_ID_6082))
    {
        boolSize = MV_SATA_GEN2E_SW_QUEUE_SIZE;
    }
    
    for (temp = 0; temp < boolSize; temp++)
    {
        if (pHost->prdPool[temp] != NULL)
        {
            kfree (pHost->prdPool[temp]);
        }
    }
    return 0;
}


/*******************************************************************************
 *  Name:   mv_ial_lib_add_done_queue
 *
 *  Description:    Add scsi_cmnd to done list. Caller must take care of
 *                  adapter_lock locking.
 *
 *  Parameters:     pAdapter - Adapter data structure
 *                  scsi_cmnd - SCSI command data sturcture
 *
 ******************************************************************************/
void mv_ial_lib_add_done_queue (struct IALAdapter *pAdapter,
                                MV_U8 channel,
                                struct scsi_cmnd   *scsi_cmnd)
{
    /* Put new command in the tail of the queue and make it point to NULL */
    ((struct mv_comp_info *)(&scsi_cmnd->SCp))->next_done = NULL;

    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Adding command @ %p to done queue, "
             "channel %d\n",scsi_cmnd, channel);
    if ((channel >= MV_SATA_CHANNELS_NUM) || (pAdapter->host[channel] == NULL))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_FATAL_ERROR, "Adding command @ %p to "
                 " invalid channel (%d)\n",scsi_cmnd, channel);
    }
    if (pAdapter->host[channel]->scsi_cmnd_done_head == NULL)
    {
        /* First command to the queue */
        pAdapter->host[channel]->scsi_cmnd_done_head = scsi_cmnd;
        pAdapter->host[channel]->scsi_cmnd_done_tail = scsi_cmnd;
    }
    else
    {
        /* We already have commands in the queue ; put this command in the tail */
        ((struct mv_comp_info *)(&pAdapter->host[channel]->scsi_cmnd_done_tail->SCp))->next_done = scsi_cmnd;
        pAdapter->host[channel]->scsi_cmnd_done_tail = scsi_cmnd;
    }
}

#ifdef MY_ABC_HERE
/**
 * Just for scsi_eh use. 
 * I don't why this driver do this originally. In order to make
 * it consiste with original code. My EH must do the same thing. 
 * 
 * This would drop current command without notify scsi layer.
 * 
 * @param pAdapter [IN] Should not be NULL.
 * @param channel  [IN] channle index.
 * 
 * @return NULL when no pending scsi command.
 *         others is the complete command head.
 */
struct scsi_cmnd *
syno_ial_lib_clear_cmnd(struct IALAdapter *pAdapter, MV_U8 channel)
{
    if (pAdapter->host[channel] != NULL)
    {        
        struct scsi_cmnd *cmnd = pAdapter->host[channel]->scsi_cmnd_done_head;
        pAdapter->host[channel]->scsi_cmnd_done_head = NULL;
        pAdapter->host[channel]->scsi_cmnd_done_tail = NULL;
        return cmnd;
    }
    return NULL;
}
#endif

/*******************************************************************************
 *  Name:   mv_ial_lib_get_first_cmnd
 *
 *  Description:    Gets first scsi_cmnd from a chain of scsi commands to be
 *                  completed, then sets NULL to head and tail.
 *                  Caller must take care of adapter_lock locking.
 *
 *  Parameters:     pAdapter - Adapter data structure
 *
 *  Return Value:   Pointer to first scsi command in chain
 ******************************************************************************/
struct scsi_cmnd * mv_ial_lib_get_first_cmnd (struct IALAdapter *pAdapter, MV_U8 channel)
{
    if (pAdapter->host[channel] != NULL)
    {
        struct scsi_cmnd *cmnd = pAdapter->host[channel]->scsi_cmnd_done_head;
#ifdef MY_ABC_HERE
        /* Do not clear command list while EH_PROCESSING. Otherwise the scsi command will lose.*/
        if (!(pAdapter->mvSataAdapter.eh[channel].flags & EH_PROCESSING)) {
            pAdapter->host[channel]->scsi_cmnd_done_head = NULL;
            pAdapter->host[channel]->scsi_cmnd_done_tail = NULL;
        }
#else
        pAdapter->host[channel]->scsi_cmnd_done_head = NULL;
        pAdapter->host[channel]->scsi_cmnd_done_tail = NULL;
#endif
        return cmnd;
    }
    return NULL;
}

#ifdef MY_ABC_HERE
/**
 * Check whether we need block the softirq or not.
 * 
 * When EH is processing, we need to blokc softirq.
 * Otherwiese the command would fail directly.
 * 
 * @param cmnd   [IN] scsi command
 * 
 * @return TRUE: we need to block softirq.
 *         FALSE: we don't need to do this
 */
static MV_BOOLEAN
blSynoEHBlockIRQ(struct scsi_cmnd *cmnd)
{
    IAL_ADAPTER_T *pAdapter = NULL;
    IAL_HOST_T *pHost = NULL;
    MV_BOOLEAN ret = MV_FALSE;
    
    if (!cmnd) {
        goto END;
    }
    
    if (!(cmnd->device) ||
        !(cmnd->device->host)) {
        WARN_ON(1);
        goto END;
    }
    
    pAdapter = MV_IAL_ADAPTER(cmnd->device->host);
    pHost = HOSTDATA(cmnd->device->host);
    if (!pAdapter ||
        !pHost) {
        WARN_ON(1);
        goto END;
    }
    
    if (pAdapter->mvSataAdapter.eh[pHost->channelIndex].flags & EH_PROCESSING) {
        /* schedule do scsi_done when eh is done */
        pAdapter->mvSataAdapter.eh[pHost->channelIndex].flags |= EH_SCSI_DONE_NEEDED;
        ret = MV_TRUE;
    }
    
END:
    return ret;
}
#endif

/*******************************************************************************
 *  Name:   mv_ial_lib_do_done
 *
 *  Description:    Calls scsi_done of chain of scsi commands.
 *                  Note that adapter_lock can be locked or unlocked, but
 *                  caller must take care that io_request_lock is locked.
 *
 *  Parameters:     cmnd - First command in scsi commands chain
 *
 ******************************************************************************/
void mv_ial_lib_do_done (struct scsi_cmnd *cmnd)
{
#ifdef MY_ABC_HERE
    if (MV_TRUE == blSynoEHBlockIRQ(cmnd)) {
        return;
    }
#endif
    /* Call done function for all commands in queue */
    while (cmnd)
    {
        struct scsi_cmnd *temp;
        temp = ((struct mv_comp_info *)(&cmnd->SCp))->next_done;

        if (cmnd->scsi_done == NULL)
        {
            return;
        }
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Calling done to command @ %p "
                 "scsi_done = %p\n",cmnd, cmnd->scsi_done);
        cmnd->scsi_done(cmnd);
        cmnd = temp;
    }
}


/*******************************************************************************
 *  Name:   mv_ial_lib_free_channel
 *
 *  Description:    free allocated queues for the given channel
 *
 *  Parameters:     pMvSataAdapter - pointer to the adapter controler this
 *                  channel connected to.
 *          channelNum - channel number.
 *
 ******************************************************************************/
void mv_ial_lib_free_channel(IAL_ADAPTER_T *pAdapter, MV_U8 channelNum)
{
    MV_SATA_CHANNEL *pMvSataChannel;

    if (channelNum >= pAdapter->mvSataAdapter.numberOfChannels)
    {
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "[%d]: Bad channelNum=%d\n",
                 pAdapter->mvSataAdapter.adapterId, channelNum);
        return;
    }

    pMvSataChannel = pAdapter->mvSataAdapter.sataChannel[channelNum];
    kfree(pMvSataChannel);
    pAdapter->mvSataAdapter.sataChannel[channelNum] = NULL;
    return;
}
/****************************************************************
 *  Name:   mv_ial_lib_init_channel
 *
 *  Description:    allocate request and response queues for the EDMA of the
 *                  given channel and sets other fields.
 *
 *  Parameters:
 *      pAdapter - pointer to the emulated adapter data structure
 *      channelNum - channel number.
 *  Return: 0 on success, otherwise on failure
 ****************************************************************/
int mv_ial_lib_init_channel(IAL_ADAPTER_T *pAdapter, MV_U8 channelNum)
{
    MV_SATA_CHANNEL *pMvSataChannel;
    dma_addr_t    req_dma_addr;
    dma_addr_t    rsp_dma_addr;

    if (channelNum >= pAdapter->mvSataAdapter.numberOfChannels)
    {
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "[%d]: Bad channelNum=%d",
                 pAdapter->mvSataAdapter.adapterId, channelNum);
        return -1;
    }

    pMvSataChannel = (MV_SATA_CHANNEL *)kmalloc(sizeof(MV_SATA_CHANNEL),
                                                GFP_ATOMIC);
    if (pMvSataChannel == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: out of memory\n",
                 pAdapter->mvSataAdapter.adapterId);
        return -1;
    }
    pAdapter->mvSataAdapter.sataChannel[channelNum] = pMvSataChannel;
    pMvSataChannel->channelNumber = channelNum;

    pMvSataChannel->requestQueue = (struct mvDmaRequestQueueEntry *)
                                   (pAdapter->requestsArrayBaseAlignedAddr +
                                    (channelNum * pAdapter->requestQueueSize));
    req_dma_addr = pAdapter->requestsArrayBaseDmaAlignedAddr +
                   (channelNum * pAdapter->requestQueueSize);

/* check the 1K alignment of the request queue*/
    if (((u64)req_dma_addr) & 0x3ffULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: request queue allocated isn't 1 K aligned,"
                 " dma_addr=%x.%x channel=%d\n", pAdapter->mvSataAdapter.adapterId,
                 (unsigned int)pci64_dma_hi32(req_dma_addr),
                 (unsigned int)pci64_dma_lo32(req_dma_addr),
                 channelNum);
        return -1;
    }
    pMvSataChannel->requestQueuePciLowAddress = pci64_dma_lo32(req_dma_addr);
    pMvSataChannel->requestQueuePciHiAddress = pci64_dma_hi32(req_dma_addr);
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d,%d]: request queue allocated: 0x%p\n",
             pAdapter->mvSataAdapter.adapterId, channelNum,
             pMvSataChannel->requestQueue);
    pMvSataChannel->responseQueue = (struct mvDmaResponseQueueEntry *)
                                    (pAdapter->responsesArrayBaseAlignedAddr +
                                     (channelNum * pAdapter->responseQueueSize));
    rsp_dma_addr = pAdapter->responsesArrayBaseDmaAlignedAddr +
                   (channelNum * pAdapter->responseQueueSize);

/* check the 256 alignment of the response queue*/
    if (((u64)rsp_dma_addr) & 0xff)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d,%d]: response queue allocated isn't 256 byte "
                 "aligned, dma_addr=%x.%x\n",
                 pAdapter->mvSataAdapter.adapterId, (unsigned int)pci64_dma_hi32(rsp_dma_addr),
                 (unsigned int)pci64_dma_lo32(rsp_dma_addr), channelNum);
        return -1;
    }
    pMvSataChannel->responseQueuePciLowAddress = pci64_dma_lo32(rsp_dma_addr);
    pMvSataChannel->responseQueuePciHiAddress = pci64_dma_hi32(rsp_dma_addr);
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d,%d]: response queue allocated: 0x%p\n",
             pAdapter->mvSataAdapter.adapterId, channelNum,
             pMvSataChannel->responseQueue);
#ifdef MY_ABC_HERE
	setup_timer(&pMvSataChannel->rstimer, ResubmitMvCommand, (unsigned long)pMvSataChannel);
	INIT_LIST_HEAD(&pMvSataChannel->pendinglh);
	pMvSataChannel->chkpower_flags = 0;
#endif /* MY_ABC_HERE */
#ifdef SYNO_SATA_PM_DEVICE_GPIO
    pMvSataChannel->PMSynoUnique = 0;
    pMvSataChannel->PMdeviceId = 0;
    pMvSataChannel->PMvendorId = 0;
#endif
#if defined(SYNO_SATA_PM_DEVICE_GPIO) || defined(MY_ABC_HERE)
    pMvSataChannel->oldDeviceType = MV_SATA_DEVICE_TYPE_UNKNOWN;
#endif
#ifdef MY_ABC_HERE
    pMvSataChannel->ssd_list = 0;
#endif
    return 0;
}

/****************************************************************
 *  Name:   mv_ial_lib_int_handler
 *
 *  Description:    Interrupt handler.
 *
 *  Parameters:     irq - Hardware IRQ number.assume that different cards will have
 *                  different IRQ's TBD
 *                  dev_id  - points to the mvxxxxxxDeviceStruct that generated
 *                    the interrupt
 *                  regs    -
 *
 *
 ****************************************************************/
irqreturn_t mv_ial_lib_int_handler (int irq, void *dev_id )
{
    IAL_ADAPTER_T       *pAdapter;
    unsigned long       flags;
    int                 handled = 0;
    struct scsi_cmnd *cmnds_done_list = NULL;
    pAdapter = (IAL_ADAPTER_T *)dev_id;

/*
 * Acquire the adapter spinlock. Meantime all completed commands will be added
 * to done queue.
 */
    spin_lock_irqsave(&pAdapter->adapter_lock, flags);

    if (mvSataInterruptServiceRoutine(&pAdapter->mvSataAdapter) == MV_TRUE)
    {
        handled = 1;
        pAdapter->procNumOfInterrupts ++;
        mvSataScsiPostIntService(pAdapter->ataScsiAdapterExt);
    }
    /* Unlock adapter lock */
    spin_unlock_irqrestore(&pAdapter->adapter_lock, flags);
    /* Check if there are commands in the done queue to be completed */
    if (handled == 1)
    {
        MV_U8 i;

        for (i = 0; i < pAdapter->maxHosts; i++)
        {
            spin_lock_irqsave(&pAdapter->adapter_lock, flags);
            cmnds_done_list = mv_ial_lib_get_first_cmnd(pAdapter, i);
            spin_unlock_irqrestore(&pAdapter->adapter_lock, flags);
            if (cmnds_done_list)
            {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
                spin_lock_irqsave(&io_request_lock, flags);
#else
                spin_lock_irqsave(pAdapter->host[i]->scsihost->host_lock, flags);
#endif
                mv_ial_lib_do_done(cmnds_done_list);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
                spin_unlock_irqrestore(&io_request_lock, flags);
#else
                spin_unlock_irqrestore(pAdapter->host[i]->scsihost->host_lock, flags);
#endif
            }
        }
    }
    return IRQ_RETVAL(handled);
}

/****************************************************************
 *  Name: mv_ial_lib_add_buffer_to_prd_table
 *
 *  Description:    insert one buffer into number of entries in the PRD table,
 *                  keeping 64KB boundaries
 *
 *  Parameters:     pPRD_table: pointer to the PRD table.
 *          table_size: number of entries in the PRD table.
 *          count: index of the next entry to add, should be updated by this
 *          function
 *          buf_addr,buf_len: the dma address and the size of the buffer to add.
 *          isEOT: 1 if this is the last entry
 *  Returns:        0 on success, otherwise onfailure.
 *
 ****************************************************************/

static int mv_ial_lib_add_buffer_to_prd_table(MV_SATA_ADAPTER   *pMvSataAdapter,
                                              MV_SATA_EDMA_PRD_ENTRY *pPRD_table,
                                              int table_size, int *count,
                                              dma_addr_t buf_addr,
                                              unsigned int buf_len,
                                              int isEOT)
{
    unsigned int    entry = *count;
    u64             xcount = 0;

    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,"insert to PRD table, count=%d, buf_addr=%x, buf_len=%x\n",
             *count,(unsigned int) buf_addr, buf_len);



    /*
    The buffer is splitted in case then either the buffer size exceeds 64 KB
    or 2 high address bits of all data in the buffer are not identical
    */
    while (buf_len)
    {
        if (entry >= table_size)
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,"PRD table too small (entry %d, table_size %d\n",
                     entry, table_size);
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,"[%d] insert to PRD table, count=%d,"
                     " buf_addr=%x, buf_len=%x\n",
                     pMvSataAdapter->adapterId,
                     *count,(unsigned int) buf_addr, buf_len);
            return -1;
        }
        else
        {
            u64 bcount = buf_len;
            /*buffer size exceeds 64K*/
            if (bcount > 0x10000)
                bcount = 0x10000;
            if (buf_addr & 0x1)
            {
                mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, " PRD entry low address is not 1 bit aligned\n");
                return -1;
            }
#if (BITS_PER_LONG > 32) || defined(CONFIG_HIGHMEM64G)
            /*Split the buffer if 2 high address bits of all
            data in the buffer are not the same*/
            if ((buf_addr | 0xFFFFFFFF)  !=
                ((buf_addr + bcount - 1) | 0xFFFFFFFF))
            {
                bcount = 0x100000000ULL - (buf_addr & 0xFFFFFFFF);
            }
#endif
	    if(((buf_addr & MRVL_SATA_BOUNDARY_MASK) + bcount) > 
	       MRVL_SATA_BUFF_BOUNDARY)
	    {
		 bcount = MRVL_SATA_BUFF_BOUNDARY -
		      (buf_addr & MRVL_SATA_BOUNDARY_MASK);
	    }
            /*In case then buffer size is 64K
            PRD entry byte count is set to zero*/
            xcount = bcount & 0xffff;
            pPRD_table[entry].lowBaseAddr =
            cpu_to_le32(pci64_dma_lo32(buf_addr));
            pPRD_table[entry].highBaseAddr =
            cpu_to_le32(pci64_dma_hi32(buf_addr));
            pPRD_table[entry].byteCount = cpu_to_le16(xcount);
            pPRD_table[entry].reserved = 0;
            /* enable snoop on data buffers */
            pPRD_table[entry].flags = 0;/*cpu_to_le16(MV_EDMA_PRD_NO_SNOOP_FLAG);*/
            if (xcount & 0x1)
            {
                mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR," PRD entry byte count is not 1 bit aligned\n");
                return -1;
            }
            buf_addr += bcount;
            buf_len -= bcount;
            entry++;
        }
    }

    if (entry)
    {
        if (isEOT)/* enable snoop on data buffers */
            pPRD_table[entry-1].flags = cpu_to_le16(MV_EDMA_PRD_EOT_FLAG /*|
                                                    MV_EDMA_PRD_NO_SNOOP_FLAG*/);

        *count = entry;
        return 0;
    }

    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "insert zero entries to PRD table \n");
    return -1;
}

/* map to pci */
int mv_ial_lib_generate_prd(MV_SATA_ADAPTER *pMvSataAdapter, struct scsi_cmnd *SCpnt,
                            struct mv_comp_info *completion_info)
{
    IAL_ADAPTER_T   *pAdapter = MV_IAL_ADAPTER(SCpnt->device->host);
    IAL_HOST_T      *pHost = HOSTDATA(SCpnt->device->host);
    MV_SATA_EDMA_PRD_ENTRY *pPRD_table = NULL;
    dma_addr_t PRD_dma_address = 0;
    dma_addr_t busaddr = 0;
    unsigned int prd_size = 0;
    struct scatterlist *sg;
    unsigned int prd_count;
    MV_SATA_DEVICE_TYPE deviceType = pAdapter->ataScsiAdapterExt->ataDriveData[pHost->channelIndex][SCpnt->device->id].identifyInfo.deviceType;
    /*should be removed*/
#ifndef   MV_SUPPORT_1MBYTE_IOS 
#ifdef MY_ABC_HERE
    if (SCpnt->sdb.length > (SCpnt->device->host->max_sectors << 9))
#else
    if (SCpnt->request_bufflen > (SCpnt->device->host->max_sectors << 9))
#endif
    {
        printk("ERROR: request length exceeds the maximum alowed value, %x %x\n",
               pMvSataAdapter->pciConfigDeviceId,
               pMvSataAdapter->pciConfigRevisionId);
    }
#endif
#ifdef MY_ABC_HERE
    if (SCpnt->sdb.table.nents)
#else
    if (SCpnt->use_sg)
#endif
    {
        unsigned int sg_count;
        unsigned int i;

#ifdef MY_ABC_HERE
        sg = (struct scatterlist *) SCpnt->sdb.table.sgl;
#else
        sg = (struct scatterlist *) SCpnt->request_buffer;
#endif

        sg_count = pci64_map_sg(pAdapter->pcidev, sg,
#ifdef MY_ABC_HERE
                                SCpnt->sdb.table.nents,
#else
                                SCpnt->use_sg,
#endif
                                scsi_to_pci_dma_dir(SCpnt->sc_data_direction));

#ifdef MY_ABC_HERE
        if (sg_count != SCpnt->sdb.table.nents)
            printk("WARNING sg_count(%d) != SCpnt->sdb.table.nents(%d)\n",
                   (unsigned int)sg_count, SCpnt->sdb.table.nents);
#else
        if (sg_count != SCpnt->use_sg)
            printk("WARNING sg_count(%d) != SCpnt->use_sg(%d)\n",
                   (unsigned int)sg_count, SCpnt->use_sg);
#endif
        if ((sg_count == 1) && (pAdapter->mvSataAdapter.sataAdapterGeneration >=
                                MV_SATA_GEN_IIE) && (sg_dma_len(sg) <= 0x10000) && 
            (deviceType != MV_SATA_DEVICE_TYPE_ATAPI_DEVICE))
        {
            completion_info->pSALBlock->singleDataRegion = MV_TRUE;
            completion_info->cpu_PRDpnt = NULL;
            completion_info->dma_PRDpnt = 0;
            completion_info->allocated_entries = 0;
            completion_info->single_buff_busaddr = 0;
            PRD_dma_address = sg_dma_address(sg);
            completion_info->pSALBlock->PRDTableLowPhyAddress = pci64_dma_lo32(PRD_dma_address);
            completion_info->pSALBlock->PRDTableHighPhyAddress = pci64_dma_hi32(PRD_dma_address);
            completion_info->pSALBlock->byteCount = sg_dma_len(sg);
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "Use single data region"
                     " buffer, size=%d\n",
                     completion_info->pSALBlock->byteCount);

            return 0;
        }
        completion_info->pSALBlock->singleDataRegion = MV_FALSE;
        prd_size = MV_PRD_TABLE_SIZE;
        pPRD_table = (MV_SATA_EDMA_PRD_ENTRY*)mv_ial_lib_prd_allocate(pHost);
        if (pPRD_table == NULL)
        {
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "Failed to allocate PRD table, requested size=%d\n"
                     , prd_size);
#ifdef MY_ABC_HERE
            pci64_unmap_sg(pAdapter->pcidev, sg, SCpnt->sdb.table.nents,
                           scsi_to_pci_dma_dir(SCpnt->sc_data_direction));
#else
            pci64_unmap_sg(pAdapter->pcidev, sg, SCpnt->use_sg,
                           scsi_to_pci_dma_dir(SCpnt->sc_data_direction));
#endif
            return -1;
        }
        prd_count=0;
        for (i=0; (i < sg_count) && (sg_dma_len(sg)); i++, sg++)
        {
            int isEOT;

            isEOT =((i+1 < sg_count) && (sg_dma_len(&sg[1]))) ? 0 : 1;

            if (mv_ial_lib_add_buffer_to_prd_table(pMvSataAdapter,
                                                   pPRD_table,
                                                   prd_size,
                                                   &prd_count,
                                                   sg_dma_address(sg),
                                                   sg_dma_len(sg),
                                                   isEOT))
            {
                mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR," in building PRD table from scatterlist, "
                         "prd_size=%d, prd_count=%d\n", prd_size,
                         prd_count);
                pci64_unmap_sg(pAdapter->pcidev,
#ifdef MY_ABC_HERE
                               (struct scatterlist *)SCpnt->sdb.table.sgl,
                               SCpnt->sdb.table.nents,
#else
                               (struct scatterlist *)SCpnt->request_buffer,
                               SCpnt->use_sg,
#endif
                               scsi_to_pci_dma_dir(SCpnt->sc_data_direction));

                return -1;
            }
        }
        PRD_dma_address = pci64_map_page(pAdapter->pcidev,
                                         pPRD_table,
                                         MV_EDMA_PRD_ENTRY_SIZE * (prd_size),
                                         PCI_DMA_TODEVICE);
    }
#ifdef MY_ABC_HERE
    else if (SCpnt->sdb.length && SCpnt->sc_data_direction != PCI_DMA_NONE)
#else
    else if (SCpnt->request_bufflen && SCpnt->sc_data_direction != PCI_DMA_NONE)
#endif
    {
        if ((pAdapter->mvSataAdapter.sataAdapterGeneration >= MV_SATA_GEN_IIE)
#ifdef MY_ABC_HERE
            && (SCpnt->sdb.length <= 0x10000) &&
#else
            && (SCpnt->request_bufflen <= 0x10000) &&
#endif
            (deviceType != MV_SATA_DEVICE_TYPE_ATAPI_DEVICE))
        {
            completion_info->pSALBlock->singleDataRegion = MV_TRUE;
            completion_info->cpu_PRDpnt = NULL;
            completion_info->dma_PRDpnt = 0;
            completion_info->allocated_entries = 0;
#ifdef MY_ABC_HERE
            busaddr = pci64_map_page(pAdapter->pcidev, SCpnt->sdb.table.sgl,
                                     SCpnt->sdb.length,
                                     scsi_to_pci_dma_dir(SCpnt->sc_data_direction));
#else
            busaddr = pci64_map_page(pAdapter->pcidev, SCpnt->request_buffer,
                                     SCpnt->request_bufflen,
                                     scsi_to_pci_dma_dir(SCpnt->sc_data_direction));
#endif
            completion_info->single_buff_busaddr = busaddr;
            completion_info->pSALBlock->PRDTableLowPhyAddress = pci64_dma_lo32(busaddr);
            completion_info->pSALBlock->PRDTableHighPhyAddress = pci64_dma_hi32(busaddr);
#ifdef MY_ABC_HERE
            completion_info->pSALBlock->byteCount = SCpnt->sdb.length;
#else
            completion_info->pSALBlock->byteCount = SCpnt->request_bufflen;
#endif
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "Use single data region"
                     " buffer, size=%d\n",
                     completion_info->pSALBlock->byteCount);
            return 0;
        }
        completion_info->pSALBlock->singleDataRegion = MV_FALSE;
        prd_size = MV_PRD_TABLE_SIZE;
        pPRD_table = (MV_SATA_EDMA_PRD_ENTRY*)mv_ial_lib_prd_allocate(pHost);
        if (pPRD_table == NULL)
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "Failed to allocate PRD table, requested size=%d\n",
                     prd_size);
            return -1;
        }

#ifdef MY_ABC_HERE
        busaddr = pci64_map_page(pAdapter->pcidev, SCpnt->sdb.table.sgl,
                                 SCpnt->sdb.length,
                                 scsi_to_pci_dma_dir(SCpnt->sc_data_direction));
#else
        busaddr = pci64_map_page(pAdapter->pcidev, SCpnt->request_buffer,
                                 SCpnt->request_bufflen,
                                 scsi_to_pci_dma_dir(SCpnt->sc_data_direction));
#endif
        prd_count = 0;
        if (mv_ial_lib_add_buffer_to_prd_table(pMvSataAdapter,
                                               pPRD_table,
                                               prd_size,
                                               &prd_count, busaddr,
#ifdef MY_ABC_HERE
                                               SCpnt->sdb.length, 1))
#else
                                               SCpnt->request_bufflen, 1))
#endif
        {

            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, " in building PRD table from buffer\n");
#ifdef MY_ABC_HERE
            pci64_unmap_page(pAdapter->pcidev, busaddr, SCpnt->sdb.length,
#else
            pci64_unmap_page(pAdapter->pcidev, busaddr, SCpnt->request_bufflen,
#endif
                             scsi_to_pci_dma_dir(SCpnt->sc_data_direction));


            return -1;
        }
        PRD_dma_address = pci64_map_page(pAdapter->pcidev,
                                         pPRD_table,
                                         MV_EDMA_PRD_ENTRY_SIZE* (prd_size),
                                         PCI_DMA_TODEVICE);
    }
    completion_info->cpu_PRDpnt = pPRD_table;
    completion_info->dma_PRDpnt = PRD_dma_address;
    completion_info->allocated_entries = prd_size;
    completion_info->single_buff_busaddr = busaddr;
    completion_info->pSALBlock->PRDTableLowPhyAddress = pci64_dma_lo32(PRD_dma_address);
    completion_info->pSALBlock->PRDTableHighPhyAddress = pci64_dma_hi32(PRD_dma_address);
    mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "PRD table allocated %p\n", pPRD_table);
    mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "PRD table allocated (dma) %x \n", PRD_dma_address);
    return 0;
}


/****************************************************************
 *  Name: mv_ial_block_requests
 *
 *  Description:    Blocks request from SCSI mid layer while channel
 *                  initialization is in progress
 *
 *  Parameters:     pAdapter, pointer to the IAL adapter data structure.
 *                  channelIndex, channel number
 *
 *  Returns:        None.
 *
 ****************************************************************/

void mv_ial_block_requests(struct IALAdapter *pAdapter, MV_U8 channelIndex)
{
    if (MV_TRUE == pAdapter->host[channelIndex]->hostBlocked)
    {
        return;
    }

    if ((pAdapter->ialCommonExt.channelState[channelIndex] != CHANNEL_READY) &&
        (pAdapter->ialCommonExt.channelState[channelIndex] != CHANNEL_NOT_CONNECTED))
    {
        pAdapter->host[channelIndex]->hostBlocked = MV_TRUE;
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: blocking SCSI host.\n",
                 pAdapter->mvSataAdapter.adapterId, channelIndex);
        scsi_block_requests(pAdapter->host[channelIndex]->scsihost);
    }
    else
    {
        pAdapter->host[channelIndex]->hostBlocked = MV_FALSE;
    }
}

/****************************************************************
 *  Name: mv_ial_unblock_requests
 *
 *  Description:    Unblocks request from SCSI mid layer for non connected
 *                  channels or channels whose initialization is finished
 *
 *  Parameters:     pAdapter -  pointer to the IAL adapter data structure.
 *                  channelIndex -  channel number
 *
 *  Returns:        None.
 *
 ****************************************************************/
static void mv_ial_unblock_requests(struct IALAdapter *pAdapter, MV_U8 channelIndex)
{
    if ((CHANNEL_NOT_CONNECTED == pAdapter->ialCommonExt.channelState[channelIndex]) ||
        (CHANNEL_READY == pAdapter->ialCommonExt.channelState[channelIndex]))
    {
        pAdapter->host[channelIndex]->hostBlocked = MV_FALSE;
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: unblocking SCSI host.\n",
                 pAdapter->mvSataAdapter.adapterId, channelIndex);
        scsi_unblock_requests(pAdapter->host[channelIndex]->scsihost);
    }
}



#ifdef MY_ABC_HERE
static inline void
SynoEHEventCommon(IAL_ADAPTER_T *pAdapter,
                  MV_SATA_ADAPTER *pMvSataAdapter,
                  MV_U8 channel)
{    
    mv_ial_block_requests(pAdapter, channel);
    queue_delayed_work(mvSata_aux_wq, &(pMvSataAdapter->eh[channel].work), 0);
    pMvSataAdapter->eh[channel].flags |= EH_PROCESSING;
}

static MV_BOOLEAN
SynoEHConnectHandle(IAL_ADAPTER_T *pAdapter,
                    MV_SATA_ADAPTER *pMvSataAdapter,
                    MV_U8 channel)
{
    MV_BOOLEAN ret = MV_FALSE;
    MV_BOOLEAN blRestartImmediately = MV_FALSE;

    if (!pAdapter ||
        !pMvSataAdapter) {
        WARN_ON(1);
        goto END;
    }

    if (NULL != pMvSataAdapter->sataChannel[channel]) {
        /* Maybe we are in eh */
        if (pMvSataAdapter->eh[channel].flags & EH_PROCESSING) {
            /* Tell eh we need a sweet retry */
            syno_eh_printk(pMvSataAdapter, channel, 
                           "EH process and channel connect again");
            pMvSataAdapter->eh[channel].flags |= EH_CONNECT_AGAIN;
            blRestartImmediately = MV_TRUE;
        } else {
            /* Maybe the channel is just stop and start fastly */
            syno_eh_printk(pMvSataAdapter, channel, 
                           "Channel is here but no EH. Restart channel immediatly");
            SynoEHEventCommon(pAdapter, pMvSataAdapter, channel);
        }
    } else {
        /* A normal plugin*/
        blRestartImmediately = MV_TRUE;
        pMvSataAdapter->eh[channel].flags |= EH_CONNECT_TRIGGER;
    }

    ret = MV_TRUE;
END:
    if (MV_TRUE == blRestartImmediately) {
        syno_eh_printk(pMvSataAdapter, channel, 
                       "Restart channel immediatly");
        mvRestartChannel(&pAdapter->ialCommonExt, channel,
                         pAdapter->ataScsiAdapterExt, MV_FALSE);
        mv_ial_block_requests(pAdapter, channel);
    }

    return ret;
}

static MV_BOOLEAN
SynoEHDisConnectHandle(IAL_ADAPTER_T *pAdapter,
                       MV_SATA_ADAPTER *pMvSataAdapter,
                       MV_U8 channel)
{
    MV_BOOLEAN ret = MV_FALSE;

    if (!pAdapter ||
        !pMvSataAdapter) {
        WARN_ON(1);
        goto END;
    }

    if (NULL != pMvSataAdapter->sataChannel[channel]) {
        if (!(pMvSataAdapter->eh[channel].flags & EH_PROCESSING)) {
            syno_eh_printk(pMvSataAdapter, channel, "Schedule eh");
            if (MV_SATA_DEVICE_TYPE_PM == pMvSataAdapter->sataChannel[channel]->deviceType) {
                pMvSataAdapter->eh[channel].flags |= EH_LINK_PMP;
            } else {
                pMvSataAdapter->eh[channel].flags |= EH_LINK_DISK;
            }
            pMvSataAdapter->eh[channel].flags |= EH_DISCONNECT_TRIGGER;
        } else {
            syno_eh_printk(pMvSataAdapter, channel, 
                           "EH is on but disconnect again, schedule EH");
        }
        SynoEHEventCommon(pAdapter, pMvSataAdapter, channel);
    } else {
        syno_eh_printk(pMvSataAdapter, channel, "Stop channel");
        SynomvStopChannel(&pAdapter->ialCommonExt, channel,
                          pAdapter->ataScsiAdapterExt);
    }

    ret = MV_TRUE;
END:
    return ret;
}

static MV_BOOLEAN
SynoEHUnCommunicationErrorHandle(IAL_ADAPTER_T *pAdapter,
                                 MV_SATA_ADAPTER *pMvSataAdapter,
                                 MV_U8 channel)
{
    MV_BOOLEAN ret = MV_FALSE;
    MV_BOOLEAN blRestartImmediately = MV_FALSE;

    if (!pAdapter ||
        !pMvSataAdapter) {
        WARN_ON(1);
        goto END;
    }

    if (NULL != pMvSataAdapter->sataChannel[channel]) {
        /* reset channel let chip can know channel status exactly*/
        mvSataChannelHardReset(pMvSataAdapter, channel);
        if (MV_FALSE == mvSataIsStorageDeviceConnected(pMvSataAdapter, channel, NULL)) {
            /* Maybe disconnect */
            syno_eh_printk(pMvSataAdapter, channel, 
                           "Looks like an error IRQ from disconnect to communication error");
            return SynoEHDisConnectHandle(pAdapter, pMvSataAdapter, channel);
        } else {
            /* Usually a cables error or chip uncompatible */

            if (pMvSataAdapter->eh[channel].flags & EH_HW_COM_ERROR &&
                MV_SATA_DEVICE_TYPE_PM != pMvSataAdapter->sataChannel[channel]->deviceType) {
                syno_eh_printk(pMvSataAdapter, channel, 
                               "EH_HW_COM_ERROR is on, set 1.5GBps communication");
                mvSataSetInterfaceSpeed(pMvSataAdapter, channel, MV_SATA_IF_SPEED_1_5_GBPS);
                mvSataChannelHardReset(pMvSataAdapter, channel);
            }

            if (!(pMvSataAdapter->eh[channel].flags & EH_PROCESSING)) {
                syno_eh_printk(pMvSataAdapter, channel, "Schedule EH");
                pMvSataAdapter->eh[channel].flags |= EH_UNRECOVER_TRIGGER;
                SynoEHEventCommon(pAdapter, pMvSataAdapter, channel);
            } else {
                blRestartImmediately = MV_TRUE;
                syno_eh_printk(pMvSataAdapter, channel, 
                               "Device is here and EH processing, Restart immediately");
            }            
        }
    } else {
        /* Perhaps a IRQ erro or cannot deactive EDMA ...*/
        syno_eh_printk(pMvSataAdapter, channel, "Restart immediately");
        blRestartImmediately = MV_TRUE;
    }

    /* Clear EH_HW_COM_ERROR */
    pMvSataAdapter->eh[channel].flags &= ~(EH_HW_COM_ERROR);
    ret = MV_TRUE;
END:
    if (MV_TRUE == blRestartImmediately) {
        mvSataChannelHardReset(pMvSataAdapter, channel);
        mvRestartChannel(&pAdapter->ialCommonExt, channel,
                         pAdapter->ataScsiAdapterExt, MV_TRUE);
        mv_ial_block_requests(pAdapter, channel);
    }
    return ret;
}
#endif

/****************************************************************
 *  Name: mv_ial_lib_event_notify
 *
 *  Description:    this function called by the low  level to notify a certain event
 *
 *  Parameters:     pMvSataAdapter, pointer to the Device data structure.
 *
 *  Returns:        MV_TRUE on success, MV_FALSE on failure.
 *
 ****************************************************************/
MV_BOOLEAN mv_ial_lib_event_notify(MV_SATA_ADAPTER *pMvSataAdapter, MV_EVENT_TYPE eventType,
                                   MV_U32 param1, MV_U32 param2)
{
    IAL_ADAPTER_T   *pAdapter = pMvSataAdapter->IALData;
    MV_U8   channel = param2;

    switch (eventType)
    {
    case MV_EVENT_TYPE_SATA_CABLE:
        {


            if (param1 == MV_SATA_CABLE_EVENT_CONNECT)
            {

                mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: device connected event received\n",
                         pMvSataAdapter->adapterId, channel);
#ifdef MY_ABC_HERE
                syno_eh_printk(pMvSataAdapter, channel, "Sata Cable Event: Connect");
#endif
#ifdef MY_ABC_HERE
                if (MV_FALSE == SynoEHConnectHandle(pAdapter, pMvSataAdapter, channel)) {
                    syno_eh_printk(pMvSataAdapter, channel, "SynoEHConnectHandle Error");
                }
#else
                mvRestartChannel(&pAdapter->ialCommonExt, channel,
                                 pAdapter->ataScsiAdapterExt, MV_FALSE);
                mv_ial_block_requests(pAdapter, channel);
#endif
            }
            else if (param1 == MV_SATA_CABLE_EVENT_DISCONNECT)
            {
                mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: device disconnected event received \n",
                         pMvSataAdapter->adapterId, channel);
#ifdef MY_ABC_HERE
                syno_eh_printk(pMvSataAdapter, channel, "Sata Cable Event: Disconnect");
#endif
                if (mvSataIsStorageDeviceConnected(pMvSataAdapter, channel, NULL) ==
                    MV_FALSE)
                {
#ifdef MY_ABC_HERE
                    if (MV_FALSE == SynoEHDisConnectHandle(pAdapter, pMvSataAdapter, channel)) {
                        syno_eh_printk(pMvSataAdapter, channel, 
                                       "SynoEHDisConnectHandle Error");
                    }
#else /* MY_ABC_HERE */
#ifdef MY_ABC_HERE
                    SynomvStopChannel(&pAdapter->ialCommonExt, channel,
                                      pAdapter->ataScsiAdapterExt);
#else
                    mvStopChannel(&pAdapter->ialCommonExt, channel,
                                  pAdapter->ataScsiAdapterExt);
#endif
#endif /* MY_ABC_HERE */
                }
                else
                {
                    mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: device disconnected event ignored.\n",
                             pMvSataAdapter->adapterId, channel);
                }

            }
            else if (param1 == MV_SATA_CABLE_EVENT_PM_HOT_PLUG)
            {
#ifdef MY_ABC_HERE
                syno_eh_printk(pMvSataAdapter, channel, "Sata Cable PM Event: HOT_PLUG");
#endif
                mvPMHotPlugDetected(&pAdapter->ialCommonExt, channel,
                                    pAdapter->ataScsiAdapterExt);
                mv_ial_block_requests(pAdapter, channel);
            }
            else
            {

                mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "illegal value for param1(%d) at "
                         "connect/disconect event, host=%d\n", param1,
                         pMvSataAdapter->adapterId );
            }
        }
        break;
    case MV_EVENT_TYPE_ADAPTER_ERROR:
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR, "DEVICE error event received, pci cause "
                 "reg=%x, don't know how to handle this\n", param1);
        return MV_TRUE;
    case MV_EVENT_TYPE_SATA_ERROR:
        switch (param1)
        {
        case MV_SATA_RECOVERABLE_COMMUNICATION_ERROR:
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,
                     " [%d %d] sata recoverable error occured\n",
                     pMvSataAdapter->adapterId, channel);
            break;
        case MV_SATA_UNRECOVERABLE_COMMUNICATION_ERROR:
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,
                     " [%d %d] sata unrecoverable error occured, restart channel\n",
                     pMvSataAdapter->adapterId, channel);
#ifdef MY_ABC_HERE
            if (pMvSataAdapter->sataChannel[channel] &&
                MV_SATA_DEVICE_TYPE_PM == pMvSataAdapter->sataChannel[channel]->deviceType)
            {
                syno_eh_printk(pMvSataAdapter, channel, "This port is port multiplier");
            }
            syno_eh_printk(pMvSataAdapter, channel, "Sata Error Event: UNRECOVERABLE_COMMUNICATION_ERROR");
#endif
#ifdef MY_ABC_HERE            
            if (MV_FALSE == SynoEHUnCommunicationErrorHandle(pAdapter, pMvSataAdapter, channel)) {
                syno_eh_printk(pMvSataAdapter, channel, "SynoEHUnCommunicationErrorHandle Error");
            }
#else
            mvSataChannelHardReset(pMvSataAdapter, channel);
            mvRestartChannel(&pAdapter->ialCommonExt, channel,
                             pAdapter->ataScsiAdapterExt, MV_TRUE);
            mv_ial_block_requests(pAdapter, channel);
#endif
            break;
        case MV_SATA_DEVICE_ERROR:
            mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,
                     " [%d %d] device error occured\n",
                     pMvSataAdapter->adapterId, channel);
            break;
        }
        break;
    default:
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,  " adapter %d unknown event %d"
                 " param1= %x param2 = %x\n", pMvSataAdapter->adapterId,
                 eventType - MV_EVENT_TYPE_ADAPTER_ERROR, param1, param2);
        return MV_FALSE;

    }/*switch*/
    return MV_TRUE;
}

MV_BOOLEAN IALConfigQueuingMode(MV_SATA_ADAPTER *pSataAdapter,
                                MV_U8 channelIndex,
                                MV_EDMA_MODE mode,
                                MV_SATA_SWITCHING_MODE switchingMode,
                                MV_BOOLEAN  use128Entries)

{
    IAL_ADAPTER_T   *pAdapter = pSataAdapter->IALData;
    pAdapter->host[channelIndex]->mode = mode;
    pAdapter->host[channelIndex]->switchingMode = switchingMode;
    pAdapter->host[channelIndex]->use128Entries = use128Entries;

    if (mvSataConfigEdmaMode(pSataAdapter, channelIndex,
                             mode, 31) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: mvSataConfigEdmaMode failed\n",
                 pSataAdapter->adapterId, channelIndex);
        return -1;
    }
    return MV_TRUE;
}

MV_BOOLEAN IALInitChannel(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex)
{
    if (mv_ial_lib_init_channel(pSataAdapter->IALData, channelIndex) == 0)
    {
        return MV_TRUE;
    }
    return MV_FALSE;
}
void IALReleaseChannel(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex)
{
    mv_ial_lib_free_channel(pSataAdapter->IALData, channelIndex);
}

void IALChannelCommandsQueueFlushed(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex)
{

}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#ifdef MY_ABC_HERE
static MV_BOOLEAN
SynoRescheduleChannelRescan(struct rescan_wrapper* rescan,
                            MV_U16 newAdd,
                            MV_U16 target)
{
    MV_BOOLEAN ret = MV_FALSE;

    /* remove specify device in this time */
    rescan->targetsToRemove = 0;
    rescan->targetsToRemove |= (1 << target);
    rescan->targetsToAdd = newAdd;
    if (0 == schedule_work(&rescan->work)) {
        goto END;
    }

    ret = MV_TRUE;
END:
    return ret;
}
#endif
static void channel_rescan(struct work_struct *work)
{
    struct rescan_wrapper* rescan = container_of(work, struct rescan_wrapper, work);
    struct Scsi_Host *host;
    struct scsi_device *sdev = NULL;
    MV_U16 target;
#ifdef MY_ABC_HERE
    MV_U16 newAdd = rescan->targetsToAdd;
    MV_BOOLEAN blReschedule = MV_FALSE;
#endif
    if (rescan->pAdapter->host[rescan->channelIndex] == NULL)
    {
        kfree(rescan);
        return;
    }
    host = rescan->pAdapter->host[rescan->channelIndex]->scsihost;
    down(&rescan->pAdapter->rescan_mutex);
    if (atomic_read(&rescan->pAdapter->stopped) > 0)
    {
        up(&rescan->pAdapter->rescan_mutex);
        kfree(rescan);
        return;
    }
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d %d] channel_rescan(): "
             "targets to add 0x%X, targets to remove 0x%X\n",
             rescan->pAdapter->mvSataAdapter.adapterId,
             rescan->channelIndex,
             rescan->targetsToRemove,
             rescan->targetsToAdd);

    for (target = 0; (rescan->targetsToRemove != 0) && (target < host->max_id); target++)
    {
        if (rescan->targetsToRemove & (1 << target))
        {
            sdev = scsi_device_lookup(host, 0, target, 0);
            if (sdev != NULL)
            {
		    /*scsi_device_cancel(sdev, 0);*/
                scsi_remove_device(sdev);
                scsi_device_put(sdev);
            }
	    else
	    {
		    mvLogMsg(MV_IAL_LOG_ID,
                         MV_DEBUG_ERROR,
                         "[%d %d %d] failed to deattach scsi device\n",
                         rescan->pAdapter->mvSataAdapter.adapterId,
                         rescan->channelIndex,
                         target);
	    }
        }
    }
    sdev = NULL;
    for (target = 0; (rescan->targetsToAdd != 0) && (target < host->max_id); target++)
    {
        if (rescan->targetsToAdd & (1 << target))
        {
            int error = scsi_add_device(host, 0, target, 0);
            if (error)
            {
                mvLogMsg(MV_IAL_LOG_ID,
                         MV_DEBUG_ERROR,
                         "[%d %d %d] Error adding scsi device\n",
                         rescan->pAdapter->mvSataAdapter.adapterId,
                         rescan->channelIndex,
                         target);
            }
#ifdef MY_ABC_HERE
            if (!error) {
                if (NULL == (sdev = scsi_device_lookup(host, 0, target, 0))) {
                    /* Perhaps get wrong name */
                    syno_eh_printk((&(rescan->pAdapter->mvSataAdapter)),
                                   rescan->channelIndex,
                                   "Perhaps the device name was wrong, reschedule channel_rescan");
                    if (MV_TRUE == SynoRescheduleChannelRescan(rescan, newAdd, target)) {
                        blReschedule = MV_TRUE;
                        goto END;
                    }
                } else {
                    scsi_device_put(sdev);
                    newAdd &= ~(1 << target);
                }
            }
#endif /* MY_ABC_HERE */
#ifdef MY_ABC_HERE
            if (!error) {
                MV_SATA_CHANNEL *pSataChannel = rescan->pAdapter->mvSataAdapter.sataChannel[rescan->channelIndex];

                if (!pSataChannel) {
                    continue;
                }

                if (NULL == (sdev = scsi_device_lookup(host, 0, target, 0))) {
                    continue;
                }

                if (test_bit(target, &(pSataChannel->ssd_list))) {
                    printk("Find SSD disks channelIndex %d target %d [%.16s]\n", rescan->channelIndex, target, sdev->model);
                }

                scsi_device_put(sdev);
            }
#endif
        }
    }
#ifdef MY_ABC_HERE
    if (EH_LINK_PMP & rescan->eh_flag ||
        MV_TRUE == syno_mvSata_is_synology_pm(&rescan->pAdapter->ialCommonExt, rescan->channelIndex)) {        
        char *envp[2];

        if (EH_CONNECT_TRIGGER & rescan->eh_flag)
            envp[0] = SZK_PMP_UEVENT"="SZV_PMP_CONNECT;
        else if (EH_DISCONNECT_TRIGGER & rescan->eh_flag)
            envp[0] = SZK_PMP_UEVENT"="SZV_PMP_DISCONNECT;
        else
            envp[0] = NULL;

        envp[1] = NULL;
        kobject_uevent_env(&host->shost_dev.kobj, KOBJ_CHANGE, envp);
    }
#endif
#ifdef MY_ABC_HERE
END:    
    if (MV_FALSE == blReschedule) {
        kfree(rescan);
    }
    up(&rescan->pAdapter->rescan_mutex);
#else
    up(&rescan->pAdapter->rescan_mutex);
    kfree(rescan);
#endif
}
#endif

MV_BOOLEAN IALBusChangeNotify(MV_SATA_ADAPTER *pSataAdapter,
                              MV_U8 channelIndex)
{
    return MV_TRUE;
}

#ifdef MY_ABC_HERE
MV_BOOLEAN IALBusChangeNotifyEx(MV_SATA_ADAPTER *pSataAdapter,
                                MV_U8 channelIndex,
                                MV_U16 targetsToRemove,
                                MV_U16 targetsToAdd,
                                MV_U32 eh_flag)
#else
MV_BOOLEAN IALBusChangeNotifyEx(MV_SATA_ADAPTER *pSataAdapter,
                                MV_U8 channelIndex,
                                MV_U16 targetsToRemove,
                                MV_U16 targetsToAdd)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    IAL_ADAPTER_T   *pAdapter = pSataAdapter->IALData;
    if (0 == atomic_read(&pAdapter->stopped))
    {
        struct rescan_wrapper* rescan =
        kmalloc(sizeof(struct rescan_wrapper), GFP_ATOMIC);
        if (rescan != NULL)
        {
            INIT_WORK(&rescan->work, channel_rescan);
            rescan->pAdapter = pAdapter;
            rescan->channelIndex = channelIndex;
            rescan->targetsToRemove = targetsToRemove;
            rescan->targetsToAdd = targetsToAdd;
#ifdef MY_ABC_HERE
            rescan->eh_flag = eh_flag;
#endif
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,
                     "[%d %d] Rescan bus: remove 0x%X, add 0x%X.\n",
                     pSataAdapter->adapterId,
                     channelIndex, targetsToRemove, targetsToAdd);
            if (schedule_work(&rescan->work) == 0)
            {
                mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,
                         "[%d %d] Rescan bus: schedule_work() failed.\n",
                         pSataAdapter->adapterId,
                         channelIndex);
                kfree(rescan);
            }
        }
        else
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,
                     "[%d %d] Rescan bus: memory allocation error.\n",
                     pSataAdapter->adapterId,
                     channelIndex);
        }
    }
#endif
    return MV_TRUE;
}


void asyncStartTimerFunction(unsigned long data)
{
    IAL_ADAPTER_T   *pAdapter = (IAL_ADAPTER_T *)data;
    unsigned long       flags;
    struct scsi_cmnd *cmnds_done_list = NULL;
    MV_U8 i;

    spin_lock_irqsave(&pAdapter->adapter_lock, flags);
    if (pAdapter->stopAsyncTimer == MV_FALSE)
    {
        mvIALTimerCallback(&pAdapter->ialCommonExt,
                           pAdapter->ataScsiAdapterExt);
        for (i = 0; i < pAdapter->maxHosts; i++)
        {
            if (MV_TRUE == pAdapter->host[i]->hostBlocked)
            {
                spin_unlock_irqrestore(&pAdapter->adapter_lock, flags);
                mv_ial_unblock_requests(pAdapter, i);
                spin_lock_irqsave(&pAdapter->adapter_lock, flags);
            }
        }
        pAdapter->asyncStartTimer.expires = jiffies + MV_LINUX_ASYNC_TIMER_PERIOD;
        add_timer (&pAdapter->asyncStartTimer);
    }
    else
    {
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG,   "[%d]: Async timer stopped\n",
                 pAdapter->mvSataAdapter.adapterId);
    }
    spin_unlock_irqrestore(&pAdapter->adapter_lock, flags);
    /* Check if there are commands in the done queue to be completed */
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        spin_lock_irqsave(&pAdapter->adapter_lock, flags);
        cmnds_done_list = mv_ial_lib_get_first_cmnd(pAdapter, i);
        spin_unlock_irqrestore(&pAdapter->adapter_lock, flags);
        if (cmnds_done_list)
        {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
            spin_lock_irqsave(&io_request_lock, flags);
#else
            spin_lock_irqsave(pAdapter->host[i]->scsihost->host_lock, flags);
#endif
            mv_ial_lib_do_done(cmnds_done_list);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
            spin_unlock_irqrestore(&io_request_lock, flags);
#else
            spin_unlock_irqrestore(pAdapter->host[i]->scsihost->host_lock, flags);
#endif
        }
    }
}

/****************************************************************
 *  Name: release_ata_mem
 *
 *  Description:   free memory allocated to the PRD table
 *          unmap the data buffers of the scsi command
 *          free completion_info data structure.
 *  Parameters:     pInfo: pointer to the data structure returned by the
 *          completion call back function to identify the origial command
 *
 ****************************************************************/
void release_ata_mem(struct mv_comp_info * pInfo)
{
    IAL_ADAPTER_T   *pAdapter = MV_IAL_ADAPTER(pInfo->SCpnt->device->host);
    IAL_HOST_T      *pHost = HOSTDATA(pInfo->SCpnt->device->host);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if(pInfo->kmap_buffer)
    {
	if( pInfo->SCpnt->sc_data_direction == DMA_FROM_DEVICE)
	{
		struct scatterlist *sg;
		MV_U8*		pBuffer;
#ifdef MY_ABC_HERE
			sg = (struct scatterlist *) pInfo->SCpnt->sdb.table.sgl;
#else
			sg = (struct scatterlist *) pInfo->SCpnt->request_buffer;
#endif

		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "SCpnt %p: copy data from temp"
			 " buffer to command buffer, length %d \n", pInfo->SCpnt, sg->length);

#ifdef MY_ABC_HERE
		pBuffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
#else
		pBuffer = kmap_atomic(sg->page, KM_IRQ0) + sg->offset;
#endif
		memcpy(pBuffer, pInfo->pSALBlock->pDataBuffer , sg->length); 	
	        kunmap_atomic(pBuffer - sg->offset, KM_IRQ0);
	}
	mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "SCpnt %p: free temp data "
		 "buffer\n", pInfo->SCpnt);
	kfree(pInfo->pSALBlock->pDataBuffer);
    }
#endif
    if (pInfo->cpu_PRDpnt)
    {
        mv_ial_lib_prd_free(pHost,
                            pInfo->allocated_entries,
                            pInfo->dma_PRDpnt,
                            pInfo->cpu_PRDpnt);
#ifdef MY_ABC_HERE
        if (pInfo->SCpnt->sdb.table.nents)
#else
        if (pInfo->SCpnt->use_sg)
#endif
        {
#ifdef MY_ABC_HERE
            pci64_unmap_sg(pAdapter->pcidev,
                           (struct scatterlist *)pInfo->SCpnt->sdb.table.sgl,
                           pInfo->SCpnt->sdb.table.nents,
                           scsi_to_pci_dma_dir(pInfo->SCpnt->sc_data_direction));
#else
            pci64_unmap_sg(pAdapter->pcidev,
                           (struct scatterlist *)pInfo->SCpnt->request_buffer,
                           pInfo->SCpnt->use_sg,
                           scsi_to_pci_dma_dir(pInfo->SCpnt->sc_data_direction));
#endif
        }
        else
        {
            pci64_unmap_page(pAdapter->pcidev,
                             pInfo->single_buff_busaddr,
#ifdef MY_ABC_HERE
                             pInfo->SCpnt->sdb.length,
#else
                             pInfo->SCpnt->request_bufflen,
#endif
                             scsi_to_pci_dma_dir(pInfo->SCpnt->sc_data_direction));
        }
    }
    pInfo->cpu_PRDpnt = NULL;
    kfree(pInfo->pSALBlock);
}

int mv_ial_lib_allocate_edma_queues(IAL_ADAPTER_T *pAdapter)
{
    ulong *tmp;
    ulong   requests_array_size;
    ulong   responses_array_size;

    if ((pAdapter->mvSataAdapter.sataAdapterGeneration >= MV_SATA_GEN_IIE) && 
		(pAdapter->mvSataAdapter.pciConfigDeviceId != MV_SATA_DEVICE_ID_6082))
    {
        pAdapter->requestQueueSize = MV_EDMA_GEN2E_REQUEST_QUEUE_SIZE;
        pAdapter->responseQueueSize = MV_EDMA_GEN2E_RESPONSE_QUEUE_SIZE;
    }
    else
    {
        pAdapter->requestQueueSize = MV_EDMA_REQUEST_QUEUE_SIZE;
        pAdapter->responseQueueSize = MV_EDMA_RESPONSE_QUEUE_SIZE;
    }
    
    requests_array_size = (pAdapter->maxHosts + 1) * (pAdapter->requestQueueSize);
    responses_array_size =(pAdapter->maxHosts + 1) * (pAdapter->responseQueueSize);

    pAdapter->requestsArrayBaseAddr =
    pci_alloc_consistent(pAdapter->pcidev,
                         requests_array_size,
                         &(pAdapter->requestsArrayBaseDmaAddr));
    if (pAdapter->requestsArrayBaseAddr == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: Failed to allocate memory for EDMA request"
                 " queues\n", pAdapter->mvSataAdapter.adapterId);
        return -1;
    }
    pAdapter->requestsArrayBaseAlignedAddr = pAdapter->requestsArrayBaseAddr +
                                             pAdapter->requestQueueSize;
    tmp = (ulong*)&pAdapter->requestsArrayBaseAlignedAddr;
    *tmp &= ~((ulong)pAdapter->requestQueueSize - 1);

    pAdapter->requestsArrayBaseDmaAlignedAddr =
    pAdapter->requestsArrayBaseDmaAddr + pAdapter->requestQueueSize;
    pAdapter->requestsArrayBaseDmaAlignedAddr &=
    ~(pAdapter->requestQueueSize - 1);

    if ((pAdapter->requestsArrayBaseDmaAlignedAddr - pAdapter->requestsArrayBaseDmaAddr) !=
        (pAdapter->requestsArrayBaseAlignedAddr - pAdapter->requestsArrayBaseAddr))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: Error in Request Queues Alignment\n",
                 pAdapter->mvSataAdapter.adapterId
                );
        pci_free_consistent(pAdapter->pcidev, requests_array_size,
                            pAdapter->requestsArrayBaseAddr,
                            pAdapter->requestsArrayBaseDmaAddr);
        return -1;
    }
/* response queues */
    pAdapter->responsesArrayBaseAddr =
    pci_alloc_consistent(pAdapter->pcidev,
                         responses_array_size,
                         &(pAdapter->responsesArrayBaseDmaAddr));
    if (pAdapter->responsesArrayBaseAddr == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: Failed to allocate memory for EDMA response"
                 " queues\n", pAdapter->mvSataAdapter.adapterId);
        pci_free_consistent(pAdapter->pcidev, requests_array_size,
                            pAdapter->requestsArrayBaseAddr,
                            pAdapter->requestsArrayBaseDmaAddr);
        return -1;
    }
    pAdapter->responsesArrayBaseAlignedAddr = pAdapter->responsesArrayBaseAddr
                                              + pAdapter->responseQueueSize;
    tmp = (ulong*)&pAdapter->responsesArrayBaseAlignedAddr;
    *tmp &= ~((ulong)pAdapter->responseQueueSize - 1);

    pAdapter->responsesArrayBaseDmaAlignedAddr =
    pAdapter->responsesArrayBaseDmaAddr + pAdapter->responseQueueSize;
    pAdapter->responsesArrayBaseDmaAlignedAddr &=
    ~(pAdapter->responseQueueSize - 1);


    if ((pAdapter->responsesArrayBaseDmaAlignedAddr - pAdapter->responsesArrayBaseDmaAddr) !=
        (pAdapter->responsesArrayBaseAlignedAddr - pAdapter->responsesArrayBaseAddr))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: Error in Response Quueues Alignment\n",
                 pAdapter->mvSataAdapter.adapterId);
        pci_free_consistent(pAdapter->pcidev, responses_array_size,
                            pAdapter->responsesArrayBaseAddr,
                            pAdapter->responsesArrayBaseDmaAddr);
        pci_free_consistent(pAdapter->pcidev, requests_array_size,
                            pAdapter->requestsArrayBaseAddr,
                            pAdapter->requestsArrayBaseDmaAddr);
        return -1;
    }
    return 0;
}

void mv_ial_lib_free_edma_queues(IAL_ADAPTER_T *pAdapter)
{
    pci_free_consistent(pAdapter->pcidev,
                        (pAdapter->maxHosts + 1) * (pAdapter->responseQueueSize),
                        pAdapter->responsesArrayBaseAddr,
                        pAdapter->responsesArrayBaseDmaAddr);
    pci_free_consistent(pAdapter->pcidev,
                        (pAdapter->maxHosts + 1) * (pAdapter->requestQueueSize),
                        pAdapter->requestsArrayBaseAddr,
                        pAdapter->requestsArrayBaseDmaAddr);
}

#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
/**
 * We use this function 
 * to tell scsi error handler that this 
 * device is offline now.
 * 
 * Caller must in the irq call stack.
 * Because after that softirq or other 
 * kernel working thread would do the unplug
 * task. We must set this before them.
 * 
 * This is the same as libata.
 * 
 * @param pSataAdapter
 * @param drivesSnapshotSave
 * @param channelIndex
 */
void SynoIALSCSINotify(struct mvSataAdapter *pSataAdapter, MV_U16 drivesSnapshotSave, MV_U8 channelIndex)
{	
	MV_U16 target;
	IAL_ADAPTER_T	*pAdapter = pSataAdapter->IALData;
	struct Scsi_Host	*host = pAdapter->host[channelIndex]->scsihost;
	struct scsi_device	*sdev = NULL;

	for (target = 0; (drivesSnapshotSave != 0) && (target < host->max_id); target++) {
		if (drivesSnapshotSave & (1 << target)) {
			sdev = scsi_device_lookup(host, 0, target, 0);
			if (sdev != NULL) {
				scsi_device_set_state(sdev, SDEV_OFFLINE);
				scsi_device_put(sdev);
			}
		}
	}
}
#endif

MV_BOOLEAN IALCompletion(struct mvSataAdapter *pSataAdapter,
                         MV_SATA_SCSI_CMD_BLOCK *pCmdBlock)
{
    struct scsi_cmnd   *SCpnt = (struct scsi_cmnd *)pCmdBlock->IALData;
    struct mv_comp_info *pInfo = ( struct mv_comp_info *) &(SCpnt->SCp);
    MV_U8       host_status;


     switch (pCmdBlock->ScsiCommandCompletion)
    {
    case MV_SCSI_COMPLETION_SUCCESS:
        host_status = DID_OK;
        break;
    case MV_SCSI_COMPLETION_BAD_SCSI_COMMAND:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with BAD_SCSI_COMMAND\n");
        host_status = DID_OK;
        break;
    case MV_SCSI_COMPLETION_ATA_FAILED:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with ATA FAILED\n");
        host_status = DID_OK;
        break;
    case MV_SCSI_COMPLETION_UA_RESET:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with UA BUS"
                 " RESET\n");
        scsi_report_bus_reset(pInfo->SCpnt->device->host, pInfo->SCpnt->device->channel);
        host_status = DID_OK;
        break;
    case MV_SCSI_COMPLETION_UA_PARAMS_CHANGED:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with UA "
                 "PARAMETERS CHANGED\n");
        scsi_report_bus_reset(pInfo->SCpnt->device->host, pInfo->SCpnt->device->channel);
        host_status = DID_OK;
        break;

    case MV_SCSI_COMPLETION_QUEUE_FULL:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with QUEUE FULL\n");
        /* flushed from ial common*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        if (pCmdBlock->ScsiStatus == MV_SCSI_STATUS_BUSY)
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Host status: DID_RESET\n");
            host_status = DID_RESET;

            pInfo->SCpnt->flags |= IS_RESETTING;
        }
        else
#endif
        {
            host_status = DID_OK;
        }
        break;

    case MV_SCSI_COMPLETION_ABORTED:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with ABORTED\n");
        host_status = DID_ERROR;
        break;

    case MV_SCSI_COMPLETION_OVERRUN:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with OVERRUN\n");
        if (pInfo->SCpnt->underflow > pCmdBlock->dataTransfered)
        {
            host_status = DID_ERROR;
        }
        else
        {
            host_status = DID_OK;
        }
        break;
    case MV_SCSI_COMPLETION_UNDERRUN:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with UNDERRUN\n");
        host_status = DID_ERROR;
        break;

    case MV_SCSI_COMPLETION_PARITY_ERROR:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "Scsi command completed with PARITY ERROR\n");
        host_status = DID_PARITY;
        break;
    case MV_SCSI_COMPLETION_INVALID_BUS:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with INVALID BUS\n");
        host_status = DID_BAD_TARGET;
        break;
    case MV_SCSI_COMPLETION_NO_DEVICE:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with NO DEVICE\n");
        host_status = DID_NO_CONNECT;
        break;
    case MV_SCSI_COMPLETION_INVALID_STATUS:
    case MV_SCSI_COMPLETION_BAD_SCB:
    case MV_SCSI_COMPLETION_NOT_READY:
    case MV_SCSI_COMPLETION_DISCONNECT:
    case MV_SCSI_COMPLETION_BUS_RESET:
    case MV_SCSI_COMPLETION_BUSY:
    default:
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,"Bad Scsi completion status %d\n",
                 pCmdBlock->ScsiCommandCompletion);
        host_status = DID_ERROR;

    }
    if ((pCmdBlock->senseDataLength == 0) && (pCmdBlock->senseBufferLength))
    {
        pCmdBlock->pSenseBuffer[0] = 0;
    }
    pInfo->SCpnt->result = host_status << 16 | (pCmdBlock->ScsiStatus & 0x3f);
#ifdef MY_ABC_HERE
    scsi_set_resid(pInfo->SCpnt, pCmdBlock->dataBufferLength - pCmdBlock->dataTransfered);
    if(scsi_get_resid(pInfo->SCpnt))
#else
    pInfo->SCpnt->resid = pCmdBlock->dataBufferLength - pCmdBlock->dataTransfered;
    if(pInfo->SCpnt->resid)
#endif
    {
	mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "Scsi command completed with resid 0x%x\n",pInfo->SCpnt->resid);
    }

    {
        MV_U8   channelIndex = pCmdBlock->bus;
        release_ata_mem(pInfo);
        mv_ial_lib_add_done_queue (pSataAdapter->IALData, channelIndex, SCpnt);
    }
    return MV_TRUE;
}

#if defined(MY_DEF_HERE) || defined(MY_ABC_HERE)
int syno_mv_scsi_host_no_get(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex)
{
    IAL_ADAPTER_T   *pAdapter = NULL;
    struct Scsi_Host *host = NULL;
    int ret = 0;
    
    if (!pSataAdapter) {
        goto END;
    }

    pAdapter = pSataAdapter->IALData;
    if (!pAdapter) {
        goto END;
    }

    if (!pAdapter->host[channelIndex]) {
        goto END;
    }

    host = pAdapter->host[channelIndex]->scsihost;
    if (!host) {
        goto END;
    }

    ret = host->host_no;
END:
    return ret;
}
#endif
