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
 * @brief OZO Audio license reader as data source.
 */

#include <stdint.h>
#include <stddef.h>
#include <new>        // std::nothrow

#include "ozoaudiocommon/ozo_audio_common.h"

namespace ozoaudiocodec {

/**
 * Read OZO license content from file. Data is read from file to a buffer upon request.
 */
class OzoLicenseReaderAsFile: public DataReader
{
public:
    OzoLicenseReaderAsFile()
    {
        this->m_totalLen = 0;
        this->m_buffer = nullptr;
        this->m_fp = nullptr;

        // Internal buffer size, this amount of data is read upon each read request
        this->m_fixedLen = 512 * 1024; // 500kB
    }

    ~OzoLicenseReaderAsFile()
    {
        if (this->m_fp)
            fclose(this->m_fp);
        delete[] this->m_buffer;
    }

    /**
     * Initialize by reading license data from specified file.
     *
     * @param file License file name.
     * @return true on success, false otherwise.
     */
    bool init(const char *file)
    {
        this->m_fp = fopen(file, "rb");

        if (this->m_fp) {
            // License size
            fseek(this->m_fp, 0, SEEK_END);
            long fileLength = ftell(this->m_fp);
            if (fileLength <= 0) {
                fclose(this->m_fp);
                this->m_fp = nullptr;
                return false;
            }
            this->m_totalLen = static_cast<size_t>(fileLength);
            fseek(this->m_fp, 0, SEEK_SET);

            this->m_buffer = new (std::nothrow) uint8_t[this->m_fixedLen];
            if (this->m_buffer == nullptr)
                return false;
        }

        return true;
    }

    virtual const uint8_t *read(size_t &size) override
    {
        // Override the request size with internal size
        size = this->m_fixedLen;
        size = fread(this->m_buffer, 1, size, this->m_fp);
        return this->m_buffer;
    }

    // Not supported
    virtual size_t read(uint8_t *, size_t) override
    {
        return 0;
    }

    // Not supported
    virtual ozoaudiocodec::AudioCodecError readLine(const char **) override
    {
        return ozoaudiocodec::AudioCodecError::PROCESSING_ERROR;
    }

    // Not supported
    virtual ozoaudiocodec::AudioCodecError seek(size_t) override
    {
        return ozoaudiocodec::AudioCodecError::PROCESSING_ERROR;
    }

    virtual size_t size() const override
    {
        return this->m_totalLen;
    }

protected:
    FILE *m_fp;
    size_t m_totalLen;
    size_t m_fixedLen;
    uint8_t *m_buffer;
};

} // namespace ozoaudiocodec
