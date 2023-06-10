/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <stdio.h>
#include "SensorThread.h"

namespace android {

static const int SENSOR_TYPE_FOLD_STATE = 33171087;

/**
 * Base class destructors
 */
bool SensorThread::threadLoop()
{
    ASensorEvent event[16];
    ssize_t n;
    uint8_t cfa;

    mSensorEventQueue->waitForEvent();
    n = mSensorEventQueue->read(event, 16);
    if (n > 0) {
        mSensorEventQueue->sendAck(event, n);
        for (int i = 0; i < n; ++i) {
            //1:floder 0: unfloder
            if (event[i].type == SENSOR_TYPE_FOLD_STATE) {
                if (event[i].data[0] == 1) {
                    cfa = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
                } else {
                   cfa = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR;
                }
                ALOGD("%s: mRWLock start locking, cfa:%d" ,__FUNCTION__,cfa);
                pthread_rwlock_wrlock(mRWLock);
                mCameraCharacteristics->update(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT, &cfa, 1);
                m_gCameraCharacteristics->update(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT, &cfa, 1);
                pthread_rwlock_unlock(mRWLock);
                ALOGD("%s: mRWLock complete unlock, cfa:%d" ,__FUNCTION__,cfa);
                break;
            }
        }
    }
    return true;
}

bool SensorFolderRegister::Init(CameraMetadata *cameraCharacteristics, CameraMetadata *gCameraCharacteristics, pthread_rwlock_t *rwLock)
{
    if (cameraCharacteristics == nullptr) {
        return false;
    }
    static android::SensorManager * smgr =
        &SensorManager::getInstanceForPackage(String16("SensorQueryOfCamera"));
    if (smgr == nullptr) {
        ALOGI("%s, sensor manager is NULL",  __FUNCTION__);
        return false;
    }
    mFolderSensor = smgr->getDefaultSensor(SENSOR_TYPE_FOLD_STATE);
    if (mFolderSensor == nullptr) {
       ALOGI("%s, folder manager is NULL", __FUNCTION__);
       return false;
    }
    android::SensorManager & mgr = *smgr;
    mSensorEventQueue = mgr.createEventQueue();
    if (mSensorEventQueue == nullptr) {
        ALOGI("%s, mSensorEventQueue is NULL", __FUNCTION__);
        return false;
    }
    mSensorThread = new SensorThread(mSensorEventQueue, cameraCharacteristics, gCameraCharacteristics, rwLock);
    if (mSensorThread == nullptr) {
        ALOGI("%s, mSensorThread is NULL", __FUNCTION__);
        return false;
    }
    mSensorThread->run("sensor-loop", PRIORITY_BACKGROUND);
    mSensorEventQueue->enableSensor(mFolderSensor);
    return true;
}

SensorFolderRegister::~SensorFolderRegister()
{
    if ((mSensorEventQueue != nullptr) && (mFolderSensor != nullptr)) {
        mSensorEventQueue->disableSensor(mFolderSensor);
    }
}

}
