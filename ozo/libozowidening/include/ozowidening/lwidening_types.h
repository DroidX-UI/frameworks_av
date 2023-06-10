#ifndef OZO_LOUDSPEAKER_WIDENING_TYPES_H
#define OZO_LOUDSPEAKER_WIDENING_TYPES_H

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

#include "widening_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file

  @ingroup lwidening

  @brief Ozo Loudspeaker Widening algorithm types

  This header defines widening algorithm types.
*/

/** @brief Device orientations

  Device orientations defined
*/
typedef enum LWidening_DeviceOrientation
{
    /** Used when the phone is in the normal orientation (earpiece to the left, bottom speaker to the right). */
    LWIDENING_ORIENTATION_LANDSCAPE = 0,
    /** Used when the phone is turned upside down (earpiece to the right, bottom speaker to the left). */
    LWIDENING_ORIENTATION_REVERSE_LANDSCAPE = 1

} LWidening_DeviceOrientation;


#ifdef __cplusplus
}
#endif

#endif /* OZO_LOUDSPEAKER_WIDENING_TYPES_H */
