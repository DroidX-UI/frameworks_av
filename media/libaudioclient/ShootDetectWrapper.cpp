/*
**
** Copyright 2010, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "ShootDetect"
#define LOG_NDDEBUG 0

#include <stdint.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>

#include <utils/Log.h>
#include <cutils/properties.h>
#include <media/AudioSystem.h>
#include <media/AudioParameter.h>

#include <ShootDetectWrapper.h>


namespace android {

// ---------------------------------------------------------------------------

ShootDetectWrapper::ShootDetectWrapper()
    : mShootDetectEnabled(false), mShootDetectHdl(nullptr)
{
}

ShootDetectWrapper::~ShootDetectWrapper()
{
}

status_t ShootDetectWrapper::initShootDetectLibAndVibrator(audio_format_t format, uint32_t  channelCount, uint32_t sampleRate, unsigned int appID)
{
    status_t status = NO_ERROR;

    ALOGD("Enter %s, NumChannels:%u, Samplerate:%u, GameType:%u", __func__, channelCount, sampleRate, appID);

    if (format != AUDIO_FORMAT_PCM_16_BIT) {
        ALOGE("Unsupport Format:0x%x, Shoot Detect Disabled.", format);
        return BAD_VALUE;
    }

    status = AudioSystem::setParameters(String8("vibrator_switch=on"));
    if (status != NO_ERROR) {
        ALOGE("vibratorInit failed in %s", __func__);
        return BAD_VALUE;
    }

    if (mShootDetectHdl == nullptr) {
        uint32_t shootDetectLibSize =  get_shoot_detect_str_size();
        mShootDetectHdl = (void *)malloc(shootDetectLibSize);
        if (mShootDetectHdl == nullptr) {
            ALOGE("malloc failed in %s", __func__);
            status = AudioSystem::setParameters(String8("vibrator_switch=off"));
            return BAD_VALUE;
        }
        shoot_detect_init(mShootDetectHdl, channelCount, sampleRate, appID);
        mShootDetectEnabled = true;
    }


    ALOGD("ShootDetect Inited Success");
    return status;
}

void ShootDetectWrapper::releaseShootDetectLibAndVibrator()
{
        ALOGD("Enter %s", __func__);

    if (mShootDetectEnabled) {
        if (mShootDetectHdl != nullptr) {
            shoot_detect_release(mShootDetectHdl);
            free(mShootDetectHdl);
            mShootDetectHdl = nullptr;
        }
        AudioSystem::setParameters(String8("vibrator_switch=off"));
        mShootDetectEnabled = false;
    }
}

void ShootDetectWrapper::ProcessShootDetectLibAndVibrator(DTYPE* in_buf, uint32_t num_samples)
{
    int detectResult = 1;

    if (in_buf == nullptr){
        ALOGE("Invalid parameter in %s", __func__);
        return;
    }
    if (mShootDetectHdl != nullptr) {
        detectResult = shoot_detect_process(mShootDetectHdl,  in_buf, num_samples);
        if(detectResult > 0 ) {
            AudioParameter requestedParameters;
            requestedParameters.addInt(String8("vibrator_on_strength"), detectResult);
            AudioSystem::setParameters(requestedParameters.toString());
            ALOGV("Shoot Detected!level:%d, frlen:%u", detectResult, num_samples);
        } else if(detectResult < 0) {
            ALOGE("ShootDetect Failed!errno:%d", detectResult);
        }
    }
}

} // namespace android
