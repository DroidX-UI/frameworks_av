/*
  Copyright (C) 2019 Nokia Corporation.
  This material, including documentation and any related
  computer programs, is protected by copyright controlled by
  Nokia Corporation. All rights are reserved. Copying,
  including reproducing, storing,  adapting or translating, any
  or all of this material requires the prior written consent of
  Nokia Corporation. This material also contains confidential
  information which may not be disclosed to others without the
  prior written consent of Nokia Corporation.
*/
#define LOG_NDEBUG 0
#define LOG_TAG "OzoEffectsApp"
#include <fcntl.h>
#include <string.h>
#include <utils/String16.h>
#include <binder/ProcessState.h>
#include <media/audiohal/EffectBufferHalInterface.h>
#include <media/audiohal/EffectHalInterface.h>
#include <media/audiohal/EffectsFactoryHalInterface.h>
#include <audio_utils/primitives.h>
#include <ozoaudiocommon/utils.h>
#include <ozoaudioenc/ozo_wrapper.h>
#include <ozo/ozo.h> // Ozo effects

#include "OzoAudioWaveFileSource.h"

namespace android {

static void
usage(const char *name)
{
    fprintf(stdout, "Encodes WAVE file into OZO Audio Distribution Format file\n");
    fprintf(stdout, "Usage: %s <options> <input-wave-file> <output-raw-file>\n", name);
    fprintf(stdout, "\nAvailable options:\n\n");
    fprintf(stdout, "-device <x>        : Capturing device ID\n");
}

typedef struct {

    const char *focus_direction;
    const char *device_id;
    const char *encoding_mode;
    int32_t channels;
    int32_t delay;
    int64_t flags;
    int32_t bitrate;
    int32_t samplerate;

} OzoAppConfig;

static int
parse_commandline(char **in_file, char **out_file, OzoAppConfig *config, int argc, char *argv[])
{
    argc--; argv++;
    while (argc > 2)
    {
        if (!strcmp(*argv, "-device"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            if (!strcmp(*argv, "none")) {
                config->device_id = "";
            } else {
                config->device_id = *argv;
            }
        }

        /*-- Unidentified switch. --*/
        else {
            fprintf(stderr, "Unknown switch: %s\n", *argv);
            return (1);
        }

        /*-- Move pointer past the value. --*/
        argv++; argc--;
    }

    *in_file = *argv;
    argv++;
    *out_file = *argv;

    return (0);
}

// OZO Audio effect UUID
#define EFFECT_UUID_OZO__ { 0x7e384a3b, 0x7850, 0x4a64, 0xa097, { 0x88, 0x42, 0x50, 0xd8, 0xa7, 0x37 } }
static const effect_uuid_t EFFECT_UUID_OZO_ = EFFECT_UUID_OZO__;
const effect_uuid_t * const EFFECT_UUID_OZO = &EFFECT_UUID_OZO_;

class OzoEffectTester
{
public:
    OzoEffectTester(OzoAppConfig &config):
    mConfig(config)
    {}

    ~OzoEffectTester()
    {
        mEffectsFactory = 0;
        mEffectInterface = 0;
        mInBuffer = 0;
        mOutBuffer = 0;
    }

    bool init()
    {
        auto status = this->findEffect();
        if (status) {
            if (mEffectsFactory->createEffect(
                &mEffectFxDesc.uuid,
                0, SESSION_ID_INVALID_AND_IGNORED,
                AUDIO_PORT_HANDLE_NONE,
                &mEffectInterface) != 0) {
                ALOGE("Unable to create OZO Audio effect");
                return false;
            }

            ALOGD("OZO Audio effect created");

            return this->initEffect(
                AUDIO_CHANNEL_IN_LEFT | AUDIO_CHANNEL_IN_RIGHT | AUDIO_CHANNEL_IN_FRONT,
                AUDIO_CHANNEL_IN_LEFT | AUDIO_CHANNEL_IN_RIGHT | AUDIO_CHANNEL_IN_FRONT,
                AUDIO_FORMAT_PCM_16_BIT,
                mConfig.samplerate,
                OzoEncoderWrapper::kNumSamplesPerFrame
            );
        }

        return status;
    }

    // Enable focus
    bool enableFocus()
    {
        return this->sendCommand(OZO_PARAM_GENERIC, "focus=on", "enabling focus");
    }

    // Focus gain
    bool setFocusGain(double gain)
    {
        char cmd[64];
        sprintf(cmd, "zoom=%f", gain);
        return this->sendCommand(OZO_PARAM_GENERIC, cmd, "setting focus gain");
    }

    // Focus sector width
    bool setFocusWidth(double width)
    {
        char cmd[64];
        sprintf(cmd, "focus-width=%f", width);
        return this->sendCommand(OZO_PARAM_GENERIC, cmd, "setting focus sector width");
    }

    // Focus sector height
    bool setFocusHeight(double height)
    {
        char cmd[64];
        sprintf(cmd, "focus-height=%f", height);
        return this->sendCommand(OZO_PARAM_GENERIC, cmd, "setting focus sector height");
    }

    // Focus azimuth
    bool setFocusAzimuth(double azimuth)
    {
        char cmd[64];
        sprintf(cmd, "focus-azimuth=%f", azimuth);
        return this->sendCommand(OZO_PARAM_GENERIC, cmd, "setting focus azimuth");
    }

    // Focus elevation
    bool setFocusElevation(double elevation)
    {
        char cmd[64];
        sprintf(cmd, "focus-elevation=%f", elevation);
        return this->sendCommand(OZO_PARAM_GENERIC, cmd, "setting focus elevation");
    }

    bool process(void *input, size_t inSize, int16_t **output, size_t &outSize)
    {
        outSize = 0;
        *output = nullptr;

        mInBuffer->setExternalData(input);
        mInBuffer->setFrameCount(inSize);
        mInBuffer->update();

        mOutBuffer->setFrameCount(inSize);

        auto status = mEffectInterface->process();
        if (status != OK) {
            ALOGE("Effect process error: %d", status);
            return false;
        }

        *output = mOutBuffer->audioBuffer()->s16;
        auto channels = audio_channel_count_from_out_mask(mEffectConfig.outputCfg.channels);
        outSize = mOutBuffer->audioBuffer()->frameCount * channels;

        // Strip the muted channels
        auto src = *output;
        auto dst = *output;
        for (size_t i = 0; i < outSize; i+=channels) {
            for (size_t ch = 0; ch < 2; ch++)
                *dst++ = *src++;
            src += channels - 2;
        }

        outSize = mOutBuffer->audioBuffer()->frameCount * 2;

        return true;
    }

protected:
    bool sendCommand(int commandId, const char *cmd, const char *cmdTag)
    {
        int cmdStatus;
        uint8_t buf[128];
        uint32_t replySize = sizeof(int);

        effect_param_t *p = (effect_param_t *) buf;
        p->psize = sizeof(uint32_t);
        p->vsize = strlen(cmd);
        *(int32_t *)p->data = commandId;
        memcpy(p->data + sizeof(uint32_t), cmd, p->vsize);

        auto cmdSize = sizeof(effect_param_t) + strlen(cmd) + 4;
        auto status = mEffectInterface->command(
            EFFECT_CMD_SET_PARAM, cmdSize, &buf, &replySize, &cmdStatus
        );

        if (status != 0 || cmdStatus != 0) {
            ALOGE("Error %d cmdStatus %d while %s", status, cmdStatus, cmdTag);
            return false;
        }

        return true;
    }

    bool findEffect()
    {
        mEffectsFactory = EffectsFactoryHalInterface::create();
        if (mEffectsFactory == 0) {
            ALOGE("Could not obtain the effects factory");
            return false;
        }

        uint32_t numEffects = 0;
        int ret = mEffectsFactory->queryNumberEffects(&numEffects);
        if (ret != 0) {
            ALOGE("Error querying number of effects: %d", ret);
            return false;
        }
        ALOGD("Number of effects available: %d", numEffects);

        bool found = false;

        for (uint32_t i = 0 ; i < numEffects; i++)
            if (mEffectsFactory->getDescriptor(i, &mEffectFxDesc) == 0)
                ALOGD("Effect %d is called %s", i, mEffectFxDesc.name);

        for (uint32_t i = 0 ; i < numEffects; i++) {
            if (mEffectsFactory->getDescriptor(i, &mEffectFxDesc) == 0) {
                if (memcmp(&mEffectFxDesc.uuid, EFFECT_UUID_OZO, sizeof(effect_uuid_t)) == 0) {
                    ALOGD("Found effect \"%s\" from %s", mEffectFxDesc.name, mEffectFxDesc.implementor);
                    found = true;
                    break;
                }
            }
        }

        return found;
    }

    bool initEffect(
        audio_channel_mask_t inputChannelMask,
        audio_channel_mask_t outputChannelMask,
        audio_format_t format,
        uint32_t sampleRate,
        size_t bufferFrameCount
    )
    {
        mEffectConfig.inputCfg.channels = inputChannelMask;
        mEffectConfig.outputCfg.channels = outputChannelMask;
        mEffectConfig.inputCfg.format = format;
        mEffectConfig.outputCfg.format = format;
        mEffectConfig.inputCfg.samplingRate = sampleRate;
        mEffectConfig.outputCfg.samplingRate = sampleRate;
        mEffectConfig.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
        mEffectConfig.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_WRITE;

        auto bytes_sample = audio_bytes_per_sample(format);
        mInFrameSize = bytes_sample * audio_channel_count_from_out_mask(inputChannelMask);
        mOutFrameSize = bytes_sample * audio_channel_count_from_out_mask(outputChannelMask);

        ALOGD(
            "audio_channel_count_from_out_mask(inputChannelMask) = %i",
            audio_channel_count_from_out_mask(inputChannelMask)
        );

        auto inFrameSize = mInFrameSize * bufferFrameCount;
        auto status = mEffectsFactory->mirrorBuffer(nullptr, inFrameSize, &mInBuffer);
        if (status != 0) {
            ALOGE("Error %d while creating input buffer", status);
            return false;
        }

        status = mEffectsFactory->mirrorBuffer(nullptr, mOutFrameSize * bufferFrameCount, &mOutBuffer);
        if (status != 0) {
            ALOGE("Error %d while creating output buffer", status);
            return false;
        }

        mEffectInterface->setInBuffer(mInBuffer);
        mEffectInterface->setOutBuffer(mOutBuffer);

        int cmdStatus;
        uint32_t replySize = sizeof(int);
        status = mEffectInterface->command(
            EFFECT_CMD_SET_CONFIG, sizeof(effect_config_t), &mEffectConfig, &replySize, &cmdStatus
        );
        if (status != 0 || cmdStatus != 0) {
            ALOGE("Error %d while setting effect config", status);
            return false;
        }

        return this->configureEffect();
    }

    // Configure effect
    bool configureEffect()
    {
        int cmdStatus;
        uint32_t replySize = sizeof(int);

        auto status = mEffectInterface->command(
                EFFECT_CMD_SET_CONFIG /*cmdCode*/, sizeof(effect_config_t) /*cmdSize*/,
                &mEffectConfig /*pCmdData*/,
                &replySize, &cmdStatus /*pReplyData*/
        );

        if (status != 0 || cmdStatus != 0) {
            ALOGE("Error %d cmdStatus %d while configuring effect", status, cmdStatus);
            return false;
        }

        // Assign device ID and then enable effect
        auto ret = this->setDevice(mConfig.device_id);
        if (ret) ret = this->enableEffect();

        return ret;
    }

    // Enable effect
    bool enableEffect()
    {
        int cmdStatus;
        uint32_t replySize = sizeof(int);

        auto status = mEffectInterface->command(
            EFFECT_CMD_ENABLE /*cmdCode*/, 0 /*cmdSize*/, NULL /*pCmdData*/,
            &replySize, &cmdStatus /*pReplyData*/
        );

        if (status != 0 || cmdStatus != 0) {
            ALOGE("Error %d cmdStatus %d while enabling effect", status, cmdStatus);
            return false;
        }

        return true;
    }

    // Set device ID to the effect
    bool setDevice(const char *device_id)
    {
#define DEVICE_CMD_BUF_SIZE sizeof(effect_param_t) + 36 + 4
        int cmdStatus;
        uint32_t replySize = sizeof(int);
        uint8_t buf[DEVICE_CMD_BUF_SIZE];

        effect_param_t *p = (effect_param_t *) buf;
        p->psize = sizeof(uint32_t);
        p->vsize = strlen(device_id);
        *(int32_t *)p->data = OZO_PARAM_DEVICE_UUID; // Command ID for device ID assignment
        memcpy(p->data + sizeof(uint32_t), device_id, p->vsize);

        auto status = mEffectInterface->command(
            EFFECT_CMD_SET_PARAM, DEVICE_CMD_BUF_SIZE, &buf, &replySize, &cmdStatus
        );

        if (status != 0 || cmdStatus != 0) {
            ALOGE("Error %d cmdStatus %d while assigning device ID", status, cmdStatus);
            return false;
        }

        return true;
    }

    static const int32_t SESSION_ID_INVALID_AND_IGNORED = -2;

    const OzoAppConfig &mConfig;

    sp<EffectsFactoryHalInterface> mEffectsFactory;
    sp<EffectHalInterface> mEffectInterface;
    size_t mInFrameSize;
    size_t mOutFrameSize;
    sp<EffectBufferHalInterface> mInBuffer;
    sp<EffectBufferHalInterface> mOutBuffer;
    effect_config_t mEffectConfig;
    effect_descriptor_t mEffectFxDesc;
};

} // namespace android

int
main(int argc, char* argv[])
{
    android::OzoAppConfig config;
    char *in_file, *out_file;

    config.focus_direction = nullptr;
    // Pixel 3 device ID
    config.device_id = "4238B2D5-0234-41C0-8E49-BB5D3FD474DC";
    config.encoding_mode = nullptr;
    config.channels = 2;
    config.delay = -1;
    config.flags = -1;
    config.bitrate = 256000;

    if (argc < 3) {
        android::usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (android::parse_commandline(&in_file, &out_file, &config, argc, argv))
    {
        android::usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    android::ProcessState::self()->startThreadPool();

    android::sp<android::MediaSource> wavSource = new android::OzoAudioWaveFileSource(in_file);

    android::sp<android::MetaData> meta = new android::MetaData;
    meta->setInt32(android::OzoAudioWaveFileSource::kKeyBufferSize, 1000);
    wavSource->start(meta.get());

    int32_t channels, samplerate;
    android::sp<android::MetaData> micFormat = wavSource->getFormat();
    micFormat->findInt32(android::kKeyChannelCount, &channels);
    micFormat->findInt32(android::kKeySampleRate, &samplerate);

    config.samplerate = samplerate;
    android::OzoEffectTester ozoEffect(config);

    if (!ozoEffect.init())
        return EXIT_FAILURE;

    FILE *fp = fopen(out_file, "wb");
    if (!fp) {
        ALOGE("Unable to open file %s for writing", out_file);
        return EXIT_FAILURE;
    }

    if (!ozoEffect.enableFocus())
        return EXIT_FAILURE;

    if (!ozoEffect.setFocusHeight(100.0))
        return EXIT_FAILURE;

    if (!ozoEffect.setFocusAzimuth(0.0))
        return EXIT_FAILURE;

    if (!ozoEffect.setFocusElevation(0.0))
        return EXIT_FAILURE;

#if 0
    bool inc = true;
#endif

#if 1
    size_t frames = 0;
    double gain = 3.0f;
    double width = 100.0f; // 30.0f
#else
    size_t frames = 0;
    double gain = 3.0f;
    double width = 30.0f;
#endif

    if (!ozoEffect.setFocusGain(gain))
        return EXIT_FAILURE;

    if (!ozoEffect.setFocusWidth(width))
        return EXIT_FAILURE;

    android::MediaBufferBase *buffer;
    while (wavSource->read(&buffer) == android::OK) {
        int16_t *outBuffer;
        size_t outSize = 0;
        size_t audioSize = buffer->size() / sizeof(int16_t);

#if 0
        if (frames && ((frames % 200) == 0)) {
            if (!ozoEffect.setFocusGain(gain))
                return EXIT_FAILURE;

            fprintf(stdout, "%f\n", gain); fflush(stdout);

            if (inc) {
                gain += 1.0f;
                if (gain > 5.0f) {
                    fprintf(stdout, "Max gain reached: %f\n", gain); fflush(stdout);
                    inc = false;
                    gain = 5.0f;
                }
            } else {
                gain -= 1.0f;
                if (gain < 3.0f) {
                    fprintf(stdout, "Min gain reached: %f\n", gain); fflush(stdout);
                    inc = true;
                    gain = 3.0f;
                }
            }
        }
#endif

#if 0
        if (1) {
            if (!ozoEffect.setFocusGain(gain))
                return EXIT_FAILURE;

            if (!ozoEffect.setFocusWidth(width))
                return EXIT_FAILURE;

            fprintf(stdout, "%f %f\n", gain, width); fflush(stdout);

            if (inc) {
                gain += 0.01f;
                width -= 0.35f;
                if (gain > 5.0f || width < 30.0f) {
                    fprintf(stdout, "Min reached: %f %f\n", gain, width); fflush(stdout);
                    inc = false;
                    gain = 5.0f;
                    width = 30.0f;
                }
            } else {
                gain -= 0.01f;
                width += 0.35f;

                if (gain < 3.0f || width > 100.0f) {
                    fprintf(stdout, "Max reached: %f %f\n", gain, width); fflush(stdout);
                    inc = true;
                    gain = 3.0f;
                    width = 100.0f;
                }
            }
        }
#endif

        //ALOGD("Input audio samples read: %zu", audioSize);

        ozoEffect.process(buffer->data(), audioSize / channels, &outBuffer, outSize);
        if (outSize) {
            fwrite(outBuffer, sizeof(int16_t), outSize, fp);
        }

        buffer->release();
        buffer = NULL;

        frames++;
    }

    fclose(fp);

    return EXIT_SUCCESS;
}
