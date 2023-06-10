#pragma once
/*
  Copyright (C) 2015 Nokia Corporation.
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
 * @brief Error codes and data structures for OZO Audio codec.
 */

#include <stdint.h>

#include "ozoaudiocommon/layout.h"

namespace ozoaudiocodec {

/**
 * Ozo codec error codes.
 */
enum AudioCodecError
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
     * Invalid handle.
     */
    INVALID_HANDLE = -2,

    /**
     * Audio codec not initialized.
     */
    AUDIOCODEC_NOT_INITIALIZED = -3,

    /**
     * Ozo codec not initialized.
     */
    NOT_INITIALIZED = -4,

    /**
     * Processing error.
     */
    PROCESSING_ERROR = -5,

    /**
     * Unsupported capturing device.
     */
    UNSUPPORTED_DEVICE = -6,

    /**
     * Unsupported channels count.
     */
    CHANNELS_ERROR = -7,

    /**
     * Unsupported encoding mode.
     */
    UNSUPPORTED_MODE = -8,

    /**
     * Data is invalid.
     */
    INVALID_DATA = -9,

    /**
     * Data is invalid.
     */
    UNSUPPORTED_CODEC = -10,

    /**
     * Input and/or output sample format configuration not supported.
     */
    UNSUPPORTED_SAMPLE_FORMAT = -11,

    /**
     * Channel layout is invalid.
     */
    INVALID_CHANNEL_LAYOUT = -12,

    /**
     * Unsupported control action.
     */
    UNSUPPORTED_CTRL_ACTION = -13,

    /**
     * Control action failed.
     */
    CTRL_ACTION_ERROR = -14,

    /**
     * Control handle not found.
     */
    CTRL_HANDLE_NOT_FOUND = -15,

    /**
     * Unable to set meta configuration data.
     */
    META_CONFIG_ERROR = -16,

    /**
     * Unable to set meta sample data.
     */
    META_SAMPLE_DATA_ERROR = -17,

    /**
     * Invalid SPAC configuration data specified
     */
    INVALID_SPAC_CONFIG = -18,

    /**
     * Unsupported playback mode.
     */
    UNSUPPORTED_PLAYBACK = -19,

    /**
     * Mismatch between channel count in SPAC config and input stream.
     */
    SPAC_CONFIG_CH_ERROR = -20,

    /**
     * Meta injector not initialized
     */
    META_INJECTOR_NOT_INITIALIZED = -21,

    /**
     * Meta injector processing error.
     */
    META_INJECTOR_PROCESSING_ERROR = -22,

    /**
     * Meta extractor processing error.
     */
    META_EXTRACTOR_PROCESSING_ERROR = -23,

    /**
     * Invalid audio specific config.
     */
    INVALID_AUDIO_CONFIG_ERROR = -24,

    /**
     * Data observer writing failed.
     */
    OBSERVER_WRITE_ERROR = -25,

    /**
     * End of stream encountered.
     */
    END_OF_STREAM = -26,

    /**
     * Permission error. Access is denied to specified functionality.
     */
    PERMISSION_ERROR = -27,

    /**
     * Invalid call was made. For example, double init is typically not allowed.
     */
    INVALID_CALL = -28,

    /**
     * Observer cannot be set. Observer set after encoder has been initialized is not allowed.
     */
    UNABLE_TO_SET_OBSERVER = -29,

    /**
     * Device configuration is missing from SDK license.
     */
    LICENSE_MISSING_DEVICE_ERROR = -30,

    /**
     * Data buffer is too small.
     */
    DATABUFFER_TOO_SMALL_ERROR = -31,
};

/**
 * Sample formats for audio data.
 */
enum SampleFormat
{

    /**
     * Sample format not applicable.
     */
    SAMPLE_NONE,

    /**
     * 2 bytes per sample in host native byte order (small endian or big endian).
     */
    SAMPLE_16BIT,

    /**
     * 3 bytes per sample in host native byte order.
     */
    SAMPLE_24BIT,

    /**
     * Normalized float sample, range -1,...,1.
     */
    SAMPLE_NORMFLOAT
};

/**
 * Maximum number of meta frames within one audio frame.
 */
const int MaxMetaPerAudioFrame = 4;

/**
 * Generic container for audio and meta data.
 */
typedef struct CodecData
{
    /**
     * Data buffer.
     */
    uint8_t *data;

    /**
     * Size of data buffer.
     */
    uint32_t data_size;

    /**
     * Version ID of the data. Value -1 indicates N/A.
     */
    int32_t version;

} CodecData;

/**
 * Configuration data for codec.
 */
typedef struct AudioDataContainer
{
    /**
     * Audio configuration data.
     */
    CodecData audio;

    /**
     * Number of meta frames per audio frame.
     */
    int16_t n_meta;

    /**
     * Meta configuration data.
     */
    CodecData meta[4];

} AudioDataContainer;


/**
 * Supported channel layouts. The decoder supports all layouts, the encoder may support only
 * a subset of layouts.
 *
 * @link #OZO_AUDIO_STEREO_LAYOUT stereo layout @endlink <br>
 * Speaker order: left, right. <br>
 * Azimuth angles of speakers: 30, -30 degrees.
 *
 * @link #OZO_AUDIO_QUAD_LAYOUT quad layout @endlink <br>
 * Speaker order: front left, front right, rear left, rear right. <br>
 * Azimuth angles of speakers: 45, -45, 135, -135 degrees.
 *
 * @link #OZO_AUDIO_40_LAYOUT 4.0 layout @endlink <br>
 * Speaker order: center, front left, front right, rear center. <br>
 * Azimuth angles of speakers: 0, 45, -45, 180 degrees.
 * Note: This layout is also used for 1st order ambisonics.
 *
 * @link #OZO_AUDIO_50_LAYOUT 5.0 layout @endlink <br>
 * Speaker order: front left, front right, center, side left, side right. <br>
 * Azimuth angles of speakers: 30, -30, 0, 110, -110 degrees.
 *
 * @link #OZO_AUDIO_50_PROTOOLS_LAYOUT ProTools 5.0 layout @endlink <br>
 * Speaker order: front left, center, front right, side left, side right. <br>
 * Azimuth angles of speakers: 30, 0, -30, 110, -110 degrees.
 *
 * @link #OZO_AUDIO_50_ISO_LAYOUT 5.0 layout according to ISO/IEC FDIS 23001-8 @endlink <br>
 * Speaker order: center, front left, front right, side left, side right. <br>
 * Azimuth angles of speakers: 0, 30, -30, 110, -110 degrees.
 *
 * @link #OZO_AUDIO_51_LAYOUT ITU 5.1 layout @endlink <br>
 * Speaker order: front left, front right, center, LFE (Low-Frequency Effects), side left, side
 * right. <br> Azimuth angles of speakers: 30, -30, 0, 0, 110, -110 degrees.
 *
 * @link #OZO_AUDIO_51_ISO_LAYOUT 5.1 layout according to ISO/IEC FDIS 23001-8 @endlink <br>
 * Speaker order: center, front left, front right, side left, side right, LFE. <br>
 * Azimuth angles of speakers: 0, 30, -30, 110, -110, -15 (elevation) degrees.
 *
 * @link #OZO_AUDIO_70_LAYOUT 7.0 layout @endlink <br>
 * Speaker order: front left, center, front right, side left, side right, rear left, rear right.
 * <br> Azimuth angles of speakers: 30, 0, -30, 90, -90, 150, -150 degrees.
 *
 * @link #OZO_AUDIO_70_NUENDO_LAYOUT Nuendo 7.0 layout @endlink <br>
 * Speaker order: front left, front right, center, rear left, rear right, side left, side right.
 * <br> Azimuth angles of speakers: 30, -30, 0, 150, -150, 90, -90 degrees.
 *
 * @link #OZO_AUDIO_71_ISO_LAYOUT 7.1 layout according to ISO/IEC FDIS 23001-8 @endlink <br>
 * Speaker order: center, front left, front right, side left, side right, rear left, rear right,
 * LFE. <br> Azimuth angles of speakers: 0, 30, -30, 45, -45, 110, -110, -15 (elevation) degrees.
 *
 * @link #OZO_AUDIO_71_ISO_OZO_LAYOUT OZO 7.1 layout <br>
 * Speaker order: front left, center, front right, side left, side right, rear left, rear right,
 * LFE. <br> Azimuth angles of speakers: 0, 30, -30, 90, -90, 150, -150, -15 (elevation) degrees.
 *
 * @link #OZO_AUDIO_80_LAYOUT 8.0 layout @endlink <br>
 * Speaker order: front, front right, side right, rear right, back, rear left, side left, front
 * left. <br> Azimuth angles of speakers: 0, -45, -90, -135, 180, 135, 90, 45 degrees.
 *
 * @link #OZO_AUDIO_100_LAYOUT 10.0 layout @endlink <br>
 * Speaker directions: front left, front right, side left, side right, rear left, rear right,
 * front left up, front right up, rear left up, rear right up. <br>
 * Azimuth angles of speakers at 0 elevation: 30, -30, 90, -90, 150, -150 <br>
 * Azimuth angles of speakers at 30 elevation: 45, -45, 135, -135
 *
 * @link #OZO_AUDIO_130_LAYOUT 13.0 layout @endlink <br>
 * Speaker directions: front left, center, front right, side left, side right, rear left, rear
 * right, front left up, center up, front right up, side left up, side right up, top. <br> Azimuth
 * angles of speakers at 0 elevation: 30, 0, -30, 90, -90, 150, -150 <br> Azimuth angles of speakers
 * at 30 elevation: 30, 0, -30, 110, -110 <br> Azimuth angles of speakers at 90 elevation: 0
 *
 * @link #OZO_AUDIO_140_LAYOUT 14.0 layout @endlink <br>
 * Speaker directions: front left, front right, side left, side right, rear left, rear right,
 * front left up, front right up, rear left up, rear right up,
 * front left down, front right down, rear left down, rear right down. <br>
 * Azimuth angles of speakers at 0 elevation: 30, -30, 90, -90, 150, -150 <br>
 * Azimuth angles of speakers at 30 elevation: 45, -45, 135, -135 <br>
 * Azimuth angles of speakers at -25 elevation: 45, -45, 135, -135
 *
 * @link #OZO_AUDIO_150_LAYOUT 15.0 layout @endlink <br>
 * Speaker directions: front left, front right, center, surround back left, surround back right,
 * surround left, surround right,
 * front left up, front right up, rear left up, rear right up,
 * front left down, front right down, rear left down, rear right down. <br>
 * Azimuth angles of speakers at 0 elevation: 30, -30, 0, 150, -150, 100, -100 <br>
 * Azimuth angles of speakers at 30 elevation: 50, -50, 125, -125 <br>
 * Azimuth angles of speakers at -25 elevation: 50, -50, 125, -125
 *
 * @link #OZO_AUDIO_151_LAYOUT 15.1 layout @endlink <br>
 * Speaker directions: front left, front right, center, LFE, surround back left, surround back
 * right, surround left, surround right, front left up, front right up, rear left up, rear right up,
 * front left down, front right down, rear left down, rear right down. <br>
 * Azimuth angles of speakers at 0 elevation: 30, -30, 0, 0 (LFE), 150, -150, 100, -100 <br>
 * Azimuth angles of speakers at 30 elevation: 50, -50, 125, -125 <br>
 * Azimuth angles of speakers at -25 elevation: 50, -50, 125, -125
 *
 * @link #OZO_AUDIO_160_NAT_LAYOUT 16.0 layout @endlink <br>
 * Speaker directions: front left, front right, center, rear center, side left, side right, rear
 * left, rear right, front left up, front right up, rear left up, rear right up, front left down,
 * front right down, rear left down, rear right down. <br> Azimuth angles of speakers at 0
 * elevation: 45, -45, 0, 180, 90, -90, 135, -135 <br> Azimuth angles of speakers at 45 elevation:
 * 45, -45, 135, -135 <br> Azimuth angles of speakers at -45 elevation: 45, -45, 135, -135
 *
 * @link #OZO_AUDIO_120_NAT_LAYOUT 12.0 layout @endlink <br>
 * Speaker directions: front left, front right, rear left, rear right,
 * front left up, front right up, rear left up, rear right up,
 * front left down, front right down, rear left down, rear right down. <br>
 * Azimuth angles of speakers at 0 elevation: 45, -45, 135, -135 <br>
 * Azimuth angles of speakers at 45 elevation: 45, -45, 135, -135 <br>
 * Azimuth angles of speakers at -45 elevation: 45, -45, 135, -135
 *
 * @link #OZO_AUDIO_60_NAT_LAYOUT 6.0 layout @endlink <br>
 * Speaker directions: front left, front right, rear left, rear right, top, bottom. <br>
 * Azimuth angles of speakers at 0 elevation: 45, -45, 135, -135 <br>
 * Azimuth angles of speakers at 90 elevation: 0 <br>
 * Azimuth angles of speakers at -90 elevation: 0
 */
static const char *const channel_layouts[] = {
    OZO_AUDIO_STEREO_LAYOUT,    OZO_AUDIO_QUAD_LAYOUT,        OZO_AUDIO_40_LAYOUT,
    OZO_AUDIO_50_LAYOUT,        OZO_AUDIO_50_PROTOOLS_LAYOUT, OZO_AUDIO_50_ISO_LAYOUT,
    OZO_AUDIO_51_LAYOUT,        OZO_AUDIO_51_ISO_LAYOUT,      OZO_AUDIO_70_LAYOUT,
    OZO_AUDIO_70_NUENDO_LAYOUT, OZO_AUDIO_71_ISO_LAYOUT,      OZO_AUDIO_71_ISO_OZO_LAYOUT,
    OZO_AUDIO_80_LAYOUT,        OZO_AUDIO_100_LAYOUT,         OZO_AUDIO_130_LAYOUT,
    OZO_AUDIO_140_LAYOUT,       OZO_AUDIO_150_LAYOUT,         OZO_AUDIO_151_LAYOUT,
    OZO_AUDIO_160_NAT_LAYOUT,   OZO_AUDIO_120_NAT_LAYOUT,     OZO_AUDIO_60_NAT_LAYOUT};

} // namespace ozoaudiocodec
