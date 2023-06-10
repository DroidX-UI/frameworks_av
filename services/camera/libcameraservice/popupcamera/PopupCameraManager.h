
#ifndef ANDROID_POPUPCAMERA_MANAGER_H
#define ANDROID_POPUPCAMERA_MANAGER_H

#include "binder/IPopupCameraManager.h"
#include <utils/Singleton.h>

// ---------------------------------------------------------------------------
namespace android {

class PopupCameraManager : public Singleton<PopupCameraManager> {

    friend class Singleton<PopupCameraManager>;
    PopupCameraManager();

public:
    ~PopupCameraManager();
    bool notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName);

private:
    void onServiceDied();

    class DeathNotifier : public IBinder::DeathRecipient {
        virtual void binderDied(const wp<IBinder>& /*who*/);
    };

    Mutex mLock;
    sp<IPopupCameraManager> mService;
    sp<DeathNotifier> mDeathNotifier;

    sp<IPopupCameraManager> getService();
};
}; // namespace android
// ---------------------------------------------------------------------------

#endif // ANDROID_POPUPCAMERA_MANAGER_H
