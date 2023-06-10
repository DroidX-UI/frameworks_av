#define LOG_TAG "AudioPolicyManagerImpl"
#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <utils/Trace.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <system/audio-hal-enums.h>
#include <binder/PermissionController.h>
#include <binder/IPCThreadState.h>

#include "AudioPolicyManagerImpl.h"
#include <EngineBase.h>
#include "send_data_to_xlog.h"
#include "CloudCtrlFwk.h"

namespace android {
using namespace audio_policy;

String16 AudioPolicyManagerImpl::getPackageName(uid_t uid)
{
    String16 mClientName;
    if (uid == 0)
        uid = IPCThreadState::self()->getCallingUid();
    else if (uid == 1000)
        return String16("systemApp");

    PermissionController permissionController;
    Vector<String16> packages;
    permissionController.getPackagesForUid(uid, packages);
    if (packages.isEmpty()) {
        ALOGV("No packages for calling UID");
    } else {
        mClientName = packages[0];
        ALOGV("getPackageName for %s  uid %d", String8(mClientName).string(),
            uid);
    }
    return mClientName;
}


//MIAUDIO_NOTIFICATION_FILTER begin

#define BT_DEVICES_ALL_MASK \
    (AUDIO_DEVICE_OUT_BLUETOOTH_A2DP |\
    AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES |\
    AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER) |\
    AUDIO_DEVICE_OUT_BLUETOOTH_SCO |\
    AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET |\
    AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT
#define HEADPHONES_DEVICES_ALL_MASK \
    (AUDIO_DEVICE_OUT_WIRED_HEADSET |\
    AUDIO_DEVICE_OUT_WIRED_HEADPHONE | AUDIO_DEVICE_OUT_USB_DEVICE |\
    AUDIO_DEVICE_OUT_LINE | AUDIO_DEVICE_OUT_USB_HEADSET)
#define MUTENOTIFICATIONMASK (0x1)
#define MUTERINGMASK (0x2)

//notification filter start
static bool notificationFilterFeature = false;
static uint32_t muteMaskInt = 0;
static audio_stream_type_t curStream;
//notification filter end

DeviceVector AudioPolicyManagerImpl::notificationFilterRemoveSpeakerDevice(
        DeviceVector device, audio_stream_type_t stream,
        DeviceVector availableOutputDevices) {
    bool muteNotification = false;
    bool muteRing = false;
    DeviceVector devices = device;

    if (MUTENOTIFICATIONMASK & muteMaskInt)
        muteNotification = true;
    if (MUTERINGMASK & muteMaskInt)
        muteRing = true;

    ALOGV("notification filter: %s muteMaskInt=%x muteRing=%d, muteNotification=%d",
                __func__, muteMaskInt, muteRing, muteNotification);
    //remove the ring from spk
    if (muteRing) {
        if (stream == AUDIO_STREAM_RING) {
            devices.remove(availableOutputDevices.getDevicesFromTypes(
                {AUDIO_DEVICE_OUT_SPEAKER, AUDIO_DEVICE_OUT_SPEAKER_SAFE}));
            ALOGV("notification filter: %s muteRing", __func__);
        }
    }

    //remove the notification from spk
    if (muteNotification) {
        if (stream == AUDIO_STREAM_NOTIFICATION) {
            devices.remove(availableOutputDevices.getDevicesFromTypes(
                {AUDIO_DEVICE_OUT_SPEAKER, AUDIO_DEVICE_OUT_SPEAKER_SAFE}));
            ALOGV("notification filter: %s muteNotification", __func__);
        }
    }
    return devices;
}

void AudioPolicyManagerImpl::notificationFilterGetDevicesForStrategyInt(
        legacy_strategy strategy, DeviceVector &devices,
        const SwAudioOutputCollection &outputs,
        DeviceVector availableOutputDevices, const Engine * engine) {
#ifdef MIAUDIO_NOTIFICATION_FILTER
    if (engine == nullptr)
        return;
    /**
      *STRATEGY_SONIFICATION_RESPECTFUL cannot out with spk when headset connected
      *strategy is certain, stream is uncertain */
    if (notificationFilterFeature &&
        (muteMaskInt > 0) && strategy == STRATEGY_SONIFICATION &&
        (curStream == AUDIO_STREAM_RING || curStream == AUDIO_STREAM_ALARM ||
            curStream == AUDIO_STREAM_NOTIFICATION)) {
        bool haveMoreDevices = false;
        audio_devices_t deviceBits = deviceTypesToBitMask(devices.types());
        if ((deviceBits & BT_DEVICES_ALL_MASK ||
            deviceBits & HEADPHONES_DEVICES_ALL_MASK) &&
            (deviceBits &
                (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_SPEAKER_SAFE))) {
            haveMoreDevices = true;
        } else {
            haveMoreDevices = false;
        }

        if (haveMoreDevices) {
            //The alarm is ringing, the one behind cannot be muted
            if ((curStream == AUDIO_STREAM_RING ||
                curStream == AUDIO_STREAM_NOTIFICATION) &&
                outputs.isActiveLocally(
                    engine->toVolumeSource(AUDIO_STREAM_ALARM))) {
                ALOGV("%s notification filter do not remove Speaker Device",
                    __func__);
            //No matter what is in front, itis an alarm clock now,
            //do not remove the device
            } else if (curStream == AUDIO_STREAM_ALARM) {
                ALOGV("%s notification filter do not remove Speaker Device",
                __func__);
            //mute spk
            } else if ((curStream == AUDIO_STREAM_RING ||
                curStream == AUDIO_STREAM_NOTIFICATION)) {
                ALOGD("%s notification filter removeSpeakerDevice", __func__);
                devices = notificationFilterRemoveSpeakerDevice(devices,
                    curStream, availableOutputDevices);
            } else {
                ALOGV("%s notification filter do nothing", __func__);
            }
        }

    }
#else
    UNUSED(strategy);
    UNUSED(devices);
    UNUSED(outputs);
    UNUSED(availableOutputDevices);
    UNUSED(engine);
#endif
    return;
}

void AudioPolicyManagerImpl::setNotificationFilter(audio_stream_type_t stream)
{
#ifdef MIAUDIO_NOTIFICATION_FILTER
    bool isFilter = property_get_bool("ro.vendor.audio.ring.filter", false);
    if (isFilter) {
        char mask[PROPERTY_VALUE_MAX] = "";
        uint32_t maskInt = 0;
        ALOGV("notification filter: %s notificationFilterFeature=%x",
            __func__, isFilter);
        property_get("persist.vendor.audio.ring.filter.mask", mask, "0");
        maskInt = atoi(mask);
        ALOGD("notification filter: %s muteMaskInt=%x", __func__, maskInt);
        notificationFilterFeature = isFilter;
        muteMaskInt = maskInt;
        curStream = stream;
    }
#else
    UNUSED(stream);
#endif
    return;
}
//MIAUDIO_NOTIFICATION_FILTER end

  /**
    * skip to select this device if it has been force
    */
bool AudioPolicyManagerImpl::multiRouteSetForceSkipDevice(
        audio_skip_device_t skipDevive, Engine* engine) {
#ifdef MIAUDIO_MULTI_ROUTE
    ALOGV("%s set force skip device %d", __func__, skipDevive);

    if (engine == nullptr)
        return false;

    if (skipDevive == AUDIO_SKIP_DEVIVE_PROXY) {
        engine->mForceSkipProxy = true;
    } else if (skipDevive == AUDIO_SKIP_DEVIVE_REMOTE_SUBMIX) {
        engine->mForceSkipRemoteSubmix = true;
    } else if (skipDevive == AUDIO_SKIP_DEVIVE_A2DP) {
        engine->mForceSkipA2DP = true;
    } else if (skipDevive == AUDIO_SKIP_DEVIVE_DEFAULT) {
        engine->mForceSkipProxy = false;
        engine->mForceSkipRemoteSubmix = false;
        engine->mForceSkipA2DP = false;
    }
#else
    UNUSED(skipDevive);
    UNUSED(engine);
#endif
    return true;
}

status_t AudioPolicyManagerImpl::multiRouteGetOutputForAttrInt(
        audio_attributes_t *resultAttr,
        audio_io_handle_t *output,
        DeviceVector&  availableOutputDevices,
        audio_output_flags_t *flags,
        audio_port_handle_t *selectedDeviceId,
        audio_session_t session,
        audio_stream_type_t *stream,
        const audio_config_t *config,
        uid_t uid,
        AudioPolicyInterface::output_type_t *outputType,
        DeviceVector& outputDevices,
        AudioPolicyManager* apm,
        EngineInstance& engine,
        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes) {
#ifdef MIAUDIO_MULTI_ROUTE
    // except app output select
    for (size_t i =0 ; i < multiRoutes.size() ; i++) {
         if (uid == multiRoutes.keyAt(i)) {
             bool isSpatialized = false;
             ALOGD("%s small window projection excepted uid = %d,package name = %s",
                __func__, uid, String8(getPackageName(uid)).string());
             audio_devices_t devices = (audio_devices_t) multiRoutes.valueAt(0);
             outputDevices = availableOutputDevices.getDevicesFromType(devices);
             // policy devices need to be removed to match correct output
             ALOGD("%s avaliable devices %s", __func__, outputDevices.toString().c_str());
             DeviceVector tmpDevices = outputDevices;
             for (const auto &device : tmpDevices){
                 if (device->address() != "0"){
                     outputDevices.remove(device);
                 }
             }
             ALOGW("%s filtered devices %s", __func__, outputDevices.toString().c_str());

             *output = AUDIO_IO_HANDLE_NONE;
             *flags = AUDIO_OUTPUT_FLAG_VIRTUAL_DEEP_BUFFER;
             ALOGD("multiRouteGetOutputForAttrInt attr->flags %x, change to non-spatializer", resultAttr->flags);
             resultAttr->flags = AUDIO_FLAG_NEVER_SPATIALIZE;
             *output = apm->getOutputForDevices(outputDevices, session, resultAttr,
                config, flags, &isSpatialized, resultAttr->flags & AUDIO_FLAG_MUTE_HAPTIC);
             if (*output == AUDIO_IO_HANDLE_NONE) {
                 return INVALID_OPERATION;
             }
             *selectedDeviceId = apm->getFirstDeviceId(outputDevices);

             if (outputDevices.onlyContainsDevicesWithType(
                    AUDIO_DEVICE_OUT_TELEPHONY_TX)) {
                 *outputType = AudioPolicyInterface::API_OUTPUT_TELEPHONY_TX;
             } else {
                 *outputType = AudioPolicyInterface::API_OUTPUT_LEGACY;
             }
            if (devices == AUDIO_DEVICE_OUT_PROXY)
                engine->setForceSkipDevice(AUDIO_SKIP_DEVIVE_PROXY);
            else if(devices == AUDIO_DEVICE_OUT_REMOTE_SUBMIX)
                engine->setForceSkipDevice(AUDIO_SKIP_DEVIVE_REMOTE_SUBMIX);

             return NO_ERROR;
         }
    }
    // explicit routing managed by getDeviceForStrategy in APM is now handled by engine
    // in order to let the choice of the order to future vendor engine
    if (multiRoutes.size() && uid != multiRoutes.keyAt(0)) {
        audio_skip_device_t skipDevive;
        if (engine == nullptr)
            return INVALID_OPERATION;
        if ((audio_devices_t) multiRoutes.valueAt(0) == AUDIO_DEVICE_OUT_PROXY) {
            skipDevive = AUDIO_SKIP_DEVIVE_PROXY;
            if (engine->setForceSkipDevice(skipDevive) != true) {
                 return INVALID_OPERATION;
            }
        } else if ((audio_devices_t) multiRoutes.valueAt(0) ==
                AUDIO_DEVICE_OUT_REMOTE_SUBMIX) {
            skipDevive = AUDIO_SKIP_DEVIVE_REMOTE_SUBMIX;
            if (engine->setForceSkipDevice(skipDevive) != true) {
                 return INVALID_OPERATION;
            }
        } else if ((audio_devices_t) multiRoutes.valueAt(0) ==
                AUDIO_DEVICE_OUT_BLUETOOTH_A2DP) {
            skipDevive = AUDIO_SKIP_DEVIVE_A2DP;
            if (engine->setForceSkipDevice(skipDevive) != true) {
                 return INVALID_OPERATION;
            }
        }
        ALOGD("%s flags %#x outputDevices.toString().c_str() = %s", __func__,
            *flags,outputDevices.toString().c_str());
    }
#else
    UNUSED(resultAttr);
    UNUSED(output);
    UNUSED(availableOutputDevices);
    UNUSED(resultAttr);
    UNUSED(output);
    UNUSED(availableOutputDevices);
    UNUSED(flags);
    UNUSED(selectedDeviceId);
    UNUSED(session);
    UNUSED(stream);
    UNUSED(config);
    UNUSED(uid);
    UNUSED(outputType);
    UNUSED(outputDevices);
    UNUSED(apm);
    UNUSED(engine);
    UNUSED(multiRoutes);
#endif
    return NOT_MULTI_ROUTES_UID;
}

status_t AudioPolicyManagerImpl::multiRouteSetProParameters(
        const String8& keyValuePairs,
        AudioPolicyManager* apm,
        EngineInstance& engine,
        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes) {
#ifdef MIAUDIO_MULTI_ROUTE
    ALOGD("%s keyvalue %s, calling pid %d calling uid %d", __func__,
               keyValuePairs.string(),
               IPCThreadState::self()->getCallingPid(),
               IPCThreadState::self()->getCallingUid());
    String8 filteredKeyValuePairs = keyValuePairs;
    String8 MultirouteState = String8("");

    ALOGD("%s: filtered keyvalue %s", __func__, filteredKeyValuePairs.string());
    if (engine == nullptr || apm == nullptr)
        return BAD_VALUE;

    AudioParameter param = AudioParameter(filteredKeyValuePairs);
    int value1, value2;
    String8 exceptDevice, exceptUid;

    if (param.get(String8(AudioParameter::keyAudioRouteCast), MultirouteState) >= 0){
     if (param.get(String8("uid"), exceptUid) == NO_ERROR &&
           param.get(String8("device"), exceptDevice) == NO_ERROR &&
           MultirouteState == AudioParameter::valueOn) {
        if (strcmp(exceptDevice,"proxy") == 0) {
            value2 = (int) AUDIO_DEVICE_OUT_PROXY;
        } else if (strcmp(exceptDevice,"remote_submix") == 0) {
            value2 = (int) AUDIO_DEVICE_OUT_REMOTE_SUBMIX;
        } else if (strcmp(exceptDevice,"a2dp") == 0) {
            value2 = (int) AUDIO_DEVICE_OUT_BLUETOOTH_A2DP;
        }

        const char *Context_uid = exceptUid.string();
        char *s = new char[strlen(Context_uid) + 1];
        strcpy(s,Context_uid);
        char* singleUid = strtok(s, ",");
        while (singleUid != NULL) {
            value1 = atoi(singleUid);
            multiRoutes.add(value1, value2);
            singleUid = strtok(NULL, ",");
        }

        const char *address = "";
        status_t status = apm->setDeviceConnectionStateInt(
                        AUDIO_DEVICE_OUT_MULTIROUTE,
                        AUDIO_POLICY_DEVICE_STATE_AVAILABLE,
                        address, "multi-route", AUDIO_FORMAT_DEFAULT);
        //for onetrack rbis
        send_projection_screen_to_xlog();
        return status;
      } else if (MultirouteState == AudioParameter::valueOff){
        const char *address = "";
        status_t status = apm->setDeviceConnectionState(
            AUDIO_DEVICE_OUT_MULTIROUTE, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE,
            address, "multi-route", AUDIO_FORMAT_DEFAULT);
        /* clear DefaultKeyedVector mMultiRoutes */
        if (multiRoutes.size() > 0)
            multiRoutes.clear();

        if (engine->setForceSkipDevice(AUDIO_SKIP_DEVIVE_DEFAULT) != true)
            ALOGW("setForceSkipDevice() invalid and skipDevive %d",
                AUDIO_SKIP_DEVIVE_DEFAULT);

        return status;
     }
   }
#endif

#ifdef MIAUDIO_EFFECT_SWITCH
    String8 olderVolumeCurve = String8("");
   if (param.get(String8("old_volume_curve"), olderVolumeCurve) >= 0) {
        if (olderVolumeCurve == String8("true")) {
            useOldVolumeCurve = true;
            property_set("persist.vendor.audio.volumecurve.old","true");
            ALOGD("set old_volume_curve to true");
        } else {
            useOldVolumeCurve = false;
            property_set("persist.vendor.audio.volumecurve.old","false");
            ALOGD("set old_volume_curve to false");
        }
        apm->setStreamVolumeIndex(AUDIO_STREAM_MUSIC, mCurrentMusicSpeakerIndex, AUDIO_DEVICE_OUT_SPEAKER);
        return NO_ERROR;
   }
#endif

 #if !defined(MIAUDIO_MULTI_ROUTE) && !defined(MIAUDIO_EFFECT_SWITCH)
    UNUSED(keyValuePairs);
    UNUSED(apm);
    UNUSED(engine);
    UNUSED(multiRoutes);
#endif
    return BAD_VALUE;
}

int AudioPolicyManagerImpl::isSmallGame(String8 ClientName) {
    return !String8("com.bubble.free.bubblestory").compare(ClientName) ||
           !String8("com.amanotes.beathopper").compare(ClientName) ||
           !String8("com.king.candycrushsaga").compare(ClientName) ||
           !String8("com.fingersoft.hillclimb").compare(ClientName) ||
           !String8("com.miniclip.eightballpool").compare(ClientName) ||
           !String8("com.wb.goog.mkx").compare(ClientName);
}

void AudioPolicyManagerImpl::set_special_region() {

    //distinguish if the region is customized
    //for example kr_skt,kr_kt,kr_lgu and fr_orange

    char value[PROPERTY_VALUE_MAX] = {0};

    if(property_get("ro.miui.customized.region", value, NULL)) {
        if(!String16("kr_skt").compare(String16(value)) ||
            !String16("kr_kt").compare(String16(value)) ||
            !String16("kr_lgu").compare(String16(value)) ||
            !String16("fr_orange").compare(String16(value)))
            special_region = true;
    }
}

bool AudioPolicyManagerImpl::get_special_region() {
    return special_region;
}

bool AudioPolicyManagerImpl::soundTransmitEnable() {
    bool soundTransmitOn = false;

#ifdef MIAUDIO_SOUND_TRANSMIT
    String8 reply;
    String8 value;

    reply = AudioSystem::getParameters(String8("sound_transmit_enable"));
    ALOGE("%s: soundTransmitEnable %s", __FUNCTION__, reply.string());
    AudioParameter repliedParameters(reply);

    if (repliedParameters.get(String8("sound_transmit_enable"), value) ==
            NO_ERROR) {
        soundTransmitOn = value.contains("1") || value.contains("6");
    }
#endif //MIAUDIO_SOUND_TRANSMIT

    return soundTransmitOn;
}

static bool dumpMixer = false;

void AudioPolicyManagerImpl::dumpAudioMixer(int fd) {
#ifdef MIAUDIO_DUMP_MIXER
    if (dumpMixer) {
        String8 reply, replyCut;
        char str[16] = "";
        String8 result;
        int maxCount = 0;
        int headerCount = 0;

        replyCut.appendFormat("\naudioMixerDump:\n");
        write(fd, replyCut, replyCut.size());

        result = AudioSystem::getParameters(AUDIO_IO_HANDLE_NONE, String8("mixer_max"));
        headerCount = String8("mixer_max=").length();
        maxCount = std::atoi((std::string(result.string()).substr(headerCount, result.length() - headerCount)).c_str());

        for(int i = 0; i < maxCount; i++) {
            memset(str, '\0', 16);
            sprintf(str, "mixerid:%d", i);
            reply = AudioSystem::getParameters(AUDIO_IO_HANDLE_NONE, String8(str));
            replyCut = String8(std::string(reply.string()).substr(1 /*= length*/ , reply.length() - 1/*= length*/).c_str());
            write(fd, replyCut, replyCut.size());
        }
    }
#else
    UNUSED(fd);
#endif
}

void AudioPolicyManagerImpl::setDumpMixerEnable() {
#ifdef MIAUDIO_DUMP_MIXER
    if (property_get_bool("ro.vendor.audio.dump.mixer", false))
        dumpMixer = true;
#else
    dumpMixer = false;
#endif
}


//dynamic log start

typedef struct audioDynamicLogNode {
    const char* fileTag;
    const char* logLevel;
} audioDynamicLogNode_t;

//if you want to enable file "ToneGenerator.cpp" logv, using below cmd:
//"adb shell setprop log.tag.ToneGenerator V"
//disable with "adb shell setprop log.tag.ToneGenerator D"
static audioDynamicLogNode_t LogNodes[] = {
    /* LOG_TAG  LOGLEVEL*/
    {"AudioAttributes", "D"},                           //AudioAttributes.cpp
    {"AudioEffect", "D"},                               //AudioEffect.cpp
    {"AudioPolicy", "D"},                               //AudioPolicy.cpp
    {"AudioProductStrategy", "D"},                      //AudioProductStrategy.cpp
    {"AudioRecord", "D"},                               //AudioRecord.cpp
    {"AudioSystem", "D"},                               //AudioSystem.cpp
    {"AudioTrack", "D"},                                //AudioTrack.cpp
    {"AudioTrackShared", "D"},                          //AudioTrackShared.cpp
    {"AudioVolumeGroup", "D"},                          //AudioVolumeGroup.cpp
    {"IAudioFlinger", "D"},                             //IAudioFlinger.cpp
    {"ToneGenerator", "D"},                             //ToneGenerator.cpp


    {"AudioFlinger", "D"},                              //AudioFlinger.cpp\Threads.cpp\AudioStreamOut.cpp\Effects.cpp
    {"AudioHwDevice", "D"},                             //AudioHwDevice.cpp
    {"FastCapture", "D"},                               //FastCapture.cpp
    {"FastMixer", "D"},                                 //FastMixer.cpp
    {"FastThread", "D"},                                //FastThread.cpp

    {"AudioFlinger::EffectBase", "D"},                  //Effects.cpp
    {"AudioFlinger::EffectModule", "D"},
    {"AudioFlinger::EffectHandle", "D"},
    {"AudioFlinger::EffectChain", "D"},
    {"AudioFlinger::DeviceEffectProxy", "D"},
    {"AudioFlinger::DeviceEffectProxy::ProxyCallback", "D"},

    {"AF::TrackBase", "D"},                             //Tracks.cpp
    {"AF::TrackHandle", "D"},
    {"AF::Track", "D"},
    {"AF::OutputTrack", "D"},
    {"AF::PatchTrack", "D"},
    {"AF::OpRecordAudioMonitor", "D"},
    {"AF::RecordHandle", "D"},
    {"FastMixerState", "D"},                            //FastMixerState.cpp

    {"APM::AudioPolicyEngine", "D"},                    //Engine.cpp
    {"AudioPolicyEffects", "D"},                        //AudioPolicyEffects.cpp
    {"AudioPolicyIntefaceImpl", "D"},                   //AudioPolicyInterfaceImpl.cpp
    {"AudioPolicyService", "D"},                        //AudioPolicyService.cpp
    {"APM::AudioPolicyEngine::Config", "D"},            //EngineConfig.cpp
    {"APM::AudioPolicyEngine::VolumeGroup", "D"},       //VolumeGroup.cpp
    {"APM::AudioPolicyEngine::ProductStrategy", "D"},   //ProductStrategy.cpp
    {"APM::AudioPolicyEngine::Base", "D"},              //EngineBase.cpp
    {"APM::VolumeCurve", "D"},                          //VolumeCurve.cpp
    {"APM::Serializer", "D"},                           //Serializer.cpp
    {"APM::Devices", "D"},                              //DeviceDescriptor.cpp
    {"APM::AudioInputDescriptor", "D"},                 //AudioInputDescriptor.cpp
    {"APM::IOProfile", "D"},                            //IOProfile.cpp
    {"APM_ClientDescriptor", "D"},                      //ClientDescriptor.cpp
    {"APM::AudioCollections", "D"},                     //AudioCollections.cpp
    {"APM::AudioOutputDescriptor", "D"},                //AudioOutputDescriptor.cpp
};

void AudioPolicyManagerImpl::audioDynamicLogInit()
{
    char logTag[128] = "";

    for (int i = 0; i < sizeof(LogNodes)/sizeof(audioDynamicLogNode_t); i++) {
        sprintf(logTag, "log.tag.%s", LogNodes[i].fileTag);
        property_set(logTag, LogNodes[i].logLevel);
        memset(logTag, '\0', 128);
    }
}
//dynamic log end

//MIAUDIO_EFFECT_SWITCH start
void AudioPolicyManagerImpl::initOldVolumeCurveSupport(){
#ifdef MIAUDIO_EFFECT_SWITCH
    oldVolumeCurveSupport = property_get_bool("ro.vendor.audio.volumecurve.old",false);
    useOldVolumeCurve = property_get_bool("persist.vendor.audio.volumecurve.old",false);
    ALOGD("%s() oldVolumeCurveSupport=%d,useOldVolumeCurve=%d",__func__,oldVolumeCurveSupport,useOldVolumeCurve);
#endif
}

bool AudioPolicyManagerImpl::isOldVolumeCurveSupport(){
#ifdef MIAUDIO_EFFECT_SWITCH
    return oldVolumeCurveSupport && useOldVolumeCurve;
#endif
    return false;
}

void AudioPolicyManagerImpl::setCurrSpeakerMusicStreamVolumeIndex(int index){
#ifdef MIAUDIO_EFFECT_SWITCH
    if(oldVolumeCurveSupport)
        mCurrentMusicSpeakerIndex = index;
#else
    UNUSED(index);
#endif
}

//MIAUDIO_EFFECT_SWITCH end

void AudioPolicyManagerImpl::reInitStreamVolSteps(AudioPolicyManager* apm) {
    if (apm == nullptr)
        return;

    int32_t voice_max_index = property_get_int32("ro.config.vc_call_vol_steps", -1);
    int32_t media_max_index = property_get_int32("ro.config.media_vol_steps", -1);
    ALOGD ("%s(): initStreamVolume, voice_max_index=%d, media_max_index=%d",
                __func__, voice_max_index, media_max_index);

    if (voice_max_index != -1)
        apm->initStreamVolume(AUDIO_STREAM_VOICE_CALL, 1, voice_max_index);
    if (media_max_index != -1)
        apm->initStreamVolume(AUDIO_STREAM_MUSIC, 0, media_max_index);
}

extern "C"  IAudioPolicyManagerStub*  create() {
    return new AudioPolicyManagerImpl();
}

extern "C" void destroy(IAudioPolicyManagerStub* impl) {
    delete impl;
}

}//namespace android
