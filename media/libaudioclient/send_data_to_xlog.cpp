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
#include "send_data_to_xlog.h"

using namespace std;
using namespace android;

#define MALLOC_SIZE 64
#define AUDIOTRACK_PARAMETER 20

void send_audiotrack_parameter_to_xlog(int playbackTime, const char* clientName, const char* usage, const char* flags, int channelMask, const char* format, int sampleRate)
{
	ALOGD("%s", __func__);
    auto msg = (struct xlog_flat_msg *)calloc(1, sizeof(struct xlog_flat_msg));
    if (msg == nullptr) {
        ALOGE("alloc memory fail !");
        return;
    }
    msg->str[0] = (char *)malloc(MALLOC_SIZE);
    msg->str[1] = (char *)malloc(MALLOC_SIZE);
    msg->str[2] = (char *)malloc(MALLOC_SIZE);
    msg->str[3] = (char *)malloc(MALLOC_SIZE);
    if (msg->str[0] == nullptr || msg->str[1] == nullptr || msg->str[2] == nullptr || msg->str[3] == nullptr) {
        ALOGE("malloc memory fail !");
        goto exit;
    }
    snprintf(msg->str[0], MALLOC_SIZE - 1, "%s", clientName);
    snprintf(msg->str[1], MALLOC_SIZE - 1, "%s", usage);
    snprintf(msg->str[2], MALLOC_SIZE - 1, "%s", flags);
    snprintf(msg->str[3], MALLOC_SIZE - 1, "%s", format);
    msg->val[0] = playbackTime;
    msg->val[1] = channelMask;
    msg->val[2] = sampleRate;

    xlog_send(AUDIOTRACK_PARAMETER, msg);
exit:
    if (msg->str[0])
        free(msg->str[0]);
    if (msg->str[1])
        free(msg->str[1]);
    if (msg->str[2])
        free(msg->str[2]);
    if (msg->str[3])
        free(msg->str[3]);
    free(msg);
    return;
}
