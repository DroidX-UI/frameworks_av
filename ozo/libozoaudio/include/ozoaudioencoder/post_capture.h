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
 * @brief OZO Audio post capture interface.
 */

#include <stddef.h>
#include <stdint.h>

#include "ozoaudiocommon/ozo_audio_common.h"
#include "ozoaudiocommon/library_export.h"
#include "ozoaudioencoder/ozo_audio_encoder.h"

namespace ozoaudioencoder {

enum
{
    // End of stream indication
    kEndOfStream = -1
};

enum class ControlValueType
{
    Unknown,
    Int32,
    Double
};

/**
 * Post capture information.
 */
typedef struct PostCaptureInfo
{
    /**
     * Current encoding position in frames. For end of stream signaling
     * ozoaudioencoder::kEndOfStream is used.
     */
    int64_t frame_pos;

    /**
     * Total number of frames available for encoding from start to the end.
     */
    size_t total_frames;

} PostCaptureInfo;


/**
 * Interface for viewing and managing control event details.
 */
class ControlEvent
{
public:
    /**
     * Return frame position of the event.
     */
    virtual size_t getFramePosition() const = 0;

    /**
     * Return codec controller ID.
     * @see ozoaudiocodec::CodecControllerID.
     */
    virtual ozoaudiocodec::CodecControllerID getControllerId() const = 0;

    /**
     * Return type for the control value.
     */
    virtual ControlValueType getValueType() const = 0;

    /**
     * Return control value for control value type ControlValueType::Unknown.
     * @see ozoaudiocodec::AudioControl for further details regarding the preset ID values.
     */
    virtual uint32_t getSelectorId() const = 0;

    /**
     * Return control value for control value type ControlValueType::Int32.
     */
    virtual int32_t getValueInt() const = 0;

    /**
     * Return control value for control value type ControlValueType::Double.
     */
    virtual double getValueDouble() const = 0;

    /**
     * Set new selector ID. Can be called for control value type ControlValueType::Unknown.
     *
     * @param selector_id New selector ID.
     * @return true on success, false otherwise.
     */
    virtual bool setSelectorId(uint32_t selector_id) = 0;

    /**
     * Set new selector ID. Can be called for control value type ControlValueType::Int32.
     *
     * @param selector_id New selector ID.
     * @return true on success, false otherwise.
     */
    virtual bool setValueInt(int32_t value) = 0;

    /**
     * Set new selector ID. Can be called for control value type ControlValueType::Double.
     *
     * @param selector_id New selector ID.
     * @return true on success, false otherwise.
     */
    virtual bool setValueDouble(double value) = 0;

protected:
    /** @cond PRIVATE */
    ControlEvent() {}
    virtual ~ControlEvent() {}
    /** @endcond */
};

/**
 * Interface for viewing and managing control events for post-processing.
 */
class ControlEventList
{
public:
    /**
     * Return size of control event list.
     */
    virtual size_t size() const = 0;

    /**
     * Return current control events. The event object is valid until new events are
     * added or existing events are removed from the list.
     *
     * @param index Control event index within the list.
     * @return Pointer to control event.
     */
    virtual ControlEvent *at(size_t index) const = 0;

    /**
     * Remove control event from specified position.
     *
     * @param index Control event index to be removed.
     * @return true on success, false otherwise.
     */
    virtual bool erase(size_t index) = 0;

    /**
     * Add new control event to list.
     *
     * @param position Frame position.
     * @param id Controller ID.
     * @param value Control value.
     * @return true on success, false otherwise.
     */
    virtual bool add(size_t position, ozoaudiocodec::CodecControllerID id, uint32_t value) = 0;

    /**
     * Add new control event to list.
     *
     * @param position Frame position.
     * @param id Controller ID.
     * @param value Control value.
     * @return true on success, false otherwise.
     */
    virtual bool add(size_t position, ozoaudiocodec::CodecControllerID id, double value) = 0;

    /**
     * Add new control event to list.
     *
     * @param position Frame position.
     * @param id Controller ID.
     * @param selector_id Controller ID subselector.
     * @param value Control value.
     * @return true on success, false otherwise.
     */
    virtual bool add(size_t position,
                     ozoaudiocodec::CodecControllerID id,
                     uint32_t selector_id,
                     int32_t value) = 0;

    /**
     * Add new control event to list.
     *
     * @param position Frame position.
     * @param id Controller ID.
     * @param selector_id Controller ID subselector.
     * @param value Control value.
     * @return true on success, false otherwise.
     */
    virtual bool add(size_t position,
                     ozoaudiocodec::CodecControllerID id,
                     uint32_t selector_id,
                     double value) = 0;

protected:
    /** @cond PRIVATE */
    ControlEventList() {}
    virtual ~ControlEventList() {}
    /** @endcond */
};

/**
 * Interface for post-processing OZO Audio content.
 */
class OZO_AUDIO_SDK_PUBLIC OzoAudioPostCapture
{
public:
    /** @name Construction and destruction of OzoAudioPostCapture */
    //@{
    /**
     * Create new Ozo Audio post capture object.
     *
     * @return Pointer to OzoAudioPostCapture object.
     *
     * @code
     *
     * OzoAudioPostCapture *encoder = ozoaudioencoder::OzoAudioPostCapture::create();
     *
     * @endcode
     */
    static OzoAudioPostCapture *create();

    /**
     * Destroy Ozo Audio post capture object.
     *
     * @code
     *
     * encoder->destroy();
     *
     * @endcode
     */
    virtual void destroy();
    //@}

    /**
     * Initialize post capture encoder.
     *
     * @param mic Raw mics data source.
     * @param ctrl Control data source.
     * @param enc_obj Audio encoder handle.
     * @param [out] config Encoding configuration data.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     *
     * @code
     *
     * auto reader = PostCaptureFileReader::create()
     * if (reader->ctrlVersions())
     *     // Multiple versions available -> select desired control data version for editing
     *     reader->setActiveVersion(<version-index>);
     *
     * if (config->postReader->init(<post-data-file-path>)) {
     *     OzoAudioEncoderOutConfig config;
     *     AudioEncoder *audio_enc = new <AudioEncoder>;
     *
     *     auto result = encoder->init(
     *         reader->getMicReader(),
     *         reader->getCtrlReader(),
     *         audio_enc, config
     *     );
     *     if (result == ozoaudiocodec::AudioCodecError::OK)
     *         // Audio decoder specific configuration available in config.vr_config.audio
     *         // Audio sample rate available in config.sample_rate
     *         // Audio frame length available in config.frame_length
     *         ;
     * }
     *
     * @endcode
     */
    virtual ozoaudiocodec::AudioCodecError init(ozoaudiocodec::DataReader *mic,
                                                ozoaudiocodec::DataReader *ctrl,
                                                AudioEncoder *enc_obj,
                                                OzoAudioEncoderOutConfig &config) = 0;

    /**
     * Encode next audio frame to desired output audio format.
     *
     * @param [out] output Encoded bitstream buffer(s). The caller must not allocate memory to any
     * of the buffers defined for this data structure.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     *
     * @code
     *
     * ozoaudiocodec::AudioDataContainer output;
     * auto result = encoder->encode(output);
     * if (result == ozoaudiocodec::AudioCodecError::OK) {
     *     // Encoded audio available in output.audio
     *     ;
     * }
     *
     * @endcode
     */
    virtual ozoaudiocodec::AudioCodecError encode(ozoaudiocodec::AudioDataContainer &output) = 0;

    /**
     * Change current encoding position.
     *
     * @param framePosition New encoding position, measured in frames. The position is always
     *                      with respect to the start of the audio.
     * @param fast_seek true if fast seek should be performed, false otherwise. Fast seek means that
     *                  next time encode() is called, the output signal is generated from clean
     * state; whereas without fast seek enabled, the succeeding encode() call always generates
     * output signal from the same state as it would be without any seek. The latter approach is
     * computationally more complex and should be used, for example, only when audio is to be
     * exported to a file. Please note that in this mode the observers remain active, which means
     * that data may be received while seek operation is in progress. Fast seek is suitable for
     * playback time listening, where minimal delay from processing is typically preferred.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual ozoaudiocodec::AudioCodecError seek(size_t framePosition, bool fast_seek = false) = 0;

    /**
     * Export post-processing control data.
     *
     * @param [in/out] writer Writer handle.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     *
     * @code
     *
     * auto writer = reader->createCtrlWriter("Some description for the exported control data")
     * auto result = encoder->exportControl(writer);
     * if (result == ozoaudiocodec::AudioCodecError::OK)
     *     reader->saveToFile();
     *
     * @endcode
     */
    virtual ozoaudiocodec::AudioCodecError exportControl(OzoEncoderObserver *writer) = 0;

    /**
     * Return controller for the post-processing. @see ozoaudiocodec::AudioControl for further
     * information regarding the controller interface.
     *
     * @return Pointer to controller object.
     */
    virtual ozoaudiocodec::AudioControl *getControl() = 0;

    /**
     * Return handle for viewing and managing control events.
     *
     * @return Pointer to control events object.
     */
    virtual ControlEventList *getControlEventList() = 0;

    /**
     * Return post capture information from current frame.
     */
    virtual const PostCaptureInfo &getInfo() const = 0;

    /**
     * Assign observer(s) to post-processing. At the moment only wind noise reduction observer is
     * allowed.
     *
     * @param obj Observer handle
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual ozoaudiocodec::AudioCodecError setObserver(OzoEncoderObserver *obj) = 0;

    /**
     * Return version string that can be used to identify the release number and other details of
     * the library.
     *
     * @return String describing the library version.
     *
     * @code
     * const char version = OzoAudioPostCapture::version();
     * @endcode
     */
    static const char *version();

protected:
    /** @cond PRIVATE */
    OzoAudioPostCapture() {}
    virtual ~OzoAudioPostCapture() {}
    /** @endcond */
};


/**
 * Interface for reading capture-time mic signals and related control data.
 */
class OZO_AUDIO_SDK_PUBLIC PostCaptureFileReader
{
public:
    /** @name Construction and destruction of PostCaptureFileReader */
    //@{
    /**
     * Create new file reader object.
     *
     * @return Pointer to PostCaptureFileReader object.
     *
     * @code
     *
     * auto *writer = ozoaudioencoder::PostCaptureFileReader::create();
     *
     * @endcode
     */
    static PostCaptureFileReader *create();

    /**
     * Destroy file reader object.
     */
    virtual void destroy();
    //@}

    /**
     * Initialize reader.
     *
     * @param fileInterface File interface handle.
     * @return true on success, false otherwise.
     *
     * @code
     *
     * auto *reader = ozoaudioencoder::PostCaptureFileReader::create();
     * auto *fd = <create FileWriterReader interface>
     * if (reader) {
     *     if (!reader->init(fd)
     *         // Error handling here
     * }
     *
     * reader->destroy();
     *
     * @endcode
     */
    virtual bool init(FileWriterReader *fileInterface) = 0;

    /**
     * Request mic reader handle. It is expected that this handle is passed to
     * OzoAudioPostCapture::init as such. There should be no need for the caller
     * to access the reader methods.
     *
     * @return Pointer to mic reader object.
     */
    virtual ozoaudiocodec::DataReader *getMicReader() = 0;

    /**
     * Request control data reader handle. It is expected that this handle is passed to
     * OzoAudioPostCapture::init as such. There should be no need for the caller
     * to access the reader methods.
     *
     * @return Pointer to control reader object.
     */
    virtual ozoaudiocodec::DataReader *getCtrlReader() = 0;

    /**
     * Return number of control data versions.
     */
    virtual size_t ctrlVersions() const = 0;

    /**
     * Set specified control version as active version.
     *
     * @param version Version index.
     * @return true on success, false otherwise.
     */
    virtual bool setActiveVersion(size_t version) = 0;

    /**
     * Return control data description for specified version.
     *
     * @param version Version index.
     * @return String describing control data.
     */
    virtual const char *getCtrlDescription(size_t version) const = 0;

    /**
     * Request new control data writer handle. The data provided via the returned object
     * will be written as new version.
     *
     * @param data Control description data. The maximum description size is 256 bytes.
     * @param [out] version Control data version index.
     * @return Pointer to control writer object.
     */
    virtual ozoaudioencoder::OzoEncoderObserver *createCtrlWriter(const char *data,
                                                                  size_t &version) = 0;

    /**
     * Save current control data changes to file.
     *
     * @return true on success, false otherwise.
     */
    virtual bool saveToFile() = 0;

    /**
     * Attach binary blob to specified control data. The maximum blob size is 512 bytes.
     *
     * @param version Control data version index.
     * @param data Binary data.
     * @param size Size of binary data.
     * @return true on success, false otherwise.
     */
    virtual bool attachBlob(size_t version, const uint8_t *data, size_t size) = 0;

    /**
     * Get binary data associated to specified control data.
     *
     * @param version Control data version index.
     * @param [out] size Size of binary data.
     * @return Binary data.
     */
    virtual const uint8_t *getBlob(size_t version, size_t &size) const = 0;

    /**
     * Erase specified control data version. Active version is set to first control data version.
     * Erase will fail if only one control data version is available in the file.
     * Make sure to save the changes also!
     *
     * @param version Control data version index to be removed.
     * @return true on success, false otherwise.
     */
    virtual bool erase(size_t version) = 0;

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
    PostCaptureFileReader() {}
    virtual ~PostCaptureFileReader() {}
    /** @endcond */
};

} // namespace ozoaudioencoder
