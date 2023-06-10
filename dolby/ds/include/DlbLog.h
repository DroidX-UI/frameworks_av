/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2014-2018 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/

#ifndef DOLBY_DLB_LOG_H_
#define DOLBY_DLB_LOG_H_

/*
    Logging primitives:
        LOG_FATAL - Log message and force process to quit (always available)
        ALOG_ASSERT - Similar to LOG_FATAL but only in debug builds
        ALOGE - Log error message (always available)
        ALOGW - Log warning message (always available)
        ALOGI - Log information message (available from DOLBY_LOG_LEVEL=1)
        ALOGD - Log debug messages (available from DOLBY_LOG_LEVEL=2)
        ALOGV - Log verbose message (available from DOLBY_LOG_LEVEL=3)
        ALOGVV - Log messages potentially clobbering the logging
                 service (enabled by defining DOLBY_LOG_LEVEL_CLOBBER)
*/

#ifdef DOLBY_LOG_DYNAMIC

#include <cutils/properties.h>
#include <log/log.h>

static inline int dlbLogLevel() {
    return property_get_int32("persist.vendor.dolby.loglevel", 2);
}

#undef ALOGI
#undef ALOGI_IF
#undef IF_ALOGI
#define ALOGI(...) if (dlbLogLevel() > 0) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#define ALOGI_IF(cond, ...) if (cond && dlbLogLevel() > 0) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#define IF_ALOGI() if (dlbLogLevel() > 0)

#undef ALOGD
#undef ALOGD_IF
#undef IF_ALOGD
#define ALOGD(...) if (dlbLogLevel() > 1) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define ALOGD_IF(cond, ...) if (cond && dlbLogLevel() > 1) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define IF_ALOGD() if (dlbLogLevel() > 1)

#undef ALOGV
#undef ALOGV_IF
#undef IF_ALOGV
#define ALOGV(...) if (dlbLogLevel() > 2) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define ALOGV_IF(cond, ...) if (cond && dlbLogLevel() > 2) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define IF_ALOGV() if (dlbLogLevel() > 2)

#undef ALOGVV
#undef ALOGVV_IF
#undef IF_ALOGVV
#define ALOGVV(...) if (dlbLogLevel() > 3) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define ALOGVV_IF(cond, ...) if (cond && dlbLogLevel() > 3) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#define IF_ALOGVV() if (dlbLogLevel() > )

#else //static build logs

#if DOLBY_LOG_LEVEL < 3
#define ALOGV(...)    ((void)0)
#define ALOGV_IF(...) ((void)0)
#define IF_ALOGV()    if (false)
#else
#define LOG_NDEBUG 0
#endif

#if DOLBY_LOG_LEVEL < 2
#define ALOGD(...)    ((void)0)
#define ALOGD_IF(...) ((void)0)
#define IF_ALOGD()    if (false)
#endif

#if DOLBY_LOG_LEVEL < 1
#define ALOGI(...)    ((void)0)
#define ALOGI_IF(...) ((void)0)
#define IF_ALOGI()    if (false)
#endif

#include <utils/Log.h>

#ifdef DOLBY_LOG_LEVEL_CLOBBER
#define ALOGVV    ALOGV
#define ALOGVV_IF ALOGV_IF
#define IF_ALOGVV IF_ALOGV
#else
#define ALOGVV(...)    ((void)0)
#define ALOGVV_IF(...) ((void)0)
#define IF_ALOGVV()    if (false)
#endif

#endif //end if DOLBY_LOG_DYNAMIC

#endif//DOLBY_DLB_LOG_H_
