//#define LOG_NDEBUG 0
#define LOG_TAG "MiMediaCodec"
#include <utils/Log.h>
#include <inttypes.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/ColorUtils.h>
#include <media/stagefright/foundation/AUtils.h>
#include <binder/IServiceManager.h>
#include "MiMediaCodec.h"
#include <string.h>
#include <utils/SystemClock.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <gui/ISurfaceComposer.h>
#include <private/gui/ComposerService.h>
#include <gui/ISurfaceComposerClient.h>
#include <gui/SurfaceComposerClient.h>
#include <private/gui/ComposerService.h>
#include <android-base/properties.h>
#define SEND_FPS_TO_DISPLAY 31115

namespace android {
namespace mimediacodec {
MiMediaCodec::MiMediaCodec()
{
    ALOGV("MiMediaCodec::MiMediaCodec()");
    //sometimes first flush then stop
    mLastSendedFps = 0.0f;
    ResetSendFpsToDisplay(false, 0);
    mTimeStampSkipValue = 700000;
    mTimeStampSkip = 0;
    mTimeStampSkipStartTime = 0;
    mLastQueueTime = 0;
    mFrcOpened = false;
    mDecRealTimeFpsSend = false;
    mFpsFromPlayer = 0.0f;
    mFrameDecodedCount = 0.0f;
    mLastFrameDecodedCount = 0.0f;
    mDefaultFrameRate = 0.0f;
    mFrameRateByTimestamp = 0.0f;
    mFrameRateByBufferCount = 0.0f;
    mLastTimeStampRealTimeUs = 0;
    mStartTimeStampRealTimeUs = 0;
    mBufferInputStartTimeUs = 0;
    mLastClacFpsPtsDiff = 0;
    mCurrentTimeStampAbsDiff = 0;
    mMinTimeStampAbsDiff = 0;
    mMinTimeStampAbsNum = 0;

}
MiMediaCodec::~MiMediaCodec()
{
    ALOGV("MiMediaCodec::~miMediaCodec()");
    //sometimes first flush then stop
    mLastSendedFps = 0.0f;
    mTimeStampSkipValue = 700000;
    mTimeStampSkip = 0;
    mTimeStampSkipStartTime = 0;
    mLastQueueTime = 0;
    mFrcOpened = false;
    mDecRealTimeFpsSend = false;
    mFpsFromPlayer = 0.0f;
    mFrameDecodedCount = 0.0f;
    mLastFrameDecodedCount = 0.0f;
    mDefaultFrameRate = 0.0f;
    mFrameRateByTimestamp = 0.0f;
    mFrameRateByBufferCount = 0.0f;
    mLastTimeStampRealTimeUs = 0;
    mStartTimeStampRealTimeUs = 0;
    mBufferInputStartTimeUs = 0;
    mLastClacFpsPtsDiff = 0;
    mCurrentTimeStampAbsDiff = 0;
    mMinTimeStampAbsDiff = 0;
    mMinTimeStampAbsNum = 0;
}
static bool waitForSurfaceFlinger() {
    // TODO: replace this with better waiting logic in future, b/35253872
    int64_t waitStartTime = elapsedRealtime();
    sp<IServiceManager> sm = defaultServiceManager();
    const String16 name("SurfaceFlinger");
    const int SERVICE_WAIT_SLEEP_MS = 100;
    const int LOG_PER_RETRIES = 10;
    const int MAX_RETRY_TIME = 10;
    int retry = 0;
    while (sm->checkService(name) == nullptr) {
        retry++;
        if ((retry % LOG_PER_RETRIES) == 0) {
            ALOGI("MediaCodec Waiting for SurfaceFlinger, waited for %" PRId64 " ms",
                  elapsedRealtime() - waitStartTime);
        }
        if (retry > MAX_RETRY_TIME) {
            ALOGI("MediaCodec Waiting for SurfaceFlinger retry %d",retry);
            return false;
        }
        usleep(SERVICE_WAIT_SLEEP_MS * 1000);
    };
    int64_t totalWaited = elapsedRealtime() - waitStartTime;
    if (totalWaited > SERVICE_WAIT_SLEEP_MS) {
        ALOGI("MediaCodec Waiting for SurfaceFlinger took %" PRId64 " ms", totalWaited);
    }
    sp<ISurfaceComposer> mComposerService = waitForService<ISurfaceComposer>(name);
    if (mComposerService == nullptr) {
        ALOGI("MediaCodec Waiting for SurfaceFlinger is NULL");
        return false;
    }
    return true;
}
void MiMediaCodec::ResetSendFpsToDisplay(bool needSendFps, uint32_t id)
{
    ALOGV("MiMediaCodec::ResetSendFpsToDisplay");
    if (needSendFps && mLastSendedFps) {
        Parcel data;
        data.writeInt32(V2D_MSG_FRAMERATE);  //fps msg to display
        data.writeInt32(0);
        data.writeUint32((uint32_t)mLastSendedFps);
        data.writeFloat(mLastSendedFps);
        if(mFpsFromPlayer) {
            data.writeBool(true);
        } else {
            data.writeBool(false);
        }
        data.writeUint32(id);
        data.setDataPosition(0);
        ALOGI("send fps to display when stop, state:0 mLastSendedFps:%f id:%u",
            mLastSendedFps, id);
        int err = SurfaceComposerClient::setFpsVideoToDisplay(SEND_FPS_TO_DISPLAY, &data);
        if (err)
            ALOGI("err = %d",err);
        mLastSendedFps = 0.0f;
    }
    mDecRealTimeFpsSend = false;
    mFrameDecodedCount = 0.0f;
    mLastFrameDecodedCount = 0.0f;
    mDefaultFrameRate = 30.0f;
    mFrameRateByTimestamp = 0.0f;
    mFrameRateByBufferCount = 0.0f;
    mBufferInputStartTimeUs = 0;
    mLastTimeStampRealTimeUs = 0;
    mStartTimeStampRealTimeUs = 0;
    mLastClacFpsPtsDiff = 0;
    mCurrentTid = 0;

    mCurrentTimeStampAbsDiff = 0;
    mMinTimeStampAbsDiff = 0;
    mMinTimeStampAbsNum = 0;

}
void MiMediaCodec::SetFpsFromPlayer(float fpsfromplayer)
{
    mFpsFromPlayer = fpsfromplayer;
    ALOGV("MiMediaCodec::SetFpsFromPlayer The Fps from player is %f",mFpsFromPlayer);
}
void MiMediaCodec::VideoToDisplayRealtimeFramerate(const sp<AMessage> &msg, uint32_t id)
{
    int64_t TimeStamp = 0;
    int32_t state = 1;
    bool frcHasChanged = false;
    int frcStatus = 0;
    bool frcEnable = false;

    if (msg->findInt32("frcStatus", &frcStatus) && frcStatus) {
        frcEnable = true;
    }

    if (msg->findInt64("timeUs", &TimeStamp)) {
        if (android::base::GetBoolProperty("xm.videotodisplay.debug", false)) {
            ALOGI("TimeStamp = %" PRId64" TimeStampDiff = %" PRId64" mCurTid = %d gettid() = %d",
                TimeStamp, TimeStamp - mLastTimeStampRealTimeUs , mCurrentTid, gettid());
        }
    } else {
        ALOGV("Cannot find TimeStamp!");
        return;
    }

    if (mCurrentTid == 0) {
        mCurrentTid = gettid();
    } else if (mCurrentTid != gettid()) {
        ALOGV("Not the same thread");
        return;
    }

    //The time since the last queue exceeds 500ms
    if ((((systemTime() - mLastQueueTime)/1000000) >= 500) && mLastSendedFps) {
        ALOGI("send last fps");
        Parcel data;
        data.writeInt32(V2D_MSG_FRAMERATE);
        data.writeInt32(state);
        if (frcEnable) {
            data.writeUint32((uint32_t)(mLastSendedFps * 2));
            ALOGI("APP FRC Enable");
            data.writeFloat(mLastSendedFps * 2);
            ALOGI("send fps to display with mLastSendedFps:%f state:%d id:%u",
                mLastSendedFps * 2 , state, id);
        } else {
            data.writeUint32((uint32_t)mLastSendedFps);
            data.writeFloat(mLastSendedFps);
            ALOGI("send fps to display with mLastSendedFps:%f state:%d id:%u",
                mLastSendedFps, state, id);
        }
        data.writeBool(true);
        data.writeUint32(id);
        data.setDataPosition(0);
        int err = SurfaceComposerClient::setFpsVideoToDisplay(SEND_FPS_TO_DISPLAY, &data);
        if (err)
             ALOGI("err = %d",err);
    }
    mLastQueueTime = systemTime();

    if (frcEnable ^ mFrcOpened) {
        frcHasChanged = true;
        mFrcOpened = frcEnable;
    }

    //ALOGV("TimeStamp = %" PRId64"",TimeStamp);
    mFrameDecodedCount ++;
    mLastFrameDecodedCount ++;
    if (TimeStamp == 0 && mDecRealTimeFpsSend == true && mFrameDecodedCount > 10) {
        // bilibili don't call stop when play end
        ALOGI("VideoToDisplayRealtimeFramerate send fps to display when stop");
        ResetSendFpsToDisplay(true, id);
        return;
    }
    //directly to SurfaceFlinger if fps from player
    if (mFpsFromPlayer) {
        if (!mDecRealTimeFpsSend || frcHasChanged) {
            Parcel data;
            data.writeInt32(V2D_MSG_FRAMERATE);  //fps msg to display
            data.writeInt32(state);
            if (frcEnable) {
                data.writeUint32((uint32_t)(mFpsFromPlayer * 2));
                ALOGI("APP FRC Enable");
                data.writeFloat(mFpsFromPlayer * 2);
                ALOGI("send fps to display with mFpsFromPlayer:%f state:%d id:%u",
                    mFpsFromPlayer * 2 , state, id);
            } else {
                data.writeUint32((uint32_t)mFpsFromPlayer);
                data.writeFloat(mFpsFromPlayer);
                ALOGI("send fps to display with mFpsFromPlayer:%f state:%d id:%u",
                    mFpsFromPlayer, state, id);
            }
            data.writeBool(true);
            data.writeUint32(id);
            data.setDataPosition(0);
            int err = SurfaceComposerClient::setFpsVideoToDisplay(SEND_FPS_TO_DISPLAY, &data);
            if (err)
                 ALOGI("err = %d",err);
            mDefaultFrameRate = mFpsFromPlayer;
            mLastSendedFps = mDefaultFrameRate;
            mDecRealTimeFpsSend = true;
            mLastFrameDecodedCount = 0;
        }
        return;
    }
    if (TimeStamp == 0) {
        ALOGI("The CSD is not calculated");
        mLastFrameDecodedCount = 0;
        return;
    }
    if (mLastFrameDecodedCount == 1) {
        if((TimeStamp > mLastTimeStampRealTimeUs) && mLastTimeStampRealTimeUs) {
            //first diff for the first fps
            mLastClacFpsPtsDiff = TimeStamp - mLastTimeStampRealTimeUs;
            mBufferInputStartTimeUs = ALooper::GetNowUs();
            mLastTimeStampRealTimeUs = TimeStamp;
            mStartTimeStampRealTimeUs = TimeStamp;
        } else {
            mLastTimeStampRealTimeUs = TimeStamp;
            mLastFrameDecodedCount = 0;
            return;
        }
    }

    if (TimeStamp > 0) {
        //sometimes douyin change program, timestamp not start from 0
        if (abs(TimeStamp - mLastTimeStampRealTimeUs) >= mTimeStampSkipValue) {
           ResetSendFpsToDisplay(false, id);
           mTimeStampSkip++;
           if (!mTimeStampSkipStartTime) {
               mTimeStampSkipStartTime = ALooper::GetNowUs();
           }
           // TimeStampSkip 5times in 5s, will set the threshold to 1s
           if ((mTimeStampSkip > 5) && ((ALooper::GetNowUs()-mTimeStampSkipStartTime) > 5000000)) {
               ALOGI("reset mTimeStampSkipValue to 1s, mTimeStampSkip= %"\
                    PRId64 " ALooper::GetNowUs() - mTimeStampSkipStartTimeUs = %" PRId64 "",
                    mTimeStampSkip, (ALooper::GetNowUs() - mTimeStampSkipStartTime));
               mTimeStampSkipValue += 1000000;
               mTimeStampSkip = 0;
               mTimeStampSkipStartTime = 0;
           }
           ALOGI("The TimeStamp has skip, restart calc fps");
           return;
        }

        //find the min timstamp diff
        mCurrentTimeStampAbsDiff = std::abs(TimeStamp - mLastTimeStampRealTimeUs);
        if (mMinTimeStampAbsDiff == 0) {
            mMinTimeStampAbsDiff = mCurrentTimeStampAbsDiff;
        } else {
            if (mCurrentTimeStampAbsDiff > mMinTimeStampAbsDiff) {
                //pts diff is 1ms
                if (1000 == (mCurrentTimeStampAbsDiff - mMinTimeStampAbsDiff)) {
                    mMinTimeStampAbsNum++;
                }
            } else if (mCurrentTimeStampAbsDiff == mMinTimeStampAbsDiff) {
                mMinTimeStampAbsNum++;
            } else {
                mMinTimeStampAbsDiff = mCurrentTimeStampAbsDiff;
                if (1000 == (mMinTimeStampAbsDiff - mCurrentTimeStampAbsDiff)) {
                    mMinTimeStampAbsNum++;
                } else {
                    mMinTimeStampAbsNum = 1;
                }
            }
        }
        if (android::base::GetBoolProperty("xm.videotodisplay.debug", false)) {
            ALOGI("mMinDiff = %" PRId64 " TimeStampDiff = %" PRId64 " mMinAbsNum = %d",
                mMinTimeStampAbsDiff, TimeStamp-mLastTimeStampRealTimeUs, mMinTimeStampAbsNum);
        }
        if (((TimeStamp > mLastTimeStampRealTimeUs) && (((((TimeStamp -
            mLastTimeStampRealTimeUs) == mLastClacFpsPtsDiff) || mLastClacFpsPtsDiff == 0)
            && (mLastFrameDecodedCount >= (mDefaultFrameRate * 2))) ||
           (!mDecRealTimeFpsSend && (mLastFrameDecodedCount >= (mDefaultFrameRate * 3)))
            )) || frcHasChanged) {
            uint64_t PtsDiff = TimeStamp - mStartTimeStampRealTimeUs;
            uint64_t nowUS = ALooper::GetNowUs();
            uint64_t SystemTimeDiff =  nowUS - mBufferInputStartTimeUs;
            float mFrameRateByTimestampDiff = 0.0;
            mLastFrameDecodedCount = mLastFrameDecodedCount - 1;
            if (PtsDiff != 0) {
                mFrameRateByTimestamp = (mLastFrameDecodedCount * 1000 * 1000) / PtsDiff;
            }
            if (android::base::GetBoolProperty("xm.videotodisplay.debug", false)) {
            ALOGI("TimeStamp =%" PRId64 " mFrameRateByTimestamp = %f mLastTimeStampRealTimeUs =%" \
               PRId64 " pts diff =%" PRId64 " mLastClacFpsPtsDiff = %" PRId64 "", TimeStamp,
               mFrameRateByTimestamp, mLastTimeStampRealTimeUs, PtsDiff, mLastClacFpsPtsDiff);
            }

            if (mMinTimeStampAbsDiff != 0) {
                mFrameRateByTimestampDiff = (float) 1000000 / mMinTimeStampAbsDiff;
                ALOGV("mFrameRateByTimestampDiff with timediff = %f ",
                    mFrameRateByTimestampDiff);
            }

            if (//(abs((int64_t)mFrameRateByTimestampDiff - (int64_t)mFrameRateByTimestamp) < 20) &&
                mMinTimeStampAbsNum >= 1) {
                if (android::base::GetBoolProperty("xm.videotodisplay.debug", false)) {
                    ALOGI("mFrameRateByTimestamp = mFrameRateByTimestampDiff = %f",
                        mFrameRateByTimestampDiff);
                }
               //use the fps calc by timestamp diff
                mFrameRateByTimestamp = mFrameRateByTimestampDiff;
            }

            // Specific value correction
            if (27 <= mFrameRateByTimestamp && mFrameRateByTimestamp <= 33) {
                ALOGV("mFrameRateByTimestamp = %f -> 30", mFrameRateByTimestamp);
                mFrameRateByTimestamp = 30;
            } else if (55 <= mFrameRateByTimestamp && mFrameRateByTimestamp <= 65) {
                ALOGV("mFrameRateByTimestamp = %f  -> 60", mFrameRateByTimestamp);
                mFrameRateByTimestamp = 60;
            } else if (mFrameRateByTimestamp > 960) {
                ALOGV("mFrameRateByTimestamp = %f -> 960", mFrameRateByTimestamp);
                mFrameRateByTimestamp = 960;
            }

            mFrameRateByBufferCount = (mLastFrameDecodedCount * 1000 * 1000)/SystemTimeDiff;
            if (android::base::GetBoolProperty("xm.videotodisplay.debug", false)) {
                ALOGI("TimeStamp =%" PRId64 \
                " mFrameRateByBufferCount = %f FrameDecodedCount=%f system time diff =%" PRId64 \
                "", TimeStamp, mFrameRateByBufferCount, mLastFrameDecodedCount,SystemTimeDiff);
            }
            mBufferInputStartTimeUs = nowUS;
            mLastFrameDecodedCount = 0;
            mStartTimeStampRealTimeUs = TimeStamp;
        } else {
            mLastTimeStampRealTimeUs = TimeStamp;
            //FPS has not been updated
            return;
        }
        mLastTimeStampRealTimeUs = TimeStamp;
    }
    if (mFrameRateByBufferCount != 0 || mFrameRateByTimestamp != 0) {
        if ((120 <= mFrameRateByTimestamp && mFrameRateByTimestamp <= 480 &&
            abs(mFrameRateByBufferCount - mFrameRateByTimestamp) >= 30) ||
            (480 < mFrameRateByTimestamp && mFrameRateByTimestamp <= 960 &&
            abs(mFrameRateByBufferCount - mFrameRateByTimestamp) >= 60)) {
            //send fps by buffer count
            if (waitForSurfaceFlinger() && ((mFrameRateByBufferCount && !mDecRealTimeFpsSend)
                || (abs(mFrameRateByBufferCount - mDefaultFrameRate) >= 1) || frcHasChanged)) {
                Parcel data;
                data.writeInt32(V2D_MSG_FRAMERATE);  //fps msg to display
                data.writeInt32(state);
                if (frcEnable) {
                    data.writeUint32((uint32_t)(mFrameRateByBufferCount * 2));
                    ALOGI("APP FRC Enable");
                    data.writeFloat(mFrameRateByBufferCount * 2.0);
                    ALOGI("send fps to display with mFrameRateByBufferCount:%f state:%d id:%u",
                        mFrameRateByBufferCount * 2, state, id);
                } else {
                    data.writeUint32((uint32_t)mFrameRateByBufferCount);
                    data.writeFloat(mFrameRateByBufferCount);
                    ALOGI("send fps to display with mFrameRateByBufferCount:%f state:%d id:%u",
                        mFrameRateByBufferCount, state, id);
                }
                data.writeBool(false);
                data.writeUint32(id);
                data.setDataPosition(0);
                int err = SurfaceComposerClient::setFpsVideoToDisplay(SEND_FPS_TO_DISPLAY, &data);
                if (err)
                    ALOGI("err = %d",err);
                mDecRealTimeFpsSend = true;
                mDefaultFrameRate = mFrameRateByBufferCount;
            }
        } else {
            //send fps by timestamp
            if (waitForSurfaceFlinger() && ((mFrameRateByTimestamp && !mDecRealTimeFpsSend) ||
                (abs(mFrameRateByTimestamp - mDefaultFrameRate) >= 1) || frcHasChanged)) {
                Parcel data;
                data.writeInt32(V2D_MSG_FRAMERATE);  //fps msg to display
                data.writeInt32(state);
                if (frcEnable) {
                    data.writeUint32((uint32_t)(mFrameRateByTimestamp * 2));
                    ALOGI("APP FRC Enable");
                    data.writeFloat(mFrameRateByTimestamp * 2);
                    ALOGI("send fps to display with mFrameRateByTimestamp:%f state:%d id:%u",
                        mFrameRateByTimestamp * 2, state, id);
                } else {
                    data.writeUint32((uint32_t)mFrameRateByTimestamp);
                    data.writeFloat(mFrameRateByTimestamp);
                    ALOGI("send fps to display with mFrameRateByTimestamp:%f state:%d id:%u",
                        mFrameRateByTimestamp, state, id);
                }
                data.writeBool(false);
                data.writeUint32(id);
                data.setDataPosition(0);
                int err = SurfaceComposerClient::setFpsVideoToDisplay(SEND_FPS_TO_DISPLAY, &data);
                if (err)
                    ALOGI("err = %d",err);
                mDecRealTimeFpsSend = true;
                mDefaultFrameRate = mFrameRateByTimestamp;
            }
        }
        //for stop and release
        mLastSendedFps = mDefaultFrameRate;
    }
}

//send dolby vision playback to display
void MiMediaCodec::SendDoviPlaybackToDisplay(int status){
    Parcel data;
    data.writeInt32(V2D_MSG_DV);  //dolby vision message
    data.writeInt32(status);  //1 playback,2 stop/release
    data.setDataPosition(0);
    ALOGE("send dolby vision to display status is %d", status);
    int err = SurfaceComposerClient::setFpsVideoToDisplay(SEND_FPS_TO_DISPLAY, &data);
    if (err)
        ALOGE("send dv mime to display  err = %d",err);
}

extern "C" IMiMediaCodecStub* create() {
    return new MiMediaCodec;
}
extern "C" void destroy(IMiMediaCodecStub* impl) {
    delete impl;
}
}
}
