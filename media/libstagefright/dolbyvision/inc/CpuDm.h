/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2019 by Dolby Laboratories,
 *                             All rights reserved.
 ******************************************************************************/

#ifndef DOLBY_VISION_DOVI_INC_CPUDM_H
#define DOLBY_VISION_DOVI_INC_CPUDM_H

#include <OMX_Video.h>

int convertfromDoviHDR(const unsigned char* const inputImg,
                       OMX_COLOR_FORMATTYPE inputFormat,
                       unsigned char* const outputImg,
                       OMX_COLOR_FORMATTYPE outputFormat,
                       size_t width,
                       size_t height);

#endif /* DOLBY_VISION_DOVI_INC_CPUDM_H */
