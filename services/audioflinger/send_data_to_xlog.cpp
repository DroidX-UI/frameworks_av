#define LOG_TAG "send_data_to_xlog"

#include <stdlib.h>
#include <cutils/log.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#include <cutils/properties.h>
#include <cutils/atomic.h>

#include "xlog.h"
#include "xlog_api.h"
#include "send_data_to_xlog.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define MALLOC_SIZE 64

static int change_game_voice_last_time = 0;
static int change_game_voice_new_time = 0;
static int time_diff = 0;
bool change_scheme = false;
//备份信息
char backup_mode[MALLOC_SIZE] = {"mode"};
char backup_packagename[MALLOC_SIZE] = {"packagename"};
char backup_scheme[MALLOC_SIZE] = {"scheme"};
char *backup_msg[3] = {backup_mode,backup_packagename,backup_scheme};
bool cnt = false;

int send_game_voice_time_to_xlog(int msg_id, const char* mode, const char* packagename, const char* scheme, bool status)
{
    struct timeval start;
    struct timeval end;
    int ret = 0;
    ALOGD("%s: mode: %s, packagename: %s ,scheme: %s ,backup_mode: %s ,backup_packagename: %s ,backup_scheme: %s", __func__, mode, packagename, scheme,backup_mode,backup_packagename,backup_scheme);

    change_scheme = false;
    //scheme发生更改，change_scheme为true
    if(((strcmp(backup_scheme,"youme")) && status)||((strcmp(backup_scheme,"xiaomi")) && status)){
        change_scheme = true;
    }
    struct xlog_flat_msg *msg = (struct xlog_flat_msg *)malloc(sizeof(struct xlog_flat_msg));
    if (msg == NULL) {
        ALOGE("%s: alloc xlog_msg memory fail !",__func__);
        return -ENOMEM;
    }
    memset(msg, 0, sizeof(struct xlog_flat_msg));
    msg->str[0] = (char *)malloc(MALLOC_SIZE);
    msg->str[1] = (char *)malloc(MALLOC_SIZE);
    msg->str[2] = (char *)malloc(MALLOC_SIZE);
    msg->str[3] = (char *)malloc(MALLOC_SIZE);
    if (msg->str[0] == NULL || msg->str[1] == NULL || msg->str[2] == NULL || msg->str[3] == NULL) {
        ALOGE("%s: malloc xlog_backup_msg memory fail !",__func__);
        ret = -ENOMEM;
        goto exit;
    }
    //除第一次外的model切换或者close
    if(cnt||!status||change_scheme){
        //获取第二次时间，作为end时间
        gettimeofday(&end,NULL);
        change_game_voice_new_time = end.tv_sec + (double)(end.tv_usec)/1000000.0;//微秒
        time_diff = change_game_voice_new_time - change_game_voice_last_time;
        snprintf(msg->str[0], MALLOC_SIZE-1, "%s", backup_msg[0]);
        snprintf(msg->str[1], MALLOC_SIZE-1, "%s", backup_msg[1]);
        snprintf(msg->str[2], MALLOC_SIZE-1, "%s", backup_msg[2]);
        snprintf(msg->str[3], MALLOC_SIZE-1, "%d", time_diff);
        ALOGD("%s: mode_backup: %s, packagename_backup: %s ,scheme_backup: %s ,time_diff: %d", __func__, backup_mode, backup_packagename, backup_scheme, time_diff);
        ret = xlog_send(msg_id, msg);
    }
    cnt = true;

    //获取第一次时间，作为start时间
    gettimeofday(&start,NULL);
    change_game_voice_last_time = start.tv_sec + (double)(start.tv_usec)/1000000.0;//微秒
    //保存信息
    snprintf(backup_msg[0], MALLOC_SIZE-1, "%s", mode);
    snprintf(backup_msg[1], MALLOC_SIZE-1, "%s", packagename);
    snprintf(backup_msg[2], MALLOC_SIZE-1, "%s", scheme);
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
    return ret;
}

