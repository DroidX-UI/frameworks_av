/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef SENSORTHREAD_H
#define SENSORTHREAD_H

#include <utils/Thread.h>
#include <sensor/Sensor.h>
#include <sensor/SensorManager.h>
#include <sensor/SensorEventQueue.h>
#include "camera/CameraMetadata.h"

namespace android {
/**
 * The SensorThread descriptor class that revice floder sensor status
 */
class SensorThread : public Thread {
public:
    SensorThread(sp < SensorEventQueue > &queue, CameraMetadata *cameraCharacteristics, CameraMetadata *gCameraCharacteristics, pthread_rwlock_t *rwLock)
    :mSensorEventQueue(queue)
    {
        mCameraCharacteristics = cameraCharacteristics;
        m_gCameraCharacteristics = gCameraCharacteristics;
        mRWLock = rwLock;
    }
    ~SensorThread() {}
private:
    virtual bool threadLoop();
    CameraMetadata *mCameraCharacteristics;
    CameraMetadata *m_gCameraCharacteristics;
    sp < SensorEventQueue > mSensorEventQueue;
    pthread_rwlock_t *mRWLock;
};


/**
 * The Sensor descriptor class that revice floder sensor status
 */
class SensorFolderRegister {
public:
    SensorFolderRegister() {
        mSensorThread = nullptr;
        mSensorEventQueue = nullptr;
        mFolderSensor = nullptr;
    }
    virtual ~SensorFolderRegister();
    bool Init(CameraMetadata *cameraCharacteristics, CameraMetadata *gCameraCharacteristics, pthread_rwlock_t *rwLock);
private:
    sp < SensorThread > mSensorThread;
    sp < SensorEventQueue > mSensorEventQueue;
    const Sensor *mFolderSensor;
};
}
#endif
