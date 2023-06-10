/*
  Copyright (C) 2018 Nokia Corporation.
  This material, including documentation and any related
  computer programs, is protected by copyright controlled by
  Nokia Corporation. All rights are reserved. Copying,
  including reproducing, storing,  adapting or translating, any
  or all of this material requires the prior written consent of
  Nokia Corporation. This material also contains confidential
  information which may not be disclosed to others without the
  prior written consent of Nokia Corporation.
*/
#define LOG_NDEBUG 0
#define LOG_TAG "OzoEncApp"
#include <fcntl.h>
#include <utils/String16.h>
#include <binder/ProcessState.h>
#include <media/stagefright/foundation/ADebug.h>

#include "audio.h"
#include "StagefrightRecorder.h"

#include "OzoAudioWaveFileSource.h"
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaCodecSource.h>
#include <media/stagefright/MPEG4Writer.h>
#include <media/stagefright/MetaData.h>
#include <media/mediaplayer.h>
#include <media/mediarecorder.h>
#include <OMX_Audio.h>


#if LOG_NDEBUG
#define UNUSED_UNLESS_VERBOSE(x) (void)(x)
#else
#define UNUSED_UNLESS_VERBOSE(x)
#endif

enum {
    OZOAUDIO_WNR_EVENT = 0x7F100001
};

enum ozoencapp_states {
    OZOENCAPP_STATE_ERROR        = 0,
    OZOENCAPP_IDLE               = 1 << 0,
    OZOENCAPP_INITIALIZED        = 1 << 1,
    OZOENCAPP_PREPARING          = 1 << 2,
    OZOENCAPP_PREPARED           = 1 << 3,
    OZOENCAPP_STARTED            = 1 << 4,
    OZOENCAPP_PAUSED             = 1 << 5,
    OZOENCAPP_STOPPED            = 1 << 6,
    OZOENCAPP_PLAYBACK_COMPLETE  = 1 << 7
};

using namespace android;

static void
usage(const char *name)
{
    fprintf(stdout, "Encodes WAVE file into OZO Audio file\n");
    fprintf(stdout, "Usage: %s <options> <input-wave-file> <output-mp4-file>\n", name);
    fprintf(stdout, "\nAvailable options:\n\n");
    fprintf(stdout, "-device <x>        : Capturing device ID\n");
    fprintf(stdout, "-encoding-mode <x> : Encoding mode (ozoaudio, ls, ...)");
    fprintf(stdout, "-bitrate <x>       : Encoding bitrate (default: 256000)");
    fprintf(stdout, "-focus <x>         : Focus capturing direction (front, back, off)\n");
    fprintf(stdout, "-delay <x>         : Encoder delay in frames\n");
    fprintf(stdout, "-wait <x>          : Encoder processing wait time in sec\n");
    fprintf(stdout, "-flags <x>         : Encoder init flags\n");
    fprintf(stdout, "-channels <x>      : Encoder output channel count\n");
    fprintf(stdout, "-use-mediacodec    : Use Ozo encoder directly via MediaCodecSource API\n");
    fprintf(stdout, "-use-tune          : Enable Ozo Audio Tune file creation\n");
    fprintf(stdout, "-no-wnr            : Disable wind noise reduction\n");
}

typedef struct {

    const char *focus_direction;
    const char *device_id;
    const char *encoding_mode;
    int32_t channels;
    int32_t delay;
    int64_t flags;
    int32_t bitrate;
    int32_t wait;

    bool use_mediacodec;
    bool use_post;
    bool use_wnr;

} OzoAppConfig;

class OzoAppListener: public BnMediaRecorderClient
{
public:
    OzoAppListener() :
        mCurrentState(OZOENCAPP_IDLE)
    {
    }

    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2) {

        UNUSED_UNLESS_VERBOSE(ext1);
        UNUSED_UNLESS_VERBOSE(ext2);

        ALOGV("MediaPlayer::notify(): %i %i %i", msgType, ext1, ext2);

        switch (msgType) {
            case MEDIA_NOP: // interface test message
            break;
            case MEDIA_PREPARED:
            ALOGV("MediaPlayer::notify() prepared");
            mCurrentState = OZOENCAPP_PREPARED;
            break;
            case MEDIA_DRM_INFO:
            ALOGV("MediaPlayer::notify() MEDIA_DRM_INFO(%d, %d, %d)", msgType, ext1, ext2);
            break;
            case MEDIA_PLAYBACK_COMPLETE:
            ALOGV("playback complete");
            if (mCurrentState == OZOENCAPP_IDLE) {
                ALOGE("playback complete in idle state");
            }
            mCurrentState = OZOENCAPP_PLAYBACK_COMPLETE;
            break;
            case MEDIA_ERROR:
                // Always log errors.
                // ext1: Media framework error code.
                // ext2: Implementation dependant error code.
            ALOGE("error (%d, %d)", ext1, ext2);
            mCurrentState = OZOENCAPP_STATE_ERROR;
            break;
            case MEDIA_INFO:
                // ext1: Media framework error code.
                // ext2: Implementation dependant error code.
            if (ext1 != MEDIA_INFO_VIDEO_TRACK_LAGGING) {
                ALOGW("info/warning (%d, %d)", ext1, ext2);
            }
            break;
            case MEDIA_SEEK_COMPLETE:
            ALOGV("Received seek complete");
            break;
            case MEDIA_BUFFERING_UPDATE:
            ALOGV("buffering %d", ext1);
            break;
            case MEDIA_SET_VIDEO_SIZE:
            ALOGV("New video size %d x %d", ext1, ext2);
            break;
            case MEDIA_NOTIFY_TIME:
            ALOGV("Received notify time message");
            break;
            case MEDIA_TIMED_TEXT:
            ALOGV("Received timed text message");
            break;
            case MEDIA_SUBTITLE_DATA:
            ALOGV("Received subtitle data message");
            break;
            case MEDIA_META_DATA:
            ALOGV("Received timed metadata message");
            break;
            case OZOAUDIO_WNR_EVENT:
            {
                uint8_t windLevel = (((unsigned int) ext2) >> 1) & 255;
                const char *windDetected = ext2 & 1 ? "wind detected" : "no wind";
                ALOGD("Received WNR event: (%d, %d, %d, (%i, %s))", msgType, ext1, ext2, windLevel, windDetected);
                break;
            }
            default:
            ALOGV("unrecognized message: (%d, %d, %d)", msgType, ext1, ext2);
            break;
        }
    }

    ozoencapp_states mCurrentState;
};

class OzoAppRecordListener: public MediaRecorderListener
{
public:
    OzoAppRecordListener():
    mCurrentState(OZOENCAPP_IDLE)
    {}

    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2)
    {
        ALOGV("OzoAppRecordListener::notify(): %i %i %i", msgType, ext1, ext2);
    }

    ozoencapp_states mCurrentState;
};

static int
parse_commandline(char **in_file, char **out_file, OzoAppConfig *config, int argc, char *argv[])
{
    argc--; argv++;
    while (argc > 2)
    {
        if (!strcmp(*argv, "-use-mediacodec"))
            config->use_mediacodec = true;

        else if (!strcmp(*argv, "-use-post"))
            config->use_post = true;

        else if (!strcmp(*argv, "-no-wnr"))
            config->use_wnr = false;

        else if (!strcmp(*argv, "-encoding-mode"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            config->encoding_mode = *argv;
        }

        else if (!strcmp(*argv, "-device"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            if (!strcmp(*argv, "none")) {
                config->device_id = "";
            } else {
                config->device_id = *argv;
            }
        }

        else if (!strcmp(*argv, "-focus"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            if (strcmp(*argv, "disabled"))
                config->focus_direction = *argv;
        }

        else if (!strcmp(*argv, "-channels"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            config->channels = atoi(*argv);
        }

        else if (!strcmp(*argv, "-bitrate"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            config->bitrate = (int32_t) atoi(*argv);
        }

        else if (!strcmp(*argv, "-delay"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            config->delay = atoi(*argv);
        }

        else if (!strcmp(*argv, "-wait"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            config->wait = atoi(*argv);
        }

        else if (!strcmp(*argv, "-flags"))
        {
            /*-- Move pointer past the value. --*/
            argv++; argc--;

            config->flags = atol(*argv);
        }

        /*-- Unidentified switch. --*/
        else {
            fprintf(stderr, "Unknown switch: %s\n", *argv);
            return (1);
        }

        /*-- Move pointer past the value. --*/
        argv++; argc--;
    }

    *in_file = *argv;
    argv++;
    *out_file = *argv;

    return (0);
}

template<typename T>
static void
setFocus(T *recorder, const char *focus_direction, bool runtime=false)
{
    char param_buffer[512];

    if (!strcmp(focus_direction, "off")) {
        sprintf(param_buffer, "vendor.ozoaudio.generic.value=focus=off");
        if (!runtime)
            CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);
        else
            CHECK_EQ(recorder->setOzoRunTimeParameters(String8(param_buffer)), (status_t) OK);
    }
    else {
        if (!strcmp(focus_direction, "front"))
            sprintf(param_buffer, "vendor.ozoaudio.generic.value=focus-azimuth=0.0");
        else
            sprintf(param_buffer, "vendor.ozoaudio.generic.value=focus-azimuth=180.0");
        if (!runtime)
            CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);
        else
            CHECK_EQ(recorder->setOzoRunTimeParameters(String8(param_buffer)), (status_t) OK);

        sprintf(param_buffer, "vendor.ozoaudio.generic.value=focus=on");
        if (!runtime)
            CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);
        else
            CHECK_EQ(recorder->setOzoRunTimeParameters(String8(param_buffer)), (status_t) OK);

        sprintf(param_buffer, "vendor.ozoaudio.generic.value=focus-elevation=0.0");
        if (!runtime)
            CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);
        else
            CHECK_EQ(recorder->setOzoRunTimeParameters(String8(param_buffer)), (status_t) OK);

        sprintf(param_buffer, "vendor.ozoaudio.generic.value=zoom=3.0");
        if (!runtime)
            CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);
        else
            CHECK_EQ(recorder->setOzoRunTimeParameters(String8(param_buffer)), (status_t) OK);
    }
}

template<typename T>
static void
setInitParameters(T *recorder, const OzoAppConfig *config, int32_t samplerate, int32_t channels)
{
    char param_buffer[512];

    CHECK_EQ(recorder->setParameters(String8("stagefrightrecorder.ozoaudio.file-source-enable=1")), (status_t) OK);
    sprintf(param_buffer, "stagefrightrecorder.ozoaudio.input-channels=%d", channels);
    CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);

    sprintf(param_buffer, "audio-param-encoding-bitrate=%d", config->bitrate);
    CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);

    sprintf(param_buffer, "audio-param-number-of-channels=%d", config->channels);
    CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);

    sprintf(param_buffer, "audio-param-sampling-rate=%d", samplerate);
    CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);
    //CHECK_EQ(recorder->setParameters(String8("max-duration=11793000")), (status_t) OK);
    //CHECK_EQ(recorder->setParameters(String8("max-filesize=8064")), (status_t) OK);

    if (config->encoding_mode) {
        sprintf(param_buffer, "vendor.ozoaudio.mode.value=%s", config->encoding_mode);
        CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);
    }

    sprintf(param_buffer, "vendor.ozoaudio.device.value=%s", config->device_id);
    CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);

    if (config->use_wnr) {
        sprintf(param_buffer, "vendor.ozoaudio.generic.value=wnr=on");
        CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);

        sprintf(param_buffer, "vendor.ozoaudio.wnr-notification.value=on");
        CHECK_EQ(recorder->setParameters(String8(param_buffer)), (status_t) OK);
    }

    if (config->focus_direction)
        setFocus<T>(recorder, config->focus_direction);
}

template<typename T, typename U>
static void
codecExec(T *recorder, const sp<U> listener, const OzoAppConfig *config, int fd,
    uint32_t &focus_idx, int32_t samplerate, int32_t channels, int postFd)
{
    CHECK_EQ(recorder->setListener(listener), (status_t) OK);
    CHECK_EQ(recorder->setAudioSource(AUDIO_SOURCE_MIC), (status_t) OK);
    CHECK_EQ(recorder->setOutputFormat(OUTPUT_FORMAT_MPEG_4), (status_t) OK);
    CHECK_EQ(recorder->setAudioEncoder(AUDIO_ENCODER_AAC), (status_t) OK);
    CHECK_EQ(recorder->setOutputFile(fd), (status_t) OK);
    close(fd);

    if (postFd != -1) {
        CHECK_EQ(recorder->setOzoAudioTuneFile(postFd), (status_t) OK);
        close(postFd);
    }

    setInitParameters<T>(recorder, config, samplerate, channels);

    CHECK_EQ(recorder->prepare(), (status_t) OK);
    CHECK_EQ(recorder->start(), (status_t) OK);

    int loop = 0;
    // 6 seconds is enough to process whole input file
    while(loop++ < config->wait) {
        usleep(1000000);

        CHECK_NE(listener->mCurrentState, OZOENCAPP_STATE_ERROR);
#if 0
        setFocus(recorder, focus_directions[focus_idx % 3], true);
#endif
        focus_idx++;

        if (loop == 10) {
            char param_buffer[512];
            sprintf(param_buffer, "vendor.ozoaudio.generic.value=zoom=5.0");
            if (recorder->setOzoRunTimeParameters(String8(param_buffer)) != (status_t) OK) {
                fprintf(stderr, "Unable to set runtime parameters\n"); fflush(stdout);
            }
            else {
                fprintf(stderr, "Runtime parameters set\n"); fflush(stdout);
            }
        }

        fprintf(stdout, "Wait (%2is) ...\r", loop); fflush(stdout);
    }

    fprintf(stdout, "\nDone processing. Stopping...\n"); fflush(stdout);

    CHECK_EQ(recorder->stop(), (status_t) OK);

    fprintf(stdout, "\nRecording stopped\n"); fflush(stdout);
}

int
main(int argc, char* argv[])
{
    OzoAppConfig config;
    char *in_file, *out_file;

    uint32_t focus_idx = 0;
#if 0
    const char *focus_directions[] = {
        "front", "back", "off"
    };
#endif // 0

    config.focus_direction = nullptr;
    // Pixel 3 device ID
    config.device_id = "4238B2D5-0234-41C0-8E49-BB5D3FD474DC";
    config.encoding_mode = nullptr;
    config.channels = 2;
    config.delay = -1;
    config.flags = -1;
    config.use_mediacodec = false;
    config.use_post = false;
    config.use_wnr = true;
    config.bitrate = 256000;
    config.wait = 6;

    if (argc < 3) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (parse_commandline(&in_file, &out_file, &config, argc, argv))
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    android::ProcessState::self()->startThreadPool();

    int fd = open(out_file, O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        fprintf(stderr, "Couldn't open file %s\n", out_file);
        return EXIT_FAILURE;
    }

    int postdataFd = -1;
    if (config.use_post) {
        postdataFd = open("/data/local/tmp/postdata.dat", O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
        if (postdataFd < 0) {
            fprintf(stderr, "Couldn't open file %s\n", "postdata.dat");
            return EXIT_FAILURE;
        }
    }

    int32_t samplerate, channels, bitrateOzo;
    samplerate = 48000;
    channels = 3;
    bitrateOzo = 0;

    // Extract parameters from input source in order to initialize the encoding correctly
    sp<MediaSource> wavSource = new OzoAudioWaveFileSource(in_file);
    wavSource->start();
    {
        sp<MetaData> micFormat = wavSource->getFormat();
        wavSource = 0;

        micFormat->findInt32(kKeyChannelCount, &channels);
        micFormat->findInt32(kKeySampleRate, &samplerate);
        micFormat->findInt32(kKeyBitRate, &bitrateOzo);
    }

    if (config.channels == 1) {
        config.bitrate = 128000;
    }

    // Use Ozo encoder from MediaCodecSource directly
    if (config.use_mediacodec) {

        sp<ALooper> looper = new ALooper;
        looper->setName("ozoloop");
        looper->start();

        int32_t bitsPerSample = (bitrateOzo / samplerate) / channels;
        if (bitsPerSample == 0) {
            bitsPerSample = 16;
        }

        String8 parameters;
        sp<AMessage> format = new AMessage;
        format->setInt32("aac-profile", OMX_AUDIO_AACObjectLC);
        format->setInt32("max-input-size", 8192);
        format->setInt32("channel-count", channels);
        format->setInt32("sample-rate", samplerate);
        format->setInt32("bitrate", config.bitrate);
        format->setString("vendor.ozoaudio.device.value", config.device_id);
        format->setString("vendor.ozoaudio.mode.value", config.encoding_mode);
        format->setInt32("vendor.ozoaudio.sample-depth.value", bitsPerSample);
        if (config.focus_direction) {
            if (!strcmp(config.focus_direction, "off"))
                parameters += "focus=off";
            else {
                if (!strcmp(config.focus_direction, "front"))
                    parameters += "focus-azimuth=0.0";
                else
                    parameters += "focus-azimuth=180.0";

                parameters += ";focus=on";
                parameters += ";focus-elevation=0.0";
            }

            format->setString("vendor.ozoaudio.generic.value=", parameters.c_str());
        }

        //format->setString("mime", MEDIA_MIMETYPE_AUDIO_AAC);
        format->setString("mime", MEDIA_MIMETYPE_AUDIO_OZOAUDIO);

        wavSource = new OzoAudioWaveFileSource(in_file);
        sp<MediaCodecSource> ozoEncoder = MediaCodecSource::Create(looper, format, wavSource);

#if 1
        sp<MPEG4Writer> writer = new MPEG4Writer(fd);
        close(fd);

        fprintf(stdout, "Adding Ozo encoder to MPEG4Writer...\n");
        writer->addSource(ozoEncoder);

        const int64_t kDurationUs = 5000000LL;  // 5 seconds

        fprintf(stdout, "Limiting recording duration to %lldus...\n", kDurationUs);
        writer->setMaxFileDuration(kDurationUs);

        fprintf(stdout, "Starting MPEG4Writer...\n");
        CHECK_EQ((status_t) OK, writer->start());

        static bool state = false;
        while (!writer->reachedEOS())
            usleep(100000);

        (void) state;

        fprintf(stdout, "\nStopping MPEG4Writer...\n");
        status_t err = writer->stop();

        fprintf(stdout, "Closing...\n");

        if (err != OK && err != ERROR_END_OF_STREAM) {
            fprintf(stderr, "Record failed: %d\n", err);
            return EXIT_FAILURE;
        }
#else
        CHECK_EQ(ozoEncoder->start(), (status_t) OK);
        ALOGD("Started Ozo encoder");

        FILE *fp = fopen(out_file, "wb");
        if (fp == NULL) {
            fprintf(stderr, "Couldn't open file %s\n", out_file);
            return EXIT_FAILURE;
        }

        MediaBuffer *buffer;
        while (ozoEncoder->read(&buffer) == OK) {
            ALOGD("Encoded buffer size: %i", buffer->size());
            if (buffer->size() > 2)
                fwrite(buffer->data(), 1, buffer->size(), fp);
            buffer->release();
            buffer = NULL;
        }

        fclose(fp);
#endif

        ozoEncoder = 0;

        return EXIT_SUCCESS;
    }

    sp<OzoAppListener> listener = new OzoAppListener;
    StagefrightRecorder *recorder = new StagefrightRecorder(String16(in_file));

    CHECK_EQ(recorder->init(), (status_t) OK);
    codecExec<StagefrightRecorder, OzoAppListener>(
        recorder, listener, &config, fd, focus_idx, samplerate, channels, postdataFd
    );

    return EXIT_SUCCESS;
}
