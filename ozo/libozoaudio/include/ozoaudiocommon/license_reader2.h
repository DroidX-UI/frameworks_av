#pragma once
/*
  Copyright (C) 2020 Nokia Corporation.
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
 * @brief OZO Audio license reader as buffer.
 */

#include <stdint.h>
#include <stddef.h>

namespace ozoaudiocodec {

/**
 * Read OZO license content from file. All data is read from file into a buffer.
 */
class OzoLicenseReader
{
public:
    OzoLicenseReader()
    {
        this->m_len = 0;
        this->m_buffer = nullptr;
    }

    ~OzoLicenseReader()
    {
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
        bool status = false;
        FILE *fp = fopen(file, "rb");

        if (fp) {
            fseek(fp, 0, SEEK_END);
            long fileLength = ftell(fp);
            if (fileLength <= 0) {
                fclose(fp);
                return false;
            }
            this->m_len = static_cast<size_t>(fileLength);
            fseek(fp, 0, SEEK_SET);

            auto size = this->m_len + 1;
            this->m_buffer = new char[size];
            this->m_buffer[size - 1] = '\0';

            if (fread(this->m_buffer, 1, this->m_len, fp) == this->m_len)
                status = true;

            fclose(fp);
        }

        return status;
    }

    /**
     * Return license data size (in bytes).
     */
    size_t getSize() const
    {
        return this->m_len;
    }

    /**
     * Return license data.
     */
    const char *getBuffer() const
    {
        return this->m_buffer;
    }

protected:
    size_t m_len;
    char *m_buffer;
};

} // namespace ozoaudiocodec
