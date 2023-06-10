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

#ifndef OZO_AUDIO_FILE_SOURCE_H_

#define OZO_AUDIO_FILE_SOURCE_H_

#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <utils/List.h>

#include <system/audio.h>

namespace android {

class AudioRecord;

struct OzoAudioFileSource : public MediaSource, public MediaBufferObserver {

    OzoAudioFileSource(
            audio_source_t inputSource,
            const content::AttributionSourceState& clientIdentity,
	    uint32_t sampleRate,
            uint32_t channels,
            uint32_t outSampleRate = 0,
            audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE,
            int64_t channelMask = -1);

    status_t initCheck() const;

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop() { return reset(); }
    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBufferBase **buffer, const ReadOptions *options = NULL);
    virtual status_t setStopTimeUs(int64_t stopTimeUs);

    status_t dataCallback(const AudioRecord::Buffer& buffer);
    virtual void signalBufferReturned(MediaBufferBase *buffer);

protected:
    virtual ~OzoAudioFileSource();

private:

    status_t reset();

private:

    char mFileName[1024];
    sp<MediaSource> mWavSource;

    Mutex mLock;

    status_t mInitCheck;
    bool mStarted;
    int32_t mSampleRate;
    int32_t mOutSampleRate;

    int64_t mPrevSampleTimeUs;
    int64_t mNumFramesReceived;
    int64_t mNumClientOwnedBuffers;

    OzoAudioFileSource(const OzoAudioFileSource &);
    OzoAudioFileSource &operator=(const OzoAudioFileSource &);
};

}  // namespace android

#endif  // OZO_AUDIO_FILE_SOURCE_H_
