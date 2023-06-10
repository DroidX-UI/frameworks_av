#pragma once

#include <audiopolicy/managerdefault/AudioPolicyManager.h>
#include <Volume.h>

namespace android {

class MiAudioPolicyManager: public AudioPolicyManager
{

public:
        MiAudioPolicyManager(AudioPolicyClientInterface *clientInterface);

        virtual ~MiAudioPolicyManager() {}

        virtual status_t startInput(audio_port_handle_t portId) override;

        virtual status_t stopInput(audio_port_handle_t portId) override;

protected:
        virtual status_t setDeviceConnectionStateInt(const sp<DeviceDescriptor> &device,
                                             audio_policy_dev_state_t state) override;

        String16 getPackageName(uid_t uid);

        virtual void checkOutputForAttributes(const audio_attributes_t &attr) override;

        virtual status_t startSource(const sp<SwAudioOutputDescriptor>& outputDesc,
                             const sp<TrackClientDescriptor>& client,
                             uint32_t *delayMs) override;
private:
        // internal method to return the output handle for the given device and format
        virtual audio_io_handle_t getOutputForDevices(
                const DeviceVector &devices,
                audio_session_t session,
                const audio_attributes_t *attr,
                const audio_config_t *config,
                audio_output_flags_t *flags,
                bool *isSpatialized,
                bool forceMutingHaptic = false) override;

        virtual status_t getOutputForAttr(const audio_attributes_t *attr,
                audio_io_handle_t *output,
                audio_session_t session,
                audio_stream_type_t *stream,
                const AttributionSourceState& attributionSource,
                const audio_config_t *config,
                audio_output_flags_t *flags,
                audio_port_handle_t *selectedDeviceId,
                audio_port_handle_t *portId,
                std::vector<audio_io_handle_t> *secondaryOutputs,
                output_type_t *outputType,
                bool *isSpatialized) override;

        uid_t mCallingAppuid;
        // 2bf9682 START
        String8 mSpatialAudioAddress;
        // 2bf9682 END
};

}; // namespace android
