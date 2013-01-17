
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

********************************************************************************/

/* Marvell Telephony Adaptation Layer */

#include "tal.h"
#include "mp.h"

/* GLobals */
static phone_register_ops_t tal_register_ops;
static tal_mmp_ops_t* tal_mmp_ops;
static phone_params_t tal_phone_params;
 
/* Static APIs */
static void tal_pcm_tx_callback(uint8_t* tx_buff);
static void tal_pcm_rx_callback(uint8_t* rx_buff);
static void tal_event_callback(phone_event* event);

/*---------------------------------------------------------------------------*
 * tal_init
 * Issue telephony subsytem initialization and callbacks registration
 *---------------------------------------------------------------------------*/
tal_stat_t tal_init(tal_params_t* tal_params, tal_mmp_ops_t* mmp_ops)
{
#if 0
	E X A M P L E
	=============
	/* Set phone parameters */
	/* Group #0 */
	mv_tal_phone_params.phone_line_config[0].valid = 1;
	mv_tal_mv_phone_params.phone_line_config[0].first = 0;
	mv_tal_mv_phone_params.phone_line_config[0].last = 0;
	mv_tal_mv_phone_params.phone_line_config[0].phone_line = MV_LINE_FXS;
	mv_tal_mv_phone_params.band_mode[0] = MV_NARROW_BAND;
	mv_tal_mv_phone_params.sample_bytes[0] = MV_LINEAR;

	/* Group #1 */
	mv_tal_phone_params.phone_line_config[1].valid = 1;
	mv_tal_mv_phone_params.phone_line_config[1].first = 1;
	mv_tal_mv_phone_params.phone_line_config[1].last = 1;
	mv_tal_mv_phone_params.phone_line_config[1].phone_line = MV_LINE_FXO;
	mv_tal_mv_phone_params.band_mode[1] = MV_NARROW_BAND;
	mv_tal_mv_phone_params.sample_bytes[1] = MV_LINEAR;
#endif

	if((tal_params == NULL) || (mmp_ops == NULL))
	{
		mvOsPrintf("%s: Error, bad parameters\n",__FUNCTION__);
		return TAL_STAT_BAD_PARAM;
	}

	if(mmp_ops->tal_mmp_event_callback == NULL ||
	   mmp_ops->tal_mmp_rx_callback == NULL ||
	   mmp_ops->tal_mmp_tx_callback == NULL) 
	{
		mvOsPrintf("%s:Error, missing callbacks(MMP)\n",__FUNCTION__);
		return TAL_STAT_BAD_PARAM;
	}

	/* Convert tal_params to phone_params */
	memcpy(&tal_phone_params, tal_params, sizeof(tal_params_t));

	/* Assign MMP operations */
	tal_mmp_ops = mmp_ops;

	/* Clear phone operations structure */
	memset(&tal_register_ops, 0, sizeof(phone_register_ops_t));
	
	/* Assign phone operations */
	tal_register_ops.phone_event_ops.event_callback = tal_event_callback;
	tal_register_ops.phone_pcm_ops.pcm_tx_callback = tal_pcm_tx_callback;
	tal_register_ops.phone_pcm_ops.pcm_rx_callback = tal_pcm_rx_callback;

	/* Dispatch phone driver */
	if(mv_phone_init(&tal_register_ops, &tal_phone_params) != MV_OK)
	{
		mvOsPrintf("%s: Error, could not init phone driver !!!\n",__FUNCTION__);
		return TAL_STAT_INIT_ERROR;
	}

	/* Verify control callbacks were assigned properly */
	if(tal_register_ops.phone_ctl_ops.ctl_fxs_get_hook_state == NULL ||
	   tal_register_ops.phone_ctl_ops.ctl_fxs_ring == NULL ||
	   tal_register_ops.phone_ctl_ops.ctl_fxs_change_line_polarity == NULL ||
	   tal_register_ops.phone_ctl_ops.ctl_fxs_linefeed_control_set == NULL ||
	   tal_register_ops.phone_ctl_ops.ctl_fxs_linefeed_control_get == NULL ||
	   tal_register_ops.phone_ctl_ops.ctl_fxo_set_hook_state == NULL ||
	   tal_register_ops.phone_ctl_ops.ctl_fxo_set_cid_path == NULL ||
	   tal_register_ops.phone_ctl_ops.ctl_pcm_start == NULL ||
	   tal_register_ops.phone_ctl_ops.ctl_pcm_stop == NULL)
	{
		mvOsPrintf("%s:Error, missing callbacks(PHONE)\n",__FUNCTION__);
		return TAL_STAT_BAD_PARAM;
	}

	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * mv_tal_event_handler
 * Notifies engine an incoming event
 *---------------------------------------------------------------------------*/
static void tal_event_callback(phone_event* event)
{
	if(event == NULL)
	{
		mvOsPrintf("%s: Error, received empty event !!!\n",__FUNCTION__);
		return;
	}

	tal_mmp_ops->tal_mmp_event_callback((tal_event*)event); 
	return;
}

/*---------------------------------------------------------------------------*
 * tal_pcm_tx_completion
 * Tx callback
 *---------------------------------------------------------------------------*/

static void tal_pcm_tx_callback(uint8_t* tx_buff)
{
	tal_mmp_ops->tal_mmp_tx_callback(tx_buff);
}

/*---------------------------------------------------------------------------*
 * tal_pcm_rx_completion
 * Rx callback
 *---------------------------------------------------------------------------*/

static void tal_pcm_rx_callback(uint8_t* rx_buff)
{
	tal_mmp_ops->tal_mmp_rx_callback(rx_buff);
}

/*---------------------------------------------------------------------------*
 * tal_fxs_get_hook_state
 * Return the current phone hook state.
 *---------------------------------------------------------------------------*/
tal_stat_t tal_fxs_get_hook_state(unsigned short line_id, tal_hook_t* hook_state)
{
	tal_register_ops.phone_ctl_ops.ctl_fxs_get_hook_state(line_id, (mv_hook_t*)hook_state);
	return TAL_STAT_OK;	
}

/*---------------------------------------------------------------------------*
 * tal_fxs_ring
 * Start/stop ring.
 *---------------------------------------------------------------------------*/
tal_stat_t tal_fxs_ring(unsigned short line_id, int start)
{
	tal_register_ops.phone_ctl_ops.ctl_fxs_ring(line_id, start);
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_fxs_enable_cid_path
 * Set/unset FXS to caller id transmission state.
 *---------------------------------------------------------------------------*/
tal_stat_t tal_fxs_linefeed_control_set(unsigned short line_id, tal_linefeed_t linefeed_state)
{
	tal_register_ops.phone_ctl_ops.ctl_fxs_linefeed_control_set(line_id, 
			(mv_linefeed_t)linefeed_state);
	return TAL_STAT_OK;

}

/*---------------------------------------------------------------------------*
 * tal_fxs_enable_cid_path
 * Set/unset FXS to caller id transmission state.
 *---------------------------------------------------------------------------*/
tal_stat_t tal_fxs_linefeed_control_get(unsigned short line_id, tal_linefeed_t* linefeed_state)
{
	tal_register_ops.phone_ctl_ops.ctl_fxs_linefeed_control_get(line_id, 
				(mv_linefeed_t*)linefeed_state);
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_fxs_change_line_polarity
 * Change the polarity of a line.
 *---------------------------------------------------------------------------*/
tal_stat_t tal_fxs_change_line_polarity(unsigned short line_id)
{
	tal_register_ops.phone_ctl_ops.ctl_fxs_change_line_polarity(line_id);
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_fxo_set_hook_state
 * Set the new hook state of the FXO line.
 *---------------------------------------------------------------------------*/
tal_stat_t tal_fxo_set_hook_state(unsigned short line_id, tal_hook_t hook_state)
{
	tal_register_ops.phone_ctl_ops.ctl_fxo_set_hook_state(line_id, (mv_hook_t)hook_state);
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_fxo_set_cid_path
 * Activate caller id receive path.
 *---------------------------------------------------------------------------*/
tal_stat_t tal_fxo_set_cid_path(unsigned short line_id, int enable)
{
	tal_register_ops.phone_ctl_ops.ctl_fxo_set_cid_path(line_id, enable);
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_custom_control
 * Generic control routine 
 *---------------------------------------------------------------------------*/
tal_stat_t tal_custom_control(unsigned short line_id, void* arg, int len)
{
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_pcm_start
 * Start PCM bus
 *---------------------------------------------------------------------------*/
tal_stat_t tal_pcm_start(void)
{
	tal_register_ops.phone_ctl_ops.ctl_pcm_start();
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * tal_pcm_stop
 * Stop PCM bus
 *---------------------------------------------------------------------------*/
tal_stat_t tal_pcm_stop(void)
{
	tal_register_ops.phone_ctl_ops.ctl_pcm_stop();
	return TAL_STAT_OK;
}

/*---------------------------------------------------------------------------*
 * phone_test_exit
 * Stop TDM channels and release all resources
 *---------------------------------------------------------------------------*/
tal_stat_t tal_exit(void)
{
	mv_phone_exit();
	return TAL_STAT_OK;
}
