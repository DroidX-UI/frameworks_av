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

#include <stdint.h>

#include "ozoaudioencoder/ozo_audio_encoder.h"

typedef struct AACENCODER *HANDLE_AACENCODER;

namespace android {

/**
 * Audio encoder adapter implementation for Ozo encoder.
 * Encapsulates platform AAC encoder implementation.
 */
class OzoAudioAdapterAACEncoder : public ozoaudioencoder::AudioEncoder
{
public:
    OzoAudioAdapterAACEncoder();

    virtual void destroy();

    virtual ozoaudioencoder::AudioEncoderError init(ozoaudioencoder::AudioEncoderConfig *config);

    virtual ozoaudioencoder::AudioEncoderError encode(uint8_t *input, uint8_t *output, int32_t *output_size,
        uint8_t **, int16_t *, int16_t);

    virtual ozoaudioencoder::AudioEncoderError reset();

    void setLevelsObserver(ozoaudioencoder::OzoEncoderObserver *observer);

protected:
    ~OzoAudioAdapterAACEncoder() {}

    ozoaudioencoder::AudioEncoderError create();

private:
    int16_t mChannels;
    size_t mMaxBufferSize;
    HANDLE_AACENCODER mAACEncoder;
    ozoaudioencoder::OzoEncoderObserver *mLevelsObserver;
};

}  // namespace android
