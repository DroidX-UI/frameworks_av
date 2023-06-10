//add for fix android R compile error
//#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

#include <unistd.h>
#include <fcntl.h>

#include <binder/Parcel.h>
#include "IPopupCameraManager.h"

namespace android {

// ------------------------------------------------------------------------------------

class BpPopupCameraManager : public BpInterface<IPopupCameraManager>
{
public:
    explicit BpPopupCameraManager(const sp<IBinder>& impl)
        : BpInterface<IPopupCameraManager>(impl)
    {
    }

    virtual bool notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName)
    {
         Parcel data, reply;
         data.writeInterfaceToken(IPopupCameraManager::getInterfaceDescriptor());
         data.writeInt32(cameraId);
         data.writeInt32(status);
         data.writeString16(clientPackageName);
         remote()->transact(NOTIFY_CAMERA_STATUS_TRANSACTION, data, &reply);
         // fail on exception
         if (reply.readExceptionCode() != 0) return false;
         return reply.readInt32() == 1;
    }
};

// ------------------------------------------------------------------------------------

IMPLEMENT_META_INTERFACE(PopupCameraManager, "miui.popupcamera.IPopupCameraManager");

}; // namespace android
