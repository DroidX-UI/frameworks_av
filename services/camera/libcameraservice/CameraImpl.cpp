#define LOG_TAG "CameraImpl"
#define ATRACE_TAG ATRACE_TAG_CAMERA
//#define LOG_NDEBUG 0

#include <log/log.h>
#include <cutils/properties.h>
#include <utils/String16.h>
#include <binder/AppOpsManager.h>
#include <binder/Parcel.h>
#include <utils/Trace.h>
#include <binder/IServiceManager.h>
#include "system/camera_metadata.h"
#include "camera_metadata_hidden.h"
#include <camera/VendorTagDescriptor.h>
#include <system/graphics-base.h>
#include <pthread.h>

#include "CameraImpl.h"

#define HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS_UBWC 0x7FA30C06
#define HAL_PIXEL_FORMAT_YCbCr_420_TP10_UBWC 0x7FA30C09

#ifdef __POPUP_CAMERA__
#include "popupcamera/PopupCameraManager.h"
#endif

using namespace android;
using namespace android::camera3;

#if defined(MIUI_VERSION)

struct SdkTag{
    union
    {
        const char *name;
        uint32_t id;
    } tag;
    bool isUsed;
    SdkTag(const char *n) : isUsed{false} { tag.name = n; }
};

static std::pair<std::string, SdkTag> sdkAlgoAndTag[] = {
    {"hdr", "com.xiaomi.algo.hdrMode"},
    {"SkinSmooth", "com.xiaomi.algo.beautySkinSmoothRatio"},
    {"SlimFace", "com.xiaomi.algo.beautySlimFaceRatio"},
    {"bokeh", "com.xiaomi.algo.mibokehEnable"},
    {"mfnr", "com.xiaomi.algo.mfnrEnable"}
};
#endif

// Values from frameworks/base/services/core/java/com/android/server/am/ProcessList.java
const int32_t INVALID_ADJ = -10000;
const int32_t UNKNOWN_ADJ = 1001;
const int32_t CACHED_APP_MAX_ADJ = 999;
const int32_t CACHED_APP_MIN_ADJ = 900;
const int32_t CACHED_APP_LMK_FIRST_ADJ = 950;
const int32_t CACHED_APP_IMPORTANCE_LEVELS = 5;
const int32_t SERVICE_B_ADJ = 800;
const int32_t PREVIOUS_APP_ADJ = 700;
const int32_t HOME_APP_ADJ = 600;
const int32_t SERVICE_ADJ = 500;
const int32_t HEAVY_WEIGHT_APP_ADJ = 400;
const int32_t BACKUP_APP_ADJ = 300;
const int32_t PERCEPTIBLE_LOW_APP_ADJ = 250;
const int32_t PERCEPTIBLE_MEDIUM_APP_ADJ = 225;
const int32_t PERCEPTIBLE_APP_ADJ = 200;
const int32_t VISIBLE_APP_ADJ = 100;
const int32_t VISIBLE_APP_LAYER_MAX = PERCEPTIBLE_APP_ADJ - VISIBLE_APP_ADJ - 1;
const int32_t PERCEPTIBLE_RECENT_FOREGROUND_APP_ADJ = 50;
const int32_t FOREGROUND_APP_ADJ = 0;
const int32_t PERSISTENT_SERVICE_ADJ = -700;
const int32_t PERSISTENT_PROC_ADJ = -800;
const int32_t SYSTEM_ADJ = -900;
const int32_t NATIVE_ADJ = -1000;
// Values from /frameworks/native/libs/binder/include/binder/ActivityManager.h
const int32_t PROCESS_STATE_UNKNOWN = -1;
const int32_t PROCESS_STATE_PERSISTENT = 0;
const int32_t PROCESS_STATE_PERSISTENT_UI = 1;
const int32_t PROCESS_STATE_TOP = 2;
const int32_t PROCESS_STATE_FOREGROUND_SERVICE_LOCATION = 3;
const int32_t PROCESS_STATE_BOUND_TOP = 4;
const int32_t PROCESS_STATE_FOREGROUND_SERVICE = 5;
const int32_t PROCESS_STATE_BOUND_FOREGROUND_SERVICE = 6;
const int32_t PROCESS_STATE_IMPORTANT_FOREGROUND = 7;
const int32_t PROCESS_STATE_IMPORTANT_BACKGROUND = 8;
const int32_t PROCESS_STATE_TRANSIENT_BACKGROUND = 9;
const int32_t PROCESS_STATE_BACKUP = 10;
const int32_t PROCESS_STATE_SERVICE = 11;
const int32_t PROCESS_STATE_RECEIVER = 12;
const int32_t PROCESS_STATE_TOP_SLEEPING = 13;
const int32_t PROCESS_STATE_HEAVY_WEIGHT = 14;
const int32_t PROCESS_STATE_HOME = 15;
const int32_t PROCESS_STATE_LAST_ACTIVITY = 16;
const int32_t PROCESS_STATE_CACHED_ACTIVITY = 17;
const int32_t PROCESS_STATE_CACHED_ACTIVITY_CLIENT = 18;
const int32_t PROCESS_STATE_CACHED_RECENT = 19;
const int32_t PROCESS_STATE_CACHED_EMPTY = 20;
const int32_t PROCESS_STATE_NONEXISTENT = 21;

CameraImpl::CameraImpl() {
    ALOGD("%s:%d ", __FUNCTION__, __LINE__);
    char value[PROPERTY_VALUE_MAX] = {0};
    mApiVersion        = -1;
    mTargetSdkVersion  = 0;
}

CameraImpl::~CameraImpl() {
    ALOGD("%s:%d ", __FUNCTION__, __LINE__);
}

void CameraImpl::setClientPackageName(const String16& clientName, int32_t api_num) {
    ALOGD("%s:%d %s api%d", __FUNCTION__, __LINE__, String8(clientName).string(), api_num);
    mClientPackageName = clientName;
    mApiVersion = api_num;
}

void CameraImpl::updateSessionParams(CameraMetadata& sessionParams) {
    ALOGV("%s:%d ", __FUNCTION__, __LINE__);
    /*if (mApiVersion <= 1) {
        ALOGV("%s %d mApiVersion %d", __FUNCTION__, __LINE__, mApiVersion);
        return;
    }*/
    sp<VendorTagDescriptor> vTags =
        VendorTagDescriptor::getGlobalVendorTagDescriptor();
    if ((nullptr == vTags.get() || 0 >= vTags->getTagCount())) {
        sp<VendorTagDescriptorCache> cache =
        VendorTagDescriptorCache::getGlobalVendorTagCache();
        if (cache.get()) {
            const camera_metadata_t *metaBuffer = sessionParams.getAndLock();
            metadata_vendor_id_t vendorId = get_camera_metadata_vendor_id(metaBuffer);
            sessionParams.unlock(metaBuffer);
            cache->getVendorTagDescriptor(vendorId, &vTags);
        }
    }

    char tagName[]="com.xiaomi.sessionparams.clientName";
    uint32_t tag = 0;
    status_t result = sessionParams.getTagFromName(tagName, vTags.get(), &tag);
    if (result == OK) {
        result = sessionParams.update(tag, String8(mClientPackageName));
        if (result == OK) {
            ALOGD("%s %d sucessfully update %s",
                __FUNCTION__, __LINE__, String8(mClientPackageName).string());
        } else {
            ALOGE("%s %d failed to update app name %s , error= %d",
                __FUNCTION__, __LINE__, String8(mClientPackageName).string(), result);
        }
    } else {
        ALOGE("%s %d failed to find tag", __FUNCTION__, __LINE__);
    }

#if defined(MIUI_VERSION)
    {
        char tagName[]="com.xiaomi.sessionparams.operation";
        status_t result = sessionParams.getTagFromName(tagName, vTags.get(), &tag);
        camera_metadata_entry_t e;
        if (result == OK) {
            e = sessionParams.find(tag);
            if(e.count && e.data.i32[0]) {
                mCameraSdkData.setSessionOp(e.data.i32[0]);
                // FIXME: if current client name is long, this string will be a lot of space in its whole life
                mCameraSdkData.setClientName(String8(mClientPackageName).string());
            } else {
                ALOGE("%s %d failed to find sessionparams.operation", __FUNCTION__, __LINE__);
                mCameraSdkData.setSessionOp(0);
                return;
            }
        } else {
            ALOGE("%s %d failed to find sessionparams.operation", __FUNCTION__, __LINE__);
            mCameraSdkData.setSessionOp(0);
            return;
        }
    }

    int length = sizeof(sdkAlgoAndTag)/sizeof(sdkAlgoAndTag[0]);
    for(int i=0;i<length;i++) {
        SdkTag &sdkTag = sdkAlgoAndTag[i].second;
        if(sdkTag.isUsed == false) {
            uint32_t id;
            if(OK == sessionParams.getTagFromName(sdkTag.tag.name, vTags.get(), &id)) {
                sdkTag.tag.id = id;
                sdkTag.isUsed = true;
            }
        }
    }
#endif
}

void CameraImpl::reportSdkConfigureStreams(const String8& cameraId,
        camera_stream_configuration *config, const CameraMetadata &sessionParams)
{
#if defined(MIUI_VERSION)
    if(!mCameraSdkData.isSdkApp())
    {
        ALOGE("%s %d %p", __FUNCTION__, __LINE__,&sessionParams);
        return;
    }
    mCameraSdkData.clear();

    uint32_t targetFormat = 0;
    auto streams = config->streams;
    for(uint32_t i=0;i<config->num_streams;i++) {
        if(HAL_PIXEL_FORMAT_YCBCR_420_888 == streams[i]->format ||
            HAL_PIXEL_FORMAT_BLOB == streams[i]->format) {
            targetFormat = streams[i]->format;
            break;
        }
    }

    mCameraSdkData.mReport.reportConfigureStream(mCameraSdkData.getClientName(),
                                                 cameraId.string(), mCameraSdkData.getSessionOp(), targetFormat);
#else
    ALOGV("it is not MIUI VERSION,will not take effect,%p %p %p", &cameraId, config, &sessionParams);
#endif
}

void CameraImpl::reportSdkRequest(camera_capture_request_t* request)
{
#if defined(MIUI_VERSION)
    if(!mCameraSdkData.isSdkApp())
    {
        ALOGE("%s %d %p", __FUNCTION__, __LINE__,request);
        return;
    }

    const camera_metadata_t *meta = request->settings;
    if(meta == nullptr || mCameraSdkData.isVideo())
        return;
    bool isSnapshot=false;

    for(uint32_t i=0;i<request->num_output_buffers;i++) {
        if(static_cast<int>(request->output_buffers[i].stream->format) == 35 ||
            static_cast<int>(request->output_buffers[i].stream->format) == 33){
            isSnapshot = true;
            break;
        }
    }

    if(!isSnapshot)
        return;

    std::string functionList = "";

    int length = sizeof(sdkAlgoAndTag)/sizeof(sdkAlgoAndTag[0]);
    for(int i=0;i<length;i++) {
        SdkTag &sdkTag = sdkAlgoAndTag[i].second;
        if(sdkTag.isUsed == false) {
            ALOGI("cant find sdk tag :%s,check it",sdkTag.tag.name);
            continue;
        }

        camera_metadata_ro_entry_t entry{};
        find_camera_metadata_ro_entry(meta, sdkTag.tag.id, &entry);
        if(entry.count) {
            if(entry.data.i32[0] > 0) {
                functionList+=sdkAlgoAndTag[i].first;
                functionList+="-";
            }
        }
    }
    if(functionList.size() > 0)
        functionList.erase(functionList.size() - 1);

    if(!isSnapshot) {
        if(mCameraSdkData.mLastPreviewFun == functionList){
            return;
        } else {
            mCameraSdkData.mLastPreviewFun = functionList;
        }
    }

    mCameraSdkData.mReport.reportRequest(mCameraSdkData.getClientName(),
                                         functionList, mCameraSdkData.getSessionOp());
#else
    ALOGV("it is not MIUI VERSION,will not take effect,%p", request);
#endif
}

void CameraImpl::adjCameraPriority(const String8& packageName,
    std::vector<int> &priorityScores, std::vector<int> &states,
    std::vector<std::pair< std::string, int>> &clientList) {

    /* Set systemui process as lower priorityscore to prevent other Apps from connect camera
     * in face unlock case from keyguard
     */
    for (auto client : clientList) {
        if (!client.first.compare(packageName) && strcmp(packageName.string(), "org.codeaurora.ims")) {
            ALOGD("%s %d pk:%s pr:%d, st:%d", __FUNCTION__, __LINE__, packageName.string(),
               priorityScores[priorityScores.size() - 1], states[states.size() - 1]);
            priorityScores[priorityScores.size() - 1] = 1001;// UNKNOWN_ADJ
            states[states.size() - 1] = 6; // PROCESS_STATE_IMPORTANT_FOREGROUND
            break;
        }
    }

    for (auto client : clientList) {
        if (client.second >= 0) {
            priorityScores[client.second] = 1001;// UNKNOWN_ADJ
            states[client.second] = 6; // PROCESS_STATE_IMPORTANT_FOREGROUND
        }
        ALOGV("%s %d client:%s-idx:%d", __FUNCTION__, __LINE__,
            client.first.c_str(), client.second);
    }
}

bool CameraImpl::isAonCameraId(const String8& cameraId){
    //AON uses camera 9 on the L1-T platform. On platform 8550, AON uses camera 8.
    //Since L1-T and 8550 share device warehouse, two properties are added
    //for 8550
    char value1[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.vendor.camera.aon.cameraId", value1, "-1");
    //for 8475
    char value2[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.vendor.camera.aon8475.cameraId", value2, "-1");
    return !cameraId.compare(String8(value1)) || !cameraId.compare(String8(value2));
}

void CameraImpl::adjNativeCameraPriority(const String8& key, int32_t& score_adj, int32_t& state_adj){
    if (isAonCameraId(key)) {
        score_adj = UNKNOWN_ADJ;
        state_adj = PROCESS_STATE_SERVICE;
     }

    if (!key.compare(String8("1"))) {
        score_adj = UNKNOWN_ADJ;
        state_adj = PROCESS_STATE_BOUND_FOREGROUND_SERVICE;
     }
}

bool CameraImpl::isFaceUnlockPackage(const String8& cameraId, bool systemNativeClient)
{
    bool ret = !cameraId.compare(String8("1")) && systemNativeClient;

    ALOGV("[%s],cameraId:%s,systemNativeClient:%d, isFaceUnlockPackage:%d",
          __FUNCTION__, cameraId.c_str(),systemNativeClient, ret);
    return ret;
}

void CameraImpl::adjSysCameraPriority(const String8& packageName,
     std::vector<int> &priorityScores, std::vector<int> &states) {
    /*Set system camera process as the highest priorityscore to avoid be evicted
     * by existing client when start system camera
     */
    ALOGD("%s:%d %zu %zu", __FUNCTION__, __LINE__, priorityScores.size(), states.size());
    String8 systemCameraPackageName("com.android.camera");
    if (!systemCameraPackageName.compare(packageName)) {
        priorityScores[priorityScores.size() - 1] = -900; // SYSTEM_ADJ
        states[states.size() - 1] = 0; // PROCESS_STATE_PERSISTENT
    }
}

status_t CameraImpl::getTagFromName(const CameraMetadata& info,
        char* tagName, uint32_t &tag) {
    sp<VendorTagDescriptor> vTags =
        VendorTagDescriptor::getGlobalVendorTagDescriptor();
    if ((nullptr == vTags.get()) || (0 >= vTags->getTagCount())) {
        sp<VendorTagDescriptorCache> cache =
            VendorTagDescriptorCache::getGlobalVendorTagCache();
        if (cache.get()) {
            const camera_metadata_t *metaBuffer = info.getAndLock();
            metadata_vendor_id_t vendorId = get_camera_metadata_vendor_id(metaBuffer);
            info.unlock(metaBuffer);
            cache->getVendorTagDescriptor(vendorId, &vTags);
        }
    }

    status_t res = info.getTagFromName(tagName, vTags.get(), &tag);
    return res;
}

void CameraImpl::getCustomBestSize(const CameraMetadata& info, int32_t width,
       int32_t height, int32_t format, int32_t &bestWidth, int32_t &bestHeight) {
    String16 systemCameraPackageName("com.android.camera");
    String16 itsCameraPackageName("com.xiaomi.module.evaluation");
    if (!systemCameraPackageName.compare(mClientPackageName)
        || !itsCameraPackageName.compare(mClientPackageName)) {
        uint32_t tag = 0;
        char tagName[] = "xiaomi.scaler.availableStreamConfigurations";
        status_t res = getTagFromName(info, tagName, tag);
        if (res == OK) {
            camera_metadata_ro_entry xiaomiStreamConfigs = info.find(tag);
            for (size_t i = 0; i < xiaomiStreamConfigs.count; i += 4) {
                int32_t fmt = xiaomiStreamConfigs.data.i32[i];
                int32_t w = xiaomiStreamConfigs.data.i32[i + 1];
                int32_t h = xiaomiStreamConfigs.data.i32[i + 2];
                // Ignore input/output type for now
                if (fmt == format && w == width && h == height) {
                    bestWidth = width;
                    bestHeight = height;
                    break;
                }
            }
        }
        ALOGV("%s:%d bestWidth %d bestHeight %d",
            __FUNCTION__, __LINE__, bestWidth, bestHeight);
    }
}

void CameraImpl::createCustomDefaultRequest(CameraMetadata& metadata) {
    ALOGV("%s %d", __FUNCTION__, __LINE__);
    //Force Enable FD for 3rd app use api2 except CTS
    camera_metadata_entry_t entry = metadata.find(ANDROID_STATISTICS_FACE_DETECT_MODE);
    for(size_t i = 0; i < entry.count; ++i) {
        uint8_t defaultFDMode = entry.data.u8[i];
        if ((defaultFDMode == ANDROID_STATISTICS_FACE_DETECT_MODE_OFF) &&
            (strcmp(String8(mClientPackageName).string(), "android.camera.cts"))) {
            uint8_t forceFDMode = ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE;
            metadata.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &forceFDMode, 1);
            ALOGV("Modify FD Mode from %u to %u for app %s",
               defaultFDMode, forceFDMode, String8(mClientPackageName).string());
        }
    }

    sp<VendorTagDescriptor> vTags =
           VendorTagDescriptor::getGlobalVendorTagDescriptor();
    if ((nullptr == vTags.get() || 0 >= vTags->getTagCount())) {
        sp<VendorTagDescriptorCache> cache =
            VendorTagDescriptorCache::getGlobalVendorTagCache();
        if (cache.get()) {
            const camera_metadata_t *metaBuffer = metadata.getAndLock();
            metadata_vendor_id_t vendorId = get_camera_metadata_vendor_id(metaBuffer);
            metadata.unlock(metaBuffer);
            cache->getVendorTagDescriptor(vendorId, &vTags);
        }
    }

    char tagName[]="com.xiaomi.sessionparams.clientName";
    uint32_t tag;
    status_t result = metadata.getTagFromName(tagName, vTags.get(), &tag);
    if (result == OK) {
        result = metadata.update(tag, String8(mClientPackageName));
        if (result == OK) {
            ALOGV("%s %d update %s",
                  __FUNCTION__, __LINE__, String8(mClientPackageName).string());
        } else {
            ALOGE("%s %d %s no update, error= %d",
                __FUNCTION__, __LINE__, String8(mClientPackageName).string(), result);
        }
    } else {
        ALOGV("%s %d tag no found", __FUNCTION__, __LINE__);
    }
}

bool CameraImpl::checkSensorFloder(CameraMetadata *CameraCharacteristics, CameraMetadata *gCameraCharacteristics, pthread_rwlock_t *rwLock)
{
    bool ret = false;
    char prop[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.build.product", prop, "default");
    ALOGD("%s:ro.build.product.prop:%s ",__FUNCTION__,prop);
    if (strcmp(prop, "cetus") == 0) {
        camera_metadata_entry facing = CameraCharacteristics->find(ANDROID_LENS_FACING);
        if ((facing.count == 1) && (facing.data.u8[0] == ANDROID_LENS_FACING_FRONT)) {
            if (pthread_rwlock_init(rwLock, nullptr) == 0) {
                if (mSensorFloder.Init(CameraCharacteristics, gCameraCharacteristics ,rwLock) == true) {
                     ret = true;
                      ALOGD("%s:successfully entered mSensorFloder.Init(),ret:%d",__FUNCTION__,ret);
                }
            }
        }
     }

     return ret;
}

String16 CameraImpl::getStringProperty(const char* key, const char* defaultValue)
{
    char value[PROPERTY_VALUE_MAX];
    property_get(key, value, defaultValue);
    return String16(value);
}

status_t CameraImpl::repeatAddFakeTriggerIds(status_t res, CameraMetadata &metadata)
{
    int index = 0;
    status_t ret = OK;
    while (res != OK) {
        usleep(1 * 1000);
        // Trigger ID 0 had special meaning in the HAL2 spec, so avoid it here
        static const int32_t fakeTriggerId = 1;
        // If AF trigger is active, insert a fake AF trigger ID if none already
        // exists
        camera_metadata_entry afTrigger = metadata.find(ANDROID_CONTROL_AF_TRIGGER);
        camera_metadata_entry afId = metadata.find(ANDROID_CONTROL_AF_TRIGGER_ID);
        if (afTrigger.count > 0 &&
                afTrigger.data.u8[0] != ANDROID_CONTROL_AF_TRIGGER_IDLE &&
                afId.count == 0) {
            ret = metadata.update(ANDROID_CONTROL_AF_TRIGGER_ID, &fakeTriggerId, 1);
            if (ret != OK)
                return ret;
        }

        // If AE precapture trigger is active, insert a fake precapture trigger ID
        // if none already exists
        camera_metadata_entry pcTrigger =
                metadata.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
        camera_metadata_entry pcId = metadata.find(ANDROID_CONTROL_AE_PRECAPTURE_ID);
        if (pcTrigger.count > 0 &&
                pcTrigger.data.u8[0] != ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE &&
                pcId.count == 0) {
            ret = metadata.update(ANDROID_CONTROL_AE_PRECAPTURE_ID,
                    &fakeTriggerId, 1);
            if (ret != OK)
                return ret;
        }
        index++;
        if (index > 20) {
            break;
        }
    }
    return ret;
}

void CameraImpl::checkAllClientsStoppedAndReclaimMem(int32_t *cameraProviderPid, bool *memDirty)
{
    (void)cameraProviderPid;

    if (*memDirty) {
        property_set("persist.sys.camera.cameraservice.uid_idle", "1");
        *memDirty = false;
    }
    ALOGI("%s reclaim camera provider anon memory.", __FUNCTION__);

}

void CameraImpl::notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName)
{
    (void) cameraId;
    (void) status;
    (void) clientPackageName;

    #ifdef __POPUP_CAMERA__
    ALOGI("PopupCameraManager (\"%s\", camera ID %d) status %d",
        String8(clientPackageName).string(), cameraId, status);
    PopupCameraManager::getInstance().notifyCameraStatus(cameraId, status, clientPackageName);

    if ((1 == status) && (1 == cameraId)) {
        bool mNeedConfig = true;
        const char* whiteNameList[] = {
            "com.android.camera",
            "com.miui.face",
            "com.android.cts.verifier"
        };

        for (uint32_t pkgNameIdx = 0; pkgNameIdx < sizeof(whiteNameList) / sizeof(whiteNameList[0]); pkgNameIdx++) {
            if (0 == String8(whiteNameList[pkgNameIdx]).compare(String8(clientPackageName))) {
                mNeedConfig = false;
            }
        }
        if (true == mNeedConfig) {
            usleep(450000);
        }
    }
    #endif
}

void CameraImpl::setTargetSdkVersion(int targetSdkVersion) {
    ALOGV("%s:%d targetSdkVersion: %d", __FUNCTION__, __LINE__, targetSdkVersion);
    mTargetSdkVersion = targetSdkVersion;
}

int CameraImpl::getTargetSdkVersion() {
    return mTargetSdkVersion;
}

// Qcom methods begin
void CameraImpl::setNfcPolling(const char *value) {
    ALOGV("%s:%d value: %s", __FUNCTION__, __LINE__, value);
}

void CameraImpl::getRawBestSize(const CameraMetadata& info, int32_t width,
       int32_t height, int32_t format, int32_t &bestWidth, int32_t &bestHeight) {
    ALOGV("%s:%d %p %d %d %d %d %d", __FUNCTION__, __LINE__,
        &info, width, height, format, bestWidth, bestHeight);
}

bool CameraImpl::isPrivilegedClient() {
    return false;
}

String16 CameraImpl::getplatformversion() {
    return String16("");
}
// Qcom methods end

bool CameraImpl::getRequestMetadata(const CameraMetadata &metadata, const char* key, int *result, size_t lens) {
    sp<VendorTagDescriptor> vTags =
            VendorTagDescriptor::getGlobalVendorTagDescriptor();
    if ((nullptr == vTags.get()) || (0 >= vTags->getTagCount())) {
        sp<VendorTagDescriptorCache> cache =
                VendorTagDescriptorCache::getGlobalVendorTagCache();
        if (cache.get()) {
            const camera_metadata_t *metaBuffer = metadata.getAndLock();
            metadata_vendor_id_t vendorId = get_camera_metadata_vendor_id(metaBuffer);
            metadata.unlock(metaBuffer);
            cache->getVendorTagDescriptor(vendorId, &vTags);
        }
    }

    uint32_t tag;
    bool isSuccess = false;
    status_t res = metadata.getTagFromName(key, vTags.get(), &tag);
    int type = vTags->getTagType(tag);
    if (res == OK) {
        camera_metadata_ro_entry entry = metadata.find(tag);
        int size = lens > entry.count ? entry.count : lens;
        for(int i = 0; i < size; i++){
            if(TYPE_BYTE == type) {
                result[i] = entry.data.u8[i] & 0xFF ;
            } else {
                result[i] = entry.data.i32[i];
            }
        }
        isSuccess = true;
    }
    return isSuccess;
}

int CameraImpl::getPluginRequestCount(const CameraMetadata &metadata, const CameraMetadata &deviceInfo) {
    int type = 0;
    bool success = getRequestMetadata(metadata, "xiaomi.burst.captureType", &type, 1);
    if(!success || 1 != type) {
        ALOGI("%s, not burst mode, captureType(%d), success(%d)", __FUNCTION__, type, success);
        return 0;
    }
    camera_metadata_ro_entry inputStreams = metadata.find(ANDROID_REQUEST_INPUT_STREAMS);
    if (inputStreams.count > 0) {
        return 0;
    }

    int heicShot = 0;
    success = getRequestMetadata(metadata, "xiaomi.HeicSnapshot.enabled", &heicShot, 1);
    if(!success || (0 != heicShot && 1 != heicShot)) {
        ALOGI("%s, get key xiaomi.HeicSnapshot.enabled(%d), success(%d)", __FUNCTION__, heicShot, success);
        return 0;
    }

    int pluginCount[2] = {0};
    success = getRequestMetadata(deviceInfo, "xiaomi.burst.capacityPlugin", pluginCount, 2);
    if(success) {
        return pluginCount[heicShot];
    }
    return 0;
}

bool CameraImpl::checkInvisibleMode(int32_t mode) {
    bool invisibleMode = false;
    if (mode == AppOpsManager::MODE_IGNORED) {
        char invisibleState[PROPERTY_VALUE_MAX] = { 0 };
        property_get("persist.sys.invisible_mode", invisibleState, "0");
        if (!strncmp(invisibleState, "1", 1)) {
            invisibleMode = true;
        }
    }
    return invisibleMode;
}

bool CameraImpl::isSupportPrivacyCamera(const CameraMetadata& cameraMetadata) {
    return PrivacyCamera::isSupportPrivacyCamera(cameraMetadata);
}

android::IPrivacyCamera* CameraImpl::createPrivacyCamera(const CameraMetadata& cameraMetadata) {
    if(isSupportPrivacyCamera(cameraMetadata)) {
        return new PrivacyCamera(cameraMetadata);
    }

    return nullptr;
}

void CameraImpl::dumpDropInfo(bool isVideoStream, int format, nsecs_t timestamp,  nsecs_t lastTimeStamp) {
    // write time diff to systrace to find frame drop
    if (!isVideoStream && (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
                format == HAL_PIXEL_FORMAT_YCbCr_420_TP10_UBWC ||
                format == HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS_UBWC) && lastTimeStamp > 0) {
        int64_t diff;
        static int64_t lastPreviewDiff = 0;
        if (mLastPreviewTimeReturn == 0) {
            mLastPreviewTimeReturn = systemTime();
        } else {
            diff = systemTime() - mLastPreviewTimeReturn;
            mLastPreviewTimeReturn = systemTime();
            ATRACE_INT64("preview", (int64_t)diff);
            if(diff > 2*lastPreviewDiff) {
                ALOGI("%s:: preview drop more frame, diff %" PRId64 ", lastDiff %" PRId64,
                        __FUNCTION__, diff, lastPreviewDiff);
            }
            lastPreviewDiff = diff;
        }

        if (mLastReadoutTime == 0) {
            diff = 0;
        } else {
            diff = timestamp - mLastReadoutTime;
        }
        mLastReadoutTime = timestamp;
        static bool isNeedSkip = false;
        if(diff < 0) {
            isNeedSkip = true;
            diff = 0;
        } else if(isNeedSkip){
            isNeedSkip = false;
            diff = 0;
        }
        ATRACE_INT64("readoutTime", (int64_t)diff);
    }
    if (isVideoStream && lastTimeStamp > 0) {
        int64_t diff;
        static int64_t lastVideoDiff = 0;
        if (mLastVideoTimeReturn == 0) {
            mLastVideoTimeReturn = systemTime();
        } else {
            diff = systemTime() - mLastVideoTimeReturn;
            mLastVideoTimeReturn = systemTime();
            ATRACE_INT64("video", (int64_t)diff);
            if(diff > 2*lastVideoDiff) {
                ALOGI("%s: video drop more frame, diff %" PRId64 ", lastDiff %" PRId64,
                        __FUNCTION__, diff, lastVideoDiff);
            }
            lastVideoDiff = diff;
        }
    }
}

void CameraImpl::resetFrameInfo() {
    mLastPreviewTimeReturn = 0;
    mLastVideoTimeReturn   = 0;
    mLastReadoutTime       = 0;
}

