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

#ifndef _RAPTOR_SETUP_H_
#define _RAPTOR_SETUP_H_

#include "type_def.h"
#include "ad9889_macros.h"

enum mytiming
{
	AD9889_480i,
	AD9889_720_480p,
	AD9889_720p,
	AD9889_1080i,
	AD9889_640_480p,
};

typedef enum mytiming Timing;

enum mycolorspace
{
	RGB,
	YCbCr601,
	YCbCr709,
};

typedef enum mycolorspace Colorspace;

enum myrange
{
	AD9889_0_255,
	AD9889_16_235,
};

typedef enum myrange Range;

enum myaspectratio
{
	_4x3,
	_16x9,
};

typedef enum myaspectratio AspectRatio;

enum mysamplingfrequency
{
	_32k,
	_44k,
	_48k,
	_88k,
	_96k,
	_176k,
	_192k,
};

typedef enum mysamplingfrequency SamplingFrequency;

enum myaudioinputmode
{
	SPDIF,
	I2S,
	I2S_LEFT,
	I2S_RIGHT,
};

typedef enum myaudioinputmode AudioInputMode;

struct aviinfoframe
{
	DEPLINT	active_format;
	DEPLINT	bar_info;
	DEPLINT	scan_info;
	DEPLINT	colorimetry;
	DEPLINT	non_unif_pic_scaling;
	DEPLINT	active_fmt_asp_rat;
	DEPLINT	active_line_start;
	DEPLINT	active_line_end;
	DEPLINT	active_pixel_start;
	DEPLINT	active_pixel_end;
};

typedef struct aviinfoframe * AVIInfoframe;

struct audioinfoframe
{
	DEPLINT	down_mix_inh;
	DEPLINT	level_shift;
};

typedef struct audioinfoframe * AudioInfoframe;

struct spdpacket
{
	BYTE	*spd_byte_1to25;
};

typedef struct spdpacket * SPDPacket;

struct mpegpacket
{
	BYTE	*mpeg_byte_0to3;
	DEPLINT	frame_repeat;
	DEPLINT	frame_indicator;
};

typedef struct mpegpacket * MPEGPacket;

struct acppacket
{
	DEPLINT	acp_type;
	DEPLINT	acp_byte_1;
};

typedef struct acppacket * ACPPacket;

struct isrcpacket
{
	DEPLINT	isrc_continued;
	DEPLINT	isrc_valid;
	DEPLINT	isrc_status;
	BYTE	*isrc_1;
	BYTE	*isrc_2;
};

typedef struct isrcpacket * ISRCPacket;

void initialize_raptor(void);
void set_spdif_audio(void);
void adi9889_early_setup(void);
void hdmi_or_dvi(void);
DEPLINT handle_edid_interrupt(void);
void handle_bksv_ready_interrupt(void);
void handle_hpd_interrupt(void);
void enable_hdcp(void);
void disable_hdcp(void);
BOOL get_hdcp_status(void);
void set_hdcp_status(BOOL);
BOOL get_hdcp_hdcp_requested(void);
void set_hdcp_hdcp_requested(BOOL);
BOOL get_hdcp_hdcp_enabled(void);
void set_hdcp_hdcp_enabled(BOOL);
BOOL get_hdmi_status(void);
void compare_bksvs(void);

//since a polling function is needed

void initialize_ad9889_i2c(DEPLINT (*)(DEPLINT, DEPLINT, BYTE *, DEPLINT), DEPLINT (*)(DEPLINT, DEPLINT, BYTE *, DEPLINT));
void initialize_error_callback(void (*error_function)(int));

extern void (*error_callback_function)(int);
extern DEPLINT (*i2c_write_bytes)(DEPLINT dev_addr_in, DEPLINT reg_addr_in, BYTE *data_in, DEPLINT count_in);
extern DEPLINT (*i2c_read_bytes)(DEPLINT dev_addr_in, DEPLINT reg_addr_in, BYTE *data_out, DEPLINT count_in);

BOOL is_repeater(void);
BOOL edid_reading_complete(void);
void set_EDID_ready(BOOL value);
void set_video_mode(Timing video_timing, Colorspace colorspace_in, Range colorspace_range, AspectRatio aspect_ratio);
void set_input_format(DEPLINT input_format, DEPLINT input_style, DEPLINT input_bit_width, DEPLINT clock_edge);
void set_audio_mode(SamplingFrequency sampling_frequency, DEPLINT bit_width, DEPLINT channel_count, DEPLINT channel_map);
void set_audio_format(AudioInputMode input_mode);
void force_hdmi(BOOL hdmi);
void clear_interrupts(void);
void set_MCLK_ratio(DEPLINT val);
void set_avi_if(AVIInfoframe info);
void set_audio_if(AudioInfoframe info);
void set_spd_packet(SPDPacket info);
void set_mpeg_packet(MPEGPacket info);
void set_acp_packet(ACPPacket info);
void set_isrc_packet(ISRCPacket info);
void set_sync_params(DEPLINT h_dur, DEPLINT h_fp, DEPLINT v_dur, DEPLINT v_fp,DEPLINT h_hpol,DEPLINT v_pol);
void set_clock_delay(DEPLINT setting);
void input_data_clock_2x(BOOL value);
DEPLINT get_version(void);

void av_mute_on(void);
void av_mute_off(void);

void AD9889_reset(void);
void AD9889_audio_mute(BOOL val);
void enable_sync_gen(BOOL val);
void enable_de_gen(BOOL val);
void set_de_params(DEPLINT h_delay, DEPLINT v_delay, DEPLINT int_offset,DEPLINT act_width,DEPLINT act_height);

BOOL check_Rx_sense(void);
BOOL query_vid(DEPLINT vid);
BOOL query_colorspace_in(Colorspace colorspace_in);
BOOL query_aspect_ratio(AspectRatio aspect_ratio);
BOOL query_sampling_frequency(SamplingFrequency sampling_frequency);
BOOL query_bit_width(DEPLINT bit_width);
BOOL query_channel_count(DEPLINT channel_count);
BOOL query_channel_map(DEPLINT channel_map);

#endif //_RAPTOR_SETUP_H_
