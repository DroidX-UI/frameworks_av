/**
* This product contains one or more programs protected under international
* and U.S. copyright laws as unpublished works.  They are confidential and
* proprietary to Dolby Laboratories.  Their reproduction or disclosure, in
* whole or in part, or the production of derivative works therefrom without
* the express permission of Dolby Laboratories is prohibited.
* Copyright 2011 - 2016 by Dolby Laboratories.
* All rights reserved.
*
*/

#ifndef _DOVI_CONTROL_H_
#define _DOVI_CONTROL_H_

#ifdef _WINDOWS_DLL
    #ifdef DOVI_CONTROL_EXPORTS
        /* We are building this library */
        #define DOVI_CONTROL_EXPORT __declspec(dllexport)
    #else
        /* We are using this library */
        #define DOVI_CONTROL_EXPORT __declspec(dllimport)
    #endif
    #define DOVI_OTT_EXPORT
#elif defined(ANDROID)
    #define DOVI_OTT_EXPORT __attribute__((visibility("default")))
    #define DOVI_CONTROL_EXPORT
#else
    #define DOVI_CONTROL_EXPORT
    #define DOVI_OTT_EXPORT
#endif

#endif
