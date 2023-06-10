#ifndef OZO_WIDENING_TYPES_H
#define OZO_WIDENING_TYPES_H

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

#ifdef __cplusplus
extern "C" {
#endif

/** @file

  @ingroup widening

  @brief Ozo Widening algorithm common types

  This header defines widening algorithm types.
*/

/** @brief Return codes

  Return codes defined
*/
typedef enum Widening_ReturnCode
{
    WIDENING_RETURN_SUCCESS = 0,
    WIDENING_RETURN_MEMORY_ALLOCATION_FAIL = -1,
    WIDENING_RETURN_INVALID_SAMPLING_RATE = -2,
    WIDENING_RETURN_INVALID_DEVICE_ID = -3,
    WIDENING_RETURN_FAIL = -4,
    WIDENING_RETURN_INVALID_BLOCKSIZE = -5,
    WIDENING_RETURN_INVALID_FLAG = -6,
    WIDENING_RETURN_INVALID_ARGUMENT = -7,
    WIDENING_RETURN_INVALID_INPUT_CHANNEL_COUNT = -8,
    WIDENING_RETURN_INVALID_LICENSE = -9,
    WIDENING_RETURN_PERMISSION_ERROR = -10

} Widening_ReturnCode;


/** @brief Processing modes

  Processing modes returned in a struct. Struct member modeName is a pointer to an array of pointers to
  individual mode names in char arrays, modeCount is the number of modes available and maxModeStringSize
  is the max size of an individual char array cointaining the name of the mode. First mode is always a
  bypass mode. The first mode name is const char * firstMode = modeName[0];

  User must not allocate memory for the modeName nor free the memory.
*/
typedef struct Widening_ProcessingModes
{
    const char *const *modeName;
    int modeCount;
    int maxModeStringSize;

} Widening_ProcessingModes;


/** @brief Initialization flags

 Initialization flags
*/
typedef enum Widening_InitFlags
{
    /** Low delay mode flag can be used to achieve shortest possible processing delay. Use of this flag requires
    constant frame size of 512 samples to be provided as maxFrameSize param in widening config and calling
    process function with frameSize value of 512 samples. */
    WIDENING_FLAG_LOW_DELAY_MODE = 0x1

} Widening_InitFlags;

#ifdef __cplusplus
}
#endif

#endif /* OZO_WIDENING_TYPES_H */
