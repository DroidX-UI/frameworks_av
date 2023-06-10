/*
  Copyright (C) 2016 Nokia Corporation.
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
 * @brief Generic audio decoder interface.
 */

#ifndef NOK_AUDIO_DECODER_GENERIC_INTERFACE_H_
#define NOK_AUDIO_DECODER_GENERIC_INTERFACE_H_

#include <stdint.h>

#include "ozoaudiocommon/ozo_audio_common.h"
#include "ozoaudiocommon/library_export.h"

namespace ozoaudiodecoder {

#define AUDIO_DECODER_META_DATA_MAX_COUNT 10

/**
 * Meta data container for audio frame.
 */
typedef struct AudioMetaData
{
    /**
     * Frame meta data, if any. The caller must not allocate any memory, the decoder will assign
     * this parameter.
     */
    uint8_t *meta_data[AUDIO_DECODER_META_DATA_MAX_COUNT];

    /**
     * Size of frame meta data.
     */
    int16_t meta_data_size[AUDIO_DECODER_META_DATA_MAX_COUNT];

    /**
     * Number of meta data buffers.
     */
    int16_t n_meta_data;

} AudioMetaData;

/**
 * Audio frame decoding information.
 */
typedef struct AudioFrameInfo
{
    /**
     * Decoded samples in interleaved format. The caller must not allocate any memory, the decoder
     * will assign this parameter.
     */
    uint8_t *samples;

    /**
     * Number of items in AudioFrameInfo::samples.
     */
    int32_t n_samples;

    /**
     * Number of channels.
     */
    int16_t n_channels;

    /**
     * Decoding sample rate, in Hz.
     */
    int32_t sample_rate;

    /**
     * Number of bytes read from input buffer. Value -1 indicates N/A.
     */
    int32_t frame_size;

    /**
     * Input output delay in samples (audio decoding delay is excluded). Value -1 indicates N/A.
     */
    int32_t delay;

    /**
     * Frame meta data, if any.
     */
    AudioMetaData meta_data;

} AudioFrameInfo;

/**
 * Decoder error codes.
 */
enum AudioDecoderError
{

    /**
     * Successful operation.
     */
    OK = 0,

    /**
     * Unable to allocate memory.
     */
    MEMORY_ALLOCATION_FAILED = -1,

    /**
     * Unsupported mode.
     */
    INVALID_PROCESSING_MODE = -2,

    /**
     * Invalid data.
     */
    INVALID_DATA = -3,

    /**
     * Decoder not initialized.
     */
    NOT_INITIALIZED = -4,

    /**
     * Error in processing current frame.
     */
    PROCESSING_ERROR = -5,

    /**
     * Unsupported decoder configuration.
     */
    UNSUPPORTED_DECODER_CONFIG = -6,

    /**
     * Native file format header not found.
     */
    NATIVE_FORMAT_NOT_FOUND = -7
};

/**
 * Audio decoder configuration. This data structure is passed to the decoder initialization method.
 * Contains data fields that describe the decoding mode and various properties from the audio
 * track.
 */
typedef struct AudioDecoderConfig
{
    /**
     * Audio stream information.
     *
     * This will be output parameter, no need to set for initialization.
     */
    AudioFrameInfo frame_data;

    /**
     * Number of audio samples per channel.
     *
     * This will be output parameter, no need to set for initialization.
     */
    int32_t frame_length;

    /**
     * Decoding mode. Implementation must support at least following modes:
     * - "decoder": Input buffer is decoded to output signal.
     */
    const char *mode;

    /**
     * File format of decoded bitstream. Implementation must support the following format(s):
     * - "raw": Input is raw bitstream.
     */
    const char *file_format;

    /**
     * Decoder implementation specific configuration information.
     */
    const char *custom_config;

    /**
     * Sample format request for decoded samples. The decoder may override this value.
     */
    ozoaudiocodec::SampleFormat sample_format;

} AudioDecoderConfig;

/**
 * Audio decoder interface definition. To enable use of 3rd party audio decoders, the Ozo audio
 * decoder uses this interface to decode the audio track present in the container format.
 */
class OZO_AUDIO_SDK_PUBLIC AudioDecoder
{
public:
    /**
     * Destroy audio decoder object.
     */
    virtual void destroy() = 0;

    /**
     * Initialize decoder.
     *
     * @param data Input configuration data.
     * @param [in,out] config Decoder configuration data.<br/><br/>
     * The following data fields must be specified as input:<br/>
     * AudioDecoderConfig::mode <br/>
     * AudioDecoderConfig::file_format <br/><br/>
     * The following data fields are decoder specific:<br/>
     * AudioDecoderConfig::custom_config <br/><br/>
     * The following data fields in @ref config->frame_data are valid on return:<br/>
     * AudioFrameInfo::n_channels <br/>
     * AudioFrameInfo::sample_rate <br/>
     * AudioFrameInfo::n_samples <br/>
     * AudioDecoderConfig::frame_size <br/>
     * AudioDecoderConfig::frame_length
     *
     * @return AudioDecoderError::OK on success, otherwise an error code.
     */
    virtual AudioDecoderError init(const ozoaudiocodec::CodecData &data,
                                   AudioDecoderConfig *config) = 0;

    /**
     * Decode input buffer.
     *
     * @param input Audio buffer to decode.
     * @param input_len Input buffer size.
     * @param [out] frame_data Decoded samples and associated data and information.
     * @return AudioDecoderError::OK on success, otherwise an error code.
     */
    virtual AudioDecoderError decode(uint8_t *input,
                                     int32_t input_len,
                                     AudioFrameInfo &frame_data) = 0;

    /**
     * Flush the audio data currently queued within decoder.
     *
     * @return AudioDecoderError::OK on success, otherwise an error code.
     */
    virtual AudioDecoderError flush() = 0;

    /**
     * Retrieve decoder name, e.g., aac.
     *
     * @return Decoder name.
     */
    virtual const char *getCodecName() const
    {
        return this->m_codecName;
    }

protected:
    /** @cond PRIVATE */

    // Use lower case letters for the name
    const char *m_codecName;

    AudioDecoder()
    {
        m_codecName = nullptr;
    };
    virtual ~AudioDecoder(){};
    /** @endcond */
};

} // namespace ozoaudiodecoder

#endif // NOK_AUDIO_DECODER_GENERIC_INTERFACE_H_
