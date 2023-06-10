/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2018-2019 by Dolby Laboratories,
 *                             All rights reserved.
 ******************************************************************************/

#include <cutils/properties.h>
#include "DlbGenClass1.h"

namespace android {

DlbGenClass1::DlbGenClass1()
{
}

DlbGenClass1::~DlbGenClass1()
{
}

void DlbGenClass1::unmangle(const char* buff, char* outputBuff, const size_t buffLen, const char* manglingSeq, int seqLen)
{
    size_t i,j;
    for (i = 0, j = buffLen % seqLen; i < buffLen; ++i, ++j) {
        outputBuff[i] = (char)(buff[i] ^ manglingSeq[j % seqLen]);
    }
}

bool DlbGenClass1::propIsMatched(const char* mangledPropName, const size_t mangledPropNameLen,
                                 const char* mangledValueName, const size_t mangledValueNameLen)
{
    char propStr[PROPERTY_VALUE_MAX+1];
    char value[PROPERTY_VALUE_MAX+1];
    char refValueStr[PROPERTY_VALUE_MAX+1];
    android::String8 refValueString8;

    if (mangledValueNameLen >= PROPERTY_VALUE_MAX) {
#ifdef DOLBY_SECURITY_CHECK_LOG_ENABLE
    ALOGE("%s property reference value length %zu is bigger than PROPERTY_VALUE_MAX.",
        __FUNCTION__, mangledValueNameLen);
#endif
    return false;
    }

    unmangle(mangledPropName, propStr, mangledPropNameLen, mMangleSeq, mMangleSeqLen);
    propStr[mangledPropNameLen] = '\0';
    if (!property_get(propStr, value, NULL)) return false;

    if (mMangledRefPropValue) {
        unmangle(mangledValueName, refValueStr, mangledValueNameLen, mMangleSeq, mMangleSeqLen);
    } else {
        for (size_t i = 0; i < mangledValueNameLen; i++) {
            refValueStr[i] = mangledValueName[i];
        }
    }
    refValueStr[mangledValueNameLen] = '\0';
    refValueString8 = android::String8(refValueStr);
#ifdef DOLBY_SECURITY_CHECK_LOG_ENABLE
    ALOGI("%s read property value = %s, reference property value = %s", __FUNCTION__, value, refValueString8.string());
#endif
    return !refValueString8.compare(android::String8(value)) ? true : false ;
}

bool DlbGenClass1::check()
{
    int count = 0;

#ifdef DOLBY_SECURITY_CHECK_LOG_ENABLE
    ALOGI("%s", __FUNCTION__);
#endif

    if (propIsMatched(str1, str1Len, refStr1, refStr1Len)) count++;
    if (propIsMatched(str2, str2Len, refStr2, refStr2Len)) count++;
    if (propIsMatched(str3, str3Len, refStr3, refStr3Len)) count++;
    if (propIsMatched(str4, str4Len, refStr4, refStr4Len)) count++;

    if (count >= mCriteriaNum) {
#ifdef DOLBY_SECURITY_CHECK_LOG_ENABLE
        ALOGI("%s Dolby Security Check is PASS.", __FUNCTION__);
#endif
        return true;
    } else {
#ifdef DOLBY_SECURITY_CHECK_LOG_ENABLE
        ALOGE("%s Dolby Security Check is FAIL.", __FUNCTION__);
#endif
        return false;
    }
}
};   // namespace android
