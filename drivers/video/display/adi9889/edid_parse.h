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
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
* THIS IS A READ-ONLY SOURCE FILE. DO NOT EDIT IT DIRECTLY.                   *
* Author: Matthew McCarn (matthew.mccarn@analog.com)                          *
* Date: May 16, 2006                                                          *
*                                                                             *
******************************************************************************/
#include "type_def.h"
#include "ad9889_macros.h"
#ifndef _EDID_PARSE_H_
#define _EDID_PARSE_H_

typedef struct MyVendorProductId *VendorProductId;

struct MyVendorProductId
{
	char manuf[4];
	BOOL prodcode[3];
	WORD serial[2];
	DEPLINT week;
	DEPLINT year;
};

typedef struct MyEdidVerRev *EdidVerRev;

struct MyEdidVerRev
{
	DEPLINT ver;
	DEPLINT rev;
};

typedef struct MyBasicDispParams *BasicDispParams;

struct MyBasicDispParams
{
	BOOL anadig;
	DEPLINT siglev;
	BOOL setup;
	BOOL sepsyn;
	BOOL compsync;
	BOOL sog;
	BOOL vsync_ser;
	BOOL dfp1x;
	DEPLINT maxh;
	DEPLINT maxv;
	DEPLINT dtc_gam;
	BOOL standby;
	BOOL suspend;
	BOOL aovlp;
	DEPLINT disptype;
	BOOL def_col_sp;
	BOOL pt_mode;
	BOOL def_gtf;
};

typedef struct MyColorCharacteristics *ColorCharacteristics;

struct MyColorCharacteristics
{
	DEPLINT rx;
	DEPLINT ry;
	DEPLINT gx;
	DEPLINT gy;
	DEPLINT bx;
	DEPLINT by;
	DEPLINT wx;
	DEPLINT wy;
};

typedef struct MyEstablishedTiming *EstablishedTiming;

struct MyEstablishedTiming
{
	// Established Timing I
	BOOL m720_400_70;
	BOOL m720_400_88;
	BOOL m640_480_60;
	BOOL m640_480_67;
	BOOL m640_480_72;
	BOOL m640_480_75;
	BOOL m800_600_56;
	BOOL m800_600_60;
	// Established Timing II
	BOOL m800_600_72;
	BOOL m800_600_75;
	BOOL m832_624_75;
	BOOL m1024_768_87;
	BOOL m1024_768_60;
	BOOL m1024_768_70;
	BOOL m1024_768_75;
	BOOL m1280_1024_75;
	// Manufacturer's Timings
	BOOL m1152_870_75;
	// Reserved
	BOOL res_7;
	BOOL res_6;
	BOOL res_5;
	BOOL res_4;
	BOOL res_3;
	BOOL res_2;
	BOOL res_1;
};

typedef struct MyStandardTimingList *StandardTimingList;

struct MyStandardTimingList
{
	DEPLINT hap;
	DEPLINT imasp;
	DEPLINT ref;
};

typedef struct MyDetailedTimingBlockList *DetailedTimingBlockList;

struct MyDetailedTimingBlockList
{
	BOOL preferred_timing;
	DEPLINT pix_clk;
	DEPLINT h_active;
	DEPLINT h_blank;
	DEPLINT h_act_blank;
	DEPLINT v_active;
	DEPLINT v_blank;
	DEPLINT v_act_blank;
	DEPLINT hsync_offset;
	DEPLINT hsync_pw;
	DEPLINT vsync_offset;
	DEPLINT vsync_pw;
	DEPLINT h_image_size;
	DEPLINT v_image_size;
	DEPLINT h_border;
	DEPLINT v_border;
	BOOL ilace;
	DEPLINT stereo;
	DEPLINT comp_sep;
	BOOL serr_v_pol;
	BOOL sync_comp_h_pol;
};

typedef struct MyMonitorDescriptorBlock *MonitorDescriptorBlock;

struct MyMonitorDescriptorBlock
{
	char mon_snum[14];
	char ascii_data[14];
	DEPLINT mrl_min_vrate;
	DEPLINT mrl_max_vrate;
	DEPLINT mrl_min_hrate;
	DEPLINT mrl_max_hrate;
	DEPLINT mrl_max_pix_clk;
	DEPLINT mrl_gtf_support;
	BOOL mrl_bad_stf;
	DEPLINT mrl_reserved_11;
	DEPLINT mrl_gtf_start_freq;
	DEPLINT mrl_gtf_c;
	DEPLINT mrl_gtf_m;
	DEPLINT mrl_gtf_k;
	DEPLINT mrl_gtf_j;
	char mon_name[14];
	DEPLINT cp_wpoint_index1;
	DEPLINT cp_w_lb1;
	BOOL cp_w_x1;
	BOOL cp_w_y1;
	BOOL cp_w_gam1;
	DEPLINT cp_wpoint_index2;
	DEPLINT cp_w_lb2;
	BOOL cp_w_x2;
	BOOL cp_w_y2;
	BOOL cp_w_gam2;
	BOOL cp_bad_set;
	DEPLINT  cp_pad1;
	DEPLINT cp_pad2;
	DEPLINT cp_pad3;
	DEPLINT st9_hap;
	DEPLINT st9_imasp;
	DEPLINT st9_ref;
	DEPLINT st10_hap;
	DEPLINT st10_imasp;
	DEPLINT st10_ref;
	DEPLINT st11_hap;
	DEPLINT st11_imasp;
	DEPLINT st11_ref;
	DEPLINT st12_hap;
	DEPLINT st12_imasp;
	DEPLINT st12_ref;
	DEPLINT st13_hap;
	DEPLINT st13_imasp;
	DEPLINT st13_ref;
	DEPLINT st14_hap;
	DEPLINT st14_imasp;
	DEPLINT st14_ref;
	BOOL st_set;
	DEPLINT ms_byte[13];
};

typedef struct MySVDescriptorList *SVDescriptorList;

struct MySVDescriptorList
{
	BOOL native;
	DEPLINT vid_id;
};

typedef struct MySADescriptorList *SADescriptorList;

struct MySADescriptorList
{
	BOOL rsvd1;
	DEPLINT aud_format;
	DEPLINT max_num_chan;
	BOOL rsvd2;
	BOOL khz_192;
	BOOL khz_176;
	BOOL khz_96;
	BOOL khz_88;
	BOOL khz_48;
	BOOL khz_44;
	BOOL khz_32;
	DEPLINT unc_rsrvd;
	BOOL unc_24bit;
	BOOL unc_20bit;
	BOOL unc_16bit;
	DEPLINT comp_maxbitrate;
	BYTE sample_sizes;
	BYTE sample_rates;
};

typedef struct MySpadPayload *SpadPayload;

struct MySpadPayload
{
	BOOL rlc_rrc;
	BOOL flc_frc;
	BOOL rc;
	BOOL rl_rr;
	BOOL fc;
	BOOL lfe;
	BOOL fl_fr;
};

typedef struct MyCeaDataBlock *CeaDataBlock;

struct MyCeaDataBlock
{
	struct MySVDescriptorList svd[SVD_MAX];
	struct MySADescriptorList sad[SAD_MAX];
	SpadPayload spad;
	WORD ieee_reg[2];
	BOOL vsdb_hdmi;
	char *vsdb_payload;
	DEPLINT hdmi_addr_a;
	DEPLINT hdmi_addr_b;
	DEPLINT hdmi_addr_c;
	DEPLINT hdmi_addr_d;
	DEPLINT vsdb_ext[26];
	BOOL supports_ai;
};

typedef struct MyCeaTimingExtension *CeaTimingExtension;

struct MyCeaTimingExtension
{
	DEPLINT cea_rev;
	BOOL underscan;
	BOOL audio;
	BOOL YCC444;
	BOOL YCC422;
	DEPLINT native_formats;
	CeaDataBlock cea;
	DEPLINT checksum;
};

typedef struct MyEdidInfo *EdidInfo;

struct MyEdidInfo{
	BOOL edid_header;
	DEPLINT edid_extensions;
	DEPLINT edid_checksum;
	DEPLINT ext_block_count;
	VendorProductId vpi;
	EdidVerRev evr;
	BasicDispParams bdp;
	ColorCharacteristics cc;
	EstablishedTiming et;
	struct MyStandardTimingList st[ST_MAX];
	struct MyDetailedTimingBlockList dtb[DTD_MAX];
	MonitorDescriptorBlock mdb;
	CeaTimingExtension cea;

};

DEPLINT parse_edid(EdidInfo e,BYTE *d);
EdidInfo init_edid_info(EdidInfo edid);
void clear_edid(EdidInfo e);

#endif
