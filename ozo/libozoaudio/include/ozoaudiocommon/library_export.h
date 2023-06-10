#pragma once
/*
Copyright (C) 2016 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

#if defined(OZO_AUDIO_SDK_SHARED_LIBRARY) && defined(WIN32)
#if defined(OZO_AUDIO_SDK_API_BUILD)
#define OZO_AUDIO_SDK_PUBLIC __declspec(dllexport)
#else
#define OZO_AUDIO_SDK_PUBLIC __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define OZO_AUDIO_SDK_PUBLIC __attribute__((visibility("default")))
#else
#define OZO_AUDIO_SDK_PUBLIC
#endif
#endif