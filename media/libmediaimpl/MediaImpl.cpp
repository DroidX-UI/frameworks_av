#define LOG_TAG "MediaImpl"

#include <utils/Log.h>
#include <binder/AppOpsManager.h>
#include <binder/PermissionController.h>
#include <cutils/properties.h>
#include <private/android_filesystem_config.h>
#include <system/audio.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <AVExtensions.h>
#include <string.h>

//#include <media/openmax/OMX_IndexExt.h>

#include "MediaImpl.h"
#include "ozoaudiodec/ozoplayer.h"
#include "OzoAudioFileSource.h"
#include "ozoCodecEventListener.h"
#include "OzoStagefright.h"

#include "send_data_to_xlog.h"//MIUI ADD FOR ONETRACK RBIS

#ifdef DOLBY_ENABLE
#include <TableXInit.h>
#include <ds_config.h>
#include <DolbyACodecExtImpl.h>
#endif

#include <OMX_Audio.h>

#include <media/openmax/OMX_AudioExt.h>
#include <media/openmax/OMX_IndexExt.h>

#ifdef DOLBY_AC4_SPLIT_SEC
#define OMX_LUT_BUFF_SZ 256
#define OMX_B_C_SZ 84
typedef struct OMX_AUDIO_PARAM_ANDROID_AC4TBL {
    OMX_U32 nSize;
    OMX_U8 seedA;
    OMX_U8 seedB;
    OMX_U8 seedC;

    OMX_U8 idA;
    OMX_U8 idB;
    OMX_U8 idC;

    OMX_U8 maskA;
    OMX_U8 maskB;
    OMX_U8 maskC;

    OMX_U32 sizeA;
    OMX_U32 sizeB;
    OMX_U32 sizeC;

    OMX_U8 bufferA[OMX_LUT_BUFF_SZ];
    OMX_U8 bufferB[OMX_B_C_SZ];
    OMX_U8 bufferC[OMX_B_C_SZ];
} OMX_AUDIO_PARAM_ANDROID_AC4TBL;
#endif // DOLBY_AC4_SPLIT_SEC

namespace android {

#ifdef MEDIA_PROJECTION
struct Package {
    std::string name;
    bool playbackCaptureAllowed = false;
};

static const bool kSupportOZO = property_get_bool("ro.vendor.audio.zoom.support", false );

//MIUI ADD FOR ONETRACK RBIS START
#define FLAG_FRC_SUPPORT "debug.config.media.video.frc.support"
#define VPP_APP_CONTROL_FRC "debug.media.video.frc"
#define FLAG_AIE_SUPPORT "debug.config.media.video.aie.support"
#define VPP_APP_CONTROL_AIE "debug.media.video.vpp"
#define FLAG_AIS_SUPPORT "debug.config.media.video.ais.support"
#define VPP_APP_CONTROL_AIS "debug.media.video.ais"
#define FLAG_DOLBYVISION_SUPPORT "ro.vendor.audio.dolby.vision.support"

#define MALLOC_SIZE 64
//MIUI ADD FOR ONETRACK RBIS END

/*white list of players which can be capture by MediaProjection even targetSDK < 29*/
std::string allowedPlaybackCaptureWhiteList[] = {
    "com.netflix.mediaclient",
    "com.hulu.plus",
    "com.amazon.avod.thirdpartyclient",
    "com.tubitv",
    "com.hbo.hbonow",
    "com.gotv.crackle.handset",
    "tv.twitch.android.app",
    "com.disney.disneyplus",
    "com.imdb.mobile",
    "com.cbs.app",
    "com.microsoft.xboxone.smartglass",
    "com.cw.fullepisodes.android",
    "com.TWCableTV",
    "com.fox.now",
    "com.nbcuni.nbc",
    "com.disney.datg.videoplatforms.android.abc",
    "com.disney.datg.videoplatforms.android.watchdc",
    "com.peacocktv.peacockandroid",
    "com.hulu.livingroomplus",
    "com.cbs.ott",
    "com.google.android.youtube",
    "com.mxtech.videoplayer.ad",
    "com.facebook.orca",
    "com.facebook.katana",
    "com.instagram.android",
    "com.snapchat.android",
    "com.whatsapp",
    "com.skype.raider",
    "com.twitter.android",
    "com.zhiliaoapp.musically",
    "kik.android",
    "com.jb.gosms",
    "com.tumblr",
    "com.viber.voip",
    "jp.naver.line.android",
    "com.android.chrome",
    "com.myyearbook.m",
    "com.reddit.frontpage",
    "com.facebook.mlite",
    "com.textmeinc.textme",
    "org.telegram.messenger",
    "com.tencent.mm",
    "com.ss.android.ugc.aweme",
    "com.android.browser",
    "com.tencent.mobileqq",
    "com.miui.video",
    "com.smile.gifmaker",
    "com.ss.android.ugc.aweme.lite",
    "com.kuaishou.nebula",
    "com.tencent.mtt",
    "tv.danmaku.bili",
    "com.sina.weibo",
    "com.UCMobile",
    "com.tencent.qqlive",
    "com.kugou.android",
    "com.netease.cloudmusic",
    "com.tencent.qqmusic",
    "com.qiyi.video",
    "com.miui.player",
    "com.zhihu.android",
    "com.ss.android.article.video",
    "com.xingin.xhs",
    "com.ximalaya.ting.android",
    "com.ss.android.ugc.live",
    "com.youku.phone",
    "com.quark.browser",
    "com.tencent.weishi",
    "com.baidu.tieba",
    "com.duowan.kiwi",
    "com.baidu.haokan",
    "air.tv.douyu.android",
    "com.baidu.searchbox.lite",
    "com.xiaomi.vipaccount",
    "com.tencent.gamehelper.smoba",
    "cn.soulapp.android",
    "cn.kuwo.player",
    "com.hunantv.imgo.activity",
    "com.immomo.momo",
    "com.tencent.qt.qtl",
    "cn.xiaochuankeji.tieba",
    "com.sup.android.superb",
    "com.le123.ysdq",
    "com.p1.mobile.putong",
    "com.babycloud.hanju",
    "com.mihoyo.hyperion",
    "com.cctv.yangshipin.app.androidp",
    "com.douban.frodo",
    "com.yuncheapp.android.pearl",
    "com.zhongduomei.rrmj.society",
    "cmccwm.mobilemusic",
    "com.cmcc.cmvideo",
    "cn.cntv",
    "tv.pps.mobile",
    "com.ume.browser.hs",
    "mark.via",
    "com.kuaiyin.player",
    "com.duowan.mobile",
    "com.ss.android.ugc.livelite",
    "fm.qingting.qtradio",
    "com.qiyi.video.lite",
    "com.qihoo.browser",
    "com.sogou.activity.src",
    "org.mozilla.firefox",
    "com.all.video",
    "com.ucmobile.lite",
    "sogou.mobile.explorer",
    "com.microsoft.emmx",
    "com.xfplay.play",
    "com.tencent.qgame",
    "tv.acfundanmaku.video",
    "com.wondertek.miguaikan",
    "com.qihoo.contents",
    "com.baidu.video",
    "com.jovetech.CloudSee.temp",
    "cn.vcinema.cinema",
    "com.wali.live",
    "com.tencent.nbagametime",
    "com.sohu.sohuvideo",
    "com.baidu.browser.apps",
    "tv.yixia.bobo",
    "com.huajiao",
    "com.tencent.tv.qie",
    "com.kugou.fanxing",
    "com.baidu.searchcraft",
    "com.kwai.thanos",
    "com.tencent.videolite.android",
    "com.tencent.now",
    "com.iyuba.ted",
    "com.tencent.tmgp.pubgmhd",
    "com.tencent.tmgp.sgame"
};

bool isInWhiteList( std::string packageName)
{
    bool boRet = false;
    uint32_t size = sizeof(allowedPlaybackCaptureWhiteList)
        / sizeof(allowedPlaybackCaptureWhiteList[0]);
    uint32_t i;
    for (i = 0; i < size; i++) {
        if (!strcmp(packageName.c_str(),
            allowedPlaybackCaptureWhiteList[i].c_str())) {
            boRet = true;
            break;
        }
    }
    return boRet;
}
#endif

//#ifdef MIAUDIO_OZO
// Trim both leading and trailing whitespace from the given string.
static void TrimString(String8 *s) {
    size_t num_bytes = s->bytes();
    const char *data = s->string();

    size_t leading_space = 0;
    while (leading_space < num_bytes && isspace(data[leading_space])) {
        ++leading_space;
    }

    size_t i = num_bytes;
    while (i > leading_space && isspace(data[i - 1])) {
        --i;
    }

    s->setTo(String8(&data[leading_space], i - leading_space));
}
//#endif //MIAUDIO_OZO

void MediaImpl::parseOzoOrientations(KeyedVector<size_t, AString> & events,
        const char *data)
{
	if(kSupportOZO){
		if (!data) return;

		char *literal = strndup(data, strlen(data));
		char *value = strtok(literal, ":");
		while (value != NULL) {
			if (strlen(value) != 0) {
				std::string params(value);

				size_t found = params.find_first_of(" ");
				if (found != std::string::npos) {
					size_t frame = (size_t)std::stoul(params.substr(0, found));
					AString event(params.substr(found + 1).c_str());
					events.add(frame, event);
				}
			}
			value = strtok(NULL, ":");
		}

		free(literal);
	} else {
	    UNUSED(events);
	    UNUSED(data);
	}
}

status_t MediaImpl::setOzoAudioTuneFile(
        sp<IMediaRecorder> &mediaRecorder,
        media_recorder_states &currentState,
        int fd ){
	if(kSupportOZO){
		ALOGV("setOzoAudioTuneFile(%d)", fd);
		if (mediaRecorder == NULL) {
			ALOGE("media recorder is not initialized yet");
			return INVALID_OPERATION;
		}
		if (!(currentState & MEDIA_RECORDER_DATASOURCE_CONFIGURED)) {
			ALOGE("setOzoAudioTuneFile called in an invalid state(%d)", currentState);
			return INVALID_OPERATION;
		}

		// It appears that if an invalid file descriptor is passed through
		// binder calls, the server-side of the inter-process function call
		// is skipped. As a result, the check at the server-side to catch
		// the invalid file descritpor never gets invoked. This is to workaround
		// this issue by checking the file descriptor first before passing
		// it through binder call.
		int flags = fcntl(fd, F_GETFL);
		if (flags == -1) {
			ALOGE("Fail to get file status flags err: %s", strerror(errno));
		}
		// fd must be in read-write mode or write-only mode.
		if ((flags & (O_RDWR | O_WRONLY)) == 0) {
			ALOGE("File descriptor is not in read-write mode or write-only mode");
			return BAD_VALUE;
		}

		status_t ret = mediaRecorder->setOzoAudioTuneFile(fd);
		if (OK != ret) {
			ALOGE("setOzoAudioTuneFile failed: %d", ret);
			currentState = MEDIA_RECORDER_ERROR;
			return ret;
		}

		return ret;
	} else {
		UNUSED(mediaRecorder);
		UNUSED(currentState);
		UNUSED(fd);
		return NO_INIT;
	}
}

status_t MediaImpl::setOzoRunTimeParametersMedia(
        sp<IMediaRecorder> &mediaRecorder,
        media_recorder_states &currentState,
        const String8& params ){
	if(kSupportOZO){
		ALOGV("setOzoRunTimeParameters(%s)", params.string());
		if (mediaRecorder == NULL) {
			ALOGE("media recorder is not initialized yet");
			return INVALID_OPERATION;
		}

		bool isInvalidState = (currentState & (MEDIA_RECORDER_PREPARED | MEDIA_RECORDER_ERROR));
		if (isInvalidState) {
			ALOGE("setOzoRunTimeParameters is called in an invalid state: %d", currentState);
			return INVALID_OPERATION;
		}

		status_t ret = mediaRecorder->setOzoRunTimeParameters(params);
		if (OK != ret) {
			ALOGE("setOzoRunTimeParameters(%s) failed: %d", params.string(), ret);
		}

		return ret;
	} else {
		UNUSED(mediaRecorder);
		UNUSED(currentState);
		UNUSED(params);
		return NO_INIT;
    }
}

void MediaImpl::createOzoAudioParamsStagefright(void* &ozoAudioParams) {
	if(kSupportOZO){
		ozoAudioParams = new struct OzoAudioParamsStagefright;
	} else {
		UNUSED(ozoAudioParams);
    }
    return;
}

void MediaImpl::destroyOzoAudioParamsStagefright(void * &ozoAudioParams) {
	if(kSupportOZO){
		if (ozoAudioParams != nullptr)
			delete (struct OzoAudioParamsStagefright *)ozoAudioParams;

		ozoAudioParams = nullptr;
	} else {
		UNUSED(ozoAudioParams);
    }
    return;
}

void MediaImpl::closeOzoAudioTuneFile(IMediaCodecEventListener* &ozoTuneWriter) {
	if(kSupportOZO){
		if (ozoTuneWriter)
			delete ozoTuneWriter;
		ozoTuneWriter = nullptr;
	} else {
        UNUSED(ozoTuneWriter);
	}
}

bool MediaImpl::ozoAudioSetParameterStagefrightImpl(void *params,
    bool &ozoFileSourceEnable, const String8 &key, const String8 &value) {
	if(kSupportOZO){
		struct OzoAudioParamsStagefright *audioParams = (OzoAudioParamsStagefright *)params;

		return OzoAudioSetParameterStagefright(*audioParams, ozoFileSourceEnable, key, value);
	} else {
		UNUSED(params);
		UNUSED(ozoFileSourceEnable);
		UNUSED(key);
		UNUSED(value);
		return false;
	}
}

void MediaImpl::ozoAudioInitStagefrightImpl(void* ozoAudioParams) {
	if(kSupportOZO){
		struct OzoAudioParamsStagefright *audioParams;

		if (ozoAudioParams == NULL)
			return;
		audioParams = (OzoAudioParamsStagefright *)ozoAudioParams;

		OzoAudioInitStagefright(*audioParams);
	} else {
		UNUSED(ozoAudioParams);
    }
}

bool MediaImpl::setOzoAudioTuneFileStagefrightRecorder(
        IMediaCodecEventListener* &ozoTuneWriter, int fd) {
	if(kSupportOZO){
		closeOzoAudioTuneFile(ozoTuneWriter);

		if (fd > 0) {
			auto writer = new OzoCodecPostDataWriter();
			if (!writer->init(fd)) {
				delete writer;
				return BAD_VALUE;
			}
			else
				ozoTuneWriter = writer;

			return OK;
		}
	} else {
		UNUSED(ozoTuneWriter);
		UNUSED(fd);
	}
    return BAD_VALUE;

}

status_t MediaImpl::setOzoRunTimeParametersStagefright(void *ozoAudioParams,
        sp<MediaCodecSource> audioEncoderSource, const String8 &params) {
	if(kSupportOZO){
		ALOGV("setOzoRunTimeParameters: %s", params.string());
		struct OzoAudioParamsStagefright *audioParams =
			(OzoAudioParamsStagefright *)ozoAudioParams;

		if (audioParams->device_id.length() == 0) {
			ALOGW("OZO Audio disabled, device ID not set");
			return OK;
		}

		const char *cparams = params.string();
		const char *key_start = cparams;
		sp<AMessage> message = new AMessage;
		for (;;) {
			const char *equal_pos = strchr(key_start, '=');
			if (equal_pos == NULL) {
				ALOGE("Parameters %s miss a value", cparams);
				return BAD_VALUE;
			}
			String8 key(key_start, equal_pos - key_start);
			TrimString(&key);
			if (key.length() == 0) {
				ALOGE("Parameters %s contains an empty key", cparams);
				return BAD_VALUE;
			}
			const char *value_start = equal_pos + 1;
			const char *semicolon_pos = strchr(value_start, ';');
			String8 value;
			if (semicolon_pos == NULL) {
				value.setTo(value_start);
			} else {
				value.setTo(value_start, semicolon_pos - value_start);
			}
			message->setString((const char *) key, (const char *) value);
			if (semicolon_pos == NULL) {
				break;  // Reaches the end
			}
			key_start = semicolon_pos + 1;
		}

		return audioEncoderSource->setRuntimeParameters(message);
	} else {
		UNUSED(ozoAudioParams);
		UNUSED(audioEncoderSource);
		UNUSED(params);
		return OK;
	}
}

status_t MediaImpl::createOzoAudioSource(
        const audio_attributes_t &attr,
        const android::content::AttributionSourceState& clientIdentity,
        uint32_t sourceSampleRate,
        uint32_t channels,
        uint32_t outSampleRate,
        audio_port_handle_t selectedDeviceId,
        audio_microphone_direction_t selectedMicDirection,
        float selectedMicFieldDimension,
        audio_encoder audioEncoder,
        output_format outputFormat,
        void *ozoAudioParams,
        sp<AudioSource> &audioSource,
        sp<AudioSource> &audioSourceNode,
        bool ozoFileSourceEnable,
        bool &use_ozo_capture) {
	if(kSupportOZO){
		if (ozoAudioParams == NULL) {
			ALOGE("%s: ozoAudioParams is NULL", __func__);
			return -1;
		}
		struct OzoAudioParamsStagefright *audioParams =
			(OzoAudioParamsStagefright *)ozoAudioParams;

		if (audioEncoder == AUDIO_ENCODER_AAC &&
				outputFormat == OUTPUT_FORMAT_MPEG_4 && sourceSampleRate == 48000) {
			use_ozo_capture = true;
		}

		if (audioParams->device_id.length() == 0) {
			ALOGW("ozo disabled, not using ozo capture");
			use_ozo_capture = false;
		}

		status_t err(OK);

		if (use_ozo_capture && ozoFileSourceEnable) {
			sp<OzoAudioFileSource> oSource = new OzoAudioFileSource(
					attr.source,
					clientIdentity,
					sourceSampleRate,
					audioParams->num_input_channels,
					outSampleRate,
					selectedDeviceId);
			audioSource = reinterpret_cast<AudioSource* > (oSource.get());
			err = oSource->initCheck();
		} else {
			sp<AudioSource> aSource = AVFactory::get()->createAudioSource(
					&attr,
					clientIdentity,
					sourceSampleRate,
					use_ozo_capture ? audioParams->num_input_channels : channels,
					outSampleRate,
					selectedDeviceId,
					selectedMicDirection,
					selectedMicFieldDimension);
			audioSource = aSource;
			err = audioSource->initCheck();
			audioSourceNode = aSource;
		}

		return err;
	} else {
		UNUSED(attr);
		UNUSED(clientIdentity);
		UNUSED(sourceSampleRate);
		UNUSED(channels);
		UNUSED(outSampleRate);
		UNUSED(selectedDeviceId);
		UNUSED(selectedMicDirection);
		UNUSED(selectedMicFieldDimension);
		UNUSED(audioEncoder);
		UNUSED(outputFormat);
		UNUSED(ozoAudioParams);
		UNUSED(audioSource);
		UNUSED(audioSourceNode);
		UNUSED(ozoFileSourceEnable);
		UNUSED(use_ozo_capture);
		return OK;
    }
}

void MediaImpl::setOzoAudioFormat(
        int32_t &channels,
        int32_t samplerate,
        void *ozoAudioParams,
        IMediaCodecEventListener* &codecEventListener,
        bool use_ozo_capture,
        bool &ozoBrandEnabled,
        int32_t &audioBitRate,
        sp<AudioSource> &audioSource,
        IMediaCodecEventListener *ozoTuneWriter,
        sp<IMediaRecorderClient> &listener,
        sp<AMessage> &format) {
	if(kSupportOZO){
		ozoBrandEnabled = false;
		struct OzoAudioParamsStagefright *audioParams =
			(OzoAudioParamsStagefright *)ozoAudioParams;

		if (use_ozo_capture) {
			bool monoOutput = (channels == 1);
			if (monoOutput && audioParams->encoding_mode == "ozoaudio") {
				audioParams->encoding_mode = "ls";
			}
			else if (audioParams->encoding_mode == "ozoaudio")
				ozoBrandEnabled = use_ozo_capture;

			audioBitRate = (monoOutput) ? std::max(audioBitRate, 128000) : std::max(audioBitRate, 256000);

			sp<MetaData> micFormat = audioSource->getFormat();
			channels = OzoAudioInitMessageStagefright(*audioParams, ozoTuneWriter ? true : false,
				format, micFormat, samplerate);

			// Codec event listener needed
			if (OzoAudioNeedListenerStagefright(*audioParams))
				codecEventListener = new OzoCodecEventListener(listener);
		}
	} else {
		UNUSED(channels);
		UNUSED(samplerate);
		UNUSED(ozoAudioParams);
		UNUSED(codecEventListener);
		UNUSED(use_ozo_capture);
		UNUSED(ozoBrandEnabled);
		UNUSED(audioBitRate);
		UNUSED(audioSource);
		UNUSED(ozoTuneWriter);
		UNUSED(listener);
		UNUSED(format);
	}
}

void MediaImpl::setOzoAudioCodecInfo(
        sp<MediaCodecSource> audioEncoderSource,
        IMediaCodecEventListener *ozoTuneWriter,
        IMediaCodecEventListener* codecEventListener) {
	if(kSupportOZO){
		if (codecEventListener)
			audioEncoderSource->setCodecEventListener(codecEventListener);
		if (ozoTuneWriter)
			audioEncoderSource->setCodecBufferPacketizer(ozoTuneWriter);
	} else {
		UNUSED(audioEncoderSource);
		UNUSED(ozoTuneWriter);
		UNUSED(codecEventListener);
	}
}

void MediaImpl::tryOzoAudioConfigureImpl(const sp<AMessage> &format,
        AString &mime, void* &oZoPlayCtr) {
	if(kSupportOZO){
		ALOGV("tryOzo audio configureImpl");
		OzoPlaybackController* playCtr = NULL;
		tryOzoAudioConfigure(format, mime, playCtr);
		oZoPlayCtr = playCtr;
	} else {
		UNUSED(format);
		UNUSED(mime);
		UNUSED(oZoPlayCtr);
	}
}

void MediaImpl::ozoPlayCtrlSignal(sp<MediaCodec> &codec, void* &oZoPlayCtr) {
	if(kSupportOZO){
		ALOGV("ozo playCtrl signal");
		OzoPlaybackController* playCtr = (OzoPlaybackController*)oZoPlayCtr;
		if (playCtr)
			playCtr->signal(codec);
	} else {
		UNUSED(codec);
		UNUSED(oZoPlayCtr);
	}
}

void MediaImpl::destroyOzoPlayCtrl(void* &oZoPlayCtr) {
	if(kSupportOZO){
		ALOGD("destroy ozo playCtrl");

		if (oZoPlayCtr) {
			delete (OzoPlaybackController*)oZoPlayCtr;
			oZoPlayCtr = NULL;
		}
	} else {
	    UNUSED(oZoPlayCtr);
	}
}

//#ifdef DOLBY_ENABLE
/*
void MediaImpl::dolbySetParameterDPE(const std::shared_ptr<Track> &track,
        int param, int valueInt) {
    ALOGD("dolby set parameter for DPE");
    switch (param) {
    case PROCESSED_TYPE: {
        int32_t processed = static_cast<int32_t>(valueInt);
        if (track->mProcessedType != processed) {
            track->mProcessedType = processed;
            ALOGD("setParameter(DAP, process type, %d)", processed);
            track->prepareForDownmix();
        }
    } break;
    default:
       LOG_ALWAYS_FATAL("setParameter DAP: bad param %d", param);
    }
}*/

/* Called in DownmixerBufferProvider */
/*
bool MediaImpl::dolbyCreateEffect(audio_channel_mask_t inputChannelMask,
        int32_t processedType, sp<EffectsFactoryHalInterface> &effectsFactory,
        int32_t sessionId, sp<EffectHalInterface> &downmixInterface) {

    bool isDapReady = false;
#ifdef DOLBY_ATMOS_GAME
    ALOGD("dolby create effect");
    if (sIsDapMultichannelCapable &&
        inputChannelMask == AUDIO_CHANNEL_OUT_5POINT1POINT2 &&
        processedType == ATMOS_GAME_512_ACTIVE) {
        ALOGD("DownmixerBufferProvider() create DAP effect");
        if (effectsFactory->createEffect(&sDapFxDesc.uuid,
                                          sessionId,
                                          SESSION_ID_INVALID_AND_IGNORED,
                                          AUDIO_PORT_HANDLE_NONE,
                                          &downmixInterface) != 0) {
            ALOGE("DownmixerBufferProvider() error creating DAP effect");
            downmixInterface.clear();
        } else {
            isDapReady = true;
        }
    }
#else
    UNUSED(inputChannelMask);
    UNUSED(processedType);
    UNUSED(effectsFactory);
    UNUSED(sessionId);
    UNUSED(downmixInterface);
#endif
	return isDapReady;
}
*/
/*static*/ // bool MediaImpl::sIsDapMultichannelCapable = false;
/*static*/  //effect_descriptor_t MediaImpl::sDapFxDesc;

/* Called in DownmixerBufferProvider */
/*
void MediaImpl::dolbyDapInit(uint32_t numEffects,
        sp<EffectsFactoryHalInterface> &effectsFactory) {
#ifdef DOLBY_ATMOS_GAME
    ALOGD("DownmixerBufferProvider dolby init ");
    for (uint32_t i = 0 ; i < numEffects ; i++) {
        if (effectsFactory->getDescriptor(i, &sDapFxDesc) == 0) {
            ALOGV("effect %d is called %s, type %x", i, sDapFxDesc.name, sDapFxDesc.type.timeLow);
            if (memcmp(&sDapFxDesc.type, &EFFECT_SL_IID_GAME_DAP, sizeof(effect_uuid_t)) == 0) {
                ALOGI("found effect \"%s\" from %s",
                        sDapFxDesc.name, sDapFxDesc.implementor);
                sIsDapMultichannelCapable = true;
                break;
            }
        }
    }
    ALOGW_IF(!sIsDapMultichannelCapable, "unable to find DAP effect");
#else
    UNUSED(numEffects);
    UNUSED(effectsFactory);
#endif
}
*/

/* Called in ACodec */
bool MediaImpl::dolbyDlbGenClass1Check() {
#ifdef DOLBY_AC4_SPLIT_SEC
    ALOGD("dolby DlbGenClass1 Check");
    DlbGenClass1* pDlbGenClass1 = new DlbGenClass1();
    bool res = pDlbGenClass1->check();
    delete pDlbGenClass1;
    if (!res) {
        ALOGE("security check failed");
        return OK;    // Security check fails but still returns OK
    } else {
        return BAD_VALUE;
    }
#else
    return BAD_VALUE;
#endif
}

/* Called in ACodec */
void MediaImpl::dolbyOMXNodeSetParameter(sp<IOMXNode> &mOMXNode) {
#ifdef DOLBY_AC4_SPLIT_SEC
    ALOGD("dolby OMXNode setParameter");
    OMX_AUDIO_PARAM_ANDROID_AC4TBL tbl;
    InitTblOMXParams(&tbl);

    TableXInit *A_OBJ = new TableXInit(AC4_TABLE_SEC_FRS_CODE,
            AC4_TABLE_SEC_FRS_MASK_VAL);

    TableXInit *B_OBJ = new TableXInit(AC4_TABLE_SEC_MDD_MAX_FRAM,
            AC4_TABLE_SEC_MMF_MASK_VAL);

    TableXInit *C_OBJ = new TableXInit(AC4_TABLE_SEC_MDD_MAX_INST,
            AC4_TABLE_SEC_MMI_MASK_VAL);

    A_OBJ->init();
    B_OBJ->init();
    C_OBJ->init();

    tbl.seedA = A_OBJ->getSeed();
    tbl.seedB = B_OBJ->getSeed();
    tbl.seedC = C_OBJ->getSeed();

    tbl.sizeA = A_OBJ->getSize();
    tbl.sizeB = B_OBJ->getSize();
    tbl.sizeC = C_OBJ->getSize();

    tbl.idA = A_OBJ->getTableID();
    tbl.idB = B_OBJ->getTableID();
    tbl.idC = C_OBJ->getTableID();

    tbl.maskA = A_OBJ->getMaskVal();
    tbl.maskB = B_OBJ->getMaskVal();
    tbl.maskC = C_OBJ->getMaskVal();

    memcpy (tbl.bufferA, A_OBJ->getBuffer(), LUT_BUFFER_SIZE);
    memcpy (tbl.bufferB, B_OBJ->getBuffer(), TABLE_B_C_U8_SZ);
    memcpy (tbl.bufferC, C_OBJ->getBuffer(), TABLE_B_C_U8_SZ);

    mOMXNode->setParameter(
            (OMX_INDEXTYPE)OMX_IndexParamAudioAndroidAc4Tbl, &tbl, sizeof(tbl));

    delete A_OBJ;
    delete B_OBJ;
    delete C_OBJ;
#else
    UNUSED(mOMXNode);
#endif
}
// DOLBY_END

void MediaImpl::setPlaybackCaptureAllowed(bool &playbackCaptureAllowed,
        void* packages) {
    ALOGD("%s: playbackCaptureAllowed %d", __func__, playbackCaptureAllowed);
#ifdef MEDIA_PROJECTION
    if (!playbackCaptureAllowed){
        for (const auto& package : *(std::vector<Package>*)packages) {
            if (isInWhiteList(package.name)) {
                playbackCaptureAllowed = true;
                break;
            }
        }
    }
#else
    #define UNUSED(x) (void)x
    UNUSED(packages);
#endif
}

static String16 resolveCallingPackage(PermissionController& permissionController,
        const std::optional<String16> opPackageName, uid_t uid) {
    if (opPackageName.has_value() && opPackageName.value().size() > 0) {
        return opPackageName.value();
    }
    // In some cases the calling code has no access to the package it runs under.
    // For example, code using the wilhelm framework's OpenSL-ES APIs. In this
    // case we will get the packages for the calling UID and pick the first one
    // for attributing the app op. This will work correctly for runtime permissions
    // as for legacy apps we will toggle the app op for all packages in the UID.
    // The caveat is that the operation may be attributed to the wrong package and
    // stats based on app ops may be slightly off.
    Vector<String16> packages;
    permissionController.getPackagesForUid(uid, packages);
    if (packages.isEmpty()) {
        ALOGE("No packages for uid %d", uid);
        return String16();
    }
    return packages[0];
}

bool MediaImpl::blockWhenInvisible(uid_t uid, String16& pkgName) {
    bool blockMode = false;
    char invisibleState[PROPERTY_VALUE_MAX] = { 0 };
    property_get("persist.sys.invisible_mode", invisibleState, "0");
    if (!strncmp(invisibleState, "1", 1)) {
        if (multiuser_get_app_id(uid) == AID_SYSTEM) {
            PermissionController permissionController;
            if (!String16("com.miui.screenrecorder").compare(resolveCallingPackage(
                permissionController, pkgName, uid))) {
                blockMode = true;
            }
        } else {
            AppOpsManager appOps;
            PermissionController permissionController;
            blockMode = appOps.noteOp(AppOpsManager::OP_RECORD_AUDIO, uid, resolveCallingPackage(
                    permissionController, pkgName, uid)) == AppOpsManager::MODE_IGNORED;
        }
    }
    if (blockMode) {
        ALOGE("MIUILOG-RecordAudio Request denied for invisible mode");
    }
    // END
    return blockMode;
}

//MIUI ADD FOR ONETRACK RBIS START
void MediaImpl::sendFrameRate(int frameRate){
    ALOGD("MediaImpl::sendFrameRate: %d", frameRate);
    mediacodec_frameRate = frameRate;
}

void MediaImpl::sendWidth(int32_t mVideoWidth){
    mediacodec_width = mVideoWidth;
}

void MediaImpl::sendHeight(int32_t mVideoHeight){
    mediacodec_height = mVideoHeight;
}

void MediaImpl::sendPackageName(const char* packageName){
    memset(mediacodec_packgeName, '\0', sizeof(mediacodec_packgeName));
    memcpy(mediacodec_packgeName, packageName, strlen(packageName)+1);
    ALOGD("MediaImpl::sendPackageName: %s", mediacodec_packgeName);
}

void MediaImpl::sendDolbyVision(const char* name){
    char valueSupport[PROPERTY_VALUE_MAX] = {0};
    if (property_get(FLAG_DOLBYVISION_SUPPORT, valueSupport, NULL)) {
        if(!strcmp(name, MEDIA_MIMETYPE_VIDEO_DOLBY_VISION/*video/dolby-vision*/)){
            mediacodec_dolbyVision = 1;
        }else{
            mediacodec_dolbyVision = 0;
        }
    } else {
        mediacodec_dolbyVision = -1;
    }
}

void MediaImpl::sendMine(const char* mime){
    memset(mediacodec_mime, '\0', sizeof(mediacodec_mime));
    memcpy(mediacodec_mime,mime,strlen(mime)+1);
    ALOGD("MediaImpl::sendMine: %s", mediacodec_mime);
}

void MediaImpl::sendHdrInfo(int32_t fHdr){
    mediacodec_hdr = fHdr;
}

void MediaImpl::sendFrameRateFloatCal(int frameRateFloatCal){
    mediacodec_frameRateCal = frameRateFloatCal;
}

void MediaImpl::updateFrcAieAisState(){
    char valueSupport[PROPERTY_VALUE_MAX] = {0};
    char valueOn[PROPERTY_VALUE_MAX] = {0};
    //FRC
    if (property_get(FLAG_FRC_SUPPORT, valueSupport, NULL)){
        if (property_get(VPP_APP_CONTROL_FRC, valueOn, NULL)
            && (!strcmp("1", valueSupport) || !strcmp("true", valueSupport))
            && (!strcmp("1", valueOn) || !strcmp("true", valueOn))) {
            mediacodec_frcState = 1;
            ALOGD("FRC switch on: valueSupport : %s, valueOn : %s", valueSupport, valueOn);
        }else{
            mediacodec_frcState = 0;
            ALOGD("FRC switch off: valueSupport : %s, valueOn : %s", valueSupport, valueOn);
        }
    }else{
        mediacodec_frcState = -1;
        ALOGD("FRC not support: valueSupport : %s", valueSupport);
    }
    //AIE
    if (property_get(FLAG_AIE_SUPPORT, valueSupport, NULL)){
        if (property_get(VPP_APP_CONTROL_AIE, valueOn, NULL)
            && (!strcmp("1", valueSupport) || !strcmp("true", valueSupport))
            && (!strcmp("1", valueOn) || !strcmp("true", valueOn))) {
            mediacodec_aieState = 1;
            ALOGD("AIE switch on: valueSupport : %s, valueOn : %s", valueSupport, valueOn);
        }else{
            mediacodec_aieState = 0;
            ALOGD("AIE switch off: valueSupport : %s, valueOn : %s", valueSupport, valueOn);
        }
    }else{
        mediacodec_aieState = -1;
        ALOGD("AIE not support: valueSupport : %s", valueSupport);
    }
    //AIS
    if (property_get(FLAG_AIS_SUPPORT, valueSupport, NULL)){
        if (property_get(VPP_APP_CONTROL_AIS, valueOn, NULL)
            && (!strcmp("1", valueSupport) || !strcmp("true", valueSupport))
            && (!strcmp("1", valueOn) || !strcmp("true", valueOn))) {
            mediacodec_aisState = 1;
            ALOGD("AIS switch on: valueSupport : %s, valueOn : %s", valueSupport, valueOn);
        }else{
            mediacodec_aisState = 0;
            ALOGD("AIS switch off: valueSupport : %s, valueOn : %s", valueSupport, valueOn);
        }
    }else{
        mediacodec_aisState = -1;
        ALOGD("AIS not support: valueSupport : %s", valueSupport);
    }
}

void MediaImpl::sendVideoPlayData(){
    ALOGD("%s, start", __func__);
    auto msg_send = (xlog_msg_send *)calloc(1, sizeof(struct xlog_msg_send));
    if(msg_send != nullptr){
        msg_send->packgeName = strdup(mediacodec_packgeName);
        if(nullptr == msg_send->packgeName) {
            ALOGE("malloc memory fail !");
            goto exit;
        }
        msg_send->mime = strdup(mediacodec_mime);
        if(nullptr == msg_send->mime) {
            ALOGE("malloc memory fail !");
            goto exit;
        }
        ALOGD("MediaImpl::sendVideoPlayData::mediacodec_mime : %s", mediacodec_mime);
        ALOGD("MediaImpl::sendVideoPlayData::mediacodec_packgeName : %s", mediacodec_packgeName);
        ALOGD("MediaImpl::sendVideoPlayData::mine : %s", msg_send->mime);
        ALOGD("MediaImpl::sendVideoPlayData::packgeName : %s", msg_send->packgeName);
        msg_send->height = mediacodec_height;
        msg_send->width = mediacodec_width;
        msg_send->frameRate = mediacodec_frameRate;
        msg_send->hdr = mediacodec_hdr;
        msg_send->frameRateCal = mediacodec_frameRateCal;
        msg_send->frcState = mediacodec_frcState;
        msg_send->aieState = mediacodec_aieState;
        msg_send->aisState = mediacodec_aisState;
        msg_send->dolbyVision = mediacodec_dolbyVision;
        send_video_play_to_xlog(msg_send);
    }else{
        ALOGE("alloc memory fail !");
        return;
    }
exit:
    if(msg_send != nullptr){
        if(msg_send->packgeName != nullptr){
            free(msg_send->packgeName);
        }
        if(msg_send->mime != nullptr){
            free(msg_send->mime);
        }
        free(msg_send);
    }
}
//MIUI ADD FOR ONETRACK RBIS END

extern "C" IMediaStub* create() {
    return new MediaImpl;
}

extern "C" void destroy(IMediaStub* impl) {
    delete impl;
}

}
