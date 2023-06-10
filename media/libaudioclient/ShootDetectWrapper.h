/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef ANDROID_SHOOTDETECT_WRAPPER_H
#define ANDROID_SHOOTDETECT_WRAPPER_H

#include <system/audio.h>
#include <utils/String16.h>
#include <utils/RefBase.h>

#include <shoot_detect.h>

namespace android {

class ShootDetectWrapper
{
public:
    ShootDetectWrapper();
    ~ShootDetectWrapper();

    bool mShootDetectEnabled;

    status_t initShootDetectLibAndVibrator(audio_format_t format, uint32_t  channelCount, uint32_t sampleRate, unsigned int appID);
    void releaseShootDetectLibAndVibrator();
    void ProcessShootDetectLibAndVibrator(DTYPE* in_buf, uint32_t num_samples);

private:
    void* mShootDetectHdl; // lib handle for ShootDetect
};

}; // namespace android

#endif  // ANDROID_SHOOTDETECT_WRAPPER_H

