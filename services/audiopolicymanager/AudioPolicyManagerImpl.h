#ifndef ANDROID_AUDIOPOLICYMANAGER_IMPL_H
#define ANDROID_AUDIOPOLICYMANAGER_IMPL_H

#include <log/log.h>
#include <utils/Errors.h>
#include <IAudioPolicyManagerStub.h>

namespace android {

using namespace audio_policy;

class AudioPolicyManagerImpl : public IAudioPolicyManagerStub {
public:
    virtual ~AudioPolicyManagerImpl() {}

// MIAUDIO_NOTIFICATION_FILTER begin
public:
    virtual void notificationFilterGetDevicesForStrategyInt(
        legacy_strategy strategy, DeviceVector &devices,
        const SwAudioOutputCollection &outputs,
        DeviceVector availableOutputDevices, const Engine * engine);
    virtual void setNotificationFilter(audio_stream_type_t stream);

private:
    DeviceVector notificationFilterRemoveSpeakerDevice(DeviceVector device,
        audio_stream_type_t stream, DeviceVector availableOutputDevices);
//MIAUDIO_NOTIFICATION_FILTER end

// MIAUDIO_MULTI_ROUTE begin
public:
    virtual bool multiRouteSetForceSkipDevice(audio_skip_device_t skipDevive,
        Engine* engine);
    virtual status_t multiRouteGetOutputForAttrInt(
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
                        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes);
    virtual status_t multiRouteSetProParameters(
                        const String8& keyValuePairs,
                        AudioPolicyManager* apm,
                        EngineInstance& engine,
                        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes);
// MIAUDIO_MULTI_ROUTE end

    virtual int isSmallGame(String8 ClientName);

    virtual void set_special_region();

    virtual bool get_special_region();

    virtual bool soundTransmitEnable();

// MIAUDIO_DUMP_MIXER begin
    virtual void dumpAudioMixer(int fd);
    virtual void setDumpMixerEnable();
// MIAUDIO_DUMP_MIXER end

    virtual void audioDynamicLogInit();

//MIAUDIO_EFFECT_SWITCH begin
    virtual void initOldVolumeCurveSupport();
    virtual bool isOldVolumeCurveSupport();
    virtual void setCurrSpeakerMusicStreamVolumeIndex(int index);
//DMIAUDIO_EFFECT_SWITCH end
    virtual void reInitStreamVolSteps(AudioPolicyManager* apm);
private:
    String16 getPackageName(uid_t uid);
    bool	special_region = false;

//MIAUDIO_EFFECT_SWITCH begin
    bool oldVolumeCurveSupport = false;
    bool useOldVolumeCurve = false;
    int mCurrentMusicSpeakerIndex;
//DMIAUDIO_EFFECT_SWITCH end

};

extern "C"  IAudioPolicyManagerStub* create();
extern "C" void destroy(IAudioPolicyManagerStub* impl);

}//namespace android

#endif
