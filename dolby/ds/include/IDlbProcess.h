/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2014-2022 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/
#ifndef DOLBY_I_DLB_PROCESS_H_
#define DOLBY_I_DLB_PROCESS_H_

#include <utils/Errors.h>
#include <system/audio.h>
#include <hardware/audio_effect.h>
#include "DapParams.h"
#include "VqeParams.h"
#include "DlbBufferProvider.h"
#include <stdlib.h>
#include <cutils/properties.h>
#include "DapParamCache.h"
// Define where the retrieved DAP parameter dump file will be placed.
#define DOLBY_PARAM_DUMP_FILE_DIR       "/data/misc/audioserver/"
#define DOLBY_EFFECT_PARAM_DUMP_FILE    "effect_params.txt"
#define DOLBY_EFFECT_SPATIALIZER_PARAM_DUMP_FILE    "effect_spatializer_params.txt"
#define DOLBY_APROCESSOR_PARAM_DUMP_FILE "aprocessor_params.txt"
#define DOLBY_PARAM_FILE_NAME_MAX       (256)
#define DOLBY_PARAM_TEE_FLAG_FILE       (0x1)
#define DOLBY_PARAM_TEE_FLAG_LOGCAT     (0x2)

#define NUM_PCM_SAMPLES_PER_BLOCK (256)

namespace dolby {

using namespace android;

class IDlbProcess
{
public:
    virtual ~IDlbProcess() { };
    virtual status_t init() = 0;
    virtual void setEnabled(bool enable) = 0;
    virtual status_t configure(int bufferSize, int sampleRate, audio_format_t format,
        audio_channel_mask_t inChannels, audio_channel_mask_t outChannels) = 0;

    virtual status_t getParam(param_id_t param, param_value_t* values, int* length) = 0;
    virtual status_t setParam(param_id_t param, const param_value_t* values, int length) = 0;
    virtual status_t setHTParam(int paramId __unused, void *pValues __unused, int length __unused)  { return NO_ERROR; };

    virtual status_t process(BufferProvider *inBuffer, BufferProvider *outBuffer) = 0;
    virtual status_t deviceChanged() { return NO_ERROR; }
    virtual status_t reset(int vbType __unused) { return NO_ERROR; }
    virtual status_t dumpParams(const char *filename) {
        ALOGV("%s()", __FUNCTION__);
        int32_t teeConfig = property_get_int32("vendor.dolby.dap.param.tee", 0);
        if (mDapParams.size() > 0 && (teeConfig & DOLBY_PARAM_TEE_FLAG_LOGCAT) != 0) {
            const char *prefix = (strstr(filename, DOLBY_APROCESSOR_PARAM_DUMP_FILE) == NULL)
                                 ? "Effect" : "DDPDecoder_AProcessor";
            for (DapParamCache::Iterator iter = mDapParams.getIterator(); !iter.finished(); iter.next()) {
                if (iter.param() == DAP_PARAM_CEQT) {
                    dumpExtraLongParam(iter.param(), (const param_value_t*)iter.values()->data(),
                                       iter.values()->length());
                } else {
                    ALOGD("%s %s(%s)", prefix, __FUNCTION__,
                        dapParamNameAllValues(iter.param(), iter.values()->data(), iter.values()->length()).string());
                }
            }
        }
        if (mDapParams.size() > 0 && (teeConfig & DOLBY_PARAM_TEE_FLAG_FILE) != 0) {
            FILE *fp = fopen(filename, "wb");
            if (fp == NULL) {
                ALOGE("%s() Error opening file %s. Have you done 'adb remount'?", __FUNCTION__, filename);
                return BAD_VALUE;
            }
            for (DapParamCache::Iterator iter = mDapParams.getIterator(); !iter.finished(); iter.next()) {
                const param_value_t *values = iter.values()->data();
                android::String8 paramStr = dapParamName(iter.param());
                paramStr.appendFormat("=%d", *values);
                for (int i = 1; i < iter.values()->length(); ++i) {
                    paramStr.appendFormat(",%d", values[i]);
                }
                paramStr.append("\n");
                size_t nBytes = fwrite(paramStr.string(), 1, paramStr.size(), fp);
                if (nBytes < paramStr.size()) {
                    ALOGW("%s() Error on dumping DAP parameter %s?", __FUNCTION__, dapParamName(iter.param()).string());
                }
            }
            fclose(fp);
        }
        return NO_ERROR;
    }

protected:
    void dumpExtraLongParam(param_id_t param, const param_value_t* values, int length) {
        const int subLength = 80;
        int nTimes = length / subLength;
        for (int i = 0; i <= nTimes; i++) {
            if (i == 0) {
                android::String8 header = dapParamName(param);
                header.appendFormat(" = #%d[", length);
                header.append(dapParamValues(values, subLength, false));
                ALOGD("Effect dumpParams(%s", header.string());
            } else if (i == nTimes) {
                android::String8 footer = dapParamValues(&values[i * subLength], length % subLength, true);
                footer.append("])");
                ALOGD("  ceqt continue:     %s", footer.string());
            } else {
                ALOGD("  ceqt continue:     %s", dapParamValues(&values[i * subLength], subLength, false).string());
            }
        }
    }
    // Defined for DAP parameter dump
    DapParamCache mDapParams;
};

} // namespace dolby

#endif//DOLBY_I_DLB_PROCESS_H_
