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

#ifndef OZOCODEC_EVENTLISTENER_H_
#define OZOCODEC_EVENTLISTENER_H_

#include <media/stagefright/IMediaCodecEvent.h>

namespace ozoaudioencoder {
class PostCaptureFileWriter;
}

namespace android {

class IMediaRecorderClient;
class MediaBufferBase;
class MediaCodecBuffer;
class FileWriter;

// Handle Ozo audio codec events
class OzoCodecEventListener: public IMediaCodecEventListener
{
public:
    OzoCodecEventListener(const sp<IMediaRecorderClient> &listener);
    virtual ~OzoCodecEventListener();

    virtual bool notify(sp<MediaCodecBuffer> &src, MediaBufferBase *dest) override;
    virtual bool notify(MediaBufferBase *) override;

    DISALLOW_EVIL_CONSTRUCTORS(OzoCodecEventListener);

private:
    sp<IMediaRecorderClient> mListener;
};

// Handle Ozo audio codec post-processing data writing
class OzoCodecPostDataWriter: public IMediaCodecEventListener
{
public:
    OzoCodecPostDataWriter();
    virtual ~OzoCodecPostDataWriter();

    bool init(int outputFd);

    virtual bool notify(sp<MediaCodecBuffer> &src, MediaBufferBase *dest) override;
    virtual bool notify(MediaBufferBase *buf) override;

    DISALLOW_EVIL_CONSTRUCTORS(OzoCodecPostDataWriter);

private:
    void *mSharedLib;
    FileWriter *mFp;
    ozoaudioencoder::PostCaptureFileWriter *mWriter;
};

}  // namespace android

#endif // OZOCODEC_EVENTLISTENER_H_
