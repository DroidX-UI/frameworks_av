/*
 * Copyright 2018 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "vendor.ozoaudio.media.c2@1.0-service"

#include <codec2/hidl/1.0/ComponentStore.h>
#include <hidl/HidlTransportSupport.h>
#include <binder/ProcessState.h>
#include <minijail.h>

#include "OzoC2VendorSupport.h"

// Base policy path
static constexpr char kBaseSeccompPolicyPath[] =
    "/vendor/etc/seccomp_policy/"
    "vendor.ozoaudio.media.c2@1.0-default-seccomp_policy";

// Additional device-specific seccomp permissions can be added in this file.
static constexpr char kExtSeccompPolicyPath[] =
    "/vendor/etc/seccomp_policy/"
    "vendor.ozoaudio.media.c2@1.0-extended-seccomp-policy";


int main(int /* argc */, char ** /* argv */)
{
    ALOGD("ozoaudio Codec2 service starting...");

    signal(SIGPIPE, SIG_IGN);
    android::SetUpMinijail(kBaseSeccompPolicyPath, kExtSeccompPolicyPath);

    // vndbinder is needed by BufferQueue.
    android::ProcessState::initWithDriver("/dev/vndbinder");
    android::ProcessState::self()->startThreadPool();

    // Extra threads may be needed to handle a stacked IPC sequence that
    // contains alternating binder and hwbinder calls. (See b/35283480.)
    android::hardware::configureRpcThreadpool(8, true /* callerWillJoin */);

    {
        using namespace ::android::hardware::media::c2::V1_0;
        android::sp<IComponentStore> store;

        // Create IComponentStore service.
        store = new utils::ComponentStore(android::GetOzoAudioCodec2VendorComponentStore());
        if (store->registerAsService("ozoaudio") != android::OK) {
            ALOGE("Cannot create ozoaudio Codec2 service");
        }
        else {
            ALOGD("ozoaudio Codec2 service created");
        }
    }

    android::hardware::joinRpcThreadpool();

    return 0;
}
