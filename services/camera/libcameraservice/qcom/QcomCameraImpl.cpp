#define LOG_TAG "QcomCameraImpl"
#define ATRACE_TAG ATRACE_TAG_CAMERA
// #define LOG_NDEBUG 0

#include <log/log.h>
#include <cutils/properties.h>
#include <utils/String16.h>
#include <binder/Parcel.h>
#include <utils/Trace.h>
#include <binder/IServiceManager.h>
#include "system/camera_metadata.h"
#include "camera_metadata_hidden.h"
#include <camera/VendorTagDescriptor.h>
#include <system/graphics-base.h>
#include <pthread.h>
#include "QcomCameraImpl.h"

using namespace android;

QcomCameraImpl::QcomCameraImpl() {
    ALOGD("%s:%d ", __FUNCTION__, __LINE__);
    kProductDevice = getStringProperty("ro.product.device", "");
    mPackageList   = getStringProperty("persist.vendor.camera.privapp.list", "");
    kProductPlatform = getStringProperty("ro.board.platform", "");
}

QcomCameraImpl::~QcomCameraImpl() {
    ALOGD("%s:%d ", __FUNCTION__, __LINE__);
}

void QcomCameraImpl::setNfcPolling(const char *value) {
    ALOGV("%s:%d E", __FUNCTION__, __LINE__);
    if (kProductDevice.startsWith(String16("venus")) || kProductDevice.startsWith(String16("renoir"))
        || kProductDevice.startsWith(String16("haydnin")) || kProductDevice.startsWith(String16("haydn"))
        || kProductDevice.startsWith(String16("cetus")) || kProductDevice.startsWith(String16("cmi"))
        || kProductDevice.startsWith(String16("umi")) || kProductDevice.startsWith(String16("alioth"))
        || kProductDevice.startsWith(String16("vayu"))) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> obj = sm->checkService(String16("nfc"));
        ALOGD("%s: defaultServiceManager %p, nfc status will be %s",
                                    __FUNCTION__, obj.get(), value);
        // nfc disable polling : "1", restore polling: "0"
        property_set("nfc.polling.disable", value);
        if (obj != NULL) {
            Parcel data;
            obj->transact(IBinder::SYSPROPS_TRANSACTION, data, NULL, 0);
        }
    }
    ALOGV("%s:%d X", __FUNCTION__, __LINE__);
}

void QcomCameraImpl::getRawBestSize(const CameraMetadata& info, int32_t width,
       int32_t height, int32_t format, int32_t &bestWidth, int32_t &bestHeight) {
    if (bestWidth == -1 && format == HAL_PIXEL_FORMAT_RAW10) {
        bool isLogicalCamera = false;
        auto entry = info.find(ANDROID_REQUEST_AVAILABLE_CAPABILITIES);
        for (size_t i = 0; i < entry.count; ++i) {
            uint8_t capability = entry.data.u8[i];
            if (capability == ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
                isLogicalCamera = true;
                break;
            }
        }

        if (isLogicalCamera == true) {
            bestWidth = width;
            bestHeight = height;
        }
        ALOGD("%s:%d Raw bestWidth %d bestHeight %d",
            __FUNCTION__, __LINE__, bestWidth, bestHeight);
    }
}

bool QcomCameraImpl::isPrivilegedClient() {
    bool privilegedClient = false;
    if (mPackageList.contains(mClientPackageName.string())) {
        privilegedClient = true;
        ALOGD("%s:%d ", __FUNCTION__, __LINE__);
    }
    return privilegedClient;
}

String16 QcomCameraImpl::getplatformversion() {
    return kProductPlatform;
}

extern "C" ICameraStub* create() {
    return new QcomCameraImpl;
}

extern "C" void destroy(ICameraStub* impl) {
    delete impl;
}
