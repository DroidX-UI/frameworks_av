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
 * @brief Channel layouts for OZO Audio codec.
 */

#include <stdint.h>

#ifndef OZO_AUDIO_CH_LAYOUT_H_
#define OZO_AUDIO_CH_LAYOUT_H_

#define OZO_AUDIO_MONO_LAYOUT "mono"
#define OZO_AUDIO_STEREO_LAYOUT "stereo"
#define OZO_AUDIO_QUAD_LAYOUT "quad"
#define OZO_AUDIO_40_LAYOUT "4.0"
#define OZO_AUDIO_50_LAYOUT "5.0"
#define OZO_AUDIO_50_PROTOOLS_LAYOUT "5.0_protools"
#define OZO_AUDIO_50_ISO_LAYOUT "5.0_iso"
#define OZO_AUDIO_51_LAYOUT "5.1"
#define OZO_AUDIO_51_ISO_LAYOUT "5.1_iso"
#define OZO_AUDIO_70_LAYOUT "7.0"
#define OZO_AUDIO_70_NUENDO_LAYOUT "7.0_nuendo"
#define OZO_AUDIO_71_ISO_LAYOUT "7.1_iso"
#define OZO_AUDIO_71_ISO_OZO_LAYOUT "7.1_iso_ozo"
#define OZO_AUDIO_80_LAYOUT "8.0"
#define OZO_AUDIO_100_LAYOUT "10.0"
#define OZO_AUDIO_130_LAYOUT "13.0"
#define OZO_AUDIO_140_LAYOUT "14.0"
#define OZO_AUDIO_150_LAYOUT "15.0"
#define OZO_AUDIO_151_LAYOUT "15.1"
#define OZO_AUDIO_160_NAT_LAYOUT "16.0"
#define OZO_AUDIO_120_NAT_LAYOUT "12.0"
#define OZO_AUDIO_60_NAT_LAYOUT "6.0"

/**
 * Loudspeaker position expressed with azimuth and elevation values.
 */
typedef struct OzoLoudspeakerPos
{
    /**
     * Loudspeaker azimuth in degrees.
     */
    int16_t azimuth;

    /**
     * Loudspeaker elevation in degrees.
     */
    int8_t elevation;

} OzoLoudspeakerPos;

/**
 * Channel layout data. The data structure can be used to specify custom channel layout
 * configuration which is then passed to Ozo audio decoder for loudspeaker rendering.
 *
 * @code
 *
 * // Example: 4.1 loudspeaker layout
 * OzoLoudspeakerPos ls41[] = {
 *   {30, 0},   // Front left
 *   {0, 0},    // Center
 *   {-30, 0},  // Front right
 *   {180, 0},  // Back
 *   {0, 0}     // LFE
 * };
 *
 * ISOChannelLayout ch_layout_41 = {
 *   nullptr, 5, ls41, 4
 * };
 * @endcode
 */
typedef struct ISOChannelLayout
{
    /**
     * Name of layout.
     */
    const char *layout;

    /**
     * Number of loudspeaker positions.
     */
    int channels;

    /**
     * Loudspeaker positions.
     */
    const OzoLoudspeakerPos *exp_ls_pos;

    /**
     * LFE (Low-Frequency effects) channel index. Negative values indicates that LFE is not present.
     */
    int lfe_index;

} ISOChannelLayout;

#endif // OZO_AUDIO_CH_LAYOUT_H_
