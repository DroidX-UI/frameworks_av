#pragma once
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

#include <utils/String8.h>

namespace android {

struct AMessage;
class MetaData;

struct OzoAudioParamsStagefright
{
    String8 device_id;
    String8 encoding_mode;
    int32_t num_input_channels;

    // Windscreen notification
    String8 wnr_notification;

    // Sound source localization notification
    String8 ssloc_notification;

    // Audio levels notification
    String8 levels_notification;

    // Mic blocking notification
    String8 micblocking_notification;

    // Generic codec parameters
    String8 mParams;

    // Channel mask when Ozo Audio used as preprocess effect
    int64_t channel_mask;
};

// Initialize OZO Audio parameters for StagefrightRecorder
void OzoAudioInitStagefright(struct OzoAudioParamsStagefright &params);

// Assign init parameters for Ozo encoder initialization
int32_t OzoAudioInitMessageStagefright(const struct OzoAudioParamsStagefright &params, bool isTune,
    sp<AMessage> &format, sp<MetaData> &micFormat, int32_t samplerate);

// Handle parameter assignment for StagefrightRecorder
bool OzoAudioSetParameterStagefright(
    struct OzoAudioParamsStagefright &params, bool &ozoFileSourceEnable, const String8 &key, const String8 &value);

// Ozo event listener needed for StagefrightRecorder
bool OzoAudioNeedListenerStagefright(const struct OzoAudioParamsStagefright &params);

}  // namespace android
