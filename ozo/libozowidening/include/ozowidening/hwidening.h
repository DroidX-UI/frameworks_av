#ifndef OZO_HEADSET_WIDENING_H
#define OZO_HEADSET_WIDENING_H

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

#include "hwidening_types.h"
#include "widening_library_export.h"
#include "hwidening_params.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file

  @ingroup hwidening

  @brief Headset widening public API

  These functions implement the headset widening public API.
  The API is not thread-safe, and thus
  all functions must be called from the same thread.
*/

/** @brief Structure for headset widening instance

*/
typedef struct HEngine *HWideningPtr;

/** @brief Allocate memory and initialize the headset widening instance data

  This function allocates memory and initializes a headset widening instance.
  It is not legal to call any other functions taking HWideningPtr instance as an argument
  before successful creation of headset widening instance.

  @param[in,out] instance     Algorithm instance data
  @param[in] config           Algorithm configuration parameters structure
  @return HWIDENING_RETURN_SUCCESS on success, error code on failure.
*/
WIDENINGAPI Widening_ReturnCode OzoHeadsetWidening_Create(HWideningPtr *instance,
                                                            const HWideningConfig *config);

/** @brief Deinitialize and deallocate the headset widening algorithm instance data

  This function deinitializes a headset widening algorithm instance and releases allocated memory.

  @param[in,out] instance     headset widening algorithm instance data
*/
WIDENINGAPI void OzoHeadsetWidening_Destroy(HWideningPtr instance);

/** @brief Reset the headset widening algorithm instance data

  This function resets the headset widening algorithm instance including all internal audio buffers.
  Reset can be safely called in between two consecutive OzoHeadsetWidening_Process() function calls.

  @param[in,out] instance     headset widening algorithm instance data
*/
WIDENINGAPI void OzoHeadsetWidening_Reset(HWideningPtr instance);

/** @brief Get the maximum frame size in stereo samples

  This function returns the maximum frame size of input and output signal buffers in stereo samples.
  Therefore, the total size of these buffers is 2 * OzoHeadsetWidening_GetFrameSize().

  @param[in] instance     headset widening algorithm instance data
  @return frame size as integer value
*/
WIDENINGAPI int OzoHeadsetWidening_GetFrameSize(const HWideningPtr instance);

/** @brief Get all available processing modes

  This function returns available processing modes in Widening_ProcessingModes struct.

  @param[in] instance     headset widening algorithm instance data
  @return LWidening_ProcessingModes
*/
WIDENINGAPI Widening_ProcessingModes
OzoHeadsetWidening_ProcessingModesAvailable(const HWideningPtr instance);

/** @brief Set the processing mode

  This function sets the processing mode of headset widening.

  @param[in,out] instance   Algorithm instance data
  @param[in]     mode       Processing mode as const char*
*/
WIDENINGAPI Widening_ReturnCode OzoHeadsetWidening_SetProcessingMode(HWideningPtr instance,
                                                                       const char *mode);

/** @brief Get the current processing mode

  This function returns the name of the current processing mode of headset widening.

  @param[in]    instance   Algorithm instance data
  @return       current processing mode as const char*
*/
WIDENINGAPI const char *OzoHeadsetWidening_GetProcessingMode(const HWideningPtr instance);

/** @brief Get the delay

  This function returns the extra delay imposed by headset widening processing in samples. The delay depends on
  the sampling rate.

  @param[in]    instance   Algorithm instance data
  @return delay as integer value
*/
WIDENINGAPI int OzoHeadsetWidening_GetDelay(const HWideningPtr instance);

/** @brief Processes signals with headset widening algorithm

  This function processes the signals (e.g. music) with the headset widening algorithm. The input and output buffer
  consist of interleaved stereo samples. The number of stereo samples to be processed is defined by frameSize
  argument. The maximum size of the input and output buffers in stereo samples is returned by the function
  OzoHeadsetWidening_GetFrameSize(). Therefore, the maximum size of both the input and output buffers is
  the same: 2 * OzoHeadsetWidening_GetFrameSize().

  @param[in,out] instance   Algorithm instance data
  @param[in]     in         Input signal buffer
  @param[out]    out        Output signal buffer
  @param[in]     frameSize  Frame size in stereo samples
*/
WIDENINGAPI void
OzoHeadsetWidening_Process(HWideningPtr instance, const float *in, float *out, int frameSize);
/** @brief Get API version

  This function returns a pointer to static char array containing version information. It is legal to call this
  function prior to calling OzoHeadsetWidening_Create().

  @return const char * version
*/
WIDENINGAPI const char *OzoHeadsetWidening_GetVersion();

#ifdef __cplusplus
}
#endif

#endif /* OZO_HEADSET_WIDENING_H */
