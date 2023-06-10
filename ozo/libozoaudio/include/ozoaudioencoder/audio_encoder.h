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
 * @brief Generic audio encoder interface.
 */

#ifndef NOK_AUDIO_ENCODER_GENERIC_INTERFACE_H_
#define NOK_AUDIO_ENCODER_GENERIC_INTERFACE_H_

#include <stdint.h>

#include "ozoaudiocommon/ozo_audio_common.h"
#include "ozoaudiocommon/library_export.h"

namespace ozoaudioencoder {

/**
 * Encoder error codes.
 */
enum AudioEncoderError
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
     * Encoder not initialized.
     */
    NOT_INITIALIZED = -4,

    /**
     * Error in processing current frame.
     */
    PROCESSING_ERROR = -5,

    /**
     * Unsupported encoder configuration.
     */
    UNSUPPORTED_ENCODER_CONFIG = -6,

    /**
     * Channel layout not supported.
     */
    UNSUPPORTED_CHANNEL_LAYOUT = -7
};

/**
 * Audio encoder configuration. This data structure is passed to the encoder initialization method.
 * Contains data fields that describe the encoding mode and various properties of the audio track
 * for encoding purposes.
 */
typedef struct AudioEncoderConfig
{
    /**
     * Maximum encoded bitstream buffer size.
     */
    int32_t max_buffer_size;

    /**
     * Input frame length, in samples per channel.
     */
    int32_t frame_length;

    /**
     * Encoding bitrate, in kbps.
     */
    int16_t bitrate;

    /**
     * Encoding mode. Implementation must support at least following modes:
     * - "array": Input signal is microphone array signal.
     * - "loudspeakers": Input signal represents loudspeaker channels.
     */
    const char *mode;

    /**
     * File format of encoded bitstream. Implementation must support at following formats:
     * - "raw": Output is raw bitstream.
     */
    const char *file_format;

    /**
     * Encoder implementation specific configuration information. Could be used, for example, as
     * semicolon separated string such as: "tool=a,b,c;configA=test".
     */
    const char *custom_config;

    /**
     * Number of input channels.
     */
    int16_t n_in_channels;

    /**
     * Input channel layout.
     * @see ozoaudiocodec::channel_layouts for available layouts.
     */
    const char *layout;

    /**
     * Encoding sample rate, in Hz.
     */
    int32_t sample_rate;

    /**
     * Audio configuration information for ISO Base Media file format.
     *
     * This will be output parameter, no need to set for initialization.
     */
    uint8_t audio_specific_config[512];

    /**
     * Size of AudioEncoderConfig::audio_specific_config.
     *
     * This will be output parameter, no need to set for initialization.
     */
    uint16_t audio_config_size;

    /**
     * Encoding delay.
     *
     * This will be output parameter, no need to set for initialization.
     */
    uint32_t encoding_delay;

    /**
     * Sample format request of input samples. Encoder may override this value.
     */
    ozoaudiocodec::SampleFormat sample_format;

    /**
     * Channel layout configuration information for ISO Base Media file format.
     *
     * This will be output parameter, no need to set for initialization.
     */
    uint8_t chnl_config[512];

    /**
     * Size of AudioEncoderConfig::chnl_config.
     *
     * This will be output parameter, no need to set for initialization.
     */
    uint16_t chnl_config_size;

    /**
     * Distribution layout for input. Input channel layout may differ from distribution channel
     * layout depending on which encoder and wheather input channel layout can be used as is by the
     * encoder. This will be output parameter, no need to set for initialization.
     * @see ozoaudiocodec::channel_layouts for available layouts.
     */
    const char *distribution_layout;

} AudioEncoderConfig;

/**
 * Audio encoder interface. To enable use of 3rd party audio encoders, the Ozo audio encoder
 * uses this interface to encode the audio track for the container format.
 */
class OZO_AUDIO_SDK_PUBLIC AudioEncoder
{
public:
    /**
     * Destroy audio encoder object.
     */
    virtual void destroy() = 0;

    /**
     * Initialize encoder.
     *
     * @param [in,out] config Encoder configuration data. The following data fields for the
     * configuration data are set by this function on return:<br/>
     * AudioEncoderConfig::max_buffer_size <br/>
     * AudioEncoderConfig::frame_length <br/>
     * AudioEncoderConfig::audio_specific_config <br/>
     * AudioEncoderConfig::audio_config_size <br/>
     * AudioEncoderConfig::encoding_delay <br/>
     * @return AudioEncoderError::OK on success, otherwise an error code.
     */
    virtual AudioEncoderError init(AudioEncoderConfig *config) = 0;

    /**
     * Encode input audio signal.
     *
     * @param input Audio signal to encode.
     * @param [out] output Encoded bitstream.
     * @param [out] Number of bytes written to output buffer.
     * @param meta_buffer Meta data buffers to be injected to bitstream, if any.
     * @param meta_size Size of meta data buffers.
     * @param n_meta_buffers Number of meta data buffers.
     * @return @ref AudioEncoderError::OK on success, otherwise an error code.
     */
    virtual AudioEncoderError encode(uint8_t *input,
                                     uint8_t *output,
                                     int32_t *output_size,
                                     uint8_t **meta_buffer,
                                     int16_t *meta_size,
                                     int16_t n_meta_buffers) = 0;

    /**
     * Reset encoder. Encoder has to re-initialized after this call has been made.
     *
     * @return AudioEncoderError::OK on success, otherwise an error code.
     */
    virtual AudioEncoderError reset() = 0;

    /**
     * Retrieve encoder name, e.g., aac.
     *
     * @return Encoder name.
     */
    virtual const char *getCodecName()
    {
        return this->m_codecName;
    }

protected:
    /** @cond PRIVATE */

    // Use lower case letters for the name
    const char *m_codecName;

    AudioEncoder()
    {
        m_codecName = nullptr;
    };
    virtual ~AudioEncoder(){};
    /** @endcond */
};

} // namespace ozoaudioencoder

#endif // NOK_AUDIO_ENCODER_GENERIC_INTERFACE_H_
