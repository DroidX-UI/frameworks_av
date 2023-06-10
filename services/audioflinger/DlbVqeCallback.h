/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2019 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/

#ifndef _DLB_VQE_CB_H_
#define _DLB_VQE_CB_H_

namespace android {
void onAddActiveTrack(bool record, bool fast, uint32_t sessionId, uint32_t uid, uint32_t sampleRate,
        uint32_t format, uint32_t channelMask);
void onRemoveActiveTrack(bool record, bool fast, uint32_t sessionId, uint32_t uid, uint32_t sampleRate,
        uint32_t format, uint32_t channelMask);
}

#endif //_DLB_VQE_CB_H_
