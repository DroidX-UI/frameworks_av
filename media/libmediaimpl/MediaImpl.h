#ifndef ANDROID_MI_MEDIA_IMPL_H
#define ANDROID_MI_MEDIA_IMPL_H

#include <log/log.h>
#include <utils/Errors.h>

#include <IMediaStub.h>

namespace android {
class MediaImpl : public IMediaStub {
public:
    virtual ~MediaImpl() {}
	// MIAUDIO_OZO begin
    virtual void parseOzoOrientations(KeyedVector<size_t, AString> & events,
            const char *data);
    virtual status_t setOzoAudioTuneFile( sp<IMediaRecorder> &mMediaRecorder,
            media_recorder_states &mCurrentState, int fd);
    virtual status_t setOzoRunTimeParametersMedia(
            sp<IMediaRecorder> &mMediaRecorder,
            media_recorder_states &mCurrentState,
            const String8& params);
    virtual void createOzoAudioParamsStagefright(void * &ozoAudioParams);
    virtual void destroyOzoAudioParamsStagefright(void * &ozoAudioParams);
    virtual void closeOzoAudioTuneFile(IMediaCodecEventListener* &ozoTuneWriter);
    virtual bool ozoAudioSetParameterStagefrightImpl(void *params,
            bool &ozoFileSourceEnable, const String8 &key, const String8 &value);
    virtual void ozoAudioInitStagefrightImpl(void* ozoAudioParams);
    virtual bool setOzoAudioTuneFileStagefrightRecorder(
            IMediaCodecEventListener* &ozoTuneWriter, int fd);
    virtual status_t setOzoRunTimeParametersStagefright(void *ozoAudioParams,
            sp<MediaCodecSource> audioEncoderSource, const String8 &params);
    virtual status_t createOzoAudioSource(
            const audio_attributes_t &attr,
            const android::content::AttributionSourceState& clientIdentity,
            uint32_t sourceSampleRate,
            uint32_t channels,
            uint32_t outSampleRate,
            audio_port_handle_t selectedDeviceId,
            audio_microphone_direction_t selectedMicDirection,
            float selectedMicFieldDimension,
            audio_encoder audioEncoder,
            output_format outputFormat,
            void *ozoAudioParams,
            sp<AudioSource> &audioSource,
            sp<AudioSource> &audioSourceNode,
            bool ozoFileSourceEnable,
            bool &use_ozo_capture);
    virtual void setOzoAudioFormat(
            int32_t &channels,
            int32_t samplerate,
            void *ozoAudioParams,
            IMediaCodecEventListener* &codecEventListener,
            bool use_ozo_capture,
            bool &ozoBrandEnabled,
            int32_t &audioBitRate,
            sp<AudioSource> &audioSource,
            IMediaCodecEventListener *ozoTuneWriter,
            sp<IMediaRecorderClient> &listener,
            sp<AMessage> &format);
    virtual void setOzoAudioCodecInfo(
            sp<MediaCodecSource> audioEncoderSource,
            IMediaCodecEventListener *ozoTuneWriter,
            IMediaCodecEventListener* codecEventListener);
    virtual void tryOzoAudioConfigureImpl( const sp<AMessage> &format,
            AString &mime, void* &playCtr);
    virtual void ozoPlayCtrlSignal(sp<MediaCodec> &codec, void* &playCtr);
	virtual void destroyOzoPlayCtrl(void* &oZoPlayCtr);
	// MIAUDIO_OZO end
    // DOLBY_ENABLE begin
    // DOLBY_ATMOS_GAME begin
    /*
	virtual bool dolbyCreateEffect(audio_channel_mask_t inputChannelMask,
		int32_t processedType, sp<EffectsFactoryHalInterface> &effectsFactory,
		int32_t sessionId, sp<EffectHalInterface> &downmixInterface);
	virtual void dolbyDapInit(uint32_t numEffects,
		sp<EffectsFactoryHalInterface> &effectsFactory);
	// DOLBY_ATMOS_GAME end
     */

	// DOLBY_AC4_SPLIT_SEC begin
	virtual bool dolbyDlbGenClass1Check();
	virtual void dolbyOMXNodeSetParameter(sp<IOMXNode> &mOMXNode);
	// DOLBY_AC4_SPLIT_SEC end

private:
	// DOLBY_ATMOS_GAME begin
    // effect descriptor for the Dap used by the mixer
    static effect_descriptor_t sDapFxDesc;
    // indicates whether a Dap effect has been found and is usable by this mixer
    static bool sIsDapMultichannelCapable;
	// DOLBY_ATMOS_GAME end
    static const int32_t SESSION_ID_INVALID_AND_IGNORED = -2;
    // DOLBY_END

public:
    virtual void setPlaybackCaptureAllowed(bool &playbackCaptureAllowed,
        void* packages);

    virtual bool blockWhenInvisible(uid_t uid, String16& pkgName);

    //MIUI ADD FOR ONETRACK RBIS START
    virtual void sendFrameRate(int frameRate);
    virtual void sendWidth(int32_t mVideoWidth);
    virtual void sendHeight(int32_t mVideoHeight);
    virtual void sendPackageName(const char* packageName);
    virtual void sendDolbyVision(const char* name);
    virtual void sendMine(const char* mime);
    virtual void sendHdrInfo(int32_t fHdr);
    virtual void sendFrameRateFloatCal(int frameRateFloatCal);
    virtual void updateFrcAieAisState();
    virtual void sendVideoPlayData();
private:
    int mediacodec_height = 0;
    int mediacodec_width = 0;
    int mediacodec_frameRate = 0;
    int mediacodec_hdr = 0;
    int mediacodec_frameRateCal = 0;
    int mediacodec_frcState = 0;
    int mediacodec_aieState = 0;
    int mediacodec_aisState = 0;
    int mediacodec_dolbyVision = 0;
    char mediacodec_packgeName[64] = "";//MALLOC_SIZE
    char mediacodec_mime[64] = "";
    //MIUI ADD FOR ONETRACK RBIS END

};

extern "C" IMediaStub* create();
extern "C" void destroy(IMediaStub* impl);

}//namespace android

#endif