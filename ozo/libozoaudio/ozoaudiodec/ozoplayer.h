#pragma once
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

#include <media/AudioEffect.h>

#include "ozoaudiodec/ozosniffer.h"
#include "ozoaudio/ozo_tags.h"

namespace android {

// Ozo playback virtualizer effect IDs (type and UUID)
#define OZO_STEREO_EFFECT_TYPE "37cc2c00-dddd-11db-8577-0002a5d5c51b"
#define OZO_STEREO_EFFECT_UUID "79bbd08d-8f08-4e96-925a-430f2eb96bf1"

class OzoPlaybackController: public RefBase
{
public:
    OzoPlaybackController(int32_t sessionId):
    mSpeakerOut(true),
    mInitialized(false)
    {
        ALOGD("OzoPlaybackController sessionId: %i", sessionId);

        if (sessionId >= 0) {
            // Look for Ozo playback effect instance and disable it
            content::AttributionSourceState attributionSource;
            attributionSource.packageName = "OzoPlaybackController";
            attributionSource.token = sp<BBinder>::make();
            mEffect = new AudioEffect(attributionSource);
            mEffect->set(OZO_STEREO_EFFECT_TYPE, OZO_STEREO_EFFECT_UUID,
                0, NULL, NULL, (audio_session_t) sessionId);

            if (mEffect.get() && mEffect->initCheck() == NO_ERROR) {
                // Effect processing must be switched to bypass mode as
                // decoder output from Ozo is already widened
                if (!this->sendEffectCommand(111111))
                    ALOGE("Unable to force Ozo widening effect into bypass mode");
            }
        }
    }

    ~OzoPlaybackController() {
        mEffect = 0;
    }

    // Detect changes in the loudspeaker/headset configuration and send change request to decoder
    void signal(const sp<MediaCodec> &decoder)
    {
        const char *strKey = "vendor.ozoaudio.ls-widening.value";

        bool status = this->isSpeakerEnabled();
        if (!mInitialized || mSpeakerOut != status) {
            ALOGD("Output device changed, signal to Ozo decoder: %i(previous), %i(current), %i(initialized)",
                mSpeakerOut, status, mInitialized);

            sp<AMessage> message = new AMessage;
            auto value = status ? OZO_FEATURE_ENABLED : OZO_FEATURE_DISABLED;
            message->setString(strKey, value);
            decoder->setParameters(message);
            mInitialized = true;
        }

        mSpeakerOut = status;
    }

private:
    bool mSpeakerOut;
    bool mInitialized;
    sp<AudioEffect> mEffect;

    bool isSpeakerEnabled()
    {
        const int32_t headsetDevices[] = {
            AUDIO_DEVICE_OUT_USB_HEADSET,
            AUDIO_DEVICE_OUT_WIRED_HEADSET,
            AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
            AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES
        };

        // Look for headset enabled audio devices
        for (size_t i = 0; i < sizeof(headsetDevices) / sizeof(int32_t); i++) {
            auto status = AudioSystem::getDeviceConnectionState((audio_devices_t) headsetDevices[i], "");
            //ALOGD("get audio device status: %i %i", headsetDevices[i], status);
            if (status == AUDIO_POLICY_DEVICE_STATE_AVAILABLE)
                return false;
        }

        return true;
    }

    bool sendEffectCommand(int commandId)
    {
        int cmdStatus;
        uint8_t buf[32];
        uint32_t replySize = sizeof(int);

        effect_param_t *p = (effect_param_t *) buf;
        p->psize = sizeof(uint32_t);
        p->vsize = 0;
        *(int32_t *)p->data = commandId;

        auto cmdSize = sizeof(effect_param_t) + sizeof(int32_t);
        auto status = mEffect->command(
            EFFECT_CMD_SET_PARAM, cmdSize, &buf, &replySize, &cmdStatus
        );

        if (status != 0 || cmdStatus != 0)
            return false;

        return true;
    }
};

// Try to configure OZO Audio decoding
static inline void tryOzoAudioConfigure(const sp<AMessage> &format, AString &mime, OzoPlaybackController* &playCtrl)
{
    sp<ABuffer> csd;
    if (format->findBuffer("csd-0", &csd)) {
        if (setOzoMimeType(csd->data(), csd->size(), mime)) {
            int32_t sessionId = -1;
            format->findInt32("audiosession-id", &sessionId);
            playCtrl = new OzoPlaybackController(sessionId);
        }
    }
}

}  // namespace android
