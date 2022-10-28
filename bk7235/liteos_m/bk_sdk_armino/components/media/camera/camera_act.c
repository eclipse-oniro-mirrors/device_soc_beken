// Copyright (C) 2022 Beken Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <components/log.h>

#include "media_core.h"
#include "camera_act.h"
#include "media_evt.h"
#include "storage_act.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>

#include "camera.h"
#include "frame_buffer.h"


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include <modules/ble.h>

#include <driver/timer.h>

#define TAG "cam_act"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define DEBUG_INTERVAL (1000 * 2)

extern bk_err_t pm_core_bus_clock_ctrl(uint32_t cksel_core, uint32_t ckdiv_core, uint32_t ckdiv_bus, uint32_t ckdiv_cpu0, uint32_t ckdiv_cpu1);

extern void transfer_dump(uint32_t ms);

media_debug_t *media_debug = NULL;
media_debug_t *media_debug_cached = NULL;

camera_info_t camera_info;

static void camera_debug_dump(timer_id_t timer_id)
{
	transfer_dump(DEBUG_INTERVAL);

	uint16_t jpg = (media_debug->isr_jpeg - media_debug_cached->isr_jpeg) / 2;
	uint16_t dec = (media_debug->isr_decoder - media_debug_cached->isr_decoder) / 2;
	uint16_t lcd = (media_debug->isr_lcd - media_debug_cached->isr_lcd) / 2;
	uint16_t fps = (media_debug->fps_lcd - media_debug_cached->fps_lcd) / 2;
	uint16_t wifi = (media_debug->fps_wifi - media_debug_cached->fps_wifi) / 2;
	uint16_t err_dec = (media_debug->err_dec - media_debug_cached->err_dec) / 2;

	media_debug_cached->isr_jpeg = media_debug->isr_jpeg;
	media_debug_cached->isr_decoder = media_debug->isr_decoder;
	media_debug_cached->isr_lcd = media_debug->isr_lcd;
	media_debug_cached->fps_lcd = media_debug->fps_lcd;
	media_debug_cached->fps_wifi = media_debug->fps_wifi;
	media_debug_cached->err_dec = media_debug->err_dec;

	LOGI("jpg: %d[%d], dec: %d[%d, %d], lcd: %d[%d], fps: %d[%d], wifi: %d[%d]\n",
		jpg, media_debug->isr_jpeg,
		dec, media_debug->isr_decoder, err_dec,
		lcd, media_debug->isr_lcd,
		fps, media_debug->fps_lcd,
		wifi, media_debug->fps_wifi);
}

void camera_dvp_open_handle(param_pak_t *param, dvp_mode_t mode)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		ret = kNoErr;
		goto out;
	}

	//pm_core_bus_clock_ctrl(2, 0, 1, 0, 0);

	frame_buffer_enable(true);

	ret = bk_dvp_camera_open(param->param, mode);


	if (ret != kNoErr)
	{
		LOGE("%s open failed\n", __func__);
		goto out;
	}

	set_camera_state(CAMERA_STATE_ENABLED);

	if (camera_info.debug)
	{
		bk_timer_start(TIMER_ID1, DEBUG_INTERVAL, camera_debug_dump);
	}

out:
	MEDIA_EVT_RETURN(param, ret);
}

void camera_dvp_close_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	bk_dvp_camera_close();

	set_camera_state(CAMERA_STATE_DISABLED);

	if (camera_info.debug)
	{
		bk_timer_stop(TIMER_ID1);
	}


	frame_buffer_enable(false);

	//pm_core_bus_clock_ctrl(3, 1, 1, 0, 0);

out:
	MEDIA_EVT_RETURN(param, ret);
}

#ifdef CONFIG_USB_UVC

void camera_uvc_open_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		ret = kNoErr;
		goto out;
	}

	if (bk_ble_get_env_state())
	{
		LOGI("bluetooth is enabled, shutdown bluetooth\n");
		bk_ble_deinit();
		rtos_delay_milliseconds(900);
	}
	else
	{
		LOGI("bluetooth state: %d\n", bk_ble_get_env_state());
	}


	//pm_core_bus_clock_ctrl(2, 0, 1, 0, 0);

	frame_buffer_enable(true);

	ret = bk_uvc_camera_open(param->param);

	if (ret != kNoErr)
	{
		LOGE("%s open failed\n", __func__);
		goto out;
	}

	set_camera_state(CAMERA_STATE_ENABLED);

	if (camera_info.debug)
	{
		bk_timer_start(TIMER_ID1, DEBUG_INTERVAL, camera_debug_dump);
	}

out:
	MEDIA_EVT_RETURN(param, ret);
}


void camera_uvc_close_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	if (camera_info.debug)
	{
		bk_timer_stop(TIMER_ID1);
	}

	bk_uvc_camera_close();

	set_camera_state(CAMERA_STATE_DISABLED);

	frame_buffer_enable(false);

	//pm_core_bus_clock_ctrl(3, 1, 1, 0, 0);
	LOGI("uvc close success!\n");

out:
	MEDIA_EVT_RETURN(param, ret);
}

#endif

void camera_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_CAM_DVP_JPEG_OPEN_IND:
			camera_dvp_open_handle((param_pak_t *)param, DVP_MODE_JPG);
			break;
		case EVENT_CAM_DVP_YUV_OPEN_IND:
			camera_dvp_open_handle((param_pak_t *)param, DVP_MODE_YUV);
			break;
		case EVENT_CAM_DVP_CLOSE_IND:
			camera_dvp_close_handle((param_pak_t *)param);
			break;

#ifdef CONFIG_USB_UVC
		case EVENT_CAM_UVC_OPEN_IND:
			camera_uvc_open_handle((param_pak_t *)param);
			break;
		case EVENT_CAM_UVC_CLOSE_IND:
			camera_uvc_close_handle((param_pak_t *)param);
			break;
#endif
	}
}

camera_state_t get_camera_state(void)
{
	return camera_info.state;
}

void set_camera_state(camera_state_t state)
{
	camera_info.state = state;
}

void camera_init(void)
{
	if (media_debug == NULL)
	{
		media_debug = (media_debug_t*)os_malloc(sizeof(media_debug_t));

		if (media_debug == NULL)
		{
			LOGE("malloc media_debug fail\n");
		}
	}

	if (media_debug_cached == NULL)
	{
		media_debug_cached = (media_debug_t*)os_malloc(sizeof(media_debug_t));
		if (media_debug_cached == NULL)
		{
			LOGE("malloc media_debug_cached fail\n");
		}
	}

	camera_info.state = CAMERA_STATE_DISABLED;
	camera_info.debug = true;
}

