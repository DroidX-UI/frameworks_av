/*
Copyright (C) 2020 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

#define LOG_TAG "OzoWideningEffects"
#define LOG_NDEBUG 0

#include <memory>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <pthread.h>

#include <hardware/audio_effect.h>
#include <audio_utils/format.h>

#include "ozowidening/lwidening.h"
#include "ozowidening/lwidening_types.h"
#include "ozowidening/hwidening.h"
#include "ozowidening/hwidening_types.h"
#include "ozowidening/license.h"
#include "ozoaudiocommon/license_reader2.h"

// Effect UUID and type
#define EFFECT_UUID_OZO { 0x79bbd08d, 0x8f08, 0x4e96, 0x925a, { 0x43, 0x0f, 0x2e, 0xb9, 0x6b, 0xf1 } }
#define EFFECT_TYPE_OZO { 0x37cc2c00, 0xdddd, 0x11db, 0x8577, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }

#define MAX_PROCESSING_FRAME_SIZE  1024
#define OUTPUT_WIDENING_DEVICE     "d265567e-bb6d-45bf-8ae3-324848a4626c"

namespace android {

static bool gLicenseSet = false;
static const char OZOPLAY_LICENSE_FILE[] = "/vendor/etc/ozoplaysdk.license";

static pthread_mutex_t gWideningLock = PTHREAD_MUTEX_INITIALIZER;

// Widening descriptor
static const effect_descriptor_t sPlaybackDescriptor = {
    EFFECT_TYPE_OZO, // type
    EFFECT_UUID_OZO, // uuid
    EFFECT_CONTROL_API_VERSION,
    EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_INSERT_LAST | EFFECT_FLAG_DEVICE_IND,
    0,
    0,
    "OZO Playback",
    "Nokia Technologies"
};

// Parameter IDs for custom commands
enum {
    OZO_PLAYBACK_FORCE_BYPASS = 111111,
};


// Types of processing modules
enum ozoproc_id
{
    OZOPROC_PLAYBACK,   // OZO Audio Playback

    OZOPROC_NUM_EFFECTS // Don't touch, this must remain the last item!
};


typedef enum
{
    PLAYBACK_STATE_ACTIVE,
    PLAYBACK_STATE_INACTIVE,
    PLAYBACK_STATE_NOPERMISSION

} ozoplayback_state_t;

typedef struct ozoplayback_object_t
{
    // Effect enabled
    bool enabled;

    // Widening is forced to operate on bypass mode
    bool force_bypass;

    // Processing state
    ozoplayback_state_t ls_state;
    ozoplayback_state_t hp_state;

    // Audio device in use
    audio_devices_t device;

    // Loudspeaker widening handle
    LWideningPtr ls_handle;

    // Headset widening handle
    HWideningPtr hp_handle;

} ozoplayback_object_t;

typedef struct ozoplayback_module_t
{
    const struct effect_interface_s *itfe;
    effect_config_t config;
    ozoplayback_object_t context;

} ozoplayback_module_t;


static void
set_audio_device(ozoplayback_module_t *module, audio_devices_t device)
{
    module->context.device = device;
}

static void
set_enabled(ozoplayback_module_t *module, bool enabled)
{
    module->context.enabled = enabled;
}

static bool is_headset_device(audio_devices_t device) {
    const int32_t headsetDevices[] = {
        AUDIO_DEVICE_OUT_USB_HEADSET,
        AUDIO_DEVICE_OUT_WIRED_HEADSET,
        AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
        AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES
    };

    for (size_t i = 0; i < sizeof(headsetDevices) / sizeof(int32_t); i++)
        if (device == (audio_devices_t) headsetDevices[i])
            return true;

    return false;
}

static int
set_loudspeaker_processing_mode(ozoplayback_module_t *module, ozoplayback_state_t state)
{
    ozoplayback_object_t *object = &module->context;

    // No permission to execute loudpeaker widening -> do nothing
    if (module->context.ls_state == PLAYBACK_STATE_NOPERMISSION || state == PLAYBACK_STATE_NOPERMISSION) {
        module->context.ls_state = PLAYBACK_STATE_NOPERMISSION;
        return 0;
    }

    if (object->ls_handle == NULL)
        return -ENOSYS;

    // Transition to active state possible only when target device known
    if (object->device == AUDIO_DEVICE_NONE && state == PLAYBACK_STATE_ACTIVE) {
        ALOGD("Transition to active loudspeaker widening state not possible: %d", module->context.device);
        return 0;
    }

    auto index = (state == PLAYBACK_STATE_INACTIVE) ? 0 : 1;

    auto modes = OzoLoudspeakerWidening_ProcessingModesAvailable(object->ls_handle);
    auto currentMode = OzoLoudspeakerWidening_GetProcessingMode(object->ls_handle);
    ALOGD("Requesting loudspeaker widening mode: %s -> %s (state %i)", currentMode, modes.modeName[index], state);

    // Change mode only when required
    if (strcmp(currentMode, modes.modeName[index])) {
        ALOGD("Change loudspeaker mode to %s", modes.modeName[index]);
        auto returnCode = OzoLoudspeakerWidening_SetProcessingMode(object->ls_handle, modes.modeName[index]);
        if (returnCode != Widening_ReturnCode::WIDENING_RETURN_SUCCESS) {
            ALOGE("Unable to set loudspeaker widening mode: %s %d", modes.modeName[index], returnCode);
            return -EINVAL;
        }

        OzoLoudspeakerWidening_Reset(object->ls_handle);
    }

    module->context.ls_state = state;

    return 0;
}

static int
set_headset_processing_mode(ozoplayback_module_t *module, ozoplayback_state_t state)
{
    ozoplayback_object_t *object = &module->context;

    // No permission to execute headset widening -> do nothing
    if (module->context.hp_state == PLAYBACK_STATE_NOPERMISSION || state == PLAYBACK_STATE_NOPERMISSION) {
        module->context.hp_state = PLAYBACK_STATE_NOPERMISSION;
        return 0;
    }

    if (object->hp_handle == NULL)
        return -ENOSYS;

    // Transition to active state possible only when target device known
    if (object->device == AUDIO_DEVICE_NONE && state == PLAYBACK_STATE_ACTIVE) {
        ALOGD("Transition to active headset widening state not possible: %d", module->context.device);
        return 0;
    }

    auto index = (state == PLAYBACK_STATE_INACTIVE) ? 0 : 1;

    auto modes = OzoHeadsetWidening_ProcessingModesAvailable(object->hp_handle);
    auto currentMode = OzoHeadsetWidening_GetProcessingMode(object->hp_handle);
    ALOGD("Requesting headset widening mode: %s -> %s (state %i)", currentMode, modes.modeName[index], state);

    // Change mode only when required
    if (strcmp(currentMode, modes.modeName[index])) {
        ALOGD("Change headset mode to %s", modes.modeName[index]);
        auto returnCode = OzoHeadsetWidening_SetProcessingMode(object->hp_handle, modes.modeName[index]);
        if (returnCode != Widening_ReturnCode::WIDENING_RETURN_SUCCESS) {
            ALOGE("Unable to set headset widening mode: %s %d", modes.modeName[index], returnCode);
            return -EINVAL;
        }

        OzoHeadsetWidening_Reset(object->hp_handle);
    }

    module->context.hp_state = state;

    return 0;
}

static int
set_next_state(ozoplayback_module_t *module)
{
    pthread_mutex_lock(&gWideningLock);

    // Widening can be activated unless explicitly forced to bypass mode
    if (!module->context.force_bypass) {
        // Loudspeaker widening is enabled when loudspeaker device in use and effect enabled
        if (module->context.device == AUDIO_DEVICE_OUT_SPEAKER && module->context.enabled) {
            auto result = set_loudspeaker_processing_mode(module, PLAYBACK_STATE_ACTIVE) |
                set_headset_processing_mode(module, PLAYBACK_STATE_INACTIVE);

            pthread_mutex_unlock(&gWideningLock);
            return result;
        }

        // Headset widening is enabled when headset device in use and effect enabled
        if (is_headset_device(module->context.device) && module->context.enabled) {
            auto result = set_headset_processing_mode(module, PLAYBACK_STATE_ACTIVE) |
                set_loudspeaker_processing_mode(module, PLAYBACK_STATE_INACTIVE);

            pthread_mutex_unlock(&gWideningLock);
            return result;
        }
    }

    // Otherwise select bypass processing
    auto result = set_loudspeaker_processing_mode(module, PLAYBACK_STATE_INACTIVE) |
            set_headset_processing_mode(module, PLAYBACK_STATE_INACTIVE);

    pthread_mutex_unlock(&gWideningLock);
    return result;
}

// Release audio handles related to Ozo Audio processing
static void
ozoplayback_object_release(ozoplayback_object_t *object)
{
    if (object->ls_handle)
        OzoLoudspeakerWidening_Destroy(object->ls_handle);
    object->ls_handle = NULL;

    if (object->hp_handle)
        OzoHeadsetWidening_Destroy(object->hp_handle);
    object->hp_handle = NULL;
}

// Create handles related to Ozo Audio processing
static int
ozoplayback_object_create(ozoplayback_module_t *module)
{
    ozoplayback_object_t *object = &module->context;

    ozoplayback_object_release(object);

    ALOGD("Ozo loudspeaker widening SDK version=%s", OzoLoudspeakerWidening_GetVersion());
    ALOGD("Ozo headset widening SDK version=%s", OzoHeadsetWidening_GetVersion());

    if (!gLicenseSet) {
        ozoaudiocodec::OzoLicenseReader license;

        if (!license.init(OZOPLAY_LICENSE_FILE)) {
            ALOGE("Unable to read license file %s", OZOPLAY_LICENSE_FILE);
            return -EINVAL;
        }
        ALOGD("License data size: %zu", license.getSize());

        if (!OzoWidening_SetLicense(license.getBuffer(), license.getSize())) {
            ALOGE("License data not accepted");
            return -EINVAL;
        }

        gLicenseSet = true;
    }

    ozoplayback_state_t target_ls_state = PLAYBACK_STATE_INACTIVE;
    ozoplayback_state_t target_hp_state = PLAYBACK_STATE_INACTIVE;

    LWideningConfig wideningConfig;

    wideningConfig.flags = 0;
    wideningConfig.fs = module->config.inputCfg.samplingRate;
    wideningConfig.maxFrameSize = MAX_PROCESSING_FRAME_SIZE;
    wideningConfig.inputChannelCount = 2;
    strcpy(wideningConfig.deviceNameId, OUTPUT_WIDENING_DEVICE);

    ALOGD("\nLoudspeaker widening details:\n");
    ALOGD("----------------\n");
    ALOGD("Sample rate: %d\n", wideningConfig.fs);
    ALOGD("Max frame size: %i\n", wideningConfig.maxFrameSize);
    ALOGD("Input channels: %d\n", wideningConfig.inputChannelCount);
    ALOGD("Device ID: %s\n", wideningConfig.deviceNameId);

    auto returnCode = OzoLoudspeakerWidening_Create(&object->ls_handle, &wideningConfig);
    if (returnCode == Widening_ReturnCode::WIDENING_RETURN_PERMISSION_ERROR) {
        target_ls_state = PLAYBACK_STATE_NOPERMISSION;
        ALOGD("No permission for loudspeaker widening");
    }
    else if (returnCode != Widening_ReturnCode::WIDENING_RETURN_SUCCESS) {
        ALOGE("Loudspeaker widening creation failed, error code: %d", returnCode);
        return -EINVAL;
    }

    HWideningConfig hWideningConfig;

    hWideningConfig.flags = 0;
    hWideningConfig.fs = module->config.inputCfg.samplingRate;
    hWideningConfig.maxFrameSize = MAX_PROCESSING_FRAME_SIZE;

    ALOGD("\nHeadset widening details:\n");
    ALOGD("----------------\n");
    ALOGD("Sample rate: %d\n", hWideningConfig.fs);
    ALOGD("Max frame size: %i\n\n", hWideningConfig.maxFrameSize);

    returnCode = OzoHeadsetWidening_Create(&object->hp_handle, &hWideningConfig);
    if (returnCode == Widening_ReturnCode::WIDENING_RETURN_PERMISSION_ERROR) {
        target_hp_state = PLAYBACK_STATE_NOPERMISSION;
        ALOGD("No permission for headset widening");
    }
    else if (returnCode != Widening_ReturnCode::WIDENING_RETURN_SUCCESS) {
        ALOGE("Headset widening creation failed, error code: %d", returnCode);
        return -EINVAL;
    }

    // Fatal license related error, should not ever occur
    if (target_ls_state == PLAYBACK_STATE_NOPERMISSION && target_hp_state == PLAYBACK_STATE_NOPERMISSION) {
        ALOGD("Invalid license, no permission for loudspeaker or headset widening!");
        return -EINVAL;
    }

    // Set to bypass mode by default
    return set_loudspeaker_processing_mode(module, target_ls_state) |
        set_headset_processing_mode(module, target_hp_state);
}


// Initialize audio widening module
static const effect_handle_t *
playback_create(ozoplayback_module_t *module)
{
    module->context.ls_state = PLAYBACK_STATE_INACTIVE;
    module->context.hp_state = PLAYBACK_STATE_INACTIVE;
    module->context.device = AUDIO_DEVICE_NONE;
    module->context.enabled = false;

    return (effect_handle_t *) module;
}

// Release audio widening module
static void
playback_release(ozoplayback_module_t *module)
{
    ozoplayback_object_release(&module->context);
}

// Initialize widening settings (config, etc)
static int
playback_init(ozoplayback_module_t *module)
{
    ALOGV("playback_init()");

    if (module == NULL)
        return -EINVAL;

    // Initial input config: stereo@48kHz, float
    module->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    module->config.inputCfg.channels = AUDIO_CHANNEL_IN_STEREO;
    module->config.inputCfg.format = AUDIO_FORMAT_PCM_FLOAT;
    module->config.inputCfg.samplingRate = 48000;
    module->config.inputCfg.bufferProvider.getBuffer = NULL;
    module->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    module->config.inputCfg.bufferProvider.cookie = NULL;
    module->config.inputCfg.mask = EFFECT_CONFIG_ALL;

    // Initial output config: stereo@48kHz, float
    module->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_WRITE;
    module->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    module->config.outputCfg.format = AUDIO_FORMAT_PCM_FLOAT;
    module->config.outputCfg.samplingRate = 48000;
    module->config.outputCfg.bufferProvider.getBuffer = NULL;
    module->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    module->config.outputCfg.bufferProvider.cookie = NULL;
    module->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    return 0;
}

// Set new widening effect config
static int
playback_setconfig(ozoplayback_module_t *module, effect_config_t *config)
{
    ALOGV("playback_setconfig()");

    // Rules when config change is allowed:
    // - Input and output sample rate must be the same and 48000Hz
    // - Input channel count must be 2
    // - Input and output format must be float
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
    if (channels != 2) {
        ALOGE("Unsupported channel count: %i", channels);
        return -EINVAL;
    }

    if (config->inputCfg.format != config->outputCfg.format) {
        ALOGE("Format mismatch: %i %i", config->inputCfg.format, config->outputCfg.format);
        return -EINVAL;
    }

    if (config->inputCfg.format != AUDIO_FORMAT_PCM_FLOAT) {
        ALOGE("Unsupported format (only float supported): %i", config->inputCfg.format);
        return -EINVAL;
    }

    if (!(config->outputCfg.accessMode == EFFECT_BUFFER_ACCESS_WRITE ||
          config->outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)) {
        ALOGE("Invalid access mode: %i", config->inputCfg.accessMode);
        return -EINVAL;
    }

    module->config = *config;

    return ozoplayback_object_create(module);
}

// Get current widening effect config
static void
playback_getconfig(ozoplayback_module_t *module, effect_config_t *config)
{
    *config = module->config;
}


// Get widening effect parameter
static int
playback_getparameter(ozoplayback_module_t * /*module*/, int32_t /*param*/, uint32_t * /*pSize*/, void * /*pValue*/)
{
    return 0;
}

// Set widening effect parameter
static int
playback_setparameter(ozoplayback_module_t *module, int32_t param, uint32_t /*size*/, void * /*pValue*/)
{
    switch (param) {
        case OZO_PLAYBACK_FORCE_BYPASS:
            ALOGD("Enable forced bypass mode");
            module->context.force_bypass = true;
            set_next_state(module);
            break;

        default:
            ALOGE("capture_setparameter(): unknown parameter %" PRId16, param);
            return -EINVAL;
    }

    return 0;
}


//
// Public interfaces related to widening effect
//

static int
Playback_Process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    ozoplayback_module_t *module = (ozoplayback_module_t *) self;

    if (module == NULL || inBuffer == NULL || outBuffer == NULL)
        return -EINVAL;

    pthread_mutex_lock(&gWideningLock);

    // Widening selection
    // Take into account also the fact that one of the widenings may not be available
    bool lsActive = module->context.ls_state == PLAYBACK_STATE_ACTIVE;
    if (module->context.ls_state == PLAYBACK_STATE_NOPERMISSION)
        lsActive = false;
    else if (module->context.hp_state == PLAYBACK_STATE_NOPERMISSION)
        lsActive = true;

    float *input = inBuffer->f32;
    float *output = outBuffer->f32;
    int samplesToProcess = inBuffer->frameCount;
    size_t inChannels = audio_channel_count_from_out_mask(module->config.inputCfg.channels);

    ALOGV("Playback_Process(): (format %u), (ch %u %u), (frCount %zu %zu), (proc %i) (states %i %i)",
        module->config.inputCfg.format, module->config.inputCfg.channels,
        module->config.outputCfg.channels, inBuffer->frameCount, outBuffer->frameCount,
        lsActive, module->context.ls_state, module->context.hp_state
    );

    while (samplesToProcess > 0) {
        int bufLen = samplesToProcess;
        if (bufLen > MAX_PROCESSING_FRAME_SIZE)
            bufLen = MAX_PROCESSING_FRAME_SIZE;

        //ALOGV("Process %d samples", bufLen);
        if (lsActive)
            OzoLoudspeakerWidening_Process(module->context.ls_handle, input, output, bufLen);
        else
            OzoHeadsetWidening_Process(module->context.hp_handle, input, output, bufLen);

        input += bufLen * inChannels;
        output += bufLen * inChannels;
        samplesToProcess -= bufLen;
    }

    pthread_mutex_unlock(&gWideningLock);
    return 0;
}

static int
Playback_Command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize, void *pReplyData)
{
    ozoplayback_module_t *module = (ozoplayback_module_t *) self;

    ALOGV("Playback_Command %" PRIu32 " cmdSize %" PRIu32, cmdCode, cmdSize);

    if (module == NULL) {
        return -EINVAL;
    }

    switch (cmdCode) {
        case EFFECT_CMD_INIT:
            ALOGV("Playback_Command: EFFECT_CMD_INIT");
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
                return -EINVAL;

            *(int *) pReplyData = playback_init(module);
            break;

        case EFFECT_CMD_SET_CONFIG:
            ALOGV("Playback_Command: EFFECT_CMD_SET_CONFIG");
            if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) ||
                pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
                return -EINVAL;

            *(int *) pReplyData = playback_setconfig(module, (effect_config_t *) pCmdData);
            break;

        case EFFECT_CMD_GET_CONFIG:
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(effect_config_t))
                return -EINVAL;

            playback_getconfig(module, (effect_config_t *) pReplyData);
            break;

        case EFFECT_CMD_ENABLE:
            ALOGV("Playback_Command: EFFECT_CMD_ENABLE");
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
                return -EINVAL;

            set_enabled(module, true);
            *(int *) pReplyData = set_next_state(module);
            break;

        case EFFECT_CMD_DISABLE:
            ALOGV("Playback_Command: EFFECT_CMD_DISABLE");
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int))
                return -EINVAL;

            set_enabled(module, false);
            *(int *) pReplyData = set_next_state(module);
            break;

        case EFFECT_CMD_GET_PARAM: {
            ALOGV(
                "Playback_Command: EFFECT_CMD_GET_PARAM pCmdData %p, *replySize %" PRIu32 ", pReplyData: %p",
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

            rep->status = playback_getparameter(
                module, *(int32_t *) rep->data, &rep->vsize, rep->data + sizeof(int32_t)
            );
            *replySize = sizeof(effect_param_t) + sizeof(int32_t) + rep->vsize;
            break;
        }

        case EFFECT_CMD_SET_PARAM: {
            ALOGV(
                "Playback_Command: EFFECT_CMD_SET_PARAM cmdSize %d pCmdData %p, *replySize %" PRIu32 ", pReplyData %p",
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

            *(int *) pReplyData = playback_setparameter(
                module, *(int32_t *) cmd->data, cmd->vsize, cmd->data + sizeof(int32_t)
            );
            break;
        }

        case EFFECT_CMD_RESET:
            ALOGV("Playback_Command: EFFECT_CMD_RESET");
            playback_setconfig(module, &module->config);
            set_next_state(module);
            break;

        case EFFECT_CMD_SET_DEVICE: {
            if (pCmdData == NULL) {
                ALOGV("Playback_Command: EFFECT_CMD_SET_DEVICE pCmdData NULL");
                return -EINVAL;
            }

            auto device = (audio_devices_t) (*(uint32_t *) pCmdData);
            ALOGV("Playback_Command: EFFECT_CMD_SET_DEVICE (device %d, speaker=%d)", device, AUDIO_DEVICE_OUT_SPEAKER);

            set_audio_device(module, device);
            set_next_state(module);
            break;
        }

        case EFFECT_CMD_SET_AUDIO_MODE:
            ALOGV("Playback_Command: EFFECT_CMD_SET_AUDIO_MODE (pass)");
            break;

        default:
            ALOGW("Playback_Command: invalid command %" PRIu32, cmdCode);
            return -EINVAL;
    }

    return 0;
}

int
Playback_GetDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
{
    ozoplayback_module_t *module = (ozoplayback_module_t *) self;

    ALOGD("Playback_GetDescriptor");

    if (module == NULL) {
        return -EINVAL;
    }

    memcpy(pDescriptor, &sPlaybackDescriptor, sizeof(effect_descriptor_t));

    return 0;
}

//
// End of public interfaces related to processing effect
//


static const struct effect_interface_s gPlaybackInterface = {
        Playback_Process,
        Playback_Command,
        Playback_GetDescriptor,
        NULL
};


// Available effect descriptors
static const effect_descriptor_t *sDescriptors[OZOPROC_NUM_EFFECTS] = {
        &sPlaybackDescriptor
};

// Available effect interface implementations
static const struct effect_interface_s *sInterfaces[OZOPROC_NUM_EFFECTS] = {
    &gPlaybackInterface
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
    ozoplayback_module_t *module;

    ALOGV("Create() for uuid=%s", uuid2Char(uuid, buffer));

    if (Ozo_GetDescriptor(uuid, index) == NULL) {
        ALOGE("Create: Descriptor not found for uuid=%s", uuid2Char(uuid, buffer));
        return -EINVAL;
    }

    module = (ozoplayback_module_t *) calloc(1, sizeof(ozoplayback_module_t));
    if (module == NULL) {
        ALOGE("Create: Unable to create module");
        return -ENOMEM;
    }

    module->itfe = sInterfaces[index];

    *pInterface = (effect_handle_t) playback_create(module);

    return 0;
}

int
Release(effect_handle_t interface)
{
    ozoplayback_module_t *module = (ozoplayback_module_t *) interface;

    ALOGV("Release()");

    if (interface == NULL)
        return -EINVAL;

    playback_release(module);
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
    .name = "OZO Playback Library",
    .implementor = "Nokia Technologies",
    .create_effect = android::Create,
    .release_effect = android::Release,
    .get_descriptor = android::GetDescriptor
};

}; // extern "C"
