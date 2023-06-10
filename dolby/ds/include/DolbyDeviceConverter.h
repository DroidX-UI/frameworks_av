/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2015-2021 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/

#ifndef DOLBY_DEVICE_CONVERTER_
#define DOLBY_DEVICE_CONVERTER_

#include <system/audio.h>
#include <utils/String8.h>

namespace dolby {

class DeviceConverter {
public:
    static audio_devices_t fromSystemDevice(audio_devices_t device)
    {
        // NOTE: The following mappings must be updated if new
        // devices are added to Android in a new version or by an OEM.
        const audio_devices_t speaker = (const audio_devices_t)
                                        (AUDIO_DEVICE_OUT_EARPIECE
                                         | AUDIO_DEVICE_OUT_SPEAKER
                                         | AUDIO_DEVICE_OUT_SPEAKER_SAFE);

        const audio_devices_t headphone = (const audio_devices_t)
                                          (AUDIO_DEVICE_OUT_WIRED_HEADSET
                                           | AUDIO_DEVICE_OUT_WIRED_HEADPHONE
                                           | AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET
                                           | AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET
                                           | AUDIO_DEVICE_OUT_HEARING_AID
                                           | AUDIO_DEVICE_OUT_ECHO_CANCELLER);

        const audio_devices_t bluetooth = (const audio_devices_t)
                                          (AUDIO_DEVICE_OUT_BLUETOOTH_SCO
                                           | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET
                                           | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT
                                           | AUDIO_DEVICE_OUT_BLUETOOTH_A2DP
                                           | AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES
                                           | AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER);

        const audio_devices_t usb = (const audio_devices_t)
                                    (AUDIO_DEVICE_OUT_USB_ACCESSORY
                                     | AUDIO_DEVICE_OUT_USB_DEVICE
                                     | AUDIO_DEVICE_OUT_USB_HEADSET);

        const audio_devices_t wired_passthrough = (const audio_devices_t)
                                                  (AUDIO_DEVICE_OUT_AUX_DIGITAL
                                                   | AUDIO_DEVICE_OUT_HDMI
                                                   | AUDIO_DEVICE_OUT_HDMI_ARC
                                                   | AUDIO_DEVICE_OUT_SPDIF
                                                   | AUDIO_DEVICE_OUT_TELEPHONY_TX
                                                   | AUDIO_DEVICE_OUT_LINE
                                                   | AUDIO_DEVICE_OUT_FM
                                                   | AUDIO_DEVICE_OUT_AUX_LINE
                                                   | AUDIO_DEVICE_OUT_IP
                                                   | AUDIO_DEVICE_OUT_BUS);

        const audio_devices_t wireless_passthrough = (const audio_devices_t)
                                                     (AUDIO_DEVICE_OUT_REMOTE_SUBMIX
                                                      | AUDIO_DEVICE_OUT_PROXY);

        // The order of following statements is important. They
        // specify preference in which device tunings are applied.
        if (device & wired_passthrough)    return AUDIO_DEVICE_OUT_AUX_DIGITAL;
        if (device & wireless_passthrough) return AUDIO_DEVICE_OUT_REMOTE_SUBMIX;
        if (device & bluetooth)   return AUDIO_DEVICE_OUT_BLUETOOTH_A2DP;
        if (device & usb)         return AUDIO_DEVICE_OUT_USB_DEVICE;
        if (device & headphone)   return AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
        if (device & speaker)     return AUDIO_DEVICE_OUT_SPEAKER;
        // Return none of device was not mapped
        return AUDIO_DEVICE_NONE;
    }

    static const char *toString(audio_devices_t device)
    {
        switch (device) {
            case AUDIO_DEVICE_OUT_SPEAKER:
                return "speaker";
            case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
                return "headset";
            case AUDIO_DEVICE_OUT_AUX_DIGITAL:
                return "hdmi";
            case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP:
                return "bluetooth";
            case AUDIO_DEVICE_OUT_USB_DEVICE:
                return "usb";
            case AUDIO_DEVICE_OUT_REMOTE_SUBMIX:
                return "miracast";
            default:
                return "invalid";
        }
    }

    static audio_devices_t fromString(const char *deviceName)
    {
        const android::String8 device(deviceName);
        static const audio_devices_t kDevices[] = {
            AUDIO_DEVICE_OUT_SPEAKER,
            AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
            AUDIO_DEVICE_OUT_BLUETOOTH_A2DP,
            AUDIO_DEVICE_OUT_USB_DEVICE,
            AUDIO_DEVICE_OUT_REMOTE_SUBMIX
        };
        // Special case for HDMI since "hdmi2", "hdmi6" & "hdmi8" are possible
        // If device name starts with hdmi, it is hdmi
        if (device.find(toString(AUDIO_DEVICE_OUT_AUX_DIGITAL)) == 0) {
            return AUDIO_DEVICE_OUT_AUX_DIGITAL;
        }
        for (unsigned i = 0; i < (sizeof(kDevices)/sizeof(kDevices[0])); ++i) {
            if (device == toString(kDevices[i])) {
                return kDevices[i];
            }
        }
        return AUDIO_DEVICE_NONE;
    }
};

}

#endif//DOLBY_DEVICE_CONVERTER_
