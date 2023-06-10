/*
 * Copyright (C) 2022 by XiaoMi BSP Audio
 */

#define LOG_TAG "Spatializer::Effect"
#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <pthread.h>
#include "HeadTrackWrapper.h"
#include <audio_effects/effect_spatializer.h>

#define SPATIALIZER_PARAM_INPUT_CHANNEL_MASK -1
#define CHANNEL_COUNT_DEFAULT 2
int input_channel_count = CHANNEL_COUNT_DEFAULT;

extern const struct effect_interface_s gInterface;

const effect_descriptor_t gSpatializerDescriptor = {
        FX_IID_SPATIALIZER_,
        {0xbb8c97a8, 0xcc6c, 0x11ec, 0x9d64, {0x02, 0x42, 0xac, 0x12, 0x00, 0x02}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        EFFECT_FLAG_DEVICE_IND |EFFECT_FLAG_INSERT_EXCLUSIVE,
        0,
        1,
        "Spatilizer",
        "Xiaomi Bsp Audio",
};

struct SpatializerContext {
    const struct effect_interface_s *mItfe;
    effect_config_t config;
    MisoundSpatialAudio::HeadTracker * mHeadTracker = NULL;
    pthread_mutex_t lock;
};

//
//--- Effect Library Interface Implementation
//

int SpatializerLib_Create(const effect_uuid_t *uuid,
                         int32_t sessionId __unused,
                         int32_t ioId __unused,
                         effect_handle_t *handle) {
    ALOGD("SpatializerLib_Create()");
    int ret;

    if (handle == NULL || uuid == NULL) {
        return -EINVAL;
    }
    if (memcmp(uuid, &gSpatializerDescriptor.uuid, sizeof(effect_uuid_t)) != 0) {
        return -EINVAL;
    }
    SpatializerContext *pContext = new SpatializerContext;
    pthread_mutex_init(&pContext->lock, NULL);
    pContext->mItfe = &gInterface;
    *handle = (effect_handle_t) pContext;
    return 0;

}

int SpatializerLib_Release(effect_handle_t handle) {
    ALOGD("SpatializerLib_Release()");
    SpatializerContext * context = (SpatializerContext*) handle;
    delete context;
    return 0;
}

int SpatializerLib_GetDescriptor(const effect_uuid_t *uuid,
                                effect_descriptor_t *pDescriptor) {

    ALOGD("Spatializer GetDescriptor");
    if (pDescriptor == NULL || uuid == NULL){
        ALOGV("SpatializerLib_GetDescriptor() called with NULL pointer");
        return -EINVAL;
    }

    if (memcmp(uuid, &gSpatializerDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        *pDescriptor = gSpatializerDescriptor;
        return 0;
    }

    return  -EINVAL;
}

int Spatializer_Configure(struct SpatializerContext *context, effect_config_t *config) {
    if (&context->config != config) {
        memcpy(&context->config, config, sizeof(effect_config_t));
        ALOGE("Spatializer_Configure inputCfg sampleRate %d, channelMask %x format %x", config->inputCfg.samplingRate, config->inputCfg.channels,config->inputCfg.format);
        if(context->mHeadTracker != NULL) {
            free(context->mHeadTracker);
        }
        context->mHeadTracker = new MisoundSpatialAudio::HeadTracker("/vendor/etc/hrtf5c.bin", config->inputCfg.samplingRate);
        context->mHeadTracker->EnableHeadTrack(1);

        ALOGD("GetVersion %d %d %d", context->mHeadTracker->GetVersion()[0], context->mHeadTracker->GetVersion()[1],context->mHeadTracker->GetVersion()[2]);
        ALOGD("GetBuildDate %d %d %d", context->mHeadTracker->GetBuildDate()[0], context->mHeadTracker->GetBuildDate()[1],context->mHeadTracker->GetBuildDate()[2]);
        ALOGD("GetVersion %d %d %d", context->mHeadTracker->GetConfigVersion()[0], context->mHeadTracker->GetConfigVersion()[1],context->mHeadTracker->GetConfigVersion()[2]);
    }
    return 0;
}

//
//--- Effect Control Interface Implementation
//
int Spatializer_Process(
        effect_handle_t handle, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    SpatializerContext * context = (SpatializerContext*) handle;

    if (context == NULL || inBuffer == NULL || inBuffer->raw == NULL ||
        outBuffer == NULL || outBuffer->raw == NULL ||
        inBuffer->frameCount != outBuffer->frameCount) {
        return -EINVAL;
    }
/* uncomment to bypass
    float *  pSrc = inBuffer->f32;
    float *  pDst = outBuffer->f32;
    size_t numFrames = outBuffer->frameCount;
    while (numFrames) {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pSrc += 6;
        pDst += 2;
        numFrames--;
    }
    ALOGD("effect bypass") ;
    return 0;
*/

/* uncomment to dump
    static int fd = -1;
    static FILE *fp = NULL;
    if(fp == NULL) {
        fp = fopen("/data/vendor/audio/dumpSpatializer.wav", "ab+");
    } else {
        ALOGD("write");
        fwrite((void *)inBuffer->f32, 1, inBuffer->frameCount * sizeof(float) *  audio_channel_count_from_out_mask(context->config.inputCfg.channels), fp);
    }
*/

    const float *pSrc = inBuffer->f32;
    if(context->mHeadTracker != NULL) {
        context->mHeadTracker->Process(inBuffer->f32, outBuffer->f32, inBuffer->frameCount * sizeof(float) *  audio_channel_count_from_out_mask(context->config.inputCfg.channels),
                                          audio_channel_count_from_out_mask(context->config.inputCfg.channels));
        //ALOGD("processed in %f out %f mFrameCount %d", inBuffer->f32[0],outBuffer->f32[0], inBuffer->frameCount);
    } else {
        ALOGD("mHeadTracker is null");
    }
    return 0;
}

int Spatializer_Command(effect_handle_t handle, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData) 
{
   //ALOGD("Spatializer_Command command %u cmdSize %u", cmdCode, cmdSize);
    SpatializerContext * context = (SpatializerContext*) handle;
    switch(cmdCode) {
        case EFFECT_CMD_SET_CONFIG: {
            if (pCmdData == nullptr || cmdSize != sizeof(effect_config_t)
                || pReplyData == nullptr || replySize == nullptr || *replySize != sizeof(int)) {
                return -EINVAL;
            }
            *(int *) pReplyData = Spatializer_Configure(
                    context, (effect_config_t *) pCmdData);
            break;
        }
        /*TODO update current channel*/
        /*case EFFECT_CMD_DISABLE : {
            ALOGD("reset input_channel_count to default");
            input_channel_count = CHANNEL_COUNT_DEFAULT;
            context->mHeadTracker->SetChannelNum(input_channel_count);
            break;
        }*/
        case EFFECT_CMD_SET_PARAM : {
            effect_param_t *pEffectParam = (effect_param_t *) pCmdData;
            int p = *((int*) pEffectParam->data);
            switch(p) {
                case SPATIALIZER_PARAM_HEAD_TO_STAGE : {
                    float data[6] = {0};
                    memcpy(data,  (uint32_t *)pEffectParam->data + 1, 6 * sizeof(float));
                    context->mHeadTracker->SetPosition(data[5] / 3.1415926 * 180.0, 0);
                    //ALOGD("SPATIALIZER_PARAM_HEAD_TO_STAGE data %f", data[5] / 3.1415926 * 180.0);
                    break;
                }
                case SPATIALIZER_PARAM_DISPLAY_ORIENTATION : {
                    float data;
                    memcpy(&data,  (uint32_t *)pEffectParam->data + 1, 1 * sizeof(float));
                    //ALOGD("SPATIALIZER_PARAM_DISPLAY_ORIENTATION %f", data);
                    break;
                }
                case SPATIALIZER_PARAM_INPUT_CHANNEL_MASK : {
                    int8_t data;
                    memcpy(&data, (uint32_t*)pEffectParam->data + 1, sizeof(int8_t));
                    ALOGD("parameter channel count %d",data);
                    context->mHeadTracker->SetChannelNum(data);
                    break;
                }
                default : {
                    //ALOGD("cmd not support %d", p);
                }
            }
            break;
        }
        case EFFECT_CMD_GET_PARAM : {
            if (pCmdData == NULL || pReplyData == NULL || replySize == NULL) {
                ALOGE("null pCmdData or pReplyData or replySize");
                return -EINVAL;
            }
            effect_param_t *pEffectParam = (effect_param_t *) pReplyData;
            memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + sizeof(uint32_t));
            t_virtualizer_stage_params p = *((t_virtualizer_stage_params*) pEffectParam->data);
            pEffectParam->status = 0;
            *replySize = sizeof(effect_param_t) + sizeof(uint32_t);
            uint8_t * offset = ((uint8_t *)pReplyData + sizeof(effect_param_t) + sizeof(uint32_t));
            uint8_t numOfValues = 2;
            switch(p) {
                case SPATIALIZER_PARAM_HEADTRACKING_SUPPORTED: {
                    *offset = 1;//supports
                    *replySize += 1;
                    ALOGD("get SPATIALIZER_PARAM_HEADTRACKING_SUPPORTED 123");
                    break;
                }
                case SPATIALIZER_PARAM_SUPPORTED_LEVELS: {
                    memcpy(offset, &numOfValues, sizeof(uint8_t));
                    offset += 1;
                    *replySize += 1;
                    uint8_t rLevels[] = {SPATIALIZATION_LEVEL_NONE, SPATIALIZATION_LEVEL_MULTICHANNEL, SPATIALIZATION_LEVEL_MCHAN_BED_PLUS_OBJECTS};
                    memcpy(offset, rLevels, sizeof(rLevels));
                    *replySize += sizeof(rLevels);
                    ALOGD("get SPATIALIZER_PARAM_SUPPORTED_LEVELS");
                    break;
                 }
                case SPATIALIZER_PARAM_SUPPORTED_SPATIALIZATION_MODES: {
                     memcpy(offset, &numOfValues, sizeof(uint8_t));
                     offset += sizeof(numOfValues);
                     *replySize += sizeof(numOfValues);
                     uint8_t rModes [] = {SPATIALIZATION_MODE_BINAURAL};
                     memcpy(offset, rModes, sizeof(rModes));
                     *replySize += sizeof(rModes);
                     ALOGD("get SPATIALIZER_PARAM_SUPPORTED_SPATIALIZATION_MODES");
                     break;
                }
                case SPATIALIZER_PARAM_SUPPORTED_CHANNEL_MASKS: {
                    audio_channel_mask_t rChannels [] = { AUDIO_CHANNEL_OUT_5POINT1, AUDIO_CHANNEL_OUT_7POINT1POINT4};
                    ALOGD("get SPATIALIZER_PARAM_SUPPORTED_CHANNEL_MASKS %d %d",sizeof(audio_channel_mask_t), sizeof(rChannels));
                    numOfValues = 2;
                    memcpy(offset, &numOfValues, 1);//size of return num should be same as channels
                    offset += sizeof(audio_channel_mask_t);
                    *replySize += sizeof(audio_channel_mask_t);
                    memcpy(offset, rChannels, sizeof(rChannels));
                    *replySize += sizeof(rChannels);
                    break;
                }
                default: {
                    //ALOGD("not supported");
                }
            }
            break;
        }
        case EFFECT_CMD_SET_DEVICE: {
            ALOGD("EFFECT_CMD_SET_DEVICE");
            if(pCmdData == NULL) {
                ALOGD("cmd data NULL");
                return -EINVAL;
            }
            break;
        }
        case EFFECT_CMD_DUMP: {
            ALOGD("TODO do something dump");
            break;
        }
        default: {
            //ALOGD("param CMD not support %d", cmdCode);
        }
    }
    return 0;
}

int Spatializer_GetDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    memcpy(pDescriptor, &gSpatializerDescriptor, sizeof(effect_descriptor_t));
    return 0;
}

// effect_handle_t interface implementation for Spatializer effect
const struct effect_interface_s gInterface = {
        Spatializer_Process,
        Spatializer_Command,
        Spatializer_GetDescriptor,
        NULL,
};

// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "Spatializer Library",
    .implementor = "Xiaomi Bsp Audio",
    .create_effect = SpatializerLib_Create,
    .release_effect = SpatializerLib_Release,
    .get_descriptor = SpatializerLib_GetDescriptor,
};

