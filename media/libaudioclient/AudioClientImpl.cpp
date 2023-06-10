#include <utils/Log.h>
#include <cutils/properties.h>
#include <media/AudioSystem.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <binder/IPCThreadState.h>
#include <binder/PermissionController.h>
#include "ShootDetectWrapper.h"
#include "utils.h"
#include "AudioClientImpl.h"

#include "send_data_to_xlog.h"
#ifdef MIAUDIO_DUMP_PCM
#include <sys/types.h>
#include <sys/stat.h>
#define DEFAULT_RECORD_DUMP_DIR "/data/miuilog/bsp/audio/traces/audio_record"
#define DEFAULT_TRACK_DUMP_DIR "/data/miuilog/bsp/audio/traces/audio_track"
#define DEFAULT_FLINGER_DUMP_DIR "/data/miuilog/bsp/audio/traces/audio_flinger"
#define DEFAULT_INIT_TRACK_DUMP_DIR "/data/miuilog/bsp/audio/traces/init_audio_track"
#endif

#ifdef MIAUDIO_3D_PLAY
#include "CMusicLimiter.h"
#include "Mi3DAudioMapping.h"
#endif

#define UNUSED(x) (void)x

#define APP_MAX_CNT 32
#define MAX_APP_NAME_LEN 64

namespace android {

int AudioClientImpl::gameEffectAudioRecordChangeInputSource(
                pid_t pid,
                audio_format_t format,
                audio_source_t inputSource,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                audio_source_t &output,
                audio_flags_mask_t &flags) {
#ifdef MIAUDIO_GAME_EFFECT
   /**
    * games in voip_tx_game_app_list don't establish voip usecase
    * when 3.5mm and USB headset plugged in.
    * in BT and Speaker case, inputsource is voice-communicaiton
    * in Headset and USB Headset case, inputsource is default
    */
    if (AudioSystem::getDeviceConnectionState(
        (audio_devices_t)AUDIO_DEVICE_OUT_WIRED_HEADSET,"") ==
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE ||
        (AudioSystem::getDeviceConnectionState(
        (audio_devices_t)AUDIO_DEVICE_OUT_USB_HEADSET,"") ==
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE) ||
        (AudioSystem::getDeviceConnectionState(
        (audio_devices_t)AUDIO_DEVICE_OUT_WIRED_HEADPHONE,"") ==
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE)) {
        // check UI switch is on or off
        // if (checkGameSoundVolumeGapSwitch()) {
        {
            // only games in voip_game_app_list support change stream type
            if (isAppNeedtoChangeInputSource(pid, format, inputSource,
                    sampleRate, channelMask)){
                ALOGD("change inputSource to AUDIO_SOURCE_VOICE_COMMUNICATION");
                output = AUDIO_SOURCE_VOICE_COMMUNICATION;
                flags = static_cast<audio_flags_mask_t>(flags | AUDIO_FLAG_CAPTURE_PRIVATE);
            }
        }
    }
#else
    UNUSED(pid);
    UNUSED(format);
    UNUSED(inputSource);
    UNUSED(sampleRate);
    UNUSED(channelMask);
    UNUSED(output);
    UNUSED(flags);
#endif
    return 0;
}

int AudioClientImpl::gameEffectAudioTrackChangeStreamType(
                pid_t pid,
                audio_format_t format,
                audio_stream_type_t streamType,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                audio_stream_type_t &output) {
#ifdef MIAUDIO_GAME_EFFECT
   /**
    * games in voip_rx_game_app_list don't establish voip usecase
    * when 3.5mm and USB headset plugged in.
    * in BT and Speaker case, stream type is voice-call, sometimes stream
    * is music when routing to
    * speaker since some app reason.
    * in Headset and USB Headset case, stream type is music.
    */
    {
        // check UI switch is on or off
        if (checkGameSoundVolumeGapSwitch()) {
            // only games in voip_rx_game_app_list support change stream type
            if (isAppNeedtoChangeStreamType(pid, format, streamType,
                    sampleRate, channelMask)){
                ALOGD("change streatype to AUDIO_STREAM_VOICE_CALL");
                output = AUDIO_STREAM_VOICE_CALL;
            }
        }
    }
#else
    UNUSED(pid);
    UNUSED(format);
    UNUSED(streamType);
    UNUSED(sampleRate);
    UNUSED(channelMask);
    UNUSED(output);
#endif
    return 0;
}

void AudioClientImpl::gameEffectAudioTrackInitShootDetect(
                pid_t processID,
                audio_format_t format,
                audio_stream_type_t streamType,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                const wp<AudioTrack>& owner) {
#ifdef MIAUDIO_GAME_4D_VIBRATE
    ShootDetectWrapper* shoot;
    // Query app's info to check whether shoot detect allowed or not
    // ShootDetectWrapper is instantiated whatever UI is on or off
    unsigned int appID;

    if (isAppSupportedBySound4D(processID, format, streamType, sampleRate,
            channelMask, &appID)) {
        sp<AudioTrack> at = owner.promote();
        if (at == nullptr) {
            return;
        }
        at->isAppSupported4D = true;
        if (at->mShootDetectWrapper == NULL) {
            shoot = new ShootDetectWrapper();
            if (shoot->initShootDetectLibAndVibrator(format, channelMask,
                    sampleRate, appID) != NO_ERROR) {
                delete shoot;
                shoot = NULL;
                ALOGE("ShootDetect Init Failed");
                return;
            }
            at->mShootDetectWrapper = (void*)shoot;
        }
    }
#else
    UNUSED(processID);
    UNUSED(format);
    UNUSED(streamType);
    UNUSED(sampleRate);
    UNUSED(channelMask);
    UNUSED(owner);
#endif
}

void AudioClientImpl::gameEffectAudioTrackReleaseShootDetect(
        const wp<AudioTrack>& owner) {
#ifdef MIAUDIO_GAME_4D_VIBRATE
    ShootDetectWrapper* shoot;
    sp<AudioTrack> at = owner.promote();
    if (at == nullptr) {
        return;
    }
    if (at->mShootDetectWrapper != NULL) {
        shoot = (ShootDetectWrapper*)at->mShootDetectWrapper;
        shoot->releaseShootDetectLibAndVibrator();
        delete shoot;
        at->mShootDetectWrapper = NULL;
        ALOGD("ShootDetect Release Success");
    }
#else
    UNUSED(owner);
#endif
}

void AudioClientImpl::gameEffectAudioTrackProcessShootDetectAndVibrator(
                const wp<AudioTrack>& owner,
                android::AudioTrack::Buffer audioBuffer,
                uint32_t channelCount)
{
#ifdef MIAUDIO_GAME_4D_VIBRATE
    ShootDetectWrapper* shoot;
    sp<AudioTrack> at = owner.promote();
    if (at == nullptr) {
        return;
    }
    if (at->isAppSupported4D &&
            property_get_bool("vendor.audio.game4D.switch", false) &&
            at->mShootDetectWrapper != NULL) {
        shoot = (ShootDetectWrapper*)at->mShootDetectWrapper;
        uint32_t sdFrLen = (audioBuffer.size >> 1) / channelCount;
        shoot->ProcessShootDetectLibAndVibrator(
            (DTYPE*)audioBuffer.raw, sdFrLen);
    }
#else
    UNUSED(owner);
    UNUSED(audioBuffer);
    UNUSED(channelCount);
#endif
}


status_t AudioClientImpl::checkFilePath(const char *filePath)
{
#ifdef MIAUDIO_DUMP_PCM
    char tmp[PATH_MAX];
    int i = 0;

    while (*filePath) {
        tmp[i] = *filePath;

        if (*filePath == '/' && i) {
            tmp[i] = '\0';
            if (access(tmp, F_OK) != 0) {
                if (mkdir(tmp, 0770) == -1) {
                    ALOGE("mkdir error! %s",(char*)strerror(errno));
                    return -1;
                }
            }
            tmp[i] = '/';
        }
        i++;
        filePath++;
    }
#else
    UNUSED(filePath);
#endif
    return 0;
}

int AudioClientImpl::dumptoFile(
    const char *filePath,
    void *buffer,
    size_t size)
{
#ifdef MIAUDIO_DUMP_PCM
    int fd = -1;
    int written = -1;
    umask(0000);
    if(checkFilePath(filePath) < 0){
        ALOGE("dump fail!!!");
        return -1;
    }
    fd = open(filePath, O_CREAT|O_RDWR|O_APPEND, 0777);
    if(fd == -1){
        ALOGD("dumpfd create failed:errno=%d",errno);
        return -1;
    }
    written = write(fd,buffer,size);
    close(fd);
    return written;

#else
    UNUSED(filePath);
    UNUSED(buffer);
    UNUSED(size);
#endif
    return 0;
}

void AudioClientImpl::dumpInitAudioTrackDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName)
{
#ifdef MIAUDIO_DUMP_PCM
    if (!(dumpSwitch & mAudioDumpSwitch)) 
        return;
    char filePathbypid[256];
    char filePathbySessionId[256];

    if (format == AUDIO_FORMAT_MP3){
        snprintf(filePathbypid, sizeof(filePathbypid), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_pkg_%s.mp3",
                 DEFAULT_INIT_TRACK_DUMP_DIR, format, sampleRate, channelCount, pid, packageName);
        snprintf(filePathbySessionId, sizeof(filePathbySessionId), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_sessionId_%d_pkg_%s.mp3",
                 DEFAULT_INIT_TRACK_DUMP_DIR, format, sampleRate, channelCount, pid, sessionId, packageName);
    }
    else{
         snprintf(filePathbypid, sizeof(filePathbypid), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_pkg_%s.pcm",
                 DEFAULT_INIT_TRACK_DUMP_DIR, format, sampleRate, channelCount, pid, packageName);
         snprintf(filePathbySessionId, sizeof(filePathbySessionId), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_sessionId_%d_pkg_%s.pcm",
                 DEFAULT_INIT_TRACK_DUMP_DIR, format, sampleRate, channelCount, pid, sessionId, packageName);
    }

    dumptoFile(filePathbypid,buffer,size);

    dumptoFile(filePathbySessionId,buffer,size);

#else
    UNUSED(buffer);
    UNUSED(size);
    UNUSED(format);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(dumpSwitch);
    UNUSED(pid);
    UNUSED(sessionId);
    UNUSED(packageName);
#endif
}

void AudioClientImpl::dumpAudioTrackDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName)
{
#ifdef MIAUDIO_DUMP_PCM
    if (!(dumpSwitch & mAudioDumpSwitch)) 
        return;

    char filePathbypid[256];
    char filePathbySessionId[256];

    if (format == AUDIO_FORMAT_MP3){
        snprintf(filePathbypid, sizeof(filePathbypid), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_pkg_%s.mp3",
                 DEFAULT_TRACK_DUMP_DIR, format, sampleRate, channelCount, pid, packageName);
        snprintf(filePathbySessionId, sizeof(filePathbySessionId), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_sessionId_%d_pkg_%s.mp3",
                 DEFAULT_TRACK_DUMP_DIR, format, sampleRate, channelCount, pid, sessionId, packageName);
    }
    else{
         snprintf(filePathbypid, sizeof(filePathbypid), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_pkg_%s.pcm",
                 DEFAULT_TRACK_DUMP_DIR, format, sampleRate, channelCount, pid, packageName);
         snprintf(filePathbySessionId, sizeof(filePathbySessionId), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_sessionId_%d_pkg_%s.pcm",
                 DEFAULT_TRACK_DUMP_DIR, format, sampleRate, channelCount, pid, sessionId, packageName);
    }

    dumptoFile(filePathbypid,buffer,size);

    dumptoFile(filePathbySessionId,buffer,size);


#else
    UNUSED(buffer);
    UNUSED(size);
    UNUSED(format);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(dumpSwitch);
    UNUSED(pid);
    UNUSED(sessionId);
    UNUSED(packageName);
#endif
}

void AudioClientImpl::dumpAudioRecordDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName)
{
#ifdef MIAUDIO_DUMP_PCM
    if (!(dumpSwitch & mAudioDumpSwitch)) 
        return;
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s_fmt_%#x_sr_%u_ch_%u_pid_%d_sessionId_%d_pkg_%s.pcm",
                 DEFAULT_RECORD_DUMP_DIR, format, sampleRate, channelCount, pid, sessionId, packageName);

    dumptoFile(filePath,buffer,size);
#else
    UNUSED(buffer);
    UNUSED(size);
    UNUSED(format);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(dumpSwitch);
    UNUSED(pid);
    UNUSED(sessionId);
    UNUSED(packageName);
#endif
}

void AudioClientImpl::dumpAudioFlingerDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                const sp<StreamOutHalInterface>& streamOut)
{
#ifdef MIAUDIO_DUMP_PCM
    if (!(dumpSwitch & mAudioDumpSwitch)) 
        return;
    audio_channel_mask_t channelMask;
    status_t result = streamOut->getAudioProperties(&sampleRate, &channelMask, &format);
    if (result != OK) {
        sampleRate = 0;
        channelCount = AUDIO_CHANNEL_NONE;
        format = AUDIO_FORMAT_DEFAULT;
    } else {
        channelCount = audio_channel_count_from_out_mask(channelMask);
    };

    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s_fmt_%#x_sr_%u_ch_%u.pcm",
                 DEFAULT_FLINGER_DUMP_DIR, format, sampleRate, channelCount);

    dumptoFile(filePath,buffer,size);

#else
    UNUSED(buffer);
    UNUSED(size);
    UNUSED(format);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(dumpSwitch);
#endif
}


void AudioClientImpl::getAudioDumpSwitch()
{
    if(mCounts >= 50 || mCounts == 0){
        mAudioDumpSwitch = property_get_int32("audio.dump.switch", 0);
        mCounts = 0;
    }
    ++mCounts;
}

void AudioClientImpl::dumpAudioDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName,
                const sp<StreamOutHalInterface>& streamOut)
{
#ifdef MIAUDIO_DUMP_PCM
    getAudioDumpSwitch();
    //mAudioDumpSwitch = property_get_int32("audio.dump.switch", 0);
    if(mAudioDumpSwitch == 0){
        return;
    }
    switch (dumpSwitch) {
        case 1:
            dumpAudioTrackDataToFile(buffer, size, format, sampleRate, channelCount, dumpSwitch, pid, sessionId, packageName);
            break;
        case 2:
            dumpAudioFlingerDataToFile(buffer, size, format, sampleRate, channelCount, dumpSwitch, streamOut);
            break;
        case 4:
            dumpAudioRecordDataToFile(buffer, size, format, sampleRate, channelCount, dumpSwitch, pid, sessionId, packageName);
            break;
        case 8:
            dumpInitAudioTrackDataToFile(buffer, size, format, sampleRate, channelCount, dumpSwitch, pid, sessionId, packageName);
            break;
        default:
            break;
    }
#else
    UNUSED(buffer);
    UNUSED(size);
    UNUSED(format);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(dumpSwitch);
    UNUSED(pid);
    UNUSED(packageName);
    UNUSED(streamOut);
    UNUSED(sessionId);
#endif
    return;
}

void AudioClientImpl::initMi3DAudioMapping(void* &mi3DAudioMapping)
{
#ifdef MIAUDIO_3D_PLAY
    Mi3DAudioMapping* map;
    if (mi3DAudioMapping == NULL) {
        map = new Mi3DAudioMapping();
        if (map == NULL) {
            ALOGE("initMi3DAudioMapping Init Failed");
        }
        mi3DAudioMapping = (void*)map;
        ALOGD("Mi3DAudioMapping init Success");
    }
#else
    UNUSED(mi3DAudioMapping);
#endif
}

void AudioClientImpl::releaseMi3DAudioMapping(void* &mi3DAudioMapping)
{
#ifdef MIAUDIO_3D_PLAY
    if (mi3DAudioMapping != NULL) {
        Mi3DAudioMapping* map = (Mi3DAudioMapping*)mi3DAudioMapping;
        delete map;
        mi3DAudioMapping = NULL;
        ALOGD("Mi3DAudioMapping Release Success");
    }
#else
    UNUSED(mi3DAudioMapping);
#endif
}

void AudioClientImpl::setMi3DAudioMappingDeviceType(
               uint32_t* channelCountStart,
               void* mi3DAudioMapping)
{
#ifdef MIAUDIO_3D_PLAY
    int mode = 0;
    if (*channelCountStart == 6) {
        char value[PROPERTY_VALUE_MAX];
        if (property_get("vendor.audio.3daudio.headphone.status", value, NULL)) {
            if ( !String16("on").compare(String16(value)) ) {
                //ALOGD("Mi3DAudioMapping setDeviceType 1");
                mode = 1;
            } else if (!String16("off").compare(String16(value))) {
                //ALOGD("Mi3DAudioMapping setDeviceType 0");
                mode = 0;
            }
            if (mi3DAudioMapping != NULL) {
                Mi3DAudioMapping* map = (Mi3DAudioMapping*)mi3DAudioMapping;
                map->setDeviceType(mode);
            }
        }
    }
#else
    UNUSED(channelCountStart);
    UNUSED(mi3DAudioMapping);
#endif
}

void AudioClientImpl::processMi3DAudioMapping(
               int8_t *audioBuffer,
               size_t audioBuffersize,
               const void* buffer,
               size_t& userSize,
               size_t& written,
               const wp<AudioTrack>& owner,
               FILE* audioFileIn)
{
#ifdef MIAUDIO_3D_PLAY
    sp<AudioTrack> at = owner.promote();
    if (at == nullptr) {
        return;
    }
    size_t toWrite = audioBuffersize / 4 * 6;

    if (at->mMi3DAudioMapping != NULL) {
        Mi3DAudioMapping* map = (Mi3DAudioMapping*)at->mMi3DAudioMapping;
        if (at->format() == AUDIO_FORMAT_PCM_FLOAT || at->format() == AUDIO_FORMAT_PCM_SUB_FLOAT) {
            map->process((float *)audioBuffer, buffer, audioBuffersize);
        } else {
            map->process((void *)audioBuffer, buffer, audioBuffersize,
                                  at->frameSize()/at->channelCount());
        }

        if (audioFileIn) {
            ALOGE("Mi3DAudioMapping dump in");
            fwrite((const uint8_t *) buffer, 1, toWrite, audioFileIn);
        }

        buffer = ((const char *) buffer) + toWrite;
        userSize -= toWrite;
        written += audioBuffersize;
    }
#else
    UNUSED(audioBuffer);
    UNUSED(audioBuffersize);
    UNUSED(buffer);
    UNUSED(userSize);
    UNUSED(written);
    UNUSED(owner);
    UNUSED(audioFileIn);
#endif
}

#ifdef MIAUDIO_ULL_CHECK
std::set<std::string> ullForbiddenPackage = {
    "com.ss.android.ugc.aweme",      // 抖音短视频
    "com.ss.android.ugc.live",       // 抖音火山版
    "com.ss.android.ugc.aweme.lite", // 抖音极速版
    "com.smile.gifmaker",            // 快手
    "com.kuaishou.nebula",           // 快手极速版
    "com.zhiliaoapp.musically",      // TikTok
    "com.zhiliaoapp.musically.go",   // TikTok lite
    "com.android.camera",            // 相机
};

bool AudioClientImpl::isUllRecordSupport(uid_t uid)
{
    String16 clientName;
    std::set<std::string>::iterator it;
    PermissionController permissionController;
    Vector<String16> packages;
    bool isSupport = true;

    if (!property_get_bool("ro.vendor.audio.record.ull.support", true)) {
        permissionController.getPackagesForUid(uid, packages);
        if (!packages.isEmpty()) {
            clientName = packages[0];
        }
        it = ullForbiddenPackage.find(String8(clientName).string());
        if (it != ullForbiddenPackage.end()) {
            isSupport = false;
        }
    }

    return isSupport;
}

//MIUI ADD: onetrack rbis start
int AudioClientImpl::reportAudiotrackParameters(int playbackTime, const char* clientName, const char* usage, const char* flags, int channelMask, const char* format, int sampleRate)
{
    int ret = 0;
    ALOGD("%s, playbackTime is %d, clientName is %s, usage is %s, flags is %s, channelMask is %d, format is %s, sampleRate is %d",
        __func__, playbackTime, clientName, usage, flags, channelMask, format, sampleRate);
    send_audiotrack_parameter_to_xlog(playbackTime, clientName, usage, flags, channelMask, format, sampleRate);
    return ret;
}
//MIUI ADD: onetrack rbis end

#endif
extern "C" IAudioClientStub* create() {
    return new AudioClientImpl;
}

extern "C" void destroy(IAudioClientStub* impl) {
    delete impl;
}

}
