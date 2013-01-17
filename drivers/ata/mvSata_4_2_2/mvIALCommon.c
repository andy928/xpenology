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
* mvIALCommon.c
*
* DESCRIPTION:
*       C implementation for IAL's common functions.
*
* DEPENDENCIES:
*   mvIALCommon.h
*
*******************************************************************************/

/* includes */
#include "mvOs.h"
#include "mvScsiAtaLayer.h"
#include "mvIALCommon.h"
#include "mvIALCommonUtils.h"
#include "mvStorageDev.h"

#ifdef SYNO_SATA_PM_DEVICE_GPIO
//#define DEBUGMSG(x...) printk(x)
#define DEBUGMSG(x...)
#endif

/* defines */
#undef DISABLE_PM_SCC
/* typedefs */

/*Static functions*/
static MV_BOOLEAN mvGetDisksModes(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                  MV_SAL_ADAPTER_EXTENSION *pScsiAdapterExt,
                                  MV_U8 channelIndex,
                                  MV_BOOLEAN *TCQ,
                                  MV_BOOLEAN *NCQ,
                                  MV_U8   *numOfDrives);

static void mvFlushSCSICommandQueue(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                    MV_U8 channelIndex);

static void mvAddToSCSICommandQueue(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                    MV_U8 channelIndex,
                                    MV_SATA_SCSI_CMD_BLOCK *Scb);

static MV_BOOLEAN mvAdapterStateMachine(
                                       MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                       MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

static MV_BOOLEAN mvChannelStateMachine(
                                       MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                       MV_U8 channelIndex,
                                       MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

static void mvSetChannelState(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                              MV_U8 channelIndex,
                              MV_CHANNEL_STATE state);

static MV_BOOLEAN clearSErrorPorts(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex, 
				   MV_U8     PMnumberOfPorts);
/*PM related*/
static MV_BOOLEAN mvPMCommandCompletionCB(MV_SATA_ADAPTER *pSataAdapter,
                                          MV_U8 channelIndex,
                                          MV_COMPLETION_TYPE comp_type,
                                          MV_VOID_PTR commandId,
                                          MV_U16 responseFlags,
                                          MV_U32 timeStamp,
                                          MV_STORAGE_DEVICE_REGISTERS *registerStruct);


static MV_BOOLEAN mvQueuePMAccessRegisterCommand(
                                                MV_IAL_COMMON_ADAPTER_EXTENSION* ialExt,
                                                MV_U8 channelIndex,
                                                MV_U8 PMPort,
                                                MV_U8 PMReg,
                                                MV_U32 Value,
                                                MV_BOOLEAN isRead);

static MV_BOOLEAN mvPMEnableCommStatusChangeBits(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                                 MV_U8 channelIndex,
                                                 MV_BOOLEAN enable);

static MV_BOOLEAN mvPMEnableAsyncNotify(
                                       MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                       MV_U8 channelIndex);

#if 0
static void mvCheckPMForError(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                              MV_U8 channelIndex);
#endif

/*End PM related*/

static MV_BOOLEAN mvStartChannelInit(MV_SATA_ADAPTER *pSataAdapter,
                                     MV_U8 channelIndex,
                                     MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                                     MV_BOOLEAN *isChannelReady);

static MV_BOOLEAN mvChannelSRSTFinished(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_SATA_CHANNEL *pSataChannel,
                                        MV_U8 channelIndex,
                                        MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                                        MV_BOOLEAN* bIsChannelReady,
                                        MV_BOOLEAN* bFatalError);

static MV_BOOLEAN mvConfigChannelQueuingMode(
                                            MV_IAL_COMMON_ADAPTER_EXTENSION* ialExt,
                                            MV_U8 channelIndex,
                                            MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

static MV_BOOLEAN mvConfigChannelDMA(
                                    MV_IAL_COMMON_ADAPTER_EXTENSION* ialExt,
                                    MV_U8 channelIndex,
                                    MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

static void mvSetChannelTimer(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                              MV_U8 channelIndex,
                              MV_U32 timeout);
static void mvDecrementChannelTimer(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                    MV_U8 channelIndex);
static MV_BOOLEAN mvIsChannelTimerExpired(
                                         MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                         MV_U8 channelIndex);


/*Channel state machine*/


static MV_BOOLEAN mvChannelNotConnectedStateHandler(
                                                   MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                                   MV_U8 channelIndex,
                                                   MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);


static MV_BOOLEAN mvChannelConnectedStateHandler(
                                                MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                                MV_U8 channelIndex,
                                                MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

MV_BOOLEAN mvChannelInSrstStateHandler(
                                      MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                      MV_U8 channelIndex,
                                      MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

static MV_BOOLEAN mvPMInitDevicesStateHandler(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
					      MV_U8 channelIndex,
					      MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

static MV_BOOLEAN mvChannelReadyStateHandler(
                                            MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                            MV_U8 channelIndex);

static MV_BOOLEAN mvChannelPMHotPlugStateHandler(
                                                MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                                MV_U8 channelIndex,
                                                MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);



static void mvDrivesInfoSaveAll(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                MV_U8 channelIndex);

static void mvDrivesInfoFlushAll(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                 MV_U8 channelIndex);

static void mvDrivesInfoFlushSingleDrive(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                         MV_U8 channelIndex, MV_U8 PMPort);

static void mvDrivesInfoSaveSingleDrive(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_U8 channelIndex,
                                        MV_U8 PMPort,
                                        MV_BOOLEAN  isDriveAdded,
                                        MV_U16_PTR identifyBuffer);

static void mvSetDriveReady(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                            MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                            MV_U8 channelIndex,
                            MV_U8 PMPort,
                            MV_BOOLEAN  isReady,
                            MV_U16_PTR identifyBuffer);

static void mvDrivesInfoGetChannelRescanParams(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                               MV_U8 channelIndex,
                                               MV_U16 *drivesToRemove,
                                               MV_U16 *drivesToAdd);



#ifdef DISABLE_PM_SCC
MV_BOOLEAN mvPMDisableSSC(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex);
#endif

MV_BOOLEAN mvPMEnableLocking(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex);

#ifdef MY_ABC_HERE
static MV_BOOLEAN blSynoEHTypeRevalidate(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                             MV_SAL_ADAPTER_EXTENSION *scsiAdapterExtstruct,
                             MV_SATA_ADAPTER *pSataAdapter,
                             MV_U8 channel,
                             MV_SATA_DEVICE_TYPE deviceType);
static MV_BOOLEAN blSynoEHDiskRevalidate(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                             MV_SAL_ADAPTER_EXTENSION *scsiAdapterExtstruct,
                             MV_SATA_ADAPTER *pSataAdapter,
                             MV_U8 channel,
                             MV_U8 PMPort,            
                             MV_U16_PTR identifyBuffer);
#endif
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
static void SynoClearEHInfo(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                MV_U8 channelIndex,
                MV_CHANNEL_STATE state);
#endif



/*Public functions*/

/*******************************************************************************
* mvAdapterStartInitialization - start adapter initialization
*
* DESCRIPTION:
*  Starts adapter initialization after driver load.
*  State - machine related data structure is initialized for adapter
*  and its channels. Begin staggered spin-up.
*  Adapter state is changed to ADAPTER_READY.
* INPUT:
*    pAdapter    - pointer to the adapter data structure.
*    scsiAdapterExt  - SCSI to ATA layer adapter extension data structure
* OUTPUT:
*    None.
*
* RETURN:
*    MV_TRUE on success
*    MV_FALSE on error
*
*******************************************************************************/

MV_BOOLEAN mvAdapterStartInitialization(MV_SATA_ADAPTER *pSataAdapter,
                                        MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_U8 channelIndex;
    ialExt->pSataAdapter = pSataAdapter;
    ialExt->adapterState = ADAPTER_INITIALIZING;
    for (channelIndex = 0; channelIndex < MV_SATA_CHANNELS_NUM; channelIndex++)
    {
        ialExt->channelState[channelIndex] = CHANNEL_NOT_CONNECTED;
        ialExt->IALChannelExt[channelIndex].SRSTTimerThreshold = 0;
        ialExt->IALChannelExt[channelIndex].SRSTTimerValue = 0;
        ialExt->IALChannelExt[channelIndex].IALChannelPendingCmdQueue = NULL;
        ialExt->IALChannelExt[channelIndex].completionError = MV_FALSE;
        ialExt->IALChannelExt[channelIndex].pmRegAccessInProgress = MV_FALSE;
        ialExt->IALChannelExt[channelIndex].pmRegPollCounter = 0;
        ialExt->IALChannelExt[channelIndex].devInSRST =
        MV_SATA_PM_CONTROL_PORT + 1;
        ialExt->IALChannelExt[channelIndex].PMdevsToInit = 0;
        ialExt->IALChannelExt[channelIndex].PMnumberOfPorts = 0;
        ialExt->IALChannelExt[channelIndex].pmAccessType = 0;
        ialExt->IALChannelExt[channelIndex].pmReg = 0;
        ialExt->IALChannelExt[channelIndex].pmAsyncNotifyEnabled = MV_FALSE;
        ialExt->IALChannelExt[channelIndex].bHotPlug = MV_FALSE;
        memset(&ialExt->IALChannelExt[channelIndex].drivesInfo, 0, sizeof(MV_DRIVES_INFO));
    }
#ifdef MY_ABC_HERE
    SynoInitChannelEH(pSataAdapter->IALData, pSataAdapter);
#endif
    return mvAdapterStateMachine(ialExt, scsiAdapterExt);
}

#ifdef MY_DEF_HERE
extern int syno_mv_scsi_host_no_get(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex);
extern int (*funcSYNOSendEboxRefreshEvent)(int portIndex);
#endif

/*******************************************************************************
* mvRestartChannel - restart specific channel
*
* DESCRIPTION:
*  The function is used in channel hot-plug to restart the channel
*  initialization sequence. The channel stated is changed to
*  CHANNEL_CONNECTED and any pending command in software queue are flushed
* INPUT:
*    pAdapter    - pointer to the adapter data structure.
*    channelIndex  - channel number
*    scsiAdapterExt  - SCSI to ATA layer adapter extension data structure
*    bBusReset       - MV_TRUE if the faunction is called because of bus reset,
*                       MV_FALSE otherwise
* OUTPUT:
*    None.
*
* RETURN:
*    MV_TRUE on success
*    MV_FALSE on error
*
*******************************************************************************/

void mvRestartChannel(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                      MV_U8 channelIndex,
                      MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                      MV_BOOLEAN bBusReset)
{
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    MV_BOOLEAN bBusChangeNotify = MV_FALSE;
    ialExt->IALChannelExt[channelIndex].bHotPlug = MV_TRUE;
    mvSetDriveReady(ialExt,
                    scsiAdapterExt,
                    channelIndex, 0xFF, MV_FALSE, NULL);
    if (pSataAdapter->sataChannel[channelIndex] != NULL)
    {
        if (mvStorageDevGetDeviceType(pSataAdapter,channelIndex)
            == MV_SATA_DEVICE_TYPE_PM)
        {
            bBusChangeNotify = MV_TRUE;
#ifdef MY_DEF_HERE
			if(funcSYNOSendEboxRefreshEvent) {
				funcSYNOSendEboxRefreshEvent(syno_mv_scsi_host_no_get(pSataAdapter, channelIndex));
			}
#endif
        }
        mvSataDisableChannelDma(pSataAdapter, channelIndex);
        mvSataFlushDmaQueue (pSataAdapter, channelIndex,
                             MV_FLUSH_TYPE_CALLBACK);
    }
    mvFlushSCSICommandQueue(ialExt, channelIndex);
    ialExt->IALChannelExt[channelIndex].SRSTTimerThreshold = 0;
    ialExt->IALChannelExt[channelIndex].SRSTTimerValue = 0;
    ialExt->channelState[channelIndex] = CHANNEL_CONNECTED;
    if (bBusReset == MV_TRUE)
    {
        if (bBusChangeNotify == MV_TRUE)
        {
            /*Notify about bus change*/
            IALBusChangeNotify(pSataAdapter, channelIndex);
        }
    }
    else
    {
        /*Notify about bus change*/
        IALBusChangeNotify(pSataAdapter, channelIndex);
    }
}



/*******************************************************************************
* mvPMHotPlugDetected - restart specific channel
*
* DESCRIPTION:
*  The function is used in PM hot-plug to wait for empty EDMA command queue
*  and then restart the channel initialization sequence.
*  The channel stated is changed to CHANNEL_PM_HOT_PLUG if there are any
*  pending command in EDMA queue
* INPUT:
*    pAdapter    - pointer to the adapter data structure.
*    channelIndex  - channel number

* OUTPUT:
*    None.
*
* RETURN:
*    None
*
*******************************************************************************/

void mvPMHotPlugDetected(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                         MV_U8 channelIndex,
                         MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    if (ialExt->channelState[channelIndex] == CHANNEL_NOT_CONNECTED ||
        ialExt->channelState[channelIndex] == CHANNEL_CONNECTED ||
        ialExt->channelState[channelIndex] == CHANNEL_IN_SRST)
    {
        return;
    }
    mvSataDisableChannelDma(ialExt->pSataAdapter, channelIndex);
    mvSataFlushDmaQueue (ialExt->pSataAdapter,
                         channelIndex, MV_FLUSH_TYPE_CALLBACK);
    mvSataChannelHardReset(ialExt->pSataAdapter, channelIndex);
    mvRestartChannel(ialExt, channelIndex, scsiAdapterExt, MV_FALSE);
}

/*******************************************************************************
* mvStopChannel - stop channel
*
* DESCRIPTION:
*  The function is used when the channel is unplugged.
*  The channel stated is changed to CHANNEL_NOT_CONNECTED
*  until further connection.
* INPUT:
*    pAdapter    - pointer to the adapter data structure.
*    channelIndex  - channel number
* OUTPUT:
*    None.
*
* RETURN:
*    None
*******************************************************************************/
void mvStopChannel(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                   MV_U8 channelIndex,
                   MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    MV_U16 drivesSnapshot =
    ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotSaved;
#ifdef MY_ABC_HERE
    MV_U32 old_eh_flags = ialExt->pSataAdapter->eh[channelIndex].flags;
#endif

#ifdef MY_ABC_HERE
    if (pSataAdapter &&
        pSataAdapter->eh[channelIndex].flags & EH_PROCESSING) {
        syno_eh_printk(pSataAdapter, channelIndex, 
                       "EH is processing. But Someone call stopchannel, just schedule EH");
        queue_delayed_work(mvSata_aux_wq, &(pSataAdapter->eh[channelIndex].work), 0);
        return;
    }
#endif

    mvDrivesInfoFlushAll(ialExt, channelIndex);
    mvSetDriveReady(ialExt, scsiAdapterExt, channelIndex, 0xFF, MV_FALSE, NULL);
    mvSetChannelState(ialExt, channelIndex, CHANNEL_NOT_CONNECTED);
    if (pSataAdapter->sataChannel[channelIndex] != NULL)
    {
        mvSataDisableChannelDma(ialExt->pSataAdapter, channelIndex);
        mvSataFlushDmaQueue (ialExt->pSataAdapter, channelIndex,
                             MV_FLUSH_TYPE_CALLBACK);
    }
    mvFlushSCSICommandQueue(ialExt, channelIndex);
    if (pSataAdapter->sataChannel[channelIndex] != NULL)
    {
        mvSataRemoveChannel(pSataAdapter,channelIndex);
        IALReleaseChannel(pSataAdapter, channelIndex);
    }
    pSataAdapter->sataChannel[channelIndex] = NULL;
    /*Notify about bus change*/
    IALBusChangeNotify(pSataAdapter, channelIndex);
    if (drivesSnapshot != 0)
    {
#ifdef MY_ABC_HERE
        IALBusChangeNotifyEx(pSataAdapter, channelIndex, drivesSnapshot, 0, old_eh_flags);
#else
        IALBusChangeNotifyEx(pSataAdapter, channelIndex, drivesSnapshot, 0);
#endif
    }

#ifdef MY_ABC_HERE
    mvSataSetInterfaceSpeed(pSataAdapter, channelIndex, MV_SATA_IF_SPEED_NO_LIMIT);
#endif
}

#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
/**
 * This function is 
 * almost the same as mvStopChannel.
 * 
 * The only difference is setting scsi
 * device to offline stat.
 * 
 * This can help us determind the reason of bad sector
 * 
 * @param ialExt
 * @param channelIndex
 * @param scsiAdapterExt
 */
void SynomvStopChannel(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                   MV_U8 channelIndex,
                   MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    MV_U16 drivesSnapshot =
    ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotSaved;
#ifdef MY_ABC_HERE
    MV_U32 old_eh_flags = ialExt->pSataAdapter->eh[channelIndex].flags;
#endif

    if (CHANNEL_NOT_CONNECTED == ialExt->channelState[channelIndex]) {        
        syno_eh_printk(ialExt->pSataAdapter, channelIndex, "channel state already become CHANNEL_NOT_CONNECTED, do nothing\n");
        return;
    }

    SynoIALSCSINotify(ialExt->pSataAdapter, drivesSnapshot, channelIndex);
    mvDrivesInfoFlushAll(ialExt, channelIndex);
    mvSetDriveReady(ialExt, scsiAdapterExt, channelIndex, 0xFF, MV_FALSE, NULL);
    mvSetChannelState(ialExt, channelIndex, CHANNEL_NOT_CONNECTED);
    if (pSataAdapter->sataChannel[channelIndex] != NULL)
    {
        mvSataDisableChannelDma(ialExt->pSataAdapter, channelIndex);
        mvSataFlushDmaQueue (ialExt->pSataAdapter, channelIndex,
                             MV_FLUSH_TYPE_CALLBACK);
    }    
    mvFlushSCSICommandQueue(ialExt, channelIndex);
    if (pSataAdapter->sataChannel[channelIndex] != NULL)
    {
        mvSataRemoveChannel(pSataAdapter,channelIndex);
        IALReleaseChannel(pSataAdapter, channelIndex);
    }
    pSataAdapter->sataChannel[channelIndex] = NULL;
    /*Notify about bus change*/
    IALBusChangeNotify(pSataAdapter, channelIndex);    
    if (drivesSnapshot != 0)
    {
#ifdef MY_ABC_HERE
        IALBusChangeNotifyEx(pSataAdapter, channelIndex, drivesSnapshot, 0, old_eh_flags);
#else
        IALBusChangeNotifyEx(pSataAdapter, channelIndex, drivesSnapshot, 0);
#endif
    }

#ifdef MY_ABC_HERE
    mvSataSetInterfaceSpeed(pSataAdapter, channelIndex, MV_SATA_IF_SPEED_NO_LIMIT);
#endif
}
#endif


/*******************************************************************************
* mvExecuteScsiCommand - execute SCSI command
*
* DESCRIPTION:
*  IAL common layer wrapper of mvSataExecuteScsiCommand function.
*  If either the adapter state is either other than ADAPTER_READY
*  or the channel is connected but channel state is not CHANNEL_READY,
*  the current SCSI command is queued in channel's SCSI commands
*  software queue until channel initialization sequence completed.
*  If channel is found in CHANNEL ready state the SCSI command is passed to
*  SCSI ATA translation layer.
* INPUT:
*    pScb    - SCSI command block structure.
*
* OUTPUT:
*    None.
*
* RETURN:
*    Return MV_SCSI_COMMAND_STATUS_COMPLETED if the command has been added
*    to channel software queue. Otherwise return the result of
*    mvSataExecuteScsiCommand function call
*******************************************************************************/
MV_SCSI_COMMAND_STATUS_TYPE mvExecuteScsiCommand(MV_SATA_SCSI_CMD_BLOCK *pScb,
                                                 MV_BOOLEAN canQueue)
{
    MV_IAL_COMMON_ADAPTER_EXTENSION* ialExt = pScb->pIalAdapterExtension;
    MV_U8 channelIndex = pScb->bus;

#if 0
    if ((ialExt->adapterState == ADAPTER_READY) &&
        (ialExt->channelState[channelIndex] == CHANNEL_READY))
    {
        mvCheckPMForError(ialExt, channelIndex);
    }
#endif

    if ((ialExt->adapterState == ADAPTER_READY) &&
        ((ialExt->channelState[channelIndex] == CHANNEL_READY) ||
         (ialExt->channelState[channelIndex] == CHANNEL_NOT_CONNECTED)))
    {
        return mvSataExecuteScsiCommand(pScb);
    }
    if (canQueue == MV_FALSE)
    {
        pScb->ScsiStatus = MV_SCSI_STATUS_BUSY;
        pScb->dataTransfered = 0;
        pScb->ScsiCommandCompletion = MV_SCSI_COMPLETION_QUEUE_FULL;
        if (pScb->completionCallBack)
        {
            pScb->completionCallBack(ialExt->pSataAdapter, pScb);
        }
        return MV_SCSI_COMMAND_STATUS_COMPLETED;
    }
    else
    {
        mvAddToSCSICommandQueue(ialExt, channelIndex, pScb);
        return MV_SCSI_COMMAND_STATUS_QUEUED_BY_IAL;
    }
}


/*******************************************************************************
* mvIALTimerCallback - IAL timer callback
*
* DESCRIPTION:
*  The adapter/channel state machine is timer-driven.
*  After being loaded, the IAL must call this callback every 0.5 seconds
* INPUT:
*    pSataAdapter    - pointer to the adapter data structure.
*    scsiAdapterExt  - SCSI to ATA layer adapter extension data structure
* OUTPUT:
*    None.
*
* RETURN:
*
*******************************************************************************/

MV_BOOLEAN mvIALTimerCallback(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                              MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{

    return mvAdapterStateMachine(ialExt,
                                 scsiAdapterExt);
}

/*******************************************************************************
* mvCommandCompletionErrorHandler - IAL common command completion error handler
*
* DESCRIPTION:
*  Called by whether SAL completion of SMART completion function. Check whether
*  command is failed because of PM hot plug
*
* INPUT:
*    pSataAdapter    - pointer to the adapter data structure.
*    channelIndex    - channelNumber
* OUTPUT:
*    None.
*
* RETURN:
*
*******************************************************************************/

void mvCommandCompletionErrorHandler(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                     MV_U8 channelIndex)
{
    MV_SATA_ADAPTER* pSataAdapter = ialExt->pSataAdapter;
    if (pSataAdapter->sataChannel[channelIndex] == NULL)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: "
                 "Invalid channel data structure pointer.\n",
                 pSataAdapter->adapterId, channelIndex);
    }

    if ((ialExt->channelState[channelIndex] != CHANNEL_READY) ||
        (mvStorageDevGetDeviceType(pSataAdapter,channelIndex) !=
         MV_SATA_DEVICE_TYPE_PM) ||
        (ialExt->IALChannelExt[channelIndex].pmAsyncNotifyEnabled == MV_TRUE))
    {
        return;
    }
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: "
             "Set completion error to MV_TRUE.\n",
             pSataAdapter->adapterId, channelIndex);
    ialExt->IALChannelExt[channelIndex].completionError = MV_TRUE;
}

/*Static functions*/

static void printAtaDeviceRegisters(
                                   MV_STORAGE_DEVICE_REGISTERS *mvStorageDevRegisters)
{

    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "ATA Drive Registers:\n");
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "%20s : %04x\n","Error",
             mvStorageDevRegisters->errorRegister);
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "%20s : %04x\n","SectorCount",
             mvStorageDevRegisters->sectorCountRegister);
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "%20s : %04x\n","LBA Low",
             mvStorageDevRegisters->lbaLowRegister);
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "%20s : %04x\n","LBA Mid",
             mvStorageDevRegisters->lbaMidRegister);
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "%20s : %04x\n","LBA High",
             mvStorageDevRegisters->lbaHighRegister);
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "%20s : %04x\n","Device",
             mvStorageDevRegisters->deviceRegister);
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "%20s : %04x\n","Status",
             mvStorageDevRegisters->statusRegister);
}



static void mvDrivesInfoSaveAll(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                MV_U8 channelIndex)
{
    /*Save disk drives information for channel*/
    ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotSaved =
    ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotCurrent;
    memcpy(ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialSaved,
           ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent,
           sizeof(ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent));
    /*Reset current disk drives information*/
    ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotCurrent = 0;
    memset(ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent,
           0,
           sizeof(ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent));

}

static void mvDrivesInfoFlushAll(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                 MV_U8 channelIndex)
{
    /*Flush drives info*/
    memset(&ialExt->IALChannelExt[channelIndex].drivesInfo, 0,
           sizeof(ialExt->IALChannelExt[channelIndex].drivesInfo));
}

static void mvDrivesInfoFlushSingleDrive(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                         MV_U8 channelIndex, MV_U8 PMPort)
{
#ifdef MY_ABC_HERE
    if (ialExt->pSataAdapter->eh[channelIndex].flags & EH_PROCESSING) {
        syno_eh_printk(ialExt->pSataAdapter, channelIndex, "EH is going, do not clear driveSerialSaved PMPort %d", PMPort);
        return;
    }
#endif
    /*Clear bit in disk drive drive snapshot*/
    ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotCurrent &=
    ~(1 << PMPort);
    ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotSaved &=
    ~(1 << PMPort);
    /*Clear disk drive serial number string*/
    ialExt->
    IALChannelExt[channelIndex].drivesInfo.driveSerialSaved[PMPort].serial[0] = 0;
    ialExt->
    IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent[PMPort].serial[0] = 0;
}


static void mvDrivesInfoSaveSingleDrive(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_U8 channelIndex,
                                        MV_U8 PMPort,
                                        MV_BOOLEAN  isDriveAdded,
                                        MV_U16_PTR identifyBuffer)
{
    if (MV_TRUE == isDriveAdded)
    {
        /*Set bit in disk drive snapshot for current disk drive*/
        ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotCurrent |=
        1 << PMPort;
        /*Save serial number for current disk drive*/
        memcpy(ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent[PMPort].serial,
               &identifyBuffer[IDEN_SERIAL_NUM_OFFSET], IDEN_SERIAL_NUM_SIZE);
    }
    else
    {
        if (0xFF == PMPort)
        {
            ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotCurrent
            = 0;
            memset(ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent,
                   0,
                   sizeof(ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent));
        }
        else
        {
            ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotCurrent
            &= ~(1 << PMPort);
            ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent[PMPort].serial[0] = 0;
        }
    }
}

static void mvSetDriveReady(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                            MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                            MV_U8 channelIndex,
                            MV_U8 PMPort,
                            MV_BOOLEAN  isReady,
                            MV_U16_PTR identifyBuffer)
{
    mvDrivesInfoSaveSingleDrive(ialExt,
                                channelIndex,
                                PMPort,
                                isReady,
                                identifyBuffer);
    mvSataScsiSetDriveReady(scsiAdapterExt,
                            channelIndex, PMPort, isReady);
}


static void mvDrivesInfoGetChannelRescanParams(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                               MV_U8 channelIndex,
                                               MV_U16 *drivesToRemove,
                                               MV_U16 *drivesToAdd)
{
    MV_U8 PMPort;

    *drivesToRemove = 0;
    *drivesToAdd = 0;

    for (PMPort = 0; PMPort < MV_SATA_PM_MAX_PORTS; PMPort++)
    {
        if (ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotCurrent
            & (1 << PMPort))
        {
            if (ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotSaved
                & (1 << PMPort))
            {
                if (memcmp(&ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialCurrent[PMPort].serial,
                           &ialExt->IALChannelExt[channelIndex].drivesInfo.driveSerialSaved[PMPort].serial,
                           IDEN_SERIAL_NUM_SIZE))
                {
                    /*Disk drive connected to port is replaced*/
                    *drivesToAdd |= (1 << PMPort);
                    *drivesToRemove |= (1 << PMPort);
                }
            }
            else
            {
                /*New drive connected to port*/
                *drivesToAdd |= (1 << PMPort);
            }
        }
        else
        {
            /*Drive removed from Port*/
            if (ialExt->IALChannelExt[channelIndex].drivesInfo.drivesSnapshotSaved
                & (1 << PMPort))
            {
                *drivesToRemove |= (1 << PMPort);
#ifdef MY_ABC_HERE
                SynoIALSCSINotify(ialExt->pSataAdapter, *drivesToRemove, channelIndex);
#endif
            }
        }
    }
}



/*SCSI command queue functions*/
static void mvAddToSCSICommandQueue(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                    MV_U8 channelIndex,
                                    MV_SATA_SCSI_CMD_BLOCK *pScb)
{

    MV_SATA_SCSI_CMD_BLOCK *cmdBlock = (MV_SATA_SCSI_CMD_BLOCK *)
                                       ialExt->IALChannelExt[channelIndex].IALChannelPendingCmdQueue;
    pScb->pNext = NULL;
    if (cmdBlock)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d] Adding next command to SW queue\n",
                 ialExt->pSataAdapter->adapterId, channelIndex);
        while (cmdBlock->pNext)
        {
            cmdBlock = cmdBlock->pNext;
        }
        cmdBlock->pNext = pScb;
    }
    else
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d] Adding first command to SW queue\n",
                 ialExt->pSataAdapter->adapterId, channelIndex);
        ialExt->IALChannelExt[channelIndex].IALChannelPendingCmdQueue =
        (MV_VOID_PTR)pScb;
    }
}

MV_BOOLEAN mvRemoveFromSCSICommandQueue(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_U8 channelIndex,
                                        MV_SATA_SCSI_CMD_BLOCK *pScb)
{

    MV_SATA_SCSI_CMD_BLOCK *cmdBlock = (MV_SATA_SCSI_CMD_BLOCK *)
                                       ialExt->IALChannelExt[channelIndex].IALChannelPendingCmdQueue;
    pScb->pNext = NULL;
    if (cmdBlock)
    {
        if (cmdBlock == pScb)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d] Removing command"
                     " %p from head of SW queue\n",
                     ialExt->pSataAdapter->adapterId,channelIndex, pScb);
            ialExt->IALChannelExt[channelIndex].IALChannelPendingCmdQueue =
            (MV_VOID_PTR) cmdBlock->pNext;
            return MV_TRUE;
        }
        else
        {
            while (cmdBlock->pNext)
            {
                if (cmdBlock->pNext == pScb)
                {
                    break;
                }
                cmdBlock = cmdBlock->pNext;
            }
            if (cmdBlock->pNext == NULL)
            {
                mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d] Removing"
                         " command %p from SW queue failed. command not found\n",
                         ialExt->pSataAdapter->adapterId,channelIndex, pScb);
                return MV_FALSE;
            }
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d] Removing command"
                     " %p from SW queue\n", ialExt->pSataAdapter->adapterId,
                     channelIndex, pScb);
            cmdBlock->pNext = cmdBlock->pNext->pNext;
            return MV_TRUE;
        }
    }
    else
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d] Removing"
                 " command %p from SW queue failed. queue empty\n",
                 ialExt->pSataAdapter->adapterId,channelIndex, pScb);
        return MV_FALSE;
    }
    return MV_FALSE;
}

static void mvFlushSCSICommandQueue(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                    MV_U8 channelIndex)
{
    /*Abort all pending commands in SW queue*/
    MV_SATA_SCSI_CMD_BLOCK *cmdBlock = (MV_SATA_SCSI_CMD_BLOCK *)
                                       ialExt->IALChannelExt[channelIndex].IALChannelPendingCmdQueue;

    while (cmdBlock)
    {
        MV_SATA_SCSI_CMD_BLOCK *nextBlock = cmdBlock->pNext;
        if (cmdBlock->completionCallBack)
        {
            cmdBlock->ScsiStatus = MV_SCSI_STATUS_BUSY;
            cmdBlock->dataTransfered = 0;
            cmdBlock->ScsiCommandCompletion = MV_SCSI_COMPLETION_QUEUE_FULL;
            cmdBlock->completionCallBack(ialExt->pSataAdapter, cmdBlock);
        }
        cmdBlock = nextBlock;
    }
    ialExt->IALChannelExt[channelIndex].IALChannelPendingCmdQueue = NULL;
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: Flush command queue is done\n",
             ialExt->pSataAdapter->adapterId, channelIndex);
}


/*Port Multilier related functions*/
static MV_BOOLEAN mvPMCommandCompletionCB(MV_SATA_ADAPTER *pSataAdapter,
                                          MV_U8 channelIndex,
                                          MV_COMPLETION_TYPE comp_type,
                                          MV_VOID_PTR commandId,
                                          MV_U16 responseFlags,
                                          MV_U32 timeStamp,
                                          MV_STORAGE_DEVICE_REGISTERS *registerStruct)
{
    MV_U32 value;
    MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt =
    (MV_IAL_COMMON_ADAPTER_EXTENSION *)commandId;
    ialExt->IALChannelExt[channelIndex].pmRegAccessInProgress = MV_FALSE;
    switch (comp_type)
    {
    case MV_COMPLETION_TYPE_NORMAL:
        if (ialExt->IALChannelExt[channelIndex].pmAccessType
            == MV_ATA_COMMAND_PM_READ_REG)
        {
            value = registerStruct->sectorCountRegister;
            value |= (registerStruct->lbaLowRegister << 8);
            value |= (registerStruct->lbaMidRegister << 16);
            value |= (registerStruct->lbaMidRegister << 24);
            if (ialExt->IALChannelExt[channelIndex].pmReg
                == MV_SATA_GSCR_ERROR_REG_NUM)
            {
                if (value != 0)
                {
                    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: PM GSCR[32] = 0x%X\n",
                             pSataAdapter->adapterId, channelIndex, value);
                    ialExt->IALChannelExt[channelIndex].PMdevsToInit =
                    (MV_U16)(value & 0x7FFF);
                    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: "
                             "PM Hot plug detected "
                             "Bitmask = 0x%X\n",
                             pSataAdapter->adapterId, channelIndex,
                             ialExt->IALChannelExt[channelIndex].PMdevsToInit);
                    mvSetChannelState(ialExt, channelIndex,
                                      CHANNEL_PM_HOT_PLUG);
                }
            }
        }
        break;
    case MV_COMPLETION_TYPE_ABORT:
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: read PM register aborted!\n",
                 pSataAdapter->adapterId, channelIndex);

        break;
    case MV_COMPLETION_TYPE_ERROR:
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: read PM register error!\n",
                 pSataAdapter->adapterId, channelIndex);
        break;
    default:
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Unknown completion type (%d)\n",
                 pSataAdapter->adapterId, channelIndex, comp_type);
        return MV_FALSE;
    }
    return MV_TRUE;
}


static MV_BOOLEAN mvQueuePMAccessRegisterCommand(
                                                MV_IAL_COMMON_ADAPTER_EXTENSION* ialExt,
                                                MV_U8 channelIndex,
                                                MV_U8 PMPort,
                                                MV_U8 PMReg,
                                                MV_U32 Value,
                                                MV_BOOLEAN isRead)
{
    MV_QUEUE_COMMAND_RESULT result;
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
#ifdef MV_SATA_STORE_COMMANDS_INFO_ON_IAL_STACK
    MV_QUEUE_COMMAND_INFO *pCommandInfo = &ialExt->IALChannelExt[channelIndex].commandInfo;
#else
    MV_QUEUE_COMMAND_INFO commandInfo, *pCommandInfo;
    pCommandInfo = &commandInfo;
#endif
    memset(pCommandInfo, 0, sizeof(MV_QUEUE_COMMAND_INFO));
    pCommandInfo->type = MV_QUEUED_COMMAND_TYPE_NONE_UDMA;
    pCommandInfo->commandParams.NoneUdmaCommand.protocolType =
    MV_NON_UDMA_PROTOCOL_NON_DATA;
    pCommandInfo->commandParams.NoneUdmaCommand.isEXT = MV_TRUE;
    pCommandInfo->commandParams.NoneUdmaCommand.bufPtr = NULL;
    pCommandInfo->PMPort = MV_SATA_PM_CONTROL_PORT;
    pCommandInfo->commandParams.NoneUdmaCommand.count = 0;
    pCommandInfo->commandParams.NoneUdmaCommand.features = PMReg;
    pCommandInfo->commandParams.NoneUdmaCommand.device = (MV_U8)PMPort;
    pCommandInfo->commandParams.NoneUdmaCommand.callBack =
    mvPMCommandCompletionCB;
#ifdef SYNO_SATA_PM_DEVICE_GPIO
    pCommandInfo->commandParams.NoneUdmaCommand.SynoExtCallBack = NULL;
#endif

    ialExt->IALChannelExt[channelIndex].pmReg = PMReg;

    if (isRead == MV_TRUE)
    {
        ialExt->IALChannelExt[channelIndex].pmAccessType =
        MV_ATA_COMMAND_PM_READ_REG;
        pCommandInfo->commandParams.NoneUdmaCommand.command =
        MV_ATA_COMMAND_PM_READ_REG;
        pCommandInfo->commandParams.NoneUdmaCommand.commandId =
        (MV_VOID_PTR)ialExt;
        pCommandInfo->commandParams.NoneUdmaCommand.sectorCount = 0;
        pCommandInfo->commandParams.NoneUdmaCommand.lbaLow = 0;
        pCommandInfo->commandParams.NoneUdmaCommand.lbaMid = 0;
        pCommandInfo->commandParams.NoneUdmaCommand.lbaHigh = 0;
    }
    else
    {
        ialExt->IALChannelExt[channelIndex].pmAccessType =
        MV_ATA_COMMAND_PM_WRITE_REG;
        pCommandInfo->commandParams.NoneUdmaCommand.command =
        MV_ATA_COMMAND_PM_WRITE_REG;
        pCommandInfo->commandParams.NoneUdmaCommand.commandId =
        (MV_VOID_PTR)ialExt;
        pCommandInfo->commandParams.NoneUdmaCommand.sectorCount =
        (MV_U16)((Value) & 0xff),
        pCommandInfo->commandParams.NoneUdmaCommand.lbaLow =
        (MV_U16)(((Value) & 0xff00) >> 8);
        pCommandInfo->commandParams.NoneUdmaCommand.lbaMid =
        (MV_U16)(((Value) & 0xff0000) >> 16);
        pCommandInfo->commandParams.NoneUdmaCommand.lbaHigh =
        (MV_U16)(((Value) & 0xff000000) >> 24);
    }
    ialExt->IALChannelExt[channelIndex].pmRegAccessInProgress = MV_TRUE;
    result = mvSataQueueCommand(pSataAdapter,
                                channelIndex,
                                pCommandInfo);
    if (result != MV_QUEUE_COMMAND_RESULT_OK)
    {
        switch (result)
        {
        case MV_QUEUE_COMMAND_RESULT_BAD_LBA_ADDRESS:
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, ": Queue PM command failed. Bad LBA "
                     "\n");
            break;
        case MV_QUEUE_COMMAND_RESULT_QUEUED_MODE_DISABLED:
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, ": Queue PM command failed. EDMA"
                     " disabled adapter %d channel %d\n",
                     pSataAdapter->adapterId, channelIndex);
            break;
        case MV_QUEUE_COMMAND_RESULT_FULL:
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, ": Queue PM command failed. Queue is"
                     " Full adapter %d channel %d\n",
                     pSataAdapter->adapterId, channelIndex);

            break;
        case MV_QUEUE_COMMAND_RESULT_BAD_PARAMS:
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, ": Queue PM command failed. (Bad "
                     "Params), pMvSataAdapter: %p, pSataChannel: %p.\n",
                     pSataAdapter, pSataAdapter->sataChannel[channelIndex]);
            break;
        default:
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, ": Queue PM command bad result value (%d) "
                     "from queue command\n",
                     result);
        }
        ialExt->IALChannelExt[channelIndex].pmRegAccessInProgress = MV_FALSE;
        return MV_FALSE;
    }
    return MV_TRUE;
}

#ifdef SYNO_SATA_PM_DEVICE_GPIO
static MV_BOOLEAN SynoMVPMCommandCompletionCB(MV_SATA_ADAPTER *pSataAdapter,
                                          MV_U8 channelIndex,
                                          MV_COMPLETION_TYPE comp_type,
                                          MV_VOID_PTR commandId,
                                          MV_U16 responseFlags,
                                          MV_U32 timeStamp,
                                          MV_STORAGE_DEVICE_REGISTERS *registerStruct,
                                          SynoCommandExt *pSynoCmdExt)
{
    struct completion *waiting = NULL;
    MV_BOOLEAN ret = MV_FALSE;

    if (!pSynoCmdExt || !pSynoCmdExt->private_data) {
        DEBUGMSG("[%d %d]: !pSynoCmdExt || !pSynoCmdExt->private_data)\n",
                 pSataAdapter->adapterId, channelIndex);
        goto END;
    }

    waiting = pSynoCmdExt->private_data;

    switch (comp_type)
    {
    case MV_COMPLETION_TYPE_NORMAL:
        set_bit(SYNO_PM_REQ_SUCCESS, &(pSynoCmdExt->flags));
        pSynoCmdExt->result_tf.errorRegister = registerStruct->errorRegister;
        pSynoCmdExt->result_tf.featuresRegister = registerStruct->featuresRegister;
        pSynoCmdExt->result_tf.commandRegister = registerStruct->commandRegister;
        pSynoCmdExt->result_tf.sectorCountRegister = registerStruct->sectorCountRegister;
        pSynoCmdExt->result_tf.lbaLowRegister = registerStruct->lbaLowRegister;
        pSynoCmdExt->result_tf.lbaMidRegister = registerStruct->lbaMidRegister;
        pSynoCmdExt->result_tf.lbaHighRegister = registerStruct->lbaHighRegister;
        pSynoCmdExt->result_tf.deviceRegister = registerStruct->deviceRegister;
        pSynoCmdExt->result_tf.statusRegister = registerStruct->statusRegister;
        break;
    case MV_COMPLETION_TYPE_ABORT:
        DEBUGMSG("[%d %d]: read PM register aborted!\n",
                 pSataAdapter->adapterId, channelIndex);
        set_bit(SYNO_PM_REQ_FAIL, &(pSynoCmdExt->flags));
        break;
    case MV_COMPLETION_TYPE_ERROR:
        DEBUGMSG("[%d %d]: read PM register error!\n",
                 pSataAdapter->adapterId, channelIndex);
        set_bit(SYNO_PM_REQ_FAIL, &(pSynoCmdExt->flags));
        break;
    default:
        DEBUGMSG("[%d %d]: Unknown completion type (%d)\n",
                 pSataAdapter->adapterId, channelIndex, comp_type);
        set_bit(SYNO_PM_REQ_FAIL, &(pSynoCmdExt->flags));
        break;
    }

    complete(waiting);
    ret = MV_TRUE;
END:
    return ret;
}

/**
 * This is compatible call back function to mvSata error handling, we don't do anything .
 * Just let orginal command timeout
 * 
 * @param pSataAdapter
 * @param channelIndex
 * @param comp_type
 * @param commandId
 * @param responseFlags
 * @param timeStamp
 * @param registerStruct
 * 
 * @return 
 */
static MV_BOOLEAN SynoMVPMCommandAbnormalCompletionCB(MV_SATA_ADAPTER *pSataAdapter,
                                          MV_U8 channelIndex,
                                          MV_COMPLETION_TYPE comp_type,
                                          MV_VOID_PTR commandId,
                                          MV_U16 responseFlags,
                                          MV_U32 timeStamp,
                                          MV_STORAGE_DEVICE_REGISTERS *registerStruct)
{    
    DEBUGMSG("%s Command fail just let it timeout we don't do anything even complete\n", __FUNCTION__);
    return MV_FALSE;
}

static MV_BOOLEAN SynoMVQueuePMAccessRegisterCommand(
                                                MV_IAL_COMMON_ADAPTER_EXTENSION* ialExt,
                                                MV_U8 channelIndex,
                                                MV_U8 PMPort,
                                                MV_U8 PMReg,
                                                MV_U32 WriteValue,
                                                MV_BOOLEAN isRead,
                                                MV_U32 *result_val)
{
    MV_QUEUE_COMMAND_RESULT result;
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    MV_QUEUE_COMMAND_INFO commandInfo, *pCommandInfo;
    SynoCommandExt synoCmdExt;
    MV_BOOLEAN ret = MV_FALSE;
    DECLARE_COMPLETION_ONSTACK(wait);

    if (CHANNEL_READY != ialExt->channelState[channelIndex]) {
        DEBUGMSG("%s: CHANNEL_READY != ialExt->channelState[channelIndex] \n", __FUNCTION__);
        goto END;
    }

    pCommandInfo = &commandInfo;    
    
    memset(pCommandInfo, 0, sizeof(MV_QUEUE_COMMAND_INFO));
    memset(&synoCmdExt, 0, sizeof(synoCmdExt));
    synoCmdExt.private_data = &wait;
    pCommandInfo->type = MV_QUEUED_COMMAND_TYPE_NONE_UDMA;
    pCommandInfo->commandParams.NoneUdmaCommand.protocolType = MV_NON_UDMA_PROTOCOL_NON_DATA;
    pCommandInfo->commandParams.NoneUdmaCommand.isEXT = MV_TRUE;
    pCommandInfo->commandParams.NoneUdmaCommand.bufPtr = NULL;
    pCommandInfo->PMPort = PMPort;
    pCommandInfo->commandParams.NoneUdmaCommand.count = 0;
    pCommandInfo->commandParams.NoneUdmaCommand.features = PMReg;
    pCommandInfo->commandParams.NoneUdmaCommand.device = (MV_U8)PMPort;
    pCommandInfo->commandParams.NoneUdmaCommand.callBack = SynoMVPMCommandAbnormalCompletionCB;
    pCommandInfo->commandParams.NoneUdmaCommand.SynoExtCallBack = SynoMVPMCommandCompletionCB;
    pCommandInfo->commandParams.NoneUdmaCommand.commandId = NULL;
    pCommandInfo->pSynoCmdExt = &synoCmdExt;

    if (isRead == MV_TRUE)
    {
        pCommandInfo->commandParams.NoneUdmaCommand.command =
        MV_ATA_COMMAND_PM_READ_REG;        
        pCommandInfo->commandParams.NoneUdmaCommand.sectorCount = 0;
        pCommandInfo->commandParams.NoneUdmaCommand.lbaLow = 0;
        pCommandInfo->commandParams.NoneUdmaCommand.lbaMid = 0;
        pCommandInfo->commandParams.NoneUdmaCommand.lbaHigh = 0;        
    }
    else
    {
        pCommandInfo->commandParams.NoneUdmaCommand.command =
        MV_ATA_COMMAND_PM_WRITE_REG;
        pCommandInfo->commandParams.NoneUdmaCommand.sectorCount =
        (MV_U16)((WriteValue) & 0xff),
        pCommandInfo->commandParams.NoneUdmaCommand.lbaLow =
        (MV_U16)(((WriteValue) & 0xff00) >> 8);
        pCommandInfo->commandParams.NoneUdmaCommand.lbaMid =
        (MV_U16)(((WriteValue) & 0xff0000) >> 16);
        pCommandInfo->commandParams.NoneUdmaCommand.lbaHigh =
        (MV_U16)(((WriteValue) & 0xff000000) >> 24);        
    }    

    result = mvSataQueueCommand(pSataAdapter,
                                channelIndex,
                                pCommandInfo);

    if (result != MV_QUEUE_COMMAND_RESULT_OK)
    {
        switch (result)
        {
        case MV_QUEUE_COMMAND_RESULT_BAD_LBA_ADDRESS:
            DEBUGMSG(": Queue PM command failed. Bad LBA \n");
            break;
        case MV_QUEUE_COMMAND_RESULT_QUEUED_MODE_DISABLED:
            DEBUGMSG(": Queue PM command failed. EDMA"
                     " disabled adapter %d channel %d\n",
                     pSataAdapter->adapterId, channelIndex);
            break;
        case MV_QUEUE_COMMAND_RESULT_FULL:
            DEBUGMSG(": Queue PM command failed. Queue is"
                     " Full adapter %d channel %d\n",
                     pSataAdapter->adapterId, channelIndex);

            break;
        case MV_QUEUE_COMMAND_RESULT_BAD_PARAMS:
            DEBUGMSG(": Queue PM command failed. (Bad "
                     "Params), pMvSataAdapter: %p, pSataChannel: %p.\n",
                     pSataAdapter, pSataAdapter->sataChannel[channelIndex]);
            break;
        default:
            DEBUGMSG(": Queue PM command bad result value (%d) "
                     "from queue command\n",
                     result);
        }
        goto END;
    }

    if (!wait_for_completion_timeout(&wait, msecs_to_jiffies(500))) {
        SynoSataPMGPIOQueueCommandTimeout(pSataAdapter,
                                          channelIndex,
                                          pCommandInfo);
        DEBUGMSG("%s: wait fail ...\n", __FUNCTION__);
        goto END;
    }

    if (test_bit(SYNO_PM_REQ_FAIL, &(synoCmdExt.flags))) {
        DEBUGMSG("%s: wait success but command fail ...\n", __FUNCTION__);
        goto END;
    }

    if (MV_TRUE == isRead) {
        *result_val = synoCmdExt.result_tf.sectorCountRegister;
        *result_val |= (synoCmdExt.result_tf.lbaLowRegister << 8);
        *result_val |= (synoCmdExt.result_tf.lbaMidRegister << 16);
        *result_val |= (synoCmdExt.result_tf.lbaHighRegister << 24);
    }

    ret = MV_TRUE;
END:
    return ret;
}
#endif

static MV_BOOLEAN mvPMEnableCommStatusChangeBits(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                                 MV_U8 channelIndex,
                                                 MV_BOOLEAN enable)
{
    MV_U32 regVal;
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;

    if (enable == MV_TRUE)
    {
        regVal = MV_BIT16;
    }
    else
    {
        regVal = 0;
    }

    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: Set PM "
             "GSCR[33] register to 0x%X\n",
             pSataAdapter->adapterId, channelIndex, regVal);
    /*Set N bit reflection in PM GSCR*/
    if (mvPMDevWriteReg(pSataAdapter, channelIndex,
                        MV_SATA_PM_CONTROL_PORT,
                        MV_SATA_GSCR_ERROR_ENABLE_REG_NUM,
                        regVal, NULL) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Failed to set "
                 "PortMultiplier Features Enable register\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    return MV_TRUE;
}

static MV_BOOLEAN mvPMEnableAsyncNotify(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_U8 channelIndex)
{
    MV_U32 regVal1, regVal2;
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    /*Features register*/
    if (mvPMDevReadReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                       MV_SATA_GSCR_FEATURES_REG_NUM,
                       &regVal1, NULL) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Failed to get Port Multiplier Features"
                 " supported register\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: Port Multiplier features supported: 0x%X\n",
             pSataAdapter->adapterId, channelIndex, regVal1);

    /*PM asynchronous notification supported*/
    if (mvPMDevReadReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                       MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM,
                       &regVal2 ,NULL) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Failed to get Port Multiplier Features"
                 " register\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;

    }

    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: Port Multiplier features enabled "
             "register: 0x%X\n",
             pSataAdapter->adapterId, channelIndex, regVal2);
    if (regVal1 & MV_BIT3)
    {
        regVal2 |= MV_BIT3;
        if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                            MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM,
                            regVal2 ,NULL) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Failed to set "
                     "PortMultiplier Features Enable register\n",
                     pSataAdapter->adapterId, channelIndex);
            return MV_FALSE;

        }
        ialExt->IALChannelExt[channelIndex].pmAsyncNotifyEnabled = MV_TRUE;
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: PM asynchronous notification is "
                 "enabled.\n", pSataAdapter->adapterId, channelIndex);
    }
    else
    {
        regVal2 &= ~MV_BIT3;
        if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                            MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM,
                            regVal2 ,NULL) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Failed to set "
                     "PortMultiplier Features Enable register\n",
                     pSataAdapter->adapterId, channelIndex);
            return MV_FALSE;

        }
        ialExt->IALChannelExt[channelIndex].pmAsyncNotifyEnabled = MV_FALSE;
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: PM asynchronous notification is "
                 "disabled.\n", pSataAdapter->adapterId, channelIndex);
    }

    return MV_TRUE;
}

static MV_BOOLEAN mvPMDisableAsyncNotify(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_U8 channelIndex)
{
    MV_U32 regVal2;
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;


    /*PM asynchronous notification supported*/
    if (mvPMDevReadReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                       MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM,
                       &regVal2 ,NULL) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Failed to get Port Multiplier Features"
                 " register\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;

    }

    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: Port Multiplier features enabled "
             "register: 0x%X\n",
             pSataAdapter->adapterId, channelIndex, regVal2);

        regVal2 &= ~MV_BIT3;
        if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                            MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM,
                            regVal2 ,NULL) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Failed to set "
                     "PortMultiplier Features Enable register\n",
                     pSataAdapter->adapterId, channelIndex);
            return MV_FALSE;

        }
        ialExt->IALChannelExt[channelIndex].pmAsyncNotifyEnabled = MV_FALSE;
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: PM asynchronous notification is "
                 "disabled.\n", pSataAdapter->adapterId, channelIndex);

    return MV_TRUE;
}




#ifdef SYNO_SATA_PM_DEVICE_GPIO
static inline void syno_prepare_custom_info(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt,
                                            MV_U8 channelIndex,
                                            MV_SATA_PM_DEVICE_INFO *PMInfo)
{
    SYNO_PM_PKG pm_pkg;
    u8 old_venderID = pIALExt->pSataAdapter->sataChannel[channelIndex]->PMvendorId;
    u8 old_deviceID = pIALExt->pSataAdapter->sataChannel[channelIndex]->PMdeviceId;

    /* must set it first. Because some function need these two val */
    pIALExt->pSataAdapter->sataChannel[channelIndex]->PMvendorId = PMInfo->vendorId;
    pIALExt->pSataAdapter->sataChannel[channelIndex]->PMdeviceId = PMInfo->deviceId;


    syno_pm_unique_pkg_init(PMInfo->vendorId,
                            PMInfo->deviceId,
                            &pm_pkg);

    syno_mvSata_pmp_write_gpio(pIALExt, channelIndex, &pm_pkg);
    syno_mvSata_pmp_read_gpio(pIALExt, channelIndex, &pm_pkg);

    pIALExt->pSataAdapter->sataChannel[channelIndex]->PMSynoUnique = pm_pkg.var;

    syno_mvSata_pm_power_ctl(pIALExt,
                             channelIndex,
                             &pm_pkg, MV_TRUE,
                             (!old_venderID && !old_deviceID) ? MV_FALSE : MV_TRUE);
}

void
syno_mvSata_pm_power_ctl(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt,
                         MV_U8 channelIndex,
                         SYNO_PM_PKG *pPKG, 
                         MV_U8 blPowerOn,
                         MV_U8 blHotplug)
{
    SYNO_PM_PKG pm_pkg;
    MV_U16 venderID = pIALExt->pSataAdapter->sataChannel[channelIndex]->PMvendorId;
    MV_U16 deviceID = pIALExt->pSataAdapter->sataChannel[channelIndex]->PMdeviceId;

    /* Ensure we don't open power while user turn off expansion box manually */
    if (blHotplug) {
        return;
    }

    if (pPKG) {
        pm_pkg = *pPKG;
    } else {
        syno_pm_unique_pkg_init(venderID, deviceID, &pm_pkg);
        syno_mvSata_pmp_write_gpio(pIALExt, channelIndex, &pm_pkg);
        syno_mvSata_pmp_read_gpio(pIALExt, channelIndex, &pm_pkg);
    }
    
    if (blPowerOn ^ syno_pm_is_poweron(venderID, deviceID, &pm_pkg)) {
		syno_pm_poweron_pkg_init(venderID, deviceID, &pm_pkg, MV_FALSE);
		syno_mvSata_pmp_write_gpio(pIALExt, channelIndex, &pm_pkg);

        if (blPowerOn) {
            mdelay(5); /* don't do it too fast. Otherwise CPLD might not response */
        } else {
            mdelay(7000); /* hardware spec */
        }

		syno_pm_poweron_pkg_init(venderID, deviceID, &pm_pkg, MV_TRUE);
		syno_mvSata_pmp_write_gpio(pIALExt, channelIndex, &pm_pkg);
	}
}
#endif
static MV_BOOLEAN mvConfigurePMDevice(
                                     MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                     MV_U8 channelIndex)
{
    MV_SATA_PM_DEVICE_INFO PMInfo;
    ialExt->IALChannelExt[channelIndex].pmAsyncNotifyEnabled = MV_FALSE;

    if (mvGetPMDeviceInfo(ialExt->pSataAdapter, channelIndex, &PMInfo)
        == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: Failed to get PortMultiplier Info\n",
                 ialExt->pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d]: PM of %d ports found\n",
             ialExt->pSataAdapter->adapterId, channelIndex,
             PMInfo.numberOfPorts);
    ialExt->IALChannelExt[channelIndex].PMnumberOfPorts = PMInfo.numberOfPorts;

#ifdef SYNO_SATA_PM_DEVICE_GPIO
    syno_prepare_custom_info(ialExt, channelIndex, &PMInfo);
    if (MV_FALSE == syno_mvSata_is_synology_pm(ialExt, channelIndex)) {
        ialExt->IALChannelExt[channelIndex].PMnumberOfPorts = 1;
    }    
#endif

#ifdef DISABLE_PM_SCC
    if (PMInfo.vendorId == 0x11AB)
    {

        if (mvPMDisableSSC(ialExt->pSataAdapter, channelIndex) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: cannot disable SSC for PM.\n"
                     "unknown vendor.\n",
                     ialExt->pSataAdapter->adapterId, channelIndex);
        }
    }
    else
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: cannot disable SSC for PM - "
                 "unknown vendor.\n",
                 ialExt->pSataAdapter->adapterId, channelIndex);
    }
#endif
#if 1
    if (clearSErrorPorts(ialExt->pSataAdapter, channelIndex,
			 ialExt->IALChannelExt[channelIndex].PMnumberOfPorts) != 
	MV_TRUE)
    {
	    return MV_FALSE;
    }
#endif
    if (mvPMEnableCommStatusChangeBits(ialExt,
                                       channelIndex,
                                       MV_FALSE) != MV_TRUE)
    {
        return MV_FALSE;
    }
    if (mvPMDisableAsyncNotify(ialExt, channelIndex) == MV_FALSE)
    {
        return MV_FALSE;
    }
#if 0

    if (mvPMDevEnableStaggeredSpinUpAll(ialExt->pSataAdapter,
                                        channelIndex,
                                        ialExt->IALChannelExt[channelIndex].PMnumberOfPorts,
                                        &ialExt->IALChannelExt[channelIndex].PMdevsToInit) == MV_FALSE)
    {
	 mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: PM Enable Staggered Spin-\
Up Failed\n",
		  ialExt->pSataAdapter->adapterId, channelIndex);
	 ialExt->IALChannelExt[channelIndex].PMdevsToInit = 0;
	 return MV_FALSE;
    }
#endif
    return MV_TRUE;
}

static MV_BOOLEAN clearSErrorPorts(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex, 
				   MV_U8     PMnumberOfPorts)
{
	MV_U8 PMPort;
	
	for (PMPort = 0; PMPort < PMnumberOfPorts; PMPort++)
	{
		if (mvPMDevWriteReg(pSataAdapter, channelIndex, PMPort,
				    MV_SATA_PSCR_SERROR_REG_NUM, 0xFFFFFFFF, NULL) ==
		    MV_FALSE)
		{
			if (mvStorageDevATASoftResetDevice(pSataAdapter, channelIndex,
							   MV_SATA_PM_CONTROL_PORT, NULL) == MV_FALSE)
			{
				mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: "
					 "failed to Soft Reset PM control port\n",
					 pSataAdapter->adapterId, channelIndex);
				return MV_FALSE;
			}
		}
	}
	return MV_TRUE;
}

MV_BOOLEAN mvPMEnableLocking(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex)
{
    MV_STORAGE_DEVICE_REGISTERS regs;
    if (mvPMDevWriteReg(pSataAdapter, channelIndex,
                        MV_SATA_PM_CONTROL_PORT,
                        0x89,
                        0x8000003F,
                        &regs) == MV_TRUE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: NCQ lock is enabled for PM.\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_TRUE;
    }
    else
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: cannot enable NCQ lock for PM.\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    return MV_TRUE;
}

#ifdef DISABLE_PM_SCC
MV_BOOLEAN mvPMDisableSSC(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex)
{
    MV_STORAGE_DEVICE_REGISTERS regs;
    MV_U32 regVal = 0;

    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: "
             "Disable SSC for all PM ports.\n",
             pSataAdapter->adapterId, channelIndex);

    if (mvPMDevReadReg(pSataAdapter,channelIndex, MV_SATA_PM_CONTROL_PORT,
                       MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM,
                       &regVal,
                       &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }

    /*Host SSC disable*/
    regVal &= ~MV_BIT2;
    if (mvPMDevWriteReg(pSataAdapter,channelIndex, MV_SATA_PM_CONTROL_PORT,
                        MV_SATA_GSCR_FEATURES_ENABLE_REG_NUM,
                        regVal,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }

    /* disable ssc for port 0*/
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x8C,
                        0,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x92,
                        0xb02a402a,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }
    /* disable ssc for port 1*/
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x8C,
                        1,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x92,
                        0xb02a402a,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }
    /* disable ssc for port 2*/
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x8C,
                        2,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x92,
                        0xb02a402a,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }

    /* disable ssc for port 3*/
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x8C,
                        3,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x92,
                        0xb02a402a,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }

    /* disable ssc for port 15*/
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x8C,
                        MV_SATA_PM_CONTROL_PORT,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, MV_SATA_PM_CONTROL_PORT,
                        0x92,
                        0xb02a402a,
                        &regs) != MV_TRUE)
    {
        return MV_FALSE;
    }
    return MV_TRUE;
}
#endif

static MV_BOOLEAN mvConfigChannelQueuingMode(MV_IAL_COMMON_ADAPTER_EXTENSION* ialExt,
                                             MV_U8 channelIndex,
                                             MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_SATA_ADAPTER         *pSataAdapter = ialExt->pSataAdapter;
    MV_EDMA_MODE            EDMAMode = MV_EDMA_MODE_NOT_QUEUED;
    MV_SATA_SWITCHING_MODE  switchingMode;
    MV_BOOLEAN              isTCQSupported = MV_FALSE;
    MV_BOOLEAN              isNCQSupported = MV_FALSE;
    MV_U8                   numOfDrives = 0;
    MV_SATA_PM_DEVICE_INFO  PMInfo;
    MV_SATA_DEVICE_TYPE     connectedDevice;
    MV_BOOLEAN              use128Entries = MV_FALSE;


    if (mvGetDisksModes(ialExt,
                        scsiAdapterExt,
                        channelIndex,
                        &isTCQSupported,
                        &isNCQSupported,
                        &numOfDrives) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,
                 "[%d %d]:mvConfigChannelQueuingMode: failed to get disks modes.\n"
                 ,pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    else
    {

        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: Supported queuing mode: TCQ = %s, "
                 "NCQ = %s. number of disks %d\n",
                 pSataAdapter->adapterId, channelIndex,
                 (isTCQSupported == MV_TRUE) ? "Yes" : "No",
                 (isNCQSupported == MV_TRUE) ? "Yes" : "No",
                 numOfDrives);
    }
    connectedDevice = mvStorageDevGetDeviceType(pSataAdapter,channelIndex);
    if (connectedDevice == MV_SATA_DEVICE_TYPE_UNKNOWN)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,
                 "[%d %d] mvConfigChannelQueuingMode: failed to get device type.\n"
                 ,pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    if (connectedDevice == MV_SATA_DEVICE_TYPE_PM)
    {
        if (mvGetPMDeviceInfo(ialExt->pSataAdapter, channelIndex, &PMInfo)
            == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,
                     "[%d %d] mvConfigChannelQueuingMode: Failed to get "
                     "PortMultiplier Info\n",
                     ialExt->pSataAdapter->adapterId, channelIndex);
            return MV_FALSE;
        }
    }
    mvSelectConfiguration(pSataAdapter, channelIndex,
                          pSataAdapter->sataAdapterGeneration,
                          connectedDevice,&PMInfo,
                          isNCQSupported,
                          isTCQSupported,
                          numOfDrives,
                          &EDMAMode,
                          &switchingMode,
                          &use128Entries);

#ifndef MV_SATA_SUPPORT_GEN2E_128_QUEUE_LEN
    use128Entries = MV_FALSE;
#endif
    switch (switchingMode)
    {
    case MV_SATA_SWITCHING_MODE_FBS:
        {
            if (mvSataSetFBSMode(pSataAdapter, channelIndex, MV_TRUE,
                                 use128Entries) == MV_FALSE)
            {
                mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,
                         "[%d %d] mvConfigChannelQueuingMode: failed to enable FBS.\n"
                         ,pSataAdapter->adapterId, channelIndex);
                return MV_FALSE;
            }
        }
        break;
    case MV_SATA_SWITCHING_MODE_QCBS:
        if (mvPMEnableLocking(pSataAdapter,channelIndex) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,
                     "[%d %d] mvConfigChannelQueuingMode: failed to enable QCBS.\n"
                     ,pSataAdapter->adapterId, channelIndex);
            return MV_FALSE;
        }
        break;
    default:
        break;
    }         
    IALConfigQueuingMode(pSataAdapter,
                         channelIndex,
                         EDMAMode,
                         switchingMode,
                         use128Entries);
    return MV_TRUE;
}



/*Channel related functions*/

static void mvSetChannelState(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                              MV_U8 channelIndex,
                              MV_CHANNEL_STATE state)
{
    if (ialExt->channelState[channelIndex] != state)
    {
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
#ifdef MY_ABC_HERE
        MV_U32 old_eh_flags = ialExt->pSataAdapter->eh[channelIndex].flags;
#endif

        SynoClearEHInfo(ialExt, channelIndex, state);
#endif
        if ((state == CHANNEL_READY) || (state == CHANNEL_NOT_CONNECTED))
        {
            ialExt->IALChannelExt[channelIndex].SRSTTimerThreshold = 0;
            ialExt->IALChannelExt[channelIndex].SRSTTimerValue = 0;
        }
        if (state == CHANNEL_READY)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: CHANNEL_READY\n",
                     ialExt->pSataAdapter->adapterId,
                     channelIndex);
            ialExt->IALChannelExt[channelIndex].pmRegAccessInProgress
            = MV_FALSE;
            ialExt->IALChannelExt[channelIndex].completionError = MV_FALSE;
            ialExt->channelState[channelIndex] = state;
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d] flush pending queue\n",
                     ialExt->pSataAdapter->adapterId, channelIndex);
            /*Abort all pending commands in SW queue*/
            mvFlushSCSICommandQueue(ialExt, channelIndex);
            if (MV_TRUE == ialExt->IALChannelExt[channelIndex].bHotPlug)
            {
                MV_U16 drivesToRemove;
                MV_U16 drivesToAdd;
                ialExt->IALChannelExt[channelIndex].bHotPlug = MV_FALSE;
                mvDrivesInfoGetChannelRescanParams(ialExt,
                                                   channelIndex,
                                                   &drivesToRemove,
                                                   &drivesToAdd);
                if (drivesToRemove != 0 || drivesToAdd != 0)
                {

#ifdef MY_ABC_HERE
                    IALBusChangeNotifyEx(ialExt->pSataAdapter, channelIndex, drivesToRemove, drivesToAdd, old_eh_flags);
#else
                    IALBusChangeNotifyEx(ialExt->pSataAdapter,
                                         channelIndex,
                                         drivesToRemove,
                                         drivesToAdd);
#endif
                }
            }
            mvDrivesInfoSaveAll(ialExt, channelIndex);
        }
        else
        {
            ialExt->channelState[channelIndex] = state;
        }
    }
}


static MV_BOOLEAN mvStartChannelInit(MV_SATA_ADAPTER *pSataAdapter,
                                     MV_U8 channelIndex,
                                     MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                                     MV_BOOLEAN* bIsChannelReady)
{
    *bIsChannelReady = MV_FALSE;

    if (mvSataConfigureChannel(pSataAdapter, channelIndex) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: configure channel failed\n",
                 pSataAdapter->adapterId,
                 channelIndex);
        return MV_FALSE;
    }

    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: start channel\n",
             pSataAdapter->adapterId,
             channelIndex);
    /*Just check SStatus in case of SATA I adapter*/
    if (pSataAdapter->sataAdapterGeneration == MV_SATA_GEN_I)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: starting SATA I channel.\n",
                 pSataAdapter->adapterId, channelIndex);
    }
    else
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: starting SATA II channel.\n",
                 pSataAdapter->adapterId, channelIndex);
    }

    return mvStorageDevATAStartSoftResetDevice(pSataAdapter,
                                               channelIndex,
                                               MV_SATA_PM_CONTROL_PORT);
}

static MV_BOOLEAN mvChannelSRSTFinished(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_SATA_CHANNEL *pSataChannel,
                                        MV_U8 channelIndex,
                                        MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                                        MV_BOOLEAN* bIsChannelReady,
                                        MV_BOOLEAN* bFatalError)
{
    MV_SATA_DEVICE_TYPE deviceType;
    MV_STORAGE_DEVICE_REGISTERS mvStorageDevRegisters;
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    MV_SATA_SCSI_DRIVE_DATA *pDriveData;
    *bIsChannelReady = MV_FALSE;
    *bFatalError = MV_FALSE;
    if (pSataAdapter->sataAdapterGeneration > MV_SATA_GEN_I)
    {
        if (mvStorageIsDeviceBsyBitOff(pSataAdapter,
                                       channelIndex,
                                       &mvStorageDevRegisters)
            == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: soft Reset PM control port "
                     "in progress\n",
                     pSataAdapter->adapterId, channelIndex);
            printAtaDeviceRegisters(&mvStorageDevRegisters);
            return MV_FALSE;
        }
        deviceType = mvGetSataDeviceType(&mvStorageDevRegisters);
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: soft reset SATA II channel - "
                 "device ready.\n",
                 pSataAdapter->adapterId, channelIndex);
    }
    else
    {
        if (mvStorageIsDeviceBsyBitOff(pSataAdapter,
                                       channelIndex,
                                       &mvStorageDevRegisters)
            == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: soft reset of SATA I channel "
                     "in progress\n",
                     pSataAdapter->adapterId, channelIndex);
            printAtaDeviceRegisters(&mvStorageDevRegisters);
            return MV_FALSE;
        }
        deviceType = mvGetSataDeviceType(&mvStorageDevRegisters);
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: soft reset SATA I channel - "
                 "device ready.\n",
                 pSataAdapter->adapterId, channelIndex);
        deviceType = mvGetSataDeviceType(&mvStorageDevRegisters);
        if (deviceType != MV_SATA_DEVICE_TYPE_ATA_DISK)
        {
            deviceType = MV_SATA_DEVICE_TYPE_UNKNOWN;
        }

    }
#ifdef MY_ABC_HERE
    if (MV_FALSE == blSynoEHTypeRevalidate(ialExt, scsiAdapterExt, pSataAdapter, channelIndex, deviceType)) {
        return MV_FALSE;
    }
#endif
    switch (deviceType)
    {
    case MV_SATA_DEVICE_TYPE_ATA_DISK:
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO,"[%d %d]: SATA disk found\n",
                 pSataAdapter->adapterId, channelIndex);
        if (mvStorageDevSetDeviceType(pSataAdapter,channelIndex, deviceType) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: Failed to initialize disk\n",
                     pSataAdapter->adapterId, channelIndex);
            *bFatalError = MV_TRUE;
            return MV_FALSE;

        }
        pDriveData = &scsiAdapterExt->ataDriveData[channelIndex][0];
        if (mvInitSataDisk(pSataAdapter,
                           channelIndex,
                           0,
                           &pDriveData->identifyInfo,
                           pDriveData->identifyBuffer) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: Failed to initialize disk\n",
                     pSataAdapter->adapterId, channelIndex);
            *bFatalError = MV_TRUE;
            mvDrivesInfoFlushSingleDrive(ialExt,
                                         channelIndex,
                                         0);
            return MV_FALSE;
        }
#ifdef MY_ABC_HERE
        if (MV_FALSE == blSynoEHDiskRevalidate(ialExt, 
                                               scsiAdapterExt, 
                                               pSataAdapter, 
                                               channelIndex,
                                               0,
                                               pDriveData->identifyBuffer)) {
            return MV_FALSE;
        }
#endif
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO,"[%d %d]: Disk ready\n",
                 pSataAdapter->adapterId, channelIndex);
        mvSetDriveReady(ialExt,
                        scsiAdapterExt,
                        channelIndex,
                        0,
                        MV_TRUE,
                        pDriveData->identifyBuffer);
        mvSataScsiNotifyUA(scsiAdapterExt, channelIndex, 0);
        *bIsChannelReady = MV_TRUE;
        return MV_TRUE;
        break;
    case MV_SATA_DEVICE_TYPE_PM:
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO,"[%d %d]: PortMultiplier device found\n",
                 pSataAdapter->adapterId, channelIndex);
        if (mvStorageDevSetDeviceType(pSataAdapter,channelIndex, deviceType) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: Failed to initialize PM\n",
                     pSataAdapter->adapterId, channelIndex);
            *bFatalError = MV_TRUE;
            return MV_FALSE;

        }
        break;
#ifdef MV_SUPPORT_ATAPI
    case MV_SATA_DEVICE_TYPE_ATAPI_DEVICE:
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO,"[%d %d]: ATAPI device found\n",
                 pSataAdapter->adapterId, channelIndex);
        if (mvStorageDevSetDeviceType(pSataAdapter,channelIndex, deviceType) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: Failed to initialize ATAPI device\n",
                     pSataAdapter->adapterId, channelIndex);
            *bFatalError = MV_TRUE;
            return MV_FALSE;

        }
        pDriveData = &scsiAdapterExt->ataDriveData[channelIndex][0];
        if (mvInitSataATAPI(pSataAdapter,
                           channelIndex,
                           0,
                           &pDriveData->identifyInfo,
                           pDriveData->identifyBuffer) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: Failed to initialize ATAPI device\n",
                     pSataAdapter->adapterId, channelIndex);
            *bFatalError = MV_TRUE;
            mvDrivesInfoFlushSingleDrive(ialExt,
                                         channelIndex,
                                         0);
            return MV_FALSE;
        }
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO,"[%d %d]: ATAPI Device ready\n",
                 pSataAdapter->adapterId, channelIndex);

        mvSetDriveReady(ialExt,
                        scsiAdapterExt,
                        channelIndex,
                        0,
                        MV_TRUE,
                        pDriveData->identifyBuffer);


        mvSataScsiNotifyUA(scsiAdapterExt, channelIndex, 0);
        *bIsChannelReady = MV_TRUE;
        return MV_TRUE;
         break;
#endif
    default:
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: ERROR: unknown device type\n",
                 pSataAdapter->adapterId, channelIndex);
        *bFatalError    =    MV_TRUE;
        return MV_FALSE;
    }
    return MV_TRUE;
}



static MV_BOOLEAN mvConfigChannelDMA(
                                    MV_IAL_COMMON_ADAPTER_EXTENSION* ialExt,
                                    MV_U8 channelIndex,
                                    MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d] config queueing mode\n",
             pSataAdapter->adapterId, channelIndex);


    if (mvConfigChannelQueuingMode(ialExt,
                                   channelIndex,
                                   scsiAdapterExt)
        == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d] Failed to config DMA queuing\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    /* Enable EDMA */
    if (mvSataEnableChannelDma(pSataAdapter, channelIndex) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d] Failed to enable DMA, channel=%d\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d]: channel started successfully\n",
             pSataAdapter->adapterId, channelIndex);
    return MV_TRUE;
}





static void mvSetChannelTimer(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                              MV_U8 channelIndex,
                              MV_U32 timeout)
{
    ialExt->IALChannelExt[channelIndex].SRSTTimerThreshold = timeout;
    ialExt->IALChannelExt[channelIndex].SRSTTimerValue = 1;
}

static void mvDecrementChannelTimer(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                    MV_U8 channelIndex)
{
    if (ialExt->IALChannelExt[channelIndex].SRSTTimerThreshold > 0)
    {
        ialExt->IALChannelExt[channelIndex].SRSTTimerValue +=
        MV_IAL_ASYNC_TIMER_PERIOD;
    }
}

static MV_BOOLEAN mvIsChannelTimerExpired(
                                         MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                         MV_U8 channelIndex)
{
    if (ialExt->IALChannelExt[channelIndex].SRSTTimerValue >
        ialExt->IALChannelExt[channelIndex].SRSTTimerThreshold)
    {
        return MV_TRUE;
    }
    else
    {
        return MV_FALSE;
    }
}

/*******************************************************************************
*State Machine related functions:
*  Return MV_TRUE to proceed to the next channel
*  Return MV_FALSE to proceed to the next state on current channel
*******************************************************************************/

static MV_BOOLEAN mvChannelNotConnectedStateHandler(
                                                   MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                                   MV_U8 channelIndex,
                                                   MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    if (pSataAdapter->sataChannel[channelIndex] != NULL)
    {
        mvSataRemoveChannel(pSataAdapter,channelIndex);
        pSataAdapter->sataChannel[channelIndex] = NULL;
        mvSetDriveReady(ialExt,
                        scsiAdapterExt,
                        channelIndex,
                        0xFF, MV_FALSE, NULL);
        mvFlushSCSICommandQueue(ialExt, channelIndex);
    }
    return MV_TRUE;
}


static MV_BOOLEAN mvChannelConnectedStateHandler(
     MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
     MV_U8 channelIndex,
     MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
     MV_BOOLEAN res = MV_FALSE;
     MV_BOOLEAN isChannelReady;
     MV_SATA_CHANNEL *pSataChannel;
     MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
     
     mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d] CHANNEL_CONNECTED\n",
	      ialExt->pSataAdapter->adapterId, channelIndex);
     if (pSataAdapter->sataChannel[channelIndex] == NULL)
    {
	 if (IALInitChannel(pSataAdapter, channelIndex) == MV_FALSE)
	 {
	      IALReleaseChannel(pSataAdapter, channelIndex);
	      mvDrivesInfoFlushAll(ialExt, channelIndex);
	      mvSetChannelState(ialExt, channelIndex, CHANNEL_NOT_CONNECTED);
	      return MV_TRUE;
	 }
    }
     pSataChannel = pSataAdapter->sataChannel[channelIndex];
     res = mvStartChannelInit(pSataAdapter,
			      channelIndex,
			      scsiAdapterExt,
			      &isChannelReady);
     if (res == MV_TRUE)
     {
	  if (isChannelReady == MV_FALSE)
	  {
	       /*SRST channel, Set polling timer*/
            mvSetChannelTimer(ialExt, channelIndex,
                              MV_IAL_SRST_TIMEOUT);
            mvSetChannelState(ialExt,
                              channelIndex,
                              CHANNEL_IN_SRST);
	  }
	  else
	  {
            if (mvConfigChannelDMA(ialExt,
                                   channelIndex,
                                   scsiAdapterExt) == MV_TRUE)
            {
		 mvSetChannelState(ialExt,
				   channelIndex,
				   CHANNEL_READY);
            }
            else
            {
                mvStopChannel(ialExt,
                              channelIndex,
                              scsiAdapterExt);
            }
	  }
    }
     else
     {
         mvStopChannel(ialExt,
                       channelIndex,
                       scsiAdapterExt);
     }
     return MV_TRUE;
}


#ifdef MY_ABC_HERE
extern MV_VOID handleDisconnect(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex);

static MV_BOOLEAN 
blSynoReprobeProcessStop(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,                   
                         MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                         MV_SATA_ADAPTER *pSataAdapter,
                         MV_U8 channelIndex)
{
    MV_BOOLEAN res = MV_TRUE;

    if (!ialExt ||
        !scsiAdapterExt ||
        !pSataAdapter) {
        goto END;
    }

#ifdef MY_ABC_HERE
    if (pSataAdapter->eh[channelIndex].flags & EH_PROCESSING) {
        if (MV_FALSE == mvSataIsStorageDeviceConnected(pSataAdapter, channelIndex, NULL)) {
            syno_eh_printk(pSataAdapter, channelIndex, "EH is on and device not here. Schedule EH");
            queue_delayed_work(mvSata_aux_wq, &(pSataAdapter->eh[channelIndex].work), 0);
            goto END;
        }
    }
#endif

    if (!test_and_set_bit(SYNO_PROBE_RETRY, &pSataAdapter->flags[channelIndex])) {
        printk("[%d %d]: Timeout. Retry connection \n", pSataAdapter->adapterId, channelIndex);
        handleDisconnect(pSataAdapter, channelIndex);
        mvRestartChannel(ialExt, channelIndex, scsiAdapterExt, MV_FALSE);
        res = MV_FALSE;
    } else if (!test_and_set_bit(SYNO_PROBE_LIMIT_TO_15, &pSataAdapter->flags[channelIndex])) {
        printk("[%d %d]: Limit to 1.5Gb and Retry\n", pSataAdapter->adapterId, channelIndex);
        mvSataSetInterfaceSpeed(pSataAdapter, channelIndex, MV_SATA_IF_SPEED_1_5_GBPS);
        handleDisconnect(pSataAdapter, channelIndex);
        mvRestartChannel(ialExt, channelIndex, scsiAdapterExt, MV_FALSE);
        res = MV_FALSE;
    } else {
        printk("[%d %d]: Give up the port after try all solutions\n", pSataAdapter->adapterId, channelIndex);
        SynomvStopChannel(ialExt, channelIndex, scsiAdapterExt);
    }    
END:
    return res;
}
#endif /* MY_ABC_HERE */
MV_BOOLEAN mvChannelInSrstStateHandler(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                       MV_U8 channelIndex,
                                       MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_BOOLEAN bFatalError;
    MV_BOOLEAN res = MV_FALSE;
    MV_BOOLEAN isChannelReady;
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    MV_SATA_CHANNEL *pSataChannel = pSataAdapter->sataChannel[channelIndex];
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d] CHANNEL_IN_SRST\n",
             pSataAdapter->adapterId, channelIndex);
    mvDecrementChannelTimer(ialExt, channelIndex);
    res = mvChannelSRSTFinished(ialExt,
                                pSataChannel,
                                channelIndex,
                                scsiAdapterExt,
                                &isChannelReady,
                                &bFatalError);
    if (res == MV_TRUE)
    {
        /*Finishing channel initialization*/
        if (isChannelReady == MV_TRUE)
        {
	     mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO,"[%d %d]: Sata device ready\n",
		      pSataAdapter->adapterId,channelIndex);
            if (mvConfigChannelDMA(ialExt,
                                   channelIndex,
                                   scsiAdapterExt) == MV_TRUE)
            {
                mvSetChannelState(ialExt,
                                  channelIndex,
                                  CHANNEL_READY);
            }
            else
            {
                mvStopChannel(ialExt,
                              channelIndex,
                              scsiAdapterExt);
            }
        }
        else
        {/*If channel not ready and function call succeed -> PM is found*/
            if (mvConfigurePMDevice(ialExt, channelIndex) == MV_FALSE)
            {
                mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: Failed to "
                         "initialize PM\n",
                         pSataAdapter->adapterId,channelIndex);
#if 0               
		mvStopChannel(ialExt,
                              channelIndex,
                              scsiAdapterExt);
#else
		mvSataChannelHardReset(pSataAdapter, channelIndex);
		mvRestartChannel(ialExt, channelIndex, scsiAdapterExt, MV_FALSE);
#endif
            }
            else
            {
		 MV_IAL_COMMON_CHANNEL_EXTENSION* channelExt = 
		      &ialExt->IALChannelExt[channelIndex];
		 
		 channelExt->port_state = MV_PORT_NOT_INITIALIZED;
		 channelExt->devInSRST = 0;
		 
                mvSetChannelState(ialExt, channelIndex,
                                  CHANNEL_PM_INIT_DEVICES);
                return MV_FALSE;
            }
        }
    }
    else
    {
#ifdef MY_ABC_HERE
        /* 
         * FIXME: Please use a new eh to handle reprobe. 
         * No matter what happend, retry probe until it really cannot be probed.
         */
        if (MV_TRUE == bFatalError) {
            /* Retry it with all possible method */
            return blSynoReprobeProcessStop(ialExt, scsiAdapterExt, pSataAdapter,channelIndex);
        } else {
            if (MV_TRUE == mvIsChannelTimerExpired(ialExt, channelIndex)) {
                return blSynoReprobeProcessStop(ialExt, scsiAdapterExt, pSataAdapter, channelIndex);
            }
        }
#else
        if (bFatalError == MV_TRUE)
        {
            mvStopChannel(ialExt,
                          channelIndex,
                          scsiAdapterExt);
        }
        else
        {
            if (mvIsChannelTimerExpired(ialExt, channelIndex) == MV_TRUE)
            {
                mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,
                         "[%d %d]: SW reset Failed, timer expired\n",
                         pSataAdapter->adapterId,channelIndex);
                mvStopChannel(ialExt,
                              channelIndex,
                              scsiAdapterExt);
            }
        }
#endif
    }
    return MV_TRUE;
}

MV_BOOLEAN classifyAndInitDevice(MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
				 MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
				 MV_U8 channelIndex, MV_U8 PMPort)
{
     
     MV_SATA_DEVICE_TYPE deviceType;
     MV_SATA_ADAPTER *pSataAdapter = scsiAdapterExt->pSataAdapter;
     MV_IAL_COMMON_CHANNEL_EXTENSION* channelExt = &ialExt->IALChannelExt[channelIndex];
     MV_SATA_SCSI_DRIVE_DATA *pDriveData = &scsiAdapterExt->ataDriveData[channelIndex][PMPort];
     MV_STORAGE_DEVICE_REGISTERS *mvStorageDevRegisters = &channelExt->mvStorageDevRegisters;

     deviceType = mvGetSataDeviceType(mvStorageDevRegisters);
     if (deviceType == MV_SATA_DEVICE_TYPE_ATA_DISK)
     {
	  if (mvInitSataDisk(pSataAdapter,
			     channelIndex ,
			     PMPort, &pDriveData->identifyInfo,
			     pDriveData->identifyBuffer) == MV_FALSE)
	  {
	       mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d %d]: Failed to initialize disk\n",
			pSataAdapter->adapterId, channelIndex, PMPort);
	       mvDrivesInfoFlushSingleDrive(ialExt, channelIndex, PMPort);
	  }
	  else
	  {
#ifdef MY_ABC_HERE
          if (MV_FALSE == blSynoEHDiskRevalidate(ialExt, 
                                                 scsiAdapterExt, 
                                                 pSataAdapter, 
                                                 channelIndex,
                                                 PMPort,
                                                 pDriveData->identifyBuffer)) {
              return MV_FALSE;
          }
#endif
	       mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO, "[%d %d %d]: Disk ready\n",
			pSataAdapter->adapterId, channelIndex, PMPort);
	       mvSetDriveReady(ialExt,
			       scsiAdapterExt,
			       channelIndex, PMPort,
			       MV_TRUE,
			       pDriveData->identifyBuffer);
	       mvSataScsiNotifyUA(scsiAdapterExt, channelIndex, PMPort);
	  }
     }
#ifdef MV_SUPPORT_ATAPI
     else if (deviceType == MV_SATA_DEVICE_TYPE_ATAPI_DEVICE)
     {
	  if (mvInitSataATAPI(pSataAdapter,
			      channelIndex ,
			      PMPort, &pDriveData->identifyInfo,
			      pDriveData->identifyBuffer) == MV_FALSE)
	  {
	       mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d %d]: Failed to initialize ATAPI device\n",
			pSataAdapter->adapterId, channelIndex, PMPort);
                    mvDrivesInfoFlushSingleDrive(ialExt, channelIndex, PMPort);
	  }
	  else
	  {
	       mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO, "[%d %d %d]: ATAPI device ready\n",
			pSataAdapter->adapterId, channelIndex, PMPort);
	       mvSetDriveReady(ialExt,
                                    scsiAdapterExt,
			       channelIndex, PMPort,
			       MV_TRUE,
			       pDriveData->identifyBuffer);
	       mvSataScsiNotifyUA(scsiAdapterExt, channelIndex, PMPort);
	  }
     }
#endif
     else  
     {
	  mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d %d]: bad type for the connected device, deviceType = %d\n",
		   pSataAdapter->adapterId, channelIndex, PMPort, deviceType - MV_SATA_DEVICE_TYPE_UNKNOWN);
	  mvDrivesInfoFlushSingleDrive(ialExt, channelIndex, PMPort);
     }
     return MV_TRUE;
}
static MV_BOOLEAN mvPMPortProbeLink(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
				    MV_U8 channelIndex,
				    MV_U8 PMPort,
				    MV_BOOLEAN force_speed_gen1,
				    MV_U32 *SStatus,
				    MV_BOOLEAN *H2DReceived)
{
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    
    mvPMDevEnableStaggeredSpinUpPort(pSataAdapter, channelIndex, PMPort, 
				     force_speed_gen1);
    /* clear N bit (16) and X bit (26)in serror */
    /* some driver expect the host to respond with R_RDY immediatly */
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, PMPort,
			MV_SATA_PSCR_SERROR_REG_NUM, 
			MV_BIT26 | MV_BIT18 |MV_BIT16, NULL) ==
	MV_FALSE)
    {
	 mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,
		  "[%d %d %d]: PM Write SERROR Failed\n",
		  pSataAdapter->adapterId, channelIndex, PMPort);
    }
    mvMicroSecondsDelay(pSataAdapter, 50000);
    *H2DReceived = mvSataIfD2HReceived(pSataAdapter, channelIndex, PMPort);
    
    /* clear again*/
    mvPMDevWriteReg(pSataAdapter, channelIndex, PMPort,
		    MV_SATA_PSCR_SERROR_REG_NUM,
		    MV_BIT26| MV_BIT18 |MV_BIT16, NULL);
    
    if (mvPMDevReadReg(pSataAdapter, channelIndex, PMPort,
		       MV_SATA_PSCR_SSTATUS_REG_NUM, SStatus, NULL) ==
	MV_FALSE)
    {
	 mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d %d]: mvPMDevReadReg Failed\n",
		  pSataAdapter->adapterId, channelIndex, PMPort);
	 return MV_FALSE;
    }
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO, "[%d %d %d]: S-Status: 0x%x\n",
	     pSataAdapter->adapterId,
	     channelIndex, PMPort, *SStatus);
    
    /* clear X bit in serror */
    if (mvPMDevWriteReg(pSataAdapter, channelIndex, PMPort,
			MV_SATA_PSCR_SERROR_REG_NUM,
			MV_BIT26 | MV_BIT18 | MV_BIT16, NULL) ==
	MV_FALSE)
    {
	 mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,
		  "[%d %d %d]: PM Write SERROR Failed\n",
		  pSataAdapter->adapterId, channelIndex, PMPort);
    }
    return MV_TRUE;      
}
static MV_BOOLEAN mvPMInitDevicesStateHandler(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
					      MV_U8 channelIndex,
					      MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_IAL_COMMON_CHANNEL_EXTENSION* channelExt =
    &ialExt->IALChannelExt[channelIndex];
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    MV_U8 PMPort = channelExt->devInSRST;
    MV_U32 SStatus;
    MV_STORAGE_DEVICE_REGISTERS *mvStorageDevRegisters = &channelExt->mvStorageDevRegisters;
    MV_BOOLEAN H2DReceived = MV_FALSE;
#ifdef MY_ABC_HERE
    MV_U8 PMPort_tmp = 0;
#endif


    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d] CHANNEL_PM_INIT_DEVICES port:%d status:%d\n",
             ialExt->pSataAdapter->adapterId, channelIndex, PMPort,
	     channelExt->port_state);
    
    if(channelExt->port_state == MV_PORT_NOT_INITIALIZED)
    {
	 MV_BOOLEAN found_device = MV_FALSE;
	 MV_U32 retry_count = 0;
	 MV_BOOLEAN force_speed_gen1 = MV_FALSE;
	 do {
	      if(mvPMPortProbeLink(ialExt, channelIndex, PMPort, force_speed_gen1,
				   &SStatus, &H2DReceived) == MV_FALSE) {
		   if (mvStorageDevATASoftResetDevice(pSataAdapter, channelIndex,
						      MV_SATA_PM_CONTROL_PORT, NULL)
		       == MV_FALSE)
		   {
			mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,
				 "[%d %d]:failed to Soft Reset PM control port\n",
				 pSataAdapter->adapterId, channelIndex);
			return MV_FALSE;
		   }
		   PMPort++;
		   if(PMPort == channelExt->PMnumberOfPorts)
			break;
		   else
			continue;
	      }

#ifdef MY_ABC_HERE
          if (((SStatus & 0xf) == 3) && (PMPort < ialExt->IALChannelExt[channelIndex].PMnumberOfPorts)) {
#else
          if (((SStatus & 0xf) == 3) && (PMPort < 4)) {
#endif
		   if(H2DReceived == MV_TRUE){
			channelExt->port_state = MV_PORT_ISSUE_SRST;
		   }else{
			mvSetChannelTimer(ialExt, channelIndex,
					  MV_IAL_WAIT_FOR_RDY_TIMEOUT);
			channelExt->port_state = MV_PORT_WAIT_FOR_RDY;
		   }
		   channelExt->devInSRST = PMPort;
		   found_device = MV_TRUE;
		   return MV_FALSE;
	      }
	      if((SStatus & 0xf) == 1) {
		   if(retry_count++ < 5) {
			/* probe link again 
			 */
			mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,
				 "[%d %d %d]: retry link (%d)\n",
				 pSataAdapter->adapterId, channelIndex, PMPort,
				 SStatus & 0xf, retry_count);

			if((SStatus & 0xf0) == 0x10)
			     force_speed_gen1 = MV_TRUE;
			continue;
		   }
	      } 
	      /* next PORT*/
	      PMPort++;
	      if(PMPort == channelExt->PMnumberOfPorts)
	      {
		   channelExt->port_state = MV_PORT_DONE;
		   break;
	      }
	 }while(found_device == MV_FALSE);
	 
    }else if(channelExt->port_state == MV_PORT_WAIT_FOR_RDY){
	 mvDecrementChannelTimer(ialExt, channelIndex);
	 if(mvSataIfD2HReceived(pSataAdapter, channelIndex, PMPort) == MV_FALSE){
	      mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,
		       "[%d %d %d] - disk not ready: wait a little more\n",
		       pSataAdapter->adapterId, channelIndex, PMPort);

	      printAtaDeviceRegisters(mvStorageDevRegisters);
	      if(mvIsChannelTimerExpired(ialExt, channelIndex) == MV_TRUE){
		   mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,
			    "[%d %d %d]: PM port signature timed out\n",
			    pSataAdapter->adapterId, channelIndex, PMPort);
	      }else{
		   return MV_TRUE;
	      }
	 }else{
	      mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO, "[%d %d %d]: "
		       "device signature received.\n",
		       pSataAdapter->adapterId, channelIndex, PMPort);
	 }
	 channelExt->port_state = MV_PORT_ISSUE_SRST;
	 channelExt->devInSRST = PMPort;
	 return MV_FALSE; /* re-call this function without delay*/
    }else if(channelExt->port_state == MV_PORT_ISSUE_SRST){
	 MV_U32 SError;
	 /* check if disk is connected*/
	 mvPMDevReadReg(pSataAdapter, channelIndex, PMPort, 
			MV_SATA_PSCR_SERROR_REG_NUM, &SError, NULL);
	 mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d %d]:SError "
		  " 0x%08x\n",
		  pSataAdapter->adapterId, channelIndex, PMPort, SError);

	 /*check N bit*/
	 if(SError & MV_BIT16)
	 {
	      mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO, "[%d %d %d]: "
		       "link was changed\n",
                       pSataAdapter->adapterId, channelIndex, PMPort);
	      if(++channelExt->devInSRST == channelExt->PMnumberOfPorts)
	      {
		   channelExt->port_state = MV_PORT_DONE;
	      }else{
		   channelExt->port_state = MV_PORT_NOT_INITIALIZED;
	      }

	      return MV_TRUE;
	 }



	 if (mvStorageDevATAStartSoftResetDevice(pSataAdapter,
						 channelIndex,
						 PMPort) == MV_FALSE)
	 {
	      mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d %d]: "
		       "failed to Soft Reset PM device port.\n",
		       pSataAdapter->adapterId, channelIndex, PMPort);
	      /* this port not functional, next port*/
	      channelExt->port_state = MV_PORT_NOT_INITIALIZED;
	      channelExt->devInSRST++;
	 }
	 else
	 {
	      mvSetChannelTimer(ialExt, channelIndex, MV_IAL_SRST_TIMEOUT);
	      channelExt->port_state = MV_PORT_IN_SRST;
	      channelExt->devInSRST = PMPort;
	      return MV_TRUE;
	 }	 
    }else if(channelExt->port_state == MV_PORT_IN_SRST){
	 mvDecrementChannelTimer(ialExt, channelIndex);
	 if (mvStorageIsDeviceBsyBitOff(pSataAdapter,
					channelIndex,
					mvStorageDevRegisters) == MV_FALSE)
	 {
	      if (mvIsChannelTimerExpired(ialExt, channelIndex) !=
		  MV_TRUE)
	      {
		   mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d %d] - disk not ready: wait a "
			    "little more\n",
			    pSataAdapter->adapterId,
			    channelIndex, PMPort);
		   printAtaDeviceRegisters(mvStorageDevRegisters);
		   return MV_TRUE;
	      }
	      /* SRST timeout*/
	      mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d %d]: Soft Reset PM port time out\n",
		       pSataAdapter->adapterId, channelIndex, PMPort);
	      
	      if (mvStorageDevATASoftResetDevice(pSataAdapter, channelIndex,
						 MV_SATA_PM_CONTROL_PORT, NULL) == MV_FALSE)
	      {
		   mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR, "[%d %d]: "
			    "failed to Soft Reset PM control port\n",
			    pSataAdapter->adapterId, channelIndex);
		   return MV_FALSE;
	      }
	      
	      if(++channelExt->devInSRST == channelExt->PMnumberOfPorts)
	      {
		   channelExt->port_state = MV_PORT_DONE;
	      }else{
		   channelExt->port_state = MV_PORT_NOT_INITIALIZED;
		   return MV_TRUE;
	      }
	      
	 }
	 /* SRST completed*/
	 channelExt->port_state = MV_PORT_INIT_DEVICE;
	 return MV_TRUE;
    }else if(channelExt->port_state == MV_PORT_INIT_DEVICE){
	 MV_U32 SError;
	 /* check if disk is connected*/
	 mvPMDevReadReg(pSataAdapter, channelIndex, PMPort, 
			MV_SATA_PSCR_SERROR_REG_NUM, &SError, NULL);
	 mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG, "[%d %d %d]:SError "
		  " 0x%08x\n",
		  pSataAdapter->adapterId, channelIndex, PMPort, SError);

	 /*check N bit*/
#ifdef MY_ABC_HERE
     if (SError & MV_BIT16) {
         /* Some disk, 9410-HD-001, is always on this bit even mvSataChannelHardReset and mvRestartChannel is used.
          * So we just clear the spurious N bit.
          */
         printk("[%d %d %d]: N of SError is on while init. clear and reset\n",
                        pSataAdapter->adapterId, channelIndex, PMPort);

         if(mvPMPortProbeLink(ialExt, channelIndex, PMPort, MV_FALSE,
				   &SStatus, &H2DReceived) == MV_TRUE) {
             if ((SStatus & 0xf) == 3) {
                 /* Device presence detected and Phy communication established */
                 if (mvStorageDevATASoftResetDevice(pSataAdapter, channelIndex,
                                                    MV_SATA_PM_CONTROL_PORT, NULL)== MV_TRUE){
#ifdef MY_ABC_HERE
                     if (MV_TRUE == classifyAndInitDevice(scsiAdapterExt, ialExt, channelIndex, PMPort)) {
                         printk("[%d %d %d]: reset success\n",
                                pSataAdapter->adapterId, channelIndex, PMPort);
                     } else {
                         return MV_TRUE;
                     }
#else
                     classifyAndInitDevice(scsiAdapterExt, ialExt, channelIndex, PMPort);
#endif
                 } else {
                     printk("[%d %d %d]: reset fail\n",
                            pSataAdapter->adapterId, channelIndex, PMPort);
                 }
             } else if ((SStatus & 0xf) == 1) {
                 /* Device presence detected, but PHY communication not established */
                 printk("[%d %d %d]: Device presence detected, but PHY communication not established, reset whole channel\n",
                        pSataAdapter->adapterId, channelIndex, PMPort);
                 mvSataChannelHardReset(pSataAdapter, channelIndex);
                 mvRestartChannel(ialExt, channelIndex, scsiAdapterExt, MV_FALSE);
                 return MV_FALSE;
             } else {
                 /* skip */
                 printk("[%d %d %d]: give up this port\n",
                        pSataAdapter->adapterId, channelIndex, PMPort);
             }
         } else {
             mvSataChannelHardReset(pSataAdapter, channelIndex);
             mvRestartChannel(ialExt, channelIndex, scsiAdapterExt, MV_FALSE);
             return MV_FALSE;
         }
     } else {
#ifdef MY_ABC_HERE
         if (MV_FALSE == classifyAndInitDevice(scsiAdapterExt, ialExt, channelIndex, PMPort)) {
             return MV_TRUE;
         }
#else
         classifyAndInitDevice(scsiAdapterExt, ialExt, channelIndex, PMPort);
#endif
	 }	 
#else /* MY_ABC_HERE */
     if(SError & MV_BIT16)
	 {
	      mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO, "[%d %d %d]: "
		       "link was changed\n",
		       pSataAdapter->adapterId, channelIndex, PMPort);
     } else {
	      classifyAndInitDevice(scsiAdapterExt, ialExt,
				    channelIndex, PMPort);
	 }
#endif /* MY_ABC_HERE */
	 /* next port*/
	 if(++channelExt->devInSRST == channelExt->PMnumberOfPorts)
	 {
	      channelExt->port_state = MV_PORT_DONE;
	 }else{
          channelExt->port_state = MV_PORT_NOT_INITIALIZED;
	      return MV_TRUE;
	 }
    }
    
    if(channelExt->port_state == MV_PORT_DONE) {
	 mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_INFO,"[%d %d]: PM devices initialized\n",  pSataAdapter->adapterId,
		  channelIndex, PMPort, channelExt->port_state);
#if 0
	 if (clearSErrorPorts(ialExt->pSataAdapter, channelIndex,
			      ialExt->IALChannelExt[channelIndex].PMnumberOfPorts) != 
	     MV_TRUE)
	 {
	      return MV_FALSE;
	 }
#endif
#ifdef MY_ABC_HERE
     for (PMPort_tmp = 0; PMPort_tmp < ialExt->IALChannelExt[channelIndex].PMnumberOfPorts; PMPort_tmp++)
     {
         MV_U32 SError = 0;
         if (MV_FALSE == mvPMDevReadReg(pSataAdapter, channelIndex, PMPort_tmp, 
                            MV_SATA_PSCR_SERROR_REG_NUM, &SError, NULL)) {
             printk("[%d %d %d]: "
                    "failed to read PSCR SError\n",
                    pSataAdapter->adapterId, channelIndex, PMPort_tmp);
         }

         if (SError & MV_BIT16) {
             printk("[%d %d %d]: "
                    "SError N is on retry this channel\n",
                    pSataAdapter->adapterId, channelIndex, PMPort_tmp);
             mvSataChannelHardReset(pSataAdapter, channelIndex);
             mvRestartChannel(ialExt, channelIndex, scsiAdapterExt, MV_FALSE);
             return MV_FALSE;
         }
     }
#endif
#if 1
	 if (mvPMEnableCommStatusChangeBits(ialExt,
					    channelIndex,
					    MV_TRUE) != MV_TRUE)
	 {
	      return MV_FALSE;
	 }
	 if (mvPMEnableAsyncNotify(ialExt, channelIndex) == MV_FALSE)
	 {
	      return MV_FALSE;
	 }
#endif
	 if (mvConfigChannelDMA(ialExt,
				channelIndex,
				scsiAdapterExt) == MV_TRUE)
	 {
	      mvSetChannelState(ialExt, channelIndex, CHANNEL_READY);
	      
	 }
	 else
	 {
	      mvStopChannel(ialExt, channelIndex, scsiAdapterExt);
	 }
    }else if(channelExt->port_state == MV_PORT_FAILED){
	 mvStopChannel(ialExt, channelIndex, scsiAdapterExt);
    }else {
	 mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,"[%d %d %d] - unknown port"
		  " status: %d\n",  pSataAdapter->adapterId,
		  channelIndex, PMPort, channelExt->port_state);
    }
    return MV_TRUE;
}

static MV_BOOLEAN mvChannelReadyStateHandler(
                                            MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                            MV_U8 channelIndex)
{

    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    if ((ialExt->IALChannelExt[channelIndex].pmRegAccessInProgress == MV_FALSE) &&
        (mvStorageDevGetDeviceType (pSataAdapter, channelIndex) == MV_SATA_DEVICE_TYPE_PM) &&
        (ialExt->IALChannelExt[channelIndex].pmAsyncNotifyEnabled == MV_FALSE))
    {
        /*poll pm GSCR error register one time of 4 steps (2 seconds)*/
        if (ialExt->IALChannelExt[channelIndex].pmRegPollCounter++ & 0x3) 
        {
            return MV_TRUE;
        }
        if (mvQueuePMAccessRegisterCommand(ialExt,
                                           channelIndex,
                                           MV_SATA_PM_CONTROL_PORT,
                                           MV_SATA_GSCR_ERROR_REG_NUM,
                                           0,
                                           MV_TRUE) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d] error reading "
                     " PM GSCR_ERROR register.\n",
                     pSataAdapter->adapterId, channelIndex);
        }
    }
    mvSata60X1B2CheckDevError(pSataAdapter, channelIndex);
    return MV_TRUE;
}


static MV_BOOLEAN mvChannelPMHotPlugStateHandler(
                                                MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                                MV_U8 channelIndex,
                                                MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d %d] CHANNEL_PM_HOT_PLUG\n",
             ialExt->pSataAdapter->adapterId, channelIndex);
    mvSataDisableChannelDma(ialExt->pSataAdapter, channelIndex);
    mvSataFlushDmaQueue (ialExt->pSataAdapter,
                         channelIndex, MV_FLUSH_TYPE_CALLBACK);
    mvSataChannelHardReset(ialExt->pSataAdapter, channelIndex);
    mvRestartChannel(ialExt, channelIndex, scsiAdapterExt, MV_FALSE);
    return MV_TRUE;
}

static MV_BOOLEAN mvChannelStateMachine(
                                       MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                       MV_U8 channelIndex,
                                       MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_BOOLEAN res = MV_FALSE;
    do
    {
        switch (ialExt->channelState[channelIndex])
        {
        case CHANNEL_NOT_CONNECTED:
            res = mvChannelNotConnectedStateHandler(ialExt,
                                                    channelIndex,
                                                    scsiAdapterExt);
            break;
        case CHANNEL_CONNECTED:
            res = mvChannelConnectedStateHandler(ialExt,
                                                 channelIndex,
                                                 scsiAdapterExt);
            break;
        case CHANNEL_IN_SRST:
            res = mvChannelInSrstStateHandler(ialExt,
                                              channelIndex,
                                              scsiAdapterExt);
            break;
        case CHANNEL_PM_INIT_DEVICES:
            res = mvPMInitDevicesStateHandler(ialExt,
					      channelIndex,
					      scsiAdapterExt);
            break;
        case CHANNEL_READY:
            res = mvChannelReadyStateHandler(ialExt,
                                             channelIndex);
            break;
        case CHANNEL_PM_HOT_PLUG:
            res = mvChannelPMHotPlugStateHandler(ialExt,
                                                 channelIndex,
                                                 scsiAdapterExt);
            break;
        default:
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: Unknown channel state.\n",
                     ialExt->pSataAdapter->adapterId, channelIndex);
            return MV_FALSE;
        }
    } while (res == MV_FALSE);

    return MV_TRUE;
}


static MV_BOOLEAN mvAdapterStateMachine(
                                       MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                       MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt)
{
    MV_BOOLEAN res = MV_TRUE;
    MV_U8 channelIndex;
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    switch (ialExt->adapterState)
    {
    case ADAPTER_INITIALIZING:     {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d] ADAPTER_INITIALIZING\n",
                     pSataAdapter->adapterId);
 
            res = mvSataEnableStaggeredSpinUpAll(pSataAdapter);
            if (res == MV_TRUE)
            {
                if (mvSataUnmaskAdapterInterrupt(pSataAdapter) == MV_FALSE)
                {
                    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d]: "
                             "mvSataUnmaskAdapterInterrupt failed\n",
                             pSataAdapter->adapterId);
                    ialExt->adapterState = ADAPTER_FATAL_ERROR;
                    return MV_FALSE;
                }
                ialExt->adapterState = ADAPTER_READY;
            }
        }
        break;
    case ADAPTER_READY:
        for (channelIndex = 0;
            channelIndex < pSataAdapter->numberOfChannels; channelIndex++)
        {

            mvChannelStateMachine(ialExt,
                                  channelIndex,
                                  scsiAdapterExt);
        }
        return MV_TRUE;
        break;
    default:
        break;
    }

    if (ialExt->adapterState != ADAPTER_READY)
    {
        return res;
    }

    mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG,"[%d] ADAPTER_READY\n",
             pSataAdapter->adapterId);

    /*Start channel initialization for connected channels*/
    for (channelIndex = 0;
        channelIndex < pSataAdapter->numberOfChannels;
        channelIndex++)
    {
        mvFlushSCSICommandQueue(ialExt, channelIndex);
        if (mvSataIsStorageDeviceConnected(pSataAdapter, channelIndex, NULL) ==
            MV_FALSE)
        {
            mvSetChannelState(ialExt,
                              channelIndex,
                              CHANNEL_NOT_CONNECTED);
            continue;
        }

        mvSetChannelState(ialExt,
                          channelIndex,
                          CHANNEL_CONNECTED);
	
        if (mvChannelStateMachine(ialExt,
                                  channelIndex,
                                  scsiAdapterExt) == MV_FALSE)
        {
            mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ERROR,"[%d %d]: Failed to "
                     "start channel.\n", pSataAdapter->adapterId, channelIndex);
            mvSetChannelState(ialExt,
                              channelIndex,
                              CHANNEL_NOT_CONNECTED);
            mvFlushSCSICommandQueue(ialExt, channelIndex);
            mvSataRemoveChannel(pSataAdapter,channelIndex);
            IALReleaseChannel(pSataAdapter, channelIndex);
            pSataAdapter->sataChannel[channelIndex] = NULL;
            mvDrivesInfoFlushAll(ialExt, channelIndex);
            mvSetDriveReady(ialExt,
                            scsiAdapterExt,
                            channelIndex,
                            0xFF, MV_FALSE, NULL);
            continue;
        }
    }
    return res;
}

static MV_BOOLEAN mvGetDisksModes(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                  MV_SAL_ADAPTER_EXTENSION *pScsiAdapterExt,
                                  MV_U8 channelIndex,
                                  MV_BOOLEAN *TCQ,
                                  MV_BOOLEAN *NCQ,
                                  MV_U8   *numOfDrives)
{
    MV_SATA_ADAPTER *pSataAdapter = ialExt->pSataAdapter;
    MV_BOOLEAN allNCQ = MV_TRUE;
    MV_BOOLEAN allTCQ = MV_TRUE;
    MV_U8               i;

    if ((pSataAdapter == NULL) || (pScsiAdapterExt == NULL))
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,"[%d %d]"
                 " mvGetDisksModes failed, bad pointer\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    if ((ialExt->adapterState != ADAPTER_READY) ||
        (ialExt->channelState[channelIndex] == CHANNEL_NOT_CONNECTED))
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,"[%d %d] "
                 "mvGetDisksModes failed,Bad Adapter or Channel State\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    if (pSataAdapter->sataChannel[channelIndex] == NULL)
    {
        mvLogMsg(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR,"[%d %d] "
                 "mvGetDisksModes failed, channel not configured\n",
                 pSataAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    *numOfDrives = 0;
    for (i = 0; i < MV_SATA_PM_MAX_PORTS; i++)
    {
        if ((pScsiAdapterExt->
            ataDriveData[channelIndex][i].driveReady == MV_TRUE) &&
           (pScsiAdapterExt->ataDriveData[channelIndex][i].identifyInfo.deviceType == MV_SATA_DEVICE_TYPE_ATA_DISK))
        {
            (*numOfDrives)++;

            if (pScsiAdapterExt->ataDriveData[channelIndex][i].
                identifyInfo.DMAQueuedModeSupported == MV_FALSE)
            {
                allTCQ = MV_FALSE;
            }
            if (pScsiAdapterExt->
                ataDriveData[channelIndex][i].identifyInfo.
                SATACapabilities.NCQSupported == MV_FALSE)
            {
                allNCQ = MV_FALSE;
            }
        }
    }

    if (TCQ)
    {
        if ((*numOfDrives > 0) && (allTCQ == MV_TRUE))
        {
            *TCQ = MV_TRUE;
        }
        else
        {
            *TCQ = MV_FALSE;
        }
    }
    if (NCQ)
    {
        if ((*numOfDrives > 0) && (allNCQ == MV_TRUE))
        {
            *NCQ = MV_TRUE;
        }
        else
        {
            *NCQ = MV_FALSE;
        }
    }
    return MV_TRUE;
}

#ifdef SYNO_SATA_PM_DEVICE_GPIO
static inline void syno_pm_device_info_set(MV_SATA_ADAPTER *pSataAdapter, 
                                    MV_U8 channelIndex, 
                                    u8 rw, 
                                    SYNO_PM_PKG *pm_pkg)
{	
    if (pSataAdapter->sataChannel[channelIndex] &&
        syno_pm_is_3726(pSataAdapter->sataChannel[channelIndex]->PMvendorId,
                        pSataAdapter->sataChannel[channelIndex]->PMdeviceId)) {
        pm_pkg->decode = SIMG3726_gpio_decode;
        pm_pkg->encode = SIMG3726_gpio_encode;
        pm_pkg->gpio_addr = MV_SATA_GSCR_3726_GPIO;
        return;
	}
}

MV_BOOLEAN
syno_mvSata_pmp_read_gpio(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt,
                                 MV_U8 channelIndex,
                                 SYNO_PM_PKG *pPM_pkg)
{
    MV_BOOLEAN ret = MV_FALSE;

    syno_pm_device_info_set(pIALExt->pSataAdapter, channelIndex, READ, pPM_pkg);

    if (MV_TRUE == pIALExt->pSataAdapter->sataChannel[channelIndex]->queueCommandsEnabled) {
        ret = SynoMVQueuePMAccessRegisterCommand(pIALExt,
                                                 channelIndex,
                                                 MV_SATA_PM_CONTROL_PORT,
                                                 pPM_pkg->gpio_addr,
                                                 0,
                                                 MV_TRUE,
                                                 &(pPM_pkg->var));
    } else {
        ret = mvPMDevReadReg(pIALExt->pSataAdapter,
                             channelIndex,
                             MV_SATA_PM_CONTROL_PORT, 
                             pPM_pkg->gpio_addr,
                             &(pPM_pkg->var),
                             NULL);
    }

    if (MV_FALSE == ret) {
        goto END;
    }     

    if (pPM_pkg->decode) {
        pPM_pkg->decode(pPM_pkg, READ);
    }
END:
    return ret;    
}

MV_BOOLEAN
syno_mvSata_pmp_write_gpio(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt,
                                  MV_U8 channelIndex,
                                  SYNO_PM_PKG *pPM_pkg)
{
    MV_BOOLEAN ret = MV_FALSE;
    syno_pm_device_info_set(pIALExt->pSataAdapter, channelIndex, READ, pPM_pkg);

    if(pPM_pkg->encode) {
        pPM_pkg->encode(pPM_pkg, WRITE);
    }
    
    if (MV_TRUE == pIALExt->pSataAdapter->sataChannel[channelIndex]->queueCommandsEnabled) {
        ret = SynoMVQueuePMAccessRegisterCommand(pIALExt,
                                                 channelIndex,
                                                 MV_SATA_PM_CONTROL_PORT,
                                                 pPM_pkg->gpio_addr,
                                                 pPM_pkg->var,
                                                 MV_FALSE,
                                                 NULL);
    } else {
        ret = mvPMDevWriteReg(pIALExt->pSataAdapter, 
                              channelIndex,
                              MV_SATA_PM_CONTROL_PORT, 
                              pPM_pkg->gpio_addr,
                              pPM_pkg->var,
                              NULL);
    }

    return ret;
}

static MV_BOOLEAN
syno_mvSata_is_synology_3726(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt, 
                                               MV_U8 channelIndex)
{
    MV_U8 ret = MV_FALSE;

    if (!syno_pm_is_3726(pIALExt->pSataAdapter->sataChannel[channelIndex]->PMvendorId,
                         pIALExt->pSataAdapter->sataChannel[channelIndex]->PMdeviceId)) {
        goto END;
    }

    if (0 >= pIALExt->pSataAdapter->sataChannel[channelIndex]->PMSynoUnique) {
        goto END;
    }

    if (!IS_SYNOLOGY_RX4(pIALExt->pSataAdapter->sataChannel[channelIndex]->PMSynoUnique) &&
	!IS_SYNOLOGY_DX5(pIALExt->pSataAdapter->sataChannel[channelIndex]->PMSynoUnique) &&
	!IS_SYNOLOGY_DXC(pIALExt->pSataAdapter->sataChannel[channelIndex]->PMSynoUnique) &&
	!IS_SYNOLOGY_RXC(pIALExt->pSataAdapter->sataChannel[channelIndex]->PMSynoUnique) &&
        !IS_SYNOLOGY_DX212(pIALExt->pSataAdapter->sataChannel[channelIndex]->PMSynoUnique)) {
        goto END;
    }
     
    ret = MV_TRUE;
END:
    return ret;
}

MV_BOOLEAN
syno_mvSata_is_synology_pm(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt, MV_U8 channelIndex)
{
    u8 ret = MV_FALSE;

    if (!pIALExt->pSataAdapter->sataChannel[channelIndex]) {
        goto END;
    }

    /* After enable preemptive kernel, below code would be dangerous because lack of spin lock */

    if (MV_SATA_DEVICE_TYPE_PM != pIALExt->pSataAdapter->sataChannel[channelIndex]->deviceType) {
        /* 
         * add this condition for those incoming command during the channel reset.
         * We would deny command if and only if gpio command coming during reset.
         * This is done by defer_gpio_cmd
         */
        if (MV_SATA_DEVICE_TYPE_PM != pIALExt->pSataAdapter->sataChannel[channelIndex]->oldDeviceType) {
            goto END;
        }        
    }

    /*
     * skip those models which we do not support
     */
    if (!is_ebox_support()) {
        goto END;
    }

    if (syno_mvSata_is_synology_3726(pIALExt, channelIndex)) {
        ret = MV_TRUE;
        goto END;
    }

END:
    return ret;
}
#endif // SYNO_SATA_PM_DEVICE_GPIO

#ifdef MY_ABC_HERE
/**
 * Because we support shock protection.
 * So we need revalidate whether the device
 * type is the same.
 * 
 * @param ialExt     [IN] Should not be NULL
 * @param scsiAdapterExtstruct
 *                   [IN] Should not be NULL
 * @param pSataAdapter
 *                   [IN] Should not be NULL
 * @param channel    [IN] channel index
 * @param deviceType [IN] new device type
 * 
 * @return TRUE: The device serial is the same 
 *         FALSE: The device serial is difference
 */
static MV_BOOLEAN
blSynoEHTypeRevalidate(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                 MV_SAL_ADAPTER_EXTENSION *scsiAdapterExtstruct,
                 MV_SATA_ADAPTER *pSataAdapter,
                 MV_U8 channel,
                 MV_SATA_DEVICE_TYPE deviceType)
{
    MV_BOOLEAN ret = MV_TRUE;
    MV_U32 old_eh_flags;
    MV_U8 blSpeedAdjust = MV_FALSE;

    if (!pSataAdapter ||
        !pSataAdapter->sataChannel[channel] ||
        !ialExt ||
        !scsiAdapterExtstruct) {
        goto END;
    }
    
    if (!(pSataAdapter->eh[channel].flags & EH_PROCESSING)) {
        /* No need revalidate because it is not EH */
        goto END;
    }

    if (MV_SATA_DEVICE_TYPE_UNKNOWN == pSataAdapter->sataChannel[channel]->oldDeviceType ||
        MV_SATA_DEVICE_TYPE_UNKNOWN == deviceType ||
        pSataAdapter->sataChannel[channel]->oldDeviceType == deviceType) {
        goto END;
    }

    syno_eh_printk(pSataAdapter, channel,
                   "Revalidation fail. new deviceType: %d, old deviceType: %d, old saved deviceType: %d",
                   deviceType,
                   pSataAdapter->sataChannel[channel]->deviceType,
                   pSataAdapter->sataChannel[channel]->oldDeviceType);

    /* Save EH flags before stop channel */
    old_eh_flags = pSataAdapter->eh[channel].flags;
    if (MV_SATA_DEVICE_TYPE_PM != deviceType &&
        MV_SATA_IF_SPEED_1_5_GBPS == mvSataGetInterfaceSpeed(pSataAdapter, channel)) {
        /* FIXME: please identify whether the device need 1.5Gbps after integrade EH with SATA_DETECT */
        blSpeedAdjust = 1;
    }

    /* Stop channel will clear flags and retry count */
    SynomvStopChannel(ialExt, channel, scsiAdapterExtstruct);    

    /* Since we are in the EH, just let it EH again with retry count=0. And it will restart kernel*/
    pSataAdapter->eh[channel].flags = old_eh_flags;
    if (blSpeedAdjust) {
        syno_eh_printk(pSataAdapter, channel, "Set interface speed to 1.5");
        mvSataSetInterfaceSpeed(pSataAdapter, channel, MV_SATA_IF_SPEED_1_5_GBPS);
    }

    mvSetChannelState(ialExt, channel, CHANNEL_CONNECTED);
    ret = MV_FALSE;
END:
    return ret;
}

/**
 * Because we support shock protection.
 * So we need revalidate whether the device is the same.
 * 
 * @param ialExt  [IN] Should not be NULL
 * @param scsiAdapterExtstruct
 *                [IN] Should not be NULL
 * @param pSataAdapter
 *                [IN] Should not be NULL
 * @param channel [IN] channel number
 * @param PMPort  [IN] PM port number
 * @param identifyBuffer
 *                [IN] Should not be NULL. Identify buffer
 * 
 * @return TRUE: The device is the same
 *         FALSE: The device type is difference
 */
static MV_BOOLEAN
blSynoEHDiskRevalidate(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                       MV_SAL_ADAPTER_EXTENSION *scsiAdapterExtstruct,
                       MV_SATA_ADAPTER *pSataAdapter,
                       MV_U8 channel,
                       MV_U8 PMPort,
                       MV_U16_PTR identifyBuffer)
{
    MV_BOOLEAN ret = MV_TRUE;
    char serial_base[IDEN_SERIAL_NUM_SIZE];

    if (!pSataAdapter ||
        !pSataAdapter->sataChannel[channel] ||
        !ialExt ||
        !scsiAdapterExtstruct ||
        !identifyBuffer) {
        goto END;
    }

    if (MV_SATA_DEVICE_TYPE_PM != pSataAdapter->sataChannel[channel]->deviceType &&
        !(pSataAdapter->eh[channel].flags & EH_PROCESSING)) {
        goto END;
    }

    memset(serial_base, 0, sizeof(serial_base));
    if (!memcmp(serial_base,
               &ialExt->IALChannelExt[channel].drivesInfo.driveSerialSaved[PMPort].serial,
               IDEN_SERIAL_NUM_SIZE)) {
        /* for port multiplier probe disk */
        goto END;
    }

    if (!memcmp(&identifyBuffer[IDEN_SERIAL_NUM_OFFSET],
               &ialExt->IALChannelExt[channel].drivesInfo.driveSerialSaved[PMPort].serial,
               IDEN_SERIAL_NUM_SIZE)){        
        goto END;
    }

    syno_eh_printk(pSataAdapter, channel, "Drive is different. PMPort %d", PMPort);
    /* we don't use mvDrivesInfoFlushAll because the disk may exist in PM, 
     * we don't use mvDrivesInfoFlushSingleDrive because we don't need it actually(This only affect PM device)
     */
    memset(&ialExt->IALChannelExt[channel].drivesInfo.driveSerialSaved[PMPort].serial,
           0,
           sizeof(MV_DRIVE_SERIAL_NUMBER));
    memset(&ialExt->IALChannelExt[channel].drivesInfo.driveSerialCurrent[PMPort].serial,
           0,
           sizeof(MV_DRIVE_SERIAL_NUMBER));
    SynoIALSCSINotify(pSataAdapter, (1<<PMPort), channel);
#ifdef MY_ABC_HERE
    IALBusChangeNotifyEx(ialExt->pSataAdapter,
                         channel,
                         (1<<PMPort),
                         0,
                         0);
#else
    IALBusChangeNotifyEx(ialExt->pSataAdapter,
                         channel,
                         (1<<PMPort),
                         0);
#endif

    mvSetChannelState(ialExt, channel, CHANNEL_CONNECTED);
    ret = MV_FALSE;
END:
    return ret;
}

/**
 * Channel EH main process. It must schedule reprobe
 * 
 * @param work   [IN] Should not be NULL.
 */
void 
SynoChannelErrorHandle(struct work_struct *work)
{
    SYNO_EH         *pEH = container_of(work, SYNO_EH, work.work);
    MV_U8           channel;
    MV_IAL_COMMON_ADAPTER_EXTENSION *pIalExt;
    struct mvSataAdapter *pSataAdapter;

    if (!pEH) {
        WARN_ON(1);
        return;
    }

    pIalExt = pEH->pIalExt;
    channel = pEH->channel;
    pSataAdapter = pEH->pSataAdapter;
    if (!pEH->pSataAdapter ||
        !pIalExt) {
        WARN_ON(1);
        return;
    }
    
    if (!(pEH->flags & EH_PROCESSING)) {
        syno_eh_printk(pSataAdapter, channel, "NO EH_PROCESSING flag");
        if (CHANNEL_READY == pIalExt->channelState[channel]) {
            syno_eh_printk(pSataAdapter, channel, "The port is ready, no need error handle");
            return;
        }
    }

    if (pEH->retry_count > MAX_CHANNEL_RETRY) {
        syno_eh_printk(pEH->pSataAdapter, pEH->channel, "Give up this port");
        if (pEH->flags & EH_CONNECT_AGAIN) {
            syno_eh_printk(pEH->pSataAdapter, pEH->channel, "EH_CONNECT_AGAIN is on, give a retry");
        } else {
            SynomvStopChannel(pEH->pIalExt, pEH->channel, pEH->pataScsiAdapterExt);
            return;
        }        
    }

    syno_eh_printk(pSataAdapter, channel, 
                   "Retry count: %d DMA command: %d ", 
                   pEH->retry_count, 
                   mvSataNumOfDmaCommands(pSataAdapter, channel));
    pEH->flags &= (~EH_CONNECT_AGAIN);
    mvSataChannelHardReset(pSataAdapter, channel);
    
    pEH->retry_count++;
    if (mvSataIsStorageDeviceConnected(pSataAdapter, channel, NULL) == MV_FALSE){
        syno_eh_printk(pSataAdapter, channel, 
                       "Device is not connected, retry again");
        queue_delayed_work(mvSata_aux_wq, &pEH->work, CHANNEL_RETRY_INTERVAL);
        return;
    } else {
        syno_eh_printk(pSataAdapter, channel, 
                       "Device back home.");
    }    

    mvRestartChannel(pEH->pIalExt, channel,
                     pEH->pataScsiAdapterExt, MV_TRUE);
}
#endif

#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)

/**
 * Clear EH information when channel 
 * status is stable (Not connect or ready)
 * 
 * @param ialExt [IN] Should not be NULL
 * @param channelIndex
 *               [IN] channel index
 * @param state  [IN] Current channel status
 */
static void 
SynoClearEHInfo(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                MV_U8 channelIndex,
                MV_CHANNEL_STATE state)
{
    if (!ialExt->pSataAdapter ||
        !ialExt->pSataAdapter->IALData) {
        WARN_ON(1);
        return;
    }    

#ifdef MY_ABC_HERE
    if (CHANNEL_NOT_CONNECTED == state) {
        clear_bit(SYNO_PROBE_RETRY, &ialExt->pSataAdapter->flags[channelIndex]);
        clear_bit(SYNO_PROBE_LIMIT_TO_15, &ialExt->pSataAdapter->flags[channelIndex]);
    }
#endif /* MY_ABC_HERE */
#ifdef MY_ABC_HERE
    if (CHANNEL_NOT_CONNECTED == state || CHANNEL_READY == state) {
        MV_U8 blflush = MV_FALSE;

        if (ialExt->pSataAdapter->eh[channelIndex].flags & EH_SCSI_DONE_NEEDED) {
            blflush = MV_TRUE;
        }
        
        ialExt->pSataAdapter->eh[channelIndex].flags = 0;
        ialExt->pSataAdapter->eh[channelIndex].retry_count = 0;
        if (MV_TRUE == blflush) {
            channel_do_scsi_done(ialExt->pSataAdapter->IALData, ialExt->pSataAdapter, channelIndex);
        }
    }
#endif /* MY_ABC_HERE */
}
#endif /* defined(MY_ABC_HERE) || defined(MY_ABC_HERE) */
