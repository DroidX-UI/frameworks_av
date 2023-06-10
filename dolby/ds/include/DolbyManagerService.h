/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2016-2022 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/
#ifndef DOLBY_DOLBYMANAGERSERVICE_H
#define DOLBY_DOLBYMANAGERSERVICE_H
#include <cutils/misc.h>
#include <cutils/config_utils.h>
#include <cutils/compiler.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AHandler.h>
#include <utils/String8.h>
#include "DsManager.h"
#include "DapParamStore.h"
#include "DmsParams.h"
#include <vendor/dolby/hardware/dms/2.0/IDmsCallbacks.h>


/*
*  Dolby Manager Service provides several functionalities for dolby modules and service management
*  1. Clients management : all the clients' handler would register to NotificationClient vectors, unregister/death/callback events would be handled by this service.
*  2. DsManager Access : DsManager is the framework to manage all the dap effects. DsManager includes tuning file parsing/management, database creation/insertion/query,
*      parameter cache and filter, and etc.
*  3. Parameter Sharing : Many parameters(generally content based) will be shared by track based dap, vlldp and decoder's dap through shared store parameter cache.
*/

namespace android {
using namespace dolby;
using ::android::hardware::hidl_death_recipient;
using ::vendor::dolby::hardware::dms::V2_0::IDmsCallbacks;


// ----------------------------------------------------------------------------
#define SHARED_BUFFER_SIZE 1024
class DolbyManagerService : public RefBase
    // public BinderService<DolbyManagerService>,
    // public BnDolbyManagerService,
    // public IBinder::DeathRecipient
{
    //friend class BinderService<DolbyManagerService>;
public:
    // for BinderService
    //static const char *getServiceName() ANDROID_API { return DOLBYSERVICE; }

    DolbyManagerService() ANDROID_API;
    virtual ~DolbyManagerService();

    /*virtual status_t onTransact(uint32_t code,
    //                            const Parcel& data,
                                Parcel* reply,
                                uint32_t flags);
    */
    // virtual void binderDied(const wp<IBinder>& who);
    // RefBase
    virtual void onFirstRef();
    // IDms api
    void registerClient(const sp<IDmsCallbacks>& client, DolbyClientType type, uint32_t uid);
    void unregisterClient(const sp<IDmsCallbacks>& client, DolbyClientType type, uint32_t uid);

    // APIs for clients management
    void removeNotificationClient(uid_t uid);
    bool hasDecCP();
    bool hasDecDV();
    bool hasMulitchannelDecDV();
    bool isMobilityProfileSelected();
    int getVolumeLevelerAount();
    bool isMonoInternalSpeaker();
    bool isSpeakerPortraitMode();
    int getDialogEnhancerAmount();
    int getTuningParamDSA();
    int getTuningParamDHSA();
    int getTuningParamDFSA();
    bool getDapEnabled();

    // APIs for shared store cache access
    bool getStoredParams(dolby::DapParamStore::ParamStore *store, bool checkToken, int32_t *token);
    bool setParam(audio_devices_t deviceId, DapParameterId param, const dap_param_value_t* values, uint32_t length);
    void commitChange();

    // API for set/get parameter through AudioEffect
    status_t setDapParam(void *pValues, uint32_t length);
    status_t getDapParam(void *inVal, uint32_t inLen, void *outVal, uint32_t outLen);

    // API for profile management
    std::string getProfileName(int profile);

    // API for ieq preset management
    std::string getIeqPresetName(int preset);

    // API for device tuning management
    int getAvailableTuningDevicesLen(int port);
    void getAvailableTuningDevices(int port, void *buf, int len);
    std::string getSelectedTuningDevice(int port);
    void setSelectedTuningDevice(int port, void *buf, int len);

    void setActiveDevice(audio_devices_t device) { mActiveDevice.store(device); }
    audio_devices_t getActiveDevice();
    void setChannelCount(uint32_t count) { mChannelCount.store(count); }
    uint32_t getChannelCount() { return mChannelCount.load(); }
    uint32_t getLastDecodeProcessTime() { return (uint32_t)mTimeOfLastDecodeProcess; }
    int64_t getDefaultXmlFormatVersion();
    int getAtmosGameSystemGain();
    int64_t getIntParam(uint32_t paramID);
    void setIntParam(uint32_t paramID, int64_t paramVal);
private:
    enum {
        kWhatSetParam = 'setp',
        kWhatSetTuning = 'sett',
        kWhatRegClient = 'regc',
    };
    // --- Notification Client ---
    class NotificationClient : public hidl_death_recipient {
    public:
        NotificationClient(const sp<DolbyManagerService>& service,
                           const sp<IDmsCallbacks>& client,
                           const DolbyClientType type,
                           uid_t uid);

        virtual             ~NotificationClient();
        // hidl_death_recipient
        virtual void serviceDied(uint64_t /*cookie*/, const wp<::android::hidl::base::V1_0::IBase> & /*who*/);
        const DolbyClientType               mType;
        const uid_t                         mUid;
        const sp<IDmsCallbacks>             mClient;
        // Callback to client
        void onDapParamUpdate(void *values, uint32_t length);
        void getDapParam(void *inVal, uint32_t inLen, void *outVal, uint32_t outLen);
    private:
        NotificationClient(const NotificationClient&);
        NotificationClient& operator = (const NotificationClient&);

        const wp<DolbyManagerService>        mService;
    };

    /*Asynchronized Event Handler for setting parameters to underlying native DsService*/
    struct EventHandler : public AHandler {
        explicit EventHandler(DsManager* manager, DolbyManagerService* service);
        void start();
        void stop();
        void sendMessage(void *pValues, uint32_t length, uint32_t what);
        void sendMessage(void *pValues, uint32_t length, int port, uint32_t what);
    protected:
        virtual void onMessageReceived(const sp<AMessage> &msg);
        virtual ~EventHandler();
    private:
        void parseAndSendParam(void *values, uint32_t length);
        sp<ALooper> mLooper;
        DsManager* mDsManager;
        DolbyManagerService* mParentService;
        DISALLOW_EVIL_CONSTRUCTORS(EventHandler);
    };

    void packAndReceiveParam(void *inVal, uint32_t inLen, void *outVal, uint32_t outLen);
    void doOnDapParamUpdate(void *values, uint32_t length);
    const wp<DolbyManagerService> mService;
    DefaultKeyedVector<uid_t, sp<NotificationClient> > mNotificationClients;
    sp<EventHandler> mHandler;
    Mutex mNotificationClientsLock;  // protects mNotificationClients
    std::atomic<int>  mHasDecCP;
    std::atomic<int>  mHasDecDV;
    std::atomic<int>  mHasMultichannelDecDV;
    DsManager* mDsManager;
    bool mIsDsManagerInit;
    void dumpPermissionDenial(int fd);

    std::atomic<audio_devices_t> mActiveDevice;
    std::atomic<uint32_t>        mChannelCount;
    int64_t mLastRegisteredCodecType;
    int64_t mTimeOfLastDecodeProcess;
    int64_t mMaxMultichannelDecOutput;
    int64_t mIsSpatializerRegistered;
    int64_t mIsGameDAPRegistered;
};
}; // namespace android

#endif // DOLBY_DOLBYMANAGERSERVICE_H
