/*
  Copyright (C) 2018 Nokia Corporation.
  This material, including documentation and any related
  computer programs, is protected by copyright controlled by
  Nokia Corporation. All rights are reserved. Copying,
  including reproducing, storing,  adapting or translating, any
  or all of this material requires the prior written consent of
  Nokia Corporation. This material also contains confidential
  information which may not be disclosed to others without the
  prior written consent of Nokia Corporation.
*/

#include <inttypes.h>
#include <stdlib.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "OzoAudioFileSource"
#include <utils/Log.h>

#include "OzoAudioFileSource.h"

#include <media/AudioRecord.h>
#include <media/stagefright/AudioSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <cutils/properties.h>


#include "OzoAudioWaveFileSource.h"

namespace android {


OzoAudioFileSource::OzoAudioFileSource(
        audio_source_t /*inputSource*/, const android::content::AttributionSourceState& identity,
        uint32_t sampleRate, uint32_t channelCount, uint32_t outSampleRate,
        audio_port_handle_t /* selectedDeviceId */, int64_t /* channelMask */)
    : mStarted(false),
      mSampleRate(sampleRate),
      mOutSampleRate(outSampleRate > 0 ? outSampleRate : sampleRate),
      mPrevSampleTimeUs(0),
      mNumFramesReceived(0),
      mNumClientOwnedBuffers(0) {
    ALOGD("sampleRate: %u, outSampleRate: %u, channelCount: %u",
        sampleRate, outSampleRate, channelCount);
    CHECK(sampleRate > 0);

    const char *file_name = identity.packageName.value().c_str();
    strlcpy(mFileName, file_name, sizeof(mFileName));
    mWavSource = new OzoAudioWaveFileSource(mFileName);
    mInitCheck = OK;
}

OzoAudioFileSource::~OzoAudioFileSource() {
    if (mStarted) {
        reset();
    }
}

status_t OzoAudioFileSource::initCheck() const {
    return mInitCheck;
}

status_t OzoAudioFileSource::start(MetaData *params) {
    Mutex::Autolock autoLock(mLock);
    if (mStarted) {
        return UNKNOWN_ERROR;
    }

    mWavSource->start(params);
    mStarted = true;

    return OK;
}

status_t OzoAudioFileSource::reset() {
    Mutex::Autolock autoLock(mLock);
    if (!mStarted) {
        return UNKNOWN_ERROR;
    }

    return mWavSource->stop();
}

sp<MetaData> OzoAudioFileSource::getFormat() {
    Mutex::Autolock autoLock(mLock);
    return mWavSource->getFormat();
}

status_t OzoAudioFileSource::read(
        MediaBufferBase **out, const ReadOptions *options) {
    Mutex::Autolock autoLock(mLock);
    return mWavSource->read(out, options);
}

status_t OzoAudioFileSource::setStopTimeUs(int64_t /*stopTimeUs*/) {
    return OK;
}

void OzoAudioFileSource::signalBufferReturned(MediaBufferBase * /*buffer*/) {
}

status_t OzoAudioFileSource::dataCallback(const AudioRecord::Buffer& /*audioBuffer*/) {
    return OK;
}


}  // namespace android
