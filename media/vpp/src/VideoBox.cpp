//#define LOG_NDEBUG 0
#define LOG_TAG "VideoBox"
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

#include "VideoBox.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#ifndef __ANDROID_VNDK___
    #include <binder/IPermissionController.h>
#endif

#define AIE 1
#define FRC 2
#define AIS 3

#define VPP_CTL "debug.media.vpp.enable"
// app control prop frc
#define VPP_APP_CONTROL_FRC "debug.media.video.frc"

// app control prop aie
#define VPP_APP_CONTROL_AIE "debug.media.video.vpp"

// app control prop ais
#define VPP_APP_CONTROL_AIS "debug.media.video.ais"

// platform prop 5:8450,4:8350,3:8250,2:mtk,1:others
#define VPP_PLATFORM_CHIPSET "debug.media.video.chipset"

#define FLAG_VPP_FRC_ENALBE 0x01
#define FLAG_VPP_FRC_BYPASS 0x10

#define FLAG_VPP_AIE_ENALBE 0x02
#define FLAG_VPP_AIE_BYPASS 0x20

#define FLAG_VPP_AIS_ENALBE 0x04
#define FLAG_VPP_AIS_BYPASS 0x40

// supoorted feature
#define FLAG_FRC_SUPPORT "debug.config.media.video.frc.support"
#define FLAG_AIE_SUPPORT "debug.config.media.video.aie.support"
#define FLAG_AIS_SUPPORT "debug.config.media.video.ais.support"

#define FLAG_VPP_FRC_ENALBE 0x01
#define FLAG_VPP_FRC_BYPASS 0x10

//max supported resolution
#define FRC_HW_MAX_WIDTH 3840
#define FRC_HW_MAX_HEIGHT 2176
#define FRC_MAX_WIDTH 1920
#define FRC_MAX_HEIGHT 1280
#define FRC_MIN_WIDTH 399
#define FRC_MIN_HEIGHT 299
#define FRC_MAX_FPS 33

#define AIS_MAX_WIDTH 1280
#define AIS_MAX_HEIGHT 720
#define AIS_MIN_WIDTH 100
#define AIS_MIN_HEIGHT 100
#define AIS_MAX_FPS 33

#define AIE_MAX_WIDTH 4096
#define AIE_MAX_HEIGHT 2176
//VPP
#define VPP_MODE_VENDOR_EXT "vendor.qti-ext-vpp.mode"
#define VPP_MODE_KEY_VALUE_OFF "HQV_MODE_OFF"
#define VPP_MODE_KEY_VALUE_AUTO "HQV_MODE_AUTO"
#define VPP_MODE_KEY_VALUE_MANUAL "HQV_MODE_MANUAL"
#define VPP_MODE_VENDOR_EXT_BYPASS "vendor.qti-ext-vpp-bypass.enable"

// AIE
#define VPP_AIE_MODE_VENDOR_EXT "vendor.qti-ext-vpp-aie.mode"
#define VPP_AIE_HUE_MODE_VENDOR_EXT "vendor.qti-ext-vpp-aie.hue-mode"
#define VPP_AIE_CADE_LEVEL_VENDOR_EXT "vendor.qti-ext-vpp-aie.cade-level"
#define VPP_AIE_LTM_LEVEL_VENDOR_EXT "vendor.qti-ext-vpp-aie.ltm-level"
#define VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT "vendor.qti-ext-vpp-aie.ltm-sat-gain"
#define VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT "vendor.qti-ext-vpp-aie.ltm-sat-offset"
#define VPP_AIE_LTM_ACE_STR_VENDOR_EXT "vendor.qti-ext-vpp-aie.ltm-ace-str"
#define VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT "vendor.qti-ext-vpp-aie.ltm-ace-brightness-high"
#define VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT "vendor.qti-ext-vpp-aie.ltm-ace-brightness-low"

#define VPP_AIE_MODE_KEY_VALUE_OFF "HQV_MODE_OFF"
#define VPP_AIE_MODE_KEY_VALUE_AUTO "HQV_MODE_AUTO"
#define VPP_AIE_MODE_KEY_VALUE_MANUAL "HQV_MODE_MANUAL"
#define VPP_AIE_HUE_MODE_KEY_VALUE_OFF "HQV_HUE_MODE_OFF"
#define VPP_AIE_HUE_MODE_KEY_VALUE_ON "HQV_HUE_MODE_ON"

// FRC
#define VPP_FRC_MODE_VENDOR_EXT "vendor.qti-ext-vpp-frc.mode"
#define VPP_FRC_LEVEL_VENDOR_EXT "vendor.qti-ext-vpp-frc.level"
#define VPP_FRC_INTERP_VENDOR_EXT "vendor.qti-ext-vpp-frc.interp"

#define VPP_FRC_MODE_KEY_VALUE_OFF "FRC_MODE_OFF"
#define VPP_FRC_MODE_KEY_VALUE_FRC_SMOOTH "FRC_MODE_SMOOTH_MOTION"
#define VPP_FRC_MODE_KEY_VALUE_SLOMO "FRC_MODE_SLOMO"

#define VPP_FRC_LEVEL_KEY_VALUE_OFF "FRC_LEVEL_OFF"
#define VPP_FRC_LEVEL_KEY_VALUE_LOW "FRC_LEVEL_LOW"
#define VPP_FRC_LEVEL_KEY_VALUE_MED "FRC_LEVEL_MEDIUM"
#define VPP_FRC_LEVEL_KEY_VALUE_HIGH "FRC_LEVEL_HIGH"

#define VPP_FRC_INTERP_KEY_VALUE_1X "FRC_INTERP_1X"
#define VPP_FRC_INTERP_KEY_VALUE_2X "FRC_INTERP_2X"
#define VPP_FRC_INTERP_KEY_VALUE_3X "FRC_INTERP_3X"
#define VPP_FRC_INTERP_KEY_VALUE_4X "FRC_INTERP_4X"

#define VPP_FRC_INTERP_KEY_FRAME_COPY_INPUT "vendor.qti-ext-vpp-frc.frame_copy_input"
#define VPP_FRC_INTERP_KEY_FRAME_COPY_ON_FALLBACK "vendor.qti-ext-vpp-frc.frame_copy_on_fallback"


//AIS
#define VPP_AIS_MODE_VENDOR_EXT "vendor.qti-ext-vpp-ais.mode"
#define VPP_AIS_MODE_VALUE_AUTO "AIS_AUTO"
#define VPP_AIS_BYPASS_VALUE_AUTO "vendor.qti-ext-vpp-ais.bypass"
#define VPP_AIS_DEMO_SIDE_VENDOR_EXT "vendor.qti-ext-vpp-demo.process-side"
#define VPP_AIS_DEMO_PERCENT_VENDOR_EXT "vendor.qti-ext-vpp-demo.process-percent"
#define VPP_AIS_DEMO_PRO_SIDE "PROCESS_LEFT"
#define VPP_AIS_DEMO_PRO_PERCENT "50"

//Lazzy
#define VPP_LAZZY_INIT_VENDOR_EXT "vendor.qti-ext-vpp-lazy-init.on"

#define VIDEOBOX_MAX_STRINGNAME_SIZE 128

#define VIDEOBOX_AIE_WHITELIST_SIZE (sizeof(VppWhiteListAIE) / sizeof(AString))
#define VIDEOBOX_FRC_WHITELIST_SIZE (sizeof(VppWhiteListFRC) / sizeof(AString))
#define VIDEOBOX_AIS_WHITELIST_SIZE (sizeof(VppWhiteListAIS) / sizeof(AString))

static int32_t mVideoBoxHandle = 0;

namespace android {
namespace videobox {

typedef enum OMX_VIDEO_HEVCPROFILETYPE {
        OMX_VIDEO_HEVCProfileUnknown      = 0x0,
        OMX_VIDEO_HEVCProfileMain         = 0x1,
        OMX_VIDEO_HEVCProfileMain10       = 0x2,
        OMX_VIDEO_HEVCProfileMainStill    = 0x04,
        // Main10 profile with HDR SEI support.
        OMX_VIDEO_HEVCProfileMain10HDR10  = 0x1000,
        OMX_VIDEO_HEVCProfileMain10HDR10Plus = 0x2000,
        OMX_VIDEO_HEVCProfileMax          = 0x7FFFFFFF
} OMX_VIDEO_HEVCPROFILETYPE;

typedef enum PLATFORM_CHIPSET {
        CHIPSET_OTHERS = 0,
        CHIPSET_NOTSUPPORT,
        CHIPSET_MTK,
        CHIPSET_KONA,
        CHIPSET_LAHAINA,
        CHIPSET_TARO,
        CHIPSET_KALAMA,
        CHIPSET_END
} PLATFORM_CHIPSET;

struct AString VppWhiteListAIE [] = {
    [0] = "com.miui.gallery",//mi gallery
    [1] = "com.miui.video",//mi video
    [2] = "com.qiyi.video",//iqiyi
    [3] = "tv.pps.mobile",//iqiyi jisu
    [4] = "air.tv.douyu.android",//douyu
    [5] = "com.tencent.qqlive",//tecent video
    [6] = "com.tencent.qqsports",//tecent sports
    [7] = "com.duowan.kiwi",//huya
    [8] = "com.youku.phone",//youku
    [9] = "com.cmcc.cmvideo",//migu
    [10] = "com.tencent.weishi",//weishi
    [11] = "tv.danmaku.bili",//bilibili
    [12] = "com.sina.weibo",//weibo
    [13] = "com.mxtech.videoplayer.ad",//mxplayer
    [14] = "com.ss.android.article.video",//xiguan video
    [15] = "com.miui.videoplayer",//mi video global
    [16] = "com.qiyi.i18n",//iqiyi global
    [17] = "com.ss.android.article.news",//jin ri tou tiao
    [18] = "com.google.android.youtube",//youtube
    [19] = "com.amazon.avod.thirdpartyclient",//prime video
    [20] = "in.startv.hotstar",//hotstar
    [21] = "org.videolan.vlc",//vlc
    [22] = "com.google.android.apps.photos",//google photos
    [23] = "com.gallery.player",//mi video no CN
    [24] = "com.netflix.mediaclient",//netflix
    [25] = "com.iqiyi.i18n", // iqiyi global
    [26] = "com.miui.mediaviewer", //xiaomi gallery
};

struct AString VppWhiteListFRC [] = {
    [0] = "com.miui.gallery",//mi gallery
    [1] = "com.miui.video",//mi video
    [2] = "com.qiyi.video",//iqiyi
    [3] = "tv.pps.mobile",//iqiyi jisu
    [4] = "air.tv.douyu.android",//douyu
    [5] = "com.youku.phone",//youku
    [6] = "com.cmcc.cmvideo",//migu
    [7] = "tv.danmaku.bili",//bilibili
    [8] = "com.mxtech.videoplayer.ad",//mxplayer
    [9] = "com.ss.android.article.video",//xiguan video
    [10] = "com.miui.videoplayer",//mi video global
    [11] = "com.qiyi.i18n",//iqiyi global
    [12] = "com.ss.android.article.news",//jin ri tou tiao
    [13] = "com.google.android.youtube",//youtube
    [14] = "com.amazon.avod.thirdpartyclient",//prime video
    [15] = "in.startv.hotstar",//hotstar
    [16] = "org.videolan.vlc",//vlc
    [17] = "com.google.android.apps.photos",//google photos
    [18] = "com.gallery.player",//mi video no CN
    [19] = "com.netflix.mediaclient",//netflix
    [20] = "com.iqiyi.i18n", // iqiyi global
    [21] = "com.miui.mediaviewer", //xiaomi gallery
    [22] = "com.tencent.qqlive",//tecent video
};

struct AString VppWhiteListAIS [] = {
    [0] = "com.miui.gallery",//mi gallery
    [1] = "com.miui.video",//mi video
    [2] = "com.qiyi.video",//iqiyi
    [3] = "tv.pps.mobile",//iqiyi jisu
    [4] = "air.tv.douyu.android",//douyu
    [5] = "com.tencent.qqlive",//tecent video
    [6] = "com.tencent.qqsports",//tecent sports
    [7] = "com.duowan.kiwi",//huya
    [8] = "com.youku.phone",//youku
    [9] = "com.cmcc.cmvideo",//migu
    [10] = "com.tencent.weishi",//weishi
    [11] = "tv.danmaku.bili",//bilibili
    [12] = "com.sina.weibo",//weibo
    [13] = "com.mxtech.videoplayer.ad",//mxplayer
    [14] = "com.ss.android.article.video",//xiguan video
    [15] = "com.miui.videoplayer",//mi video global
    [16] = "com.qiyi.i18n",//iqiyi global
    [17] = "com.ss.android.article.news",//jin ri tou tiao
    [18] = "com.google.android.youtube",//youtube
    [19] = "com.amazon.avod.thirdpartyclient",//prime video
    [20] = "in.startv.hotstar",//hotstar
    [21] = "org.videolan.vlc",//vlc
    [22] = "com.google.android.apps.photos",//google photos
    //[23] = "com.tencent.mm",//wechat
    //[24] = "com.smile.gifmaker",//kuaishou
    [23] = "com.gallery.player",//mi video no CN
    [24] = "com.netflix.mediaclient",//netflix
    [25] = "com.iqiyi.i18n", // iqiyi global
    [26] = "com.miui.mediaviewer", //xiaomi gallery
};

int32_t read_prop(const char *pc)
{
    char value[PROPERTY_VALUE_MAX];
    value[0] = '\0';

    if (!pc)
        return -1;

    property_get(pc, value, NULL);
    return atoi(value);
}

bool isHDR(const sp<AMessage> &format) {
    uint32_t standard, range, transfer;
    if (!format->findInt32("color-standard", (int32_t*)&standard)) {
        standard = 0;
    }
    if (!format->findInt32("color-range", (int32_t*)&range)) {
        range = 0;
    }
    if (!format->findInt32("color-transfer", (int32_t*)&transfer)) {
        transfer = 0;
    }
    ALOGI("isHDR: standard =%d,transfer = %d",standard,transfer);
    return standard == ColorUtils::kColorStandardBT2020 || \
            (transfer == ColorUtils::kColorTransferST2084 ||
            transfer == ColorUtils::kColorTransferHLG);
}

VideoBox::VideoBox()
    : mVppContext(false),
      mFlagVppFrcUse(false),
      mFlagVppAieUse(false),
      mFlagVppAisUse(false),
      mFrameDecodeCnt(0),
      mFrameRate(0),
      mFrameRateRealTime(0),
      mOperatingRate(0),
      mFlagDisableEnable(false),
      mLastTimeStamp(0),
      mLastTimeStampRealTime(0),
      mLastTimeStampInterp(0),
      mFrameDecoded(0),
      mFrameDecodedFPS(0),
      mFrameInterp(0),
      mLastFrameInterp(false),
      mVppEnableFlags(0),
      mFlagVppFrcSupport(false),
      mFlagVppAieSupport(false),
      mFlagVppAisSupport(false),
      mIsVppActive(false),
      mIsFRCEnabled(false),
      mIsAIEEnabled(false),
      mIsAISEnabled(false),
      mIsVppEnabled(false),
      mIsHevc(false),
      mProfile(0),
      mVppEnabled(false),
      mPlatFormValue(0){
    mVppVideoResolution.width = 0;
    mVppVideoResolution.height= 0;
    const uid_t uid = IPCThreadState::self()->getCallingUid();
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("permission"));
    if (binder == 0) {
        ALOGE("Cannot get permission service");
        return;
    }

    sp<IPermissionController> permCtrl = interface_cast<IPermissionController>(binder);
    Vector<String16> packages;

    permCtrl->getPackagesForUid(uid, packages);

    if (packages.isEmpty()) {
        ALOGE("No packages for calling UID");
    } else {
        mClientName = packages[0];
    }

    ALOGI("VideoBox::VideoBox");
}

VideoBox::~VideoBox()
{
    if (mFlagVppFrcSupport && (mFrameInterp > 0))
        ALOGI("video clip interpolated = %d%%,%d--%d",(mFrameInterp * 100)/mFrameDecoded,mFrameInterp,mFrameDecoded);

    mFrameDecoded = 0;
    mFrameRateRealTime = 0;
    mLastTimeStampRealTime = 0;
    mLastTimeStampInterp = 0;
    mLastFrameInterp = false;
    mFrameInterp = 0;

    mVideoBoxHandle --;
    if (mVideoBoxHandle < 0)
        mVideoBoxHandle = 0;

    ALOGI("VideoBox::~VideoBox");
}

bool VideoBox::initVideobox(const AString &name,bool isvideo,sp<CodecBase> codec)
{
    ALOGI("initVideobox");
    char value[PROPERTY_VALUE_MAX];
    value[0] = '\0';
    mCodec = codec;
    if ((property_get(VPP_CTL, value, "false")) \
            && (!strcmp("0", value) || !strcmp("false", value))) {
        ALOGI("initVideobox return false for videobox is not enabled");
        mVppEnabled = false;
        return false;
    }

    if (isvideo && (((name.find("decoder", 0) > 0) && ((name.find("qti", 0) > 0) || (name.find("qcom", 0) > 0))) || \
            ((name.find("DECODER",0) > 0) && (name.find("MTK",0)  > 0))) && (!(name.find("HEIF", 0) > 0)) && \
        (!(name.find("secure", 0) > 0))) {
        ALOGI("initVideobox going %s",name.c_str());
    } else {
        ALOGI("initVideobox return false for not support");
        mVppEnabled = false;
        return false;
    }

    property_get(VPP_PLATFORM_CHIPSET, value, "0");
    if (!strcmp("0", value) || !strcmp("1", value)) {
        ALOGI("initVideobox return false for platform is not support vpp");
        mVppEnabled = false;
        return false;
    } else if (!strcmp("2", value)) {
        ALOGI("initVideobox platform is MTK");
        mPlatFormValue = atoi(value);
    } else if (!strcmp("3", value)) {
        ALOGI("initVideobox return false for platform is kona");
        mVppEnabled = false;
        return false;
    } else if (!strcmp("4", value)) {
        ALOGI("initVideobox platform is Lahaina");
        mPlatFormValue = atoi(value);
    } else if (!strcmp("5", value)) {
        ALOGI("initVideobox platform is Taro");
        mPlatFormValue = atoi(value);
    } else if (!strcmp("6", value)) {
        ALOGI("initVideobox platform is Kalama");
        mPlatFormValue = atoi(value);
    } else {
        ALOGI("initVideobox return false for platform is not defined");
        mVppEnabled = false;
        return false;
    }

    if ((name.find("hevc", 0) > 0) || (name.find("HEVC", 0) > 0)) {
        mIsHevc = true;
    }

    if ((name.find("low_latency", 0) > 0) && (mPlatFormValue != CHIPSET_KALAMA)){
        ALOGI("initVideobox return false for low latency is not support");
        mVppEnabled = false;
        return false;
    }

    if (!name.compare("c2.qti.avc.decoder") || !name.compare("c2.qti.hevc.decoder") || \
            !name.compare("c2.qti.vp9.decoder") || !name.compare("c2.qti.mpeg2.decoder") || \
            !name.compare("c2.qti.av1.decoder") || !name.compare("OMX.qcom.video.decoder.avc") || \
            !name.compare("OMX.qcom.video.decoder.hevc") || \
            !name.compare("OMX.MTK.VIDEO.DECODER.AVC") || \
            !name.compare("OMX.MTK.VIDEO.DECODER.HEVC") || \
            !name.compare("c2.qti.avc.decoder.low_latency") || \
            !name.compare("c2.qti.hevc.decoder.low_latency") || \
            !name.compare("c2.qti.vp9.decoder.low_latency") || \
            !name.compare("c2.qti.av1.decoder.low_latency") || \
            !name.compare("OMX.qcom.video.decoder.avc.low_latency") || \
            !name.compare("OMX.qcom.video.decoder.hevc.low_latency")) {
        getSupportedProps();
        if (mFlagVppFrcUse) {
            auto ret = videoBoxWhiteList(mClientName,FRC);
            if (ret == false) {
                mFlagVppFrcSupport = false;
            } else {
                mVppEnabled = true;
                mFlagVppFrcSupport = true;
            }
        }

        if (mFlagVppAieUse) {
            auto ret = videoBoxWhiteList(mClientName,AIE);
            if (ret == false) {
                mFlagVppAieSupport = false;
            } else {
                mVppEnabled = true;
                mFlagVppAieSupport = true;
            }
        }

        if (mFlagVppAisUse) {
            auto ret = videoBoxWhiteList(mClientName,AIS);
            if (ret == false) {
                mFlagVppAisSupport = false;
            } else {
                mVppEnabled = true;
                mFlagVppAisSupport = true;
            }
        }

    }

    if (mVppEnabled == true) {
        ALOGI("initVideobox true");
    }

    return mVppEnabled;
}

bool VideoBox::configVideobox(const sp<AMessage> format)
{
    ALOGI("configVideobox");
    int32_t frameRateInt;
    float frameRateFloat;

    int32_t thumbnailMode = 0;
    format->findInt32("thumbnail-mode", &thumbnailMode);
    int32_t heicMode = 0;
    format->findInt32("vendor.qti-ext-dec-heif-mode.value", &heicMode);
    int32_t frameMode = 0;
    format->findInt32("frame-thumbnail-mode", &frameMode);
    int32_t VideoAnalysisMode = 0;
    format->findInt32("video-analysis-mode", &VideoAnalysisMode);
    int32_t lowLatency = 0;
    format->findInt32("low-latency", &lowLatency);
    int32_t rateInt = -1;
    float rateFloat = -1;
    if (!format->findFloat("operating-rate", &rateFloat)) {
        format->findInt32("operating-rate", &rateInt);
        rateFloat = (float)rateInt;
    }

    if (mPlatFormValue == CHIPSET_KALAMA) {
        lowLatency = 0;
    }

    if (mVppEnabled == false || mPlatFormValue == CHIPSET_OTHERS || mPlatFormValue == CHIPSET_NOTSUPPORT || \
            mPlatFormValue == CHIPSET_KONA) {
        ALOGI("video box disabled not support mediacodec mode");
        return false;
    }
    if (mVppEnabled && (thumbnailMode ==0) && (heicMode ==0) && (frameMode == 0) && (VideoAnalysisMode == 0) && \
        (rateFloat <= FRC_MAX_FPS) && (lowLatency == 0)) {
        ALOGI("video box going");
    } else {
        mVppEnabled = false;
        ALOGI("video box disabled");
        return false;
    }

    if (!format->findFloat("frame-rate", &frameRateFloat)) {
        if (!format->findInt32("frame-rate", &frameRateInt)) {
            frameRateInt = -1;
        }
        frameRateFloat = (float)frameRateInt;
    }
    int32_t profile = 0;
    format->findInt32("profile", &profile);
    int32_t width = 0;
    int32_t height = 0;
    format->findInt32("width", &width);
    format->findInt32("height", &height);
    mVppVideoResolution.width = width;
    mVppVideoResolution.height = height;
    mFrameRate = frameRateFloat;
    ALOGV("mFrameRate:%d",mFrameRate);
    mProfile = profile;
    ALOGI("mProfile:%x",mProfile);
    auto ret = checkVideoBoxFeature(format);
    if (ret == false) {
        mVppEnabled = false;
        ALOGI("video box disabled not support of checkVideoBoxFeature");
        return ret;
    }

    mVideoBoxHandle ++;
    ALOGI("video box instance count = %d",mVideoBoxHandle);
    readVppProps();
    if (!mIsVppActive) {
        initVppDefaultValue();
        ALOGI("video box disabled for all vpp usecase not enabled");
    }

    if (mPlatFormValue == CHIPSET_MTK) {
        format->setInt32("video-memc-enable", 1);
        ALOGI("video box enabled mtk");
    }
    ALOGI("video box enabled");
    return true;
}

bool VideoBox::videoBoxWhiteList(String16 name,int flag)
{
    ALOGV("videoBoxWhiteList");
    bool found = false;
    int ret = -1;
    char *line = nullptr;
    size_t len = 0;
    ssize_t n = 0;

    if (flag == AIE) {
        auto fp = fopen("/data/system/vppaie.txt", "r");
        if (fp != nullptr) {
            ALOGV("use file");
            while ((n = getline(&line,&len,fp)) != -1) {
                ret = strncmp(line,(char *)name.string(),n - 1);
                ALOGV("getline aie namelist file = %s,n = %zu",line,n);

                if (ret == 0)
                    break;
            }
           fclose(fp);
        } else {
            for (int i = 0;i < VIDEOBOX_AIE_WHITELIST_SIZE;i++) {
                ret = String16(VppWhiteListAIE[i].c_str()).compare(name);
                ALOGV("getline aie namelist = %s",VppWhiteListAIE[i].c_str());
                if (ret == 0)
                    break;
            }
        }
    }

    if (flag == FRC) {
        auto fp = fopen("/data/system/vppfrc.txt", "r");
        if (fp != nullptr) {
            ALOGV("use file");
            while ((n = getline(&line,&len,fp)) != -1) {
                ret = strncmp(line,(char *)name.string(),n - 1);
                ALOGV("getline frc namelist file = %s,n = %zu",line,n);

                if (ret == 0)
                    break;
            }
            fclose(fp);
        } else {
            for (int i = 0;i < VIDEOBOX_FRC_WHITELIST_SIZE;i++) {
                ret = String16(VppWhiteListFRC[i].c_str()).compare(name);
                ALOGV("getline frc namelist = %s",VppWhiteListAIE[i].c_str());

                if (ret == 0)
                    break;
            }
        }
    }

    if (flag == AIS) {
        auto fp = fopen("/data/system/vppais.txt", "r");
        if (fp != nullptr) {
            ALOGV("use file");
            while ((n = getline(&line,&len,fp)) != -1) {
                ret = strncmp(line,(char *)name.string(),n - 1);
                ALOGV("getline ais namelist file = %s,n = %zu",line,n);

                if (ret == 0)
                    break;
            }
           fclose(fp);
        } else {
            for (int i = 0;i < VIDEOBOX_AIS_WHITELIST_SIZE;i++) {
                ret = String16(VppWhiteListAIS[i].c_str()).compare(name);
                ALOGV("getline ais namelist = %s",VppWhiteListAIE[i].c_str());
                if (ret == 0)
                    break;
            }
        }
    }

    if (ret == 0)
        found = true;

    if (found == true)
        ALOGI("videoBoxWhiteList found true");

    if (line != nullptr)
        free(line);

    return found;
}

bool VideoBox::readVideoBox(int32_t portIndex,const sp<MediaCodecBuffer> &buffer)
{
    if ((mVppEnabled == false) || (mPlatFormValue == CHIPSET_MTK) || (portIndex == kPortIndexInput))
        return false;

    int64_t timeUs = 0;
    buffer->meta()->findInt64("timeUs", &timeUs);
    getInterpolated(timeUs);

    if ((mFrameDecodeCnt++ % 10 == 0) && (mFrameDecodeCnt >= 4)) {
        if (mFlagVppFrcSupport || mFlagVppAieSupport || mFlagVppAisSupport) {
            readVppProps();
	 }
    }
    return true ;
}

void VideoBox::initVppDefaultValue()
{
    ALOGI("initVppDefaultValue");
 /*
    if (mFlagVppFrcSupport) {
        sp<AMessage> formatfrc =new AMessage;
        formatfrc->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
        formatfrc->setString(VPP_FRC_MODE_VENDOR_EXT,VPP_FRC_MODE_KEY_VALUE_FRC_SMOOTH);
        formatfrc->setString(VPP_FRC_LEVEL_VENDOR_EXT,VPP_FRC_LEVEL_KEY_VALUE_OFF);
        formatfrc->setString(VPP_FRC_INTERP_VENDOR_EXT,VPP_FRC_INTERP_KEY_VALUE_1X);
        formatfrc->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_INPUT,0);
        formatfrc->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_ON_FALLBACK,0);
        mCodec->signalSetParameters(formatfrc);
    }
    if (mFlagVppAieSupport) {
        sp<AMessage> formataie =new AMessage;
        formataie->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
        formataie->setString(VPP_AIE_MODE_VENDOR_EXT,VPP_AIE_MODE_KEY_VALUE_MANUAL);
        formataie->setString(VPP_AIE_HUE_MODE_VENDOR_EXT,VPP_AIE_HUE_MODE_KEY_VALUE_OFF);
        formataie->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,0);
        formataie->setInt32(VPP_AIE_LTM_LEVEL_VENDOR_EXT,0);
        formataie->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,0);
        formataie->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,50);
        formataie->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,0);
        formataie->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        formataie->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,0);
        mCodec->signalSetParameters(formataie);
    }
    if (mFlagVppAisSupport) {
        sp<AMessage> formatais =new AMessage;
        formatais->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
        formatais->setString(VPP_AIS_MODE_VENDOR_EXT,VPP_AIS_MODE_VALUE_AUTO);
        formatais->setInt32(VPP_AIS_BYPASS_VALUE_AUTO,1);
        mCodec->signalSetParameters(formatais);
    }
*/
    sp<AMessage> format =new AMessage;
    format->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
    if (mFlagVppFrcSupport) {
        format->setString(VPP_FRC_MODE_VENDOR_EXT,VPP_FRC_MODE_KEY_VALUE_FRC_SMOOTH);
        format->setString(VPP_FRC_LEVEL_VENDOR_EXT,VPP_FRC_LEVEL_KEY_VALUE_OFF);
        format->setString(VPP_FRC_INTERP_VENDOR_EXT,VPP_FRC_INTERP_KEY_VALUE_1X);
        format->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_INPUT,0);
        format->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_ON_FALLBACK,0);
    }

    if (mFlagVppAieSupport) {
        format->setString(VPP_AIE_MODE_VENDOR_EXT,VPP_AIE_MODE_KEY_VALUE_MANUAL);
        format->setString(VPP_AIE_HUE_MODE_VENDOR_EXT,VPP_AIE_HUE_MODE_KEY_VALUE_OFF);
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_LEVEL_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,50);
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,0);
    }

    if (mFlagVppAisSupport) {
        format->setString(VPP_AIS_MODE_VENDOR_EXT,VPP_AIS_MODE_VALUE_AUTO);
        format->setInt32(VPP_AIS_BYPASS_VALUE_AUTO,1);
    }
    if (mFlagVppFrcSupport || mFlagVppAieSupport || mFlagVppAisSupport) {
        format->setInt32(VPP_MODE_VENDOR_EXT_BYPASS,1);
        mCodec->signalSetParameters(format);
    }
}

void VideoBox::readVppProps()
{
    ALOGV("readVppProps");
    if (mVppEnabled == false)
        return ;
    readAppCtrlProps();
    readVppFrcProps();
    readVppAieProps();
    readVppAisProps();
}

void VideoBox::enableFrc()
{
    ALOGV("enableFrc");
    if (mIsAISEnabled)
        bypassAis();
    sp<AMessage> format =new AMessage;
    format->setInt32(VPP_LAZZY_INIT_VENDOR_EXT,1);
    format->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
    format->setInt32(VPP_MODE_VENDOR_EXT_BYPASS,0);
    format->setString(VPP_FRC_MODE_VENDOR_EXT,VPP_FRC_MODE_KEY_VALUE_FRC_SMOOTH);
    format->setString(VPP_FRC_LEVEL_VENDOR_EXT,VPP_FRC_LEVEL_KEY_VALUE_HIGH);
    format->setString(VPP_FRC_INTERP_VENDOR_EXT,VPP_FRC_INTERP_KEY_VALUE_2X);
    format->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_INPUT,0);
    format->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_ON_FALLBACK,0);
    mCodec->signalSetParameters(format);

    if (!mIsVppEnabled) {
        if (mFlagVppAisSupport)
            bypassAis();
        if (mFlagVppAieSupport)
            bypassAie();
        mIsVppEnabled = true;
    }

    mIsVppActive = true;
    mIsFRCEnabled = true;
}

void VideoBox::enableAie()
{
    ALOGV("enableAie");
    sp<AMessage> format =new AMessage;
    format->setInt32(VPP_LAZZY_INIT_VENDOR_EXT,1);
    format->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
    format->setInt32(VPP_MODE_VENDOR_EXT_BYPASS,0);
    format->setString(VPP_AIE_MODE_VENDOR_EXT,VPP_AIE_MODE_KEY_VALUE_MANUAL);
    format->setString(VPP_AIE_HUE_MODE_VENDOR_EXT,VPP_AIE_HUE_MODE_KEY_VALUE_ON);
    format->setInt32(VPP_AIE_LTM_LEVEL_VENDOR_EXT,1);

    ALOGI("width x height:%d x %d",mVppVideoResolution.width,mVppVideoResolution.height);
    uint32_t value = mVppVideoResolution.width*mVppVideoResolution.height;
    if(value <=(352*288)) {
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,6);
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,55);
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,55);
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,35);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,10);
        ALOGI("352x288 value is used");
    } else if (value <=(640*480)) {
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,7);
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,55);
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,55);
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,36);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,15);
        ALOGI("640x480 value is used");
    } else if (value <=(448*960)) {
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,8);
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,55);
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,55);
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,37);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,20);
        ALOGI("448x960 value is used");
    } else if (value <=(1280*720)) {
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,8);
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,55);
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,55);
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,38);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,20);
        ALOGI("1280x720 value is used");
    } else if (value <=(1920*1088)) {
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,8);
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,58);
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,58);
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,40);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,20);
        ALOGI("1920x1088 value is used");
    } else if (value <=(4096*2160)) {
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,8);
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,60);
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,60);
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,41);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,20);
        ALOGI("4096x2060 value is used");
    } else {
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,8);
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,62);
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,62);
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,41);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,20);
        ALOGI("high value is used");
    }
    int32_t v = read_prop("debug.media.vpp.debug.value.use");
    if(v > 0) {
        format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,read_prop("debug.media.vpp.aie.cade"));
        format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,read_prop("debug.media.vpp.aie.ltmsatgain"));
        format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,read_prop("debug.media.vpp.aie.ltmsatoff"));
        format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,read_prop("debug.media.vpp.aie.ltmacestr"));
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,read_prop("debug.media.vpp.aie.ltmacebrih"));
        format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,read_prop("debug.media.vpp.aie.ltmacebril"));
        ALOGI("debug prop value is used");
    }
    mCodec->signalSetParameters(format);

    if (!mIsVppEnabled) {
        if (mFlagVppAisSupport)
            bypassAis();
        if (mFlagVppFrcSupport)
            bypassFrc();
        mIsVppEnabled = true;
    }

    mIsVppActive = true;
    mIsAIEEnabled = true;
}

void VideoBox::enableAis()
{
    ALOGI("enableAis");
    if (mIsFRCEnabled)
        bypassFrc();
    sp<AMessage> format =new AMessage;
    format->setInt32(VPP_LAZZY_INIT_VENDOR_EXT,1);
    format->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
    format->setInt32(VPP_MODE_VENDOR_EXT_BYPASS,0);
    format->setString(VPP_AIS_MODE_VENDOR_EXT,VPP_AIS_MODE_VALUE_AUTO);
    format->setInt32(VPP_AIS_BYPASS_VALUE_AUTO,0);
    mCodec->signalSetParameters(format);

    if (!mIsVppEnabled) {
        if (mFlagVppAieSupport)
            bypassAie();
        if (mFlagVppFrcSupport)
            bypassFrc();
        mIsVppEnabled = true;
    }

    mIsVppActive = true;
    mIsAISEnabled = true;
}

void VideoBox::bypassAis()
{
    ALOGI("bypassAis");
    sp<AMessage> format =new AMessage;
    format->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
    format->setString(VPP_AIS_MODE_VENDOR_EXT,VPP_AIS_MODE_VALUE_AUTO);
    format->setInt32(VPP_AIS_BYPASS_VALUE_AUTO,1);
    mCodec->signalSetParameters(format);

    mIsAISEnabled = false;
    mIsVppActive = true;
}

void VideoBox::disableAis()
{
    ALOGV("disableAis");
    mIsAISEnabled = false;
    mIsVppActive = false;
}

void VideoBox::bypassFrc()
{
    ALOGV("bypassFrc");
    sp<AMessage> format =new AMessage;
    format->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
    format->setString(VPP_FRC_MODE_VENDOR_EXT,VPP_FRC_MODE_KEY_VALUE_FRC_SMOOTH);
    format->setString(VPP_FRC_LEVEL_VENDOR_EXT,VPP_FRC_LEVEL_KEY_VALUE_OFF);
    format->setString(VPP_FRC_INTERP_VENDOR_EXT,VPP_FRC_INTERP_KEY_VALUE_1X);
    format->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_INPUT,0);
    format->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_ON_FALLBACK,0);
    mCodec->signalSetParameters(format);
    mIsVppActive = true;
    mIsFRCEnabled = false;
}

void VideoBox::disableFrc()
{
    ALOGV("disableFrc");
    sp<AMessage> format =new AMessage;
    format->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
    format->setString(VPP_FRC_MODE_VENDOR_EXT,VPP_FRC_MODE_KEY_VALUE_OFF);
    format->setString(VPP_FRC_LEVEL_VENDOR_EXT,VPP_FRC_LEVEL_KEY_VALUE_OFF);
    format->setString(VPP_FRC_INTERP_VENDOR_EXT,VPP_FRC_INTERP_KEY_VALUE_1X);
    format->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_INPUT,0);
    format->setInt32(VPP_FRC_INTERP_KEY_FRAME_COPY_ON_FALLBACK,0);
    mCodec->signalSetParameters(format);
    mIsVppActive = false;
    mIsFRCEnabled = false;
}

void VideoBox::bypassAie()
{
    ALOGV("bypassAie");
    sp<AMessage> format =new AMessage;
    format->setString(VPP_MODE_VENDOR_EXT,VPP_MODE_KEY_VALUE_MANUAL);
    format->setString(VPP_AIE_MODE_VENDOR_EXT,VPP_AIE_MODE_KEY_VALUE_MANUAL);
    format->setString(VPP_AIE_HUE_MODE_VENDOR_EXT,VPP_AIE_HUE_MODE_KEY_VALUE_OFF);
    format->setInt32(VPP_AIE_CADE_LEVEL_VENDOR_EXT,0);
    format->setInt32(VPP_AIE_LTM_LEVEL_VENDOR_EXT,0);
    format->setInt32(VPP_AIE_LTM_SAT_GAIN_VENDOR_EXT,0);
    format->setInt32(VPP_AIE_LTM_SAT_OFFSET_VENDOR_EXT,50);
    format->setInt32(VPP_AIE_LTM_ACE_STR_VENDOR_EXT,0);
    format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_H_VENDOR_EXT,0);
    format->setInt32(VPP_AIE_LTM_ACE_BRIGHT_L_VENDOR_EXT,0);
    mCodec->signalSetParameters(format);
    mIsVppActive = true;
    mIsAIEEnabled = false;
}

void VideoBox::getSupportedProps()
{
    char value[PROPERTY_VALUE_MAX] = {0};

    if ((property_get(FLAG_FRC_SUPPORT, value, NULL))
            && (!strcmp("1", value) || !strcmp("true", value))) {
        mFlagVppFrcUse = true;
        ALOGI("FRC is used");
    }

    if ((property_get(FLAG_AIE_SUPPORT, value, NULL))
            && (!strcmp("1", value) || !strcmp("true", value))) {
        mFlagVppAieUse = true;
        ALOGI("AIE is used");
    }

    if ((property_get(FLAG_AIS_SUPPORT, value, NULL))
            && (!strcmp("1", value) || !strcmp("true", value))) {
        mFlagVppAisUse = true;
        ALOGI("AIS is used");
    }
}

void  VideoBox::readAppCtrlProps()
{
    char value[PROPERTY_VALUE_MAX] = {0};
// hqv aie
    if ((property_get(VPP_APP_CONTROL_AIE, value, NULL))
            && (!strcmp("1", value) || !strcmp("true", value))) {
        mVppEnableFlags |= FLAG_VPP_AIE_ENALBE;
	 mVppEnableFlags &= ~FLAG_VPP_AIE_BYPASS;
        ALOGV("APP AIE Enable");
    }
    else {
        mVppEnableFlags &= ~FLAG_VPP_AIE_ENALBE;
        mVppEnableFlags |= FLAG_VPP_AIE_BYPASS;
        ALOGV("APP AIE Bypass");
    }
// frc
    if ((property_get(VPP_APP_CONTROL_FRC, value, NULL))
            && (!strcmp("1", value) || !strcmp("true", value))) {
        mVppEnableFlags |= FLAG_VPP_FRC_ENALBE;
        mVppEnableFlags &= ~ FLAG_VPP_FRC_BYPASS;
	 ALOGV("APP FRC Enable");
    }
    else {
        mVppEnableFlags &= ~FLAG_VPP_FRC_ENALBE;
        mVppEnableFlags |= FLAG_VPP_FRC_BYPASS;
	 ALOGV("APP FRC Bypass");
    }
// hqv ais
    if ((property_get(VPP_APP_CONTROL_AIS, value, NULL))
            && (!strcmp("1", value) || !strcmp("true", value))) {
        mVppEnableFlags |= FLAG_VPP_AIS_ENALBE;
	 mVppEnableFlags &= ~FLAG_VPP_AIS_BYPASS;
        ALOGV("APP AIS Enable");
    }
    else {
        mVppEnableFlags &= ~FLAG_VPP_AIS_ENALBE;
        mVppEnableFlags |= FLAG_VPP_AIS_BYPASS;
        ALOGV("APP AIS Bypass");
    }
}

void VideoBox::readVppFrcProps()
{
    if (mFlagVppFrcSupport) {
        if ((mVppEnableFlags & FLAG_VPP_FRC_ENALBE) && (mOperatingRate <= FRC_MAX_FPS)) {
            ALOGV("FRC ENALBE");
            if (!mIsFRCEnabled) {
                mIsFRCEnabled = true;
                enableFrc();
            }
        }

        if (mVppEnableFlags & FLAG_VPP_FRC_BYPASS) {
            ALOGV("FRC BYPASS");
            if (mIsFRCEnabled) {
                mIsFRCEnabled = false;
                bypassFrc();
            }
        }
    }
}

void VideoBox::readVppAieProps()
{
    if (mFlagVppAieSupport && (mVppEnableFlags & FLAG_VPP_AIE_ENALBE)) {
        ALOGV("AIE ENALBE");
        if (!mIsAIEEnabled) {
            mIsAIEEnabled = true;
            enableAie();
        }
    }

    if (mFlagVppAieSupport && (mVppEnableFlags & FLAG_VPP_AIE_BYPASS)) {
        ALOGV("AIE BYPASS");
        if (mIsAIEEnabled) {
            mIsAIEEnabled = false;
            bypassAie();
        }
    }
}

void VideoBox::readVppAisProps()
{
    if (mFlagVppAisSupport && (mVppEnableFlags & FLAG_VPP_AIS_ENALBE)) {
        ALOGV("AIS ENALBE");
        if (!mIsAISEnabled) {
            mIsAISEnabled = true;
            enableAis();
        }
    }

    if (mFlagVppAisSupport && (mVppEnableFlags & FLAG_VPP_AIS_BYPASS)) {
        ALOGV("AIS BYPASS");
        if (mIsAISEnabled) {
            mIsAISEnabled = false;
            bypassAis();
        }
    }
}

bool VideoBox::checkVideoBoxFeature(const sp<AMessage> format)
{
    ALOGI("checkVideoBoxFeature");
    if (mFlagVppFrcUse && mFlagVppFrcSupport) {
        uint32_t w = mVppVideoResolution.width;
        uint32_t h = mVppVideoResolution.height;
        uint32_t frc_support_max_height = 0;
        uint32_t frc_support_max_width = 0;
        if (mVppVideoResolution.width < mVppVideoResolution.height) {
            w = mVppVideoResolution.height;
            h = mVppVideoResolution.width;
        }
        if (mPlatFormValue == CHIPSET_KALAMA) {
            frc_support_max_height = FRC_HW_MAX_HEIGHT;
            frc_support_max_width = FRC_HW_MAX_WIDTH;
        } else {
            frc_support_max_height = FRC_MAX_HEIGHT;
            frc_support_max_width = FRC_MAX_WIDTH;
        }

        if (w > frc_support_max_width || h > frc_support_max_height || w <= FRC_MIN_WIDTH ||h <= FRC_MIN_HEIGHT || \
                (mIsHevc && (mProfile == OMX_VIDEO_HEVCProfileMain10 ||mProfile == OMX_VIDEO_HEVCProfileMain10HDR10 || \
                mProfile == OMX_VIDEO_HEVCProfileMain10HDR10Plus)) || mFrameRate > FRC_MAX_FPS || isHDR(format)) {
            if (mPlatFormValue == CHIPSET_MTK) {
                mFlagVppFrcSupport = false;
                ALOGI("FRC OFF");
                return false;
            }
            mFlagVppFrcSupport = false;
            ALOGI("FRC OFF");
            if (mVppEnableFlags & FLAG_VPP_FRC_ENALBE) {
                disableFrc();
            }
            ALOGI("frc valid: false");
        } else {
            mFlagVppFrcSupport = true;
            ALOGI("frc valid: true");
        }
    }

    if (mFlagVppAieUse && mFlagVppAieSupport) {
        if (mVppVideoResolution.width > AIE_MAX_WIDTH || mVppVideoResolution.height > AIE_MAX_HEIGHT || \
               (mIsHevc && (mProfile == OMX_VIDEO_HEVCProfileMain10 ||mProfile == OMX_VIDEO_HEVCProfileMain10HDR10 || \
               mProfile == OMX_VIDEO_HEVCProfileMain10HDR10Plus)) || isHDR(format)) {
            mFlagVppAieSupport = false;
            if (mVppEnableFlags & FLAG_VPP_AIE_ENALBE) {
                bypassAie();
            }
            ALOGI("aie valid: false");
        } else {
            mFlagVppAieSupport = true;
            ALOGI("aie valid: true");
        }
    }

    if (mFlagVppAisUse && mFlagVppAisSupport) {
        uint32_t w = mVppVideoResolution.width;
        uint32_t h = mVppVideoResolution.height;
        if (mVppVideoResolution.width < mVppVideoResolution.height) {
            w = mVppVideoResolution.height;
            h = mVppVideoResolution.width;
        }
        if (w > AIS_MAX_WIDTH ||h > AIS_MAX_HEIGHT || isHDR(format) || \
                (mIsHevc && (mProfile == OMX_VIDEO_HEVCProfileMain10 ||mProfile == OMX_VIDEO_HEVCProfileMain10HDR10 || \
                mProfile == OMX_VIDEO_HEVCProfileMain10HDR10Plus)) || mFrameRate > AIS_MAX_FPS || \
                mVppVideoResolution.width <= AIS_MIN_WIDTH ||mVppVideoResolution.height <= AIS_MIN_HEIGHT) {
            mFlagVppAisSupport = false;
            ALOGI("AIS OFF");
            if (mVppEnableFlags & FLAG_VPP_AIS_ENALBE) {
                disableAis();
            }
            ALOGI("ais valid: false");
        } else {
            mFlagVppAisSupport = true;
            ALOGI("ais valid: true");
        }
    }

    if ((mFlagVppAieSupport == false) && (mFlagVppFrcSupport == false) && (mFlagVppAisSupport == false)) {
        return false;
    }
    return true;
}

void VideoBox::getInterpolated(int64_t timestamp)
{
    if ((mVppEnabled == false) || (mPlatFormValue == CHIPSET_MTK) || !mFlagVppFrcSupport)
        return ;

    if (mFrameRate == 0) {
        mFrameRate = mFrameRateRealTime;
        mOperatingRate = mFrameRate;
    }

    ALOGV("Interpolated:timestamp=%" PRId64",mFrameRate = %d",timestamp,mFrameRate);

    if (mFrameDecoded > 0) {
        if ((timestamp > mLastTimeStampInterp) && \
                    (timestamp < mLastTimeStampInterp + (1000/mFrameRate) * 1000*2/3) && \
                    (mLastFrameInterp == false)) {
            mFrameInterp++;
            mLastFrameInterp = true;
            ALOGV("Interpolated:%d",mFrameInterp);
        } else {
            mLastFrameInterp = false;
            mLastTimeStampInterp = timestamp;
        }
    }

    if ((mFrameInterp > 0) && (mFrameInterp % 60 == 0) && mFlagVppFrcSupport) {
        ALOGV("video clip interpolated = %d%%,%d--%d",(mFrameInterp * 100)/mFrameDecoded,mFrameInterp,mFrameDecoded);
    }
}

bool VideoBox::getRealtimeFramerate(const sp<AMessage> &msg)
{
    int64_t TimeStamp;
    msg->findInt64("timeUs", &TimeStamp);
    ALOGV("TimeStamp = %" PRId64"",TimeStamp);
    if ((mVppEnabled == false) || (mPlatFormValue == CHIPSET_MTK))
        return false;

    mFrameDecoded ++;

    if (TimeStamp > 0) {
        if (TimeStamp > mLastTimeStampRealTime) {
            uint64_t TS = TimeStamp -mLastTimeStampRealTime;
            mFrameRateRealTime = 1000/(TS/1000);
            ALOGV("mFrameRateRealTime = %" PRId64" ts=%" PRId64 " ts=%" PRId64 " TimeStamp =%" PRId64 "",\
                    mFrameRateRealTime,mLastTimeStampRealTime,TS,TimeStamp);
        }
        mLastTimeStampRealTime = TimeStamp;
    }
    return true ;
}

bool VideoBox::isFrcEnable() {
    return mIsFRCEnabled;
}

extern "C" IVideoBoxStub* create() {
    return new VideoBox;
}

extern "C" void destroy(IVideoBoxStub* impl) {
    delete (videobox::VideoBox*)impl;
}

}
}
