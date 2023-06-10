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
#define LOG_TAG "OzoStagefright"

#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/MediaDefs.h>

#include "OzoStagefright.h"
#include "ozoaudio/ozo_tags.h"
#include "ozoaudioencoder/ozo_audio_encoder.h"

namespace android {

// Ozo codec parameters
static const char *const kOzoSampleDepthTag = "vendor.ozoaudio.sample-depth.value";
static const char *const kOzoDeviceIdTag = "vendor.ozoaudio.device.value";
static const char *const kOzoEncodingModeTag = "vendor.ozoaudio.mode.value";
static const char *const kOzoWnrNotificationTag = "vendor.ozoaudio.wnr-notification.value";
static const char *const kOzoTuneTag = "vendor.ozoaudio.postdata.value";
static const char *const kOzoSSLocNotificationTag = "vendor.ozoaudio.ssloc-notification.value";
static const char *const kOzoAudioLevelsNotificationTag = "vendor.ozoaudio.audiolevels-notification.value";
static const char *const kOzoMicBlockingNotificationTag = "vendor.ozoaudio.micblocking-notification.value";
static const char *const kOzoGenericParamTag = "vendor.ozoaudio.generic.value";

static const char *const kOzoDeviceInputChannelsTag = "stagefrightrecorder.ozoaudio.input-channels";
static const char *const kOzoFileSourceEnableTag = "stagefrightrecorder.ozoaudio.file-source-enable";
static const char *const kOzoChannelMaskTag = "stagefrightrecorder.ozoaudio.channelmask";


// Attempt to parse an int64 literal optionally surrounded by whitespace,
// returns true on success, false otherwise.
static bool
safe_strtoi64(const char *s, int64_t *val) {
    char *end;

    // It is lame, but according to man page, we have to set errno to 0
    // before calling strtoll().
    errno = 0;
    *val = strtoll(s, &end, 10);

    if (end == s || errno == ERANGE) {
        return false;
    }

    // Skip trailing whitespace
    while (isspace(*end)) {
        ++end;
    }

    // For a successful return, the string must contain nothing but a valid
    // int64 literal optionally surrounded by whitespace.

    return *end == '\0';
}

// Return true if the value is in [0, 0x007FFFFFFF]
static bool
safe_strtoi32(const char *s, int32_t *val) {
    int64_t temp;
    if (safe_strtoi64(s, &temp)) {
        if (temp >= 0 && temp <= 0x007FFFFFFF) {
            *val = static_cast<int32_t>(temp);
            return true;
        }
    }
    return false;
}

void
OzoAudioInitStagefright(struct OzoAudioParamsStagefright &params)
{
    params.encoding_mode = ozoaudioencoder::OZOSDK_OZOAUDIO_ID;
    params.device_id = ""; // Empty Ozo device ID => OZO Audio disabled
    params.num_input_channels = 2;
    params.wnr_notification = OZO_FEATURE_UNINITIALIZED;
    params.ssloc_notification = OZO_FEATURE_UNINITIALIZED;
    params.levels_notification = OZO_FEATURE_UNINITIALIZED;
    params.micblocking_notification = OZO_FEATURE_UNINITIALIZED;
    params.channel_mask = -1;
}

int32_t
OzoAudioInitMessageStagefright(const struct OzoAudioParamsStagefright &params, bool isTune,
    sp<AMessage> &format, sp<MetaData> &micFormat, int32_t samplerate)
{
    format->setString("mime", MEDIA_MIMETYPE_AUDIO_OZOAUDIO);
    format->setString(kOzoDeviceIdTag, (const char *) params.device_id);
    format->setString(kOzoEncodingModeTag, (const char *) params.encoding_mode);
    format->setString(kOzoWnrNotificationTag, (const char *) params.wnr_notification);
    format->setString(kOzoSSLocNotificationTag, (const char *) params.ssloc_notification);
    format->setString(kOzoAudioLevelsNotificationTag, (const char *) params.levels_notification);
    format->setString(kOzoMicBlockingNotificationTag, (const char *) params.micblocking_notification);
    format->setString(kOzoGenericParamTag, (const char *) params.mParams.c_str());

    // Enable Ozo codec post capture data file creation
    if (isTune)
        format->setInt32(kOzoTuneTag, 1);

    auto channels = params.num_input_channels;
    micFormat->findInt32(kKeyChannelCount, &channels);

    int32_t bitrate = 0;
    micFormat->findInt32(kKeyBitRate, &bitrate);

    int32_t bitsPerSample = (bitrate / samplerate) / channels;
    if (bitsPerSample == 0)
        bitsPerSample = 16;

    format->setInt32(kOzoSampleDepthTag, bitsPerSample);

    return channels;
}

bool
OzoAudioSetParameterStagefright(struct OzoAudioParamsStagefright &params, bool &ozoFileSourceEnable,
    const String8 &key, const String8 &value)
{
    if (key == kOzoDeviceIdTag) {
        params.device_id = value;
        return true;
    } else if (key == kOzoDeviceInputChannelsTag) {
        int32_t numInputChannels;
        if (safe_strtoi32(value.string(), &numInputChannels)) {
            params.num_input_channels = numInputChannels;
            return true;
        }
    } else if (key == kOzoChannelMaskTag) {
        int64_t mask;
        if (safe_strtoi64(value.string(), &mask)) {
            ALOGD("%s: %x", key.c_str(), (uint32_t) mask);
            params.channel_mask = mask;
            return true;
        }
    } else if (key == kOzoFileSourceEnableTag) {
        int32_t _ozoFileSourceEnable;
        if (safe_strtoi32(value.string(), &_ozoFileSourceEnable)) {
            ozoFileSourceEnable = _ozoFileSourceEnable == 1;
            return true;
        }
    } else if (key == kOzoEncodingModeTag) {
        params.encoding_mode = value;
        return true;
    } else if (key == kOzoWnrNotificationTag) {
        params.wnr_notification = value;
        return true;
    } else if (key == kOzoAudioLevelsNotificationTag) {
        params.levels_notification = value;
        return true;
    } else if (key == kOzoMicBlockingNotificationTag) {
        params.micblocking_notification = value;
        return true;
    } else if (key == kOzoGenericParamTag) {
        String8 str = value + ";";
        params.mParams += str;
        return true;
    } else if (key == kOzoSSLocNotificationTag) {
        params.ssloc_notification = value;
        return true;
    }

    return false;
}

bool
OzoAudioNeedListenerStagefright(const struct OzoAudioParamsStagefright &params)
{
    if (params.wnr_notification == OZO_FEATURE_ENABLED ||
        params.ssloc_notification == OZO_FEATURE_ENABLED ||
        params.levels_notification == OZO_FEATURE_ENABLED ||
        params.micblocking_notification == OZO_FEATURE_ENABLED)
        return true;

    return false;
}

}  // namespace android
