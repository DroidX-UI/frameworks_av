#ifndef OZO_LICENSE_H
#define OZO_LICENSE_H

/*
Copyright (C) 2020 Nokia Technologies.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Technologies. All rights are reserved. Copying,
including reproducing, storing,  adapting or translating, any
or all of this material requires the prior written consent of
Nokia Technologies. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Technologies.
*/

#include "widening_library_export.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file

  @brief License file setting.

  This function implements the setting of the license file for loudspeaker and headset widening.
*/

/** @brief Set playback license.

  The widening will not work without the license assignment.
  This should be done only once!

  @param[in] license   Playback license data buffer.
  @param[in] size      Size of license data.
  @return true if license was accepted, false otherwise.
*/

WIDENINGAPI bool OzoWidening_SetLicense(const char *license, size_t size);


#ifdef __cplusplus
}
#endif

#endif
