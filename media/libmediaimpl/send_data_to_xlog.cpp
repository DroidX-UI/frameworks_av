#define LOG_TAG "send_data_to_xlog"

#include <stdlib.h>
#include <cutils/log.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#include <cutils/properties.h>
#include <cutils/atomic.h>

//#include <media/stagefright/Utils.h>
#include "xlog.h"
#include "xlog_api.h"
#include "send_data_to_xlog.h"
#include <stdio.h>
#include <string.h>
#include <media/stagefright/foundation/AMessage.h>

#define SM_MAX_STRING_LEN 64
#define MALLOC_SIZE 64
#define VIDEO_PLAY 13

namespace android {

void send_video_play_to_xlog( const xlog_msg_send *sendMsgData){
    if(sendMsgData == nullptr){
       return;
    }
    ALOGE("%s, start", __func__);
    //struct xlog_flat_msg *msg = (struct xlog_flat_msg *)malloc(sizeof(struct xlog_flat_msg));
    auto msg = (struct xlog_flat_msg *)calloc(1, sizeof(struct xlog_flat_msg));
    if (msg == nullptr) {
        ALOGE("alloc memory fail !");
        return;
    }

    int framerateInt, framerateIntCal;
    int width, height, frcState, aieState, aisState, dolbyVision, maxvalue, minvalue;

    msg->str[0] = (char *)malloc(MALLOC_SIZE);
    msg->str[1] = (char *)malloc(MALLOC_SIZE);
    if (msg->str[0] == nullptr || msg->str[1] == nullptr) {
        ALOGE("malloc memory fail !");
        goto exit;
    }

    framerateInt = sendMsgData->frameRate;
    framerateIntCal = sendMsgData->frameRateCal;
    width = sendMsgData->width;
    height = sendMsgData->height;
    frcState = sendMsgData->frcState;
    aieState = sendMsgData->aieState;
    aisState = sendMsgData->aisState;
    dolbyVision = sendMsgData->dolbyVision;

    snprintf(msg->str[0], MALLOC_SIZE-1, "%s", sendMsgData->mime);
    snprintf(msg->str[1], MALLOC_SIZE-1, "%s", sendMsgData->packgeName);
    msg->val[0] = framerateInt;
    msg->val[1] = sendMsgData->hdr;
    maxvalue = width > height ? width : height;
    minvalue = width < height ? width : height;
    if(126 <= maxvalue && maxvalue <= 226){
        if(94 <= minvalue && minvalue <= 194){
            width = 176;
            height = 144;//计数一次"176x144"
        }
    }else if(270 <=maxvalue && maxvalue <= 370){
        if(190 <= minvalue && minvalue <= 290){
            width = 320;
            height = 240;//计数一次"320x240"
        }
    }else if(430 <=maxvalue && maxvalue <= 530){
        if(310 <= minvalue && minvalue <= 410){
            width = 480;
            height = 360;//计数一次"480x360"
        }
    }else if(590 <=maxvalue && maxvalue <= 680){
        //if(430 <= minvalue && minvalue <= 530){
        if(310 <= minvalue && minvalue <= 530){
            width = 640;
            height = 480;//计数一次"640x480"
        }
    }else if(681 <=maxvalue && maxvalue <= 770){
        if(430 <= minvalue && minvalue <= 530){
            width = 720;
            height = 480;//计数一次"720x480"
        }
    }else if(810 <=maxvalue && maxvalue <= 910){
        if(430 <= minvalue && minvalue <= 530){
            width = 860;
            height = 480;//计数一次"860x480"
        }
    }else if(911 <=maxvalue && maxvalue <= 1010){
        //if(490 <= minvalue && minvalue <= 590){
        if(430 <= minvalue && minvalue <= 590){
            width = 960;
            height = 540;//计数一次"960x540"
        }
    }else if(1011 <=maxvalue && maxvalue <= 1130){
        if(540 <= minvalue && minvalue <= 770){
            width = 1080;
            height = 720;//计数一次"1080x720"
        }else if(1030 <=maxvalue && maxvalue <= 1130){
            width = 1080;
            height = 1080;//计数一次"1080x1080"
        }
    }else if(1180 <=maxvalue && maxvalue <= 1380){
        if(620 <= minvalue && minvalue <= 820){
            width = 1280;
            height = 720;//计数一次"1280x720"
        }
    }else if(1820 <=maxvalue && maxvalue <= 2020){
        if(980 <= minvalue && minvalue <= 1180){
            width = 1920;
            height = 1080;//计数一次"1920x1080"
        }
    }else if(2240 <=maxvalue && maxvalue <= 2440){
        if(980 <= minvalue && minvalue <= 1180){
            width = 2340;
            height = 1080;//计数一次"2340x1080"
        }
    }else if(2460 <=maxvalue && maxvalue <= 2660){
        if(1340 <= minvalue && minvalue <= 1540){
            width = 2340;
            height = 1440;//计数一次"2560x1440"
        }
    }else if(3740 <=maxvalue && maxvalue <= 3940){
        if(2060 <= minvalue && minvalue <= 2260){
            width = 3840;
            height = 2160;//计数一次"3840x2160"
        }
    }else if(7580 <=maxvalue && maxvalue <= 7780){
        if(4220 <= minvalue && minvalue <= 4420){
            width = 7680;
            height = 4320;//计数一次"7680x4320"
        }
    }else{
        //计数一次"其他"
    }

    msg->val[2] = width;
    msg->val[3] = height;
    msg->val[4] = framerateIntCal;
    msg->val[5] = frcState;
    msg->val[6] = aieState;
    msg->val[7] = aisState;
    msg->val[8] = dolbyVision;

    ALOGD("send_video_play_to_xlog::mine : %s", msg->str[0]);
    ALOGD("send_video_play_to_xlog::packgeName : %s", msg->str[1]);
    xlog_send(VIDEO_PLAY, msg);

exit:
    if (msg->str[0]){
        free(msg->str[0]);
    }
    if (msg->str[1]){
        free(msg->str[1]);
    }
    free(msg);
    return;
}


}  // namespace android