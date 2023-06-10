#if defined(MIUI_VERSION)
#define LOG_TAG "CameraOnetrack"
#define ATRACE_TAG ATRACE_TAG_CAMERA
//#define LOG_NDEBUG 0

#include <log/log.h>
#include <pthread.h>
#include "CameraOnetrack.h"

using namespace android;

static unsigned long CameraOnetrackReportNums = 0;

void CameraOnetrack::addDeferredTask(std::function<void(void)> task)
{
    std::lock_guard<std::mutex> locker{mMutex};
    mTaskQueue.push(task);
    ALOGI("camera_onetrack_report: add a new task,task sum: %d", static_cast<int>(mTaskQueue.size()));
    mCond.notify_one();
}

void CameraOnetrack::deferredTasksLoop()
{
    std::unique_lock<std::mutex> locker{mMutex};
    while(true) {
        mCond.wait(locker, [this](){
            if (mExitThread)
                return true;
            return !mTaskQueue.empty();
        });

        if (mExitThread) {
            break;
        }

        auto &task = mTaskQueue.front();
        ALOGI("camera_onetrack_report: do a task,task sum: %d", static_cast<int>(mTaskQueue.size()));
        locker.unlock();

        task();

        locker.lock();
        mTaskQueue.pop();
    }
}

CameraOnetrack::CameraOnetrack() : mExitThread(false) {
    mDeferredTasksThread = std::thread{
        [this]()
        { deferredTasksLoop(); }};
    ALOGD("%s:%d ", __FUNCTION__, __LINE__);
}

CameraOnetrack::~CameraOnetrack() {
    std::unique_lock<std::mutex> locker{mMutex};
    mExitThread = true;
    locker.unlock();

    mCond.notify_one();
    mDeferredTasksThread.join();
    ALOGD("%s:%d ", __FUNCTION__, __LINE__);
}

void CameraOnetrack::onetrack_report(std::string &data, String8 &appId, std::string clientName) {
    ALOGI("camera_onetrack_report: %s", data.c_str());

    int onetrackFlag = MQSServiceManager::getInstance().reportOneTrackEvent(appId, String8(clientName.c_str()), String8(data.c_str()), 0);
    if (0 != onetrackFlag) {
        ALOGE("camera_onetrack_report: reportOneTrackEvent failed");
    }
    else {
        ALOGI("camera_onetrack_report: onetrack_report_nums: %lu", ++CameraOnetrackReportNums);
    }
}

void CameraOnetrack::reportData(Event &event, String8 &appId, std::string clientName)
{
    auto dataList = event.mDataList;
    std::string jsonData("{");
    jsonData.reserve(dataList.size()*2*5);
    for (auto &data : dataList) {
        jsonData += "\"";
        jsonData += data.first;
        jsonData += "\":";
        jsonData += data.second;
        jsonData += ",";
    }
    jsonData[jsonData.size()-1] = '}';
    onetrack_report(jsonData, appId, clientName);
}

CameraSdkReporter::CameraSdkReporter() : 
        mAppid{"31000000774"},
        mConfigEvent{"configureStreams",{"cameraId", "opMode", "snapshotFormat"}},
        mRequestEvent{"request",{"functionList"}}
{

}

void CameraSdkReporter::reportConfigureStream(std::string &name,const char* cameraId,uint32_t opMode,uint32_t format)
{
    std::string cId(cameraId);
    CameraOnetrack::getInstance()->addDeferredTask([=]() {
        mConfigEvent.clear();
        mConfigEvent.setParams(cId,opMode,format);
        CameraOnetrack::getInstance()->reportData(mConfigEvent, mAppid, name);
    });
}

void CameraSdkReporter::reportRequest(std::string &clientName, std::string &list, uint32_t opMode)
{
    CameraOnetrack::getInstance()->addDeferredTask([=]() {
        mRequestEvent.clear();

        if(list.size() == 0) {
            mRequestEvent.setParams(opMode);
        } else {
            mRequestEvent.setParams(list);
        }
        CameraOnetrack::getInstance()->reportData(mRequestEvent, mAppid, clientName);
    });
}

#endif
