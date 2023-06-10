#ifndef ANDROID_QCOMCAMERAIMPL_H
#define ANDROID_QCOMCAMERAIMPL_H

#include "camera/CameraMetadata.h"
#include <CameraImpl.h>

using android::CameraMetadata;
using android::String16;
using android::String8;
using android::sp;
using android::status_t;

class QcomCameraImpl : public CameraImpl{
public:
    QcomCameraImpl();
    virtual ~QcomCameraImpl();
    virtual void setNfcPolling(const char *value);
    virtual void getRawBestSize(const CameraMetadata& info, int32_t width /*in*/,
        int32_t height /*in*/, int32_t format /*in*/, int32_t &bestWidth /*out*/,
        int32_t &bestHeight /*out*/);
    virtual bool isPrivilegedClient();
    virtual String16 getplatformversion();

private:
    String16    kProductDevice;
    String16    mPackageList;
    String16    kProductPlatform;
};
#endif
