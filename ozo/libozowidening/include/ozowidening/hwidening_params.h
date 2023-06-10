#ifndef OZO_HEADSET_WIDENING_CONFIG_H
#define OZO_HEADSET_WIDENING_CONFIG_H

/*
Copyright (C) 2019 Nokia Technologies.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Technologies. All rights are reserved. Copying,
including reproducing, storing,  adapting or translating, any
or all of this material requires the prior written consent of
Nokia Technologies. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Technologies.
*/

#ifdef __cplusplus
extern "C" {
#endif

/** @file

  @ingroup hwidening

  @brief HWidening algorithm configuration parameters

  This header defines configuration params for the headset widening algorithm.
*/

/** @brief Structure for passing headset widening configuration parameters

  This struct is used to pass the adjustable parameters of headset widening.
  These are initialization parameters and cannot be changed during run time.
*/
typedef struct HWideningConfig
{
    /** Audio sampling rate in Hz. The supported values are: 48000 */
    int fs;

    /** Maximum size of input and output audio frame in stereo samples.
        Largest acceptable size is 8192.
    */
    int maxFrameSize;

    /** Special initialization flags defined in Widening_InitFlags */
    unsigned int flags;

} HWideningConfig;

#ifdef __cplusplus
}
#endif

#endif /* OZO_HEADSET_WIDENING_CONFIG_H */
