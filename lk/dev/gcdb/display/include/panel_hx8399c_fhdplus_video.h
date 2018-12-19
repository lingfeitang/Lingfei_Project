/* Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PANEL_HX8399C_fhdplusPLUS_VIDEO_H_

#define _PANEL_HX8399C_fhdplusPLUS_VIDEO_H_
/*---------------------------------------------------------------------------*/
/* HEADER files                                                              */
/*---------------------------------------------------------------------------*/
#include "panel.h"

/*---------------------------------------------------------------------------*/
/* Panel configuration                                                       */
/*---------------------------------------------------------------------------*/

static struct panel_config hx8399c_fhdplus_video_panel_data = {
  "qcom,mdss_dsi_hx8399c_fhdplus_video", "dsi:0:", "qcom,mdss-dsi-panel",
  10, 0, "DISPLAY_1", 0, 0, 60, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, NULL
};

/*---------------------------------------------------------------------------*/
/* Panel resolution                                                          */
/*---------------------------------------------------------------------------*/
static struct panel_resolution hx8399c_fhdplus_video_panel_res = {
  1080, 2280, 24, 40, 40, 0, 9, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel Color Information                                                   */
/*---------------------------------------------------------------------------*/
static struct color_info hx8399c_fhdplus_video_color = {
  24, 0, 0xff, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel Command information                                                 */
/*---------------------------------------------------------------------------*/
static char hx8399c_fhdplus_video_on_cmd0[] = {
0x04, 0x00, 0x39, 0xC0,
0xB9, 0xFF, 0x83, 0x99, };

static char hx8399c_fhdplus_video_on_cmd1[] = {
0x02, 0x00, 0x39, 0xC0,
0x11, 0x00, 0xFF, 0xFF, };

static char hx8399c_fhdplus_video_on_cmd2[] = {
0x02, 0x00, 0x39, 0xC0,
0x29, 0x00, 0xFF, 0xFF, };

static char hx8399c_fhdplus_video_on_cmd3[] = {
0x03, 0x00, 0x39, 0xC0,
0x51, 0x0F, 0xFF, 0xFF, };

static char hx8399c_fhdplus_video_on_cmd4[] = {
0x0A, 0x00, 0x39, 0xC0,
0xC9, 0x03, 0x00, 0x16,
0x1E, 0x31, 0x1E, 0x00,
0x91, 0x00, 0xFF, 0xFF, };

static char hx8399c_fhdplus_video_on_cmd5[] = {
0x02, 0x00, 0x39, 0xC0,
0x55, 0x01, 0xFF, 0xFF, };

static char hx8399c_fhdplus_video_on_cmd6[] = {
0x02, 0x00, 0x39, 0xC0,
0x53, 0x2C, 0xFF, 0xFF, };

static char hx8399c_fhdplus_video_on_cmd7[] = {
0x0A, 0x00, 0x39, 0xC0,
0xCA, 0x24, 0x23, 0x23,
0x21, 0x23, 0x21, 0x20,
0x20, 0x20, 0xFF, 0xFF, };

static struct mipi_dsi_cmd hx8399c_fhdplus_video_on_command[] = {
{0x08, hx8399c_fhdplus_video_on_cmd0, 0x00},
{0x08, hx8399c_fhdplus_video_on_cmd1, 0x78},
{0x08, hx8399c_fhdplus_video_on_cmd2, 0x14},
{0x08, hx8399c_fhdplus_video_on_cmd3, 0x01},
{0x10, hx8399c_fhdplus_video_on_cmd4, 0x01},
{0x08, hx8399c_fhdplus_video_on_cmd5, 0x05},
{0x08, hx8399c_fhdplus_video_on_cmd6, 0x01},
{0x10, hx8399c_fhdplus_video_on_cmd7, 0x01},
};
#define HX8399C_FHDPLUS_VIDEO_ON_COMMAND 8


static char hx8399c_fhdplus_video_off_cmd0[] = {
0x02, 0x00, 0x39, 0xC0,
0x28, 0x00, 0xFF, 0xFF, };

static char hx8399c_fhdplus_video_off_cmd1[] = {
0x02, 0x00, 0x39, 0xC0,
0x10, 0x00, 0xFF, 0xFF, };

static struct mipi_dsi_cmd hx8399c_fhdplus_video_off_command[] = {
{ 0x8 , hx8399c_fhdplus_video_off_cmd0, 0x32},
{ 0x8 , hx8399c_fhdplus_video_off_cmd1, 0x78},
};
#define HX8399C_FHDPLUS_VIDEO_OFF_COMMAND 2


static struct command_state hx8399c_fhdplus_video_state = {
  0, 0
};

/*---------------------------------------------------------------------------*/
/* Command mode panel information                                            */
/*---------------------------------------------------------------------------*/

static struct commandpanel_info hx8399c_fhdplus_video_command_panel = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Video mode panel information                                              */
/*---------------------------------------------------------------------------*/

static struct videopanel_info hx8399c_fhdplus_video_video_panel = {
  0, 0, 0, 0, 1, 1, 1, 0, 0x9
};

/*---------------------------------------------------------------------------*/
/* Lane Configuration                                                        */
/*---------------------------------------------------------------------------*/

static struct lane_configuration hx8399c_fhdplus_video_lane_config = {
  4, 0, 1, 1, 1, 1, 0
};


/*---------------------------------------------------------------------------*/
/* Panel Timing                                                              */
/*---------------------------------------------------------------------------*/
const uint32_t hx8399c_fhdplus_video_timings[] = {
  //0xE6, 0x38, 0x26, 0x00, 0x68, 0x6e, 0x2A, 0x3c, 0x44, 0x03, 0x04, 0x00
  0x29, 0x40, 0x2c, 0x01, 0x76, 0x7a, 0x30, 0x44, 0x34, 0x03, 0x04, 0x00
};

static struct panel_timing hx8399c_fhdplus_video_timing_info_8937 = {
  //0, 4, 0x20, 0x2c
   0, 4, 0x0e, 0x34
};

static const uint32_t hx8399c_fhdplus_14nm_video_timings[] = {
	0x25, 0x1f, 0x9, 0xa, 0x6, 0x3, 0x4, 0xa0,
	0x25, 0x1f, 0x9, 0xa, 0x6, 0x3, 0x4, 0xa0,
	0x25, 0x1f, 0x9, 0xa, 0x6, 0x3, 0x4, 0xa0,
	0x25, 0x1f, 0x9, 0xa, 0x6, 0x3, 0x4, 0xa0,
	0x25, 0x1e, 0x8, 0xa, 0x6, 0x3, 0x4, 0xa0,
};

static struct panel_timing hx8399c_fhdplus_video_timing_info = {
  //0, 4, 0x20, 0x2c
   0, 4, 0x0e, 0x34
};

static struct panel_reset_sequence hx8399c_fhdplus_video_panel_reset_seq = {
{ 1, 0, 1}, { 10, 10, 50 }, 2
};

/*---------------------------------------------------------------------------*/
/* Dynamic fps supported frequencies by panel                                */
/*---------------------------------------------------------------------------*/
static const struct dfps_panel_info hx8399c_fhdplus_video_dfps = {
	1, 13, {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60}
};

/*---------------------------------------------------------------------------*/
/* Backlight Settings                                                        */
/*---------------------------------------------------------------------------*/

static struct backlight hx8399c_fhdplus_video_backlight = {
  1, 1, 4095, 100, 1, "PMIC_8941"
};

#define HX8399C_FHDPLUS_VIDEO_SIGNATURE 0xFFFF

#endif /*_TRULY_1080P_VIDEO_H_*/
