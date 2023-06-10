/*
Copyright (C) 2019,2020 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

#ifndef OZO_C2CONFIG_H_
#define OZO_C2CONFIG_H_

#include <C2.h>
#include <C2Component.h>
#include <C2Enum.h>
#include <C2ParamDef.h>

enum ExtendedVendorC2ParamIndexKind : C2Param::type_index_t {
    // Encoder
    kParamIndexVendorSampleDepth = C2Param::TYPE_INDEX_VENDOR_START,
    kParamIndexVendorDevice,
    kParamIndexVendorEncMode,
    kParamIndexVendorWnrNotification,
    kParamIndexVendorTune,
    kParamIndexVendorFramingNotification,
    kParamIndexVendorAudioLevelsNotification,
    kParamIndexVendorMicBlockingNotification,
    kParamIndexVendorGenericParam,

    // Decoder
    kParamIndexVendorDecoderGuidance,
    kParamIndexVendorDecoderOrientation,
    kParamIndexVendorDecoderLsWidening,
};

/**
 * Audio sample depth.
 */
typedef C2StreamParam<C2Info, C2Uint32Value, kParamIndexVendorSampleDepth> C2StreamSampleDepthInfoOzo;
constexpr char C2_OZO_PARAMKEY_SAMPLE_DEPTH[] = "ozoaudio.sample-depth";

/**
 * OZO Audio device ID.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorDevice> C2StreamDeviceIdOzo;
constexpr char C2_OZO_PARAMKEY_DEVICE_ID[] = "ozoaudio.device";

/**
 * OZO Audio encoding mode.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorEncMode> C2StreamEncModeOzo;
constexpr char C2_OZO_PARAMKEY_ENC_MODE[] = "ozoaudio.mode";

/**
 * OZO Audio wind noise notification mode.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorWnrNotification> C2StreamWnrNotificationOzo;
constexpr char C2_OZO_PARAMKEY_WNR_NOTIFICATION[] = "ozoaudio.wnr-notification";

/**
 * OZO Audio Tune mode.
 */
typedef C2StreamParam<C2Info, C2Uint32Value, kParamIndexVendorTune> C2StreamPostModeOzo;
constexpr char C2_OZO_PARAMKEY_POSTDATA_MODE[] = "ozoaudio.postdata";

/**
 * OZO Audio sound source localization notification mode.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorFramingNotification> C2StreamSSLocNotificationOzo;
constexpr char C2_OZO_PARAMKEY_SSLOC_NOTIFICATION[] = "ozoaudio.ssloc-notification";

/**
 * OZO Audio levels notification mode.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorAudioLevelsNotification> C2StreamAudioLevelsNotificationOzo;
constexpr char C2_OZO_PARAMKEY_AUDIOLEVELS_NOTIFICATION[] = "ozoaudio.audiolevels-notification";

/**
 * OZO Audio mic blocking notification mode.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorMicBlockingNotification> C2StreamMicBlockingNotificationOzo;
constexpr char C2_OZO_PARAMKEY_MICBLOCKING_NOTIFICATION[] = "ozoaudio.micblocking-notification";

/**
 * OZO Audio generic parameter carriage mechanism.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorGenericParam> C2StreamGenericParamOzo;
constexpr char C2_OZO_PARAMKEY_GENERIC[] = "ozoaudio.generic";


/**
 * OZO Audio decoder rendering guidance.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorDecoderGuidance> C2StreamDecoderGuidanceOzo;
constexpr char C2_OZO_PARAMKEY_GUIDANCE_MODE[] = "ozoaudio.guidance";

/**
 * OZO Audio decoder listener orientation.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorDecoderOrientation> C2StreamDecoderListenerOzo;
constexpr char C2_OZO_PARAMKEY_LISTENER[] = "ozoaudio.orientation";

/**
 * OZO Audio decoder headphone vs loudspeaker rendering.
 */
typedef C2StreamParam<C2Info, C2StringValue, kParamIndexVendorDecoderLsWidening> C2StreamDecoderLsWideningOzo;
constexpr char C2_OZO_PARAMKEY_LS_WIDENING[] = "ozoaudio.ls-widening";


#define OZOCOMMON_INJECT_PARAMS() \
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
            .build());

namespace android {

template <typename T>
static C2R dummyStringSetter(bool /*mayBlock*/, C2InterfaceHelper::C2P<T> & /*me*/) { return C2R::Ok(); }

} // namespace android

#endif  // OZO_C2CONFIG_H_
