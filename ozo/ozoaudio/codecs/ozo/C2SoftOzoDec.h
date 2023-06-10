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

#ifndef ANDROID_C2_SOFT_OZO_DEC_H_
#define ANDROID_C2_SOFT_OZO_DEC_H_

#include <SimpleC2Component.h>

namespace android {

class OzoDecoderWrapper;
struct OzoDecoderConfigParams_s;
struct OzoDecoderStreamInfo_s;

struct C2SoftOzoDec : public SimpleC2Component {
public:
    class IntfImpl;

    C2SoftOzoDec(const char *name, c2_node_id_t id, const std::shared_ptr<IntfImpl> &intfImpl);
    virtual ~C2SoftOzoDec();

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
    struct OzoConfig {
        bool widening;

        size_t configSize;
        uint8_t configData[128];
    };

    status_t initDecoder(const uint8_t *csd, size_t size);
    void updateRuntimeParameters();

    std::shared_ptr<IntfImpl> mIntf;

    bool mSignalledError;
    OzoDecoderWrapper *mDecoder;
    struct OzoDecoderConfigParams_s *mParams;
    const struct OzoDecoderStreamInfo_s *mStreamInfo;
    struct OzoConfig mConfig;
    size_t mDelayedFrames;
    int64_t mTs[2];
    size_t mTsIndex;

    C2_DO_NOT_COPY(C2SoftOzoDec);
};

}  // namespace android

#endif  // ANDROID_C2_SOFT_OZO_DEC_H_
