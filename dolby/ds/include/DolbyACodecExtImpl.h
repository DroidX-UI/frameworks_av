/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *               Copyright (C) 2015-2019 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/

#include "ds_config.h"
#include "TableXInit_impl.h"
#include "DlbGenClass1.h"
#include "DlbGenClass1_impl.h"

// Note: This header file must be included only from ACodec.cpp
namespace android {

#ifdef DOLBY_AC4_SPLIT_SEC
template<class T>
static void InitTblOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->seedA = 0;
    params->seedB = 0;
    params->seedC = 0;

    params->idA = 0;
    params->idB = 0;
    params->idC = 0;

    params->maskA = 0;
    params->maskB = 0;
    params->maskC = 0;

    params->sizeA = 0;
    params->sizeB = 0;
    params->sizeC = 0;
}
#endif //DOLBY_AC4_SPLIT_SEC
} // namespace android
