#pragma once
/*
  Copyright (C) 2018 Nokia Corporation.
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
 * @brief OZO Audio Tune writer interface.
 */

#include <stddef.h>
#include <stdint.h>

#include "ozoaudiocommon/ozo_audio_common.h"
#include "ozoaudiocommon/library_export.h"
#include "ozoaudioencoder/config.h"

namespace ozoaudioencoder {

/**
 * Interface for writing capture-time mic signals and related control data.
 */
class OZO_AUDIO_SDK_PUBLIC PostCaptureFileWriter
{
public:
    /** @name Construction and destruction of PostCaptureFileWriter */
    //@{
    /**
     * Create new file writer object.
     *
     * @return Pointer to PostCaptureFileWriter object.
     *
     * @code
     *
     * auto *writer = ozoaudioencoder::PostCaptureFileWriter::create();
     *
     * @endcode
     */
    static PostCaptureFileWriter *create();

    /**
     * Destroy file writer object.
     */
    virtual void destroy();
    //@}

    /**
     * Initialize writer using writer object.
     *
     * @param writer File writer object.
     * @return true on success, false otherwise.
     */
    virtual bool init(FileWriterReader *writer) = 0;

    /**
     * Request mic writer handle.
     *
     * @return Pointer to mic writer object.
     */
    virtual ozoaudioencoder::OzoEncoderObserver *getMicWriter() = 0;

    /**
     * Request control data writer handle.
     *
     * @return Pointer to control writer object.
     */
    virtual ozoaudioencoder::OzoEncoderObserver *getCtrlWriter() = 0;

    /**
     * Set control description. The maximum description size is 256 bytes.
     *
     * @param data Control description data.
     * @return true on success, false otherwise.
     */
    virtual bool setControlDescription(const char *data) = 0;

    /**
     * Return version string that can be used to identify the release number and other details of
     * the library.
     *
     * @return String describing the library version.
     *
     * @code
     * const char version = ozoaudioencoder::PostCaptureFileWriter::version();
     * @endcode
     */
    static const char *version();

protected:
    /** @cond PRIVATE */
    PostCaptureFileWriter() {}
    virtual ~PostCaptureFileWriter() {}
    /** @endcond */
};

} // namespace ozoaudioencoder
