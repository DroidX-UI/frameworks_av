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

#pragma once

#include <string>
#include <vector>
#include <utils/Mutex.h>

#include "ozoaudioencoder/ozo_audio_encoder.h"
#include "ozoaudio/utils.h"
#include "ozoaudiocommon/utils.h"
#include "parameters.h"

typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace android {

class OzoAudioAdapterAACEncoder;

typedef struct OzoEncoderParams_s
{
    uint32_t channels;
    uint32_t samplerate;
    uint32_t bitrate;
    uint32_t profile;
    std::string deviceId;
    size_t sampledepth;
    std::string encoderMode;

    std::vector<EventEmitter *> eventEmitters;

    // Post data file
    bool postData;

    // Should the output be encoded or not
    bool pcmOnly;
    ozoaudiocodec::SampleFormat pcmFormat;

} OzoEncoderParams_t;

class OzoEncoderWrapper
{
public:
    OzoEncoderWrapper();
    ~OzoEncoderWrapper();

    enum {
        kNumSamplesPerFrame = 1024,
    };

    bool init(const OzoEncoderParams_t &params, OzoParameterUpdater &paramUpdater);
    bool encode(uint8_t *input, ozoaudiocodec::AudioDataContainer &output);
    bool setRuntimeParameters(OzoParameterUpdater &params);

    const ozoaudiocodec::CodecData & getConfig() const;

    size_t getInitialDelay() const;

    ozoaudioencoder::RawMicWriterBuffered *mMicWriter;
    ozoaudioencoder::CtrlWriterBuffered *mCtrlWriter;

    ozoaudioencoder::OzoAudioEncoder *getOzoAudioHandle() const
    {
        return this->mOzoHandle;
    }

private:
    ozoaudiocodec::SampleFormat getSampleFormat();

    OzoEncoderParams_t mParams;

    ozoaudioencoder::OzoAudioEncoder *mOzoHandle;
    ozoaudioencoder::OzoAudioEncoderInitConfig mInConfig;
    ozoaudioencoder::OzoAudioEncoderCodecConfig mCodecConfig;
    ozoaudioencoder::OzoAudioEncoderConfig mOzoConfig;
    OzoAudioAdapterAACEncoder *mAdapterEncoder;

    Mutex mLock;

    bool m441kHz;
    uint8_t mEncConfig[64];
    uint8_t mEncOutput[4096];
    SpeexResamplerState *mResampler;
    ozoaudiocodec::OzoRingBufferT<int16_t> *mResamplerOutput;
};

// Initialize Ozo Audio parameters to default values
void init_default_ozoparams(OzoEncoderParams_t *params);

}  // namespace android
