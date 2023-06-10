/*
Copyright (C) 2019 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

#define LOG_TAG "OzoAudioEffects"
#define LOG_NDEBUG 0

#include <memory>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <pthread.h>

#include <hardware/audio_effect.h>
#include <audio_utils/format.h>

#include "ozoaudioenc/ozo_wrapper.h"
#include "ozoaudioenc/event_emitter.h"
#include "ozoaudiocommon/utils.h"
#include "./ozo.h"

// Effect UUID and type
#define EFFECT_UUID_OZO { 0x7e384a3b, 0x7850, 0x4a64, 0xa097, { 0x88, 0x42, 0x50, 0xd8, 0xa7, 0x37 } }
#define EFFECT_TYPE_OZO { 0x56d6b082, 0x1a83, 0x455a, 0x84a8, { 0x9d, 0xb3, 0xa3, 0x5c, 0xf5, 0x32 } }

namespace android {

static pthread_mutex_t gCaptureLock = PTHREAD_MUTEX_INITIALIZER;

// Capture descriptor
static const effect_descriptor_t sCaptureDescriptor = {
    EFFECT_TYPE_OZO, // type
    EFFECT_UUID_OZO, // uuid
    EFFECT_CONTROL_API_VERSION,
    EFFECT_FLAG_TYPE_PRE_PROC,
    0, // FIXME indicate CPU load
    0, // FIXME indicate memory usage
    "OZO Audio Capture",
    "Nokia Technologies"
};


// Types of processing modules
enum ozoproc_id
{
    OZOPROC_CAPTURE,    // OZO Audio capture

    OZOPROC_NUM_EFFECTS // Don't touch, this must remain the last item!
};


typedef enum
{
    OZOAUDIO_STATE_CREATED,
    OZOAUDIO_STATE_ACTIVE,

} ozoaudio_state_t;

typedef struct ozoaudio_object_t
{
    // Number of frames processed (informative data)
    size_t frames;

    // Processing state
    ozoaudio_state_t state;

    // Number of output channels
    int out_channels;

    // Input buffer
    ozoaudiocodec::OzoRingBufferT<int16_t> *buffer;

    // Output buffer
    ozoaudiocodec::OzoRingBufferT<int16_t> *output;

    // Input frame under processing
    int16_t *input;

    // Encoder handle and its initialization parameters
    OzoEncoderWrapper *encoder;
    struct OzoEncoderParams_s *params;
    OzoParameterUpdater paramUpdater;
    std::string initParams;

    // Current noise level
    uint8_t noise_level;

    // Current audio levels
    int stereoLevels[2];

    // Mic blocking detection data (can hold 8-mics data at max)
    int nMicBlockData;
    int micBlocking[8 * 2]; // Holds (mic indec + level) * number of mics

} ozoaudio_object_t;

typedef struct ozoaudio_module_t
{
    const struct effect_interface_s *itfe;
    effect_config_t config;
    ozoaudio_object_t context;

} ozoaudio_module_t;


// Release audio handles related to Ozo Audio processing
static void
ozoaudio_object_release(ozoaudio_object_t *object)
{
    if (object->buffer)
        delete object->buffer;
    object->buffer = nullptr;

    if (object->output)
        delete object->output;
    object->output = nullptr;

    if (object->encoder)
        delete object->encoder;
    object->encoder = nullptr;

    if (object->input)
        delete [] object->input;
    object->input = nullptr;
}

// Create handles related to Ozo Audio processing
static int
ozoaudio_object_create(ozoaudio_module_t *module)
{
    ozoaudio_object_t *object = &module->context;

    ozoaudio_object_release(object);

    module->context.frames = 0;
    module->context.out_channels = 2; // By default request 2-channel output from Ozo SDK

    object->noise_level = 0;
    object->stereoLevels[0] = 0;
    object->stereoLevels[1] = 0;

    object->nMicBlockData = 0;
    memset(object->micBlocking, 0, sizeof(object->micBlocking));

    // Number of input channels
    module->context.params->channels = audio_channel_count_from_out_mask(module->config.inputCfg.channels);

    // 2 Input frames are always stored to ring buffer
    auto size = 2 * OzoEncoderWrapper::kNumSamplesPerFrame * module->context.params->channels;

    object->buffer = new (std::nothrow) ozoaudiocodec::OzoRingBufferT<int16_t>(size);
    if (!object->buffer) return -ENOMEM;

    object->input = new (std::nothrow) int16_t[size];
    if (!object->input) return -ENOMEM;

    // 2 output frames are always stored to ring buffer
    size = 2 * OzoEncoderWrapper::kNumSamplesPerFrame * module->context.out_channels;

    object->output = new (std::nothrow) ozoaudiocodec::OzoRingBufferT<int16_t>(size);
    if (!object->output) return -ENOMEM;

    // This creates 1 additional frame delay (since we need to return always data as output)
    object->output->write(size / 2);

    object->encoder = new (std::nothrow) OzoEncoderWrapper();
    if (!object->encoder) return -ENOMEM;

    module->context.paramUpdater.setParams(module->context.initParams.c_str());
    if (!object->encoder->init(*module->context.params, module->context.paramUpdater)) {
        ALOGE("Ozo encoder initialization failed");
        delete object->encoder;
        object->encoder = nullptr;

        return -EINVAL;
    }

    return 0;
}


// Initialize audio capture module
static const effect_handle_t *
capture_create(ozoaudio_module_t *module)
{
    module->context.frames = 0;
    module->context.state = OZOAUDIO_STATE_CREATED;

    module->context.params = new OzoEncoderParams_t;
    module->context.buffer = nullptr;
    module->context.output = nullptr;
    module->context.input = nullptr;
    module->context.encoder = nullptr;

    module->context.paramUpdater.clear();
    module->context.initParams.clear();

    // Defaults
    init_default_ozoparams(module->context.params);

    // Register WNR observer
    module->context.params->eventEmitters.push_back(new WnrEventEmitter());

    // Register audio levels observer
    module->context.params->eventEmitters.push_back(new AudioLevelsEmitter());

    // Operate only in 16-bit domain
    module->context.params->sampledepth = 16;

    // Rewuest OZO Audio output from Ozo Audio SDK
    module->context.params->encoderMode = ozoaudioencoder::OZOSDK_OZOAUDIO_ID;

    // Request output as PCM data from Ozo Audio SDK
    module->context.params->pcmOnly = true;
    module->context.params->pcmFormat = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;

    return (effect_handle_t *) module;
}

// Release audio capture module
static void
capture_release(ozoaudio_module_t *module)
{
    ozoaudio_object_release(&module->context);

    for (size_t i = 0; i < module->context.params->eventEmitters.size(); i++)
        delete module->context.params->eventEmitters[i];

    delete module->context.params;

    module->context.initParams.clear();
}

// Initialize capture settings (config, etc)
static int
capture_init(ozoaudio_module_t *module)
{
    ALOGV("capture_init()");

    if (module == NULL)
        return -EINVAL;

    module->context.frames = 0;

    // Initial input config, channels count does not matter as we accept new configs
    module->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    module->config.inputCfg.channels = AUDIO_CHANNEL_IN_LEFT | AUDIO_CHANNEL_IN_RIGHT | AUDIO_CHANNEL_IN_FRONT;
    module->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    module->config.inputCfg.samplingRate = module->context.params->samplerate;
    module->config.inputCfg.bufferProvider.getBuffer = NULL;
    module->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    module->config.inputCfg.bufferProvider.cookie = NULL;
    module->config.inputCfg.mask = EFFECT_CONFIG_ALL;

    // Initial output config
    module->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_WRITE;
    module->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    module->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    module->config.outputCfg.samplingRate = module->context.params->samplerate;
    module->config.outputCfg.bufferProvider.getBuffer = NULL;
    module->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    module->config.outputCfg.bufferProvider.cookie = NULL;
    module->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    return 0;
}

// Set new capture effect config
static int
capture_setconfig(ozoaudio_module_t *module, effect_config_t *config)
{
    ALOGV("capture_setconfig()");

    // Rules when config change is allowed:
    // - Input and output sample rate must be the same and 48000Hz
    // - Input channel count must be 2 or greater
    // - Input and output format must be 16-bit
    // - Output buffer access mode must allow write

    if (config->inputCfg.samplingRate != config->outputCfg.samplingRate) {
        ALOGE("Sample rate mismatch: %i %i", config->inputCfg.samplingRate, config->outputCfg.samplingRate);
        return -EINVAL;
    }

    if (config->inputCfg.samplingRate != 48000) {
        ALOGE("Unsupported sample rate: %i", config->inputCfg.samplingRate);
        return -EINVAL;
    }

    auto channels = audio_channel_count_from_out_mask(config->outputCfg.channels);
    if (channels < 2) {
        ALOGE("Unsupported output channel count: %i", channels);
        return -EINVAL;
    }

    if (config->inputCfg.format != config->outputCfg.format) {
        ALOGE("Format mismatch: %i %i", config->inputCfg.format, config->outputCfg.format);
        return -EINVAL;
    }

    if (config->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT) {
        ALOGE("Unsupported format (only 16-bit supported): %i", config->inputCfg.format);
        return -EINVAL;
    }

    if (!(config->outputCfg.accessMode == EFFECT_BUFFER_ACCESS_WRITE ||
          config->outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)) {
        ALOGE("Invalid access mode: %i", config->inputCfg.accessMode);
        return -EINVAL;
    }

    module->config = *config;

    return 0;
}

// Get current capture effect config
static void
capture_getconfig(ozoaudio_module_t *module, effect_config_t *config)
{
    *config = module->config;
}

// Enable capture effect
static int
capture_enable(ozoaudio_module_t *module)
{
    if (module->context.state != OZOAUDIO_STATE_CREATED)
        return -ENOSYS;

    auto result = ozoaudio_object_create(module);
    if (result == 0)
        module->context.state = OZOAUDIO_STATE_ACTIVE;

    return result;
}

// Disable capture effect
static int
capture_disable(ozoaudio_module_t *module)
{
    if (module->context.state != OZOAUDIO_STATE_ACTIVE)
        return -ENOSYS;

    ozoaudio_object_release(&module->context);
    module->context.state = OZOAUDIO_STATE_CREATED;

    return 0;
}

// Get capture effect parameter
static int
capture_getparameter(ozoaudio_module_t *module, int32_t param, uint32_t *pSize, void *pValue)
{
    switch (param) {
        // Device UUID
        case OZO_PARAM_DEVICE_UUID: {
            if (*pSize < module->context.params->deviceId.size()) {
                ALOGD(
                    "Unable to query device UUID: output buffer too short (%u vs %zu bytes)",
                    *pSize, module->context.params->deviceId.size()
                );
                return -EINVAL;
            }

            *pSize = module->context.params->deviceId.size();
            memcpy(pValue, module->context.params->deviceId.c_str(), *pSize);
            break;
        }

        case OZO_CAPTURE_WNR_LEVEL:
            if (*pSize < 1) {
                ALOGD(
                    "Unable to query wind noise level: output buffer too short (%u vs %zu bytes)",
                    *pSize, (size_t) 1
                );
                return -EINVAL;
            }

            *pSize = 1;
            memcpy(pValue, &module->context.noise_level, 1);
            return 0;

        case OZO_CAPTURE_AUDIO_LEVEL:
            if (*pSize < 2 * sizeof(int)) {
                ALOGD(
                    "Unable to query audio levels: output buffer too short (%u vs %zu bytes)",
                    *pSize, 2 * sizeof(int)
                );
                return -EINVAL;
            }

            // Make sure processing does not mess up the levels while querying the levels
            pthread_mutex_lock(&gCaptureLock);

            *pSize = 2 * sizeof(int);
            memcpy(pValue, module->context.stereoLevels, 2 * sizeof(int));
            module->context.stereoLevels[0] = module->context.stereoLevels[1] = 0;
            pthread_mutex_unlock(&gCaptureLock);
            return 0;

        case OZO_CAPTURE_MICBLOCKING_LEVEL: {
            size_t dataSize = sizeof(module->context.micBlocking) + sizeof(int);

            if (*pSize < dataSize) {
                ALOGD(
                    "Unable to query mic blocking data: output buffer too short (%u vs %zu bytes)",
                    *pSize, dataSize
                );
                return -EINVAL;
            }

            // Make sure processing does not mess up the data while querying the data
            pthread_mutex_lock(&gCaptureLock);

            *pSize = dataSize;
            uint8_t *pValueInt = (uint8_t *) pValue;
            memcpy(pValueInt, &module->context.nMicBlockData, sizeof(int));
            memcpy(pValueInt + sizeof(int), module->context.micBlocking, sizeof(module->context.micBlocking));
            module->context.nMicBlockData = 0;
            pthread_mutex_unlock(&gCaptureLock);
            return 0;
        }
    }

    return 0;
}

// Set capture effect parameter
static int
capture_setparameter(ozoaudio_module_t *module, int32_t param, uint32_t size, void *pValue)
{
    char tmp[64];

    switch (param) {
        // Device UUID
        case OZO_PARAM_DEVICE_UUID: {
            #define DEVICE_UUID_SIZE 36
            if (size > DEVICE_UUID_SIZE) {
                ALOGE("capture_setparameter(): invalid uuid size=%u", size);
                return -EINVAL;
            }

            memcpy(tmp, pValue, size); tmp[size] = '\0';

            module->context.params->deviceId = tmp;
            ALOGV("capture_setparameter(): device uuid=%s", module->context.params->deviceId.c_str());
            break;
        }

        // Enable mic blocking detection
        case OZO_PARAM_MICBLOCKING_MODE:
            module->context.params->eventEmitters.push_back(new MicBlockingEmitter());
            break;

        case OZO_PARAM_GENERIC: {
            char param[1024];

            memcpy(param, pValue, size); param[size] = '\0';

            if (module->context.encoder) {
                module->context.paramUpdater.setParams(param);
                if (!module->context.encoder->setRuntimeParameters(module->context.paramUpdater)) {
                    ALOGE("capture_setparameter(): failed to set %s", param);
                    return -EINVAL;
                }
            } else {
                ALOGD("Deferred parameter: %s", param);

                // Defer the parameter assignment after encoder has been initialized
                module->context.initParams += std::string(param) + ";";
            }

            return 0;
        }

        default:
            ALOGE("capture_setparameter(): unknown parameter %" PRId16, param);
            return -EINVAL;
    }

    return 0;
}


// Store processing related event parameters
static void
process_getparameter(ozoaudio_module_t *module)
{
    for (size_t i = 0; i < module->context.params->eventEmitters.size(); i++) {
        int *data;
        int event = 0, dataLen = 0;
        if (module->context.params->eventEmitters[i]->getEvent(event, &data, dataLen)) {
            if (event == kOzoWindNoiseEvent) {
                module->context.noise_level = ((uint8_t) *data) >> 1; // Extract just the level
            }
            else if (event == kOzoAudioLevelEvent) {
                auto left = data[1]; // Left channel
                auto right = data[2]; // Right channel

                if (module->context.stereoLevels[0] < left)
                    module->context.stereoLevels[0] = left;

                if (module->context.stereoLevels[1] < right)
                    module->context.stereoLevels[1] = right;
            }
            else if (event == kOzoMicBlockingEvent) {
                module->context.nMicBlockData = data[0];
                for (int n = 0, offset = 1; n < module->context.nMicBlockData; n++) {
                    module->context.micBlocking[offset - 1] = data[offset]; offset++;
                    module->context.micBlocking[offset - 1] = data[offset]; offset++;
                }

                /*
                ALOGD("Mic blocking count: %i", module->context.nMicBlockData);
                for (int n = 0; n < module->context.nMicBlockData; n++)
                    ALOGD("Mic blocking data: %i %i %i", n, module->context.micBlocking[2 * n],
                        module->context.micBlocking[2 * n + 1]);
                */
            }
        }
    }
}

//
// Public interfaces related to capture effect
//

static int
Capture_Process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    ozoaudio_module_t *module = (ozoaudio_module_t *) self;

    if (module == NULL || inBuffer == NULL || outBuffer == NULL) {
        return -EINVAL;
    }

    if (module->context.state != OZOAUDIO_STATE_ACTIVE) {
        ALOGE("Capture_Process(): Not in active state");
        return -ENODATA;
    }

    if (!module->context.encoder) {
        ALOGE("Capture_Process(): No encoder handle");
        return -EINVAL;
    }

    // Frame count must not exceed threshold as otherwise buffering scheme will not work
    if (inBuffer->frameCount > OzoEncoderWrapper::kNumSamplesPerFrame) {
        ALOGE(
            "Capture_Process(): Input buffer frame count %zu exceeds %i",
            inBuffer->frameCount, OzoEncoderWrapper::kNumSamplesPerFrame
        );
        return -EINVAL;
    }

    // We assume that one frame delay is needed to get the buffering scheme to work.
    // If these assumptions do not hold, then abort
    if (2 * inBuffer->frameCount < OzoEncoderWrapper::kNumSamplesPerFrame) {
        ALOGE("Capture_Process(): Input buffer frame count %zu is too low", inBuffer->frameCount);
        return -EINVAL;
    }

    size_t inChannels = audio_channel_count_from_out_mask(module->config.inputCfg.channels);
    size_t outChannels = audio_channel_count_from_out_mask(module->config.outputCfg.channels);

    audio_format_t informat = (audio_format_t) outBuffer->frameCount;
    audio_format_t outformat = AUDIO_FORMAT_PCM_16_BIT;
    auto sampleCount = inBuffer->frameCount * inChannels;

    // Special trick for HAL integration. Input format has been configured to be 16-bit but
    // input format that HAL is serving us here may differ from this. So internally convert the
    // input to 16-bit, do processing and then serve the output again in the original input format.
    // Since we need to manually add the effect process call, null output buffer indicates
    // that call is coming from HAL.
    bool HALcall = outBuffer->raw == 0;

    // Invalid buffer configuration
    if (!HALcall && inBuffer->frameCount != outBuffer->frameCount) {
        ALOGE(
            "Capture_Process(): Input and output buffer count mismatch occured %zu %zu",
            inBuffer->frameCount, outBuffer->frameCount
        );
        return -EINVAL;
    }

    ALOGV("Capture_Process(): %zu %u %u %u %zu %zu %zu %zu %p 0x%x %zu",
        module->context.frames, module->config.inputCfg.format,
        module->config.inputCfg.channels, module->config.outputCfg.channels,
        inBuffer->frameCount, outBuffer->frameCount, inChannels, outChannels,
        inBuffer->raw, informat, audio_bytes_per_sample(informat)
    );

    int16_t *audioInput = (HALcall) ? module->context.input : inBuffer->s16;

    if (HALcall) {
        outBuffer->raw = inBuffer->raw;
        outBuffer->frameCount = inBuffer->frameCount;
        memcpy_by_audio_format(audioInput, outformat, inBuffer->raw, informat, sampleCount);
    }

    // Save input for later processing
    module->context.buffer->write(audioInput, sampleCount);

    // Process buffered input
    auto frameLength = OzoEncoderWrapper::kNumSamplesPerFrame * inChannels;
    if (module->context.buffer->samplesAvailable() >= frameLength) {
        ozoaudiocodec::AudioDataContainer output;

        // Apply processing.
        // Regardless of the number of inputs, the output is always 'module->context.out_channels'
        module->context.buffer->read(module->context.input, frameLength);
        pthread_mutex_lock(&gCaptureLock);
        if (!module->context.encoder->encode((uint8_t *) module->context.input, output)) {
            ALOGE("Capture_Process(): Encode failed");
            pthread_mutex_unlock(&gCaptureLock);
            return -EINVAL;
        }

        // Output needs to be buffered due to frame length mismatch
        module->context.output->write((int16_t *) output.audio.data, output.audio.data_size / sizeof(int16_t));

        // Save event parameters, if any
        process_getparameter(module);
        pthread_mutex_unlock(&gCaptureLock);
    }

    // Extract output
    auto outSize = outBuffer->frameCount * module->context.out_channels;
    if (module->context.output->samplesAvailable() >= outSize) {
        module->context.output->read(module->context.input, outSize);

        auto fillChannels = outChannels - module->context.out_channels;
        if (fillChannels > 0) {
            auto outSizeShuffle = outBuffer->frameCount * outChannels;
            auto src = module->context.input + outSize - 1;
            auto dst = module->context.input + outSizeShuffle - 1;

            // Place 2-channel output as last channels and mute remaining.
            // Start from the end of the audio buffer and proceed towards the start.
            // We assume that the caller will strip the muted channels at some point in the processing chain.
            for (size_t i = 0; i < outSize; ) {
                for (int j = 0; j < module->context.out_channels; j++, i++)
                    *dst-- = *src--;

                for (size_t j = 0; j < fillChannels; j++)
                    *dst-- = 0;
            }
        }

        if (HALcall)
            memcpy_by_audio_format(outBuffer->raw, informat, module->context.input, outformat, sampleCount);
        else
            memcpy(outBuffer->s16, module->context.input, sampleCount * sizeof(int16_t));

    } else {
        ALOGV("No output data available");
        if (HALcall)
            memset(outBuffer->raw, 0, sampleCount * audio_bytes_per_sample(informat));
        else
            memset(outBuffer->s16, 0, sampleCount * sizeof(int16_t));

        return -EINVAL;
    }

    module->context.frames++;

    return 0;
}

static int
Capture_Command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    ozoaudio_module_t *module = (ozoaudio_module_t *) self;

    ALOGV("Capture_Command %" PRIu32 " cmdSize %" PRIu32, cmdCode, cmdSize);

    if (module == NULL) {
        return -EINVAL;
    }

    switch (cmdCode) {
        case EFFECT_CMD_INIT:
            ALOGV("Capture_Command: EFFECT_CMD_INIT");
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
                return -EINVAL;

            *(int *) pReplyData = capture_init(module);
            break;

        case EFFECT_CMD_SET_CONFIG:
            ALOGV("Capture_Command: EFFECT_CMD_SET_CONFIG");
            if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) ||
                pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
                return -EINVAL;

            *(int *) pReplyData = capture_setconfig(module, (effect_config_t *) pCmdData);
            break;

        case EFFECT_CMD_GET_CONFIG:
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(effect_config_t))
                return -EINVAL;

            capture_getconfig(module, (effect_config_t *) pReplyData);
            break;

        case EFFECT_CMD_GET_PARAM: {
            ALOGV(
                "Capture_Command: EFFECT_CMD_GET_PARAM pCmdData %p, *replySize %" PRIu32 ", pReplyData: %p",
                pCmdData, *replySize, pReplyData
            );

            if (pCmdData == NULL || cmdSize < (int)(sizeof(effect_param_t) + sizeof(int32_t)) ||
                pReplyData == NULL || replySize == NULL ||
                *replySize < (int) sizeof(effect_param_t) + sizeof(int32_t)) {
                return -EINVAL;
            }
            effect_param_t *rep = (effect_param_t *) pReplyData;
            memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + sizeof(int32_t));

            ALOGV(
                "Capture_Command: EFFECT_CMD_GET_PARAM param %" PRId32 ", replySize %" PRIu32,
                *(int32_t *)rep->data, rep->vsize
            );

            rep->status = capture_getparameter(
                module, *(int32_t *) rep->data, &rep->vsize, rep->data + sizeof(int32_t)
            );
            *replySize = sizeof(effect_param_t) + sizeof(int32_t) + rep->vsize;
            break;
        }

        case EFFECT_CMD_SET_PARAM: {
            ALOGV(
                "Capture_Command: EFFECT_CMD_SET_PARAM cmdSize %d pCmdData %p, *replySize %" PRIu32 ", pReplyData %p",
                cmdSize, pCmdData, *replySize, pReplyData
            );

            if (pCmdData == NULL || (cmdSize < (int)(sizeof(effect_param_t) + sizeof(int32_t))) ||
                pReplyData == NULL || replySize == NULL || *replySize != (int) sizeof(int32_t)) {
                return -EINVAL;
            }
            effect_param_t *cmd = (effect_param_t *) pCmdData;
            if (cmd->psize != sizeof(int32_t)) {
                android_errorWriteLog(0x534e4554, "63662938");
                return -EINVAL;
            }

            ALOGV(
                "Capture_Command: EFFECT_CMD_SET_PARAM param %" PRId32 ", size=%i, value=%i",
                *(uint32_t *) cmd->data, cmd->psize, cmd->vsize
            );

            *(int *) pReplyData = capture_setparameter(
                module, *(int32_t *) cmd->data, cmd->vsize, cmd->data + sizeof(int32_t)
            );
            break;
        }

        case EFFECT_CMD_ENABLE:
            ALOGV("Capture_Command: EFFECT_CMD_ENABLE");
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
                return -EINVAL;

            *(int *) pReplyData = capture_enable(module);
            break;

        case EFFECT_CMD_DISABLE:
            ALOGV("Capture_Command: EFFECT_CMD_DISABLE");
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
                return -EINVAL;

            *(int *) pReplyData = capture_disable(module);
            break;

        case EFFECT_CMD_RESET:
            ALOGV("Capture_Command: EFFECT_CMD_RESET (pass)");
            break;

        case EFFECT_CMD_SET_DEVICE:
            ALOGV("Capture_Command: EFFECT_CMD_SET_DEVICE (pass)");
            break;

        case EFFECT_CMD_SET_VOLUME:
            ALOGV("Capture_Command: EFFECT_CMD_SET_VOLUME: (pass)");
            break;

        case EFFECT_CMD_SET_AUDIO_MODE:
            ALOGV("Capture_Command: EFFECT_CMD_SET_AUDIO_MODE (pass)");
            break;

        case EFFECT_CMD_SET_CONFIG_REVERSE:
            ALOGV("Capture_Command: EFFECT_CMD_SET_AUDIO_MODE (pass)");
            break;

        case EFFECT_CMD_SET_INPUT_DEVICE:
            ALOGV("Capture_Command: EFFECT_CMD_SET_INPUT_DEVICE (pass)");
            break;

        case EFFECT_CMD_SET_PARAM_DEFERRED:
            ALOGV("Capture_Command: EFFECT_CMD_SET_PARAM_DEFERRED (pass)");
            break;

        case EFFECT_CMD_SET_PARAM_COMMIT:
            ALOGV("Capture_Command: EFFECT_CMD_SET_PARAM_COMMIT (pass)");
            break;

        default:
            ALOGW("Capture_Command: invalid command %" PRIu32, cmdCode);
            return -EINVAL;
    }

    return 0;
}

int
Capture_GetDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    ozoaudio_module_t *module = (ozoaudio_module_t *) self;

    ALOGD("Capture_GetDescriptor");

    if (module == NULL) {
        return -EINVAL;
    }

    memcpy(pDescriptor, &sCaptureDescriptor, sizeof(effect_descriptor_t));

    return 0;
}

//
// End of public interfaces related to capture effect
//


static const struct effect_interface_s gCaptureInterface = {
        Capture_Process,
        Capture_Command,
        Capture_GetDescriptor,
        NULL
};


// Available effect descriptors
static const effect_descriptor_t *sDescriptors[OZOPROC_NUM_EFFECTS] = {
        &sCaptureDescriptor
};

// Available effect interface implementations
static const struct effect_interface_s *sInterfaces[OZOPROC_NUM_EFFECTS] = {
    &gCaptureInterface
};


static const effect_descriptor_t *
Ozo_GetDescriptor(const effect_uuid_t *uuid, size_t &index)
{
    for (size_t i = 0; i < OZOPROC_NUM_EFFECTS; i++)
        if (memcmp(&sDescriptors[i]->uuid, uuid, sizeof(effect_uuid_t)) == 0) {
            index = i;
            return sDescriptors[i];
        }

    return NULL;
}

static const char *
uuid2Char(const effect_uuid_t *uuid, char *buffer)
{
    sprintf(buffer, "%08X-%04X-%04X-%04X-...", uuid->timeLow, uuid->timeMid, uuid->timeHiAndVersion, uuid->clockSeq);
    return buffer;
}


int
Create(const effect_uuid_t *uuid, int32_t /*sessionId*/, int32_t /*ioId*/, effect_handle_t *pInterface)
{
    char buffer[64];
    size_t index = 0;
    ozoaudio_module_t *module;

    ALOGV("Create() for uuid=%s", uuid2Char(uuid, buffer));

    if (Ozo_GetDescriptor(uuid, index) == NULL) {
        ALOGE("Create: Descriptor not found for uuid=%s", uuid2Char(uuid, buffer));
        return -EINVAL;
    }

    module = (ozoaudio_module_t *) calloc(1, sizeof(ozoaudio_module_t));
    if (module == NULL) {
        ALOGE("Create: Unable to create module");
        return -ENOMEM;
    }

    module->itfe = sInterfaces[index];

    *pInterface = (effect_handle_t) capture_create(module);

    return 0;
}

int
Release(effect_handle_t interface)
{
    ozoaudio_module_t *module = (ozoaudio_module_t *) interface;

    ALOGV("Release()");

    if (interface == NULL) {
        return -EINVAL;
    }

    ALOGV("# of frames processed: %zu", module->context.frames);
    capture_release(module);
    free(module);

    return 0;
}

int
GetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
    char buffer[64];
    size_t index = 0;

    ALOGV("GetDescriptor() for uuid=%s", uuid2Char(uuid, buffer));

    if (pDescriptor == NULL || uuid == NULL) {
        return -EINVAL;
    }

    auto desc = Ozo_GetDescriptor(uuid, index);
    if (desc == NULL) {
        ALOGE("GetDescriptor: Descriptor not found for uuid=%s", uuid2Char(uuid, buffer));
        return  -EINVAL;
    }

    *pDescriptor = *desc;

    return 0;
}

} // namespace android

extern "C" {

// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "OZO Audio Processing Library",
    .implementor = "Nokia Technologies",
    .create_effect = android::Create,
    .release_effect = android::Release,
    .get_descriptor = android::GetDescriptor
};

}; // extern "C"
