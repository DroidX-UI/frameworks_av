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
#define LOG_TAG "OzoAudioAdapterAACEncoder"
#include <log/log.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "./aac_enc.h"
#include "./ozo_wrapper.h"
#include "aacenc_lib.h"
#include "ozoaudioencoder/ozo_audio_encoder.h"
#include "ozoaudio/utils.h"

namespace android {

OzoAudioAdapterAACEncoder::OzoAudioAdapterAACEncoder():
mChannels(2),
mMaxBufferSize(4096),
mAACEncoder(NULL),
mLevelsObserver(NULL)
{
    ALOGV("Birth");
    m_codecName = "aac";

    this->create();
}

void
OzoAudioAdapterAACEncoder::destroy()
{
    ALOGV("Death");
    aacEncClose(&mAACEncoder);
    delete this;
}

ozoaudioencoder::AudioEncoderError
OzoAudioAdapterAACEncoder::init(ozoaudioencoder::AudioEncoderConfig *config)
{
    ALOGV("init(): %p", mAACEncoder);

    // Set profile, use LC profile
    auto result = aacEncoder_SetParam(this->mAACEncoder, AACENC_AOT, AOT_AAC_LC);
    if (result != AACENC_OK) {
        ALOGE("Failed to set AAC encoder profile");
        return ozoaudioencoder::AudioEncoderError::NOT_INITIALIZED;
    }

    // Set sample rate
    result = aacEncoder_SetParam(this->mAACEncoder, AACENC_SAMPLERATE, config->sample_rate);
    if (result != AACENC_OK) {
        ALOGE("Failed to set AAC encoder sample rate");
        return ozoaudioencoder::AudioEncoderError::NOT_INITIALIZED;
    }

    // Set bitrate, in bps
    result = aacEncoder_SetParam(this->mAACEncoder, AACENC_BITRATE, config->bitrate * 1000);
    if (result != AACENC_OK) {
        ALOGE("Failed to set AAC encoder bitrate");
        return ozoaudioencoder::AudioEncoderError::NOT_INITIALIZED;
    }

    // Set channel mode
    this->mChannels = config->n_in_channels;
    auto mode = (this->mChannels == 2) ? MODE_2 : (this->mChannels == 4) ? MODE_1_2_1 : MODE_1;
    result = aacEncoder_SetParam(this->mAACEncoder, AACENC_CHANNELMODE, mode);
    if (result != AACENC_OK) {
        ALOGE("Failed to set AAC encoder channel mode");
        return ozoaudioencoder::AudioEncoderError::NOT_INITIALIZED;
    }

    // Set raw transport mode (suitable for MP4)
    result = aacEncoder_SetParam(this->mAACEncoder, AACENC_TRANSMUX, TT_MP4_RAW);
    if (result != AACENC_OK) {
        ALOGE("Failed to set AAC encoder transport mode");
        return ozoaudioencoder::AudioEncoderError::NOT_INITIALIZED;
    }

    // No, SBR is not in use
    result = aacEncoder_SetParam(this->mAACEncoder, AACENC_SBR_RATIO, 0);
    if (result != AACENC_OK) {
        ALOGE("Failed to set AAC encoder SBR ratio");
        return ozoaudioencoder::AudioEncoderError::NOT_INITIALIZED;
    }

    // Initial encoding, needed probably to generate decoder config data
    result = aacEncEncode(mAACEncoder, NULL, NULL, NULL, NULL);
    if (result != AACENC_OK) {
        ALOGE("Unable to retrieve decoder specific config data from encoder");
        return ozoaudioencoder::AudioEncoderError::NOT_INITIALIZED;
    }

    // Fetch the decoder specific configuration data
    AACENC_InfoStruct encInfo;
    result = aacEncInfo(mAACEncoder, &encInfo);
    if (result != AACENC_OK) {
        ALOGE("Failed to get AAC encoder info");
        return ozoaudioencoder::AudioEncoderError::NOT_INITIALIZED;
    }

    config->encoding_delay = 2048;
    config->audio_config_size = encInfo.confSize;
    config->max_buffer_size = this->mMaxBufferSize;
    config->frame_length = OzoEncoderWrapper::kNumSamplesPerFrame;
    memcpy(config->audio_specific_config, encInfo.confBuf, config->audio_config_size);

    return ozoaudioencoder::AudioEncoderError::OK;
}

ozoaudioencoder::AudioEncoderError
OzoAudioAdapterAACEncoder::encode(uint8_t *input, uint8_t *output, int32_t *output_size,
        uint8_t **, int16_t *, int16_t)
{
    // Assuming that input is using 16-bit sample depth
    size_t numBytesPerInputFrame = OzoEncoderWrapper::kNumSamplesPerFrame * this->mChannels * sizeof(int16_t);

    AACENC_InArgs inargs;
    AACENC_OutArgs outargs;
    memset(&inargs, 0, sizeof(inargs));
    memset(&outargs, 0, sizeof(outargs));
    inargs.numInSamples = numBytesPerInputFrame / sizeof(int16_t);

    void* inBuffer[]        = { (unsigned char *) input };
    INT   inBufferIds[]     = { IN_AUDIO_DATA };
    INT   inBufferSize[]    = { (INT) numBytesPerInputFrame };
    INT   inBufferElSize[]  = { sizeof(int16_t) };

    AACENC_BufDesc inBufDesc;
    inBufDesc.numBufs           = sizeof(inBuffer) / sizeof(void *);
    inBufDesc.bufs              = (void **) &inBuffer;
    inBufDesc.bufferIdentifiers = inBufferIds;
    inBufDesc.bufSizes          = inBufferSize;
    inBufDesc.bufElSizes        = inBufferElSize;

    void* outBuffer[]       = { output };
    INT   outBufferIds[]    = { OUT_BITSTREAM_DATA };
    INT   outBufferSize[]   = { 0 };
    INT   outBufferElSize[] = { sizeof(UCHAR) };

    AACENC_BufDesc outBufDesc;
    outBufDesc.numBufs           = sizeof(outBuffer) / sizeof(void *);
    outBufDesc.bufs              = (void **) &outBuffer;
    outBufDesc.bufferIdentifiers = outBufferIds;
    outBufDesc.bufSizes          = outBufferSize;
    outBufDesc.bufElSizes        = outBufferElSize;

    memset(&outargs, 0, sizeof(outargs));

    outBuffer[0] = output;
    outBufferSize[0] = this->mMaxBufferSize;

    // Determine audio levels for left and right channels
    if (this->mLevelsObserver && this->mChannels == 2) {
        const int16_t *audio = (const int16_t *) input;
        calcMaxOzoAudioLevels(this->mLevelsObserver, audio, audio + 1, OzoEncoderWrapper::kNumSamplesPerFrame);
    }

    // Encode input signal
    auto encoderErr = aacEncEncode(this->mAACEncoder, &inBufDesc, &outBufDesc, &inargs, &outargs);
    if (encoderErr != AACENC_OK) {
        ALOGE("AAC encoding failed");
        return ozoaudioencoder::AudioEncoderError::PROCESSING_ERROR;
    }

    *output_size = outargs.numOutBytes;

    return ozoaudioencoder::AudioEncoderError::OK;
}

void
OzoAudioAdapterAACEncoder::setLevelsObserver(ozoaudioencoder::OzoEncoderObserver *observer)
{
    this->mLevelsObserver = observer;
}

ozoaudioencoder::AudioEncoderError
OzoAudioAdapterAACEncoder::reset()
{
    ALOGV("reset()");
    aacEncClose(&mAACEncoder);
    return this->create();
}

ozoaudioencoder::AudioEncoderError
OzoAudioAdapterAACEncoder::create()
{
    ALOGV("create()");
    if (AACENC_OK != aacEncOpen(&this->mAACEncoder, 0, 0)) {
        ALOGE("Failed to initialize platform AAC encoder");
        this->mAACEncoder = NULL;
        return ozoaudioencoder::AudioEncoderError::MEMORY_ALLOCATION_FAILED;
    }

    return ozoaudioencoder::AudioEncoderError::OK;
}

}  // namespace android
