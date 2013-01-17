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

*******************************************************************************/

#ifndef _MP_H_
#define _MP_H_

#include "voiceband/slic/proslic.h"
#include "voiceband/daa/daa.h"
#include "voiceband/tdm/mvTdm.h"

/* Defines */
#define MAX_PHONE_TYPES		2
 
/* Enumurators */
typedef enum {
	PHONE_EVENT_OFF_HOOK = 0,
	PHONE_EVENT_ON_HOOK,
	PHONE_EVENT_RING,
	PHONE_EVENT_LINE_REVERSAL,
	PHONE_EVENT_DROP_OUT
} phone_event_t;

/* Structures */
typedef struct {
	mv_line_t phone_line;
	unsigned short first;		/* first device in range */
	unsigned short last;		/* last device in range */
	int valid;
} phone_line_config_t;

typedef struct {
	phone_line_config_t phone_line_config[MAX_PHONE_TYPES];
	mv_band_mode_t band_mode[MAX_PHONE_TYPES];
	mv_pcm_format_t pcm_format[MAX_PHONE_TYPES];
	unsigned char sample_period;
} phone_params_t;

typedef struct {
	unsigned short line_id;
	unsigned char valid;		/* valid event */
	mv_line_t phone_line;
	phone_event_t event_type;
	unsigned char shadow;
} phone_event;

/* control callbacks */
typedef struct {
	void (*ctl_fxs_get_hook_state)(unsigned short line_id, mv_hook_t* hook_state);
	void (*ctl_fxs_ring)(unsigned short line_id, int start);
	void (*ctl_fxs_change_line_polarity)(unsigned short line_id);
	void (*ctl_fxs_linefeed_control_set)(unsigned short line_id, mv_linefeed_t linefeed_state);
	void (*ctl_fxs_linefeed_control_get)(unsigned short line_id, mv_linefeed_t* linefeed_state);
	void (*ctl_fxo_set_hook_state)(unsigned short line_id, mv_hook_t hook_state);
	void (*ctl_fxo_set_cid_path)(unsigned short line_id, cid_state_t cid_state);
	void (*ctl_pcm_start)(void);
	void (*ctl_pcm_stop)(void);
} phone_ctl_ops_t;

/* events callback */
typedef struct {
	void (*event_callback)(phone_event* event);
} phone_event_ops_t;

/* pcm callbacks */
typedef struct {
	void (*pcm_tx_callback)(unsigned char* tx_buff);
	void (*pcm_rx_callback)(unsigned char* rx_buff);
} phone_pcm_ops_t;

typedef struct {
	phone_ctl_ops_t phone_ctl_ops;
	phone_event_ops_t phone_event_ops;
	phone_pcm_ops_t phone_pcm_ops;
} phone_register_ops_t;

/* APIs */
MV_STATUS mv_phone_init(phone_register_ops_t* register_ops, phone_params_t* phone_params);
void mv_phone_exit(void);

#endif /*_MP_H_*/

