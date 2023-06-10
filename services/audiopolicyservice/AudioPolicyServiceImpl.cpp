#define LOG_TAG "AudioPolicyImpl"
#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <utils/Trace.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <mediautils/FeatureManager.h>

#include "AudioPolicyServiceImpl.h"
#include <android/media/permission/Identity.h>

namespace android {

constexpr const char* GLOBAL_EFFECTS[] = {"misoundfx","dap"};

using content::AttributionSourceState;
using namespace effectsConfig;

/*global audio effect*/
void AudioPolicyServiceImpl::createGlobalEffects(
        const sp<AudioPolicyEffects>& ape) {
    // create audio global processors if not yet
    if (android_atomic_cmpxchg(0, 1, &(ape->mGlobalEffectsInitialized)) == 0) {
        for (size_t i = 0; i < ape->mGlobalEffectDescs.size(); i++) {
            const AudioPolicyEffects::EffectDesc &effect =
                ape->mGlobalEffectDescs[i];
            int64_t token = IPCThreadState::self()->clearCallingIdentity();
            AttributionSourceState attributionSource;
            attributionSource.packageName = "android";
            attributionSource.token = sp<BBinder>::make();
            sp<AudioEffect> fx = new AudioEffect(attributionSource);
            fx->set(NULL, &effect.mUuid, 0, 0, 0, AUDIO_SESSION_NONE,
                AUDIO_IO_HANDLE_NONE);
            IPCThreadState::self()->restoreCallingIdentity(token);
            status_t status = fx->initCheck();
            if (status != NO_ERROR && status != ALREADY_EXISTS) {
                ALOGW("Failed to create Fx %s", effect.mName);
                //fx goes out of scope and strong ref on AudioEffect is released
                continue;
            }
            for (size_t j = 0; j < effect.mParams.size(); j++) {
                fx->setParameter(effect.mParams[j]);
            }

            // MIUI ADD: DOLBY_ENABLE
            if (FeatureManager::isFeatureEnable(AUDIO_DOLBY_ENABLE)) {
                if(strcmp(effect.mName, "dap") == 0){
                    dolbySetupDaxEffect(fx);
                }
            }
            // MIUI END

            ALOGI("createGlobalEffects() adding effect %s uuid %08x",
                effect.mName, effect.mUuid.timeLow);
            ape->mGlobalEffects.add(fx);
        }
    }
}

void AudioPolicyServiceImpl::loadGlobalAudioEffectXmlConfig(
        Vector< AudioPolicyEffects::EffectDesc > &descs,
        struct ParsingResult &result) {

    auto loadGlobalEffectChain = [](auto& effects, auto& descs) {
        for (const char*  globalEffect : GLOBAL_EFFECTS) {
            for (auto& effect : effects) {
                ALOGD("loadGlobalEffects() effect:%s globalEffects:%s",
                    effect.name.c_str(), globalEffect);
                int cmp = strncmp(effect.name.c_str(), globalEffect,
                    EFFECT_STRING_LEN_MAX);
                if (cmp == 0) {
                    AudioPolicyEffects::EffectDesc effectDescs(
                        effect.name.c_str(), effect.uuid);
                    ALOGD("loadGlobalEffects() adding effect %s uuid %08x",
                        effect.name.c_str(), effect.uuid.timeLow);
                    descs.add(effectDescs);
                }
            }
        }
    };
    loadGlobalEffectChain( result.parsedConfig->effects, descs);

}

status_t AudioPolicyServiceImpl::loadGlobalEffects(
        const sp<AudioPolicyEffects>& ape, cnode *root,
        const Vector <AudioPolicyEffects::EffectDesc *>& effects) {
    cnode *node = config_find(root, GLOBALPROCESSING_TAG);
    if (node == NULL) {
        return -ENOENT;
    }
    node = node->first_child;
    if (ape == nullptr) {
        return -ENOENT;
    }
    while (node) {
        size_t i;
        for (i = 0; i < effects.size(); i++) {
            int cmp = strncmp(effects[i]->mName, node->name,
                EFFECT_STRING_LEN_MAX);
            if (cmp == 0) {
                ALOGD("loadGlobalEffects() found effect %s in list",
                    node->name);
                break;
            }
        }
        if (i == effects.size()) {
            ALOGV("loadGlobalEffects() effect %s not in list", node->name);
            node = node->next;
            continue;
        }
        AudioPolicyEffects::EffectDesc effect(*effects[i]); // deep copy
        ape->loadEffectParameters(node, effect.mParams);
        ALOGD("loadGlobalEffects() adding effect %s uuid %08x", effect.mName,
            effect.mUuid.timeLow);
        ape->mGlobalEffectDescs.add(effect);
        node = node->next;
    }
    return NO_ERROR;
}

void AudioPolicyServiceImpl::dolbySetupDaxEffect(sp<AudioEffect> fx){
    if (FeatureManager::isFeatureEnable(AUDIO_DOLBY_ENABLE)) {
        int status = getBoolParam(fx, EFFECT_PARAM_EFFECT_ENABLE);
        if (status != -1) {
            fx->setEnabled(status > 0?1:0);
        }

        ALOGD("%s(): set dolby enable: %d, status = %d", __func__, status > 0?1:0, status == -1?-1:0);
    }
}

int AudioPolicyServiceImpl::getBoolParam(const sp<AudioEffect> fx, int effEvt) {
    int status = NO_ERROR;
    int psize = sizeof(int32_t);
    int vsize = BASIC_BUF_SIZE * BYTES_PER_INT;
    char *baValue = new char[BASIC_BUF_SIZE * BYTES_PER_INT];
    int32_t param = EFFECT_PARAM_CPDP_VALUES + effEvt;
    
    status = getParameter(fx, &param, psize, baValue, vsize);
    int value = -1;
    if (status == NO_ERROR) {
        value = ((int*)baValue)[0] > 0 ? 1:0;
    }

    delete[] baValue;
    return value;
}

int AudioPolicyServiceImpl::getParameter(const sp<AudioEffect> fx, void* pdata, int psize, void* pvalues, int vsize) {
    int status = NO_ERROR;
    if (pdata == NULL || psize == 0 || pvalues == NULL || vsize == 0) {
        return -4; // AUDIOEFFECT_ERROR_BAD_VALUE
    }

    int voffset = ((psize - 1) / sizeof(int) + 1) * sizeof(int);
    effect_param_t *p = (effect_param_t *) malloc(sizeof(effect_param_t) + voffset + vsize);
    memcpy(p->data, pdata, psize);
    p->psize = psize;
    p->vsize = vsize;

    status = fx->getParameter(p);
    if (status == NO_ERROR) {
        status = p->status;
        if (status == NO_ERROR) {
            memcpy(pvalues, p->data + voffset, p->vsize);
        }
    }

    free(p);
    return status;
}

bool AudioPolicyServiceImpl::isAllowedAPP(String16 ClientName) {
    return !String16("com.miui.vpnsdkmanager").compare(ClientName) ||
           !String16("com.miui.accessibility").compare(ClientName);
}

extern "C"  IAudioPolicyServiceStub*  create() {
    return new AudioPolicyServiceImpl();
}

extern "C" void destroy(IAudioPolicyServiceStub* impl) {
    delete impl;
}

}//namespace android
