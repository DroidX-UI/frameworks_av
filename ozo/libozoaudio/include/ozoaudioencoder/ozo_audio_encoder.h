#pragma once
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
 * @brief OZO Audio encoder interface.
 */

#include <stdint.h>

#include "ozoaudioencoder/config.h"
#include "ozoaudioencoder/audio_encoder.h"
#include "ozoaudiocommon/library_export.h"

namespace ozoaudioencoder {

/**
 * Identifiers for creating various OzoAudioEncoder instances.
 */
static const char OZOSDK_LS_ID[] = "ls";
static const char OZOSDK_OZOAUDIO_ID[] = "ozoaudio";
static const char OZOSDK_OZOAUDIO_NOMETA_ID[] = "ozoaudio-nometa";
static const char OZOSDK_AMBISONICS_ID[] = "ambisonics";

/**
 * Interface for encoding OZO Audio content.
 */
class OZO_AUDIO_SDK_PUBLIC OzoAudioEncoder
{
public:
    /** @name Construction and destruction of OzoAudioEncoder */
    //@{
    /**
     * Create new Ozo audio encoder object.
     *
     * @param enc_str Encoder mode.
     * @return Pointer to OzoAudioEncoder object.
     *
     * @code
     * OzoAudioEncoder *encoder;
     *
     * // Create encoder object that encodes microphone array input to encoded audio + spatial
     * metadata encoder = OzoAudioEncoder::create(OZOSDK_OZOAUDIO_ID);
     *
     * // Create encoder object that encodes microphone array input to encoded ambisonics
     * encoder = OzoAudioEncoder::create(OZOSDK_AMBISONICS_ID);
     *
     * @endcode
     */
    static OzoAudioEncoder *create(const char *enc_str);

    /**
     * Destroy Ozo audio encoder object.
     */
    virtual void destroy();
    //@}

    /**
     * Initialize encoder.
     *
     * The following input and output sample format configurations are currently supported:
     *
     * @verbatim
       Mode          |  Input/output sample format
       -----------------------------------------------------------------------------
       ozoaudio         16-bit, 24-bit and normalized float as input, none as output
       ambisonics       16-bit, 24-bit and normalized float as input, none as output
       -----------------------------------------------------------------------------
       @endverbatim
     *
     * @param [in,out] config Ozo encoding configuration data.
     * @param enc_obj Audio encoder handle.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     *
     * @code
     * // Create initialized encoder
     *
     * AudioEncoder *enc_obj;
     * OzoAudioEncoderConfig config;
     * OzoAudioEncoderInitConfig in_config;
     * OzoAudioEncoderCodecConfig codec_config;
     *
     * // Create audio encoder object that implements AudioEncoder interface
     * // This class must be implemented outside of this SDK
     * enc_obj = new <audio-encoder-class>;
     *
     * // Example: Encode 4 mic array input to stereo audio channels @160kbps + spatial metadata.
     *
     * OzoAudioEncoder *encoder = OzoAudioEncoder::create(OZOSDK_OZOAUDIO_ID);
     *
     * if (encoder) {
     *    codec_config.bitrate = 160;
     *    in_config.n_in_channels = 4;
     *    in_config.in_layout = nullptr;
     *    in_config.in_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;
     *    in_config.n_out_channels = 2;
     *    in_config.out_layout = nullptr;
     *    in_config.out_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NONE;
     *    in_config.sample_rate = 48000;
     *    in_config.device = <device-name>; // Name of capturing device
     *    config.in_config = &in_config;
     *    config.codec_config = &codec_config;
     *
     *    ozoaudiocodec::AudioCodecError result = encoder->init(config, enc_obj);
     *    if (result == ozoaudiocodec::OK) {
     *        // Encoder handle successfully initialized
     *
     *        // Audio configuration data in config.vr_config.audio
     *    }
     *    else {
     *        // Error handling here
     *    }
     * }
     *
     * // Example: Encode 4 mic array input to stereo audio channels (no encoding) + spatial
     metadata.
     *
     * OzoAudioEncoder *encoder = OzoAudioEncoder::create(OZOSDK_OZOAUDIO_ID);
     *
     * if (encoder) {
     *    codec_config.bitrate = 0;
     *    in_config.n_in_channels = 4;
     *    in_config.in_layout = nullptr;
     *    in_config.in_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;
     *    in_config.n_out_channels = 2;
     *    in_config.out_layout = nullptr;
     *    in_config.out_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NONE;
     *    in_config.sample_rate = 48000;
     *    in_config.device = <device-name>; // Name of capturing device
     *    config.in_config = &in_config;
     *    config.codec_config = &codec_config;
     *
     *    ozoaudiocodec::AudioCodecError result = encoder->init(config, nullptr);
     *    if (result == ozoaudiocodec::OK) {
     *        // Encoder handle successfully initialized
     *
     *        // Audio configuration data in config.vr_config.audio
     *    }
     *    else {
     *        // Error handling here
     *    }
     * }
     *
     * // Example: Encode 4 mic array input to binaural audio channels @160kbps + spatial metadata.
     *
     * OzoAudioEncoder *encoder = OzoAudioEncoder::create(OZOSDK_OZOAUDIO_ID);
     *
     * if (encoder) {
     *    codec_config.bitrate = 160;
     *    in_config.n_in_channels = 4;
     *    in_config.in_layout = nullptr;
     *    in_config.in_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;
     *    in_config.n_out_channels = 2;
     *    in_config.out_layout = nullptr;
     *    in_config.out_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NONE;
     *    in_config.sample_rate = 48000;
     *    in_config.device = <device-name>; // Name of capturing device
     *    config.in_config = &in_config;
     *    config.codec_config = &codec_config;
     *
     *    ozoaudiocodec::AudioCodecError result = encoder->init(config, enc_obj);
     *    if (result == ozoaudiocodec::OK) {
     *        // Encoder handle successfully initialized
     *
     *        // Audio configuration data in config.vr_config.audio
     *    }
     *    else {
     *        // Error handling here
     *    }
     * }
     *
     * // Example: Same as previous example but capture accepts now also directional information.
     *
     * OzoAudioEncoder *encoder = OzoAudioEncoder::create(OZOSDK_OZOAUDIO_ID);
     *
     * if (encoder) {
     *    <same config init as in previous example>
     *    config.in_config = &in_config;
     *    config.codec_config = nullptr;
     *
     *    ozoaudiocodec::AudioCodecError result = encoder->init(config, enc_obj);
     *    if (result == ozoaudiocodec::OK) {
     *        // Encoder handle successfully initialized
     *
     *        // Set capture focus direction to front with focus sector size
     *        // and focus gain definitions
     *        if (encoder->getControl()->setState(ozoaudiocodec::kFocusId, (uint32_t)
     *           ozoaudiocodec::kFocusAzimuth, 0.0) == ozoaudiocodec::OK &&
     *        encoder->getControl()->setState(ozoaudiocodec::kFocusId, (uint32_t)
     *           ozoaudiocodec::kFocusElevation, 0.0) == ozoaudiocodec::OK) &&
     *        encoder->getControl()->setState(ozoaudiocodec::kFocusId, (uint32_t)
     *           ozoaudiocodec::kFocusSectorWidth, 90.0) == ozoaudiocodec::OK) &&
     *        encoder->getControl()->setState(ozoaudiocodec::kFocusId, (uint32_t)
     *           ozoaudiocodec::kFocusSectorHeight, 90.0) == ozoaudiocodec::OK) &&
     *        encoder->getControl()->setState(ozoaudiocodec::kFocusId, (uint32_t)
     *           ozoaudiocodec::kFocusGain, 2.5) == ozoaudiocodec::OK) {
     *
     *            // Audio configuration data in config.out_config.vr_config.audio
     *        }
     *    }
     *    else {
     *        // Error handling here
     *    }
     * }
     *
     * // Example: Encode 8 mic array input to ambisonics audio @512kbps.
     *
     * OzoAudioEncoder *encoder = OzoAudioEncoder::create(OZOSDK_AMBISONICS_ID);
     *
     * if (encoder) {
     *    codec_config.bitrate = 512;
     *    in_config.n_in_channels = 8;
     *    in_config.in_layout = nullptr;
     *    in_config.in_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;
     *    in_config.n_out_channels = 4;
     *    in_config.out_layout = nullptr;
     *    in_config.out_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NONE;
     *    in_config.sample_rate = 48000;
     *    in_config.device = <device-name>; // Name of capturing device
     *    config.in_config = &in_config;
     *    config.codec_config = &codec_config;
     *
     *    ozoaudiocodec::AudioCodecError result = encoder->init(config, enc_obj);
     *    if (result == ozoaudiocodec::OK) {
     *        // Encoder handle successfully initialized
     *
     *        // Audio configuration data in config.vr_config.audio
     *    }
     *    else {
     *        // Error handling here
     *    }
     * }
     *
     * // Example: Encode 3 mic array input to mono audio @128kbps.
     *
     * OzoAudioEncoder *encoder = OzoAudioEncoder::create(OZOSDK_LS_ID);
     *
     * if (encoder) {
     *    codec_config.bitrate = 128;
     *    in_config.n_in_channels = 3;
     *    in_config.in_layout = nullptr;
     *    in_config.in_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;
     *    in_config.n_out_channels = 1;
     *    in_config.out_layout = "mono";
     *    in_config.out_sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NONE;
     *    in_config.sample_rate = 48000;
     *    in_config.device = <device-name>; // Name of capturing device
     *    config.in_config = &in_config;
     *    config.codec_config = &codec_config;
     *
     *    ozoaudiocodec::AudioCodecError result = encoder->init(config, enc_obj);
     *    if (result == ozoaudiocodec::OK) {
     *        // Encoder handle successfully initialized
     *
     *        // Audio configuration data in config.vr_config.audio
     *    }
     *    else {
     *        // Error handling here
     *    }
     * }
     * @endcode
     */
    virtual ozoaudiocodec::AudioCodecError init(OzoAudioEncoderConfig &config,
                                                AudioEncoder *enc_obj) = 0;

    /**
     * Encode input frame audio to desired output audio format.
     *
     * @param input Input audio in interleaved format.
     * @param [out] output Encoded bitstream buffer(s). The caller must not allocate memory to any
     * of the buffers defined for this data structure.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     *
     * @code
     * // Encode audio frame
     *
     * OzoAudioEncoder *encoder = <create-initialized-encoder>;
     *
     * if (encoder) {
     *     for <loop-audio-frames> {
     *         ozoaudiocodec::AudioCodecError result;
     *         ozoaudiocodec::AudioDataContainer output;
     *
     *         // In ozoaudio mode, the capture direction can be changed according to
     *         if (encoder->getControl()->setState(ozoaudiocodec::kFocusId, (uint32_t)
     * ozoaudiocodec::kFocusAzimuth, 180.0) != ozoaudiocodec::OK) ; // Error occured when changing
     * capture direction, possible error handling here
     *
     *         // Number of audio samples to input for encode method is config.n_in_channels *
     * config.frame_length; result = encoder->encode(<interleaved-audio-frame>, output); if (result
     * == ozoaudiocodec::OK) {
     *             // Encoding succeeded
     *
     *             // Audio data in output.audio
     *             // Metadata (if applicable) in output.meta
     *         }
     *         else {
     *             // Error handling here
     *         }
     *     }
     * }
     *
     * enc_obj->destroy();
     * encoder->destroy();
     * @endcode
     */
    virtual ozoaudiocodec::AudioCodecError encode(const uint8_t *input,
                                                  ozoaudiocodec::AudioDataContainer &output) = 0;

    /**
     * Return controller for the encoder. @see ozoaudiocodec::AudioControl for further information
     * regarding the controller interface.
     *
     * @return Pointer to controller object.
     */
    virtual ozoaudiocodec::AudioControl *getControl() = 0;

    /**
     * Return meta controller for the encoder. Additional initialization parameters can be set via
     * this interface.
     *
     * @return Pointer to meta attributes object.
     *
     * @code
     * auto meta = encoder->getInitMetaControl();
     *
     * // Select recording mode to binaural - for stereo+meta, set to false
     * meta->setInt32Data(ozoaudioencoder::kBinauralTag, true);
     *
     * // Proceed to encoder initialization
     * ozoaudiocodec::AudioCodecError result = encoder->init(config, enc_obj);
     * ...
     *
     * @endcode
     */
    virtual ozoaudiocodec::OzoAudioMeta *getInitMetaControl() = 0;

    /**
     * Assign observer(s) to encoder. One or more observers can be assigned.
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
     * auto version = OzoAudioEncoder::version();
     * @endcode
     */
    static const char *version();

protected:
    /** @cond PRIVATE */
    OzoAudioEncoder(){};
    virtual ~OzoAudioEncoder(){};
    /** @endcond */
};

} // namespace ozoaudioencoder
