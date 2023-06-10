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

#ifndef OZO_C2CONFIG_DEC_H_
#define OZO_C2CONFIG_DEC_H_

#include "OzoC2Config.h"
#include "ozoaudio/ozo_tags.h"
#include "ozoaudiodecoder/ozo_audio_decoder.h"


#define OZODEC_INJECT_PARAMS() \
    addParameter( \
            DefineParam(mSampleRateOut, C2_PARAMKEY_SAMPLE_RATE) \
            .withDefault(new C2StreamSampleRateInfo::output(0u, 48000)) \
            .withFields({C2F(mSampleRateOut, value).oneOf({48000})}) \
            .withSetter((Setter<decltype(*mSampleRateOut)>::StrictValueWithNoDeps)) \
            .build()); \
    addParameter( \
            DefineParam(mChannelCountOut, C2_PARAMKEY_CHANNEL_COUNT) \
            .withDefault(new C2StreamChannelCountInfo::output(0u, 2)) \
            .withFields({C2F(mChannelCountOut, value).inRange(2, 8)}) \
            .withSetter(Setter<decltype(*mChannelCountOut)>::StrictValueWithNoDeps) \
            .build()); \
    addParameter( \
                DefineParam(mGuidance, C2_OZO_PARAMKEY_GUIDANCE_MODE) \
                .withDefault(AllocSharedString<C2StreamDecoderGuidanceOzo::input>("")) \
                .withFields({C2F(mGuidance, m.value).any()}) \
                .withSetter(dummyStringSetter<C2StreamDecoderGuidanceOzo::input>) \
                .build()); \
    addParameter( \
                DefineParam(mListener, C2_OZO_PARAMKEY_LISTENER) \
                .withDefault(AllocSharedString<C2StreamDecoderListenerOzo::input>("")) \
                .withFields({C2F(mListener, m.value).any()}) \
                .withSetter(dummyStringSetter<C2StreamDecoderListenerOzo::input>) \
                .build()); \
    addParameter( \
                DefineParam(mLsWidening, C2_OZO_PARAMKEY_LS_WIDENING) \
                .withDefault(AllocSharedString<C2StreamDecoderLsWideningOzo::input>("")) \
                .withFields({C2F(mLsWidening, m.value).any()}) \
                .withSetter(dummyStringSetter<C2StreamDecoderLsWideningOzo::input>) \
                .build());


namespace android {

class C2SoftOzoDecBaseParams
{
public:
    explicit C2SoftOzoDecBaseParams()
    {}

    uint32_t getSampleRate() const { return mSampleRateOut->value; }
    uint32_t getChannelCount() const { return mChannelCountOut->value; }

    ozoaudiodecoder::AudioRenderingGuidance getRenderingGuidance() const {
        return (!strcmp(mGuidance->m.value, "head-tracking")) ?
            ozoaudiodecoder::AudioRenderingGuidance::HEAD_TRACKING :
            ozoaudiodecoder::AudioRenderingGuidance::NO_TRACKING;
    }

    const char *getListenerOrientation() const {
        return mListener->m.value;
    }

    bool isLsWidening() const {
        return (!strcmp(mLsWidening->m.value, OZO_FEATURE_ENABLED)) ? true : false;
    }

protected:
    std::shared_ptr<C2StreamSampleRateInfo::output> mSampleRateOut;
    std::shared_ptr<C2StreamChannelCountInfo::output> mChannelCountOut;

    // Rendering guidance mode
    std::shared_ptr<C2StreamDecoderGuidanceOzo::input> mGuidance;

    // Listener position
    std::shared_ptr<C2StreamDecoderListenerOzo::input> mListener;

    // Loudspeaker widening
    std::shared_ptr<C2StreamDecoderLsWideningOzo::input> mLsWidening;
};

} // namespace android

#endif  // OZO_C2CONFIG_DEC_H_
