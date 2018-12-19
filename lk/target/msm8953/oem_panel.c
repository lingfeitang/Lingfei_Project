/* Copyright (c) 2016,2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <debug.h>
#include <string.h>
#include <err.h>
#include <smem.h>
#include <msm_panel.h>
#include <board.h>
#include <qtimer.h>
#include <mipi_dsi.h>
#include <mdp5.h>
#include <target/display.h>

#include "gcdb_display.h"
#include "include/panel.h"
#include "panel_display.h"
#include <platform/gpio.h> //for gpio control

/*---------------------------------------------------------------------------*/
/* GCDB Panel Database                                                       */
/*---------------------------------------------------------------------------*/
#include "include/panel_truly_1080p_video.h"
#include "include/panel_truly_1080p_cmd.h"
#include "include/panel_r69006_1080p_video.h"
#include "include/panel_r69006_1080p_cmd.h"
#include "include/panel_truly_wuxga_video.h"
//#include "include/panel_hx8399c_fhd_pluse_video.h"
#include "include/panel_hx8399c_hd_plus_video.h"
#include "include/panel_hx8399c_fhdplus_video.h"
#if defined(PROJECT_VINCE)
#include "include/panel_td4310_fhdplus_video_e7.h"
#include "include/panel_td4310_fhdplus_video_e7_g55.h"
#include "include/panel_td4310_ebbg_fhdplus_video_e7.h"
#include "include/panel_nt36672_tianma_fhdplus_video_e7.h"
#include "include/panel_nt36672_csot_fhdplus_video_e7.h"
#endif

/*Rainbow first LCD */
#include "include/panel_otm1911_fhd_video.h"
#include "include/panel_otm1911_fhdplus_video.h"
#include "include/panel_ili7807_fhdplus_video.h"
/*---------------------------------------------------------------------------*/
/* static panel selection variable                                           */
/*---------------------------------------------------------------------------*/
enum {
#if defined(PROJECT_VINCE)
	TD4310_FHDPLUS_VIDEO_E7_PANEL,
	TD4310_FHDPLUS_VIDEO_E7_G55_PANEL,
	TD4310_EBBG_FHDPLUS_VIDEO_E7_PANEL,
	NT36672_TIANMA_FHDPLUS_VIDEO_E7_PANEL,
	NT36672_CSOT_FHDPLUS_VIDEO_E7_PANEL,
#endif
	OTM1911_FHD_VIDEO_PANEL,
	OTM1911_FHDPLUS_VIDEO_PANEL,
	ILI7807_FHDPLUS_VIDEO_PANEL,


	TRULY_1080P_VIDEO_PANEL,
	TRULY_1080P_CMD_PANEL,
	R69006_1080P_VIDEO_PANEL,
	R69006_1080P_CMD_PANEL,
	TRULY_WUXGA_VIDEO_PANEL,
	HX8399C_HD_PLUS_VIDEO_PANEL,
	HX8399C_FHDPLUS_VIDEO_PANEL,
	UNKNOWN_PANEL
};

static struct gpio_pin lcd_gpio66 = {
  "msmgpio", 66, 3, 1, 0, 0
};

static struct gpio_pin lcd_gpio94 = {
  "msmgpio", 94, 3, 1, 0, 0
};
int special_panel = UNKNOWN_PANEL;

int backlight_delay_flag =0;

#if defined(PROJECT_SAKURA)||defined(PROJECT_VINCE)||defined(PROJECT_DAISY)
int panel_sakura_id = UNKNOWN_PANEL;
#endif
/*
 * The list of panels that are supported on this target.
 * Any panel in this list can be selected using fastboot oem command.
 */
static struct panel_list supp_panels[] = {
#if defined(PROJECT_VINCE)
	{"td4310_fhdplus_video_e7", TD4310_FHDPLUS_VIDEO_E7_PANEL},
	{"td4310_fhdplus_video_e7_g55", TD4310_FHDPLUS_VIDEO_E7_G55_PANEL},
	{"td4310_ebbg_fhdplus_video_e7", TD4310_EBBG_FHDPLUS_VIDEO_E7_PANEL},
	{"nt36672_tianma_fhdplus_video_e7", NT36672_TIANMA_FHDPLUS_VIDEO_E7_PANEL},
	{"nt36672_csot_fhdplus_video_e7", NT36672_CSOT_FHDPLUS_VIDEO_E7_PANEL},
#endif
	{"truly_1080p_video", TRULY_1080P_VIDEO_PANEL},
	{"truly_1080p_cmd", TRULY_1080P_CMD_PANEL},
	{"r69006_1080p_video", R69006_1080P_VIDEO_PANEL},
	{"r69006_1080p_cmd", R69006_1080P_CMD_PANEL},
	{"truly_wuxga_video", TRULY_WUXGA_VIDEO_PANEL},
	{"otm1911_fhd_video", OTM1911_FHD_VIDEO_PANEL},
	{"otm1911_fhdplus_video", OTM1911_FHDPLUS_VIDEO_PANEL},
	{"ili7807_fhdplus_video", ILI7807_FHDPLUS_VIDEO_PANEL},
	{"hx8399c_fhdplus_video", HX8399C_FHDPLUS_VIDEO_PANEL},
	{"hx8399c_hd_plus_video", HX8399C_HD_PLUS_VIDEO_PANEL},
};

static uint32_t panel_id;

int oem_panel_rotation()
{
	return NO_ERROR;
}

#define TRULY_1080P_PANEL_ON_DELAY 40
int oem_panel_on()
{
	if (panel_id == TRULY_1080P_CMD_PANEL ||
			panel_id == TRULY_1080P_VIDEO_PANEL)
		mdelay(TRULY_1080P_PANEL_ON_DELAY);
	else if (panel_id == R69006_1080P_CMD_PANEL) {
		mdelay(R69006_1080P_CMD_PANEL_ON_DELAY);
	}

	return NO_ERROR;
}

int oem_panel_off()
{
	/* OEM can keep their panel specific off instructions
	 * in this function
	 */
	return NO_ERROR;
}

static int init_panel_data(struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	int pan_type = PANEL_TYPE_DSI;

	switch (panel_id) {
#if defined(PROJECT_VINCE)
	case TD4310_FHDPLUS_VIDEO_E7_PANEL:
	    panelstruct->paneldata    = &td4310_fhdplus_video_e7_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 0;
		panelstruct->panelres     = &td4310_fhdplus_video_e7_panel_res;
		panelstruct->color        = &td4310_fhdplus_video_e7_color;
		panelstruct->videopanel   = &td4310_fhdplus_video_e7_video_panel;
		panelstruct->commandpanel = &td4310_fhdplus_video_e7_command_panel;
		panelstruct->state        = &td4310_fhdplus_video_e7_state;
		panelstruct->laneconfig   = &td4310_fhdplus_video_e7_lane_config;
		panelstruct->paneltiminginfo
			= &td4310_fhdplus_video_e7_timing_info_8937;
		panelstruct->paneltiminginfo
			= &td4310_fhdplus_video_e7_timing_info;
		panelstruct->panelresetseq
					 = &td4310_fhdplus_video_e7_panel_reset_seq;
		panelstruct->backlightinfo = &td4310_fhdplus_video_e7_backlight;
		pinfo->mipi.panel_on_cmds
			= td4310_fhdplus_video_e7_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= TD4310_FHDPLUS_VIDEO_E7_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= td4310_fhdplus_video_e7_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= TD4310_FHDPLUS_VIDEO_E7_OFF_COMMAND;
		memcpy(phy_db->timing,
			td4310_fhdplus_14nm_video_e7_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->dfps.panel_dfps = td4310_fhdplus_video_e7_dfps;
		pinfo->mipi.signature 	= TD4310_FHDPLUS_VIDEO_E7_SIGNATURE;
	    break;
		
	case TD4310_FHDPLUS_VIDEO_E7_G55_PANEL:
		panelstruct->paneldata	  = &td4310_fhdplus_video_e7_g55_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 0;
		panelstruct->panelres	  = &td4310_fhdplus_video_e7_g55_panel_res;
		panelstruct->color		  = &td4310_fhdplus_video_e7_g55_color;
		panelstruct->videopanel   = &td4310_fhdplus_video_e7_g55_video_panel;
		panelstruct->commandpanel = &td4310_fhdplus_video_e7_g55_command_panel;
		panelstruct->state		  = &td4310_fhdplus_video_e7_g55_state;
		panelstruct->laneconfig   = &td4310_fhdplus_video_e7_g55_lane_config;
		panelstruct->paneltiminginfo
			= &td4310_fhdplus_video_e7_g55_timing_info_8937;
		panelstruct->paneltiminginfo
			= &td4310_fhdplus_video_e7_g55_timing_info;
		panelstruct->panelresetseq
					 = &td4310_fhdplus_video_e7_g55_panel_reset_seq;
		panelstruct->backlightinfo = &td4310_fhdplus_video_e7_g55_backlight;
		pinfo->mipi.panel_on_cmds
			= td4310_fhdplus_video_e7_g55_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= TD4310_FHDPLUS_VIDEO_E7_G55_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= td4310_fhdplus_video_e7_g55_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= TD4310_FHDPLUS_VIDEO_E7_G55_OFF_COMMAND;
		memcpy(phy_db->timing,
			td4310_fhdplus_14nm_video_e7_g55_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->dfps.panel_dfps = td4310_fhdplus_video_e7_g55_dfps;
		pinfo->mipi.signature	= TD4310_FHDPLUS_VIDEO_E7_G55_SIGNATURE;
		break;

	case TD4310_EBBG_FHDPLUS_VIDEO_E7_PANEL:
	    panelstruct->paneldata    = &td4310_ebbg_fhdplus_video_e7_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 0;
		panelstruct->panelres     = &td4310_ebbg_fhdplus_video_e7_panel_res;
		panelstruct->color        = &td4310_ebbg_fhdplus_video_e7_color;
		panelstruct->videopanel   = &td4310_ebbg_fhdplus_video_e7_video_panel;
		panelstruct->commandpanel = &td4310_ebbg_fhdplus_video_e7_command_panel;
		panelstruct->state        = &td4310_ebbg_fhdplus_video_e7_state;
		panelstruct->laneconfig   = &td4310_ebbg_fhdplus_video_e7_lane_config;
		panelstruct->paneltiminginfo
			= &td4310_ebbg_fhdplus_video_e7_timing_info_8937;
		panelstruct->paneltiminginfo
			= &td4310_ebbg_fhdplus_video_e7_timing_info;
		panelstruct->panelresetseq
					 = &td4310_ebbg_fhdplus_video_e7_panel_reset_seq;
		panelstruct->backlightinfo = &td4310_ebbg_fhdplus_video_e7_backlight;
		pinfo->mipi.panel_on_cmds
			= td4310_ebbg_fhdplus_video_e7_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= TD4310_EBBG_FHDPLUS_VIDEO_E7_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= td4310_ebbg_fhdplus_video_e7_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= TD4310_EBBG_FHDPLUS_VIDEO_E7_OFF_COMMAND;
		memcpy(phy_db->timing,
			td4310_ebbg_fhdplus_14nm_video_e7_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->dfps.panel_dfps = td4310_ebbg_fhdplus_video_e7_dfps;
		pinfo->mipi.signature 	= TD4310_EBBG_FHDPLUS_VIDEO_E7_SIGNATURE;
	    break;

	case NT36672_TIANMA_FHDPLUS_VIDEO_E7_PANEL:
		panelstruct->paneldata    = &nt36672_tianma_fhdplus_video_e7_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 0;
		panelstruct->panelres     = &nt36672_tianma_fhdplus_video_e7_panel_res;
		panelstruct->color        = &nt36672_tianma_fhdplus_video_e7_color;
		panelstruct->videopanel   = &nt36672_tianma_fhdplus_video_e7_video_panel;
		panelstruct->commandpanel = &nt36672_tianma_fhdplus_video_e7_command_panel;
		panelstruct->state        = &nt36672_tianma_fhdplus_video_e7_state;
		panelstruct->laneconfig   = &nt36672_tianma_fhdplus_video_e7_lane_config;
		panelstruct->paneltiminginfo
			= &nt36672_tianma_fhdplus_video_e7_timing_info_8937;
		panelstruct->paneltiminginfo
			= &nt36672_tianma_fhdplus_video_e7_timing_info;
		panelstruct->panelresetseq
					 = &nt36672_tianma_fhdplus_video_e7_panel_reset_seq;
		panelstruct->backlightinfo = &nt36672_tianma_fhdplus_video_e7_backlight;
		pinfo->mipi.panel_on_cmds
			= nt36672_tianma_fhdplus_video_e7_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= NT36672_TIANMA_FHDPLUS_VIDEO_E7_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= nt36672_tianma_fhdplus_video_e7_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= NT36672_TIANMA_FHDPLUS_VIDEO_E7_OFF_COMMAND;
		memcpy(phy_db->timing,
			nt36672_tianma_fhdplus_14nm_video_e7_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->dfps.panel_dfps = nt36672_tianma_fhdplus_video_e7_dfps;
		pinfo->mipi.signature 	= NT36672_TIANMA_fhdplus_VIDEO_E7_SIGNATURE;
	    break;

	case NT36672_CSOT_FHDPLUS_VIDEO_E7_PANEL:
		panelstruct->paneldata    = &nt36672_csot_fhdplus_video_e7_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 0;
		panelstruct->panelres     = &nt36672_csot_fhdplus_video_e7_panel_res;
		panelstruct->color        = &nt36672_csot_fhdplus_video_e7_color;
		panelstruct->videopanel   = &nt36672_csot_fhdplus_video_e7_video_panel;
		panelstruct->commandpanel = &nt36672_csot_fhdplus_video_e7_command_panel;
		panelstruct->state        = &nt36672_csot_fhdplus_video_e7_state;
		panelstruct->laneconfig   = &nt36672_csot_fhdplus_video_e7_lane_config;
		panelstruct->paneltiminginfo
			= &nt36672_csot_fhdplus_video_e7_timing_info_8937;
		panelstruct->paneltiminginfo
			= &nt36672_csot_fhdplus_video_e7_timing_info;
		panelstruct->panelresetseq
					 = &nt36672_csot_fhdplus_video_e7_panel_reset_seq;
		panelstruct->backlightinfo = &nt36672_csot_fhdplus_video_e7_backlight;
		pinfo->mipi.panel_on_cmds
			= nt36672_csot_fhdplus_video_e7_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= NT36672_CSOT_FHDPLUS_VIDEO_E7_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= nt36672_csot_fhdplus_video_e7_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= NT36672_CSOT_FHDPLUS_VIDEO_E7_OFF_COMMAND;
		memcpy(phy_db->timing,
			nt36672_csot_fhdplus_14nm_video_e7_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->dfps.panel_dfps = nt36672_csot_fhdplus_video_e7_dfps;
		pinfo->mipi.signature 	= NT36672_CSOT_fhdplus_VIDEO_E7_SIGNATURE;
	    break;
#endif

	case TRULY_1080P_VIDEO_PANEL:
		panelstruct->paneldata    = &truly_1080p_video_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 0;
		panelstruct->panelres     = &truly_1080p_video_panel_res;
		panelstruct->color        = &truly_1080p_video_color;
		panelstruct->videopanel   = &truly_1080p_video_video_panel;
		panelstruct->commandpanel = &truly_1080p_video_command_panel;
		panelstruct->state        = &truly_1080p_video_state;
		panelstruct->laneconfig   = &truly_1080p_video_lane_config;
		panelstruct->paneltiminginfo
			= &truly_1080p_video_timing_info;
		panelstruct->panelresetseq
					 = &truly_1080p_video_panel_reset_seq;
		panelstruct->backlightinfo = &truly_1080p_video_backlight;
		pinfo->mipi.panel_on_cmds
			= truly_1080p_video_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= TRULY_1080P_VIDEO_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= truly_1080p_video_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= TRULY_1080P_VIDEO_OFF_COMMAND;
		memcpy(phy_db->timing,
			truly_1080p_14nm_video_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->dfps.panel_dfps = truly_1080p_video_dfps;
		pinfo->mipi.signature 	= TRULY_1080P_VIDEO_SIGNATURE;
		break;
	case TRULY_1080P_CMD_PANEL:
		panelstruct->paneldata    = &truly_1080p_cmd_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 0;
		panelstruct->panelres     = &truly_1080p_cmd_panel_res;
		panelstruct->color        = &truly_1080p_cmd_color;
		panelstruct->videopanel   = &truly_1080p_cmd_video_panel;
		panelstruct->commandpanel = &truly_1080p_cmd_command_panel;
		panelstruct->state        = &truly_1080p_cmd_state;
		panelstruct->laneconfig   = &truly_1080p_cmd_lane_config;
		panelstruct->paneltiminginfo
			= &truly_1080p_cmd_timing_info;
		panelstruct->panelresetseq
					 = &truly_1080p_cmd_panel_reset_seq;
		panelstruct->backlightinfo = &truly_1080p_cmd_backlight;
		pinfo->mipi.panel_on_cmds
			= truly_1080p_cmd_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= TRULY_1080P_CMD_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= truly_1080p_cmd_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= TRULY_1080P_CMD_OFF_COMMAND;
		memcpy(phy_db->timing,
			truly_1080p_14nm_cmd_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->mipi.signature 	= TRULY_1080P_CMD_SIGNATURE;
		break;
	case R69006_1080P_VIDEO_PANEL:
		panelstruct->paneldata    = &r69006_1080p_video_panel_data;
		panelstruct->panelres     = &r69006_1080p_video_panel_res;
		panelstruct->color        = &r69006_1080p_video_color;
		panelstruct->videopanel   = &r69006_1080p_video_video_panel;
		panelstruct->commandpanel = &r69006_1080p_video_command_panel;
		panelstruct->state        = &r69006_1080p_video_state;
		panelstruct->laneconfig   = &r69006_1080p_video_lane_config;
		panelstruct->paneltiminginfo
			= &r69006_1080p_video_timing_info;
		panelstruct->panelresetseq
			= &r69006_1080p_video_reset_seq;
		panelstruct->backlightinfo = &r69006_1080p_video_backlight;
		pinfo->labibb = &r69006_1080p_video_labibb;
		pinfo->mipi.panel_on_cmds
			= r69006_1080p_video_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= R69006_1080P_VIDEO_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= r69006_1080p_video_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= R69006_1080P_VIDEO_OFF_COMMAND;
		memcpy(phy_db->timing,
			r69006_1080p_14nm_video_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->mipi.signature = R69006_1080P_VIDEO_SIGNATURE;
		break;
	case R69006_1080P_CMD_PANEL:
		panelstruct->paneldata    = &r69006_1080p_cmd_panel_data;
		panelstruct->panelres     = &r69006_1080p_cmd_panel_res;
		panelstruct->color        = &r69006_1080p_cmd_color;
		panelstruct->videopanel   = &r69006_1080p_cmd_video_panel;
		panelstruct->commandpanel = &r69006_1080p_cmd_command_panel;
		panelstruct->state        = &r69006_1080p_cmd_state;
		panelstruct->laneconfig   = &r69006_1080p_cmd_lane_config;
		panelstruct->paneltiminginfo
			= &r69006_1080p_cmd_timing_info;
		panelstruct->panelresetseq
			= &r69006_1080p_cmd_reset_seq;
		panelstruct->backlightinfo = &r69006_1080p_cmd_backlight;
		pinfo->labibb = &r69006_1080p_cmd_labibb;
		pinfo->mipi.panel_on_cmds
			= r69006_1080p_cmd_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= R69006_1080P_CMD_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= r69006_1080p_cmd_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= R69006_1080P_CMD_OFF_COMMAND;
		memcpy(phy_db->timing,
			r69006_1080p_14nm_cmd_timings,
			MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->mipi.signature = R69006_1080P_CMD_SIGNATURE;
		pinfo->mipi.tx_eot_append = true;
		pinfo->mipi.rx_eot_ignore = true;
		break;
	case TRULY_WUXGA_VIDEO_PANEL:
		panelstruct->paneldata    = &truly_wuxga_video_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 1;
		panelstruct->panelres     = &truly_wuxga_video_panel_res;
		panelstruct->color        = &truly_wuxga_video_color;
		panelstruct->videopanel   = &truly_wuxga_video_video_panel;
		panelstruct->commandpanel = &truly_wuxga_video_command_panel;
		panelstruct->state        = &truly_wuxga_video_state;
		panelstruct->laneconfig   = &truly_wuxga_video_lane_config;
		panelstruct->paneltiminginfo
			= &truly_wuxga_video_timing_info;
		panelstruct->panelresetseq
					 = &truly_wuxga_video_panel_reset_seq;
		panelstruct->backlightinfo = &truly_wuxga_video_backlight;
		pinfo->mipi.panel_on_cmds
			= truly_wuxga_video_on_command;
		pinfo->mipi.num_of_panel_on_cmds
			= TRULY_WUXGA_VIDEO_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
			= truly_wuxga_video_off_command;
		pinfo->mipi.num_of_panel_off_cmds
			= TRULY_WUXGA_VIDEO_OFF_COMMAND;
		memcpy(phy_db->timing,
			truly_wuxga_14nm_video_timings, MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->dfps.panel_dfps = truly_wuxga_video_dfps;
		pinfo->mipi.signature 	= TRULY_WUXGA_VIDEO_SIGNATURE;
		break;
        case OTM1911_FHD_VIDEO_PANEL:
                panelstruct->paneldata    = &otm1911_fhd_video_panel_data;
                panelstruct->paneldata->panel_with_enable_gpio = 0;
                panelstruct->panelres     = &otm1911_fhd_video_panel_res;
                panelstruct->color        = &otm1911_fhd_video_color;
                panelstruct->videopanel   = &otm1911_fhd_video_video_panel;
                panelstruct->commandpanel = &otm1911_fhd_video_command_panel;
                panelstruct->state        = &otm1911_fhd_video_state;
                panelstruct->laneconfig   = &otm1911_fhd_video_lane_config;
                panelstruct->paneltiminginfo
                 = &otm1911_fhd_video_timing_info_8937;
                panelstruct->paneltiminginfo
                 = &otm1911_fhd_video_timing_info;
                panelstruct->panelresetseq
                                 = &otm1911_fhd_video_panel_reset_seq;
                panelstruct->backlightinfo = &otm1911_fhd_video_backlight;
                pinfo->mipi.panel_on_cmds
                 = otm1911_fhd_video_on_command;
                pinfo->mipi.num_of_panel_on_cmds
                 = OTM1911_FHD_VIDEO_ON_COMMAND;
                pinfo->mipi.panel_off_cmds
                 = otm1911_fhd_video_off_command;
                pinfo->mipi.num_of_panel_off_cmds
                 = OTM1911_FHD_VIDEO_OFF_COMMAND;
                memcpy(phy_db->timing,
                otm1911_fhd_14nm_video_timings,
                MAX_TIMING_CONFIG * sizeof(uint32_t));
                pinfo->dfps.panel_dfps = otm1911_fhd_video_dfps;
                pinfo->mipi.signature   = OTM1911_FHD_VIDEO_SIGNATURE;
                break;
	case HX8399C_HD_PLUS_VIDEO_PANEL:
		panelstruct->paneldata    = &hx8399c_hd_plus_video_panel_data;
		panelstruct->panelres     = &hx8399c_hd_plus_video_panel_res;
		panelstruct->color        = &hx8399c_hd_plus_video_color;
		panelstruct->videopanel   =
				&hx8399c_hd_plus_video_video_panel;
		panelstruct->commandpanel =
				&hx8399c_hd_plus_video_command_panel;
		panelstruct->state        = &hx8399c_hd_plus_video_state;
		panelstruct->laneconfig   =
				&hx8399c_hd_plus_video_lane_config;
		panelstruct->paneltiminginfo
				= &hx8399c_hd_plus_video_timing_info;
		panelstruct->panelresetseq
				= &hx8399c_hd_plus_video_panel_reset_seq;
		panelstruct->backlightinfo = &hx8399c_hd_plus_video_backlight;
		pinfo->labibb = &hx8399c_hd_plus_video_labibb;
		pinfo->mipi.panel_on_cmds
				= hx8399c_hd_plus_video_on_command;
		pinfo->mipi.num_of_panel_on_cmds
				= HX8399C_HD_PLUS_VIDEO_ON_COMMAND;
		pinfo->mipi.panel_off_cmds
				= hx8399c_hd_plus_video_off_command;
		pinfo->mipi.num_of_panel_off_cmds
				= HX8399C_HD_PLUS_VIDEO_OFF_COMMAND;
		memcpy(phy_db->timing,
			hx8399c_hd_plus_14nm_video_timings, MAX_TIMING_CONFIG * sizeof(uint32_t));
		pinfo->mipi.signature    = HX8399C_HD_PLUS_VIDEO_SIGNATURE;
		break;
        case OTM1911_FHDPLUS_VIDEO_PANEL:
		panelstruct->paneldata    = &otm1911_fhdplus_video_panel_data;
                panelstruct->paneldata->panel_with_enable_gpio = 0;
                panelstruct->panelres     = &otm1911_fhdplus_video_panel_res;
                panelstruct->color        = &otm1911_fhdplus_video_color;
                panelstruct->videopanel   = &otm1911_fhdplus_video_video_panel;
                panelstruct->commandpanel = &otm1911_fhdplus_video_command_panel;
                panelstruct->state        = &otm1911_fhdplus_video_state;
                panelstruct->laneconfig   = &otm1911_fhdplus_video_lane_config;
                panelstruct->paneltiminginfo
                        = &otm1911_fhdplus_video_timing_info_8937;
                panelstruct->paneltiminginfo
                        = &otm1911_fhdplus_video_timing_info;
                panelstruct->panelresetseq
                                         = &otm1911_fhdplus_video_panel_reset_seq;
                panelstruct->backlightinfo = &otm1911_fhdplus_video_backlight;
                pinfo->mipi.panel_on_cmds
                        = otm1911_fhdplus_video_on_command;
                pinfo->mipi.num_of_panel_on_cmds
                        = OTM1911_FHDPLUS_VIDEO_ON_COMMAND;
                pinfo->mipi.panel_off_cmds
                        = otm1911_fhdplus_video_off_command;
                pinfo->mipi.num_of_panel_off_cmds
                        = OTM1911_FHDPLUS_VIDEO_OFF_COMMAND;
                memcpy(phy_db->timing,
                        otm1911_fhdplus_14nm_video_timings,
                        MAX_TIMING_CONFIG * sizeof(uint32_t));
                pinfo->dfps.panel_dfps = otm1911_fhdplus_video_dfps;
                pinfo->mipi.signature   = OTM1911_FHDPLUS_VIDEO_SIGNATURE;
                break;
#if 1//defined(PROJECT_SAKURA)
	case ILI7807_FHDPLUS_VIDEO_PANEL:
                panelstruct->paneldata    = &ili7807_fhdplus_video_panel_data;
                panelstruct->paneldata->panel_with_enable_gpio = 0;
                panelstruct->panelres     = &ili7807_fhdplus_video_panel_res;
                panelstruct->color        = &ili7807_fhdplus_video_color;
                panelstruct->videopanel   = &ili7807_fhdplus_video_video_panel;
                panelstruct->commandpanel = &ili7807_fhdplus_video_command_panel;
                panelstruct->state        = &ili7807_fhdplus_video_state;
                panelstruct->laneconfig   = &ili7807_fhdplus_video_lane_config;
                panelstruct->paneltiminginfo
                        = &ili7807_fhdplus_video_timing_info_8937;
                panelstruct->paneltiminginfo
                        = &ili7807_fhdplus_video_timing_info;
                panelstruct->panelresetseq
                                         = &ili7807_fhdplus_video_panel_reset_seq;
                panelstruct->backlightinfo = &ili7807_fhdplus_video_backlight;
                pinfo->mipi.panel_on_cmds
                        = ili7807_fhdplus_video_on_command;
                pinfo->mipi.num_of_panel_on_cmds
                        = ILI7807_FHDPLUS_VIDEO_ON_COMMAND;
                pinfo->mipi.panel_off_cmds
                        = ili7807_fhdplus_video_off_command;
                pinfo->mipi.num_of_panel_off_cmds
                        = ILI7807_FHDPLUS_VIDEO_OFF_COMMAND;
                memcpy(phy_db->timing,
                        ili7807_fhdplus_14nm_video_timings,
                        MAX_TIMING_CONFIG * sizeof(uint32_t));
                pinfo->dfps.panel_dfps = ili7807_fhdplus_video_dfps;
                pinfo->mipi.signature   = ILI7807_FHDPLUS_VIDEO_SIGNATURE;
                break;
        case HX8399C_FHDPLUS_VIDEO_PANEL:
                panelstruct->paneldata = &hx8399c_fhdplus_video_panel_data;
                panelstruct->paneldata->panel_with_enable_gpio = 0;
                panelstruct->panelres = &hx8399c_fhdplus_video_panel_res;
                panelstruct->color = &hx8399c_fhdplus_video_color;
                panelstruct->videopanel = &hx8399c_fhdplus_video_video_panel;
                panelstruct->commandpanel = &hx8399c_fhdplus_video_command_panel;
                panelstruct->state         = &hx8399c_fhdplus_video_state;
                panelstruct->laneconfig   = &hx8399c_fhdplus_video_lane_config;
                panelstruct->paneltiminginfo = &hx8399c_fhdplus_video_timing_info_8937;
                panelstruct->paneltiminginfo = &hx8399c_fhdplus_video_timing_info;
                panelstruct->panelresetseq = &hx8399c_fhdplus_video_panel_reset_seq;
                panelstruct->backlightinfo = &hx8399c_fhdplus_video_backlight;
                pinfo->mipi.panel_on_cmds = hx8399c_fhdplus_video_on_command;
                pinfo->mipi.num_of_panel_on_cmds = HX8399C_FHDPLUS_VIDEO_ON_COMMAND;
                pinfo->mipi.panel_off_cmds = hx8399c_fhdplus_video_off_command;
                pinfo->mipi.num_of_panel_off_cmds = HX8399C_FHDPLUS_VIDEO_OFF_COMMAND;
                memcpy(phy_db->timing, hx8399c_fhdplus_14nm_video_timings, MAX_TIMING_CONFIG * sizeof(uint32_t));
                pinfo->dfps.panel_dfps = hx8399c_fhdplus_video_dfps;
                pinfo->mipi.signature = HX8399C_FHDPLUS_VIDEO_SIGNATURE;
                break;
#endif

	case UNKNOWN_PANEL:
	default:
		memset(panelstruct, 0, sizeof(struct panel_struct));
		memset(pinfo->mipi.panel_on_cmds, 0,
					sizeof(struct mipi_dsi_cmd));
		pinfo->mipi.num_of_panel_on_cmds = 0;
		memset(pinfo->mipi.panel_off_cmds, 0,
					sizeof(struct mipi_dsi_cmd));
		pinfo->mipi.num_of_panel_off_cmds = 0;
		memset(phy_db->timing, 0, TIMING_SIZE);
		pan_type = PANEL_TYPE_UNKNOWN;
		break;
	}
	return pan_type;
}

#define DISPLAY_MAX_PANEL_DETECTION 2

uint32_t oem_panel_max_auto_detect_panels()
{
	return target_panel_auto_detect_enabled() ?
		DISPLAY_MAX_PANEL_DETECTION : 0;
}

#if defined(PROJECT_SAKURA)||defined(PROJECT_VINCE)||defined(PROJECT_DAISY)
int do_wpoint_cali = 0;
int panel_td4310_id = 0xff;
#endif

int switch_panel_id(void)
{
      int lcd_id0;
      int lcd_id1;

       gpio_tlmm_config(lcd_gpio66.pin_id, 0,
                               lcd_gpio66.pin_direction, lcd_gpio66.pin_pull,
                               lcd_gpio66.pin_strength, lcd_gpio66.pin_state);

       gpio_tlmm_config(lcd_gpio94.pin_id, 0,
                               lcd_gpio94.pin_direction, lcd_gpio94.pin_pull,
                               lcd_gpio94.pin_strength, lcd_gpio94.pin_state);
       mdelay(10);
       lcd_id1=1;
       lcd_id0=1;
       lcd_id1=gpio_status(lcd_gpio94.pin_id); 
       lcd_id0=gpio_status(lcd_gpio66.pin_id);    
         
       dprintf(INFO,"lancelot lcd_id0=%d.\n",lcd_id0);
       dprintf(INFO,"lancelot lcd_id1=%d.\n",lcd_id1);

#if defined(PROJECT_SAKURA)||defined(PROJECT_DAISY)
        if (1 == lcd_id0 && 0 == lcd_id1){
                do_wpoint_cali = 1;
                return HX8399C_FHDPLUS_VIDEO_PANEL;
        }else if(0 == lcd_id0 && 1 == lcd_id1){
                backlight_delay_flag=1;
                do_wpoint_cali = 1;
                return ILI7807_FHDPLUS_VIDEO_PANEL;
        }else if(0 == lcd_id0 && 0 == lcd_id1){
                do_wpoint_cali = 1;
                return OTM1911_FHDPLUS_VIDEO_PANEL;
        }else{
                backlight_delay_flag=1;
                do_wpoint_cali = 1;
                return ILI7807_FHDPLUS_VIDEO_PANEL;
        }
#elif defined(PROJECT_VINCE)
        if(panel_td4310_id == TD4310_FHDPLUS_VIDEO_E7_G55_PANEL)
                return TD4310_FHDPLUS_VIDEO_E7_G55_PANEL;

        if(1==lcd_id0&&1==lcd_id1)
                return NT36672_TIANMA_FHDPLUS_VIDEO_E7_PANEL;
        else if(0==lcd_id0&&1==lcd_id1){
                do_wpoint_cali = 1;
                return NT36672_CSOT_FHDPLUS_VIDEO_E7_PANEL;
        }
        else if(1==lcd_id0&&0==lcd_id1){
                do_wpoint_cali = 1;
                return TD4310_EBBG_FHDPLUS_VIDEO_E7_PANEL;
        }
        else{
                do_wpoint_cali = 1;
                return TD4310_FHDPLUS_VIDEO_E7_PANEL;
        }
#else
        dprintf(INFO,"%s:return default video panel:ILI7807_FHDPLUS_VIDEO_PANEL\n",__func__);
        return ILI7807_FHDPLUS_VIDEO_PANEL;
#endif

#if defined(PROJECT_SAKURA)||defined(PROJECT_DAISY)
	if (1 == lcd_id0 && 0 == lcd_id1){
		do_wpoint_cali = 1;
		return HX8399C_FHDPLUS_VIDEO_PANEL;
	}else if(0 == lcd_id0 && 1 == lcd_id1){
		backlight_delay_flag=1;
		do_wpoint_cali = 1;
		return ILI7807_FHDPLUS_VIDEO_PANEL;
	}else if(0 == lcd_id0 && 0 == lcd_id1){
		do_wpoint_cali = 1;
		return OTM1911_FHDPLUS_VIDEO_PANEL;
	}else{
		 backlight_delay_flag=1;
		do_wpoint_cali = 1;
		return ILI7807_FHDPLUS_VIDEO_PANEL;
	}
#elif defined(PROJECT_VINCE)
	if(panel_td4310_id == TD4310_FHDPLUS_VIDEO_E7_G55_PANEL)
		return TD4310_FHDPLUS_VIDEO_E7_G55_PANEL;

	if(1==lcd_id0&&1==lcd_id1)
		return NT36672_TIANMA_FHDPLUS_VIDEO_E7_PANEL;
	else if(0==lcd_id0&&1==lcd_id1){
		do_wpoint_cali = 1;
		return NT36672_CSOT_FHDPLUS_VIDEO_E7_PANEL;
	}
	else if(1==lcd_id0&&0==lcd_id1){
		do_wpoint_cali = 1;
		return TD4310_EBBG_FHDPLUS_VIDEO_E7_PANEL;
	}
	else{
		do_wpoint_cali = 1;
                return TD4310_FHDPLUS_VIDEO_E7_PANEL;
	}
#else
	dprintf(INFO,"%s:return default video panel:ILI7807_FHDPLUS_VIDEO_PANEL\n",__func__);
	return ILI7807_FHDPLUS_VIDEO_PANEL;
#endif
}

int oem_panel_select(const char *panel_name, struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	int32_t panel_override_id;
	phy_db->pll_type = DSI_PLL_TYPE_THULIUM;

	if (panel_name) {
		panel_override_id = panel_name_to_id(supp_panels,
				ARRAY_SIZE(supp_panels), panel_name);

		if (panel_override_id < 0) {
			dprintf(CRITICAL, "Not able to search the panel:%s\n",
					 panel_name + strspn(panel_name, " "));
		} else if (panel_override_id < UNKNOWN_PANEL) {
			/* panel override using fastboot oem command */
			panel_id = panel_override_id;

			dprintf(INFO, "OEM panel override:%s\n",
					panel_name + strspn(panel_name, " "));
			goto panel_init;
		}
	}
#if 0	
	switch (hw_id) {
	case HW_PLATFORM_MTP:
	case HW_PLATFORM_SURF:
	case HW_PLATFORM_RCM:
		panel_id = TRULY_1080P_VIDEO_PANEL;
		if (platform_subtype == 0x02)
			 panel_id = HX8399C_FHD_PLUSE_VIDEO_PANEL;
		break;
	case HW_PLATFORM_QRD:
		panel_id = R69006_1080P_CMD_PANEL;
		break;
	default:
		dprintf(CRITICAL, "Display not enabled for %d HW type\n",
			hw_id);
		return PANEL_TYPE_UNKNOWN;
	}
#endif
      if((panel_id=switch_panel_id())==UNKNOWN_PANEL)
      {
                return PANEL_TYPE_UNKNOWN;
          }
         special_panel = panel_id;

panel_init:
	/*
	 * Update all data structures after 'panel_init' label. Only panel
	 * selection is supposed to happen before that.
	 */
	pinfo->pipe_type = MDSS_MDP_PIPE_TYPE_RGB;
	return init_panel_data(panelstruct, pinfo, phy_db);
}
