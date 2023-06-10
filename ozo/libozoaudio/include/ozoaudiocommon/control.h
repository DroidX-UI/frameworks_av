#pragma once
/*
  Copyright (C) 2016 Nokia Corporation.
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
 * @brief Control interface for OZO Audio codec.
 */

#include <stdint.h>

#include "ozoaudiocommon/data.h"

namespace ozoaudiocodec {

#define OZO_CONTROL_FOURCC(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

/**
 * Constants for Focus gain values.
 */

/**
 * Value to set zero gain for focus processing.
 */
const double kFocusZeroGain = 0.0;

/**
 * Value to set low gain for focus processing.
 */
const double kFocusMinGain = 1.0;

/**
 * Value to set low gain for focus processing.
 */
const double kFocusLowGain = 2.0;

/**
 * Value to set default gain for focus processing.
 */
const double kFocusDefaultGain = 3.0;

/**
 * Value to set high gain for focus processing.
 */
const double kFocusHighGain = 4.0;

/**
 * Value to set max gain for focus processing.
 */
const double kFocusMaxGain = 5.0;

/**
 * Codec controller IDs.
 */
enum CodecControllerID
{

    /**
     * AGC (Automatic Gain Control) control ID.
     */
    kAGCId = OZO_CONTROL_FOURCC('a', 'g', 'c', 'C'),

    /**
     * Output channel control ID.
     */
    kOutChannelId = OZO_CONTROL_FOURCC('c', 'h', 'o', 'C'),

    /**
     * Listener control ID.
     */
    kListenerId = OZO_CONTROL_FOURCC('l', 'i', 's', 'C'),

    /**
     * Source orientation control ID.
     */
    kOrientationId = OZO_CONTROL_FOURCC('o', 'r', 'i', 'C'),

    /**
     * Source flip control ID.
     */
    kFlipId = OZO_CONTROL_FOURCC('f', 'l', 'i', 'C'),

    /**
     * Encoder directional capture control ID.
     */
    kFocusId = OZO_CONTROL_FOURCC('f', 'o', 'c', 'C'),

    /**
     * Encoder high-pass filter cutoff frequency control ID.
     */
    kHpFilterCutoffId = OZO_CONTROL_FOURCC('h', 'p', 'f', 'C'),

    /**
     * Encoder wind noise reduction control ID.
     */
    kWnrId = OZO_CONTROL_FOURCC('w', 'n', 'r', 'C'),

    /**
     * Encoder analysis sector control ID.
     */
    kAnalysisSectorId = OZO_CONTROL_FOURCC('s', 'e', 'c', 'C'),

    /**
     * Encoder custom control ID.
     */
    kCustomId = OZO_CONTROL_FOURCC('c', 'u', 's', 'C'),

    /**
     * Encoder noise suppression control ID.
     */
    kNoiseSuppressionId = OZO_CONTROL_FOURCC('a', 'n', 's', 'C')
};

/**
 * Codec controller values.
 */
enum
{

    /**
     * AGC controller related values.
     */

    /**
     * AGC processing is off.
     */
    kAgcOffMode = OZO_CONTROL_FOURCC('a', 'g', 'c', 'o'),

    /**
     * Apply AGC soft knee limiting to avoid audible clipping of sound at high signal levels.
     */
    kAgcLimiterMode = OZO_CONTROL_FOURCC('a', 'g', 'c', 'l'),

    /**
     * Both AGC compression and limiting is applied to the audio signal.
     */
    kAgcUserMode = OZO_CONTROL_FOURCC('a', 'g', 'c', 'u'),

    /**
     * Both compression and limiting is applied to the audio signal. However, the output
     * gain can not be controlled. The mute functionality can be used in this mode.
     */
    kAgcAutoMode = OZO_CONTROL_FOURCC('a', 'g', 'c', 'a'),

    // ------------------------------------------------------


    /**
     * Channel controller related values.
     */

    /**
     * Channel gain mode.
     */
    kChannelGain = OZO_CONTROL_FOURCC('c', 'h', 'c', 'g'),

    /**
     * Channel mute mode.
     */
    kChannelMute = OZO_CONTROL_FOURCC('c', 'h', 'c', 'm'),

    /**
     * Value to mute output.
     */
    kMuteValue = OZO_CONTROL_FOURCC('c', 'h', 'm', 'm'),

    /**
     * Value to unmute output.
     */
    kUnMuteValue = OZO_CONTROL_FOURCC('c', 'h', 'm', 'u'),

    // ------------------------------------------------------


    /**
     * Orientation controller related values.
     */

    /**
     * Orientation rotation around the vertical axis.
     */
    kOrientationYaw = OZO_CONTROL_FOURCC('o', 'r', 'i', 'y'),

    /**
     * Orientation rotation around the side-to-side axis.
     */
    kOrientationPitch = OZO_CONTROL_FOURCC('o', 'r', 'i', 'p'),

    /**
     * Orientation rotation around the front-to-back axis.
     */
    kOrientationRoll = OZO_CONTROL_FOURCC('o', 'r', 'i', 'r'),

    // ------------------------------------------------------


    /**
     * Focus controller related values.
     */

    /**
     * Value to enable focus processing.
     */
    kFocusOnMode = OZO_CONTROL_FOURCC('f', 'o', 'm', '1'),

    /**
     * Value to disable focus processing.
     */
    kFocusOffMode = OZO_CONTROL_FOURCC('f', 'o', 'm', '0'),


    /**
     *  Number of focus beams available.
     */
    kFocusBeamCount = OZO_CONTROL_FOURCC('f', 'o', 'c', 'b'),

    /**
     * Focus mode.
     */
    kFocusMode = OZO_CONTROL_FOURCC('f', 'o', 'c', 'm'),

    /**
     * Focus gain.
     */
    kFocusGain = OZO_CONTROL_FOURCC('f', 'o', 'c', 'g'),

    /**
     * Azimuth value of focus direction (-180.0 - +180.0).
     */
    kFocusAzimuth = OZO_CONTROL_FOURCC('f', 'o', 'c', 'a'),

    /**
     * Elevation value of focus direction (-90.0 - +90.0).
     */
    kFocusElevation = OZO_CONTROL_FOURCC('f', 'o', 'c', 'e'),

    /**
     * Width of focus sector (0.0 - +360.0).
     */
    kFocusSectorWidth = OZO_CONTROL_FOURCC('f', 'o', 'c', 'w'),

    /**
     * Height of focus sector (0.0 - +180.0).
     */
    kFocusSectorHeight = OZO_CONTROL_FOURCC('f', 'o', 'c', 'h'),

    // ------------------------------------------------------


    /**
     * Flip controller related values.
     */

    /**
     * Regular orientation.
     */
    kFlipRegularValue = OZO_CONTROL_FOURCC('f', 'l', 'o', 'r'),

    /**
     * Inverted orientation.
     */
    kFlipInvertedValue = OZO_CONTROL_FOURCC('f', 'l', 'o', 'i'),

    // ------------------------------------------------------


    /**
     * Encoder high-pass filter cutoff frequency controller related values.
     */

    /**
     * Value to set high-pass filter cutoff to off mode.
     */
    kHpFilterCutoffOffMode = OZO_CONTROL_FOURCC('h', 'p', 'f', 'o'),

    /**
     * Value to set high-pass filter cutoff to low mode.
     */
    kHpFilterCutoffLowMode = OZO_CONTROL_FOURCC('h', 'p', 'f', 'l'),

    /**
     * Value to set high-pass filter cutoff to mid mode.
     */
    kHpFilterCutoffMidMode = OZO_CONTROL_FOURCC('h', 'p', 'f', 'm'),

    /**
     * Value to set high-pass filter cutoff to high mode.
     */
    kHpFilterCutoffHighMode = OZO_CONTROL_FOURCC('h', 'p', 'f', 'h'),

    // ------------------------------------------------------


    /**
     * Wind noise reduction controller related values.
     */

    /**
     * Value to enable wind noise reduction processing and wind noise indication.
     */
    kWnrOnValue = OZO_CONTROL_FOURCC('w', 'n', 'r', '1'),

    /**
     * Value to disable wind noise reduction processing.
     */
    kWnrOffValue = OZO_CONTROL_FOURCC('w', 'n', 'r', '0'),

    /**
     * Wind noise reduction processing on/off toggle.
     */
    kWnrToggle = OZO_CONTROL_FOURCC('w', 'n', 'r', 't'),

    // ------------------------------------------------------


    /**
     * Encoder analysis sector related values.
     */

    /**
     *  Number of analysis sectors available.
     */
    kAnalysisSectorCount = OZO_CONTROL_FOURCC('s', 'e', 'c', 'c'),

    /**
     * ID value of analysis sector.
     */
    kAnalysisSectorQueryId = OZO_CONTROL_FOURCC('s', 'e', 'c', 'i'),

    /**
     * Azimuth value of analysis sector direction (-180.0 - +180.0).
     */
    kAnalysisSectorAzimuth = OZO_CONTROL_FOURCC('s', 'e', 'c', 'a'),

    /**
     * Elevation value of analysis sector direction (-90.0 - +90.0).
     */
    kAnalysisSectorElevation = OZO_CONTROL_FOURCC('s', 'e', 'c', 'e'),

    /**
     * Width value of analysis sector (0.0 - +360.0).
     */
    kAnalysisSectorWidth = OZO_CONTROL_FOURCC('s', 'e', 'c', 'w'),

    /**
     * Height value of analysis sector (0.0 - +180.0).
     */
    kAnalysisSectorHeight = OZO_CONTROL_FOURCC('s', 'e', 'c', 'h'),


    // ------------------------------------------------------

    /**
     * Encoder custom controller related values.
     */

    /**
     * Custom control mode selector.
     */

    kCustomMode = OZO_CONTROL_FOURCC('c', 'u', 's', 'm'),


    /**
     * Value to set custom control to mode0.
     */
    kCustomMode0Value = OZO_CONTROL_FOURCC('c', 'u', 's', '0'),

    /**
     * Value to set custom control to mode1.
     */
    kCustomMode1Value = OZO_CONTROL_FOURCC('c', 'u', 's', '1'),

    /**
     * Value to set custom control to mode2.
     */
    kCustomMode2Value = OZO_CONTROL_FOURCC('c', 'u', 's', '2'),


    // ------------------------------------------------------

    /**
     * Noise suppression controller related values.
     */

    /**
     * Value to enable smart noise suppression processing.
     */
    kNsSmartValue = OZO_CONTROL_FOURCC('a', 'n', 's', 's'),

    /**
     * Value to enable noise suppression processing.
     */
    kNsOnValue = OZO_CONTROL_FOURCC('a', 'n', 's', '1'),

    /**
     * Value to disable noise suppression processing.
     */
    kNsOffValue = OZO_CONTROL_FOURCC('a', 'n', 's', '0'),
};

/**
 * Generic controller interface for audio codec. The interface consists of setter and getter methods
 * that can be used to control the codec input and/or output behaviour. Each setter and/or getter
 * requires controller ID as first input parameter. Currently available controllers are listed in
 * \link #CodecControllerID CodecControllerId\endlink. Depending on the codec setup and whether the
 * interface belongs to audio encoder or decoder instance, only a subset of the controllers may be
 * present. If certain controller is not available, approriate error code is returned to indicate
 * this (AudioCodecError::CTRL_HANDLE_NOT_FOUND).
 *
 * @code
 *
 * double val;
 * AudioControl *ctrl = <controller-instance>;
 *
 * // Example: AGC mode
 * // -----------------
 *
 * uint32_t agc_mode = ozoaudiocodec::kAgcUserMode;
 *
 * // Set AGC mode
 * if (ctrl->setState(ozoaudiocodec::kAGCId, agc_mode) != ozoaudiocodec::AudioCodecError::OK)
 *    ; // Error handling
 *
 * // Get AGC mode
 * if (ctrl->getState(ozoaudiocodec::kAGCId, agc_mode) != ozoaudiocodec::AudioCodecError::OK)
 *    ; // Error handling
 *
 * // Get AGC gain
 * if (ctrl->getState(ozoaudiocodec::kOutChannelId, (uint32_t) ozoaudiocodec::kChannelGain, val) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling else ; // AGC gain value available in
 * variable val
 *
 *
 * // Example: Listener orientation
 * // -----------------------------
 *
 * if (ctrl->setState(ozoaudiocodec::kOrientationId, (uint32_t) ozoaudiocodec::kOrientationYaw,
 * <yaw-value>) == ozoaudiocodec::AudioCodecError::OK) { if
 * (ctrl->setState(ozoaudiocodec::kOrientationId, (uint32_t) ozoaudiocodec::kOrientationPitch,
 * <pitch-value>) == ozoaudiocodec::AudioCodecError::OK) { if
 * (ctrl->setState(ozoaudiocodec::kOrientationId, (uint32_t) ozoaudiocodec::kOrientationRoll,
 * <roll-value>) != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *    }
 *    else
 *        ; // Error handling
 * }
 * else
 *    ; // Error handling
 *
 *
 * // Example: Output channels control
 * // --------------------------------
 *
 * // Set global gain for output channels
 * if (ctrl->setState(ozoaudiocodec::kOutChanneld, (uint32_t) ozoaudiocodec::kChannelGain, 6.0) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Get global gain of output channels
 * if (ctrl->getState(ozoaudiocodec::kOutChanneld, (uint32_t) ozoaudiocodec::kChannelGain, val) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling else ; // Global gain value available in
 * variable val
 *
 * // Set gain for specific output channel (0)
 * if (ctrl->setState(ozoaudiocodec::kOutChanneld, (uint32_t) ozoaudiocodec::kChannelGain, 0, 6.0)
 * != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Get gain for specific output channel (3)
 * if (ctrl->getState(ozoaudiocodec::kOutChanneld, (uint32_t) ozoaudiocodec::kChannelGain, 3, val)
 * != ozoaudiocodec::AudioCodecError::OK) ; // Error handling else ; // Channel (3) gain available
 * in variable val
 *
 * // Mute output
 * if (ctrl->setState(ozoaudiocodec::kOutChannelId, (uint32_t) ozoaudiocodec::kChannelMute,
 * ozoaudiocodec::kMuteValue) != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Unmute output
 * if (ctrl->setState(ozoaudiocodec::kOutChannelId, (uint32_t) ozoaudiocodec::kChannelMute,
 * ozoaudiocodec::kUnMuteValue) != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 *
 * // Example: Focus direction control for audio capture
 * // ----------------------------------
 *
 * // Set focus direction processing on
 * if (ctrl->setState(ozoaudiocodec::kFocusId, (uint32_t) ozoaudiocodec::kFocusMode,
 * ozoaudiocodec::kFocusOnMode) != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Set focus direction processing off
 * if (ctrl->setState(ozoaudiocodec::kFocusId, (uint32_t) ozoaudiocodec::kFocusMode,
 * ozoaudiocodec::kFocusOffMode) != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Set focus processing gain to default value
 * if (ctrl->setState(ozoaudiocodec::kFocusId, (uint32_t) ozoaudiocodec::kFocusGain,
 * kFocusDefaultGain) != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Set arbitrary focus processing gain value
 * if (ctrl->setState(ozoaudiocodec::kFocusId, (uint32_t) ozoaudiocodec::kFocusGain, 2.5) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Set focus azimuth
 * if (ctrl->setState(ozoaudiocodec::kFocusId, (uint32_t) ozoaudiocodec::kFocusAzimuth, 180.0) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Set focus elevation
 * if (ctrl->setState(ozoaudiocodec::kFocusId, (uint32_t) ozoaudiocodec::kFocusElevation, 0.0) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Set focus sector width
 * if (ctrl->setState(ozoaudiocodec::kFocusId, (uint32_t) ozoaudiocodec::kFocusSectorWidth, 100.0)
 * != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Set focus sector height
 * if (ctrl->setState(ozoaudiocodec::kFocusId, (uint32_t) ozoaudiocodec::kFocusSectorHeight, 100.0)
 * != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 *
 * // Example: High-pass filter cutoff frequency mode
 * // -----------------
 *
 * uint32_t hp_filter_mode = ozoaudiocodec::kHpFilterCutoffMidMode;
 *
 * // Set cutoff frequency mode
 * if (ctrl->setState(ozoaudiocodec::kHpFilterCutoffId, hp_filter_mode) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * // Get cutoff frequency mode
 * if (ctrl->getState(ozoaudiocodec::kHpFilterCutoffId, hp_filter_mode) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 *
 * // Example: Wind noise reduction control on/off toggle
 * // -----------------
 *
 *
 * // Enable wind noise processing
 * if (ctrl->setState(ozoaudiocodec::kWnrId, ozoaudiocodec::kWnrToggle, ozoaudiocodec::kWnrOnValue)
 * != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * if (ctrl->getState(ozoaudiocodec::kWnrId, ozoaudiocodec::kWnrToggle, val) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 *
 * // Example: Analysis sector value query
 * // -----------------
 *
 * // Get number of analysis sectors
 * int32_t nSectors;
 * if (ctrl->getState(ozoaudiocodec::kAnalysisSectorId, ozoaudiocodec::kAnalysisSectorCount,
 * nSectors) != ozoaudiocodec::AudioCodecError::OK) ; // Error handling else ;
 *
 * // Get azimuth value of first sector
 * int32_t sectorIndex = 0;
 * double azimuth;
 * if (ctrl->getState(ozoaudiocodec::kAnalysisSectorId, sectorIndex,
 * ozoaudiocodec::kAnalysisSectorAzimuth, azimuth) != ozoaudiocodec::AudioCodecError::OK) ;
 * // Error handling
 *
 *
 * // Example: Custom control mode
 * // -----------------
 *
 * // Set custom processing mode
 * if (ctrl->setState(ozoaudiocodec::kCustomId, ozoaudiocodec::kCustomMode,
 * ozoaudiocodec::kCustomMode0Value)
 * != ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * if (ctrl->getState(ozoaudiocodec::kCustomId, ozoaudiocodec::kCustomMode, val) !=
 * ozoaudiocodec::AudioCodecError::OK) ; // Error handling
 *
 * @endcode
 */
class AudioControl
{
public:
    /**
     * Set controller state.
     */
    virtual ozoaudiocodec::AudioCodecError setState(CodecControllerID id, double value) = 0;

    /**
     * Set controller state.
     */
    virtual ozoaudiocodec::AudioCodecError setState(CodecControllerID id, uint32_t value) = 0;

    /**
     * Set controller state.
     */
    virtual ozoaudiocodec::AudioCodecError setState(CodecControllerID id,
                                                    uint32_t selector_id,
                                                    double value) = 0;

    /**
     * Set controller state.
     */
    virtual ozoaudiocodec::AudioCodecError setState(CodecControllerID id,
                                                    uint32_t selector_id,
                                                    int32_t value) = 0;

    /**
     * Set controller state.
     */
    virtual ozoaudiocodec::AudioCodecError setState(CodecControllerID id,
                                                    uint32_t selector_id,
                                                    uint32_t selector_sub_id,
                                                    double value) = 0;

    /**
     * Set controller state.
     */
    virtual ozoaudiocodec::AudioCodecError setState(CodecControllerID id,
                                                    uint32_t selector_id,
                                                    uint32_t selector_sub_id,
                                                    int32_t value) = 0;

    /**
     * Retrieve controller state.
     */
    virtual ozoaudiocodec::AudioCodecError getState(CodecControllerID id, double &value) = 0;

    /**
     * Retrieve controller state.
     */
    virtual ozoaudiocodec::AudioCodecError getState(CodecControllerID id, uint32_t &value) = 0;

    /**
     * Retrieve controller state.
     */
    virtual ozoaudiocodec::AudioCodecError getState(CodecControllerID id,
                                                    uint32_t selector_id,
                                                    double &value) = 0;

    /**
     * Retrieve controller state.
     */
    virtual ozoaudiocodec::AudioCodecError getState(CodecControllerID id,
                                                    uint32_t selector_id,
                                                    int32_t &value) = 0;

    /**
     * Retrieve controller state.
     */
    virtual ozoaudiocodec::AudioCodecError getState(CodecControllerID id,
                                                    uint32_t selector_id,
                                                    uint32_t selector_sub_id,
                                                    double &value) = 0;

    /**
     * Retrieve controller state.
     */
    virtual ozoaudiocodec::AudioCodecError getState(CodecControllerID id,
                                                    uint32_t selector_id,
                                                    uint32_t selector_sub_id,
                                                    int32_t &value) = 0;

protected:
    /** @cond PRIVATE */
    AudioControl(){};
    virtual ~AudioControl(){};
    /** @endcond */
};

} // namespace ozoaudiocodec
