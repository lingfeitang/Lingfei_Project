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

#ifndef _PANEL_NT36672_TIANMA_FHDPLUS_VIDEO_E7_H_

#define _PANEL_NT36672_TIANMA_FHDPLUS_VIDEO_E7_H_
/*---------------------------------------------------------------------------*/
/* HEADER files                                                              */
/*---------------------------------------------------------------------------*/
#include "panel.h"

/*---------------------------------------------------------------------------*/
/* Panel configuration                                                       */
/*---------------------------------------------------------------------------*/

static struct panel_config nt36672_tianma_fhdplus_video_e7_panel_data = {
  "qcom,mdss_dsi_nt36672_tianma_fhdplus_video_e7", "dsi:0:", "qcom,mdss-dsi-panel",
  10, 0, "DISPLAY_1", 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, NULL
};

/*---------------------------------------------------------------------------*/
/* Panel resolution                                                          */
/*---------------------------------------------------------------------------*/
static struct panel_resolution nt36672_tianma_fhdplus_video_e7_panel_res = {
  1080, 2160, 16, 64, 20, 0, 4, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel Color Information                                                   */
/*---------------------------------------------------------------------------*/
static struct color_info nt36672_tianma_fhdplus_video_e7_color = {
  24, 0, 0xff, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel Command information                                                 */
/*---------------------------------------------------------------------------*/
static char nt36672_tianma_fhdplus_video_e7_on_cmd0[] = { 
0x02, 0x00, 0x29, 0xC0,
0xff, 0x10, 0xFF, 0xFF, };

static char nt36672_tianma_fhdplus_video_e7_on_cmd1[] = { 
0x02, 0x00, 0x29, 0xC0,
0x35, 0x00, 0xFF, 0xFF, };

static char nt36672_tianma_fhdplus_video_e7_on_cmd2[] = { 
0x02, 0x00, 0x29, 0xC0,
0x51, 0xff, 0xFF, 0xFF, };

static char nt36672_tianma_fhdplus_video_e7_on_cmd3[] = { 
0x02, 0x00, 0x29, 0xC0,
0x53, 0x2c, 0xFF, 0xFF, };

static char nt36672_tianma_fhdplus_video_e7_on_cmd4[] = { 
0x02, 0x00, 0x29, 0xC0,
0x55, 0x00, 0xFF, 0xFF, };

static char nt36672_tianma_fhdplus_video_e7_on_cmd5[] = { 
0x02, 0x00, 0x29, 0xC0,
0xff, 0x10, 0xFF, 0xFF, };

static char nt36672_tianma_fhdplus_video_e7_on_cmd6[] = { 
0x02, 0x00, 0x29, 0xC0,
0x11, 0x00, 0xFF, 0xFF, };

static char nt36672_tianma_fhdplus_video_e7_on_cmd7[] = { 
0x02, 0x00, 0x29, 0xC0,
0x29, 0x00, 0xFF, 0xFF, };

static struct mipi_dsi_cmd nt36672_tianma_fhdplus_video_e7_on_command[] = {
{0x08, nt36672_tianma_fhdplus_video_e7_on_cmd0, 0x00},
{0x08, nt36672_tianma_fhdplus_video_e7_on_cmd1, 0x00},
{0x08, nt36672_tianma_fhdplus_video_e7_on_cmd2, 0x00},
{0x08, nt36672_tianma_fhdplus_video_e7_on_cmd3, 0x00},
{0x08, nt36672_tianma_fhdplus_video_e7_on_cmd4, 0x00},
{0x08, nt36672_tianma_fhdplus_video_e7_on_cmd5, 0x00},
{0x08, nt36672_tianma_fhdplus_video_e7_on_cmd6, 0x78},
{0x08, nt36672_tianma_fhdplus_video_e7_on_cmd7, 0x14}
};

#define NT36672_TIANMA_FHDPLUS_VIDEO_E7_ON_COMMAND 8


static char nt36672_tianma_fhdplus_video_e7_off_cmd0[] = {
0x28, 0x00, 0x05, 0x80 };

static char nt36672_tianma_fhdplus_video_e7_off_cmd1[] = {
0x10, 0x00, 0x05, 0x80 };


static struct mipi_dsi_cmd nt36672_tianma_fhdplus_video_e7_off_command[] = {
{ 0x4 , nt36672_tianma_fhdplus_video_e7_off_cmd0, 0x14},
{ 0x4 , nt36672_tianma_fhdplus_video_e7_off_cmd1, 0x78}
};
#define NT36672_TIANMA_FHDPLUS_VIDEO_E7_OFF_COMMAND 2

 
static struct command_state nt36672_tianma_fhdplus_video_e7_state = {
  0, 1
};

/*---------------------------------------------------------------------------*/
/* Command mode panel information                                            */
/*---------------------------------------------------------------------------*/

static struct commandpanel_info nt36672_tianma_fhdplus_video_e7_command_panel = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Video mode panel information                                              */
/*---------------------------------------------------------------------------*/

static struct videopanel_info nt36672_tianma_fhdplus_video_e7_video_panel = {
  0, 0, 0, 0, 1, 1, 1, 0, 0x9
};

/*---------------------------------------------------------------------------*/
/* Lane Configuration                                                        */
/*---------------------------------------------------------------------------*/

static struct lane_configuration nt36672_tianma_fhdplus_video_e7_lane_config = {
  4, 0, 1, 1, 1, 1, 0
};


/*---------------------------------------------------------------------------*/
/* Panel Timing                                                              */
/*---------------------------------------------------------------------------*/
const uint32_t nt36672_tianma_fhdplus_video_e7_timings[] = {
  0xE7, 0x36, 0x24, 0x00, 0x66, 0x6a, 0x2A, 0x3a, 0x2d, 0x03, 0x04, 0x00
};

static struct panel_timing nt36672_tianma_fhdplus_video_e7_timing_info_8937 = {
   0, 4, 0x03, 0x2d
};

static const uint32_t nt36672_tianma_fhdplus_14nm_video_e7_timings[] = {
	0x24, 0x1F, 0x8, 0x9, 0x5, 0x3, 0x4, 0xa0,
	0x24, 0x1F, 0x8, 0x9, 0x5, 0x3, 0x4, 0xa0,
	0x24, 0x1F, 0x8, 0x9, 0x5, 0x3, 0x4, 0xa0,
	0x24, 0x1F, 0x8, 0x9, 0x5, 0x3, 0x4, 0xa0,
	0x24, 0x1C, 0x8, 0x9, 0x5, 0x3, 0x4, 0xa0,
};

static struct panel_timing nt36672_tianma_fhdplus_video_e7_timing_info = {
   0, 4, 0x0E, 0x31
};

static struct panel_reset_sequence nt36672_tianma_fhdplus_video_e7_panel_reset_seq = {
{ 1, 0, 1}, { 10, 10, 10 }, 2
};

/*---------------------------------------------------------------------------*/
/* Dynamic fps supported frequencies by panel                                */
/*---------------------------------------------------------------------------*/
static const struct dfps_panel_info nt36672_tianma_fhdplus_video_e7_dfps = {
	1, 13, {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60}
};

/*---------------------------------------------------------------------------*/
/* Backlight Settings                                                        */
/*---------------------------------------------------------------------------*/

static struct backlight nt36672_tianma_fhdplus_video_e7_backlight = {
  1, 1, 4095, 100, 1, "PMIC_8941"
};

#define NT36672_TIANMA_fhdplus_VIDEO_E7_SIGNATURE 0xFFFF

#endif /*_TRULY_1080P_VIDEO_H_*/
