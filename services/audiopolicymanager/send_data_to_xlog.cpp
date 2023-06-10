#define LOG_TAG "send_data_to_xlog"

#include "send_data_to_xlog.h"

#include <cutils/log.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <system/audio-base.h>
#include <system/audio.h>
#include <utils/Errors.h>

#include <iostream>

#include "xlog.h"
#include "xlog_api.h"

using namespace std;
using namespace android;

#define MALLOC_SIZE 64

static double connect_headphone_last_time = 0.0;

void send_projection_screen_to_xlog()
{
    ALOGD("%s", __func__);
    int xlog_value[1];
    xlog_value[0] = 0;
    xlog_send_int(PROJECTION_SCREEN, xlog_value);
}

string get_headset_type(audio_devices_t type) {
    if (type < AUDIO_DEVICE_OUT_EARPIECE || type > AUDIO_DEVICE_OUT_DEFAULT) {
        ALOGE("%s the type(0x%X) is invaluable", __func__, type);
    }

    if (type & (AUDIO_DEVICE_OUT_WIRED_HEADSET))
        return "wired-headset-4";
    if (type & (AUDIO_DEVICE_OUT_WIRED_HEADPHONE))
        return "wired-headphone-3";
    if (type & AUDIO_DEVICE_OUT_BLUETOOTH_SCO)
        return "bluetooth-sco";
    if (type & AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)
        return "bluetooth-sco_headset";
    if (type & AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT)
        return "bluetooth-sco_carkit";
    if (type & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP)
        return "bluetooth-a2dp";
    if (type & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES)
        return "bluetooth-a2dp_headphones";
    if (type & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER)
        return "bluetooth-a2dp_speaker";
    if (type & (AUDIO_DEVICE_OUT_USB_ACCESSORY | AUDIO_DEVICE_OUT_USB_DEVICE |
                AUDIO_DEVICE_OUT_USB_HEADSET))
        return "usb";
    return "others";
}

int send_headphone_type_to_xlog(audio_devices_t type, const char *name, audio_format_t encodedFormat) {
    if (name == NULL || strlen(const_cast<char *>(name)) == 0) {
        ALOGE("%s the name is NULL", __func__);
        return -1;
    }
    ALOGI("%s type:%d, name:%s, encodedFormat:0x%X", __func__, type, name, encodedFormat);

    if (!audio_is_output_device(type)) {
        ALOGE("%s the device is invaluable", __func__);
        return BAD_VALUE;
    }

    string headset_type = get_headset_type(type);
    ALOGI("%s curent device type is %s", __func__, headset_type.c_str());

    if (!headset_type.compare("others")) {
        ALOGE("%s the device is others when use audiovisual", __func__);
        return -1;
    }

    int ret = 0;
    struct timeval start;
    struct timeval end;
    struct xlog_flat_msg *msg = (struct xlog_flat_msg *)malloc(sizeof(struct xlog_flat_msg));
    if (msg == NULL) {
        ALOGE("alloc memory fail !");
        return -ENOMEM;
    }
    memset(msg, 0, sizeof(struct xlog_flat_msg));

    gettimeofday(&end, NULL);
    double connect_headphone_new_time = end.tv_sec + (double)(end.tv_usec) / 1000000.0;  // 微秒
    double time_diff = connect_headphone_new_time - connect_headphone_last_time;

    msg->str[0] = (char *)malloc(MALLOC_SIZE);
    msg->str[1] = (char *)malloc(MALLOC_SIZE);
    msg->str[2] = (char *)malloc(MALLOC_SIZE);

    if (msg->str[0] == NULL || msg->str[1] == NULL || msg->str[2] == NULL) {
        ALOGE("malloc memory fail !");
        ret = -ENOMEM;
        goto exit;
    }

    snprintf(msg->str[0], MALLOC_SIZE - 1, "%s", const_cast<char *>(headset_type.c_str()));
    snprintf(msg->str[1], MALLOC_SIZE - 1, "%s", name);
    snprintf(msg->str[2], MALLOC_SIZE - 1, "%x", encodedFormat);

    // 两次设备切换的时间差大于0.5秒才会打点
    if (time_diff > 0.5) {
        ALOGD("%s : time diff = %lf", __func__, time_diff);
        ret = xlog_send(HEADPHONES, msg);
    }
    gettimeofday(&start, NULL);
    connect_headphone_last_time = start.tv_sec + (double)(start.tv_usec) / 1000000.0;

exit:
    if (msg->str[0])
        free(msg->str[0]);
    if (msg->str[1])
        free(msg->str[1]);
    if (msg->str[2])
        free(msg->str[2]);
    free(msg);
    return ret;
}
