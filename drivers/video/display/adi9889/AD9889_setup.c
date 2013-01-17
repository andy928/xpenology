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
//#include <stdlib.h>
//#include <system.h>
#include "type_def.h"
#include "edid_parse.h"
#include "reg_handler.h"
#include "AD9889_setup.h"
#include "ad9889_macros.h"
#include "AD9889A_i2c_handler.h"


#if defined(HDCP_OPTION)
//static DEPLINT bksv_flag_count = 0;
static DEPLINT hdcp_status = 0;
static DEPLINT hdcp_enabled = 0;
static DEPLINT hdcp_requested = 0;
static DEPLINT bksv_flag_counter = 0;
static DEPLINT bksv_count = 0;
static DEPLINT total_bksvs = 0;
static BYTE bksv[BKSV_MAX];
static BYTE hdcp_bstatus[2];
#endif

EdidInfo edid = NULL;
DEPLINT (*i2c_write_bytes)(DEPLINT dev_addr_in, DEPLINT reg_addr_in, BYTE *data_in, DEPLINT count_in);
DEPLINT (*i2c_read_bytes)(DEPLINT dev_addr_in, DEPLINT reg_addr_in, BYTE *data_out, DEPLINT count_in);

void (*error_callback_function)(int);

DEPLINT next_segment_number;

BOOL edid_ready = FALSE;

BOOL csc_444_422;


void av_mute_on()
{
	ADI89_SET_CH0PWRDWNI2C(1);
	ADI89_SET_CH1PWRDWNI2C(1);
	ADI89_SET_CH2PWRDWNI2C(1);
	ADI89_SET_SET_AVMUTE(1);
	ADI89_SET_CLEAR_AVMUTE(0);
}

void av_mute_off()
{
	ADI89_SET_CH0PWRDWNI2C(0);
	ADI89_SET_CH1PWRDWNI2C(0);
	ADI89_SET_CH2PWRDWNI2C(0);
	ADI89_SET_SET_AVMUTE(0);
	ADI89_SET_CLEAR_AVMUTE(1);
}

void set_MCLK_ratio(int val)
{
	ADI89_SET_MCLK_RATIO(val);
}

#if defined(HDCP_OPTION)
BOOL is_repeater()
{
	return ADI89_GET_BCAPS;
}
#endif

void clear_interrupts()
{
	BYTE hexFF[2] = {0xFF,0xFF};

	i2c_write_bytes(AD9889_ADDRESS, 0x96, hexFF, 2);
}

extern void msleep(int);

void handle_hpd_interrupt()
{
	if (ADI89_GET_HPD_STATE)
	{
		// Power down TMDS
		DEBUG_MESSAGE("POWER DOWN TMDS");
		ADI89_SET_CH0PWRDWNI2C(1);
		ADI89_SET_CH1PWRDWNI2C(1);
		ADI89_SET_CH2PWRDWNI2C(1);
		ADI89_SET_POWER_DOWN(0); /* Rabeeh - review this - TODO */
		msleep(1000);
#if defined(HDCP_OPTION)
		disable_hdcp();
#endif
		ADI89_SET_MASK1(0x84);
		ADI89_SET_MASK2(0x3);
	}

	if (edid)
		clear_edid(edid);
}

void set_EDID_ready(BOOL value)
{
	edid_ready = value;
}

BOOL edid_reading_complete()
{
	return edid_ready;
}

DEPLINT handle_edid_interrupt()
{
	BYTE edid_dat[256];

	i2c_read_bytes(0x7e, 0x00, edid_dat, 256);
	edid = init_edid_info(edid);

	return parse_edid(edid, edid_dat);
}

void initialize_raptor()
{
	BYTE value;

	printk ("Initializing raptor\n");

#if defined(HDCP_OPTION)
	// Set internal or external HDCP keys
	ADI89_SET_INT_EPP_ON(HDMI_TX);
#endif

	// Power Down TMDS lines
	av_mute_on();
	ADI89_SET_I2S3_ENABLE(0);
	ADI89_SET_I2S2_ENABLE(0);
	ADI89_SET_I2S1_ENABLE(0);
	ADI89_SET_I2S0_ENABLE(0);

	// Power up chip
	ADI89_SET_POWER_DOWN(0);

	// Set static reserved registers
	ADI89_SET_AVE_MODE(0);
	value = 0x03;
	i2c_write_bytes(AD9889_ADDRESS, 0x98, &value, 1);
	value = 0x38;
	i2c_write_bytes(AD9889_ADDRESS, 0x9C, &value, 1);
	value = 0x87;
	i2c_write_bytes(AD9889_ADDRESS, 0xA2, &value, 1);
	i2c_write_bytes(AD9889_ADDRESS, 0xA3, &value, 1);
	value = 0xFF;
	i2c_write_bytes(AD9889_ADDRESS, 0xBB, &value, 1);
	ADI89_SET_CLK_DELAY(3);
	ADI89_SET_R30(0x8);

	DEBUG_MESSAGE("Raptor Presets Loaded\n");
}

void AD9889_reset(void)
{
	ADI89_SET_LOGIC_RESET(1);
	ADI89_SET_LOGIC_RESET(0);
}

void setup_colorspace(Colorspace colorspace_in, Range colorspace_range)
{
	BYTE rgb_rgb[24] = {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0};
//	BYTE rgb_rgb[24] = {0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0};
	BYTE rgb_hdtv_ycrcb_0[24] = {8, 45, 24, 147, 31, 63, 8, 0, 3, 104, 11, 113, 1, 39, 0, 0, 30, 33, 25, 178, 8, 45, 8, 0};
	BYTE rgb_hdtv_ycrcb_16[24] = {7, 6, 25, 160, 31, 91, 8, 0, 2, 237, 9, 211, 0, 253, 1, 0, 30, 100, 26, 150, 7, 6, 8, 0};
	BYTE rgb_sdtv_ycrcb_0[24] = {8, 45, 25, 39, 30, 172, 8, 0, 4, 201, 9, 100, 1, 211, 0, 0, 29, 63, 26, 147, 8, 45, 8, 0};
	BYTE rgb_sdtv_ycrcb_16[24] = {7, 6, 26, 30, 30, 220, 8, 0, 4, 28, 8, 17, 1, 145, 1, 0, 29, 163, 27, 87, 7, 6, 8, 0};
	BYTE hdtv_ycrcb_0_rgb[24] = {12, 82, 8, 0, 0, 0, 25, 215, 28, 84, 8, 0, 30, 137, 2, 145, 0, 0, 8, 0, 14, 135, 24, 188};
	BYTE hdtv_ycrcb_0_hdtv_ycrcb_0[24] = {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0};
	BYTE hdtv_ycrcb_0_hdtv_ycrcb_16[24] = {13, 190, 0, 0, 0, 0, 1, 33, 0, 0, 13, 190, 0, 0, 1, 0, 0, 0, 0, 0, 13, 190, 1, 33};
	BYTE hdtv_ycrcb_16_rgb[24] = {7, 44, 4, 168, 0, 0, 28, 31, 29, 221, 4, 168, 31, 38, 1, 52, 0, 0, 4, 168, 8, 117, 27, 123};
	BYTE hdtv_ycrcb_16_hdtv_ycrcb_0[24] = {9, 81, 0, 0, 0, 0, 31, 88, 0, 0, 9, 81, 0, 0, 31, 107, 0, 0, 0, 0, 9, 81, 31, 88};
	BYTE hdtv_ycrcb_16_hdtv_ycrcb_16[24] = {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0};
	BYTE sdtv_ycrcb_0_rgb[24] = {10, 248, 8, 0, 0, 0, 26, 132, 26, 106, 8, 0, 29, 80, 4, 35, 0, 0, 8, 0, 13, 219, 25, 18};
	BYTE sdtv_ycrcb_0_sdtv_ycrcb_0[24] = {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0};
	BYTE sdtv_ycrcb_0_sdtv_ycrcb_16[24] = {13, 190, 0, 0, 0, 0, 1, 33, 0, 0, 13, 190, 0, 0, 1, 0, 0, 0, 0, 0, 13, 190, 1, 33};
	BYTE sdtv_ycrcb_16_rgb[24] = {6, 99, 4, 168, 0, 0, 28, 132, 28, 192, 4, 168, 30, 111, 2, 30, 0, 0, 4, 168, 8, 17, 27, 173};
	BYTE sdtv_ycrcb_16_sdtv_ycrcb_0[24] = {9, 81, 0, 0, 0, 0, 31, 88, 0, 0, 9, 81, 0, 0, 31, 107, 0, 0, 0, 0, 9, 81, 31, 88};
	BYTE sdtv_ycrcb_16_sdtv_ycrcb_16[24] = {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0};
	// Select colorspace in based on parameters
	DEPLINT space_in = 0;
	DEPLINT space_out;

	if (colorspace_in == RGB)
	{
		space_in = 1;
		if (colorspace_range != AD9889_0_255)
		{
			ERROR_MESSAGE("Error: Colorspace in not supported");
			error_callback_function(0x11);
		}
	}
	else if (colorspace_in == YCbCr709)
	{
		if (colorspace_range == AD9889_0_255)
		{
			space_in = 2;
		}
		else if (colorspace_range == AD9889_16_235)
		{
			space_in = 3;
		}
		else
		{
			ERROR_MESSAGE("Error: Colorspace in not supported");
			error_callback_function(0x11);
		}
	}
        else if (colorspace_in == YCbCr601)
	{
		if (colorspace_range == AD9889_0_255)
		{
			space_in = 4;
		}
		else if (colorspace_range == AD9889_16_235)
		{
			space_in = 5;
		}
		else
		{
			ERROR_MESSAGE("Error: Problem selecting colorspace");
			error_callback_function(0x11);
		}
	}
	else
	{
		ERROR_MESSAGE("Error: Problem selecting colorspace");
		error_callback_function(0x11);
	}

	// Select colorspace out based on EDID information and colorspace in
	if (space_in == 1)
		space_out = 1;
	if ((space_in == 2) || (space_in == 3) || (space_in == 4) || (space_in == 5))
	{
		if (((edid->cea->YCC444)==TRUE) || ((edid->cea->YCC422)==TRUE))
			space_out = space_in;
		else
			space_out = 1;
	}

	// Set colorspace registers
	if (space_out == space_in)
	{
		if (colorspace_in == RGB)
		{
			ADI89_SET_VFE_OUT_FMT(0);
			ADI89_SET_Y1Y0(0);
		}
		if (((colorspace_in == YCbCr709) || (colorspace_in == YCbCr601)) && !csc_444_422)
		{
			ADI89_SET_VFE_OUT_FMT(2);
			ADI89_SET_Y1Y0(1);
		}
		if (((colorspace_in == YCbCr709) || (colorspace_in == YCbCr601)) && csc_444_422)
		{
			ADI89_SET_VFE_OUT_FMT(1);
			ADI89_SET_Y1Y0(2);
		}
	}
	else
	{
		ADI89_SET_VFE_OUT_FMT(0);
		ADI89_SET_Y1Y0(0);
	}

	if (space_in==1 && space_out==1)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,rgb_rgb ,24);
		ADI89_SET_CSC_MODE(1);
	}

	if (space_in==1 && space_out==2)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,rgb_hdtv_ycrcb_0 ,24);
		ADI89_SET_CSC_MODE(0);
	}

	if (space_in==1 && space_out==3)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,rgb_hdtv_ycrcb_16 ,24);
		ADI89_SET_CSC_MODE(0);
	}

	if (space_in==2 && space_out==1)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,hdtv_ycrcb_0_rgb ,24);
		ADI89_SET_CSC_MODE(1);
	}

	if (space_in==2 && space_out==2)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,hdtv_ycrcb_0_hdtv_ycrcb_0 ,24);
		ADI89_SET_CSC_MODE(1);
	}

	if (space_in==2 && space_out==3)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,hdtv_ycrcb_0_hdtv_ycrcb_16 ,24);
		ADI89_SET_CSC_MODE(0);
	}

	if (space_in==3 && space_out==1)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,hdtv_ycrcb_16_rgb ,24);
		ADI89_SET_CSC_MODE(2);
	}

	if (space_in==3 && space_out==2)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,hdtv_ycrcb_16_hdtv_ycrcb_0 ,24);
		ADI89_SET_CSC_MODE(0);
	}

	if (space_in==3 && space_out==3)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,hdtv_ycrcb_16_hdtv_ycrcb_16 ,24);
		ADI89_SET_CSC_MODE(1);
	}

	if (space_in==1 && space_out==4)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,rgb_sdtv_ycrcb_0 ,24);
		ADI89_SET_CSC_MODE(0);
	}

	if (space_in==1 && space_out==5)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,rgb_sdtv_ycrcb_16 ,24);
		ADI89_SET_CSC_MODE(0);
	}

	if (space_in==4 && space_out==1)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,sdtv_ycrcb_0_rgb ,24);
		ADI89_SET_CSC_MODE(1);
	}

	if (space_in==4 && space_out==4)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,sdtv_ycrcb_0_sdtv_ycrcb_0 ,24);
		ADI89_SET_CSC_MODE(1);
	}

	if (space_in==4 && space_out==5)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,sdtv_ycrcb_0_sdtv_ycrcb_16 ,24);
		ADI89_SET_CSC_MODE(1);
	}

	if (space_in==5 && space_out==1)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,sdtv_ycrcb_16_rgb ,24);
		ADI89_SET_CSC_MODE(2);
	}

	if (space_in==5 && space_out==4)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,sdtv_ycrcb_16_sdtv_ycrcb_0 ,24);
		ADI89_SET_CSC_MODE(1);
	}

	if (space_in==5 && space_out==5)
	{
		i2c_write_bytes(AD9889_ADDRESS,0x18,sdtv_ycrcb_16_sdtv_ycrcb_16 ,24);
		ADI89_SET_CSC_MODE(1);
	}

	ADI89_SET_CSC_ENABLE(1);
}

#if defined(HDCP_OPTION)
void enable_hdcp(void)
{
	hdcp_requested = 1;
	ADI89_SET_FRAME_ENC(1);
	ADI89_SET_HDCP_DESIRED(1);
}

void disable_hdcp()
{
	ADI89_SET_HDCP_DESIRED(0);
	ADI89_SET_FRAME_ENC(0);
	hdcp_enabled = 0;
	hdcp_requested = 0;
	hdcp_status = 0;
}

BOOL get_hdcp_status()
{
	return hdcp_status;
}

void set_hdcp_status(BOOL status)
{
	hdcp_status = status;
}

BOOL get_hdcp_hdcp_requested()
{
	return hdcp_requested;
}

void set_hdcp_hdcp_requested(BOOL requested)
{
	hdcp_requested = requested;
}

BOOL get_hdcp_hdcp_enabled()
{
	return hdcp_enabled;
}

void set_hdcp_hdcp_enabled(BOOL enabled)
{
	hdcp_enabled = enabled;
}

void compare_bksvs()
{
}
#endif

BOOL get_hdmi_status()
{
	return ADI89_GET_EXT_HDMI_MODE;
}

#if defined(HDCP_OPTION)
void handle_bksv_ready_interrupt()
{
	if (hdcp_requested == 1)
	{
		//DEPLINT i = 0;
		bksv_count = ADI89_GET_BKSV_COUNT;

		hdcp_enabled = 1;

		if (bksv_count == 0)
		{
			// Read BKSV registers first
			total_bksvs = 1;
			bksv_flag_counter = 0;
			//free(bksv);
			//bksv = (BYTE *)chkmalloc(5);
			//DEPLINT wait;
			// for(wait = 0;wait < 1000000; wait++);
			bksv[4] = ADI89_GET_BKSV1;
			bksv[3] = ADI89_GET_BKSV2;
			bksv[2] = ADI89_GET_BKSV3;
			bksv[1] = ADI89_GET_BKSV4;
			bksv[0] = ADI89_GET_BKSV5;

			hdcp_bstatus[1] = 0;
			hdcp_bstatus[0] = 0;

			if(ADI89_GET_HDMI_MODE)
				hdcp_bstatus[1] = 0x10;
		}

		// Read downstream KSVs from EDID memory for a repeater
		if (bksv_count > 0)
		{
			if (bksv_flag_counter == 0)
			{
				i2c_read_bytes(AD9889_MEMORY_ADDRESS,0xF9,hdcp_bstatus,2);
			}

			i2c_read_bytes(AD9889_MEMORY_ADDRESS, 0x00, bksv + 5 + (bksv_flag_counter*65), (bksv_count*5));

			total_bksvs += bksv_count;
			bksv_flag_counter++;
		}
	}
	else
	{
		ERROR_MESSAGE("Error: BKSV Interrupt with no Request\n");
	}
}
#endif

void initialize_ad9889_i2c(DEPLINT (*read_function)(DEPLINT, DEPLINT, BYTE *, DEPLINT), DEPLINT (*write_function)(DEPLINT, DEPLINT, BYTE *, DEPLINT))
{
	i2c_read_bytes = read_function;
	i2c_write_bytes = write_function;
	DEBUG_MESSAGE("I2C read and write functions assigned\n");
}

void initialize_error_callback(void (*error_function)(int))
{
	error_callback_function = error_function;
}

void set_video_mode(Timing video_timing, Colorspace colorspace_in, Range colorspace_range, AspectRatio aspect_ratio)
{
	setup_colorspace(colorspace_in, colorspace_range);

	if (aspect_ratio == _4x3)
	{
		ADI89_SET_ASP_RATIO(0);
		ADI89_SET_M1M0(1);
	}
	if (aspect_ratio == _16x9)
	{
		ADI89_SET_ASP_RATIO(1);
		ADI89_SET_M1M0(2);
	}

	hdmi_or_dvi();
}

void set_input_format(DEPLINT input_format, DEPLINT input_style, DEPLINT input_bit_width, DEPLINT clock_edge)
{
	ADI89_SET_VFE_INPUT_ID(input_format);
	ADI89_SET_VFE_422_WIDTH(input_bit_width);
	ADI89_SET_VFE_INPUT_STYLE(input_style);
	ADI89_SET_VFE_INPUT_EDGE(clock_edge);

#if 0 /* TODO - Rabeeh - review this code */
	if(edid->cea->YCC444)
	{
		csc_444_422 = TRUE;
	}
	else if(edid->cea->YCC422)
	{
		csc_444_422 = FALSE;
	}
	else
	{
		csc_444_422 = TRUE;
	}
#endif
}

void set_audio_mode(SamplingFrequency sampling_frequency, DEPLINT bit_width, DEPLINT channel_count, DEPLINT channel_map)
{
	DEPLINT fs;
	DEPLINT sample[8]={0,4096,6272,6144,12544,12288,25088,24576};
	BYTE n[3];

	if (sampling_frequency == _32k)
	{
		ADI89_SET_I2S_SF(3);
		fs = 1;
	}

	if (sampling_frequency == _44k)
	{
		ADI89_SET_I2S_SF(0);
		fs = 2;
	}

	if (sampling_frequency == _48k)
	{
		ADI89_SET_I2S_SF(2);
		fs = 3;
	}

	if (sampling_frequency == _88k)
	{
		ADI89_SET_I2S_SF(8);
		fs = 4;
	}

	if (sampling_frequency == _96k)
	{
		ADI89_SET_I2S_SF(10);
		fs = 5;
	}

	if (sampling_frequency == _176k)
	{
		ADI89_SET_I2S_SF(12);
		fs = 6;
	}

	if (sampling_frequency == _192k)
	{
		ADI89_SET_I2S_SF(14);
		fs = 7;
	}

	n[0] = (char)((sample[fs] >> 16) & 0x0F);
	n[1] = (char)((sample[fs] >> 8) & 0xFF);
	n[2] = (char)(sample[fs] & 0xFF);

	i2c_write_bytes(AD9889_ADDRESS,0x01,n,3);

	ADI89_SET_I2S_BIT_WIDTH(bit_width);

	if (channel_count)
		ADI89_SET_AUDIO_IF_CC(channel_count-1);
	else
		ADI89_SET_AUDIO_IF_CC(0);

	if (channel_count >= 0)
		ADI89_SET_I2S0_ENABLE(1);
	else
		ADI89_SET_I2S0_ENABLE(0);

	if (channel_count > 2)
		ADI89_SET_I2S1_ENABLE(1);
	else
		ADI89_SET_I2S1_ENABLE(0);

	if (channel_count > 4)
		ADI89_SET_I2S2_ENABLE(1);
	else
		ADI89_SET_I2S2_ENABLE(0);

	if (channel_count > 6)
		ADI89_SET_I2S3_ENABLE(1);
	else
		ADI89_SET_I2S3_ENABLE(0);

	ADI89_SET_SPEAKER_MAPPING(channel_map);
}

void adi9889_early_setup(void)
{
	adi_set_i2c_reg(AD9889_ADDRESS,0xa5, 0xc0, 6, 0x3);
	adi_set_i2c_reg(AD9889_ADDRESS,0x9d, 0xff, 0, 0x68);
}

void set_spdif_audio(void)
{
	ADI89_SET_EXT_AUDIOSF_SEL(0);
	ADI89_SET_CR_BIT(1);
}

void set_audio_format(AudioInputMode input_mode)
{
	if (input_mode == SPDIF)
	{
		ADI89_SET_SPDIF_EN(1);
		ADI89_SET_AUDIO_SEL(1);
	}

	if (input_mode == I2S)
	{
		ADI89_SET_AUDIO_SEL(0);
		ADI89_SET_I2S_FORMAT(0);
	}

	if (input_mode == I2S_LEFT)
	{
		ADI89_SET_AUDIO_SEL(0);
		ADI89_SET_I2S_FORMAT(2);
	}

	if (input_mode == I2S_RIGHT)
	{
		ADI89_SET_AUDIO_SEL(0);
		ADI89_SET_I2S_FORMAT(1);
	}
}

BOOL query_vid(DEPLINT vid)
{
	DEPLINT i;

	for (i = 0; i < SVD_MAX; i++)
	{
		if (edid->cea->cea->svd[i].vid_id == vid)
			return TRUE;
	}
	return FALSE;
}

BOOL query_colorspace_in(Colorspace colorspace_in)
{
/*
	if(colorspace_in == RGB)
		return TRUE;
	else if((colorspace_in == YCbCr444) && edid->cea->YCC444)
		return TRUE;
	else if((colorspace_in == YCbCr422) && edid->cea->YCC422)
		return TRUE;
	else
*/
        return FALSE;
}

BOOL query_aspect_ratio(AspectRatio aspect_ratio)
{
	if (aspect_ratio == edid->st->imasp)
		return TRUE;
	else
		return FALSE;
}

BOOL query_sampling_frequency(SamplingFrequency sampling_frequency)
{
	DEPLINT i;

	for (i = 0; i < SAD_MAX; i++)
	{
		if ((sampling_frequency == _32k) && edid->cea->cea->sad[i].khz_32)
			return TRUE;
		if ((sampling_frequency == _44k) && edid->cea->cea->sad[i].khz_44)
			return TRUE;
		if ((sampling_frequency == _48k) && edid->cea->cea->sad[i].khz_48)
			return TRUE;
		if ((sampling_frequency == _88k) && edid->cea->cea->sad[i].khz_88)
			return TRUE;
		if ((sampling_frequency == _96k) && edid->cea->cea->sad[i].khz_96)
			return TRUE;
		if ((sampling_frequency == _176k) && edid->cea->cea->sad[i].khz_176)
			return TRUE;
		if ((sampling_frequency == _192k) && edid->cea->cea->sad[i].khz_192)
			return TRUE;
	}
	return FALSE;
}

BOOL query_bit_width(DEPLINT bit_width)
{
	DEPLINT i;

	for (i = 0; i < SAD_MAX; i++)
	{
		if ((bit_width == 16) && edid->cea->cea->sad[i].unc_16bit)
			return TRUE;
		if ((bit_width == 20) && edid->cea->cea->sad[i].unc_20bit)
			return TRUE;
		if ((bit_width == 24) && edid->cea->cea->sad[i].unc_24bit)
			return TRUE;
	}
	return FALSE;
}

BOOL query_channel_count(DEPLINT channel_count)
{
	DEPLINT i;

	for (i = 0; i < SAD_MAX; i++)
	{
		if (channel_count <= (edid->cea->cea->sad[i].max_num_chan)+1)
			return TRUE;
	}
	return FALSE;
}

BOOL query_channel_map(DEPLINT channel_map)
{
	if (channel_map & 1)
	{
		if (!(edid->cea->cea->spad->fl_fr))
			return FALSE;
	}
	if (channel_map & 2)
	{
		if (!(edid->cea->cea->spad->lfe))
			return FALSE;
	}
	if (channel_map & 4)
	{
		if (!(edid->cea->cea->spad->fc))
			return FALSE;
	}
	if (channel_map & 8)
	{
		if (!(edid->cea->cea->spad->rl_rr))
			return FALSE;
	}
	if (channel_map & 16)
	{
		if (!(edid->cea->cea->spad->rc))
			return FALSE;
	}
	if (channel_map & 32)
	{
		if (!(edid->cea->cea->spad->flc_frc))
			return FALSE;
	}
	if (channel_map & 64)
	{
		if (!(edid->cea->cea->spad->rlc_rrc))
			return FALSE;
	}
	return TRUE;
}

void hdmi_or_dvi()
{
	if (edid_ready)
	{
		if (edid->cea->cea->vsdb_hdmi)
		{
			printk ("Setting mode to HDMI\n");
			ADI89_SET_EXT_HDMI_MODE(1);
		}
		else
		{
			printk ("Setting mode to DVI\n");
			ADI89_SET_EXT_HDMI_MODE(0);
		}
	}
	else
	{
		printk ("Setting mode to DVI - But this is fallback !!!\n");
		ADI89_SET_EXT_HDMI_MODE(0);
	}
}

void force_hdmi(BOOL hdmi)
{
	ADI89_SET_EXT_HDMI_MODE(hdmi);
}

BOOL check_Rx_sense()
{
	return ADI89_GET_RX_SENSE;
}

void set_avi_if(AVIInfoframe info)
{
	ADI89_SET_A0(info->active_format);
	ADI89_SET_B1B0(info->bar_info);
	ADI89_SET_S1S0(info->scan_info);
	ADI89_SET_C1C0(info->colorimetry);
	ADI89_SET_SC(info->non_unif_pic_scaling);
	ADI89_SET_R30(info->active_fmt_asp_rat);
	ADI89_SET_AVI_LINE_START(info->active_line_start);
	ADI89_SET_AVI_LINE_END(info->active_line_end);
	ADI89_SET_AVI_PIXEL_START(info->active_pixel_start);
	ADI89_SET_AVI_PIXEL_END(info->active_pixel_end);
	ADI89_SET_AVIIF_PKT_EN(1);
}

void set_audio_if(AudioInfoframe info)
{
	ADI89_SET_AUDIO_IF_DM_INH(info->down_mix_inh);
	ADI89_SET_AUDIO_IF_LSV(info->level_shift);
	ADI89_SET_AUDIOIF_PKT_EN(1);
}

void set_spd_packet(SPDPacket info)
{
	i2c_write_bytes(AD9889_ADDRESS, 0x52, info->spd_byte_1to25, 25);
	ADI89_SET_SPD_PKT_EN(1);
}

void set_mpeg_packet(MPEGPacket info)
{
	i2c_write_bytes(AD9889_ADDRESS, 0x6B, info->mpeg_byte_0to3, 4);
	ADI89_SET_MPEG_FR(info->frame_repeat);
	ADI89_SET_MPEG_MF(info->frame_indicator);
	ADI89_SET_MPEG_PKT_EN(1);
}

void set_acp_packet(ACPPacket info)
{
	ADI89_SET_ACP_TYPE(info->acp_type);
	ADI89_SET_ACP_BYTE1(info->acp_byte_1);
	ADI89_SET_ACP_PKT_EN(1);
}

void set_isrc_packet(ISRCPacket info)
{
	ADI89_SET_ISRC_CONT(info->isrc_continued);
	ADI89_SET_ISRC_VALID(info->isrc_valid);
	ADI89_SET_ISRC_STATUS(info->isrc_status);
	i2c_write_bytes(AD9889_ADDRESS, 0x74, info->isrc_1, 16);
	i2c_write_bytes(AD9889_ADDRESS, 0x84, info->isrc_2, 16);
	ADI89_SET_ISRC_PKT_EN(1);
}

void set_sync_params(DEPLINT h_dur, DEPLINT h_fp, DEPLINT v_dur, DEPLINT v_fp,DEPLINT h_pol,DEPLINT v_pol)
{
	ADI89_SET_VFE_HS_PLA(h_fp);
	ADI89_SET_VFE_HS_DUR(h_dur);
	ADI89_SET_VFE_VS_PLA(v_fp);
	ADI89_SET_VFE_VS_DUR(v_dur);
	ADI89_SET_ITU_HYSNC_POL(h_pol);
	ADI89_SET_ITU_VSYNC_POL(v_pol);
	ADI89_SET_SYNC_GEN_EN(1);
}

void set_clock_delay(DEPLINT setting)
{
	ADI89_SET_CLK_DELAY(setting);
}

void input_data_clock_2x(BOOL value)
{
	BYTE x = 0x04;
	BYTE y = 0x40;

	if (value == TRUE)
	{
		ADI89_SET_CLK_DELAY(0);
		i2c_write_bytes(AD9889_ADDRESS,0x9D,&x,1);
		i2c_write_bytes(AD9889_ADDRESS,0xA4,&y,1);
	}
	else
	{
		ADI89_SET_CLK_DELAY(3);
		i2c_write_bytes(AD9889_ADDRESS,0x9D,&x,1);
		i2c_write_bytes(AD9889_ADDRESS,0xA4,&y,1);
	}
}

DEPLINT get_version()
{
	return 0x12;
}

void enable_sync_gen(BOOL val)
{
	ADI89_SET_SYNC_GEN_EN(val);
}

void enable_de_gen(BOOL val)
{
	ADI89_SET_DEGEN_EN(val);
}

void set_de_params(DEPLINT h_delay, DEPLINT v_delay, DEPLINT int_offset,DEPLINT act_width,DEPLINT act_height)
{
	ADI89_SET_VFE_HSYNCDELAYIN(h_delay);
	ADI89_SET_VFE_VSYNCDELAYIN(v_delay);
	ADI89_SET_VFE_OFFSET(int_offset);
	ADI89_SET_VFE_WIDTH(act_width);
	ADI89_SET_VFE_HEIGHT(act_height);
	ADI89_SET_DEGEN_EN(1);
}

void AD9889_audio_mute(BOOL val)
{

}
