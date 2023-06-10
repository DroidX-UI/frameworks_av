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
#include <utils/Mutex.h>

#include "ozoaudiodecoder/ozo_audio_decoder.h"

typedef struct LEngine *WideningPtr;

namespace android {

static const int OZO_MAX_DECODER_STREAMS = 1;

typedef struct OzoDecoderStreamInfo_s
{
    int16_t frameSize;
    int16_t numChannels;
    int32_t sampleRate;

} OzoDecoderStreamInfo;

typedef struct OzoDecoderConfigParams_s
{
    uint8_t *config[OZO_MAX_DECODER_STREAMS];
    int32_t config_size[OZO_MAX_DECODER_STREAMS];
    ozoaudiodecoder::TrackType track_type;
    ozoaudiodecoder::AudioRenderingGuidance guidance;
    std::string orientation;
    bool lsWidening;
} OzoDecoderConfigParams_t;

typedef struct OzoDecoderDecodeParams_s
{
    uint8_t *data[OZO_MAX_DECODER_STREAMS];
    int32_t data_size[OZO_MAX_DECODER_STREAMS];

} OzoDecoderDecodeParams_t;

class OzoDecoderWrapper
{
public:
    OzoDecoderWrapper();
    ~OzoDecoderWrapper();

    enum {
        kOrientationRuntimeFlag = 1
    };

    bool init(OzoDecoderConfigParams_t *params);
    bool decode(OzoDecoderDecodeParams_t *params, uint8_t *output, size_t *output_size);
    bool flush();

    bool setRuntimeParameters(const OzoDecoderConfigParams_t &params, uint64_t flags);

    const OzoDecoderStreamInfo *getStreamInfo() const;

    size_t getInitialDelay() const;

    static void readCSD(int32_t & sampleRate, int32_t & numChannel, const uint8_t *csd, size_t csd_size);

private:
    class TrackData
    {
    public:
        ozoaudiodecoder::AudioDecoder *mAdapterDecoder;
        ozoaudiodecoder::OzoAudioMultiTrackDecoderConfig mTrackConfig;

        TrackData(ozoaudiodecoder::TrackType track_type,
            ozoaudiodecoder::AudioRenderingGuidance guidance);
        ~TrackData();

        bool init(uint8_t *config, int32_t size);
    };

    bool addTrack(TrackData *track, uint8_t *config, int32_t size);
    bool initMixerConfig(bool lsWidening);
    bool initDecoder(bool lsWidening);
    void reset();

    bool initLsWidening(const OzoDecoderStreamInfo &streamInfo);

    bool setOrientation(const std::string &orientation);
    bool setOrientationParams(ozoaudiocodec::AudioControl *ctrl, double yaw, double pitch,
        double roll);

    ozoaudiodecoder::OzoAudioMultiTrackDecoder *mOzoHandle;
    ozoaudiodecoder::OzoAudioDecoderMixerConfig mOzoMixerConfig;

    TrackData *mTrack;

    OzoDecoderStreamInfo mStreamInfo;

    Mutex mLock;

    WideningPtr mWideningHandle;
};

}  // namespace android
