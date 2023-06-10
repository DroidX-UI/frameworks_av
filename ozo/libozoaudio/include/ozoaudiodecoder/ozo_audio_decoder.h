#pragma once
/*
  Copyright (C) 2017 Nokia Corporation.
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
 * @brief Multistream audio decoding.
 */

#include <stdint.h>
#include <stddef.h>

#include "ozoaudiodecoder/config.h"
#include "ozoaudiodecoder/audio_decoder.h"
#include "ozoaudiocommon/library_export.h"

namespace ozoaudiodecoder {

/**
 * Interface for decoding multistream audio content.
 */
class OZO_AUDIO_SDK_PUBLIC OzoAudioMultiTrackDecoder
{
public:
    /** @name Construction and destruction of OzoAudioMultiTrackDecoder */
    //@{
    /**
     * Create new audio decoder object capable of decoding multiple audio tracks.
     *
     * @return Pointer to OzoAudioMultiTrackDecoder object.
     */
    static OzoAudioMultiTrackDecoder *create();

    /**
     * Destroy decoder object.
     */
    virtual void destroy();
    //@}

    /**
     * Add track for decoding.
     *
     * The following input sample format configurations are currently supported:
     *
     * @verbatim
       Input sample format
       -------------------
       none
       -------------------
       @endverbatim
     *
     * @param track_config Track configuration data.
     * @param dec_obj Audio decoder handle that is responsible of decoding the track data.
     * @param render_config Render configuration data. Currently all tracks must specify same output
     render configuration.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual ozoaudiocodec::AudioCodecError
    addTrack(const OzoAudioMultiTrackDecoderConfig &track_config,
             const OzoAudioDecoderMixerConfig &render_config,
             AudioDecoder *dec_obj) = 0;

    /**
     * Initialize decoder. All added audio tracks are now initialized and routed to produce single
     output audio.
     * Error code is returned if unsupported track combinations are found.
     *
     * The following output sample format configurations are currently supported:
     *
     * @verbatim
       Output sample format
       ------------------------------------
       16-bit, 24-bit, and normalized float
       ------------------------------------
       @endverbatim
     *
     * @param mixer_config Mixer configuration data.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     *
     * @code
     *
     * OzoAudioMultiTrackDecoderConfig config_track1;
     * OzoAudioMultiTrackDecoderConfig config_track2;
     * OzoAudioDecoderMixerConfig config_mixer;
     *
     * auto decoder = OzoAudioMultiTrackDecoder::create();
     *
     * // Create audio decoder object that implements AudioDecoder interface
     * // This class is to be implemented outside of this SDK
     * // In this example both audio tracks are using same audio coding format
     * auto dec_obj = new <audio-decoder-class>;
     *
     * // Mixer configuration: request binaural output
     * config_render.ch_layout = nullptr;
     * config_render.n_out_channels = 2;
     * config_render.render_mode = ozoaudiodecoder::binauralRender;
     * config_render.sample_format = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;
     *
     * // Configure 1st track
     * config_track1.track_type = TrackType::LOUDSPEAKER_TRACK;
     * config_track1.sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NONE;
     * config_track1.config_data.audio.data = <audio-track1-configuration-data-buffer>;
     * config_track1.config_data.audio.data_size = <audio-track1-configuration-data-buffer-size>;
     * config_track1.config_data.n_meta = 0;
     *
     * if (decoder->addTrack(config_track1, config_render, dec_obj) !=
     ozoaudiocodec::AudioCodecError::OK) {
     *     // Abort with error handling here
     * }
     *
     * // Configure 2nd track
     * config_track2.track_type = TrackType::OZO_TRACK;
     * config_track2.sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NONE;
     * config_track2.config_data.audio.data = <audio-track2-configuration-data-buffer>;
     * config_track2.config_data.audio.data_size = <audio-track2-configuration-data-buffer-size>;
     * config_track2.config_data.n_meta = 1;
     * config_track2.config_data.meta[0].data = <track2-metadata-configuration-data-buffer>;
     * config_track2.config_data.meta[0].data_size =
     <track2-metadata-configuration-data-buffer-size>;
     * config_track2.config_data.meta[0].version = <track2-metadata-configuration-data-version>;
     *
     * if (decoder->addTrack(config_track2, config_render, dec_obj) !=
     ozoaudiocodec::AudioCodecError::OK) {
     *     // Abort with error handling here
     * }
     *
     * // Initialize decoder
     * if (decoder->init(config_render) == ozoaudiocodec::AudioCodecError::OK) {
     *     // Decoder handle successfully initialized
     * }
     * else {
     *     // Error handling here
     * }
     *
     * @endcode
     */
    virtual ozoaudiocodec::AudioCodecError init(const OzoAudioDecoderMixerConfig &mixer_config) = 0;

    /**
     * Retrieve audio track information. Information is available after
     * OzoAudioMultiTrackDecoder::init has been successfully called.
     *
     * @param track_index Audio track index. The indexing follows
     * OzoAudioMultiTrackDecoder::addTrack call order. Index -1 indicates that info from decoder
     * output should be used.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     * The following data fields are valid on return:<br/>
     * AudioFrameInfo::n_channels (number of channels in the audio track) <br/>
     * AudioFrameInfo::sample_rate (sample rate of the track) <br/><br/>
     *
     * @code
     * AudioFrameInfo info;
     * decoder->getTrackInfo(0, info); // Loudspeaker track
     * decoder->getTrackInfo(1, info); // OZO Audio track
     * @endcode
     */
    virtual ozoaudiocodec::AudioCodecError getTrackInfo(int64_t track_index,
                                                        AudioFrameInfo &info) const = 0;

    /**
     * Decode input frame from multiple tracks, produce mixed output.
     *
     * @param data Input frame data for each audio track (using OzoAudioMultiTrackDecoder::addTrack
     * call order).
     * @param [out] info Decoded output frame information. The following data fields are valid on
     * return:<br/> AudioFrameInfo::n_channels<br/> AudioFrameInfo::sample_rate<br/>
     * AudioFrameInfo::n_samples <br/>
     * AudioFrameInfo::samples (in units of OzoAudioDecoderMixerConfig::out_sample_format)<br/><br/>
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     *
     * @code
     *
     * // Decode multi-track audio
     * if (decoder) {
     *     OzoAudioDecoderDynamicData *input[2];
     *     for <loop-frames-in-format-container> {
     *         OzoAudioDecoder::AudioFrameInfo info;
     *         ozoaudiocodec::AudioCodecError result;
     *
     *         // Loudspeaker track
     *         input[0].enc_data.audio = <audio-sample-data-track1>;
     *         input[0].enc_data.n_meta = 0;
     *
     *         // OZO Audio track
     *         input[1].enc_data.audio = <audio-sample-data-track2>;
     *         input[1].enc_data.n_meta = 1;
     *         input[1].enc_data.meta = <meta-sample-data-track2>;
     *
     *         if (decoder->decode(&input, info) == ozoaudiocodec::AudioCodecError::OK) {
     *             // Decoding succeeded
     *
     *             // Mixer PCM samples available in info.samples
     *             // Number of PCM samples decoded is info.n_samples
     *         }
     *     }
     * }
     *
     * dec_obj->destroy();
     * decoder->destroy();
     * @endcode
     */
    virtual ozoaudiocodec::AudioCodecError
    decode(const OzoAudioDecoderDynamicData *const *const data, AudioFrameInfo &info) = 0;

    /**
     * Flush the audio data currently queued within decoder.
     *
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual ozoaudiocodec::AudioCodecError flush() = 0;

    /**
     * Return controller for the decoder. @see ozoaudiocodec::AudioControl for further information
     * regarding the controller interface.
     *
     * @param track_index Audio track index. If no track index is specified, the controller actions
     * are applied to all tracks.
     * @return Pointer to controller object.
     */
    virtual ozoaudiocodec::AudioControl *getControl(int64_t track_index = -1) const = 0;

    /**
     * Return version string that can be used to identify the release number and other details of
     * the library.
     *
     * @return String describing the library version.
     *
     * @code
     * const char *version = OzoAudioMultiTrackDecoder::version();
     * @endcode
     */
    static const char *version();

protected:
    /** @cond PRIVATE */
    OzoAudioMultiTrackDecoder(){};
    virtual ~OzoAudioMultiTrackDecoder(){};
    /** @endcond */
};

/**
 * Create custom decoder config for audio tracks that are not in encoded format.
 *
 * @param config_buffer [out] Track configuration data.
 * @param buffer_size [in/out] Size of track configuration data, in bytes. The input should
 be the size of the configuration data buffer and on return this will represent the actual
 size of the custom decoder config. Currently the minimum data buffer size is 6 bytes.
 * @param sample_rate Sample rate of the audio track, in Hz.
 * @param channels Number of channel in the audio track.
 * @param format Optional format identifier.
 * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
 */
OZO_AUDIO_SDK_PUBLIC ozoaudiocodec::AudioCodecError createCustomConfig(uint8_t *const config_buffer,
                                                                       size_t &buffer_size,
                                                                       uint32_t sample_rate,
                                                                       uint16_t channels,
                                                                       const char *format = nullptr);

/**
 * Detect presence of OZO Audio within the audio track.
 *
 * @param codec Name of audio track decoder. The following decoders are currently supported: aac
 * @param decoder_config Decoder specific configuration of the audio track.
 * @param size Size of audio specific decoder configuration buffer.
 * @return true if OZO Audio detected, false otherwise.
 */
OZO_AUDIO_SDK_PUBLIC bool detectOzoAudio(const char *codec,
                                         const uint8_t *decoder_config,
                                         size_t size);

/**
 * Read audio parameters from decoder specific config data.
 *
 * @param decoder_config Decoder specific configuration of the audio track.
 * @param size Size of audio specific decoder configuration buffer.
 * @param sample_rate [out] Audio sample rate.
 * @param num_channels [out] Number of audio channels.
 * @return true if parameter reading successful, false otherwise.
 */
OZO_AUDIO_SDK_PUBLIC bool
readCSD(const uint8_t *decoder_config, size_t size, int32_t &sample_rate, int32_t &num_channels);

/**
 * Register decoder logger.
 *
 * @param logger Logger receiver.
 * @param levels Logging levels for the logger, multiple levels are allowed.
 * @see ozoaudiocodec::LoggerLevel for further information regarding the levels.
 * @return true on success, false otherwise.
 */
OZO_AUDIO_SDK_PUBLIC bool registerLogger(const ozoaudiocodec::Logger &logger, size_t levels);

/**
 * Unregister decoder logger.
 *
 * @param logger Logger receiver to be removed.
 * @return true on success, false otherwise.
 */
OZO_AUDIO_SDK_PUBLIC bool removeLogger(const ozoaudiocodec::Logger &logger);

} // namespace ozoaudiodecoder
