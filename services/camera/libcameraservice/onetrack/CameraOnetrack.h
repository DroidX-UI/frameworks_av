#if defined(MIUI_VERSION)

#ifndef ANDROID_CAMERA_ONETRACK_H
#define ANDROID_CAMERA_ONETRACK_H

#include <utils/RefBase.h>
#include "MQSServiceManager.h"
#include <utility>
#include <initializer_list>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

using android::String16;
using android::String8;

typedef std::vector<std::pair<std::string,std::string>>
        EventData;

class Event{
public:
    Event() = delete;
    // init the Event need provide event_name and param_name
    // then set param_value in the order of param_name
    Event(std::string name,std::initializer_list<std::string> params) : mEventName(name),mCurPointer(1)
    {
        mDataList.push_back({"EVENT_NAME",mEventName});
        for(auto &param : params) {
            mDataList.push_back({param,""});
        }
    }

    template <typename T>
    inline void setParams(T val)
    {
        addParameter(val);
    }
    template <typename T,typename... Ts>
    void setParams(T val,Ts... args)
    {
        addParameter(val);
        setParams(args...);
    }

    void clear() {
        mCurPointer = 1;
    }
    EventData mDataList;

private:
    template <typename T>
    inline void addParameter(T val)
    {
        if(mCurPointer<mDataList.size())
            mDataList[mCurPointer++].second = std::to_string(val);
    }
    inline void addParameter(std::string val)
    {
        if(mCurPointer<mDataList.size())
            mDataList[mCurPointer++].second = val;
    }
    inline void addParameter(const char *val)
    {
        if(mCurPointer<mDataList.size())
            mDataList[mCurPointer++].second = val;
    }


    std::string mEventName;
    size_t mCurPointer;
};

class CameraOnetrack : public android::RefBase{
public:
    static CameraOnetrack* getInstance()
    {
        static CameraOnetrack cameraOnetrack;
        return &cameraOnetrack;
    }
    virtual ~CameraOnetrack();

    //param : the list of key value pair
    void reportData(Event &event, String8 &appId, std::string clientName);
    void onetrack_report(std::string& data, String8 &appId, std::string clientName);
    void addDeferredTask(std::function<void(void)> task);

private:
    CameraOnetrack();
    void deferredTasksLoop();
    std::thread mDeferredTasksThread;
    bool mExitThread;
    std::queue<std::function<void(void)>> mTaskQueue;
    std::mutex mMutex;
    std::condition_variable mCond;
};

class CameraSdkReporter{
public:
    CameraSdkReporter();
    void reportConfigureStream(std::string &name, const char *cameraId, uint32_t opMode, uint32_t format);
    void reportRequest(std::string &clientName, std::string &list, uint32_t opMode);
    String8 mAppid;
private:
    Event mConfigEvent;
    Event mRequestEvent;
};

#endif

#endif
