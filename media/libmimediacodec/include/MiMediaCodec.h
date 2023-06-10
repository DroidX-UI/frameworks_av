#ifndef MIMEDIACODEC_IMPL_H
#define MIMEDIACODEC_IMPL_H
#include <memory>
#include <vector>
#include <media/stagefright/foundation/AHandler.h>
#include <utils/Vector.h>
#include <media/stagefright/CodecBase.h>
#include <utils/String16.h>
#include <media/stagefright/foundation/AString.h>
#include <media/MediaCodecBuffer.h>
namespace android {
namespace mimediacodec {
class IMiMediaCodecStub {
public:
    virtual ~IMiMediaCodecStub() {}
    virtual void ResetSendFpsToDisplay(bool needSendFps, uint32_t id);
    virtual void VideoToDisplayRealtimeFramerate(const sp<AMessage> &msg, uint32_t id);
    virtual void SetFpsFromPlayer(float fpsfromplayer);
    virtual void SendDoviPlaybackToDisplay(int status);
};
class MiMediaCodec : public IMiMediaCodecStub {
public:
    MiMediaCodec();
    virtual ~MiMediaCodec();
    void ResetSendFpsToDisplay(bool needSendFps, uint32_t id);
    void VideoToDisplayRealtimeFramerate(const sp<AMessage> &msg, uint32_t id);
    void SetFpsFromPlayer(float fpsfromplayer);
    void SendDoviPlaybackToDisplay(int status);

private:
    bool mDecRealTimeFpsSend;
    float mFpsFromPlayer;
    float mFrameDecodedCount;
    float mLastFrameDecodedCount;
    float mDefaultFrameRate;
    float mFrameRateByTimestamp;
    float mFrameRateByBufferCount;
    float mLastSendedFps;
    int64_t mLastTimeStampRealTimeUs;
    int64_t mStartTimeStampRealTimeUs;
    int64_t mBufferInputStartTimeUs;
    int64_t mLastClacFpsPtsDiff;
    int64_t mTimeStampSkipValue;
    int64_t mTimeStampSkip;
    int64_t mTimeStampSkipStartTime;
    uint32_t mCurrentTid;
    //calc fps by timestamp diff
    int64_t mCurrentTimeStampAbsDiff;
    int64_t mMinTimeStampAbsDiff;
    int mMinTimeStampAbsNum;
    int64_t mLastQueueTime;
    bool mFrcOpened;
};
extern "C" IMiMediaCodecStub* create();
extern "C" void destroy(IMiMediaCodecStub* impl);
}
}
#endif
