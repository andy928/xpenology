/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2006 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the AD9889A and derivative parts only*
*																		                                          *
******************************************************************************/

/******************************************************************************
*                                                                             *
* THIS IS A READ-ONLY SOURCE FILE. DO NOT EDIT IT DIRECTLY.                   *
* Author: Matthew McCarn (matthew.mccarn@analog.com)                          *
* Date: May 16, 2006                                                          *
*                                                                             *
******************************************************************************/
//#include <stdio.h>
//#include <system.h>
//#include <altera_avalon_pio_regs.h>
#include "type_def.h"
#include "AD9889_setup.h"
#include "AD9889A_i2c_handler.h"
#include "reg_handler.h"
#include "AD9889_interrupt_handler.h"
#include "ad9889_macros.h"


//unsigned char edid_dat[256];
//unsigned char revocation_list[64];
static DEPLINT next_segment = 0;
static DEPLINT edid_block_map_offset = 0;
static DEPLINT timer_count = 0;
static DEPLINT hdcp_check_count = 0;
static DEPLINT hdcp_nack_count = 0;
//static BOOL retry_edid = FALSE;

DEPLINT ad9889_interrupt_handler(void (* setup_audio_video)(void))
{
	// to contain the interupt information from AD9889
	BYTE interrupt_registers[2];
	DEPLINT interrupt_code = 0;
	BYTE interrupt_remain[2] = {0xFF,0xFF};

	// read interrupt registers
	while ((interrupt_remain[0]&0xC4) | (interrupt_remain[1]&0xc0))
	{
		//printk ("\n\n\n\n\nprintk before first i2c read\n");
		i2c_read_bytes(AD9889_ADDRESS, 0x96, interrupt_registers, 2);
		//printk ("After 0x%x, 0x%x\n",interrupt_registers[0], interrupt_registers[1]);

		// clear interrupt reigisters
		//DEBUG_MESSAGE_2("AD9889 Interrupt Detected\n  Register 0x97 = 0x%2.2x\n  Register 0x96 = 0x%2.2x\n",
		//		(DEPLINT)interrupt_registers[1],(DEPLINT)interrupt_registers[0]);

		// process error flag
		if ((interrupt_registers[1] & 0x80))
		{
			DEPLINT error;
			//after 4 No Acks when HDCP is requested, assume that sink is not HDCP capable
			if (ADI89_GET_HDCP_CONTROLLER_ERROR == 4)
			{
				//if EDID times out attempt more times
#if defined(HDCP_OPTION)
				if (get_hdcp_hdcp_requested()==FALSE)
				{
#endif
					ADI89_SET_EDID_TRYS(15);
#if defined(HDCP_OPTION)
				}
				else
				{
					hdcp_nack_count++;
					if (hdcp_nack_count > MAX_HDCP_NACK)
					{
						error_callback_function(0x14);
						hdcp_nack_count = 0;
					}
				}
#endif
			}
			error = ADI89_GET_HDCP_CONTROLLER_ERROR;
			printk ("HDCP controller status = 0x%x\n",adi_get_i2c_reg(AD9889_ADDRESS,0xc8, 0xff, 0));
			if (error == 2)
			{
				if (ADI89_GET_RX_SENSE)
				{
					printk ("Here %d\n",__LINE__);
					error_callback_function(error);
				}
			}
			else
			{
				printk ("Here %d\n",__LINE__);
				error_callback_function(error);
				interrupt_code |= INT_ERROR;
#if 1
			{
				static int workaround = 0;

				if (workaround == 0) {
					workaround=1;
					av_mute_on();
					initialize_raptor();
					set_EDID_ready(FALSE);
					setup_audio_video();
					av_mute_off();
				}
			}
#endif
			}
		}

		// AD9889 reverts to defaults on powerdown, so it is reinitialized every HPD
		if ((interrupt_registers[0] & 0x80))
		{
			handle_hpd_interrupt();
			set_EDID_ready(FALSE);
#if defined(HDCP_OPTION)
			set_hdcp_hdcp_enabled(FALSE);
			set_hdcp_hdcp_requested(FALSE);
			set_hdcp_status(FALSE);
#endif
			edid_block_map_offset = 0;
			interrupt_code |= HPD_INT;
		}

		// check for EDID ready flag, then call EDID Handler
		if ((interrupt_registers[0] & 0x04))
		{
			if (!edid_reading_complete())
			{
				DEBUG_MESSAGE("processing EDID...\n");
				next_segment = handle_edid_interrupt();
				interrupt_code |= EDID_INT;
			}
		}

		// verify BKSV
#if defined(HDCP_OPTION)
		if (interrupt_registers[1]&0x40)
		{
			handle_bksv_ready_interrupt();
			interrupt_code |= HDCP_INT;
		}
#endif

		// re-enable the interrupt before requesting additional EDID
		i2c_write_bytes(AD9889_ADDRESS, 0x96, interrupt_registers, 2);

		// if final EDID interrupt was processed tell system that EDID is ready and
		// initiate audio video setup
		if (interrupt_code&EDID_INT)
		{
			// setup_audio_video();
			DEBUG_MESSAGE_1("next segment is %d\n",edid_block_map_offset + next_segment);

			if (next_segment==0)
			{
				av_mute_on();
				initialize_raptor();
				set_EDID_ready(TRUE);
				setup_audio_video();
				av_mute_off();
			}
			else
			{
				ADI89_SET_EDID_SEGMENT(edid_block_map_offset+next_segment);
			}
			if (next_segment==0x40)
			{
				edid_block_map_offset=0x40;
			}
		}

		// Check to see if additional interrupts occured during processing
		i2c_read_bytes(AD9889_ADDRESS, 0x96, interrupt_remain, 2);
	}

	return interrupt_code;
}

/*
 * Every 2 seconds this function is called.  The HDMI and HDCP leds are updated.
 * If HDCP is supposed to be enabled, the status is checked.
 */
#if defined(HDCP_OPTION)
void timer_2_second_handler(void)
{
	timer_count++;

	if ((get_hdcp_hdcp_requested()==TRUE) && (get_hdcp_status()==FALSE))
	{
		hdcp_check_count++;
		if (hdcp_check_count == 16)
		{
			// disable hdcp so it will reset
			DEBUG_MESSAGE("disabling hdcp");
			disable_hdcp();
			hdcp_check_count = 0;
		}

		DEBUG_MESSAGE("check hdcp");

		if (ADI89_GET_HDCP_CONTROLLER_STATE==4)
		{
			set_hdcp_status(TRUE);
			compare_bksvs();
		}
	}

	if (timer_count == TIMER_CALLS_PER_2_SEC)
	{
		timer_count = 0;

		//DEBUG_MESSAGE("tick");

		if ((get_hdcp_hdcp_enabled()) && (!ADI89_GET_ENC_ON) && ADI89_GET_RX_SENSE)
		{
			ERROR_MESSAGE("Unexpected loss of HDCP, Stop audio/video transmission and reauthenticate\n");
			ADI89_SET_HDCP_DESIRED(1);
			set_hdcp_status(FALSE);
			set_hdcp_hdcp_enabled(FALSE);
			set_hdcp_hdcp_requested(TRUE);
			error_callback_function(0x10);
		}
	}
}
#endif
