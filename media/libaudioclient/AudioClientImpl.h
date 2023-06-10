#ifndef ANDROID_AUDIOCLIENT_IMPL_H
#define ANDROID_AUDIOCLIENT_IMPL_H

#include <log/log.h>
#include <utils/Errors.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/AudioRecord.h>
#include <IAudioClientStub.h>

namespace android {
//namespace AudioClient {
class AudioClientImpl : public IAudioClientStub {
public:
    virtual ~AudioClientImpl() {}
    // MIAUDIO_GAME_EFFECT begin
    virtual int gameEffectAudioRecordChangeInputSource(
                pid_t pid,
                audio_format_t format,
                audio_source_t inputSource,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                audio_source_t &output,
                audio_flags_mask_t &flags);

    virtual int gameEffectAudioTrackChangeStreamType(
                pid_t pid,
                audio_format_t format,
                audio_stream_type_t streamType,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                audio_stream_type_t &output);
    // MIAUDIO_GAME_EFFECT end

    // MIAUDIO_GAME_4D_VIBRATE begin
    virtual void gameEffectAudioTrackInitShootDetect(
                pid_t processID,
                audio_format_t format,
                audio_stream_type_t streamType,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                const wp<AudioTrack>& owner);

    virtual void gameEffectAudioTrackReleaseShootDetect(
                const wp<AudioTrack>& owner);

    virtual void gameEffectAudioTrackProcessShootDetectAndVibrator(
                const wp<AudioTrack>& owner,
                android::AudioTrack::Buffer audioBuffer,
                uint32_t channelCount);
    // MIAUDIO_GAME_4D_VIBRATE end

    // MIAUDIO_DUMP_PCM begin
    virtual void dumpAudioDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName,
                const sp<StreamOutHalInterface>& streamOut);
    //MIAUDIO_DUMP_PCM end

    // MIAUDIO_3D_PLAY begin
    virtual void initMi3DAudioMapping(void* &mi3DAudioMapping);
    virtual void releaseMi3DAudioMapping(void* &mi3DAudioMapping);
    virtual void setMi3DAudioMappingDeviceType(
               uint32_t* channelCountStart,
               void* mi3DAudioMapping);
    virtual void processMi3DAudioMapping(
               int8_t *audioBuffer,
               size_t audioBuffersize,
               const void* buffer,
               size_t& userSize,
               size_t& written,
               const wp<AudioTrack>& owner,
               FILE* audioFileIn);
    // MIAUDIO_3D_PLAY end

    //MIAUIDO_ULL_CHECK begin
    virtual bool isUllRecordSupport(uid_t uid);
    //MIAUDIO_ULL CHECK end

    //MIUI ADD: onetrack rbis start
    virtual int reportAudiotrackParameters(int playbackTime, const char* clientName, const char* usage, const char* flags, int channelMask, const char* format, int sampleRate);
    //MIUI ADD: onetrack rbis end

private:
        // MIAUDIO_DUMP_PCM begin
        void dumpInitAudioTrackDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName);
        void dumpAudioTrackDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName);
        void dumpAudioRecordDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName);
        void dumpAudioFlingerDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                const sp<StreamOutHalInterface>& streamOut);
        status_t checkFilePath(const char *filePath);
        void getAudioDumpSwitch();
        int dumptoFile(const char *filePath, void *buffer, size_t size);

        int mCounts = 0;
        int mAudioDumpSwitch = 0;
        // MIAUDIO_DUMP_PCM end
};

extern "C" IAudioClientStub* create();
extern "C" void destroy(IAudioClientStub* impl);

//}//namespace AudioClient
}//namespace android

#endif
