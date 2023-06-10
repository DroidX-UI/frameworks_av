#ifndef ANDROID_CAMERA_ICAMERAIMPL_H
#define ANDROID_CAMERA_ICAMERAIMPL_H

#include <xm/ICameraStub.h>
#include "common/SensorThread.h"
#include "privacy/PrivacyCamera.h"

#if defined(MIUI_VERSION)
#include "onetrack/CameraOnetrack.h"
#endif

#if defined(MIUI_VERSION)

class CameraSdkData
{
public:
    CameraSdkData() : mSessionOp(0) {}

    void setSessionOp(int32_t sessionOp) { mSessionOp = sessionOp; }
    uint32_t getSessionOp() {return mSessionOp;}

    void setClientName(std::string clientName) { mClientName = clientName; }
    std::string& getClientName() {return mClientName;}

    bool isSdkApp() { return mSessionOp != 0; }
    bool isVideo() {
        if(mSessionOp == 0xff0d || mSessionOp == 0xff10 || mSessionOp == 0xff11 ||
                mSessionOp == 0xff13 || mSessionOp == 0xff14)
        {
            return true;
        } else {
            return false;
        }
    }

    void clear()
    {
        mLastPreviewFun.clear();
    }

    CameraSdkReporter mReport;
    // to avoid report same message
    std::string mLastPreviewFun;

private:
    uint32_t mSessionOp;
    std::string mClientName;
};

#endif

class CameraImpl : public ICameraStub {
public:
    CameraImpl();
    virtual ~CameraImpl();
    virtual void setClientPackageName(const String16& clientName, int32_t api_num);
    virtual void updateSessionParams(CameraMetadata& sessionParams);
    virtual void adjSysCameraPriority(const String8& packageName,
             std::vector<int> &priorityScores, std::vector<int> &states);
    virtual void adjCameraPriority(const String8& packageName,
        std::vector<int> &priorityScores, std::vector<int> &states,
        std::vector<std::pair<std::string, int>> &clientList);
    virtual void adjNativeCameraPriority(const String8& key, int32_t& score_adj, int32_t& state_adj);
    virtual bool isFaceUnlockPackage(const String8& cameraId, bool systemNativeClient);
    virtual status_t getTagFromName(const CameraMetadata& info,
            char* tagName, uint32_t &tag);
    virtual void getCustomBestSize(const CameraMetadata& info, int32_t width /*in*/,
        int32_t height /*in*/, int32_t format /*in*/, int32_t &bestWidth /*out*/,
        int32_t &bestHeight /*out*/);
    virtual void createCustomDefaultRequest(CameraMetadata& metadata);
    virtual bool checkInvisibleMode(int32_t mode);
    virtual String16 getStringProperty(const char* key, const char* defaultValue);
    virtual status_t repeatAddFakeTriggerIds(status_t res, CameraMetadata &metadata);
    virtual void checkAllClientsStoppedAndReclaimMem(int32_t *cameraProviderPid, bool *memDirty);
    virtual bool checkSensorFloder(CameraMetadata *CameraCharacteristics,
        CameraMetadata *gCameraCharacteristics, pthread_rwlock_t *rwLock);
    virtual void notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName);
    virtual void setTargetSdkVersion(int targetSdkVersion);
    virtual int getTargetSdkVersion();
    // Qcom methods begin
    virtual void setNfcPolling(const char *value);
    virtual void getRawBestSize(const CameraMetadata& info, int32_t width /*in*/,
        int32_t height /*in*/, int32_t format /*in*/, int32_t &bestWidth /*out*/,
        int32_t &bestHeight /*out*/);
    virtual bool isPrivilegedClient();
    virtual String16 getplatformversion();
    // Qcom methods end
    virtual int getPluginRequestCount(const CameraMetadata &metadata, const CameraMetadata &deviceInfo);
    bool isSupportPrivacyCamera(const CameraMetadata& cameraMetadata);
    android::IPrivacyCamera* createPrivacyCamera(const CameraMetadata & cameraMetadata);
    void reportSdkConfigureStreams(const String8& cameraId, camera_stream_configuration *config, const CameraMetadata &sessionParams) override;
    void reportSdkRequest(camera_capture_request_t* request) override;
    virtual void dumpDropInfo(bool isVideoStream, int format, nsecs_t timestamp,  nsecs_t lastTimeStamp);
    virtual void resetFrameInfo();

public:
    String16    mClientPackageName;
    int32_t     mApiVersion;
    int         mTargetSdkVersion;
    nsecs_t     mLastPreviewTimeReturn; //for trace
    nsecs_t     mLastVideoTimeReturn; //for trace
    nsecs_t     mLastReadoutTime; // for trace

    android::SensorFolderRegister mSensorFloder;

private:
    bool getRequestMetadata(const CameraMetadata &metadata, const char* key, int *result, size_t lens);
    bool isAonCameraId(const String8& cameraId);

#if defined(MIUI_VERSION)
    CameraSdkData mCameraSdkData;
#endif
};

#endif
