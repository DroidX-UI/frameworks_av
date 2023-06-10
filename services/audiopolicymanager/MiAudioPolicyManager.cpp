#define LOG_TAG "MiAudioPolicyManager"
#define LOG_NDEBUG 0

#include "MiAudioPolicyManager.h"

#include <binder/IPCThreadState.h>
#include <binder/PermissionController.h>
#include <cutils/bitops.h>
#include <cutils/properties.h>
#include <media/AudioParameter.h>
#include <utils/Log.h>

// 70207ba
#include "send_data_to_xlog.h"
#include "xlog_api.h"

namespace android {

// 4563d58
static const int MAX_DEVICE_SIZE = 2;
static const int MAX_DEVICE_NAME_SIZE = 32;
static const int spatialDevice[MAX_DEVICE_SIZE][MAX_DEVICE_NAME_SIZE] = {
    {0x58, 0x69, 0x61, 0x6f, 0x6d, 0x69, 0x20, 0x42, 0x75, 0x64, 0x73, 0x20, 0x33, 0x20, 0x50, 0x72, 0x6f},
    {0x58, 0x69, 0x61, 0x6f, 0x6d, 0x69, 0x20, 0x42, 0x75, 0x64, 0x73, 0x20, 0x33, 0x54, 0x20, 0x50, 0x72, 0x6f}};

// ----------------------------------------------------------------------------
// AudioPolicyInterface implementation
// ----------------------------------------------------------------------------
extern "C" AudioPolicyInterface *createAudioPolicyManager(
    AudioPolicyClientInterface *clientInterface) {
    MiAudioPolicyManager *apm = new MiAudioPolicyManager(clientInterface);
    status_t status = apm->initialize();
    if (status != NO_ERROR) {
        delete apm;
        apm = nullptr;
    }
    return apm;
}

extern "C" void destroyAudioPolicyManager(AudioPolicyInterface *interface) {
    delete interface;
}

// TODO move into Audio Feature Contorl Center
static int isKaraokeApp(String16 ClientName)  // 0b43f08
{
    return !String16("com.tencent.karaoke").compare(ClientName) ||
           !String16("com.changba").compare(ClientName);
}

//UllApp
static int isUseUllApp(String16 ClientName)
{
    return !String16("com.wedobest.szhrd.mi").compare(ClientName) ||
           !String16("io.iftech.android.box").compare(ClientName) ||
           !String16("com.android.cts.verifier").compare(ClientName);
}

// 4563d58
static bool isSpatialAudioSupport(const char *device_name) {
    char deviceNameSwitch[MAX_DEVICE_SIZE][MAX_DEVICE_NAME_SIZE];
    for (int i = 0; i < MAX_DEVICE_SIZE; i++) {
        for (int j = 0; j < MAX_DEVICE_NAME_SIZE; j++) {
            if (spatialDevice[i][j] == 0)
                break;
            deviceNameSwitch[i][j] = (char)spatialDevice[i][j];
        }
        if (strcmp(deviceNameSwitch[i], device_name) == 0)
            return true;
    }
    return false;
}

// 0b43f08
String16 MiAudioPolicyManager::getPackageName(uid_t uid) {
    String16 mClientName;
    if (uid == 0)
        uid = IPCThreadState::self()->getCallingUid();

    PermissionController permissionController;
    Vector<String16> packages;
    permissionController.getPackagesForUid(uid, packages);
    if (packages.isEmpty()) {
        ALOGD("getPackageName: No packages for calling UID");
    } else {
        mClientName = packages[0];
        ALOGD("getPackageName: %s  uid %d", String8(mClientName).string(), uid);
    }
    return mClientName;
}

status_t MiAudioPolicyManager::setDeviceConnectionStateInt(const sp<DeviceDescriptor> &device,
                                                         audio_policy_dev_state_t state)
{
    status_t status = NO_ERROR;
    audio_devices_t deviceType = device->type();
    const std::string& device_address = device->address();
    const std::string& device_name = device->getName();
    audio_format_t encodedFormat = device->getEncodedFormat();

    ALOGD("setDeviceConnectionStateInt() device: 0x%X, state %d, address %s name %s format 0x%X",
          deviceType, state, device_address.c_str(), device_name.c_str(), encodedFormat);

    // 70207ba
    // for onetrack rbis
    if (state == AUDIO_POLICY_DEVICE_STATE_AVAILABLE) {
        send_headphone_type_to_xlog(deviceType, device_name.c_str(), encodedFormat);
    }

    // 2bf9682 START
    // #ifdef SPATIAL_AUDIO_ENABLED
    if (deviceType == AUDIO_DEVICE_OUT_BLUETOOTH_A2DP) {
        ALOGD("update %s connect status", device_name.c_str());
        if (isSpatialAudioSupport(device_name.c_str()) && state == 1) {
            mSpatialAudioAddress.setTo(device_address.c_str());
            mpClientInterface->setParameters(AUDIO_IO_HANDLE_NONE,
                                             String8("spatial_audio_headphone_connect=true"));
        } else if (strcmp(mSpatialAudioAddress.c_str(), "00:00:00:00:00:00") != 0 && strcmp(mSpatialAudioAddress.c_str(), device_address.c_str()) == 0 && state != 1) {
            mpClientInterface->setParameters(AUDIO_IO_HANDLE_NONE,
                                             String8("spatial_audio_headphone_connect=false"));
        }
    }
    // #endif
    // 2bf9682 END

    // 4a0d74c START
    for (size_t i = 0; i < mOutputs.size(); i++) {
        sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
        if (desc->isActive() && ((mEngine->getPhoneState() != AUDIO_MODE_IN_CALL) ||
                                 (desc != mPrimaryOutput))) {
            if ((deviceType == AUDIO_DEVICE_OUT_MULTIROUTE) &&
                (state == AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE)) {
                mEngine->setForceSkipDevice(AUDIO_SKIP_DEVIVE_DEFAULT);
            }
        }
    }
    // 4a0d74c END

    // handle output devices
    if (audio_is_output_device(deviceType)) {
        switch (state) {
            // handle output device connection
            case AUDIO_POLICY_DEVICE_STATE_AVAILABLE: {
                // 7c19778 START
                if ((AUDIO_DEVICE_OUT_WIRED_HEADSET == deviceType) || (AUDIO_DEVICE_OUT_USB_HEADSET == deviceType)) {
                    property_set("persist.audio.headset.plug.status", "on");
                }  // 7c19778 END
            } break;
            // handle output device disconnection
            case AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE: {
                // 7c19778 START
                if ((AUDIO_DEVICE_OUT_WIRED_HEADSET == deviceType) || (AUDIO_DEVICE_OUT_USB_HEADSET == deviceType)) {
                    property_set("persist.audio.headset.plug.status", "off");
                }  // 7c19778 END
            } break;
            default:
                ALOGE("%s() invalid state: %x", __func__, state);
                return BAD_VALUE;
        }
    }  // end if is output device

    // c6446a8 START
    status = AudioPolicyManager::setDeviceConnectionStateInt(device, state);

    if (status == NO_ERROR) {
        if (audio_is_output_device(deviceType)) {
            switch (state) {
                case AUDIO_POLICY_DEVICE_STATE_AVAILABLE: {
                } break;
                case AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE: {
                    if ((popcount(deviceType) == 1) && (deviceType == AUDIO_DEVICE_OUT_WIRED_HEADPHONE)) {
                        for (const auto &activeDesc : mInputs.getActiveInputs()) {
                            if (activeDesc->source() == AUDIO_SOURCE_VOICE_COMMUNICATION &&
                                activeDesc->getDeviceType() == AUDIO_DEVICE_IN_BACK_MIC) {
                                auto newDevice = getNewInputDevice(activeDesc);
                                if (activeDesc->mProfile->getSupportedDevices().contains(newDevice)) {
                                    ALOGD("route tx to AUDIO_DEVICE_IN_BUILTIN_MIC for VoIP when 3-pole headphone disconnected");
                                    setInputDevice(activeDesc->mIoHandle, newDevice);
                                }
                            }
                        }
                    }
                } break;
                default:
                    break;
            }
        }  // end if is output device
    }
    // c6446a8 END

    return status;
}

status_t MiAudioPolicyManager::getOutputForAttr(const audio_attributes_t *attr,
                                                audio_io_handle_t *output,
                                                audio_session_t session,
                                                audio_stream_type_t *stream,
                                                const AttributionSourceState &attributionSource,
                                                const audio_config_t *config,
                                                audio_output_flags_t *flags,
                                                audio_port_handle_t *selectedDeviceId,
                                                audio_port_handle_t *portId,
                                                std::vector<audio_io_handle_t> *secondaryOutputs,
                                                output_type_t *outputType,
                                                bool *isSpatialized) {
    mCallingAppuid = attributionSource.uid;  // 0b43f08
    return AudioPolicyManager::getOutputForAttr(attr, output, session, stream,
                                                attributionSource, config,
                                                flags,
                                                selectedDeviceId,
                                                portId,
                                                secondaryOutputs,
                                                outputType,
                                                isSpatialized);
}

status_t MiAudioPolicyManager::startSource(const sp<SwAudioOutputDescriptor> &outputDesc,
                                           const sp<TrackClientDescriptor> &client,
                                           uint32_t *delayMs) {
    // 5ec36bb START
    if (mMultiRoutes.size() > 0) {
        bool deviceSwitch = false;
        DeviceVector devices;
        audio_stream_type_t stream = client->stream();
        if (outputDesc->mFlags & AUDIO_OUTPUT_FLAG_VIRTUAL_DEEP_BUFFER) {
            audio_devices_t device = (audio_devices_t)mMultiRoutes.valueAt(0);
            DeviceVector newDevice = mAvailableOutputDevices.getDevicesFromType(device);
            devices.add(newDevice);
        } else {
            deviceSwitch = true;
        }

        if (deviceSwitch) {
            if (devices.isEmpty()) {
                devices = getNewOutputDevices(outputDesc, false /*fromCache*/);
            }
            setOutputDevices(outputDesc, devices, /*force*/ false, 0, NULL, /*requiresMuteCheck*/ false);

            // apply volume rules for current stream and device if necessary
            auto &curves = getVolumeCurves(client->attributes());
            checkAndSetVolume(curves, client->volumeSource(),
                              getVolumeCurves(stream).getVolumeIndex(outputDesc->devices().types()),
                              outputDesc,
                              outputDesc->devices().types());
        }
    }  // 5ec36bb END

    return AudioPolicyManager::startSource(outputDesc, client, delayMs);
}

void MiAudioPolicyManager::checkOutputForAttributes(const audio_attributes_t &attr) {
    // 5ec36bb START
    if (mMultiRoutes.size()) {
        DeviceVector oldDevices = mEngine->getOutputDevicesForAttributes(attr, 0, true /*fromCache*/);
        DeviceVector newDevices = mAvailableOutputDevices.getDevicesFromType(AUDIO_DEVICE_OUT_MULTIROUTE);
        SortedVector<audio_io_handle_t> srcOutputs = getOutputsForDevices(oldDevices, mPreviousOutputs);
        SortedVector<audio_io_handle_t> dstOutputs = getOutputsForDevices(newDevices, mOutputs);

        // also take into account external policy-related changes: add all outputs which are
        // associated with policies in the "before" and "after" output vectors
        ALOGD("%s(): policy related outputs", __func__);
        for (size_t i = 0; i < mPreviousOutputs.size(); i++) {
            const sp<SwAudioOutputDescriptor> desc = mPreviousOutputs.valueAt(i);
            if (desc != 0 && desc->mPolicyMix != NULL) {
                srcOutputs.add(desc->mIoHandle);
                ALOGD(" previous outputs: adding %d", desc->mIoHandle);
            }
        }
        for (size_t i = 0; i < mOutputs.size(); i++) {
            const sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
            if (desc != 0 && desc->mPolicyMix != NULL) {
                dstOutputs.add(desc->mIoHandle);
                ALOGD(" new outputs: adding %d", desc->mIoHandle);
            }
        }
    }  // 5ec36bb END

    /*Super method calling*/
    AudioPolicyManager::checkOutputForAttributes(attr);
}

audio_io_handle_t MiAudioPolicyManager::getOutputForDevices(
    const DeviceVector &devices,
    audio_session_t session,
    const audio_attributes_t *attr,
    const audio_config_t *config,
    audio_output_flags_t *flags,
    bool *isSpatialized,
    bool forceMutingHaptic) {
    // 0b43f08 START
    bool is_karaoke_mode_on = false;
    AudioParameter ret;
    String8 value_Str;
    String16 clientName = getPackageName(mCallingAppuid);
    audio_io_handle_t superGetOutputForDevicesRet;
    audio_stream_type_t stream = mEngine->getStreamTypeForAttributes(*attr);
    value_Str = mpClientInterface->getParameters((audio_io_handle_t)0,
                                                 String8("audio_karaoke_running"));
    ret = AudioParameter(value_Str);
    if (ret.get(String8("audio_karaoke_running"), value_Str) == NO_ERROR) {
        is_karaoke_mode_on = value_Str.contains("true");
    }
    if ((*flags == AUDIO_OUTPUT_FLAG_NONE) &&
        (stream == AUDIO_STREAM_MUSIC) &&
        ((config->offload_info.usage == AUDIO_USAGE_MEDIA) ||
         (config->offload_info.usage == AUDIO_USAGE_GAME))) {
        audio_output_flags_t old_flags = *flags;
        if (is_karaoke_mode_on || isKaraokeApp(clientName)) {
            *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_DEEP_BUFFER);
            ALOGD("Force Deep buffer Flag .. old flags(0x%x)", old_flags);
        }
    }  // 0b43f08 END
    if((*flags & (AUDIO_OUTPUT_FLAG_RAW | AUDIO_OUTPUT_FLAG_MMAP_NOIRQ)) != 0 && !isUseUllApp(clientName)){
        *flags = (audio_output_flags_t)(*flags & ~ (AUDIO_OUTPUT_FLAG_RAW | AUDIO_OUTPUT_FLAG_MMAP_NOIRQ));
        ALOGD("Remove AUDIO_OUTPUT_FLAG_RAW Flag and AUDIO_OUTPUT_FLAG_MMAP_NOIRQ for forbiding ull track route");
    }
     /*
      * Check for VOIP Flag override for voice streams using linear pcm,
      * but not when intended for uplink device(i.e. Telephony Tx)
      */
     if (stream == AUDIO_STREAM_VOICE_CALL &&
        audio_is_linear_pcm(config->format) &&
        !devices.onlyContainsDevicesWithType(AUDIO_DEVICE_OUT_TELEPHONY_TX)) {
        if ((config->channel_mask == 1 || config->channel_mask == 3) &&
                   (config->sample_rate == 8000 || config->sample_rate == 16000 ||
                    config->sample_rate == 32000 || config->sample_rate == 48000)) {
            //check if VoIP output is not opened already
            bool voip_pcm_already_in_use = false;
            for (size_t i = 0; i < mOutputs.size(); i++) {
                 sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(i);
                 if (desc->mFlags == (AUDIO_OUTPUT_FLAG_VOIP_RX | AUDIO_OUTPUT_FLAG_DIRECT)) {
                     //close voip output if currently open by the same client with different device
                     if ((desc->mDirectClientSession == session) &&
                         (desc->devices() != devices)) {
                         closeOutput(desc->mIoHandle);
                     } else {
                         voip_pcm_already_in_use = true;
                         ALOGD("VoIP PCM already in use .");
                     }
                     break;
                 }
            }

            if (!voip_pcm_already_in_use &&
                    String16("com.android.soundrecorder").compare(AudioPolicyManager::callingAppName)) {
                *flags = (audio_output_flags_t)(AUDIO_OUTPUT_FLAG_VOIP_RX |
                                               AUDIO_OUTPUT_FLAG_DIRECT);
                ALOGD("Set VoIP and Direct output flags for PCM format .");
            }
        }
    } /* voip flag override block end */

    // Super method calling
    superGetOutputForDevicesRet = AudioPolicyManager::getOutputForDevices(devices, session, attr, config, flags, isSpatialized, forceMutingHaptic);

    return superGetOutputForDevicesRet;
}

status_t MiAudioPolicyManager::startInput(audio_port_handle_t portId) {
    // Parent returned value of startInput
    status_t superStartInputRet;

    // 0b43f08 START
    sp<AudioInputDescriptor> inputDesc = mInputs.getInputForClient(portId);
    if (inputDesc == 0) {
        ALOGW("%s no input for client %d", __FUNCTION__, portId);
        return BAD_VALUE;
    }

    audio_io_handle_t input = inputDesc->mIoHandle;
    sp<RecordClientDescriptor> client = inputDesc->getClient(portId);

    String8 currClientName = String8(getPackageName(client->uid()));
    if (client->uid() != 1000) {
        String8 strParameter("appname=+");
        strParameter.append(currClientName);
        mpClientInterface->setParameters(input, strParameter);
    } else {
        mpClientInterface->setParameters(input, String8("appname=+system.app"));
    }

    /*Super method calling*/
    superStartInputRet = AudioPolicyManager::startInput(portId);

    if (superStartInputRet != NO_ERROR) {
        if (client->uid() != 1000) {
            String8 strParameter("appname=-");
            strParameter.append(currClientName);
            mpClientInterface->setParameters(input, strParameter);
        } else {
            mpClientInterface->setParameters(input, String8("appname=-system.app"));
        }
    }
    // 0b43f08 END
    return superStartInputRet;
}

status_t MiAudioPolicyManager::stopInput(audio_port_handle_t portId) {
    status_t status = NO_ERROR;

    status = AudioPolicyManager::stopInput(portId);

    if (status == NO_ERROR) {
        sp<AudioInputDescriptor> inputDesc = mInputs.getInputForClient(portId);
        audio_io_handle_t input = inputDesc->mIoHandle;
        sp<RecordClientDescriptor> client = inputDesc->getClient(portId);

        if (client && client->uid() != 1000) {
            String8 strParameter;
            strParameter += "appname=-";
            strParameter += client->getClientAppName();
            mpClientInterface->setParameters(input, strParameter);
        }

        if (client->uid() == 1000) {
            mpClientInterface->setParameters(input, String8("appname=-system.app"));
        }
    }

    return status;
}

MiAudioPolicyManager::MiAudioPolicyManager(AudioPolicyClientInterface *clientInterface) : AudioPolicyManager(clientInterface) {
    property_set("persist.audio.headset.plug.status", "off");  // 7c19778
}

}  // namespace android
