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

#ifndef _TAL_H_
#define _TAL_H_

#include "mvOs.h" /* for kernel abstraction wrappers */

/* Defines */
#define MAX_TAL_PHONE_TYPES	2

/* Enumerators */
typedef enum {
	TAL_NARROW_BAND = 0,
	TAL_WIDE_BAND,
} tal_band_mode_t;

typedef enum {
	TAL_LINE_FXO = 0,
	TAL_LINE_FXS
} tal_line_t;

typedef enum {
	TAL_ON_HOOK = 0,
	TAL_OFF_HOOK
} tal_hook_t;

typedef enum {
	TAL_PCM_FORMAT_ALAW = 0,
	TAL_PCM_FORMAT_ULAW,
	TAL_PCM_FORMAT_LINEAR = 3
} tal_pcm_format_t;

typedef enum {
	TAL_LINEFEED_OPEN = 0,
	TAL_LINEFEED_FORWARD_ACTIVE,
	TAL_LINEFEED_FORWARD_ON_HOOK_TRANSMISSION,
	TAL_LINEFEED_TIP_OPEN,
	TAL_LINEFEED_RINGING,
	TAL_LINEFEED_REVERESE_ACTIVE,
	TAL_LINEFEED_REVERSE_ON_HOOK_TRANSMISSION,
	TAL_LINEFEED_RING_OPEN
} tal_linefeed_t;

typedef enum {
	TAL_EVENT_OFF_HOOK = 0,
	TAL_EVENT_ON_HOOK,
	TAL_EVENT_RING,
	TAL_EVENT_LINE_REVERSAL,
	TAL_EVENT_DROP_OUT
} tal_event_t;

typedef enum {
	TAL_STAT_OK = 0,
	TAL_STAT_BAD_PARAM,
	TAL_STAT_INIT_ERROR
} tal_stat_t;

/* Structures */
typedef struct {
	tal_line_t tal_line;
	unsigned short first;		/* first device in range */
	unsigned short last;		/* last device in range */
	int valid;
} tal_line_config_t;

typedef struct {
	tal_line_config_t tal_line_config[MAX_TAL_PHONE_TYPES];
	tal_band_mode_t band_mode[MAX_TAL_PHONE_TYPES];
	tal_pcm_format_t pcm_format[MAX_TAL_PHONE_TYPES];
	unsigned char sample_period;
} tal_params_t;

typedef struct {
	unsigned short line_id;
	unsigned char valid;		/* valid event */
	tal_line_t line_type;
	tal_event_t event_type;
	unsigned char reserved;
} tal_event;

typedef struct {
	void (*tal_mmp_event_callback)(tal_event* event);
	void (*tal_mmp_rx_callback)(unsigned char* rx_buff);
	void (*tal_mmp_tx_callback)(unsigned char* tx_buff);
} tal_mmp_ops_t;

/* APIs */

tal_stat_t tal_init(tal_params_t* tal_params, tal_mmp_ops_t* mmp_ops);
tal_stat_t tal_fxs_get_hook_state(unsigned short line_id, tal_hook_t* hook_state);
tal_stat_t tal_fxs_ring(unsigned short line_id, int start);
tal_stat_t tal_fxs_linefeed_control_set(unsigned short line_id, tal_linefeed_t linefeed_state);
tal_stat_t tal_fxs_linefeed_control_get(unsigned short line_id, tal_linefeed_t* linefeed_state);
tal_stat_t tal_fxs_change_line_polarity(unsigned short line_id);
tal_stat_t tal_fxo_set_hook_state(unsigned short line_id, tal_hook_t hook_state);
tal_stat_t tal_fxo_set_cid_path(unsigned short line_id, int enable);
tal_stat_t tal_custom_control(unsigned short line_id, void* arg, int len);
tal_stat_t tal_pcm_start(void);
tal_stat_t tal_pcm_stop(void);
tal_stat_t tal_exit(void);


#endif /* _TAL_H */
