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
 * @brief Helpers for post-processing integration to target platform.
 */

#include <stdio.h>
#include <string.h>
#include <vector>

#include "ozoaudioencoder/post_writer.h"

namespace ozoaudioencoder {

/**
 * File writer and reader implementation using C file I/O interfaces.
 */
class DefaultFileWriterReader : public FileWriterReader
{
public:
    DefaultFileWriterReader(FILE *fp) : m_fp(fp) {}
    virtual ~DefaultFileWriterReader()
    {
        this->close();
    }

    virtual size_t write(const void *data, size_t size) final
    {
        return fwrite(data, 1, size, this->m_fp);
    }

    virtual size_t read(void *data, size_t size) final
    {
        return fread(data, 1, size, this->m_fp);
    }

    virtual bool seek(size_t offset, int whence = SEEK_SET) final
    {
        return !fseek(this->m_fp, (long)offset, whence) ? true : false;
    }

    virtual size_t readLine(char *data, size_t len) const final
    {
        if (fgets(data, (int)len, m_fp) != NULL)
            return strlen(data);

        return 0;
    }

    virtual size_t position() const final
    {
        return (size_t)ftell(this->m_fp);
    }

    virtual void close() final
    {
        if (this->m_fp)
            fclose(this->m_fp);
        this->m_fp = NULL;
    }

protected:
    FILE *m_fp;
};

// Base post capture observer interface for receiving data from encoder
class BasePostProcessingWriter : public ozoaudioencoder::OzoEncoderObserver
{
public:
    virtual bool process(const void *indata, size_t size)
    {
        const uint8_t *data = (const uint8_t *)indata;
        for (size_t i = 0; i < size; i++)
            this->m_data.push_back(data[i]);

        return true;
    }

    const uint8_t *data() const
    {
        return this->m_data.data();
    }

    size_t size() const
    {
        return this->m_data.size();
    }

    void clear()
    {
        if (this->m_canClear)
            this->m_data.clear();
    }

    void setClear(bool status)
    {
        this->m_canClear = status;
    }

protected:
    BasePostProcessingWriter(size_t size) : m_canClear(false)
    {
        this->m_data.reserve(size);
    }

    virtual ~BasePostProcessingWriter() {}

    bool m_canClear;
    std::vector<uint8_t> m_data;
};

// Observer interface for receiving mic data from encoder
class RawMicWriterBuffered : public BasePostProcessingWriter
{
public:
    // By default assume audio input size consists of following
    // - Frame size: 1024
    // - Number of input mics: 3
    // - 16-bit input sample depth (2 bytes)
    // - Holds audio data for 4 frames
    RawMicWriterBuffered(size_t size = 1024 * 3 * 2 * 4) : BasePostProcessingWriter(size) {}

    virtual ozoaudioencoder::ObserverMode getMode() const
    {
        return ozoaudioencoder::ObserverMode::MICROPHONE;
    }
};

// Observer interface for receiving control data from encoder
class CtrlWriterBuffered : public BasePostProcessingWriter
{
public:
    // Default control buffer size is assumed to be 2048 bytes
    CtrlWriterBuffered(size_t size = 2048) : BasePostProcessingWriter(size) {}

    virtual ozoaudioencoder::ObserverMode getMode() const
    {
        return ozoaudioencoder::ObserverMode::CONTROL;
    }
};

} // namespace ozoaudioencoder
