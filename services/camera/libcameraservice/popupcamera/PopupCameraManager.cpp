
#include <mutex>
#include <unistd.h>
#include <binder/Binder.h>
#include <binder/IServiceManager.h>

#include <utils/SystemClock.h>
#include "PopupCameraManager.h"

namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(PopupCameraManager);

void PopupCameraManager::DeathNotifier::binderDied(const wp<IBinder>& /*who*/) {
    PopupCameraManager::getInstance().onServiceDied();
}

PopupCameraManager::PopupCameraManager()
{
}

PopupCameraManager::~PopupCameraManager() {
    Mutex::Autolock _l(mLock);
    if (mDeathNotifier != nullptr && mService != nullptr) {
        IInterface::asBinder(mService)->unlinkToDeath(mDeathNotifier);
    }
}

sp<IPopupCameraManager> PopupCameraManager::getService()
{
    if (mService != nullptr) {
        return mService;
    }
    // Get popupcamera service from service manager
    const sp<IServiceManager> sm(defaultServiceManager());
    if (sm != nullptr) {
        sp<IBinder> binder = sm->checkService(String16("popupcamera"));
        if (binder == nullptr) {
            ALOGE("popupcamera service is null");
            return nullptr;
        } else {
            mService = interface_cast<IPopupCameraManager>(binder);
            mDeathNotifier = new DeathNotifier();
            IInterface::asBinder(mService)->linkToDeath(mDeathNotifier);
            ALOGE("popupcamera service is %p", &mService);
        }
        }
    return mService;
}

bool PopupCameraManager::notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName)
{
    Mutex::Autolock _l(mLock);
    sp<IPopupCameraManager> service = getService();
    if (service != NULL) {
        return service->notifyCameraStatus(cameraId, status, clientPackageName);
    }
    return false;
}

void PopupCameraManager::onServiceDied() {
    Mutex::Autolock _l(mLock);
    mService.clear();
    mDeathNotifier.clear();
    ALOGE("popupcamera service is died");
}

}; // namespace android
