/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2020-2021 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/
#ifndef BINDING_BLOB_UTILS_IMPL_H
#define BINDING_BLOB_UTILS_IMPL_H

#include "BindingBlobTypes.h"
#include "DlbGenClass3.h"
#include <dlfcn.h>

namespace dolby {

#define BINDING_BLOB_SYMBOL_NAME "dd_data"

static BindingBlobStorage dd_data;

static inline int64_t getCustomizedParamBBH()
{
    return dd_data.split.high;
}

static inline int64_t getCustomizedParamBBL()
{
    return dd_data.split.low;
}

} // namespace dolby

#if 1
#define READ_BINDING_BLOB() \
{\
    char fileStr[VENDOR_BINDING_BLOB_LIBRARY_PATH_LEN+1] = {'\0'};\
    dlbGenClass3Func(dlbGenClass3Str1, fileStr);\
    void* __h = dlopen(fileStr, RTLD_NOW);\
    if (__h)\
    {\
        void* bb_data = dlsym(__h, BINDING_BLOB_SYMBOL_NAME);\
        if (bb_data)\
        {\
            memcpy((void*)dd_data.raw, bb_data, BINDING_BLOB_SIZE);\
        }\
        else\
        {\
            ALOGI("DLBS init info log #2");\
        }\
        dlclose(__h);\
    }\
    else\
    {\
        ALOGI("DLBS init info log #1");\
    }\
}

#else
#define READ_BINDING_BLOB() \
{\
    unsigned char bb_data[BINDING_BLOB_SIZE] = {\
        0x69, 0x4e, 0x04, 0xc0, 0xd3, 0xbb, 0x87, 0x72,\
        0x8c, 0xab, 0xe7, 0xdb, 0x0f, 0x89, 0xe8, 0xda\
    };\
    memcpy((void*)dd_data.raw, bb_data, BINDING_BLOB_SIZE);\
}

#endif

#endif //BINDING_BLOB_UTILS_IMPL_H
