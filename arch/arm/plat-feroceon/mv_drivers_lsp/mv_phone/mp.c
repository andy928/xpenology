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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File under the following licensing terms. 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer. 

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution. 

    *   Neither the name of Marvell nor the names of its contributors may be 
        used to endorse or promote products derived from this software without 
        specific prior written permission. 
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#include "mp.h"

/* defines */
#define MAX_EVENTS_QUEUE_SIZE	256

/* externs */
extern unsigned short tdmSlotInfo[MV_TDM_TOTAL_CHANNELS][2];

/* Forward declarations */
static irqreturn_t mv_phone_interrupt_handler(int irq, void* dev_id);
static unsigned char mv_phone_line_interrupt_get(unsigned short line_id, mv_line_t phone_line);
static int mv_phone_cmdline_config(char *s);
static inline int mv_phone_parse_config(void);

/* Control Callbacks wrappers */
/* FXS */
static void mv_phone_fxs_get_hook_state(unsigned short line_id, mv_hook_t* hook_state);
static void mv_phone_fxs_ring(unsigned short line_id, int start);
static void mv_phone_fxs_change_line_polarity(unsigned short line_id);
static void mv_phone_fxs_linefeed_control_set(unsigned short line_id, mv_linefeed_t linefeed_state);
static void mv_phone_fxs_linefeed_control_get(unsigned short line_id, mv_linefeed_t* linefeed_state);
/* FXO */
static void mv_phone_fxo_set_hook_state(unsigned short line_id, mv_hook_t hook_state);
static void mv_phone_fxo_set_cid_path(unsigned short line_id, cid_state_t cid_state);

/* PCM */
static void mv_phone_pcm_start(void);
static void mv_phone_pcm_stop(void);

/* Rx/Tx Tasklets  */
static void mv_phone_pcm_rx_process(unsigned long arg);
static void mv_phone_pcm_tx_process(unsigned long arg);

/* Events kernel thread */
static int mv_phone_event_proc(void* arg);

/* Globals */
static unsigned short phone_lines = 0;
static int mv_phone_enable = 0;
static unsigned long flags;
static phone_register_ops_t* mv_phone_register_ops;
static phone_params_t mv_phone_params;
static DECLARE_TASKLET(mv_phone_rx_tasklet, mv_phone_pcm_rx_process, 0);
static DECLARE_TASKLET(mv_phone_tx_tasklet, mv_phone_pcm_tx_process, 0);
static DECLARE_WAIT_QUEUE_HEAD(phone_event_wait);
static DEFINE_SPINLOCK(mv_phone_lock);
static char *cmdline = "dev[0-15]:fxs,dev[16-31]:fxo";
static atomic_t event_count;
static struct completion phone_event_exited;
static pid_t phone_event_pid = (pid_t) -1;
static unsigned char *rxBuff = NULL, *txBuff = NULL;
static phone_event events_array[MAX_EVENTS_QUEUE_SIZE];
static unsigned int event_empty = 0;
static unsigned int event_ready = 0;


static int
mv_phone_cmdline_config(char *s)
{
	cmdline = s;
	return 1;
}

__setup("mv_phone_config=", mv_phone_cmdline_config);

static inline int
mv_phone_parse_config(void)
{
	int count = 0;
	char *fxs_str = "fxs";
	char *fxo_str = "fxo";
	char *tmp;
	
	/* Backup cmdline */
	tmp = cmdline;

	/* Extarct first telephony device range & type */
	mv_phone_params.phone_line_config[0].first = 0; 	/* first device must be '0' */
	
	cmdline += 5; 	/* set cmdline to the range index  */
	if(*cmdline == '-') {
		cmdline++;
		mv_phone_params.phone_line_config[0].last = (unsigned short)simple_strtol(cmdline, &cmdline, 10);
	}
	else
		mv_phone_params.phone_line_config[0].last = 0;

	cmdline += 2; 	/* set cmdline to the device type  */
	if(!memcmp(cmdline,fxs_str, 3))
		mv_phone_params.phone_line_config[0].phone_line = MV_LINE_FXS;
	else if(!memcmp(cmdline,fxo_str, 3))
		mv_phone_params.phone_line_config[0].phone_line = MV_LINE_FXO;
	else {
		printk("mv_phone: unknown telephony device type !!!\n");
		return 0;
	}

	count++;
	mv_phone_params.phone_line_config[0].valid = 1; /* Mark entry as valid */


	/* Extarct second telephony device range & type */
	if(*(cmdline + 3) == ',') { 		/* continue to the next device */
		cmdline += 8;
		mv_phone_params.phone_line_config[1].first = (unsigned short)simple_strtol(cmdline, &cmdline, 10);
		if(*cmdline == '-') {
			cmdline++;
			mv_phone_params.phone_line_config[1].last = (unsigned short)simple_strtol(cmdline, &cmdline, 10);
		}
		else
			mv_phone_params.phone_line_config[1].last = mv_phone_params.phone_line_config[1].first;

		cmdline += 2; 	/* set cmdline to the device type  */
		if(!memcmp(cmdline,fxs_str, 3))
			mv_phone_params.phone_line_config[1].phone_line = MV_LINE_FXS;
		else if(!memcmp(cmdline,fxo_str, 3))
			mv_phone_params.phone_line_config[1].phone_line = MV_LINE_FXO;
		else {
			printk("mv_phone: unknown telephony device type !!!\n");
			return 0;
		}
		count++;
		mv_phone_params.phone_line_config[1].valid = 1; /* Mark entry as valid */
		phone_lines += mv_phone_params.phone_line_config[1].last + 1;
	}
	else
		phone_lines += mv_phone_params.phone_line_config[0].last + 1;

	/* Restore cmdline */
	cmdline = tmp;

	return count;
}

MV_STATUS
mv_phone_init(phone_register_ops_t* register_ops, phone_params_t* phone_params)
{
	mv_tdm_params_t tdm_params;
	mv_band_mode_t band_mode;
	mv_pcm_format_t pcm_format;
	mv_line_t line;
	long first, last;
	int grp = 0, grps;
		
	printk("Marvell Telephony Driver:\n");

	/* Reset globals */
	phone_lines = 0;
	event_empty = event_ready = 0;
	rxBuff = txBuff = NULL;

#if defined(CONFIG_MV78200) || defined(CONFIG_MV632X)
	if (MV_FALSE == mvSocUnitIsMappedToThisCpu(TDM)) {
		printk("%s: TDM is not mapped to this CPU\n", __FUNCTION__);
		return MV_NO_SUCH;
	}
#endif

	if (MV_FALSE == mvCtrlPwrClckGet(TDM_UNIT_ID, 0)) {
		printk("%s: Warning, TDM is powered off\n",__FUNCTION__);
		return MV_OK;
	}

	if(register_ops == NULL) {
		printk("%s: bad parameters\n",__FUNCTION__);
		return MV_ERROR;

	}

	/* Verify no missing callbacks */
	if(register_ops->phone_event_ops.event_callback == NULL ||
	   register_ops->phone_pcm_ops.pcm_tx_callback == NULL ||
	   register_ops->phone_pcm_ops.pcm_rx_callback == NULL ) {
		printk("%s: missing parameters\n",__FUNCTION__);
		return MV_ERROR;

	}

	memset(&mv_phone_params, 0, sizeof(phone_params_t));

	if(phone_params != NULL) {
		memcpy(&mv_phone_params, phone_params, sizeof(phone_params_t));
	}
	else {
		printk("%s: Error, failed to init parameters\n",__FUNCTION__);
		return MV_ERROR;
	}

	/* Direct parameters injection or thru boot string ??? */
	if(phone_params->phone_line_config[0].valid == 0) {
		if(!mv_phone_parse_config()) {
			printk("%s:Error, bad env. parameter !!!\n",__FUNCTION__);
			return MV_ERROR;
		}
	}

	/* Start Marvell trace */
	TRC_START(); 
	TRC_INIT(NULL, NULL, 0, 0);
	TRC_REC("->%s\n",__FUNCTION__);

	grps = mv_phone_params.phone_line_config[0].valid + 
		mv_phone_params.phone_line_config[1].valid;
	
	/* Assign TDM MPPs  - TBD */
    	mvBoardTdmMppSet(mv_phone_params.phone_line_config[grps-1].phone_line);

	/* Assign TDM required parameters */
	tdm_params.bandMode = phone_params->band_mode[0];
	tdm_params.pcmFormat = phone_params->pcm_format[0];
	tdm_params.samplePeriod = phone_params->sample_period;

	/* TDM init */
	mvTdmInit(&tdm_params);

	/* SLIC/DAA init */
	for(grp = 0; grp < grps; grp++) {
			first = mv_phone_params.phone_line_config[grp].first;
			last = mv_phone_params.phone_line_config[grp].last;
			line = mv_phone_params.phone_line_config[grp].phone_line;
			band_mode = mv_phone_params.band_mode[grp];
			pcm_format = mv_phone_params.pcm_format[grp];
			
			if(line == MV_LINE_FXS) { /* FXS */
				if(mvSlicInit(first, last, tdmSlotInfo, phone_lines, band_mode, pcm_format) != MV_OK) {
					printk("%s: Error, FXS init failed !!!\n",__FUNCTION__);
					return MV_ERROR;
				}
			}
			else { /* FXO */
				if(mvDaaInit(first, last, tdmSlotInfo, phone_lines, band_mode, pcm_format) != MV_OK) {
					printk("%s: Error, FXO init failed !!!\n",__FUNCTION__);
					return MV_ERROR;
				}
			}
	}

	/* Assign control callbacks */
	mv_phone_register_ops = register_ops;
	mv_phone_register_ops->phone_ctl_ops.ctl_fxs_get_hook_state = mv_phone_fxs_get_hook_state;
	mv_phone_register_ops->phone_ctl_ops.ctl_fxs_ring = mv_phone_fxs_ring;
	mv_phone_register_ops->phone_ctl_ops.ctl_fxs_change_line_polarity = mv_phone_fxs_change_line_polarity;
	mv_phone_register_ops->phone_ctl_ops.ctl_fxs_linefeed_control_set = mv_phone_fxs_linefeed_control_set;
	mv_phone_register_ops->phone_ctl_ops.ctl_fxs_linefeed_control_get = mv_phone_fxs_linefeed_control_get;
	mv_phone_register_ops->phone_ctl_ops.ctl_fxo_set_hook_state = mv_phone_fxo_set_hook_state;
	mv_phone_register_ops->phone_ctl_ops.ctl_fxo_set_cid_path = mv_phone_fxo_set_cid_path;
	mv_phone_register_ops->phone_ctl_ops.ctl_pcm_start = mv_phone_pcm_start;
	mv_phone_register_ops->phone_ctl_ops.ctl_pcm_stop = mv_phone_pcm_stop;

	/* Used as counter for unhandled events */
	atomic_set(&event_count, 0);

	/* Reset events array */
	memset(events_array, 0, (MAX_EVENTS_QUEUE_SIZE * sizeof(phone_event)));

	init_completion(&phone_event_exited);

	phone_event_pid = 0; /* to avoid race condition */
	phone_event_pid = kernel_thread(mv_phone_event_proc, NULL, CLONE_FS|CLONE_FILES);
	if (phone_event_pid < 0) {
		printk("%s: cannot start events thread(error %d)",__FUNCTION__, phone_event_pid);
		return MV_ERROR;
	}


	/* Register TDM interrupt */
	if (request_irq(TDM_IRQ_INT, mv_phone_interrupt_handler, IRQF_DISABLED, "tdm", events_array)) {
		printk("%s: Failed to connect irq(%d)\n", __FUNCTION__, TDM_IRQ_INT);
		return MV_ERROR;
	}

	TRC_REC("<-%s\n",__FUNCTION__);
	return MV_OK;
}


void
mv_phone_exit(void)
{
	TRC_REC("->%s\n",__FUNCTION__);

	/* Stop PCM bus */
	mv_phone_pcm_stop();

	/* Free phone resources */
	wake_up_interruptible(&phone_event_wait);
	phone_event_pid = (pid_t) -1;
	wait_for_completion(&phone_event_exited);

	/* Free low level drivers resources */
	mvSlicRelease();
	mvDaaRelease();
	mvTdmRelease();

	/* Release IRQ */
	free_irq(TDM_IRQ_INT, events_array);

	TRC_REC("<-%s\n",__FUNCTION__);
	TRC_OUTPUT();
}

static void
mv_phone_pcm_start(void)
{
	TRC_REC("->%s\n",__FUNCTION__);

	spin_lock_irqsave(&mv_phone_lock, flags);
	if(!mv_phone_enable) {
		rxBuff = txBuff = NULL;
		mv_phone_enable = 1;
		mvTdmPcmStart();
	}
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	TRC_REC("<-%s\n",__FUNCTION__);
	return;
}

static void
mv_phone_pcm_stop(void)
{
	TRC_REC("->%s\n",__FUNCTION__);

	spin_lock_irqsave(&mv_phone_lock, flags);
	if(mv_phone_enable){
		mv_phone_enable = 0;	
		mvTdmPcmStop();
	}
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	TRC_REC("<-%s\n",__FUNCTION__);
	return;
}

static irqreturn_t
mv_phone_interrupt_handler(int irq, void* dev_id)
{
	mv_tdm_int_info_t tdm_int_info;
	mv_line_t line;
	volatile unsigned char shadow;
	unsigned short line_id,first, last;
	int grp, grps;
	unsigned int int_type;

	TRC_REC("->%s\n",__FUNCTION__);

	/* Extract interrupt information from low level ISR */
	mvTdmIntLow(&tdm_int_info);
	int_type = tdm_int_info.intType;

	/* Nothing to do - return */
	if(int_type == MV_EMPTY_INT)
		goto out;

	/* Support multiple interrupt handling */
	/* RX interrupt */
	if(int_type & MV_RX_INT) {
		if(rxBuff != NULL)
			printk("%s: Warning, missed Rx buffer processing !!!\n",__FUNCTION__);

		rxBuff = tdm_int_info.tdmRxBuff;

		/* Schedule Rx processing within SOFT_IRQ context */
		TRC_REC("%s: schedule Rx tasklet\n", __FUNCTION__);
		tasklet_hi_schedule(&mv_phone_rx_tasklet);
	}
	/* TX interrupt */
	if(int_type & MV_TX_INT) {
		if(txBuff != NULL)
			printk("%s: Warning, missed Tx buffer processing !!!\n",__FUNCTION__);

		txBuff = tdm_int_info.tdmTxBuff;

		/* Schedule Tx processing within SOFT_IRQ context */
		TRC_REC("%s: schedule Tx tasklet\n", __FUNCTION__);
		tasklet_hi_schedule(&mv_phone_tx_tasklet);
	}
	/* PHONE interrupt */
	if(int_type & MV_PHONE_INT) {
		grps = mv_phone_params.phone_line_config[0].valid + 
			mv_phone_params.phone_line_config[1].valid;
	
		for(grp = 0; grp < grps; grp++) {
			first = mv_phone_params.phone_line_config[grp].first;
			last = mv_phone_params.phone_line_config[grp].last;
			line = mv_phone_params.phone_line_config[grp].phone_line;
			for(line_id = (u16)first; line_id < (u16)(last + 1); line_id++) {
				if((shadow = mv_phone_line_interrupt_get(line_id, line))) {
					if(!events_array[event_empty].valid) {
						/* Fill event info(except event type) */
						events_array[event_empty].valid = 1;
						events_array[event_empty].line_id = line_id;
						events_array[event_empty].phone_line = line;
						events_array[event_empty].shadow = shadow;
						event_empty++;
						if(event_empty == MAX_EVENTS_QUEUE_SIZE)
							event_empty = 0;
						
						atomic_inc(&event_count);
					}
					else {
						printk("%s: Error, event entry already in use(%u) !!!\n",
								__FUNCTION__, line_id);		
					}
				}		
			}
		}
		/* Checks if events handler thread should be signaled */
		if(atomic_read(&event_count) > 0) {
			wake_up_interruptible(&phone_event_wait);
		}
	}
	/* ERROR interrupt */
	if(int_type & MV_ERROR_INT) {
		printk("%s: Error was generated by TDM HW !!!\n",__FUNCTION__);
	}


out:
	TRC_REC("<-%s\n",__FUNCTION__);
	return IRQ_HANDLED;
}

/* Rx tasklet */
static void
mv_phone_pcm_rx_process(unsigned long arg)
{
	TRC_REC("->%s\n",__FUNCTION__);

	if(mv_phone_enable) {
		if(rxBuff == NULL) {
			printk("%s: Error, empty Rx processing\n",__FUNCTION__);
			return;
		}

		/* Fill TDM Rx aggregated buffer */
		if(mvTdmRx(rxBuff) == MV_OK) 
			mv_phone_register_ops->phone_pcm_ops.pcm_rx_callback(rxBuff); /* Dispatch Rx handler */
		else
			printk("%s: could not fill TDM Rx buffer\n",__FUNCTION__);

		/* Clear rxBuff for next iteration */
		rxBuff = NULL;
	}

	TRC_REC("<-%s\n",__FUNCTION__);
	return;
}

/* Tx tasklet */
static void
mv_phone_pcm_tx_process(unsigned long arg)
{
	TRC_REC("->%s\n",__FUNCTION__);

	if(mv_phone_enable) {
		if(txBuff == NULL) {
			printk("%s: Error, empty Tx processing\n",__FUNCTION__);
			return;
		}

		/* Dispatch Tx handler */
		mv_phone_register_ops->phone_pcm_ops.pcm_tx_callback(txBuff);

		/* Fill Tx aggregated buffer */
		if(mvTdmTx(txBuff) != MV_OK)
			printk("%s: could not fill TDM Tx buffer\n",__FUNCTION__);

		/* Clear txBuff for next iteration */
		txBuff = NULL;
	}

	TRC_REC("<-%s\n",__FUNCTION__);
	return;
}

/* kernel thread for events */
static int
mv_phone_event_proc(void* arg)
{
	unsigned char shadow;
	
	TRC_REC("->%s\n",__FUNCTION__);

	daemonize("mv_phone_event");

	for(;;) {
		TRC_REC("checks for new events(%d)\n", atomic_read(&event_count));
		while(atomic_read(&event_count) > 0) {
			if(events_array[event_ready].valid == 0)
				printk("%s: Error, empty event\n", __FUNCTION__);

			shadow = events_array[event_ready].shadow;
			if(events_array[event_ready].phone_line == MV_LINE_FXS) { /* FXS */
				if(shadow & ON_HOOK) { /* ON-HOOK */
					TRC_REC("on-hook detected for line %d\n",events_array[event_ready].line_id);
					events_array[event_ready].event_type = PHONE_EVENT_ON_HOOK;
				}
				else { /* OFF-HOOK */
					TRC_REC("off-hook detected for line %d\n",events_array[event_ready].line_id);
					events_array[event_ready].event_type = PHONE_EVENT_OFF_HOOK;
				}
			}
			else { /* FXO */
				if(shadow & DAA_RDTI)
				{
					TRC_REC("ring detected for line %d\n",events_array[event_ready].line_id);
					events_array[event_ready].event_type = PHONE_EVENT_RING;
				}
				if(shadow & DAA_POLI)
				{
					TRC_REC("reverse polarity detected for line %d\n",events_array[event_ready].line_id);
					events_array[event_ready].event_type = PHONE_EVENT_LINE_REVERSAL;

				}
				if(shadow & DAA_DODI)
				{
					TRC_REC("drop-out detected for line %d\n",events_array[event_ready].line_id);
					events_array[event_ready].event_type = PHONE_EVENT_DROP_OUT;

				}
			}

			/* Dispatch event handler */
			mv_phone_register_ops->phone_event_ops.event_callback(events_array+event_ready);
			/* Clear event indicator */
			events_array[event_ready].valid = 0;
			event_ready++;
			if(event_ready == MAX_EVENTS_QUEUE_SIZE)
				event_ready = 0;

			atomic_dec(&event_count);
		}

		wait_event_interruptible(phone_event_wait, (phone_event_pid == (pid_t) -1 || 
						atomic_read(&event_count) > 0));
			if (signal_pending (current))
				flush_signals(current);

			if(phone_event_pid == (pid_t) -1)
				break;
	}
	
	complete_and_exit(&phone_event_exited, 0);

	TRC_REC("<-%s\n",__FUNCTION__);
}

static void
mv_phone_fxs_linefeed_control_set(unsigned short line_id, mv_linefeed_t linefeed_state)
{
	TRC_REC("->%s\n",__FUNCTION__);

	spin_lock_irqsave(&mv_phone_lock, flags);
	mvSlicLinefeedControlSet(line_id, linefeed_state);
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	TRC_REC("<-%s\n",__FUNCTION__);
	return;
}

static void
mv_phone_fxs_linefeed_control_get(unsigned short line_id, mv_linefeed_t* linefeed_state)
{
	TRC_REC("->%s\n",__FUNCTION__);

	spin_lock_irqsave(&mv_phone_lock, flags);
	mvSlicLinefeedControlGet(line_id, linefeed_state);
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	TRC_REC("<-%s\n",__FUNCTION__);
	return;
}

static unsigned char
mv_phone_line_interrupt_get(unsigned short line_id, mv_line_t phone_line)
{
	int ret_val;
	TRC_REC("->%s(ch%d): %s\n",__FUNCTION__, line_id, phone_line == MV_LINE_FXS ? "FXS":"FXO");

	if(phone_line == MV_LINE_FXS)
		ret_val = mvSlicIntGet(line_id);  
	else
		ret_val = mvDaaIntGet(line_id);
   	
	TRC_REC("<-%s\n",__FUNCTION__);
	return ret_val;
}

static void
mv_phone_fxs_get_hook_state(unsigned short line_id, mv_hook_t* hook_state)
{
	unsigned char state = 0;

	TRC_REC("->%s ch%d\n",__FUNCTION__, line_id);

	spin_lock_irqsave(&mv_phone_lock, flags);
	mvSlicHookStateGet(line_id, &state);
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	if(state == ON_HOOK) {
		*hook_state = MV_ON_HOOK;
	}
	else {
		*hook_state = MV_OFF_HOOK;
	}

	TRC_REC("<-%s\n",__FUNCTION__);
	return;

}

static void
mv_phone_fxs_ring(unsigned short line_id, int start)
{
	TRC_REC("->%s\n",__FUNCTION__);

	spin_lock_irqsave(&mv_phone_lock, flags);
	if(start) {
		mvSlicActivateRinging(line_id);
	}
	else {
		mvSlicStopRinging(line_id);
	}
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	TRC_REC("<-%s\n",__FUNCTION__);
}

static void
mv_phone_fxs_change_line_polarity(unsigned short line_id)
{
	TRC_REC("->%s\n",__FUNCTION__);

	spin_lock_irqsave(&mv_phone_lock, flags);
	mvSlicReverseDcPolarity(line_id);
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	TRC_REC("<-%s\n",__FUNCTION__);
}

static void
mv_phone_fxo_set_hook_state(unsigned short line_id, mv_hook_t hook_state)
{
	TRC_REC("->%s\n",__FUNCTION__);

	spin_lock_irqsave(&mv_phone_lock, flags);
	mvDaaPstnStateSet(hook_state, line_id);
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	TRC_REC("<-%s\n",__FUNCTION__);
}

static void
mv_phone_fxo_set_cid_path(unsigned short line_id, cid_state_t cid_state)
{
	TRC_REC("->%s\n",__FUNCTION__);

	spin_lock_irqsave(&mv_phone_lock, flags);
	mvDaaCidStateSet(cid_state, line_id);
	spin_unlock_irqrestore(&mv_phone_lock, flags);

	TRC_REC("<-%s\n",__FUNCTION__);
}
