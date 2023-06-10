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

#ifndef ANDROID_C2_SOFT_OZO_ENC_H_
#define ANDROID_C2_SOFT_OZO_ENC_H_

#include <SimpleC2Component.h>

namespace android {

class OzoEncoderWrapper;
struct OzoEncoderParams_s;
class OzoParameterUpdater;

class C2SoftOzoEnc : public SimpleC2Component {
public:
    class IntfImpl;

    C2SoftOzoEnc(const char *name, c2_node_id_t id, const std::shared_ptr<IntfImpl> &intfImpl);
    virtual ~C2SoftOzoEnc();

    // From SimpleC2Component
    c2_status_t onInit() override;
    c2_status_t onStop() override;
    void onReset() override;
    void onRelease() override;
    c2_status_t onFlush_sm() override;
    void process(
            const std::unique_ptr<C2Work> &work,
            const std::shared_ptr<C2BlockPool> &pool) override;
    c2_status_t drain(
            uint32_t drainMode,
            const std::shared_ptr<C2BlockPool> &pool) override;

private:
    void updateRuntimeParameters();

    std::shared_ptr<IntfImpl> mIntf;

    size_t mNumBytesPerInputFrame;
    size_t mOutBufferSize;

    bool mSentCodecSpecificData;
    size_t mInputSize;
    uint8_t *mInputFrame;
    c2_cntr64_t mInputTimeUs;

    bool mSignalledError;

    bool mOzoInitialized;
    size_t mDelayedFrames;
    c2_cntr64_t mDelayTimeUs;
    size_t mBytesPerSample;
    OzoEncoderWrapper *mEncoder;
    struct OzoEncoderParams_s *mParams;
    OzoParameterUpdater *mParamUpdater;
    size_t mFrames;

    status_t initEncoder();
    void destroy();

    C2_DO_NOT_COPY(C2SoftOzoEnc);
};

}  // namespace android

#endif  // ANDROID_C2_SOFT_OZO_ENC_H_
