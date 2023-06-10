#ifndef ANDROID_AUDIOPOLICY_IMPL_H
#define ANDROID_AUDIOPOLICY_IMPL_H

#include <log/log.h>
#include <utils/Errors.h>
#include <IAudioPolicyServiceStub.h>

namespace android {

using namespace effectsConfig;

class AudioPolicyServiceImpl : public IAudioPolicyServiceStub {
public:
    virtual ~AudioPolicyServiceImpl() {}
    virtual status_t loadGlobalEffects(const sp<AudioPolicyEffects>& ape,
        cnode *root, const Vector <AudioPolicyEffects::EffectDesc *>& effects);
    virtual void createGlobalEffects(const sp<AudioPolicyEffects>& policyEffects);
    virtual void loadGlobalAudioEffectXmlConfig(
        Vector< AudioPolicyEffects::EffectDesc > &descs,
        struct ParsingResult &result);
    virtual bool isAllowedAPP(String16 ClientName);

private:
    // MIUI ADD: DOLBY_ENABLE, may be need a more suitable place to add.
    void dolbySetupDaxEffect(sp<AudioEffect> fx);
    int  getBoolParam(sp<AudioEffect> fx, int effEvt);
    int  getParameter(sp<AudioEffect> fx, void* pdata, int psize, void* pvalues, int vsize);

    // This must be consists with DolbyAudioEffect.java
    static const int32_t BASIC_BUF_SIZE = 3;
    static const int32_t BYTES_PER_INT = 4;
    static const int32_t EFFECT_PARAM_CPDP_VALUES = 5; // ds_config.h
    static const int32_t EFFECT_PARAM_EFFECT_ENABLE = 0x00000000; // DsParams.java
    // MIUI END
};

extern "C"  IAudioPolicyServiceStub* create();
extern "C" void destroy(IAudioPolicyServiceStub* impl);

}//namespace android

#endif
