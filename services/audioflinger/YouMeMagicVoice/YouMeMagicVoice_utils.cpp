#define LOG_TAG "YouMeMagicVoice_utils"
#define LOG_NDEBUG 0

#include "YouMeMagicVoice_utils.h"
#include <stdlib.h>
#include <cutils/log.h>
#include <pthread.h>
#include <dlfcn.h>
#include <string.h>

#define YOUMEMAGICVOICE_LIB32 "system/lib/libXiaoMiMagicVoice.so"
#define YOUMEMAGICVOICE_LIB "system/lib64/libXiaoMiMagicVoice.so"

typedef void* (*engine_createMagicVoiceEngine_t) ();
typedef void (*engine_releaseMagicVoiceEngine_t) (void* magicVoiceEngine);
typedef int (*engine_setMagicVoiceInfo_t) (const char *effectInfo);
typedef int (*engine_clearMagicVoiceInfo_t) ();
typedef int (*engine_setMagicVoiceAdjust_t) (double dFS, double dSemitones);

static pthread_mutex_t YouMeMagicVoice_mutex_lock = PTHREAD_MUTEX_INITIALIZER;

// Add for libYouMeMagicVoice
struct YouMeMagicVoice_handle {
    void *lib_handle;
    engine_createMagicVoiceEngine_t   engine_createMagicVoiceEngine;
    engine_releaseMagicVoiceEngine_t  engine_releaseMagicVoiceEngine;
    engine_setMagicVoiceInfo_t        engine_setMagicVoiceInfo;
    engine_clearMagicVoiceInfo_t      engine_clearMagicVoiceInfo;
    engine_setMagicVoiceAdjust_t      engine_setMagicVoiceAdjust;
};

struct YouMeMagicVoice_handle *g_YouMeMagicVoice_handle = NULL;

int get_handle() {
    ALOGD(" %s ", __func__);
    g_YouMeMagicVoice_handle->lib_handle = dlopen(YOUMEMAGICVOICE_LIB, RTLD_NOW);
    if (g_YouMeMagicVoice_handle->lib_handle == NULL) {
            ALOGE( "%s dlopen failed for %s", __func__, YOUMEMAGICVOICE_LIB);
            return -1;
    } else {
        // for engine function
        g_YouMeMagicVoice_handle->engine_createMagicVoiceEngine = (engine_createMagicVoiceEngine_t)dlsym(g_YouMeMagicVoice_handle->lib_handle, "YouMeMagicVoiceEngine_createMagicVoiceEngine");
        if (!g_YouMeMagicVoice_handle->engine_createMagicVoiceEngine) {
            ALOGE("%s: Could not find YouMeMagicVoiceEngine_createMagicVoiceEngine from %s", __func__, YOUMEMAGICVOICE_LIB);
            return -1;
        }

        g_YouMeMagicVoice_handle->engine_releaseMagicVoiceEngine = (engine_releaseMagicVoiceEngine_t)dlsym(g_YouMeMagicVoice_handle->lib_handle, "YouMeMagicVoiceEngine_releaseMagicVoiceEngine");
        if (!g_YouMeMagicVoice_handle->engine_releaseMagicVoiceEngine) {
            ALOGE("%s: Could not find YouMeMagicVoiceEngine_releaseMagicVoiceEngine from %s", __func__, YOUMEMAGICVOICE_LIB);
            return -1;
        }

        g_YouMeMagicVoice_handle->engine_setMagicVoiceInfo = (engine_setMagicVoiceInfo_t)dlsym(g_YouMeMagicVoice_handle->lib_handle, "YouMeMagicVoiceEngine_setMagicVoiceInfo");
        if (!g_YouMeMagicVoice_handle->engine_setMagicVoiceInfo) {
            ALOGE("%s: Could not find YouMeMagicVoiceEngine_setMagicVoiceInfo from %s", __func__, YOUMEMAGICVOICE_LIB);
            return -1;
        }

        g_YouMeMagicVoice_handle->engine_clearMagicVoiceInfo = (engine_clearMagicVoiceInfo_t)dlsym(g_YouMeMagicVoice_handle->lib_handle, "YouMeMagicVoiceEngine_clearMagicVoiceInfo");
        if (!g_YouMeMagicVoice_handle->engine_clearMagicVoiceInfo) {
            ALOGE("%s: Could not find YouMeMagicVoiceEngine_clearMagicVoiceInfo from %s", __func__, YOUMEMAGICVOICE_LIB);
            return -1;
        }

        g_YouMeMagicVoice_handle->engine_setMagicVoiceAdjust = (engine_setMagicVoiceAdjust_t)dlsym(g_YouMeMagicVoice_handle->lib_handle, "YouMeMagicVoiceEngine_setMagicVoiceAdjust");
        if (!g_YouMeMagicVoice_handle->engine_setMagicVoiceAdjust) {
            ALOGE("%s: Could not find YouMeMagicVoiceEngine_setMagicVoiceAdjust from %s", __func__, YOUMEMAGICVOICE_LIB);
            return -1;
        }
    }
    ALOGD("get YouMeMagicVoice handle");
    return 0;
}

int YouMeMagicVoice_init() {
    ALOGV(" %s ", __func__);
    int ret;
    pthread_mutex_lock(&YouMeMagicVoice_mutex_lock);
    if (g_YouMeMagicVoice_handle  != NULL) {
        ALOGD("YouMeMagicVoice already init");
         pthread_mutex_unlock(&YouMeMagicVoice_mutex_lock);
        return 0;
     }
    g_YouMeMagicVoice_handle = (YouMeMagicVoice_handle*) calloc(1, sizeof(struct YouMeMagicVoice_handle));
    if (!g_YouMeMagicVoice_handle) {
        ALOGD("MALLOC FAIL");
         pthread_mutex_unlock(&YouMeMagicVoice_mutex_lock);
        return -1;
    }
    ret =  get_handle();
    if(ret <0) {
        free(g_YouMeMagicVoice_handle);
        g_YouMeMagicVoice_handle = NULL;
    }
    pthread_mutex_unlock(&YouMeMagicVoice_mutex_lock);
    return ret;
}

void YouMeMagicVoice_deinit() {
    ALOGD(" %s ", __func__);
    pthread_mutex_lock(&YouMeMagicVoice_mutex_lock);
    if(g_YouMeMagicVoice_handle) {
        free(g_YouMeMagicVoice_handle);
    }
    g_YouMeMagicVoice_handle = NULL;
    pthread_mutex_unlock(&YouMeMagicVoice_mutex_lock);
}

/************************engine function define******************************/

void* engine_createMagicVoiceEngine() {
    ALOGV(" %s ", __func__);
    void* engine = NULL;
    if(g_YouMeMagicVoice_handle != NULL)
        engine = g_YouMeMagicVoice_handle->engine_createMagicVoiceEngine();
    return engine;
}

void engine_releaseMagicVoiceEngine(void* engine) {
    ALOGV(" %s ", __func__);
    if(g_YouMeMagicVoice_handle != NULL)
        g_YouMeMagicVoice_handle->engine_releaseMagicVoiceEngine(engine);
}

int engine_setMagicVoiceInfo(const char *effectInfo) {
    ALOGV(" %s ", __func__);
    int ret = -1;
    if(g_YouMeMagicVoice_handle != NULL)
        ret = g_YouMeMagicVoice_handle->engine_setMagicVoiceInfo(effectInfo);
    return ret;
}

int engine_clearMagicVoiceInfo() {
    ALOGV(" %s ", __func__);
    int ret = -1;
    if(g_YouMeMagicVoice_handle != NULL)
        ret = g_YouMeMagicVoice_handle->engine_clearMagicVoiceInfo();
    return ret;
}

int engine_setMagicVoiceAdjust(double dFS, double dSemitones) {
    ALOGV(" %s ", __func__);
    int ret = -1;
    if(g_YouMeMagicVoice_handle != NULL)
        ret = g_YouMeMagicVoice_handle->engine_setMagicVoiceAdjust(dFS, dSemitones);
    return ret;
}
// Add for libYouMeMagicVoice end
