#ifndef OZO_WIDENING_LIBRARY_EXPORT_H
#define OZO_WIDENING_LIBRARY_EXPORT_H

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

#if defined(WIDENING_SHARED_LIBRARY) && defined(WIN32)
#if defined(WIDENING_API_BUILD)
#define WIDENINGAPI __declspec(dllexport)
#else
#define WIDENINGAPI __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define WIDENINGAPI __attribute__((visibility("default")))
#else
#define WIDENINGAPI
#endif
#endif

#endif /* OZO_WIDENING_LIBRARY_EXPORT_H */
