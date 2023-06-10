/*
Copyright (C) 2019 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "OzoEncoderWrapper"
#include <log/log.h>
#include <OMX_Audio.h> // OMX_AUDIO_AACObjectLC

#include "ozo_wrapper.h"
#include "./event_emitter.h"
#include "./aac_enc.h"
#include "ozoaudio/ozo_tags.h"
#include "ozoaudio/utils.h"

#include "speex/speex_resampler.h"

#define OZO_LOGGER
#define UNINITIALIZED_FOCUS_SECTOR_VALUE (360.0f)

namespace android {

void
init_default_ozoparams(OzoEncoderParams_t *params)
{
    // Defaults
    // REMEMBER TO CHANGE THESE TO MATCH YOUR DEVICE SETTINGS (otherwise VTS tests fail)!!
    // deviceId MUST be changed
    // channels also unless device is 3-mic device
    params->channels = 3;
    params->samplerate = 48000;
    params->bitrate = 256;
    params->profile = OMX_AUDIO_AACObjectLC;
    // Pixel 3 device ID
    params->deviceId = "3D7BEEFA-93C7-420E-BD34-A5C4B3C0F830";
    params->sampledepth = 16;
    params->encoderMode = ozoaudioencoder::OZOSDK_OZOAUDIO_ID;
    params->postData = false;
    params->pcmOnly = false;
    params->pcmFormat = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;
}


#ifdef OZO_LOGGER
class OzoLoggerReceiver: public ozoaudiocodec::Logger
{
public:
    OzoLoggerReceiver() {};
    ~OzoLoggerReceiver() {};

    virtual void log(ozoaudiocodec::LoggerLevel, const char *msg)
    {
        ALOGD("Ozo logger: %s", msg);
    }
};

static OzoLoggerReceiver logger;

#endif // OZO_LOGGER


OzoEncoderWrapper::OzoEncoderWrapper():
mMicWriter(NULL),
mCtrlWriter(NULL),
mOzoHandle(NULL),
m441kHz(false),
mResampler(NULL),
mResamplerOutput(NULL)
{
    ALOGD("Constructor");
    ALOGD("ozoaudioencoder SDK version=%s\n", ozoaudioencoder::OzoAudioEncoder::version());

    this->mParams.channels = 3;
    this->mParams.samplerate = 48000;
    this->mParams.bitrate = 192000;
    this->mParams.profile = OMX_AUDIO_AACObjectLC;
    this->mParams.deviceId = "";
    this->mParams.sampledepth = 16;

    this->mAdapterEncoder = new OzoAudioAdapterAACEncoder();
}

OzoEncoderWrapper::~OzoEncoderWrapper()
{
    ALOGD("Destructor");

#ifdef OZO_LOGGER
    ozoaudiocodec::removeLogger(logger);
#endif // OZO_LOGGER

    if (this->mAdapterEncoder)
        this->mAdapterEncoder->destroy();
    this->mAdapterEncoder = NULL;

    if (this->mOzoHandle)
        this->mOzoHandle->destroy();
    this->mOzoHandle = NULL;

    delete this->mMicWriter;
    this->mMicWriter = NULL;

    delete this->mCtrlWriter;
    this->mCtrlWriter = NULL;

    if (this->mResampler)
        speex_resampler_destroy(this->mResampler);
    this->mResampler = NULL;

    if (this->mResamplerOutput)
        delete this->mResamplerOutput;
    this->mResamplerOutput = NULL;
}

bool
OzoEncoderWrapper::init(const OzoEncoderParams_t &params, OzoParameterUpdater &paramUpdater)
{
    ALOGD("init");

#ifdef OZO_LOGGER
    ozoaudiocodec::registerLogger(logger, ozoaudiocodec::LoggerLevel::OZO_INFO_LEVEL);
#endif // OZO_LOGGER

    if (!setOzoAudioSdkLicense())
        return false;

    this->mParams = params;

    // Mic array input @48kHz is processed using Ozo Audio SDK which returns
    // mono PCM output @48kHz which is then resampled to 44kHz before
    // AAC encoding.
    this->m441kHz = (this->mParams.encoderMode == "ls-mono-44.1");

    // Create data observers to collect the raw mic and associated control data for post capture
    if (params.postData) {
        this->mMicWriter = new ozoaudioencoder::RawMicWriterBuffered();
        this->mCtrlWriter = new ozoaudioencoder::CtrlWriterBuffered();
    }

    auto createMode = (this->m441kHz) ? ozoaudioencoder::OZOSDK_LS_ID : this->mParams.encoderMode.c_str();

    this->mOzoHandle = ozoaudioencoder::OzoAudioEncoder::create(createMode);
    if (!this->mOzoHandle) {
        ALOGE("Failed to encoder instance for mode: %s", createMode);
        return false;
    }

    bool monoMode = (this->mParams.encoderMode == ozoaudioencoder::OZOSDK_LS_ID || this->m441kHz);
    bool ambisonicsMode = (this->mParams.encoderMode == ozoaudioencoder::OZOSDK_AMBISONICS_ID);
    bool pcm = (this->m441kHz) ? true : this->mParams.pcmOnly;

    /*
     * Use fixed bitrate for Ambisonics. If bitrate is set in the upper layers (particularly
     * in StagefrightRecorder), it will get cut to a encoder specific maximum bitrate which
     * may be too low for high quality Ambisonics encoding.
     */
    if (ambisonicsMode)
        this->mParams.bitrate = 512000;

    this->mCodecConfig.bitrate = this->mParams.bitrate / 1000;
    this->mInConfig.n_in_channels = this->mParams.channels;
    this->mInConfig.in_layout = NULL;
    this->mInConfig.in_sample_format = this->getSampleFormat();
    this->mInConfig.n_out_channels = monoMode ? 1 : (ambisonicsMode) ? 4 : 2;
    this->mInConfig.out_layout = monoMode ? OZO_AUDIO_MONO_LAYOUT : (ambisonicsMode) ? OZO_AUDIO_40_LAYOUT : NULL;
    this->mInConfig.out_sample_format = (pcm) ? this->mParams.pcmFormat : ozoaudiocodec::SAMPLE_NONE;
    this->mInConfig.sample_rate = this->mParams.samplerate;
    this->mInConfig.device = this->mParams.deviceId.c_str();

    this->mOzoConfig.in_config = &this->mInConfig;
    this->mOzoConfig.codec_config = &this->mCodecConfig;

    // Observer subscriptions
    for (size_t i = 0; i < this->mParams.eventEmitters.size(); i++) {
        if (this->mParams.eventEmitters[i]->isSdkObserver()) {
            ALOGD("Observer subscription: %i", this->mParams.eventEmitters[i]->getMode());
            auto err = this->mOzoHandle->setObserver(this->mParams.eventEmitters[i]);
            if (err != ozoaudiocodec::AudioCodecError::OK) {
                fprintf(stderr, "Error: Unable to set observer, index: %zu - error code: %i", i, err);
                return false;
            }
        }
    }

    // Audio levels meter analysis requested from AAC encoder
    if (!pcm) {
        for (size_t i = 0; i < this->mParams.eventEmitters.size(); i++) {
            if (this->mParams.eventEmitters[i]->getMode() == kOzoAudioLevelObserverID) {
                this->mAdapterEncoder->setLevelsObserver(this->mParams.eventEmitters[i]);
                break;
            }
        }
    }

    // Observer subscriptions for post data collection
    if (this->mParams.postData) {
        auto err = this->mOzoHandle->setObserver(this->mMicWriter);
        if (err != ozoaudiocodec::AudioCodecError::OK) {
            fprintf(stderr, "Error: Unable to set microphone data observer, error code: %i", err);
            return false;
        }

        err = this->mOzoHandle->setObserver(this->mCtrlWriter);
        if (err != ozoaudiocodec::AudioCodecError::OK) {
            fprintf(stderr, "Error: Unable to set control data observer, error code: %i", err);
            return false;
        }
    }

    ALOGD("Initializing OZO Audio encoder with following parameters:");
    ALOGD("---------------------------------------------------------");
    ALOGD("Input channels: %i", this->mInConfig.n_in_channels);
    ALOGD("Input sample format: %i", this->mInConfig.in_sample_format);
    ALOGD("Output channels: %i", this->mInConfig.n_out_channels);
    ALOGD("Output sample format: %i", this->mInConfig.out_sample_format);
    ALOGD("Sample rate: %i", this->mInConfig.sample_rate);
    ALOGD("Capture device: %s", this->mInConfig.device);
    ALOGD("Bitrate: %ikbps", this->mCodecConfig.bitrate);
    ALOGD("Capture input sample bits: %zu", this->mParams.sampledepth);
    ALOGD("Capture mode: %s", this->mParams.encoderMode.c_str());
    ALOGD("Capture layout: %s", this->mInConfig.out_layout);
    ALOGD("Include AAC encoder: %s", pcm ? "No" : "Yes");
    ALOGD("Instance creation mode: %s", createMode);
    ALOGD("---------------------------------------------------------");

    // Initialize codec, use platform AAC encoder
    auto result = this->mOzoHandle->init(this->mOzoConfig, (pcm) ? nullptr : this->mAdapterEncoder);
    if (result != ozoaudiocodec::OK) {
        ALOGE("Failed to initialize encoder, error code: %i", result);
        return false;
    }

    // Initialize AAC encoder outside of Ozo Audio SDK as encoding is done as post-processing step
    if (this->m441kHz) {
        int error;

        // Initialize 48 -> 44.1kHz resampling
        this->mResampler = speex_resampler_init(1, this->mInConfig.sample_rate, 44100, 10, &error);
        if (!this->mResampler) {
            ALOGE("Failed to initialize resampler, error code: %i", error);
            return false;
        }

        // Resampled input samples are stored here
        this->mResamplerOutput = new (std::nothrow) ozoaudiocodec::OzoRingBufferT<int16_t>(2048);
        if (!this->mResamplerOutput) {
            ALOGE("Failed to initialize resampler output storage");
            return false;
        }

        ozoaudioencoder::AudioEncoderConfig aac_config;

        // Initialize AAC encoder for 44.1kHz input
        aac_config.n_in_channels = 1;
        aac_config.sample_rate = 44100;
        aac_config.bitrate = this->mCodecConfig.bitrate;

        auto aac_result = this->mAdapterEncoder->init(&aac_config);
        if (aac_result != ozoaudioencoder::OK) {
            ALOGE("Failed to initialize AAC encoder, error code: %i", aac_result);
            return false;
        }

        // Save decoder specific config
        this->mOzoConfig.out_config.vr_config.audio.data = this->mEncConfig;
        this->mOzoConfig.out_config.vr_config.audio.data_size = aac_config.audio_config_size;
        memcpy(this->mEncConfig, aac_config.audio_specific_config, aac_config.audio_config_size);
    }

    ALOGD("OZO Audio encoding details:");
    ALOGD("---------------------------");
    ALOGD("Input frame length: %i samples", this->mOzoConfig.out_config.frame_length);
    ALOGD("---------------------------");

    if (!(monoMode || ambisonicsMode)) {
        if (!paramUpdater.updateParams(this->mOzoHandle->getControl())) {
            ALOGE("Unable to set initial capture settings");
            return false;
        }
    }

    return true;
}

bool
OzoEncoderWrapper::encode(uint8_t *input, ozoaudiocodec::AudioDataContainer &output)
{
    Mutex::Autolock autoLock(this->mLock);

    if (this->mParams.postData) {
        this->mMicWriter->clear();
        this->mCtrlWriter->clear();
    }

    // Call OZO Audio
    auto result = this->mOzoHandle->encode(input, output);
    if (result != ozoaudiocodec::OK) {
        ALOGD("Encoding failed, error code: %i", result);
        return false;
    }

    // Calculate audio levels from Ozo Audio output if it is not in encoded format
    if (this->mParams.pcmOnly) {
        for (size_t i = 0; i < this->mParams.eventEmitters.size(); i++) {
            if (this->mParams.eventEmitters[i]->getMode() == kOzoAudioLevelObserverID) {
                const int16_t *audio = (const int16_t *) output.audio.data;
                calcMaxOzoAudioLevels(this->mParams.eventEmitters[i], audio, audio + 1,
                    this->mOzoConfig.out_config.frame_length);
                break;
            }
        }
    }

    // Encode the PCM output to AAC
    if (this->m441kHz) {
#define SAMPLES_44_1 941 // round((44100 / 48000) * 1024)

        int16_t resampleBuf[1024] = {0};
        spx_uint32_t frIn = this->mOzoConfig.out_config.frame_length;
        spx_uint32_t frOut = SAMPLES_44_1;

        // Be default no output is provided
        output.audio.data_size = 0;

        // Resample output from Ozo Audio SDK
        speex_resampler_process_int(this->mResampler, 0, (int16_t *) output.audio.data, &frIn, resampleBuf, &frOut);
        this->mResamplerOutput->write(resampleBuf, frOut);

        // Check if enough samples are available for encoding
        if (this->mResamplerOutput->samplesAvailable() >= frIn) {
            int32_t enc_size;
            int16_t aacInput[1024];

            // Get the resampled input samples
            this->mResamplerOutput->read(aacInput, frIn);

            // AAC encode
            auto aac_result = this->mAdapterEncoder->encode(
                (uint8_t *) aacInput, this->mEncOutput, &enc_size, nullptr, nullptr, 0
            );

            if (aac_result != ozoaudioencoder::OK) {
                ALOGE("AAC encoding failed, error code: %i", aac_result);
                return false;
            }

            output.audio.data = this->mEncOutput;
            output.audio.data_size = enc_size;
        }
    }

    return true;
}

const ozoaudiocodec::CodecData &
OzoEncoderWrapper::getConfig() const
{
    return this->mOzoConfig.out_config.vr_config.audio;
}

ozoaudiocodec::SampleFormat
OzoEncoderWrapper::getSampleFormat()
{
    switch(this->mParams.sampledepth) {
        case 16:
            return ozoaudiocodec::SAMPLE_16BIT;

        case 24:
            return ozoaudiocodec::SAMPLE_24BIT;

        case 32:
            return ozoaudiocodec::SAMPLE_NORMFLOAT;

        default:
            ALOGE("Unsupported input sample size: %zu-bits", this->mParams.sampledepth);
            return ozoaudiocodec::SAMPLE_16BIT;
    }
}

size_t
OzoEncoderWrapper::getInitialDelay() const
{
    return (this->mParams.encoderMode == ozoaudioencoder::OZOSDK_OZOAUDIO_ID) ? 0 : 0;
}

bool
OzoEncoderWrapper::setRuntimeParameters(OzoParameterUpdater &params)
{
    Mutex::Autolock autoLock(this->mLock);
    return params.updateParams(this->mOzoHandle->getControl());
}

}  // namespace android
