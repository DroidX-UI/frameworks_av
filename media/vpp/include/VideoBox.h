#ifndef VIDEO_BOX_VPP_IMPL_H
#define VIDEO_BOX_VPP_IMPL_H

#include <memory>
#include <vector>

#include <media/stagefright/foundation/AHandler.h>
#include <utils/Vector.h>
#include <media/stagefright/CodecBase.h>
#include <utils/String16.h>
#include <media/stagefright/foundation/AString.h>
#include <media/MediaCodecBuffer.h>

namespace android {
namespace videobox {

enum {
	kPortIndexInput         = 0,
	kPortIndexOutput        = 1,
};

class IVideoBoxStub {
public:
	virtual ~IVideoBoxStub() {}
	virtual bool initVideobox(const AString &name,bool isvideo,sp<CodecBase> codec);
	virtual bool configVideobox(const sp<AMessage> format);
	virtual bool readVideoBox(int32_t portIndex,const sp<MediaCodecBuffer> &buffer);
	virtual bool getRealtimeFramerate(const sp<AMessage> &msg);
	virtual bool isFrcEnable() { return false; };
};

class VideoBox :public IVideoBoxStub {
public:
	VideoBox();
	virtual ~VideoBox();
	virtual bool initVideobox(const AString &name,bool isvideo,sp<CodecBase> codec);
	virtual bool configVideobox(const sp<AMessage> format);
	bool videoBoxWhiteList(String16 name,int flag);
	virtual bool readVideoBox(int32_t portIndex,const sp<MediaCodecBuffer> &buffer);
	void bypassFrc();
	void disableFrc();
	void bypassAie();
	void bypassAis();
	void disableAis();
	void enableFrc();
	void enableAie();
	void enableAis();
	void getSupportedProps();
	void readVppProps();
	void readAppCtrlProps();
	void readVppFrcProps();
	void readVppAieProps();
	void readVppAisProps();
	void initVppDefaultValue();
	bool checkVideoBoxFeature(const sp<AMessage> format);
	virtual bool getRealtimeFramerate(const sp<AMessage> &msg);
	void getInterpolated(int64_t timestamp);
	void setReadVppProps();
	bool isFrcEnable();

private:
	sp<CodecBase> mCodec;
	bool mVppContext;
	bool mFlagVppFrcUse;
	bool mFlagVppAieUse;
	bool mFlagVppAisUse;
	int64_t mFrameDecodeCnt;
	uint32_t mFrameRate;
	int64_t mFrameRateRealTime;
	uint32_t mOperatingRate;
	bool mFlagDisableEnable;
	int64_t mLastTimeStamp;
	int64_t mLastTimeStampRealTime;
	int64_t mLastTimeStampInterp;
	uint32_t mFrameDecoded;
	uint32_t mFrameDecodedFPS;
	uint32_t mFrameInterp;
	bool mLastFrameInterp;
	String16 mClientName;
	uint32_t mVppEnableFlags;
	bool mFlagVppFrcSupport;
	bool mFlagVppAieSupport;
	bool mFlagVppAisSupport;
	bool mIsVppActive;
	bool mIsFRCEnabled;
	bool mIsAIEEnabled;
	bool mIsAISEnabled;
	bool mIsVppEnabled;
	bool mIsHevc;
	uint32_t mProfile;
	bool mVppEnabled;
	uint32_t mPlatFormValue;

	struct vppVideoResolution {
		uint32_t width;
		uint32_t height;
	}mVppVideoResolution;
};

extern "C" IVideoBoxStub* create();
extern "C" void destroy(IVideoBoxStub* impl);
}
}

#endif
