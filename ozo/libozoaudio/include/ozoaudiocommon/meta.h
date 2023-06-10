#pragma once
/*
  Copyright (C) 2018 Nokia Corporation.
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
 * @brief Generic meta data interface.
 */

#include <stdint.h>

namespace ozoaudiocodec {

class OzoAudioMeta
{
public:
    /**
     * Assign 32-bit integer value for specified data identifier.
     *
     * @param key Data identifier.
     * @param value Data value.
     * @return true on success, false otherwise.
     *
     * @code
     *
     * OzoAudioMeta meta;
     *
     * // Set value 3 for data identifier "ozo"
     * if (!meta.setInt32Data("ozo", 3)) {
     *     // Error handling here
     * }
     *
     * @endcode
     */
    virtual bool setInt32Data(const char *key, int32_t value) = 0;

    /**
     * Retrieve 32-bit integer value assigned for specific data identifier.
     *
     * @param key Data identifier.
     * @param [out] value Data value.
     * @return true on success, false otherwise.
     *
     * @code
     *
     * int32_t value;
     * if (!meta.getInt32Data("ozo", value)) {
     *     // Data does not exist, error handling here
     * }
     *
     * @endcode
     */
    virtual bool getInt32Data(const char *key, int32_t &val) const = 0;

    /**
     * Assign string value for specified data identifier.
     *
     * @param key Data identifier.
     * @param value String value.
     * @return true on success, false otherwise.
     *
     * @code
     *
     * OzoAudioMeta meta;
     *
     * // Set value "audio" for data identifier "ozo"
     * if (!meta.setInt32Data("ozo", "audio")) {
     *     // Error handling here
     * }
     *
     * @endcode
     */
    virtual bool setString(const char *key, const char *value) = 0;

    /**
     * Retrieve string value assigned for specific data identifier.
     *
     * @param key Data identifier.
     * @return String value or NULL if value is not specified.
     *
     * @code
     * auto value = meta.getStringData("ozo");
     * @endcode
     */
    virtual const char *getString(const char *key) const = 0;

    /**
     * Check object's write state.
     *
     * @return true if object is in read only state, false otherwise.
     */
    virtual bool isReadOnly() const = 0;

protected:
    /** @cond PRIVATE */
    OzoAudioMeta(){};
    virtual ~OzoAudioMeta(){};
    /** @endcond */
};

} // namespace ozoaudiocodec
