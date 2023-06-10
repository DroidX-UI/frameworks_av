
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <utils/Trace.h>
#include <binder/Parcel.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <media/AudioParameter.h>
#include <media/AudioSystem.h>

#include "AudioFlingerImpl.h"

//for onetrack rbis
#include "xlog_api.h"
#include "send_data_to_xlog.h"
//AudioCloudCtrl
#include "CloudCtrlFwk.h"
//#ifdef MIAUDIO_GAME_4D_VIBRATE
#include <android/hardware/vibrator/IVibrator.h>
namespace aidl = android::hardware::vibrator;
//#endif // MIAUDIO_GAME_4D_VIBRATE

#ifdef MIAUDIO_DUMP_PCM
#include <sys/types.h>
#include <sys/stat.h>
#define DEFAULT_FLINGER_DUMP_DIR "/data/miuilog/bsp/audio/traces/audio_flinger"
#endif

#define XIAOMI_VOICE_CHANGE 1
#define YOUME_VOICE_CAHNGE 2
#define LOG_TAG "AudioFlingerImpl"

namespace android {

// MIAUDIO_GAME_4D_VIBRATE begin
static sp<aidl::IVibrator> sVibratorHal = nullptr;

typedef struct {
    int vibratorTime;
    vibrator_type_t type;
} vibrator_type_info_t;
static vibrator_type_info_t vibratorInfo[TYPE_VIBRATOR_NUM];

typedef enum {
    TYPE_DEFAULT = 1,
    TYPE_LINEAR,
} motor_type_t;

static motor_type_t motor_type;

typedef enum {
    TYPE_SOUND_STRENGTH_LEVEL1 = 1,
    TYPE_SOUND_STRENGTH_LEVEL2,
    TYPE_SOUND_STRENGTH_LEVEL3,
} sound_type_t;

static void vibratorOn(vibrator_type_t type)
{
    if (motor_type == TYPE_DEFAULT) {
        if (type >= TYPE_VIBRATOR_NUM)
            return;

        if (sVibratorHal != nullptr) {
            auto retStatus =
                sVibratorHal->on(vibratorInfo[type].vibratorTime, nullptr);
            if (!retStatus.isOk()) {
                ALOGV("vibratorOn command failed");
            }
        } else {
            ALOGW("Tried to vibrate but there is no vibrator device.");
        }
    } else {
        if (type >= TYPE_VIBRATOR_NUM)
            return;

        if (sVibratorHal != nullptr) {
            int32_t lengthMs;
            aidl::Effect effect;
            switch (type) {
                case TYPE_VIBRATOR_WEAK:
                {
                    effect = aidl::Effect::CLICK;
                    break;
                }
                case TYPE_VIBRATOR_NORMAL:
                {
                    effect = aidl::Effect::TICK;
                    break;
                }
                case TYPE_VIBRATOR_STRONG:
                {
                    effect = aidl::Effect::HEAVY_CLICK;
                    break;
                }
                default:
                    ALOGE("vibratorOn failed with invalid vibrator_type:%u! , fallback to CLICK", type);
                    effect = aidl::Effect::CLICK;
            }
            auto ret = sVibratorHal->perform(effect,
                aidl::EffectStrength::STRONG, nullptr, &lengthMs);
            if(!ret.isOk()) {
                ALOGE("vibrator perform effect %d failed", TYPE_VIBRATOR_WEAK);
            }
        }
    }
}

AudioFlingerImpl::VibratorMsgHandler::VibratorMsgHandler() {
}

AudioFlingerImpl::VibratorMsgHandler::~VibratorMsgHandler() {
    mVibratorCommands.clear();
}

void AudioFlingerImpl::VibratorMsgHandler::onFirstRef() {
   run("VibratorMsgThread", ANDROID_PRIORITY_NORMAL);
}

bool AudioFlingerImpl::VibratorMsgHandler::threadLoop() {
    mLock.lock();
    while (!exitPending())
    {
        while (!mVibratorCommands.isEmpty() && !exitPending()) {
                sp<VibratorCommand> command = mVibratorCommands[0];
                mVibratorCommands.removeAt(0);

                switch (command->mVibratorType) {
                    case TYPE_VIBRATOR_STRONG:
                    case TYPE_VIBRATOR_NORMAL:
                    case TYPE_VIBRATOR_WEAK: {
                            mLock.unlock();
                            vibratorOn(command->mVibratorType);
                            mLock.lock();
                        }
                        break;
                    default:
                        break;
                }
        }

        if (!exitPending()) {
            mWaitWorkCV.wait(mLock);
        }
    }

    mLock.unlock();
    ALOGD("threadLoop exit");
    return true;
}

void AudioFlingerImpl::VibratorMsgHandler::sendCommand(vibrator_type_t type) {
    sp<VibratorCommand> command = new VibratorCommand();
    command->mVibratorType = type;
    {
        Mutex::Autolock _l(mLock);
        mVibratorCommands.insertAt(command, mVibratorCommands.size());
        mWaitWorkCV.signal();
    }
}

void AudioFlingerImpl::VibratorMsgHandler::exit() {
    ALOGD("VibratorMsgHandler::exit");
    {
        AutoMutex _l(mLock);
        requestExit();
        mWaitWorkCV.signal();
    }
    requestExitAndWait();
}
//  MIAUDIO_GAME_4D_VIBRATE end

int AudioFlingerImpl::gameEffectInitVibratorMsgHandler(
        const String8& keyValuePairs)
{
#ifdef MIAUDIO_GAME_4D_VIBRATE
    String8 filteredKeyValuePairs = keyValuePairs;
    AudioParameter param = AudioParameter(filteredKeyValuePairs);
        // vibrator init/uninit
        String8 value1;
        if (param.get(String8("vibrator_switch"), value1) == NO_ERROR) {
            bool isVibratorOn = (value1 == AudioParameter::valueOn);
            if (isVibratorOn) {
                ALOGD("vibrator_switch=on, type=%d", motor_type);
                if (sVibratorHal == nullptr) {
                    sVibratorHal = waitForVintfService<aidl::IVibrator>();
                    if (sVibratorHal == nullptr) {
                        ALOGE("IVirator::getService failed in %s", __func__);
                    }
                    char prop_str[PROP_VALUE_MAX] = {0};
                    property_get("sys.haptic.motor", prop_str, "");
                    if (strlen(prop_str))
                        motor_type = TYPE_LINEAR;
                    else
                        motor_type = TYPE_DEFAULT;
                    property_get("sys.haptic.down.strong", prop_str, "13");
                    vibratorInfo[TYPE_VIBRATOR_STRONG].vibratorTime =
                        atoi(prop_str);
                    property_get("sys.haptic.down.normal", prop_str, "8");
                    vibratorInfo[TYPE_VIBRATOR_NORMAL].vibratorTime =
                        atoi(prop_str);
                    property_get("sys.haptic.down.weak", prop_str, "3");
                    vibratorInfo[TYPE_VIBRATOR_WEAK].vibratorTime =
                        atoi(prop_str);
                    ALOGD("VibratorTime: Strong:%d, Normal:%d, Weak:%d",
                        vibratorInfo[TYPE_VIBRATOR_STRONG].vibratorTime,
                        vibratorInfo[TYPE_VIBRATOR_NORMAL].vibratorTime,
                        vibratorInfo[TYPE_VIBRATOR_WEAK].vibratorTime);
                }
                if (mVibratorMsgHandler == NULL) {
                   mVibratorMsgHandler = new VibratorMsgHandler();
                }
            } else {
                ALOGD("vibrator_switch=off");
                if (mVibratorMsgHandler != NULL) {
                    mVibratorMsgHandler->exit();
                    mVibratorMsgHandler.clear();
                }
            }
        }
        // vibrator on/off
        int value2;
        if (param.getInt(String8("vibrator_on_strength"), value2) == NO_ERROR) {
            if (mVibratorMsgHandler != NULL) {
                switch (value2) {
                case TYPE_SOUND_STRENGTH_LEVEL1:
                    mVibratorMsgHandler->sendCommand(TYPE_VIBRATOR_WEAK);
                    break;
                case TYPE_SOUND_STRENGTH_LEVEL2:
                    mVibratorMsgHandler->sendCommand(TYPE_VIBRATOR_NORMAL);
                    break;
                case TYPE_SOUND_STRENGTH_LEVEL3:
                    mVibratorMsgHandler->sendCommand(TYPE_VIBRATOR_STRONG);
                    break;
                default:
                    break;
                }
            }
            return OK;
        }
#else
    UNUSED(keyValuePairs);
#endif
    return BAD_VALUE;
}


// MIAUDIO_GLOBAL_AUDIO_EFFECT begin
/*global audio effect*/
status_t AudioFlingerImpl::setStreamVolume(
        const sp<AudioFlinger::EffectModule>& effect,
        audio_stream_type_t stream, float value) {
    effect->lock();
    status_t status = NO_ERROR;
    if ((effect->desc().flags & EFFECT_FLAG_STREAM_VOLUME_MASK) ==
            EFFECT_FLAG_STREAM_VOLUME_IND) {
        status_t cmdStatus;
        uint32_t status_size = sizeof(status_t);
        uint32_t cmd[2];
        uint32_t volume = (uint32_t)(value * (1 << 24));

        cmd[0] = stream;
        cmd[1] = volume;

        status = effect->mEffectInterface->command(EFFECT_CMD_SET_STREAM_VOLUME,
                                              sizeof(uint32_t) * 2,
                                              cmd,
                                              &status_size,
                                              &cmdStatus);
        if (status == NO_ERROR) {
            status = cmdStatus;
        }
    }
    effect->unlock();
    return status;
}

status_t AudioFlingerImpl::setStandby(
        const sp<AudioFlinger::EffectModule>& effect) {
    effect->lock();
    status_t status = NO_ERROR;
    if ((effect->desc().flags & EFFECT_FLAG_AUDIO_STANDBY_MASK) ==
            EFFECT_FLAG_AUDIO_STANDBY_IND) {
        status_t cmdStatus;
        uint32_t size = sizeof(status_t);
        status = effect->mEffectInterface->command(EFFECT_CMD_SET_AUDIO_STANDBY,
                                              0,
                                              NULL,
                                              &size,
                                              &cmdStatus);
        if (status == NO_ERROR) {
            status = cmdStatus;
        }
    }
    effect->unlock();
    return status;
}

status_t AudioFlingerImpl::setAppName(
        const sp<AudioFlinger::EffectModule>& effect,
        audio_stream_type_t stream, const char *name, int size) {
    effect->lock();
    status_t status = NO_ERROR;

    if ((effect->desc().flags & EFFECT_FLAG_APP_NAME_MASK) ==
            EFFECT_FLAG_APP_NAME_IND) {
        status_t cmdStatus;
        char data[256 + sizeof(uint32_t)] = {0};
        uint32_t status_size = sizeof(status_t);

        *((uint32_t *)data) = stream;
        strncpy((char *)((uint32_t *)data + 1), name, size);

        status = effect->mEffectInterface->command(EFFECT_CMD_SET_APP_NAME,
                                              size + sizeof(uint32_t),
                                              (void *)data,
                                              &status_size,
                                              &cmdStatus);
        if (status == NO_ERROR) {
            status = cmdStatus;
        }
    }
    effect->unlock();
    return status;
}

status_t AudioFlingerImpl::getAppNameByPid(int pid, char *pName, int size)
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
// MIAUDIO_GLOBAL_AUDIO_EFFECT end

void AudioFlingerImpl::audioEffectSetStreamVolume(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, float value) {
#ifdef MIAUDIO_GLOBAL_AUDIO_EFFECT
    size_t effectChainsSize = effectChains.size();
    for (size_t i = 0; i < effectChainsSize; i++) {
        size_t effectSize = effectChains[i]->mEffects.size();;
        for (size_t j = 0; j < effectSize; j++) {
            setStreamVolume(effectChains[i]->mEffects[j], stream, value);
        }
    }
#else
    UNUSED(effectChains);
    UNUSED(stream);
    UNUSED(value);
#endif
}

void AudioFlingerImpl::audioEffectSetStandby(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains) {
#ifdef MIAUDIO_GLOBAL_AUDIO_EFFECT
    size_t effectChainSize = effectChains.size();
    for (size_t i = 0; i < effectChainSize; i++) {
        size_t effectSize = effectChains[i]->mEffects.size();
        for (size_t j = 0; j < effectSize; j++) {
           setStandby(effectChains[i]->mEffects[j]);
        }
    }
#else
    UNUSED(effectChains);
#endif
}

void AudioFlingerImpl::audioEffectSetAppName(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, bool active) {
#ifdef MIAUDIO_GLOBAL_AUDIO_EFFECT
    char name[256];
    int pid = IPCThreadState::self()->getCallingPid();

    name[0] = active ? '+' :'-';
    getAppNameByPid(pid, name+1, 255);
    ALOGI("setAppName(), name=[%s], active=[%d]", name+1, active);

    size_t EffectChainsSize = effectChains.size();
    for (size_t i = 0; i < EffectChainsSize; i++) {
        size_t effectsSize = effectChains[i]->mEffects.size();
        for (size_t j = 0; j < effectsSize; j++) {
            setAppName(effectChains[i]->mEffects[j], stream, name, 256);
        }
    }
#else
    UNUSED(effectChains);
    UNUSED(stream);
    UNUSED(active);
#endif
}

void AudioFlingerImpl::setGameParameters(const String8& keyValuePairs,
        const DefaultKeyedVector< audio_io_handle_t, sp<AudioFlinger::RecordThread> >& RecordThreads) {
#ifdef MIAUDIO_VOICE_CHANGE
    ALOGD("setGameParameters keyvalue: %s", keyValuePairs.string());
    size_t count = RecordThreads.size();
    AudioParameter param = AudioParameter(keyValuePairs);
    bool needToSet = false;
    if (param.get(String8("misound_voice_change_switch"), gameModeOpen) == NO_ERROR) {
        needToSet = true;
    }
    if (param.get(String8("misound_voice_change_mode"), voiceMode) == NO_ERROR) {
        needToSet = true;
    }
    int temp_int;
    if (param.getInt(String8("misound_voice_change_uid"), temp_int) == NO_ERROR) {
        gameUid = temp_int;
        needToSet = true;
    }
    if (param.get(String8("misound_voice_change_pcakge"), package_name) == NO_ERROR) {
        needToSet = true;
    }
    if (needToSet) {
        for(size_t i = 0; i < count; i++)
            RecordThreads.valueAt(i)->setParameters(keyValuePairs);
    }
#ifdef MIAUDIO_MAGICVOICE
    String8 str_temp;
    needToSet = false;
    if(param.get(String8("misound_voice_change_switch"), str_temp)==NO_ERROR || param.get(String8("clearMagicVoiceInfo"), str_temp)==NO_ERROR)  magicVoiceInfo.clear();
    if(param.get(String8("magicVoiceInfo"), str_temp) == NO_ERROR || param.get(String8("clearMagicVoiceInfo"), str_temp) == NO_ERROR) gameModeOpen.clear();

    if (param.get(String8("magicVoiceInfo"), magicVoiceInfo) == NO_ERROR) {
        needToSet = true;
    }
    if (param.get(String8("clearMagicVoiceInfo"), str_temp) == NO_ERROR) {
        needToSet = true;
    }
    float temp_float;
    if (param.getFloat(String8("semitonesadjust"), temp_float) == NO_ERROR) {
        semitonesadjust = temp_float;
        needToSet = true;
    }
    if (param.getFloat(String8("fsadjust"), temp_float) == NO_ERROR) {
        fsadjust = temp_float;
        needToSet = true;
    }
    if (needToSet) {
        for(size_t i = 0; i < count; i++)
            RecordThreads.valueAt(i)->setParameters(keyValuePairs);
    }
#endif
#else
    UNUSED(keyValuePairs);
    UNUSED(RecordThreads);
#endif
    return;
}

void AudioFlingerImpl::updateCloudCtrlData(const String8& keyValuePairs) {
    ALOGD("AudioCloudControl: remote updated xml, so need update framework data");
    std::string strkv(keyValuePairs.c_str());
    std::string subv = strkv.erase(0, strlen("audiocloudctrl_xxx_"));
    if(keyValuePairs.find("audiocloudctrl_new_")!=std::string::npos && !subv.empty()){
        auto key =strkv.erase(strkv.find("="), strkv.size()-1);
        auto values =subv.erase(0, subv.find("=")+1);
        if(AudioCloudCtrl::CloudCtrlFwk::newKVPairs(key.c_str(), values.c_str()))
            ALOGD("AudioCloudCtrl: NewKvToCloudCtrlXml = true");
    }
    else if(keyValuePairs.find("audiocloudctrl_add_")!=std::string::npos && !subv.empty()){
        auto key =strkv.erase(strkv.find("="), strkv.size()-1);
        auto values =subv.erase(0, subv.find("=")+1);
        if(AudioCloudCtrl::CloudCtrlFwk::addKVPairs(key.c_str(), values.c_str()))
            ALOGD("AudioCloudCtrl: AddKvToCloudCtrlXml = true");
    }
    else if(keyValuePairs.find("audiocloudctrl_del_")!=std::string::npos && !subv.empty()){
        auto key =strkv.erase(strkv.find("="), strkv.size()-1);
        auto values =subv.erase(0, subv.find("=")+1);
        if(AudioCloudCtrl::CloudCtrlFwk::delKVPairs(key.c_str(), values.c_str()))
            ALOGD("AudioCloudCtrl: DelKvToCloudCtrlXml = true");
    }
}

void AudioFlingerImpl::getGameMode(String8& keyValuePair) {
#ifdef MIAUDIO_VOICE_CHANGE
    if(!voiceMode.isEmpty() && !gameModeOpen.isEmpty() && gameUid) {
        ALOGI("createRecord() setgameparameters open:%s mode:%s uid:%d",
            gameModeOpen.c_str(), voiceMode.c_str(), gameUid);
        String8 gameMode_str("misound_voice_change_switch=");
        gameMode_str.append(gameModeOpen);
        gameMode_str.append(";misound_voice_change_mode=");
        gameMode_str.append(voiceMode);
        gameMode_str.append(";misound_voice_change_uid=");
        std::string str = std::to_string(gameUid);
        gameMode_str.append(str.c_str());
        gameMode_str.append(";misound_voice_change_pcakge=");
        gameMode_str.append(package_name);
        keyValuePair = gameMode_str;
    }

#ifdef MIAUDIO_MAGICVOICE
    if(!magicVoiceInfo.isEmpty()) {
        String8 magicVoice_str("magicVoiceInfo=");
        magicVoice_str.append(magicVoiceInfo);
        magicVoice_str.append(";semitonesadjust=");
        magicVoice_str.append(std::to_string(semitonesadjust).c_str());
        magicVoice_str.append(";fsadjust=");
        magicVoice_str.append(std::to_string(fsadjust).c_str());
        magicVoice_str.append(";misound_voice_change_uid=");
        std::string str = std::to_string(gameUid);
        magicVoice_str.append(str.c_str());
        magicVoice_str.append(";misound_voice_change_pcakge=");
        magicVoice_str.append(package_name);
        keyValuePair = magicVoice_str;
    }
#endif
#else
    UNUSED(keyValuePair);
#endif
    return;
}

bool AudioFlingerImpl::isGameVoip(int uid, int sample_rate)
{
    ALOGD("isGameVoip uid:%d, sample_rate:%d", uid, sample_rate);
    if(sample_rate == 16000 && uid == mGameUid)
        return true;
    else
        return false;
}

void AudioFlingerImpl::setGameMode(
    const SortedVector< sp<AudioFlinger::RecordThread::RecordTrack> >& Tracks,
    int version, uint32_t sampleRate, uint32_t channelCount,
    YouMeMagicVoiceChanger* &mYouMeMagicVoiceChanger,
    ComprehensiveVocoderStereo* &VocoderStereo) {
#ifdef MIAUDIO_VOICE_CHANGE
    ALOGD("%s version %d open %d, mode %d, uid %d",
        __func__, version, mOpenGameMode, mVoiceMode, mGameUid);
    size_t size = Tracks.size();
    sp<AudioFlinger::RecordThread::RecordTrack> Track;

    for (size_t i = 0; i < size; i++) {
        if (Tracks[i] == NULL)
            return;
        Track = Tracks[i];

        if(version == XIAOMI_VOICE_CHANGE) {
            Track->useYouMeMagicVoice = false;
#ifdef MIAUDIO_MAGICVOICE
            if(mYouMeMagicVoiceChanger != nullptr){
                ALOGD("YouMeMagicVoiceChanger free");
                engine_releaseMagicVoiceEngine((void *)mYouMeMagicVoiceChanger);
                YouMeMagicVoice_deinit();
                mYouMeMagicVoiceChanger = nullptr;
            }
#endif
            if(Track->useVoiceChange) {
                if(!mOpenGameMode ||
                    !isGameVoip(Track->uid(),
                        Track->mRecordBufferConverter->getDstSampleRate())){
                    send_game_voice_time_to_xlog(GAME_VOICE_CHANGE, mVoiceMode_str.string(), mGameName.string(), "xiaomi",0);
                    ALOGD("%s: track(uid:%d sessionid:%d) close game mode", __func__, Track->uid(), Track->sessionId());
                    Track->useVoiceChange = false;
                    if(VocoderStereo != NULL) delete VocoderStereo;
                    VocoderStereo = NULL;
                    continue;
                }
                ALOGD("%s: track(uid:%d sessionid:%d) reset voicemode to %d", __func__, Track->uid(), Track->sessionId(), mVoiceMode);
                if (VocoderStereo != NULL) {
                    VocoderStereo->setFs(
                        Track->mRecordBufferConverter->getDstSampleRate());
                    VocoderStereo->setChannel(
                        Track->mRecordBufferConverter->getDstChannelCount());
                    VocoderStereo->setMode(mVoiceMode);
                }
                int msg_id = GAME_VOICE_CHANGE;
                if(mLastVoiceMode != mVoiceMode && mVoiceMode != NONEEFFECT && mGameName!=""){
                    mLastVoiceMode = mVoiceMode;
                    send_game_voice_time_to_xlog(GAME_VOICE_CHANGE, mVoiceMode_str.string(), mGameName.string(), "xiaomi",1);
                }
                return;
            }
            if(mOpenGameMode && mVoiceMode>0 &&
                isGameVoip(Track->uid(),
                    Track->mRecordBufferConverter->getDstSampleRate())){
                ALOGD("%s: track(uid:%d sessionid:%d) open game mode %d", __func__, Track->uid(), Track->sessionId(), mVoiceMode);
                Track->useVoiceChange = true;
                if(VocoderStereo == NULL) VocoderStereo =
                    new ComprehensiveVocoderStereo();
                VocoderStereo->setFs(
                    Track->mRecordBufferConverter->getDstSampleRate());
                VocoderStereo->setChannel(
                    Track->mRecordBufferConverter->getDstChannelCount());
                VocoderStereo->setMode(mVoiceMode);
                int msg_id = GAME_VOICE_CHANGE;
                if(mLastVoiceMode != mVoiceMode && mVoiceMode != NONEEFFECT && mGameName!=""){
                    mLastVoiceMode = mVoiceMode;
                    send_game_voice_time_to_xlog(GAME_VOICE_CHANGE, mVoiceMode_str.string(), mGameName.string(), "xiaomi",1);
                }
            }
        }
#ifdef MIAUDIO_MAGICVOICE
       else if(version == YOUME_VOICE_CAHNGE) {
            Track->useVoiceChange = false;
            if(VocoderStereo != NULL) {
                delete VocoderStereo;
                VocoderStereo = NULL;
            }
            if(Track->useYouMeMagicVoice)
            {
                if(!mOpenGameMode || !isGameVoip(Track->uid(),
                        Track->mRecordBufferConverter->getDstSampleRate())) {
                    ALOGD("YouMeMagicVoice %s: track(uid:%d sessionid:%d) close game mode", __func__, Track->uid(), Track->sessionId());
                    Track->useYouMeMagicVoice = false;
                    send_game_voice_time_to_xlog(GAME_VOICE_CHANGE, "unknown", mGameName.string(), "youme",0);
                    if(mYouMeMagicVoiceChanger != nullptr){
                        ALOGD("YouMeMagicVoiceChanger free");
                        engine_releaseMagicVoiceEngine((void *)mYouMeMagicVoiceChanger);
                        YouMeMagicVoice_deinit();
                        mYouMeMagicVoiceChanger = nullptr;
                    }
                    continue;
                }
                ALOGD("YouMeMagicVoice %s: track(uid:%d sessionid:%d) reset voicemode", __func__, Track->uid(), Track->sessionId());
                processMagicVoiceParameter(sampleRate, channelCount,
                    mYouMeMagicVoiceChanger);
                if(mOpenGameMode && mGameName!=""){
                    send_game_voice_time_to_xlog(GAME_VOICE_CHANGE, "unknown", mGameName.string(), "youme",1);
                }
                return;
            }
            if(mOpenGameMode && isGameVoip(Track->uid(),
                    Track->mRecordBufferConverter->getDstSampleRate())) {
                ALOGD("YouMeMagicVoice %s: track(uid:%d sessionid:%d) open game mode", __func__, Track->uid(), Track->sessionId());
                Track->useYouMeMagicVoice = true;
                VocoderStereo = NULL;
                if(mYouMeMagicVoiceChanger == nullptr){
                    ALOGI("new YouMeMagicVoiceChanger()");
                    YouMeMagicVoice_init();
                    mYouMeMagicVoiceChanger = (YouMeMagicVoiceChanger*) engine_createMagicVoiceEngine();
                    processMagicVoiceParameter(sampleRate, channelCount,
                        mYouMeMagicVoiceChanger);
                    if(mOpenGameMode && mGameName!=""){
                        send_game_voice_time_to_xlog(GAME_VOICE_CHANGE, "unknown", mGameName.string(), "youme",1);
                    }
                }
            }
        }
#endif
    }
#else
    UNUSED(Tracks);
    UNUSED(version);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(mYouMeMagicVoiceChanger);
    UNUSED(VocoderStereo);
#endif
}

void  AudioFlingerImpl::checkForGameParameter_l(const String8& keyValuePair,
    const SortedVector < sp<AudioFlinger::RecordThread::RecordTrack> >& Tracks,
    uint32_t sampleRate, uint32_t channelCount, void* &voiceChanger,
    void* &stereo) {
    YouMeMagicVoiceChanger* mYouMeMagicVoiceChanger =
        (YouMeMagicVoiceChanger*)voiceChanger;
    ComprehensiveVocoderStereo* VocoderStereo =
        (ComprehensiveVocoderStereo*)stereo;
#ifdef MIAUDIO_VOICE_CHANGE
    ALOGD("checkForGameParameter_l() set events %s", keyValuePair.c_str());
    AudioParameter param = AudioParameter(keyValuePair);
    String8 str;
    bool needToSet = false;

    if (param.get(String8("misound_voice_change_pcakge"), str) == NO_ERROR){
        if(str != mGameName){
            mGameName = str;
            needToSet= true;
        }
    }

    if (param.get(String8("misound_voice_change_switch"), str) == NO_ERROR){
        bool openGameMode = str=="on" ? true : false;
        mOpenGameMode = openGameMode;
        needToSet= true;
    }
    if (param.get(String8("misound_voice_change_mode"), str) == NO_ERROR){
        mVoiceMode_str = str;
        int voiceMode;
        if(str=="loli") voiceMode = LOLI;
        else if(str=="lady") voiceMode = LADY;
        else if(str=="cartoon") voiceMode = CARTOON;
        else if(str=="robot") voiceMode = ROBOT;
        else if(str=="men") voiceMode = MEN;
        else voiceMode = NONEEFFECT;
        if(mVoiceMode != voiceMode){
            mVoiceMode = voiceMode;
            needToSet= true;
        }
    }

    int value;
    if (param.getInt(String8("misound_voice_change_uid"), value) == NO_ERROR){
        if(mGameUid != value){
            mGameUid = value;
        }
        needToSet = true;
    }

    if(needToSet) {
        setGameMode(Tracks,XIAOMI_VOICE_CHANGE,sampleRate,channelCount,
            mYouMeMagicVoiceChanger, VocoderStereo);
        voiceChanger = mYouMeMagicVoiceChanger;
        stereo = VocoderStereo;
    }
#ifdef MIAUDIO_MAGICVOICE
    needToSet = false;
    if (param.get(String8("magicVoiceInfo"), str) == NO_ERROR) {
        mOpenGameMode = true;
        needToSet = true;
        engine_setMagicVoiceInfo(str.c_str());
    } else if (param.get(String8("clearMagicVoiceInfo"), str) == NO_ERROR) {
        mOpenGameMode = false;
        needToSet = true;
        engine_clearMagicVoiceInfo();
    }
    if (param.getFloat(String8("semitonesadjust"), semitonesadjust) == NO_ERROR) {
        engine_setMagicVoiceAdjust(fsadjust, semitonesadjust);
    }
    if (param.getFloat(String8("fsadjust"), fsadjust) == NO_ERROR) {
        engine_setMagicVoiceAdjust(fsadjust, semitonesadjust);
    }

    if(needToSet) {
        setGameMode(Tracks,YOUME_VOICE_CAHNGE,sampleRate,channelCount,
            mYouMeMagicVoiceChanger, VocoderStereo);
        voiceChanger = mYouMeMagicVoiceChanger;
        stereo = VocoderStereo;
        if (param.get(String8("magicVoiceInfo"), str) == NO_ERROR) {
            engine_setMagicVoiceInfo(str.c_str());
        }
    }
#endif
#else
    UNUSED(keyValuePair);
    UNUSED(Tracks);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(voiceChanger);
    UNUSED(stereo);
#endif
    return;

}

bool AudioFlingerImpl::voiceProcess(
    const sp<AudioFlinger::RecordThread::RecordTrack>& activeTrack,
    size_t framesOut, size_t frameSize, uint32_t sampleRate,
    uint32_t channelCount, void* &voiceChanger, void* &stereo) {
#ifdef MIAUDIO_VOICE_CHANGE
    if(activeTrack->useVoiceChange){
        ALOGV("using VoiceChange.");
        ComprehensiveVocoderStereo* VocoderStereo =
            (ComprehensiveVocoderStereo*)stereo;
        if (VocoderStereo == NULL) {
            ALOGD("%s: vocoder is null", __func__);
            VocoderStereo = new ComprehensiveVocoderStereo();
            if (VocoderStereo == NULL) {
                ALOGE("%s: failed to create vocoder", __func__);
                return false;
            }
            stereo = VocoderStereo;

            VocoderStereo->setFs(
                activeTrack->mRecordBufferConverter->getDstSampleRate());
            VocoderStereo->setChannel(
                activeTrack->mRecordBufferConverter->getDstChannelCount());
            VocoderStereo->setMode(mVoiceMode);
        }
        if(activeTrack->mSink.raw && framesOut*frameSize) {
            VocoderStereo->process((uint8_t*)activeTrack->mSink.raw,
                framesOut*frameSize);
        } else {
            ALOGE("%s: raw is %p, size is %lu", __func__, activeTrack->mSink.raw, framesOut*frameSize);
        }
    }

#ifdef MIAUDIO_MAGICVOICE
    ALOGV("YouMeMagicVoiceChanger activeTrack->useYouMeMagicVoice :%d",
        (int)activeTrack->useYouMeMagicVoice);
    YouMeMagicVoiceChanger* mYouMeMagicVoiceChanger =
        (YouMeMagicVoiceChanger*)voiceChanger;
    if(activeTrack->useYouMeMagicVoice) {
        if (mYouMeMagicVoiceChanger == nullptr) {
            ALOGD("%s: voice changer is null", __func__);
            YouMeMagicVoice_init();
            mYouMeMagicVoiceChanger = (YouMeMagicVoiceChanger*) engine_createMagicVoiceEngine();
            if (mYouMeMagicVoiceChanger == nullptr) {
                ALOGE("%s: failed to create magic voice changer", __func__);
                return false;
            }

            processMagicVoiceParameter(sampleRate, channelCount,
                mYouMeMagicVoiceChanger);
            voiceChanger = (void*)mYouMeMagicVoiceChanger;
        }

        if(mYouMeMagicVoiceChanger->getState()
                != YOUME_MAGICVOICE_STATE_STARTED) {
            processMagicVoiceParameter(sampleRate, channelCount,
                mYouMeMagicVoiceChanger);
        }
        int putSampleNum = mYouMeMagicVoiceChanger->putSamples(
            (int16_t*)((uint8_t*)activeTrack->mSink.raw),
                framesOut*frameSize/2, 16);

        uint32_t magicVoiceNumSamples = mYouMeMagicVoiceChanger->numSamples();
        if(magicVoiceNumSamples > 0 &&
                (uint32_t)magicVoiceNumSamples >= framesOut*frameSize/2){
            mYouMeMagicVoiceChanger->getSamples(
                (int16_t*)((uint8_t*)activeTrack->mSink.raw),
                    framesOut*frameSize/2);
        }else{
            return false;
        }
        ALOGV("YouMeMagicVoiceChanger-send need put : %u , sucess put sample num:%d",
            (int)(framesOut*frameSize/2), putSampleNum);
    }

#endif
#else
    UNUSED(activeTrack);
    UNUSED(framesOut);
    UNUSED(frameSize);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(voiceChanger);
    UNUSED(stereo);
#endif
    return true;
}

void AudioFlingerImpl::deleteVocoderStereo(void* &stereo) {
#ifdef MIAUDIO_VOICE_CHANGE
    ComprehensiveVocoderStereo* VocoderStereo =
        (ComprehensiveVocoderStereo*)stereo;
    if(VocoderStereo){
        delete VocoderStereo;
        VocoderStereo = NULL;
        stereo = NULL;
    }
#else
    UNUSED(stereo);
#endif
}

void AudioFlingerImpl::YouMeMagicVoice(void* &voiceChanger) {
#ifdef MIAUDIO_VOICE_CHANGE
    YouMeMagicVoiceChanger* mYouMeMagicVoiceChanger =
        (YouMeMagicVoiceChanger*)voiceChanger;
    if(mYouMeMagicVoiceChanger != nullptr){
        ALOGD("YouMeMagicVoiceChanger free");
        engine_releaseMagicVoiceEngine((void *)mYouMeMagicVoiceChanger);
        YouMeMagicVoice_deinit();
        mYouMeMagicVoiceChanger = nullptr;
        voiceChanger = mYouMeMagicVoiceChanger;
    }
#else
    UNUSED(voiceChanger);
#endif
}

bool AudioFlingerImpl::processMagicVoiceParameter(uint32_t sampleRate,
        uint32_t channelCount,
        YouMeMagicVoiceChanger* &mYouMeMagicVoiceChanger) {
#ifdef MIAUDIO_MAGICVOICE
    ALOGV("processMagicVoiceParameter");
    if(mYouMeMagicVoiceChanger == nullptr) {
        YouMeMagicVoice_init();
        mYouMeMagicVoiceChanger =
            (YouMeMagicVoiceChanger*) engine_createMagicVoiceEngine();
    }

    if(mYouMeMagicVoiceChanger != nullptr &&
        mYouMeMagicVoiceChanger->getState() == YOUME_MAGICVOICE_STATE_INITED) {
        ALOGD("YouMeMagicVoiceChanger set parameter in processMagicVoiceParameter");
        mYouMeMagicVoiceChanger->setSampleRate((int)sampleRate);
        mYouMeMagicVoiceChanger->setChannels((int)channelCount);
        mYouMeMagicVoiceChanger->setProcessUnitMS(140);
        mYouMeMagicVoiceChanger->setOverlapFactor(0.5);
        mYouMeMagicVoiceChanger->setOverlapSmoothMs(20);
        mYouMeMagicVoiceChanger->start();
    } else if(mYouMeMagicVoiceChanger!=nullptr &&
        mYouMeMagicVoiceChanger->getState() == YOUME_MAGICVOICE_STATE_STOPPED) {
        ALOGI("getState = %d", mYouMeMagicVoiceChanger->getState());
        mYouMeMagicVoiceChanger->start();
    }
    ALOGD("processMagicVoiceParameter end");
    return true;
#else
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(mYouMeMagicVoiceChanger);
    return false;
#endif
}

void AudioFlingerImpl::stopYouMeMagicVoice(void* &voiceChanger) {
#ifdef MIAUDIO_VOICE_CHANGE
    YouMeMagicVoiceChanger* mYouMeMagicVoiceChanger =
        (YouMeMagicVoiceChanger*)voiceChanger;
    if(mYouMeMagicVoiceChanger != nullptr &&
        YOUME_MAGICVOICE_STATE_STOPPED != mYouMeMagicVoiceChanger->getState()){
        ALOGD("YouMeMagicVoiceChanger  stop for go to mWaitWorkCV.wait");
        mYouMeMagicVoiceChanger->stop();
    }
#else
    UNUSED(voiceChanger);
#endif
}

status_t AudioFlingerImpl::checkFilePath(const char *filePath)
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

void AudioFlingerImpl::dumpAudioFlingerDataToFile(
                uint8_t *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                const char *name)
{
#ifdef MIAUDIO_DUMP_PCM
    static int audio_flinger_fd = -1;
    int32_t dump_switch = 0;
    dump_switch = property_get_int32("audio.dump.switch", 0);

    if (!(dump_switch & 0x2)) {
        if (audio_flinger_fd > 0) {
            ALOGE("%s: dump is not enabled, close output_fd %d",
                __func__, audio_flinger_fd);
            close(audio_flinger_fd);
            audio_flinger_fd = -1;
        }
        return;
    }

    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s_%s_fmt_%#x_sr_%u_ch_%u.pcm",
                 DEFAULT_FLINGER_DUMP_DIR, name, format, sampleRate, channelCount);

    umask(0000);
    if (audio_flinger_fd == -1) {
        int ret = checkFilePath(filePath);
        if (ret < 0) {
            ALOGE("dump fail!!!");
        } else {
            audio_flinger_fd = open(filePath, O_CREAT|O_RDWR|O_APPEND, 0777);
        }
    }

    if (audio_flinger_fd != -1) {
        write(audio_flinger_fd, buffer, size);
    }
#else
    UNUSED(buffer);
    UNUSED(size);
    UNUSED(format);
    UNUSED(sampleRate);
    UNUSED(channelCount);
    UNUSED(name);
#endif
}

#ifdef MIAUDIO_SPITAL_AUDIO
static const int MAX_DEVICE_SIZE = 2;
static const int MAX_DEVICE_NAME_SIZE = 32;
static const int spatialDevice[MAX_DEVICE_SIZE][MAX_DEVICE_NAME_SIZE] = {
    {0x58,0x69,0x61,0x6f,0x6d,0x69,0x20,0x42,0x75,0x64,0x73,0x20,0x33,0x20,0x50,0x72,0x6f},
    {0x58,0x69,0x61,0x6f,0x6d,0x69,0x20,0x42,0x75,0x64,0x73,0x20,0x33,0x54,0x20,0x50,0x72,0x6f}
};
#endif

String8 AudioFlingerImpl::getSpatialDevice(const String8& keys) const
{
#ifdef MIAUDIO_SPITAL_AUDIO
    if(keys.contains("spatial_device_")){
        const char* deviceStr = keys.c_str();
        bool result = false;
        for (size_t i = 0; i < MAX_DEVICE_SIZE; i++){
            for (size_t j = 15; j < keys.size(); j++){
                result = (spatialDevice[i][j - 15] == deviceStr[j]);
                if (!result){
                    break;
                }
            }
            if (result){
                return String8(keys+"=true");
            }
        }
        return String8(keys+"=false");
    }else{
        return String8("");
    }
#else
    UNUSED(keys);
    return String8("");
#endif
}

bool AudioFlingerImpl::shouldMutedProcess(uid_t uid) {
#ifdef MIPERF_PRELOAD_MUTE
    android::RWLock::AutoRLock _l(mPreloadRWLock);
    return mPreloadUids.find(uid) != mPreloadUids.end();
#endif
    return false;
}

bool AudioFlingerImpl::checkAndUpdatePreloadProcessVolume(const String8& keyValuePairs) {
#ifdef MIPERF_PRELOAD_MUTE
    AudioParameter param = AudioParameter(keyValuePairs);
    String8 value;
    if (param.get(String8("mute_preload_uid"), value) == NO_ERROR) {
        android::RWLock::AutoWLock _l(mPreloadRWLock);
        uid_t uid = atoi(value.c_str());
        mPreloadUids.insert(uid);
        ALOGW("mute preloadApp uid: %u",uid);
        return true;
    } else if (param.get(String8("remove_mute_preload_uid"), value) == NO_ERROR) {
        android::RWLock::AutoWLock _l(mPreloadRWLock);
        uid_t uid = atoi(value.c_str());
        if (uid == -1) {
            mPreloadUids.clear();
        } else if (mPreloadUids.find(uid) != mPreloadUids.end()) {
            mPreloadUids.erase(uid);
        }
        ALOGW("remove mute preloadApp uid: %u",uid);
        return true;
    }
#endif
    return false;
}

extern "C" IAudioFlingerStub* create() {
    return new AudioFlingerImpl;
}

extern "C" void destroy(IAudioFlingerStub* impl) {
    delete impl;
}

}//namespace android
