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

#include "tal.h"
#include "mp.h"

/* TEST IDs */
#define SELF_ECHO_PHONE_TEST		1
#define FXS_FXS_LOOPBACK_PHONE_TEST	2
#define FXS_FXO_LOOPBACK_PHONE_TEST	3
#define FXS_RING_PHONE_TEST		4
#define FXS_SW_TONE			5

#define PCM_FORMAT			MV_PCM_FORMAT_LINEAR /* set to default: linear mode */
#define BAND_MODE			MV_NARROW_BAND
#define SAMPLE_PERIOD			MV_TDM_BASE_SAMPLE_PERIOD
#define FACTOR				(SAMPLE_PERIOD / MV_TDM_BASE_SAMPLE_PERIOD)
#define LINE_BUFF_SIZE			MV_TDM_CH_BUFF_SIZE(PCM_FORMAT, BAND_MODE, FACTOR)
#define TOTAL_LINES_BUFF_SIZE		(MV_TDM_TOTAL_CHANNELS * LINE_BUFF_SIZE)
#define BUFF_ADDR(buff, line)		((unsigned char*)buff + (line*LINE_BUFF_SIZE))


/* sin table, 256 points */
static short sinTbl[] = {0,402,804,1205,1606,2005,2404,2801,3196,3590,3981,4370,4756,
5139,5519,5896,6270,6639,7005,7366,7723,8075,8423,8765,9102,9433,9759,10079,10393,
10701,11002,11297,11585,11865,12139,12405,12664,12915,13159,13394,13622,13841,14052,
14255,14449,14634,14810,14977,15136,15285,15425,15556,15678,15790,15892,15985,16068,
16142,16206,16260,16304,16339,16363,16378,16383,16378,16363,16339,16304,16260,16206,
16142,16068,15985,15892,15790,15678,15556,15425,15285,15136,14977,14810,14634,14449,
14255,14052,13841,13622,13394,13159,12915,12664,12405,12139,11865,11585,11297,11002,
10701,10393,10079,9759,9433,9102,8765,8423,8075,7723,7366,7005,6639,6270,5896,5519,
5139,4756,4370,3981,3590,3196,2801,2404,2005,1606,1205,804,402,0,-402,-804,-1205,-1606,
-2005,-2404,-2801,-3196,-3590,-3981,-4370,-4756,-5139,-5519,-5896,-6270,-6639,-7005,
-7366,-7723,-8075,-8423,-8765,-9102,-9433,-9759,-10079,-10393,-10701,-11002,-11297,
-11585,-11865,-12139,-12405,-12664,-12915,-13159,-13394,-13622,-13841,-14052,-14255,
-14449,-14634,-14810,-14977,-15136,-15285,-15425,-15556,-15678,-15790,-15892,-15985,
-16068,-16142,-16206,-16260,-16304,-16339,-16363,-16378,-16383,-16378,-16363,-16339,
-16304,-16260,-16206,-16142,-16068,-15985,-15892,-15790,-15678,-15556,-15425,-15285,
-15136,-14977,-14810,-14634,-14449,-14255,-14052,-13841,-13622,-13394,-13159,-12915,
-12664,-12405,-12139,-11865,-11585,-11297,-11002,-10701,-10393,-10079,-9759,-9433,-9102,
-8765,-8423,-8075,-7723,-7366,-7005,-6639,-6270,-5896,-5519,-5139,-4756,-4370,-3981,
-3590,-3196,-2801,-2404,-2005,-1606,-1205,-804,-402,0};

/* GLobals */
static tal_params_t phone_test_params;
static tal_mmp_ops_t phone_test_ops;

static unsigned char rxBuff[TOTAL_LINES_BUFF_SIZE], txBuff[TOTAL_LINES_BUFF_SIZE];
static volatile int count = 0;
static unsigned short f1Mem = 0;
static unsigned short f2Mem = 0;
static int tone_indx = 0;
static unsigned int freq[3] = {2457, 5161, 8192};
static char* tones[3] = {"300Hz", "630Hz", "1000Hz"};

/* Module parameters */
static unsigned short line0_id = 0;
static unsigned short line1_id = 0;
static int test_id = 0;

module_param_named(line0_id, line0_id, short, S_IRUGO);
MODULE_PARM_DESC(line0_id, " Associated line #0 test parameter");

module_param_named(line1_id, line1_id, short, S_IRUGO);
MODULE_PARM_DESC(line1_id, " Associated line #1 test parameter");

module_param_named(test_id, test_id, int, S_IRUGO);
MODULE_PARM_DESC(test_id, "  Specifies the following test options:\n\t\t\t  1 - Self echo on `line0_id`\n\t\t\t  2 - Loopback between 2 FXS ports(line0_id & line1_id)\n\t\t\t  3 - Loopback between FXS and FXO ports(line0_id & line1_id respectively)\n\t\t\t  4 - Ring on FXS line `line0_id`\n\t\t\t  5 - Generate SW tones(300Hz, 630Hz, 1000Hz) on FXS line `line0_id`");

/* Forward declarations */
static int __init phone_test_init(void);
static void __exit phone_test_exit(void);
static void phone_test_pcm_tx_callback(unsigned char* tx_buff);
static void phone_test_pcm_rx_callback(unsigned char* rx_buff);
static void phone_test_event_callback(tal_event* event);
static void phone_test_gen_tone(unsigned char* tx_buff);


static int __init phone_test_init(void)
{
	printk("Marvell phone test module started(params: line0_id(%d), line1_id(%d), test_id(%d))\n",
				line0_id, line1_id, test_id);
	if(test_id < 1) {
		printk("%s: Error: unknown test_id !!!\n",__FUNCTION__);
		return 0;
	}

	/* Set phone parameters */
	phone_test_params.tal_line_config[0].valid = 0;
	phone_test_params.band_mode[0] = phone_test_params.band_mode[1] = BAND_MODE;
	phone_test_params.pcm_format[0] = phone_test_params.pcm_format[1] = PCM_FORMAT;
	phone_test_params.sample_period = SAMPLE_PERIOD;


	/* Assign phone operations */
	phone_test_ops.tal_mmp_event_callback = phone_test_event_callback;
	phone_test_ops.tal_mmp_rx_callback = phone_test_pcm_rx_callback;
	phone_test_ops.tal_mmp_tx_callback = phone_test_pcm_tx_callback;


	if(tal_init(&phone_test_params, &phone_test_ops) != MV_OK) {
		printk("%s: Error, could not init phone driver !!!\n",__FUNCTION__);
		return 0;
	}

	/* Clear buffers */
	memset(rxBuff, 0, TOTAL_LINES_BUFF_SIZE);
	memset(txBuff, 0, TOTAL_LINES_BUFF_SIZE);
	
	count = 0;

	switch(test_id) {
			case FXS_RING_PHONE_TEST:
				/* Activate ring */
				tal_fxs_ring(line0_id, 1);
				mdelay(2000);
				/* Deactivate ring */
				tal_fxs_ring(line0_id, 0);
				break;
	}
	return 0;
}

static void phone_test_pcm_tx_callback(unsigned char* tx_buff)
{
	TRC_REC("->%s\n",__FUNCTION__);

	switch(test_id) {
			case SELF_ECHO_PHONE_TEST:
				memcpy(BUFF_ADDR(tx_buff, line0_id), BUFF_ADDR(rxBuff, line0_id), LINE_BUFF_SIZE);
				break;

			case FXS_FXS_LOOPBACK_PHONE_TEST:
			case FXS_FXO_LOOPBACK_PHONE_TEST:
				memcpy(BUFF_ADDR(tx_buff, line0_id), BUFF_ADDR(rxBuff, line0_id), LINE_BUFF_SIZE);
				memcpy(BUFF_ADDR(tx_buff, line1_id), BUFF_ADDR(rxBuff, line1_id), LINE_BUFF_SIZE);
				break;

			case FXS_SW_TONE:
				phone_test_gen_tone(tx_buff);	
				break;
	}

	TRC_REC("<-%s\n",__FUNCTION__);
	return;
}

static void phone_test_pcm_rx_callback(unsigned char* rx_buff)
{
	TRC_REC("->%s\n",__FUNCTION__);

	switch(test_id) {
			case SELF_ECHO_PHONE_TEST:
				memcpy(BUFF_ADDR(rxBuff, line0_id), BUFF_ADDR(rx_buff, line0_id), LINE_BUFF_SIZE);
				break;

			case FXS_FXS_LOOPBACK_PHONE_TEST:
			case FXS_FXO_LOOPBACK_PHONE_TEST:
				memcpy(BUFF_ADDR(rxBuff, line1_id), BUFF_ADDR(rx_buff, line0_id), LINE_BUFF_SIZE);
				memcpy(BUFF_ADDR(rxBuff, line0_id), BUFF_ADDR(rx_buff, line1_id), LINE_BUFF_SIZE);
				break;
	}

	TRC_REC("<-%s\n",__FUNCTION__);
	return;

}

static void phone_test_event_callback(tal_event* event)
{
	TRC_REC("->%s\n",__FUNCTION__);
	if(event->event_type == TAL_EVENT_OFF_HOOK) {
		printk("line %d goes off-hook\n",event->line_id);
		count++;
		switch(test_id) {
			case SELF_ECHO_PHONE_TEST:
				if(event->line_id == line0_id)
					tal_pcm_start();
				break;

			case FXS_FXS_LOOPBACK_PHONE_TEST:
				if(count == 2)
					tal_pcm_start();
				break;

			case FXS_FXO_LOOPBACK_PHONE_TEST:
				/* Set fxo to off-hook */
				tal_fxo_set_hook_state(line1_id, TAL_OFF_HOOK);
				tal_pcm_start();
				break;

			case FXS_SW_TONE:
				if(event->line_id == line0_id) {
					tal_pcm_start();
					printk("Generating %s tone\n", tones[tone_indx]);
				}
				break;
		}
	}
	else if(event->event_type == TAL_EVENT_ON_HOOK) {
		printk("line %d goes on-hook\n",event->line_id);
		count--;
		switch(test_id) {
			case SELF_ECHO_PHONE_TEST:
				if(event->line_id == line0_id)
					tal_pcm_stop();
				break;

			case FXS_FXS_LOOPBACK_PHONE_TEST:
				if(count == 1)
					tal_pcm_stop();
				break;

			case FXS_FXO_LOOPBACK_PHONE_TEST:
				/* Set fxo to on-hook */
				tal_fxo_set_hook_state(line1_id, TAL_ON_HOOK);
				tal_pcm_stop();
				break;

			case FXS_SW_TONE:
				if(event->line_id == line0_id) {
					tal_pcm_stop();
					tone_indx = ((tone_indx + 1) % 3);
				}
				break;
		}

	}
	else if(event->event_type == TAL_EVENT_RING) {
			printk("Ringing on line %d...\n", event->line_id);
	}

	TRC_REC("<-%s\n",__FUNCTION__);
}

static void phone_test_gen_tone(unsigned char* tx_buff)
{
	short i;
	short buf[SAMPLES_BUFF_SIZE(BAND_MODE, FACTOR)];
	short sample;

	for(i = 0; i < SAMPLES_BUFF_SIZE(BAND_MODE, FACTOR); i++) {
		sample = (sinTbl[f1Mem >> 8] + sinTbl[f2Mem >> 8]) >> 2;
#ifndef CONFIG_CPU_BIG_ENDIAN 
		buf[i] = sample;
#else
		buf[i] = ((sample & 0xff) << 8)+ (sample >> 8);
#endif 
		f1Mem += freq[tone_indx];
		f2Mem += freq[tone_indx];
	}
	memcpy(BUFF_ADDR(tx_buff, line0_id), (void *)buf, LINE_BUFF_SIZE);
}

static void __exit phone_test_exit(void)
{
	printk("Marvell phone test module exits\n");

	/* Stop TDM channels and release all resources */
	tal_exit();
}

/* Module stuff */
module_init(phone_test_init);
module_exit(phone_test_exit);
MODULE_DESCRIPTION("Marvell Telephony Test Module - www.marvell.com");
MODULE_AUTHOR("Eran Ben-Avi <benavi@marvell.com>");
MODULE_LICENSE("GPL");


