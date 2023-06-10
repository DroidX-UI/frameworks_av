#ifndef SEND_DATA_TO_XLOG_H
#define SEND_DATA_TO_XLOG_H

struct xlog_msg_send {
    int height;
    int width;
    int frameRate;
    int hdr;
    int frameRateCal;
    int frcState;
    int aieState;
    int aisState;
    int dolbyVision;
    char* packgeName;
    char* mime;
};

namespace android {
void send_video_play_to_xlog(const xlog_msg_send *portDefn);
//void send_video_play_to_xlog(const char *mime);

}

#endif