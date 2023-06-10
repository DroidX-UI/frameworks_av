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

#ifndef OZO_C2CONFIG_ENC_H_
#define OZO_C2CONFIG_ENC_H_

#include "OzoC2Config.h"
#include "ozoaudio/ozo_tags.h"
#include "ozoaudioencoder/ozo_audio_encoder.h"


#define OZOENC_INJECT_PARAMS() \
    addParameter( \
            DefineParam(mSampleRate, C2_PARAMKEY_SAMPLE_RATE) \
            .withDefault(new C2StreamSampleRateInfo::input(0u, 48000)) \
            .withFields({C2F(mSampleRate, value).oneOf({48000})}) \
            .withSetter((Setter<decltype(*mSampleRate)>::StrictValueWithNoDeps)) \
            .build()); \
    addParameter( \
            DefineParam(mChannelCount, C2_PARAMKEY_CHANNEL_COUNT) \
            .withDefault(new C2StreamChannelCountInfo::input(0u, 2)) \
            .withFields({C2F(mChannelCount, value).inRange(2, 8)}) \
            .withSetter(Setter<decltype(*mChannelCount)>::StrictValueWithNoDeps) \
            .build()); \
    addParameter( \
            DefineParam(mBitrate, C2_PARAMKEY_BITRATE) \
            .withDefault(new C2StreamBitrateInfo::output(0u, 160000)) \
            .withFields({C2F(mBitrate, value).inRange(160000, 960000)}) \
            .withSetter(Setter<decltype(*mBitrate)>::NonStrictValueWithNoDeps) \
            .build()); \
    addParameter( \
            DefineParam(mInputMaxBufSize, C2_PARAMKEY_INPUT_MAX_BUFFER_SIZE) \
            .withDefault(new C2StreamMaxBufferSizeInfo::input(0u, 4 * 1024 * 4)) \
            .withFields({C2F(mInputMaxBufSize, value).inRange(2 * 2 * 1024, 8 * 4 * 1024)}) \
            .withSetter(Setter<decltype(*mInputMaxBufSize)>::StrictValueWithNoDeps) \
            .build()); \
    addParameter( \
            DefineParam(mSampleDepth, C2_OZO_PARAMKEY_SAMPLE_DEPTH) \
            .withDefault(new C2StreamSampleDepthInfoOzo::input(0u, 16)) \
            .withFields({C2F(mSampleDepth, value).oneOf({16, 24})}) \
            .withSetter((Setter<decltype(*mSampleDepth)>::StrictValueWithNoDeps)) \
            .build()); \
    addParameter( \
            DefineParam(mDeviceId, C2_OZO_PARAMKEY_DEVICE_ID) \
            .withDefault(AllocSharedString<C2StreamDeviceIdOzo::input>("")) \
            .withFields({C2F(mDeviceId, m.value).any()}) \
            .withSetter(dummyStringSetter<C2StreamDeviceIdOzo::input>) \
            .build()); \
    addParameter( \
            DefineParam(mEncMode, C2_OZO_PARAMKEY_ENC_MODE) \
            .withDefault(AllocSharedString<C2StreamEncModeOzo::input>(ozoaudioencoder::OZOSDK_OZOAUDIO_ID)) \
            .withFields({C2F(mEncMode, m.value).any()}) \
            .withSetter(dummyStringSetter<C2StreamEncModeOzo::input>) \
            .build()); \
    addParameter( \
            DefineParam(mPostMode, C2_OZO_PARAMKEY_POSTDATA_MODE) \
            .withDefault(new C2StreamPostModeOzo::input(0u, 0)) \
            .withFields({C2F(mPostMode, value).oneOf({0, 1})}) \
            .withSetter((Setter<decltype(*mPostMode)>::StrictValueWithNoDeps)) \
            .build()); \
    addParameter( \
            DefineParam(mWnrNotification, C2_OZO_PARAMKEY_WNR_NOTIFICATION) \
            .withDefault(AllocSharedString<C2StreamWnrNotificationOzo::input>(OZO_FEATURE_DISABLED)) \
            .withFields({C2F(mWnrNotification, m.value).any()}) \
            .withSetter(dummyStringSetter<C2StreamWnrNotificationOzo::input>) \
            .build()); \
    addParameter( \
            DefineParam(mSSLocNotification, C2_OZO_PARAMKEY_SSLOC_NOTIFICATION) \
            .withDefault(AllocSharedString<C2StreamSSLocNotificationOzo::input>(OZO_FEATURE_DISABLED)) \
            .withFields({C2F(mSSLocNotification, m.value).any()}) \
            .withSetter(dummyStringSetter<C2StreamSSLocNotificationOzo::input>) \
            .build()); \
    addParameter( \
            DefineParam(mAudioLevelsNotification, C2_OZO_PARAMKEY_AUDIOLEVELS_NOTIFICATION) \
            .withDefault(AllocSharedString<C2StreamAudioLevelsNotificationOzo::input>(OZO_FEATURE_DISABLED)) \
            .withFields({C2F(mAudioLevelsNotification, m.value).any()}) \
            .withSetter(dummyStringSetter<C2StreamAudioLevelsNotificationOzo::input>) \
            .build()); \
    addParameter( \
            DefineParam(mMicBlockingNotification, C2_OZO_PARAMKEY_MICBLOCKING_NOTIFICATION) \
            .withDefault(AllocSharedString<C2StreamMicBlockingNotificationOzo::input>(OZO_FEATURE_DISABLED)) \
            .withFields({C2F(mMicBlockingNotification, m.value).any()}) \
            .withSetter(dummyStringSetter<C2StreamMicBlockingNotificationOzo::input>) \
            .build()); \
    addParameter( \
            DefineParam(mGenericParam, C2_OZO_PARAMKEY_GENERIC) \
            .withDefault(AllocSharedString<C2StreamGenericParamOzo::input>(OZO_FEATURE_UNINITIALIZED)) \
            .withFields({C2F(mGenericParam, m.value).any()}) \
            .withSetter(dummyStringSetter<C2StreamGenericParamOzo::input>) \
            .build()); \


namespace android {

class C2SoftOzoEncBaseParams
{
public:
    explicit C2SoftOzoEncBaseParams()
    {}

    uint32_t getSampleRate() const { return mSampleRate->value; }
    uint32_t getChannelCount() const { return mChannelCount->value; }
    uint32_t getBitrate() const { return mBitrate->value; }

    uint32_t getSampleDepth() const { return mSampleDepth->value; }
    const char *getDeviceId() const { return mDeviceId->m.value; }
    const char *getEncMode() const { return mEncMode->m.value; }
    bool getPostMode() const { return mPostMode->value == 1 ? true : false; }
    bool getWnrNotification() const {
        return !strcmp(mWnrNotification->m.value, OZO_FEATURE_ENABLED) ? true : false;
    }

    bool getSSLocNotification() const {
        return !strcmp(mSSLocNotification->m.value, OZO_FEATURE_ENABLED) ? true : false;
    }

    bool getAudioLevelsNotification() const {
        return !strcmp(mAudioLevelsNotification->m.value, OZO_FEATURE_ENABLED) ? true : false;
    }

    bool getMicBlockingNotification() const {
        return !strcmp(mMicBlockingNotification->m.value, OZO_FEATURE_ENABLED) ? true : false;
    }

    const char *getGenericParam() const { return mGenericParam->m.value; }

protected:
    std::shared_ptr<C2StreamSampleRateInfo::input> mSampleRate;
    std::shared_ptr<C2StreamChannelCountInfo::input> mChannelCount;
    std::shared_ptr<C2StreamBitrateInfo::output> mBitrate;

    // Current assumptions, modify if needed
    // Default size: 4 mics, 24-bits sample depth
    // Min - Max size: 2 mics@16-bit - 8mics@24-bit
    std::shared_ptr<C2StreamMaxBufferSizeInfo::input> mInputMaxBufSize;

    // Audio sample depth
    std::shared_ptr<C2StreamSampleDepthInfoOzo::input> mSampleDepth;
    // Device ID
    std::shared_ptr<C2StreamDeviceIdOzo::input> mDeviceId;
    // Encoding mode
    std::shared_ptr<C2StreamEncModeOzo::input> mEncMode;

    // WNR notification events (i.e., wind levels)
    std::shared_ptr<C2StreamWnrNotificationOzo::input> mWnrNotification;

    // Ozo Tune
    std::shared_ptr<C2StreamPostModeOzo::input> mPostMode;

    // Sound source locallization notification events
    std::shared_ptr<C2StreamSSLocNotificationOzo::input> mSSLocNotification;

    // Audio levels notification events (i.e., L/R levels)
    std::shared_ptr<C2StreamAudioLevelsNotificationOzo::input> mAudioLevelsNotification;

    // Mic blocking notification events
    std::shared_ptr<C2StreamMicBlockingNotificationOzo::input> mMicBlockingNotification;

    // Generic encoder parameter set
    std::shared_ptr<C2StreamGenericParamOzo::input> mGenericParam;
};

} // namespace android

#endif  // OZO_C2CONFIG_ENC_H_
