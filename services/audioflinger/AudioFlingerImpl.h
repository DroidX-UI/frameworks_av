#ifndef ANDROID_AUDIOFLINGER_IMPL_H
#define ANDROID_AUDIOFLINGER_IMPL_H

#include <log/log.h>
#include <utils/Errors.h>

#include <IAudioFlingerStub.h>
#include "../Thread.h"
#include "voicechange/ComprehensiveVocoderStereo.h"
#include "YouMeMagicVoice/YouMeMagicVoice_utils.h"
#include "YouMeMagicVoice/YouMeMagicVoiceEngine.h"
#include <set>
#include "utils/RWLock.h"

namespace android {

//#ifdef MIAUDIO_GAME_4D_VIBRATE
typedef enum {
    TYPE_VIBRATOR_WEAK,
    TYPE_VIBRATOR_NORMAL,
    TYPE_VIBRATOR_STRONG,
    TYPE_VIBRATOR_NUM,
} vibrator_type_t;
//#endif // MIAUDIO_GAME_4D_VIBRATE

class AudioFlingerImpl : public IAudioFlingerStub {
public:
    virtual ~AudioFlingerImpl() {}
   // MIAUDIO_GAME_4D_VIBRATE begin
    virtual int gameEffectInitVibratorMsgHandler(const String8& keyValuePairs);

private:
    // --- Vibrator Message Handler ---
    class VibratorMsgHandler : public Thread {
    public:
        VibratorMsgHandler();
        virtual ~VibratorMsgHandler();

        // Thread virtuals
        virtual     void        onFirstRef();
        virtual     bool        threadLoop();
        void        exit();
        void sendCommand(vibrator_type_t type);

    private:
        // descriptor for virbrator event
        class VibratorCommand: public RefBase {

        public:
            VibratorCommand()
            : mVibratorType(TYPE_VIBRATOR_NORMAL) {}

            // TYPE_VIBRATOR_STRONG, TYPE_VIBRATOR_NORMAL, TYPE_VIBRATOR_WEAK.
            vibrator_type_t mVibratorType;
        };

        Mutex   mLock;
        Condition mWaitWorkCV;
        Vector < sp<VibratorCommand> > mVibratorCommands;
    };

    sp<VibratorMsgHandler> mVibratorMsgHandler = NULL;
    // MIAUDIO_GAME_4D_VIBRATE end

    // MIAUDIO_GLOBAL_AUDIO_EFFECT begin
public:
    virtual void audioEffectSetStreamVolume(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, float value);
    virtual void audioEffectSetStandby(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains);
    virtual void audioEffectSetAppName(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, bool active);
private:
    status_t setStreamVolume(const sp<AudioFlinger::EffectModule>& effect,
        audio_stream_type_t stream, float value);
    status_t setStandby(const sp<AudioFlinger::EffectModule>& effect);
    status_t setAppName(const sp<AudioFlinger::EffectModule>& effect,
        audio_stream_type_t stream, const char *name, int size);
    status_t getAppNameByPid(int pid, char *pName, int size);
    // MIAUDIO_GLOBAL_AUDIO_EFFECT end

    // MIAUDIO_VOICE_CHANGE begin
public:
    //game voice change
    virtual void setGameParameters(const String8& keyValuePairs,
        const DefaultKeyedVector< audio_io_handle_t, sp<AudioFlinger::RecordThread> >& RecordThreads);
    virtual void getGameMode(String8& keyValuePair);
private:
    String8 voiceMode;
    String8 gameModeOpen;
    String8 package_name;
    int gameUid = 0;
    int mLastVoiceMode = -1;
    String8 mVoiceMode_str;

public:
    virtual void checkForGameParameter_l(const String8& keyValuePair,
        const SortedVector<sp<AudioFlinger::RecordThread::RecordTrack>>& Tracks,
        uint32_t sampleRate, uint32_t channelCount, void* &voiceChanger,
        void* &stereo);
    virtual bool voiceProcess(
        const sp<AudioFlinger::RecordThread::RecordTrack>& activeTrack,
        size_t framesOut, size_t frameSize, uint32_t sampleRate,
        uint32_t channelCount, void* &voiceChanger, void* &stereo);
    virtual void deleteVocoderStereo(void* &stereo);

    virtual void YouMeMagicVoice(void* &voiceChanger);
    virtual void stopYouMeMagicVoice(void* &voiceChanger);
private:
    bool mOpenGameMode = false;
    int mVoiceMode = -1;
    int mGameUid = -1;
    String8 mGameName;
    ComprehensiveVocoderStereo* VocoderStereo = NULL;

    bool processMagicVoiceParameter(uint32_t sampleRate, uint32_t channelCount,
        YouMeMagicVoiceChanger* &mYouMeMagicVoiceChanger);
    void setGameMode(
        const SortedVector<sp<AudioFlinger::RecordThread::RecordTrack>>& Tracks,
        int version, uint32_t sampleRate, uint32_t channelCount,
        YouMeMagicVoiceChanger* &mYouMeMagicVoiceChanger,
        ComprehensiveVocoderStereo* &VocoderStereo);
    bool isGameVoip(int uid, int sample_rate);
    // MIAUDIO_VOICE_CHANGE end

public:
    // AudioCloudCtrl
    virtual void updateCloudCtrlData(const String8& keyValuePairs);
    // MIAUDIO_DUMP_PCM begin
    virtual void dumpAudioFlingerDataToFile(
                uint8_t *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                const char *name);
private:
    status_t checkFilePath(const char *filePath);
    // MIAUDIO_DUMP_PCM end

    // MIAUDIO_SPITAL_AUDIO start
public:
    virtual String8 getSpatialDevice(const String8& keys) const;
    // MIAUDIO_SPITAL_AUDIO end


    // MIPERF_PRELOAD_MUTE begin
public:
    virtual bool shouldMutedProcess(uid_t uid);
    virtual bool checkAndUpdatePreloadProcessVolume(const String8& keyValuePairs);
private:
    mutable android::RWLock mPreloadRWLock;
    std::set<uid_t> mPreloadUids;
    // MIPERF_PRELOAD_MUTE end
};

extern "C" IAudioFlingerStub* create();
extern "C" void destroy(IAudioFlingerStub* impl);

}//namespace android

#endif
