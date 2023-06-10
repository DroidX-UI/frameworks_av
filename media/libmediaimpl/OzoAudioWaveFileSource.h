#pragma once

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

#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>

#include "wavformat.h"

namespace android {

class MediaBufferGroup;

struct OzoAudioWaveFileSource : public MediaSource, public MediaBufferObserver {
    OzoAudioWaveFileSource(const char *file_name);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(MediaBufferBase **out, const ReadOptions *options = NULL);

    status_t initCheck() const
    {
        return OK;
    }

    int16_t getMaxAmplitude()
    {
        return 0;
    }

    status_t dataCallback(const AudioRecord::Buffer& /*buffer*/)
    {
        return OK;
    }

    virtual void signalBufferReturned(MediaBufferBase * /*buffer*/)
    {
        return;
    }

    enum {
        kKeyBufferSize = 'abcd'
    };

protected:
    virtual ~OzoAudioWaveFileSource();

    bool open();

private:
    enum { kBufferSize =  798 };

    bool mOpened;
    bool mStarted;
    wav_t mWavData;
    size_t mFrameSize;
    int32_t mBufferSize;
    int64_t mCurrentTime;
    const char *mFilename;
    MediaBufferGroup *mGroup;
};

}  // android
