/*
  Copyright (C) 2015-2018 Nokia Corporation.
  This material, including documentation and any related
  computer programs, is protected by copyright controlled by
  Nokia Corporation. All rights are reserved. Copying,
  including reproducing, storing,  adapting or translating, any
  or all of this material requires the prior written consent of
  Nokia Corporation. This material also contains confidential
  information which may not be disclosed to others without the
  prior written consent of Nokia Corporation.
*/

/**
 * @file
 * @brief OZO Audio codec definitions. Includes error codes and data structures that are common to
 * both encoder and decoder.
 */

#ifndef OZO_AUDIO_COMMON_INTERFACE_H_
#define OZO_AUDIO_COMMON_INTERFACE_H_

#include <stddef.h>

#include "ozoaudiocommon/control.h"
#include "ozoaudiocommon/meta.h"
#include "ozoaudiocommon/library_export.h"

namespace ozoaudiocodec {

/**
 * Interface for abstracting access to a data source. Source can be file, buffer, etc.
 */
class DataReader
{
public:
    /**
     * Read data from object.
     *
     * @param [in/out] size Size of data to be read. Returns the size of data actually read.
     * @return Pointer to read data. The ownership of the pointer remains with the callee.
     */
    virtual const uint8_t *read(size_t &size) = 0;

    /**
     * Read data from object.
     *
     * @param [out] data Read data.
     * @param size Size of data to be read.
     * @return Number of items read.
     */
    virtual size_t read(uint8_t *data, size_t size) = 0;

    /**
     * Read newline terminated string from data source.
     *
     * @param [out] data String data.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual ozoaudiocodec::AudioCodecError readLine(const char **data) = 0;

    /**
     * Change data source read position.
     *
     * @param offset New position in bytes with respect to the start of the data source.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual ozoaudiocodec::AudioCodecError seek(size_t offset) = 0;

    /**
     * Return size of the data. The exact meaning of the size varies depending
     * on the file context. For example, for binary data, the size is measured in bytes.
     */
    virtual size_t size() const = 0;

protected:
    /** @cond PRIVATE */
    DataReader() {}
    virtual ~DataReader() {}
    /** @endcond */
};

/**
 * Logger levels.
 */
typedef enum
{
    OZO_DEBUG_LEVEL = 1,
    OZO_INFO_LEVEL = 2
} LoggerLevel;

/**
 * Base class for receiving logging information.
 */
class Logger
{
public:
    Logger() {}
    virtual ~Logger() {}

    /**
     * Receive logging data.
     *
     * @param level Logging level of the data.
     * @param msg Logging data.
     **/
    virtual void log(LoggerLevel /*level*/, const char * /*msg*/) = 0;
};

/**
 * Register encoder logger.
 *
 * @param logger Logger receiver.
 * @param levels Logging levels for the logger, multiple levels are allowed.
 * @see ozoaudiocodec::LoggerLevel for further information regarding the levels.
 * @return true on success, false otherwise.
 */
OZO_AUDIO_SDK_PUBLIC bool registerLogger(const Logger &logger, size_t levels);

/**
 * Unregister encoder logger.
 *
 * @param logger Logger receiver to be removed.
 * @return true on success, false otherwise.
 */
OZO_AUDIO_SDK_PUBLIC bool removeLogger(const Logger &logger);

/**
 * Set SDK license. The encoder or decoder will not work without the license assignment.
 * This should be done only once!
 *
 * @param reader SDK license data reader.
 * The following DataReader interface methods need to be implemented for the reader:
 *   - const uint8_t *read(size_t &size)
 *   - size_t size() const
 * The other methods can have dummy implementation.
 * @return true if license was accepted, false otherwise.
 */
OZO_AUDIO_SDK_PUBLIC bool setLicense(DataReader &reader);

} // namespace ozoaudiocodec

#endif // OZO_AUDIO_COMMON_INTERFACE_H_
