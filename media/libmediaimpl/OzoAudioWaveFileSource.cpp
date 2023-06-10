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

#define LOG_TAG "OzoAudioWaveFileSource"

#include <string.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>

#include "OzoAudioWaveFileSource.h"

namespace android {

#define IS_VALID_FILE(name) strcmp("com.android.camera2", name)

OzoAudioWaveFileSource::OzoAudioWaveFileSource(const char *file_name):
mOpened(false),
mStarted(false),
mFrameSize(0),
mBufferSize(0),
mCurrentTime(0),
mFilename(file_name),
mGroup(NULL)
{
}

OzoAudioWaveFileSource::~OzoAudioWaveFileSource() {
    if (mStarted) {
        stop();
    }
}

bool
OzoAudioWaveFileSource::open() {
    if (!this->mOpened) {
        if (IS_VALID_FILE(this->mFilename)) {
            ALOGD("Opening WAVE file: %s", this->mFilename);
            if (!wav_open(this->mFilename, &this->mWavData))
                return false;

            ALOGD("WAVE audio file details:");
            wav_info(&this->mWavData);
        }
        else {
            // No file available, create silence from 3 microphones @48kHz. 16-bit
            this->mWavData.fmt.nChannels = 3;
            this->mWavData.fmt.nSamplesPerSec = 48000;
            this->mWavData.fmt.wBitsPerSample = 16;

            ALOGD("Invalid WAVE file. Creating silence using %i channels @%uHz, %i-bit: ", this->mWavData.fmt.nChannels,
                this->mWavData.fmt.nSamplesPerSec, this->mWavData.fmt.wBitsPerSample );
        }

        this->mOpened = true;
    }

    return true;
}

status_t OzoAudioWaveFileSource::start(MetaData *params) {
    CHECK(!this->mStarted);

    if (!this->open())
        return NO_INIT;

    mBufferSize = kBufferSize;
    if (params) params->findInt32(kKeyBufferSize, &mBufferSize);
    this->mFrameSize = mBufferSize * this->mWavData.fmt.nChannels;

    this->mGroup = new MediaBufferGroup;
    if (!this->mGroup)
        return NO_MEMORY;

    MediaBuffer *buffer = new MediaBuffer(this->mFrameSize * (this->mWavData.fmt.wBitsPerSample/8));
    if (!buffer)
        return NO_MEMORY;

    this->mGroup->add_buffer(buffer);
    this->mStarted = true;

    return OK;
}

status_t OzoAudioWaveFileSource::stop() {
    if (!this->mStarted) {
        this->mStarted = false;
        this->mCurrentTime = 0;

        delete this->mGroup;
        this->mGroup = NULL;

        return OK;
    }

    return UNKNOWN_ERROR;
}

sp<MetaData> OzoAudioWaveFileSource::getFormat() {
    this->open();

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
    meta->setInt32(kKeyBufferSize, this->mBufferSize);
    meta->setInt32(kKeyChannelCount, this->mWavData.fmt.nChannels);
    meta->setInt32(kKeySampleRate, this->mWavData.fmt.nSamplesPerSec);
    meta->setInt32(kKeyMaxInputSize, this->mFrameSize * (this->mWavData.fmt.wBitsPerSample / 8));
    meta->setInt32(kKeyBitRate, this->mWavData.fmt.nChannels * this->mWavData.fmt.nSamplesPerSec * this->mWavData.fmt.wBitsPerSample);

    return meta;
}

status_t OzoAudioWaveFileSource::read(MediaBufferBase **out, const ReadOptions * /* options */) {
    *out = NULL;

    if (!this->mStarted)
        return NO_INIT;

    MediaBufferBase *buffer;
    status_t err = this->mGroup->acquire_buffer(&buffer);

    if (err != OK) {
        return err;
    }

    size_t sampleSizeBytes = this->mWavData.fmt.wBitsPerSample / 8;
    size_t n_samples = this->mFrameSize;
    if (IS_VALID_FILE(this->mFilename))
        n_samples = fread(buffer->data(), sampleSizeBytes, this->mFrameSize, this->mWavData.fp);
    else
        memset((uint8_t *) buffer->data(), 0, this->mFrameSize * sampleSizeBytes);

    if (n_samples == 0) {
        buffer->release();
        return ERROR_END_OF_STREAM;
    }

    if (n_samples < this->mFrameSize)
        memset(((uint8_t *) buffer->data()) + n_samples * sampleSizeBytes, 0, (this->mFrameSize - n_samples) * sampleSizeBytes);

    this->mCurrentTime += (int64_t) ((mBufferSize * 1000000L) / this->mWavData.fmt.nSamplesPerSec);
    buffer->meta_data().setInt64(kKeyTime, this->mCurrentTime);
    buffer->meta_data().setInt64(kKeyAnchorTime, 0);

    buffer->set_range(0, this->mFrameSize * sampleSizeBytes);

    *out = buffer;

    return OK;
}

}  // namespace android
