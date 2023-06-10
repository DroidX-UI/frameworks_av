#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <media/AudioParameter.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <media/AudioSystem.h>
#include "utils.h"

namespace android {
//--------------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#define AUDIO_PARAMETERS_AUDIO_GAME_SOUND_EFFECT_SWITCH "audio_game_sound_effect_switch"
#define AUDIO_PARAMETERS_AUDIO_GAME_4D_SWITCH  "audio_game_4D_switch"
#define AUDIO_PARAMETER_KEY_GAME_SOUNED_VOLUME_GAP_SWITCH "audio_game_sound_volume_gap_switch"

typedef enum {
    APP_ID_DEFAULT = 0,
    APP_ID_4D_HYXD = APP_ID_DEFAULT,
    APP_ID_4D_END,
} app_game_name_4d_t;

typedef enum {
    APP_ID_VOIP_PUBGMHD = 0,
    APP_ID_VOIP_PUBGM = 1,
    APP_ID_VOIP_WZRY = 2,
    APP_ID_VOIP_HYXD = 3,
    APP_ID_VOIP_IG = 4,
    APP_ID_VOIP_CLDT = 5,
    APP_ID_VOIP_MRZH_MI = 6,
    APP_ID_VOIP_MRZH = 7,
    APP_ID_VOIP_END = 8,
} app_game_name_voip_t;

typedef struct app_audiorecord_info_list
{
    const char* appName;
    audio_source_t inputSource;
    uint32_t sampleRate;
    audio_format_t format;
    audio_channel_mask_t channelMask;
}app_audiorecord_info_list_t;


static app_audiorecord_info_list_t voip_tx_game_app_list[] = {
    // voip audiorecord: AUDIO_SOURCE_DEFAULT/16000Hz/16bit/0x10
    [APP_ID_VOIP_PUBGMHD] = {"com.tencent.tmgp.pubgmhd", AUDIO_SOURCE_DEFAULT, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_MONO},
    // voip audiorecord: AUDIO_SOURCE_DEFAULT/16000Hz/16bit/0x10
    [APP_ID_VOIP_PUBGM] = {"com.tencent.tmgp.pubgm", AUDIO_SOURCE_DEFAULT, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_MONO},
    // voip audiorecord: AUDIO_SOURCE_DEFAULT/16000Hz/16bit/0x10
    [APP_ID_VOIP_WZRY] = {"com.tencent.tmgp.sgame", AUDIO_SOURCE_DEFAULT, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_MONO},
    // voip audiorecord: AUDIO_SOURCE_DEFAULT/44100Hz/16bit/0x10
    [APP_ID_VOIP_HYXD] = {"com.netease.hyxd.mi", AUDIO_SOURCE_MIC, 44100, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_MONO},
    // voip audiorecord: AUDIO_SOURCE_DEFAULT/16000Hz/16bit/0x10
    [APP_ID_VOIP_IG] = {"com.tencent.ig", AUDIO_SOURCE_DEFAULT, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_MONO},
    // voip audiorecord: AUDIO_SOURCE_DEFAULT/16000Hz/16bit/0x10
    [APP_ID_VOIP_CLDT] = {"com.activision.callofduty.shooter", AUDIO_SOURCE_DEFAULT, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_MONO},
    // voip audiorecord: AUDIO_SOURCE_DEFAULT/44100Hz/16bit/0x10
    [APP_ID_VOIP_MRZH_MI] = {"com.netease.mrzh.mi", AUDIO_SOURCE_MIC, 44100, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_MONO},
    [APP_ID_VOIP_MRZH] = {"com.netease.mrzh", AUDIO_SOURCE_MIC, 44100, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_IN_MONO}
};

typedef struct app_audiotrack_info_list
{
    const char* appName;
    audio_stream_type_t streamType;
    uint32_t sampleRate;
    audio_format_t format;
    audio_channel_mask_t channelMask;
}app_audiotrack_info_list_t;

static app_audiotrack_info_list_t fourD_game_app_list[] = {
    // shoot sound: AUDIO_STREAM_MUSIC/48000Hz/16bit/0x3
    //[APP_ID_4D_PUBGMHD] = {"com.tencent.tmgp.pubgmhd", AUDIO_STREAM_MUSIC, 48000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO},
    // shoot sound: AUDIO_STREAM_MUSIC/48000/16bit/0x3
    //[APP_ID_4D_PUBGM] = {"com.tencent.tmgp.pubgm", AUDIO_STREAM_MUSIC, 48000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO},
    // shoot sound: AUDIO_STREAM_MUSIC/24000/16bit/0x3
    [APP_ID_4D_HYXD] = {"com.netease.hyxd.mi", AUDIO_STREAM_MUSIC, 24000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO}
};

static app_audiotrack_info_list_t voip_rx_game_app_list[] = {
    // voip stream type: AUDIO_STREAM_MUSIC/16000Hz/16bit/0x1
    [APP_ID_VOIP_PUBGMHD] = {"com.tencent.tmgp.pubgmhd", AUDIO_STREAM_MUSIC, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_MONO},
    // voip stream type: AUDIO_STREAM_MUSIC/16000Hz/16bit/0x1
    [APP_ID_VOIP_PUBGM] = {"com.tencent.tmgp.pubgm", AUDIO_STREAM_MUSIC, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_MONO},
    // voip stream type: AUDIO_STREAM_MUSIC/16000Hz/16bit/0x1
    [APP_ID_VOIP_WZRY] = {"com.tencent.tmgp.sgame", AUDIO_STREAM_MUSIC, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_MONO},
    // voip stream type: AUDIO_STREAM_DEFAULT/48000Hz/16bit/0x1
    [APP_ID_VOIP_HYXD] = {"com.netease.hyxd.mi", AUDIO_STREAM_DEFAULT, 48000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_MONO},
    // voip stream type: AUDIO_STREAM_MUSIC/16000Hz/16bit/0x1
    [APP_ID_VOIP_IG] = {"com.tencent.ig", AUDIO_STREAM_MUSIC, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_MONO},
    // voip stream type: AUDIO_STREAM_MUSIC/16000Hz/16bit/0x1
    [APP_ID_VOIP_CLDT] = {"com.activision.callofduty.shooter", AUDIO_STREAM_MUSIC, 16000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_MONO},
    // voip stream type: AUDIO_STREAM_DEFAULT/48000Hz/16bit/0x1
    [APP_ID_VOIP_MRZH_MI] = {"com.netease.mrzh.mi", AUDIO_STREAM_DEFAULT, 48000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_MONO},
    [APP_ID_VOIP_MRZH] = {"com.netease.mrzh", AUDIO_STREAM_DEFAULT, 48000, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_MONO}
};

static status_t getAppNameByPid(int pid, char *pName, int size)
{
    int fd, ret;
    char path[128];

    if (pName == NULL || size <= 0) {
        return -ENOENT;
    }

    memset(pName, 0, size);
    memset(path, 0, 128);
    snprintf(path, 128, "/proc/%d/cmdline", pid);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ALOGE("open %s error", path);
        return NAME_NOT_FOUND;
    } else {
        ret = read(fd, pName, size-1);
        close(fd);
        if (ret < 0)
            ret = 0;
        pName[ret] = 0;
        return NO_ERROR;
    }
}

bool checkGameSoundVolumeGapSwitch()
{
    String8 reply;
    String8 value;
    AudioParameter param;

    reply = AudioSystem::getParameters(String8(AUDIO_PARAMETER_KEY_GAME_SOUNED_VOLUME_GAP_SWITCH));
    param = AudioParameter(reply);
    if (param.get(String8(AUDIO_PARAMETER_KEY_GAME_SOUNED_VOLUME_GAP_SWITCH), value) == NO_ERROR) {
        return (value == AudioParameter::valueOn) ? true : false;
    } else
        return false;
}

bool isAppNeedtoChangeInputSource(pid_t processID, audio_format_t format,
                                       audio_source_t inputSource,  uint32_t sampleRate, audio_channel_mask_t channelMask)
{
    bool isAppinWhiteList = false;
    char app_name[256];
    status_t app_state;

    app_state = getAppNameByPid(processID, app_name, 255);
    if(app_state == NO_ERROR) {
        for (unsigned int app_index = 0; app_index < APP_ID_VOIP_END; app_index++) {
            if (!strncmp(app_name, voip_tx_game_app_list[app_index].appName, strlen(voip_tx_game_app_list[app_index].appName)) &&
                format == voip_tx_game_app_list[app_index].format &&
                sampleRate == voip_tx_game_app_list[app_index].sampleRate &&
                inputSource == voip_tx_game_app_list[app_index].inputSource &&
                channelMask == voip_tx_game_app_list[app_index].channelMask){
                isAppinWhiteList = true;
                break;
            }
        }
    }
    return isAppinWhiteList;
}
bool isAppNeedtoChangeStreamType(pid_t processID, audio_format_t format,
                                      audio_stream_type_t streamType,  uint32_t sampleRate, audio_channel_mask_t channelMask)
{
   bool isAppinWhiteList = false;
   char app_name[256];
   status_t app_state;

   app_state = getAppNameByPid(processID, app_name, 255);
   if(app_state == NO_ERROR) {
       for (unsigned int app_index = 0; app_index < APP_ID_VOIP_END; app_index++) {
           if (!strncmp(app_name, voip_rx_game_app_list[app_index].appName, strlen(voip_rx_game_app_list[app_index].appName)) &&
               format == voip_rx_game_app_list[app_index].format &&
               sampleRate == voip_rx_game_app_list[app_index].sampleRate &&
               streamType == voip_rx_game_app_list[app_index].streamType &&
               channelMask == voip_rx_game_app_list[app_index].channelMask){
               isAppinWhiteList = true;
               break;
           }
       }
   }
   return isAppinWhiteList;
}

bool checkGameSound4DUISwitch()
{
    String8 reply;
    String8 value;
    AudioParameter param;

    reply = AudioSystem::getParameters(String8(AUDIO_PARAMETERS_AUDIO_GAME_4D_SWITCH));
    param = AudioParameter(reply);
    if (param.get(String8(AUDIO_PARAMETERS_AUDIO_GAME_4D_SWITCH), value) == NO_ERROR) {
        return (value == AudioParameter::valueOn) ? true : false;
    } else
        return false;
}

bool isAppSupportedBySound4D(pid_t processID, audio_format_t format,
                                       audio_stream_type_t streamType,  uint32_t sampleRate, audio_channel_mask_t channelMask, unsigned int* appID)
{
    bool isAppinWhiteList = false;
    char app_name[256];
    status_t app_state;
    *appID = APP_ID_DEFAULT;

    app_state = getAppNameByPid(processID, app_name, 255);
    if(app_state == NO_ERROR) {
        for (unsigned int app_index = 0; app_index < APP_ID_4D_END; app_index++) {
            if (!strncmp(app_name, fourD_game_app_list[app_index].appName, strlen(fourD_game_app_list[app_index].appName)) &&
                format == fourD_game_app_list[app_index].format &&
                sampleRate == fourD_game_app_list[app_index].sampleRate &&
                streamType == fourD_game_app_list[app_index].streamType &&
                channelMask == fourD_game_app_list[app_index].channelMask){
                isAppinWhiteList = true;
                *appID = app_index;
                break;
            }
        }
    }
    return isAppinWhiteList;
}

#ifdef __cplusplus
            }
#endif //__cplusplus

}  // namespace android
