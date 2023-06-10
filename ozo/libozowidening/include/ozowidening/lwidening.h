#ifndef OZO_LOUDSPEAKER_WIDENING_H
#define OZO_LOUDSPEAKER_WIDENING_H

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

#include "lwidening_types.h"
#include "widening_library_export.h"
#include "lwidening_params.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file

  @ingroup widening

  @brief Loudspeaker widening public API

  These functions implement the loudspeaker widening public API. The API is not thread-safe, and thus
  all functions must be called from the same thread.
*/

/** @brief Structure for widening instance

*/
typedef struct LEngine *LWideningPtr;

/** @brief Allocate memory and initialize the widening instance data

  This function allocates memory and initializes a widening instance.
  It is not legal to call any other functions taking LWideningPtr instance as an argument
  (excluding OzoLoudspeakerWidening_Reset()) before successful creation of widening instance.

  @param[in,out] instance     Algorithm instance data
  @param[in] params           Algorithm configuration parameters structure
  @return WIDENING_RETURN_SUCCESS on success, error code on failure.
*/
WIDENINGAPI Widening_ReturnCode OzoLoudspeakerWidening_Create(LWideningPtr *instance,
                                                              const LWideningConfig *config);

/** @brief Deinitialize and deallocate the widening algorithm instance data

  This function deinitializes a widening algorithm instance and releases allocated memory.

  @param[in,out] instance     widening algorithm instance data
*/
WIDENINGAPI void OzoLoudspeakerWidening_Destroy(LWideningPtr instance);

/** @brief Reset the widening algorithm instance data

  This function resets the widening algorithm instance including all internal audio buffers.
  Reset can be safely called in between two consecutive OzoLoudspeakerWidening_Process() function calls.

  @param[in,out] instance     widening algorithm instance data
*/
WIDENINGAPI void OzoLoudspeakerWidening_Reset(LWideningPtr instance);

/** @brief Get the maximum frame size in samples

  This function returns the maximum frame size of input and output signal buffers.
  The total size of input buffers is inputChannelCount * OzoLoudspeakerWidening_GetFrameSize().

  @param[in] instance     widening algorithm instance data
  @return frame size as integer value
*/
WIDENINGAPI int OzoLoudspeakerWidening_GetFrameSize(const LWideningPtr instance);

/** @brief Get all available processing modes

  This function returns available processing modes in LWidening_ProcessingModes struct.

  @param[in] instance     widening algorithm instance data
  @return LWidening_ProcessingModes
*/
WIDENINGAPI Widening_ProcessingModes
OzoLoudspeakerWidening_ProcessingModesAvailable(const LWideningPtr instance);

/** @brief Set the processing mode

  This function sets the processing mode of widening.

  @param[in,out] instance   Algorithm instance data
  @param[in]     mode       Processing mode as const char*
*/
WIDENINGAPI Widening_ReturnCode OzoLoudspeakerWidening_SetProcessingMode(LWideningPtr instance,
                                                                         const char *mode);

/** @brief Get the current processing mode

  This function returns the name of the current processing mode of widening.

  @param[in]    instance   Algorithm instance data
  @return       current processing mode as const char*
*/
WIDENINGAPI const char *OzoLoudspeakerWidening_GetProcessingMode(const LWideningPtr instance);

/** @brief Set the device orientation

  This function sets the device orientation.

  @param[in,out] instance       Algorithm instance data
  @param[in]     orientation    Device orientation
*/
WIDENINGAPI void
OzoLoudspeakerWidening_SetDeviceOrientation(LWideningPtr instance,
                                            LWidening_DeviceOrientation orientation);

/** @brief Get the device orientation

  This function returns the device orientation value.

  @param[in]    instance   Algorithm instance data
  @return LWidening_DeviceOrientation
*/
WIDENINGAPI LWidening_DeviceOrientation
OzoLoudspeakerWidening_GetDeviceOrientation(const LWideningPtr instance);

/** @brief Get the delay

  This function returns the extra delay imposed by widening processing in samples. The delay depends on
  the sampling rate.

  @param[in]    instance   Algorithm instance data
  @return delay as integer value
*/
WIDENINGAPI int OzoLoudspeakerWidening_GetDelay(const LWideningPtr instance);

/** @brief Processes signals with widening algorithm

  This function processes the signals (e.g. music) with the widening algorithm. The input and output buffer
  consist of interleaved stereo samples. The number of stereo samples to be processed is defined by frameSize
  argument. Different frameSize value may be provided for consecutive calls to this function. The maximum
  size of the input and output buffers in stereo samples is returned by the function
  OzoLoudspeakerWidening_GetFrameSize(). Therefore, the maximum size of both the input and output buffers is
  the same: 2 * OzoLoudspeakerWidening_GetFrameSize().

  @param[in,out] instance   Algorithm instance data
  @param[in]     in         Input signal buffer
  @param[out]    out        Output signal buffer
  @param[in]     frameSize  Frame size in stereo samples
*/
WIDENINGAPI void
OzoLoudspeakerWidening_Process(LWideningPtr instance, const float *in, float *out, int frameSize);

/** @brief Get API version

  This function returns a pointer to static char array containing version information. It is legal to call this
  function prior to calling OzoLoudspeakerWidening_Create().

  @return const char * version
*/
WIDENINGAPI const char *OzoLoudspeakerWidening_GetVersion();

#ifdef __cplusplus
}
#endif

#endif /* OZO_LOUDSPEAKER_WIDENING_H */
