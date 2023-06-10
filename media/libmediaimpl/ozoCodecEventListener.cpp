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

//#define LOG_NDEBUG 0
#define LOG_TAG "OzoCodecEventListener"
#include <log/log.h>
#include <utils/Log.h>
#include <unistd.h>
#include <dlfcn.h>

#include <media/IMediaRecorderClient.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaBufferBase.h>
#include <media/stagefright/MetaDataBase.h>
#include <media/MediaCodecBuffer.h>

#include "ozoCodecEventListener.h"
#include "ozoaudioenc/event_parser.h"
#include "ozoaudio/utils.h"
#include "ozoaudioencoder/post_writer.h"

// Location of OZO Tune file writing interface library
#if defined(__arm__)
#define OZOPATH "/system_ext/lib/libozoaudioutils.so"
#else // defined(__aarch64__)
#define OZOPATH "/system_ext/lib64/libozoaudioutils.so"
#endif

namespace android {

enum {
    // Mic data is stored under this key within metadata
    kKeyMicData    = 'micd',

    // Control data is stored under this key within metadata
    kKeyCtrlData   = 'ctrl',

    // Events data is stored under this key within metadata
    kKeyEventsData = 'evts',
};

static bool
parseOzoAudioEncData(sp<MediaCodecBuffer> &src, MediaBufferBase *dest)
{
    auto postdata = (uint8_t *) src->data();
    uint8_t *mic_data, *ctrl_data, *events_data;
    size_t encsize, micsize, ctrlsize, size_unit;

    auto eventsDataSize = OzoAudioUtilities::depacketize_data(
        postdata, encsize, micsize, ctrlsize, size_unit,
        &mic_data, &ctrl_data, &events_data
    );

    // Include mic and control data as meta data to the encoded audio data
    dest->meta_data().setData(kKeyMicData, 0, mic_data, micsize);
    dest->meta_data().setData(kKeyCtrlData, 0, ctrl_data, ctrlsize);

    // Include events data
    if (eventsDataSize)
        dest->meta_data().setData(kKeyEventsData, 0, events_data, eventsDataSize);

    // Valid encoded audio data is available after specified offset.
    // The encoded audio is eventually copied to another buffer so make
    // sure it is accessed from right position.
    src->setRange(size_unit, encsize);

    // The buffer was originally created based on packetized data size which
    // is incorrect for the encoded audio only. Thus, adjust the offset
    // so that subsequent access to the data always points to the encoded
    // audio data section.
    dest->set_range(0, encsize);

    return true;
}

OzoCodecEventListener::OzoCodecEventListener(const sp<IMediaRecorderClient> &listener):
mListener(listener)
{}

OzoCodecEventListener::~OzoCodecEventListener()
{}

bool
OzoCodecEventListener::notify(MediaBufferBase *buf)
{
    uint32_t type;
    const void *data;
    size_t events_size = 0;

    if (!mListener.get())
        return true;

    // Read events data buffer
    if (buf->meta_data().findData(kKeyEventsData, &type, &data, &events_size) && events_size) {
        size_t nEvents;
        auto ptr = OzoAudioUtilities::get_events_count((const uint8_t *) data, nEvents);

        // Notify events via listener
        for (size_t i = 0; i < nEvents; i++) {
            // Extract next event
            OzoAudioUtilities::EventData event;
            ptr = OzoAudioUtilities::get_next_event_data(ptr, event);

            // Send event data
            int msg, ext1, ext2;
            auto ozoEvent = OzoEventParser(event);
            for (int i = 0; i < ozoEvent.getEventsCount(); i++)
                if (ozoEvent.getNotifyData(msg, ext1, ext2, i))
                    mListener->notify(msg, ext1, ext2);
        }
    }

    return true;
}

bool
OzoCodecEventListener::notify(sp<MediaCodecBuffer> &src, MediaBufferBase *dest)
{
    return parseOzoAudioEncData(src, dest);
}


class FileWriter: public ozoaudioencoder::FileWriterReader
{
public:
    FileWriter(int fd):
    m_fd(fd)
    {}

    virtual ~FileWriter() final {}

    virtual size_t write(const void *data, size_t size) final
    {
        auto result = ::write(this->m_fd, data, size);
        return result != -1 ? result : 0;
    }

    virtual size_t read(void *, size_t) final
    {
        return 0;
    }

    virtual size_t readLine(char *, size_t) const final
    {
        return 0;
    }

    virtual bool seek(size_t offset, int whence) final
    {
        auto result = ::lseek(this->m_fd, (off_t) offset, whence);
        return result != -1 ? true : false;
    }

    virtual size_t position() const final
    {
        return 0;
    }

    virtual void close() final
    {
        if (this->m_fd)
            ::close(this->m_fd);
        this->m_fd = -1;
    }

protected:
    int m_fd;
};


OzoCodecPostDataWriter::OzoCodecPostDataWriter():
mSharedLib(nullptr),
mFp(nullptr),
mWriter(nullptr)
{}

OzoCodecPostDataWriter::~OzoCodecPostDataWriter()
{
    if (this->mWriter)
        this->mWriter->destroy();
    this->mWriter = nullptr;

    delete this->mFp;
    this->mFp = nullptr;

    if (this->mSharedLib)
        dlclose(this->mSharedLib);
    this->mSharedLib = nullptr;
}

bool
OzoCodecPostDataWriter::init(int outputFd)
{
    if (outputFd < 0) {
        ALOGE("Invalid file descriptor: %d", outputFd);
        return false;
    }

    // Start with a clean, empty file
    auto result = ftruncate(outputFd, 0);
    if (result == -1) {
        ALOGE("Unable to clean file: %d", outputFd);
        return false;
    }

    auto fd = dup(outputFd);
    if (fd == -1) {
        ALOGE("Unable to dup() file descriptor: %d", outputFd);
        return false;
    }

    this->mFp = new (std::nothrow) FileWriter(fd);
    if (!this->mFp) {
        ALOGE("Unable to create file writer object");
        return false;
    }

    this->mSharedLib = dlopen(OZOPATH, RTLD_NOW);
    ALOGD("Open libozoaudioutils library = %p (%s)", this->mSharedLib, OZOPATH);
    if (this->mSharedLib == NULL) {
        ALOGE("Could not open OZO Tune library: %s", dlerror());
        return false;
    }

    // Extract create interface
    ozoaudioencoder::PostCaptureFileWriter * (*createTuneFile)();
    createTuneFile = (ozoaudioencoder::PostCaptureFileWriter * (*)()) dlsym(this->mSharedLib, "_ZN15ozoaudioencoder21PostCaptureFileWriter6createEv");
    if (!createTuneFile) {
        ALOGE("Unable to get OZO Tune factory function (create)");
        dlclose(this->mSharedLib);
        this->mSharedLib = nullptr;
        return false;
    }

    this->mWriter = createTuneFile();
    if (!this->mWriter) {
        ALOGE("Unable to create post capture file writer");
        dlclose(this->mSharedLib);
        this->mSharedLib = nullptr;
        return false;
    }

    return this->mWriter->init(this->mFp);
}

bool
OzoCodecPostDataWriter::notify(sp<MediaCodecBuffer> &src, MediaBufferBase *dest)
{
    return parseOzoAudioEncData(src, dest);
}

bool
OzoCodecPostDataWriter::notify(MediaBufferBase *buf)
{
    uint32_t type;
    const void *data;
    size_t mic_size = 0, ctrl_size = 0;

    // Write mic data, if present
    if (buf->meta_data().findData(kKeyMicData, &type, &data, &mic_size) && mic_size)
        this->mWriter->getMicWriter()->process((const uint8_t *) data, mic_size);

    // Write control data, if present
    if (buf->meta_data().findData(kKeyCtrlData, &type, &data, &ctrl_size) && ctrl_size)
        this->mWriter->getCtrlWriter()->process((const uint8_t *) data, ctrl_size);

    ALOGD("Post data sizes - mic: %zu, ctrl: %zu", mic_size, ctrl_size);

    return true;
}

}  // namespace android
