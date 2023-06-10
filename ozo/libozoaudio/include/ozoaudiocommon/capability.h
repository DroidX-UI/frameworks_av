#pragma once
/*
  Copyright (C) 2019 Nokia Corporation.
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
 * @brief OZO Audio capability interface.
 */

#include <stdint.h>
#include <stddef.h>

#include "ozoaudiocommon/library_export.h"

namespace ozoaudiocodec {

enum
{
    /**
     * Permission key to check if wind noise reduction is available.
     */
    kWnrPermission = 0,

    /**
     * Permission key to check if sound source localization is available.
     */
    kSourceLocalizationPermission,

    /**
     * Permission key to check if Audio Focus is available.
     */
    kAudioFocusPermission,

    /**
     * Number of input channels query as integer value.
     */
    kInputChannelsRequired,

    /**
     * Query if SDK license has been set.
     * Returns value 1 if license has been set, 0 otherwise.
     */
    kHasLicenseSet,

    /**
     * Permission key to check if noise suppression is available.
     */
    kNoiseSuppressionPermission,

    /**
     * Permission key to check if smart noise suppression is available.
     */
    kSmartNoiseSuppressionPermission,

    /**
     * Permission key to check if blocked mic indication is available.
     */
    kBlockedMicIndicationPermission,
};

/**
 * Interface for querying OZO Audio capabilities.
 */
class OZO_AUDIO_SDK_PUBLIC OzoAudioCapabilityManager
{
public:
    /** @name Construction and destruction of OzoAudioCapabilityManager */
    //@{
    /**
     * Create capability manager.
     *
     * @return Reference to OzoAudioCapabilityManager object.
     *
     * @code
     *
     * auto manager = OzoAudioCapabilityManager::create();
     *
     * @endcode
     */
    static const OzoAudioCapabilityManager &create();
    //@}

    /**
     * Query if specified permission has been granted to OZO Audio library.
     *
     * @param permKey Permission key.
     * @return true if permission available, false otherwise.
     *
     * @code
     * auto perm = manager->hasPermission(ozoaudiocodec::kWnrPermission);
     * @endcode
     */
    virtual bool hasPermission(size_t permKey) const = 0;

    /**
     * Query device specific values.
     *
     * @param uuid Device ID.
     * @param valueKey Query ID.
     * @param [out] value Query output value.
     * @return true on success, false otherwise.
     *
     * @code
     *
     * int nChannels;
     *
     * // Query number of input channels required
     * auto status = manager->getValue(
     *   "<device-id>",
     *   ozoaudiocodec::kInputChannelsRequired,
     *   nChannels
     * );
     *
     * if (status)
     *   // Number of input channels required is specified by nChannels variable
     *
     * @endcode
     */
    virtual bool getValue(const char *uuid, size_t valueKey, int &value) const = 0;

    /**
     * Query SDK values.
     *
     * @param valueKey Query ID.
     * @param [out] value Query output value.
     * @return true on success, false otherwise.
     *
     * @code
     *
     * int licenseSet;
     *
     * // Query if license has been set
     * auto status = manager->getValue(
     *   ozoaudiocodec::kHasLicenseSet,
     *   licenseSet
     * );
     *
     * if (status)
     *   // License status is specified by licenseSet variable
     *
     * @endcode
     */
    virtual bool getValue(size_t valueKey, int &value) const = 0;

    /**
     * Return version string that can be used to identify the release number and
     * other details of the library.
     *
     * @return String describing the library version.
     *
     * @code
     * auto version = OzoAudioCapabilityManager::version();
     * @endcode
     */
    static const char *version();

protected:
    /** @cond PRIVATE */
    OzoAudioCapabilityManager(){};
    virtual ~OzoAudioCapabilityManager(){};
    /** @endcond */
};

} // namespace ozoaudiocodec
