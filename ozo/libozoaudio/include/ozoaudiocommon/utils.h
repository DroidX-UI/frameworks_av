#pragma once
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

/**
 * @file
 * @brief Helper utilities.
 */

#include <stdint.h>
#include <string.h>

namespace ozoaudiocodec {

template <typename T>
class OzoRingBufferT
{
public:
    OzoRingBufferT(size_t len) : mStart(0), mLen(0), mSize(len), mBufData(new T[len])
    {
        memset(mBufData, 0, sizeof(T) * len);
    }

    ~OzoRingBufferT()
    {
        delete[] this->mBufData;
    }

    void write(size_t len)
    {
        this->mLen += len;
    }

    void write(T *data, size_t len)
    {
        auto curr_end = this->mStart + this->mLen;
        if (curr_end > this->mSize)
            curr_end -= this->mSize;

        if (curr_end + len < this->mSize)
            memcpy(this->mBufData + curr_end, data, len * sizeof(T));
        else {
            auto copy_len = this->mSize - curr_end;
            memcpy(this->mBufData + curr_end, data, copy_len * sizeof(T));
            memcpy(this->mBufData, data + copy_len, (len - copy_len) * sizeof(T));
        }

        this->mLen += len;
    }

    void read(T *data, size_t len)
    {
        if (this->mStart + len < this->mSize) {
            memcpy(data, this->mBufData + this->mStart, len * sizeof(T));
            this->mStart += len;
        }
        else {
            auto copy_len = this->mSize - this->mStart;
            memcpy(data, this->mBufData + this->mStart, copy_len * sizeof(T));
            memcpy(data + copy_len, this->mBufData, (len - copy_len) * sizeof(T));
            this->mStart = len - copy_len;
        }

        this->mLen -= len;
    }

    size_t samplesAvailable() const
    {
        return this->mLen;
    }

protected:
    size_t mStart;
    size_t mLen;
    size_t mSize;
    T *mBufData;
};

} // namespace ozoaudiocodec
