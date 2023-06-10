#ifndef OZO_LOUDSPEAKER_WIDENING_CONFIG_H
#define OZO_LOUDSPEAKER_WIDENING_CONFIG_H

/*
Copyright (C) 2018 Nokia Technologies.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Technologies. All rights are reserved. Copying,
including reproducing, storing,  adapting or translating, any
or all of this material requires the prior written consent of
Nokia Technologies. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Technologies.
*/

#define WIDENING_NAME_ID_LENGTH_MAX 37

#ifdef __cplusplus
extern "C" {
#endif

/** @file

  @ingroup widening

  @brief Widening algorithm configuration parameters

  This header defines configuration params for the widening algorithm.
*/

/** @brief Structure for passing widening configuration parameters

  This struct is used to pass the adjustable parameters of widening.
  These are initialization parameters and cannot be changed during run time.
*/
typedef struct LWideningConfig
{
    /** Audio sampling rate in Hz. The supported values are: 48000, 44100 */
    int fs;

    /** Device UUID as '\0' null terminated char array */
    char deviceNameId[WIDENING_NAME_ID_LENGTH_MAX];

    /** Maximum size of input and output audio frame.
        Largest acceptable size is 8192 */
    int maxFrameSize;

    /** Number of input channels (2 or 4) */
    int inputChannelCount;

    /** Special initialization flags defined in Widening_InitFlags */
    unsigned int flags;

} LWideningConfig;

#ifdef __cplusplus
}
#endif

#endif /* OZO_LOUDSPEAKER_WIDENING_CONFIG_H */
