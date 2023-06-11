#ifndef _VIDEO_INFO_IMP_
#define _VIDEO_INFO_IMP_

#include <sys/types.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <media/stagefright/foundation/AString.h>

enum DataType {
    DATA_TYPE_RAW,
    DATA_TYPE_YUV,
    DATA_TYPE_MAX,
};

namespace android {

class VideoInfoImp {
public:
    String16 mClientName;
    AString mCodecName;

    uint64_t mLastBufferQueueTime;
    uint64_t mLastBufferDequeueTime;
    uint64_t mLastBufferRenderTime;
    uint64_t mMaxQueueInputInterver;
    uint64_t mInputBufferNum;
    uint64_t mMaxDequeueOutputInterver;
    uint64_t mOutputBufferNum;
    uint64_t mMaxRenderOutputInterver;
    uint64_t mRenderBufferNum;

    bool mVideoDebugPerf;
    bool mVideoDebugDataDump;

    VideoInfoImp(String16 &clientName, AString &codecName);
    ~VideoInfoImp();
    void dumpVideoData(bool input, DataType type, void *buffer, uint64_t size, uint32_t width = 0, uint32_t height = 0);
    void printInputBufferInterver(int64_t presentationTimeUs);
    void printOutputBufferInterver(int64_t presentationTimeUs);
    void printRenderBufferInterver(int64_t presentationTimeUs);

private:
    int32_t openDumpFd(const char *filePath, int32_t &fd);
    void dumpDataToFile(char *filePath, void *buffer, uint64_t size);

protected:

};

} //namespace android

#endif
