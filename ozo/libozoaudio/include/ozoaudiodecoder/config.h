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

#include <stdint.h>

#include "ozoaudiocommon/ozo_audio_common.h"

namespace ozoaudiodecoder {

/**
 * Supported renderings.
 */
static const char *const binauralRender = "binaural";
static const char *const loudspeakerRender = "loudspeakers";
static const char *const ambisonicsRender = "ambisonics";

/**
 * Audio rendering guidance.
 */
enum AudioRenderingGuidance
{

    /**
     * No tracking of user's head movement.
     */
    NO_TRACKING = 0,

    /**
     * User's head movements are tracked.
     */
    HEAD_TRACKING
};

/**
 * Supported audio track types.
 */
enum TrackType
{

    /**
     * Track is using loudspeaker configuration.
     */
    LOUDSPEAKER_TRACK,

    /**
     * Track is using OZO Audio configuration.
     */
    OZO_TRACK,

    /**
     * Track is using ambisonics configuration.
     */
    AMBISONICS_TRACK
};

/**
 * Track related input configuration data.
 */
typedef struct OzoAudioMultiTrackDecoderConfig
{
    /**
     * Track type.
     */
    TrackType track_type;

    /**
     * Input sample format.
     */
    ozoaudiocodec::SampleFormat sample_format;

    /**
     * Decoder configuration data buffers. This contains the audio and metadata decoder
     * specific configuration information. Depending on track type, metadata configuration
     * may not be always present; for example ambisonics decoding does not need any metadata
     * configuration.
     */
    ozoaudiocodec::AudioDataContainer config_data;

    /**
     * Track processing guidance.
     */
    AudioRenderingGuidance guidance;

} OzoAudioMultiTrackDecoderConfig;


/**
 * Mixer configuration data for multi-track decoding.
 */
typedef struct OzoAudioDecoderMixerConfig
{
    /**
     * Output sample format.
     */
    ozoaudiocodec::SampleFormat sample_format;

    /**
     * Number of output channels.
     */
    int16_t n_out_channels;

    /**
     * Output channel layout. Must be specified for loudspeaker rendering unless explicit
     * loudspeaker positioning is used (OzoAudioDecoderMixerConfig::ch_layout).
     * @see ozoaudiocodec::channel_layouts for available layouts.
     */
    const char *layout;

    /**
     * Explicit channel layout for output rendering. Valid only if rendering mode is set to
     * loudspeakers. Use null value if rendering should follow the output channel setup as
     * defined by OzoAudioDecoderMixerConfig::layout.
     */
    const ISOChannelLayout *ch_layout;

    /**
     * Target rendering mode. Supported values: ozoaudiodecoder::binauralRender,
     * ozoaudiodecoder::loudspeakerRender and ozoaudiodecoder::ambisonicsRender.
     */
    const char *render_mode;

} OzoAudioDecoderMixerConfig;

/**
 * Dynamic codec data, that is, data that changes from frame to frame.
 */
typedef struct OzoAudioDecoderDynamicData
{
    /**
     * Dynamic data this is in encoded format.
     */
    ozoaudiocodec::AudioDataContainer enc_data;

} OzoAudioDecoderDynamicData;

} // namespace ozoaudiodecoder
