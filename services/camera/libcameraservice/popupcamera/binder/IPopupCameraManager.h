//
// Created by longhai on 19-2-13.
//

#ifndef ANDROID_IPOPUPCAMERAMANAGER_H
#define ANDROID_IPOPUPCAMERAMANAGER_H

#include <binder/IInterface.h>

namespace android {

// ------------------------------------------------------------------------------------

class IPopupCameraManager : public IInterface
{
public:
    DECLARE_META_INTERFACE(PopupCameraManager)

    virtual bool notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName) = 0;

    enum {
        NOTIFY_CAMERA_STATUS_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION,
    };
};

// ------------------------------------------------------------------------------------

}; // namespace android

#endif //ANDROID_IPOPUPCAMERAMANAGER_H
