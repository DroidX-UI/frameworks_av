/*
Copyright (C) 2019 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/


//#define LOG_NDEBUG 0
#define LOG_TAG "OzoDecoderWrapper"
#include <log/log.h>
#include <utils/Log.h>

#include <string>
#include <map>
#include <vector>

#include "./decoder.h"
#include "aacdecoder_lib.h"
#include "ozowidening/lwidening.h"
#include "ozowidening/lwidening_types.h"
#include "ozowidening/license.h"
#include "ozoaudio/utils.h"
#include "ozoaudiocommon/license_reader2.h"

#define MAX_CHANNEL_COUNT        8  /* maximum number of audio channels that can be decoded */
#define INT16_2_FLOAT_MULT       1.0f / 32768.0f

#define OUTPUT_WIDENING_DEVICE  "d265567e-bb6d-45bf-8ae3-324848a4626c"

/**
 * Round normalized float to 16-bit integer.
 */
#define ROUND_NORMFLOAT_2_SAMPLE16BIT(output, input) \
    float tmp = 32768 * input;                       \
    if ((tmp) > 32767.0) {                           \
        *(output) = 0x7fff;                          \
    }                                                \
    else if ((tmp) < -32768.0) {                     \
        *(output) = -0x8000;                         \
    }                                                \
    else if ((tmp) <= 0.0f) {                        \
        *(output) = (int16_t)(tmp - 0.5f);           \
    }                                                \
    else {                                           \
        *(output) = (int16_t)(tmp + 0.5f);           \
    }

namespace android {

static inline void float_2_int16(const float *input, int16_t *output, size_t len)
{
    for (size_t i = 0; i < len; i++, input++) {
        ROUND_NORMFLOAT_2_SAMPLE16BIT(output + i, *input);
    }
}

static std::string sampleFormat2String(ozoaudiocodec::SampleFormat format)
{
    if (format == ozoaudiocodec::SampleFormat::SAMPLE_16BIT)
        return "16-bit";
    else if (format == ozoaudiocodec::SampleFormat::SAMPLE_24BIT)
        return "24-bit";
    else if (format == ozoaudiocodec::SampleFormat::SAMPLE_NORMFLOAT)
        return "Normalized float";
    else if (format == ozoaudiocodec::SampleFormat::SAMPLE_NONE)
        return "Encoded";

    return "Unknown";
}

const char *const decompositionRender = "decomposition";

static bool gWideningLicenseSet = false;
static const char OZOPLAY_LICENSE_FILE[] = "/vendor/etc/ozoplaysdk.license";

/**
 * Audio decoder adapter implementation for Ozo decoder.
 * Encapsulates platform AAC decoder implementation.
 */
class OzoAudioAdapterAACDecoder : public ozoaudiodecoder::AudioDecoder
{
public:
    OzoAudioAdapterAACDecoder():
    mAACDecoder(NULL),
    mStreamInfo(NULL),
    mLogging(true)
    {
        ALOGD("Create platform AAC decoder");
        this->mAACDecoder = aacDecoder_Open(TT_MP4_ADIF, /* num layers */ 1);
        if (this->mAACDecoder != NULL) {
            this->mStreamInfo = aacDecoder_GetStreamInfo(this->mAACDecoder);
        }

        this->m_codecName = "aac";
    }

    virtual void destroy()
    {
        this->reset();
    }

    void logStreamInfo() {
        ALOGD("AAC stream info: Channel count: %i; Sample rate: %i %i; AAC decoder delay: %i",
            this->mStreamInfo->numChannels,
            this->mStreamInfo->sampleRate, this->mStreamInfo->frameSize,
            this->mStreamInfo->outputDelay);
    }

    virtual ozoaudiodecoder::AudioDecoderError init(const ozoaudiocodec::CodecData &data, ozoaudiodecoder::AudioDecoderConfig *config)
    {
        ALOGD("Init AAC decoder");

        // Multiple init not allowed
        if (!this->mAACDecoder || !this->mStreamInfo) {
            ALOGD("Multiple AAC adapter decoder initialization not allowed");
            return ozoaudiodecoder::AudioDecoderError::NOT_INITIALIZED;
        }

        UCHAR *inBuffer[1] = {data.data};
        UINT inBufferLength[1] = {data.data_size};
        auto err = aacDecoder_ConfigRaw(this->mAACDecoder, inBuffer, inBufferLength);
        if (err != AAC_DEC_OK) {
            ALOGD("Unable to decode decoder specific data, err: 0x%4.4x", err);
            return ozoaudiodecoder::AudioDecoderError::NOT_INITIALIZED;
        }

        aacDecoder_SetParam(this->mAACDecoder, AAC_PCM_MAX_OUTPUT_CHANNELS, -1);
        aacDecoder_SetParam(this->mAACDecoder, AAC_PCM_OUTPUT_CHANNEL_MAPPING, 0);

        int32_t sampleRate = 48000;
        int32_t numChannels = 2;
        OzoDecoderWrapper::readCSD(sampleRate, numChannels, data.data, data.data_size);
        if (numChannels == 0) {
            numChannels = 2;
        }

        config->frame_data.samples = NULL;
        config->frame_data.meta_data.n_meta_data = 0;
        config->frame_data.frame_size = -1;
        config->frame_length = 1024;
        config->frame_data.n_channels = numChannels;
        config->frame_data.sample_rate = sampleRate;
        config->frame_data.n_samples = config->frame_data.n_channels * config->frame_length;

        ALOGD("AudioDecoderConfig:");
        ALOGD(" - channels %d", config->frame_data.n_channels);
        ALOGD(" - sample-rate %d", config->frame_data.sample_rate);
        ALOGD(" - samples %d", config->frame_data.n_samples);
        ALOGD(" - frame-size %d", config->frame_data.frame_size);
        ALOGD(" - frame-length %d", config->frame_length);

        logStreamInfo();

        return ozoaudiodecoder::AudioDecoderError::OK;
    }

    virtual ozoaudiodecoder::AudioDecoderError decode(uint8_t *input, int32_t input_len, ozoaudiodecoder::AudioFrameInfo &frame_data)
    {
        UCHAR *inBuffer[1] = {input};
        UINT inBufferLength[1] = {static_cast<UINT>(input_len)};
        UINT bytesValid[1] = {static_cast<UINT>(input_len)};

        aacDecoder_Fill(this->mAACDecoder, inBuffer, inBufferLength, bytesValid);

        auto err = aacDecoder_DecodeFrame(this->mAACDecoder, (INT_PCM *) this->mDecOutput, 2048 * MAX_CHANNEL_COUNT, 0 /* flags */);
        if (err != AAC_DEC_OK) {
            ALOGD("AAC decoding failed, err: 0x%4.4x", err);
            return ozoaudiodecoder::AudioDecoderError::PROCESSING_ERROR;
        }

        frame_data.samples = this->mDecOutput;
        frame_data.meta_data.n_meta_data = 0;
        frame_data.n_channels = this->mStreamInfo->numChannels;
        frame_data.sample_rate = this->mStreamInfo->sampleRate;
        frame_data.n_samples = frame_data.n_channels * this->mStreamInfo->frameSize;
        frame_data.frame_size = frame_data.n_samples * sizeof(int16_t);

        // Limit logging only to first decoded frame
        if (this->mLogging)
            logStreamInfo();
        this->mLogging = false;

        return ozoaudiodecoder::AudioDecoderError::OK;
    }

    virtual ozoaudiodecoder::AudioDecoderError flush()
    {
        INT_PCM tmpOutBuffer[2048 * 2 * 2];

        // Flush the overlap-add buffer
        ALOGD("Flush AAC decoder");
        aacDecoder_DecodeFrame(this->mAACDecoder, tmpOutBuffer, sizeof(tmpOutBuffer) / sizeof(INT_PCM), AACDEC_FLUSH);
        this->mLogging = true;

        return ozoaudiodecoder::AudioDecoderError::OK;
    }

private:
    ~OzoAudioAdapterAACDecoder() {}

    void reset()
    {
        aacDecoder_Close(this->mAACDecoder);
        this->mAACDecoder = NULL;
    }

protected:
    HANDLE_AACDECODER mAACDecoder;
    CStreamInfo *mStreamInfo;
    uint8_t mDecOutput[2048 * MAX_CHANNEL_COUNT * 2];
    bool mLogging;
};


typedef std::map<ozoaudiodecoder::TrackType, std::string> TrackTypeMapper;
typedef std::map<ozoaudiodecoder::AudioRenderingGuidance, std::string> GuidanceTypeMapper;

static const TrackTypeMapper track_type_mapper = {
    {ozoaudiodecoder::TrackType::OZO_TRACK, "OZO Audio track"},
    {ozoaudiodecoder::TrackType::AMBISONICS_TRACK, "Ambisonics"}
};

static const GuidanceTypeMapper guidance_mapper = {
    {ozoaudiodecoder::AudioRenderingGuidance::NO_TRACKING, "Head tracking disabled"},
    {ozoaudiodecoder::AudioRenderingGuidance::HEAD_TRACKING, "Head tracking enabled"}
};

OzoDecoderWrapper::TrackData::TrackData(ozoaudiodecoder::TrackType track_type,
    ozoaudiodecoder::AudioRenderingGuidance guidance):
mAdapterDecoder(NULL)
{
    this->mTrackConfig.track_type = track_type;
    this->mTrackConfig.sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NONE;
    this->mTrackConfig.guidance = guidance;
}

OzoDecoderWrapper::TrackData::~TrackData()
{
    if (this->mAdapterDecoder)
        this->mAdapterDecoder->destroy();
    this->mAdapterDecoder = NULL;
}

bool
OzoDecoderWrapper::TrackData::init(uint8_t *config, int32_t size)
{
    // Double init not allowed
    if (this->mAdapterDecoder)
        return false;

    this->mAdapterDecoder = new OzoAudioAdapterAACDecoder();
    if (!this->mAdapterDecoder)
        return false;

    memset(&this->mTrackConfig.config_data, 0 , sizeof(this->mTrackConfig.config_data));

    // Initialize decoder configuration structures
    this->mTrackConfig.config_data.audio.data = config;
    this->mTrackConfig.config_data.audio.data_size = size;

    return true;
}


OzoDecoderWrapper::OzoDecoderWrapper():
mOzoHandle(NULL),
mTrack(NULL),
mWideningHandle(NULL)
{
    ALOGD("ozoaudiodecoder SDK version=%s", ozoaudiodecoder::OzoAudioMultiTrackDecoder::version());
    ALOGD("ozolswidening SDK version=%s", OzoLoudspeakerWidening_GetVersion());

    this->mStreamInfo.frameSize = -1;
    this->mStreamInfo.numChannels = -1;
    this->mStreamInfo.sampleRate = -1;
}

OzoDecoderWrapper::~OzoDecoderWrapper()
{
    this->reset();
}

/** Parse CSD, read sample rate and number of channels if found
*/
void OzoDecoderWrapper::readCSD(int32_t & sampleRate, int32_t & numChannels, const uint8_t *csd, size_t csd_size) {

    ALOGD("readCSD");
    ozoaudiodecoder::readCSD(csd, csd_size, sampleRate, numChannels);
}

void
OzoDecoderWrapper::reset()
{
    if (this->mOzoHandle)
        this->mOzoHandle->destroy();
    this->mOzoHandle = NULL;

    if (this->mTrack)
        delete this->mTrack;
    this->mTrack = NULL;

    if (this->mWideningHandle)
        OzoLoudspeakerWidening_Destroy(this->mWideningHandle);
    this->mWideningHandle = NULL;
}

bool
OzoDecoderWrapper::init(OzoDecoderConfigParams_t *params)
{
    this->reset();

    if (!this->mOzoHandle) {

        this->mOzoHandle = ozoaudiodecoder::OzoAudioMultiTrackDecoder::create();
        if (!this->mOzoHandle) {
            ALOGD("Unable to create Ozo decoder instance");
            return false;
        }

        if (!setOzoAudioSdkLicense())
            return false;

        this->mTrack = new TrackData(params->track_type, params->guidance);
        if (!this->mTrack) {
            ALOGD("Unable to create track decoder instance");
            return false;
        }

        if (!this->initMixerConfig(params->lsWidening))
            return false;

        if (!this->addTrack(this->mTrack, params->config[0], params->config_size[0]))
            return false;

        if (!this->initDecoder(params->lsWidening))
            return false;

        if (params->orientation.size() && !this->setOrientation(params->orientation))
            return false;

        return true;
    }

    return false;
}

bool
OzoDecoderWrapper::addTrack(TrackData *track, uint8_t *config, int32_t size)
{
    if (!track->init(config, size)) {
        ALOGE("Unable to initialize track");
        return false;
    }

    ALOGD("\nTrack details:");
    ALOGD("--------------");
    ALOGD("Track type: %s",  track_type_mapper.find(track->mTrackConfig.track_type)->second.c_str());
    ALOGD("Track input sample format: %s", sampleFormat2String(track->mTrackConfig.sample_format).c_str());
    ALOGD("Track rendering guidance: %s", guidance_mapper.find(track->mTrackConfig.guidance)->second.c_str());
    ALOGD("Audio config size: %i\n\n", track->mTrackConfig.config_data.audio.data_size);

    // Add track
    auto result = this->mOzoHandle->addTrack(track->mTrackConfig, this->mOzoMixerConfig, track->mAdapterDecoder);
    if (result != ozoaudiocodec::AudioCodecError::OK) {
        ALOGE("Unable to add track to decoder, error code: %i", result);
        return false;
    }

    return true;
}

bool
OzoDecoderWrapper::initMixerConfig(bool lsWidening)
{
    // Configure output signal from decoder
    if (lsWidening) {
        // Output for loudspeaker widening
        this->mOzoMixerConfig.n_out_channels = 4;
        this->mOzoMixerConfig.render_mode = decompositionRender;
        this->mOzoMixerConfig.sample_format = ozoaudiocodec::SampleFormat::SAMPLE_NORMFLOAT;
    }
    else {
        // Binaural output
        this->mOzoMixerConfig.n_out_channels = 2;
        this->mOzoMixerConfig.render_mode = ozoaudiodecoder::binauralRender;
        this->mOzoMixerConfig.sample_format = ozoaudiocodec::SampleFormat::SAMPLE_16BIT;
    }

    this->mOzoMixerConfig.layout = nullptr;
    this->mOzoMixerConfig.ch_layout = nullptr;

    ALOGD("\nMixer details:\n");
    ALOGD("----------------\n");
    ALOGD("Rendering mode: %s\n", this->mOzoMixerConfig.render_mode);
    ALOGD("Number of rendering channels: %i\n", this->mOzoMixerConfig.n_out_channels);
    ALOGD("Output channels layout: %s\n", this->mOzoMixerConfig.layout);
    ALOGD("Decoder output sample format: %s\n", sampleFormat2String(this->mOzoMixerConfig.sample_format).c_str());

    return true;
}

bool
OzoDecoderWrapper::initDecoder(bool lsWidening)
{
    auto result = this->mOzoHandle->init(this->mOzoMixerConfig);
    if (result != ozoaudiocodec::OK) {
        ALOGE("Failed to initialize Ozo decoder, error code: %i", result);
        return false;
    }

    ozoaudiodecoder::AudioFrameInfo info;
    if (this->mOzoHandle->getTrackInfo(-1, info) == ozoaudiocodec::OK) {
        this->mStreamInfo.numChannels = (lsWidening) ? info.n_channels / 2 : info.n_channels;
        this->mStreamInfo.sampleRate = info.sample_rate;

        if (this->mOzoHandle->getTrackInfo(0, info) == ozoaudiocodec::OK) {
            this->mStreamInfo.frameSize = info.frame_size;

            ALOGD("Output info: frame_size=%i, channels(out)=%i, channels(decoded)=%i, sample_rate=%i",
                this->mStreamInfo.frameSize, this->mStreamInfo.numChannels, info.n_channels,
                this->mStreamInfo.sampleRate);

            // Initialize widening for loudspeaker output if needed
            return (lsWidening) ? this->initLsWidening(this->mStreamInfo) : true;
        }
    }

    ALOGE("Unable to retrieve decoder output track info");

    return false;
}

bool OzoDecoderWrapper::initLsWidening(const OzoDecoderStreamInfo &streamInfo)
{
    if (!gWideningLicenseSet) {
        ozoaudiocodec::OzoLicenseReader license;

        if (!license.init(OZOPLAY_LICENSE_FILE)) {
            ALOGE("Unable to read Ozo widening license file %s", OZOPLAY_LICENSE_FILE);
            return false;
        }
        ALOGD("Ozo widening license data size: %zu", license.getSize());

        if (!OzoWidening_SetLicense(license.getBuffer(), license.getSize())) {
            ALOGE("Ozo widening license data not accepted");
            return false;
        }

        gWideningLicenseSet = true;
    }

    LWideningConfig wideningConfig;

    wideningConfig.flags = 0;
    wideningConfig.fs = streamInfo.sampleRate;
    wideningConfig.maxFrameSize = streamInfo.frameSize;
    wideningConfig.inputChannelCount = streamInfo.numChannels * 2;
    strcpy(wideningConfig.deviceNameId, OUTPUT_WIDENING_DEVICE);

    ALOGD("\nWidening details:\n");
    ALOGD("----------------\n");
    ALOGD("Sample rate: %d\n", wideningConfig.fs);
    ALOGD("Frame size: %i\n", wideningConfig.maxFrameSize);
    ALOGD("Input channels: %d\n", wideningConfig.inputChannelCount);
    ALOGD("Device ID: %s\n", wideningConfig.deviceNameId);

    auto returnCode = OzoLoudspeakerWidening_Create(&this->mWideningHandle, &wideningConfig);
    if (returnCode != Widening_ReturnCode::WIDENING_RETURN_SUCCESS) {
        ALOGD("Widening creation failed, error code: %d", returnCode);
        return false;
    }

    auto modes = OzoLoudspeakerWidening_ProcessingModesAvailable(this->mWideningHandle);

    ALOGD("Requesting widening mode: %s\n\n", modes.modeName[1]);
    returnCode = OzoLoudspeakerWidening_SetProcessingMode(this->mWideningHandle, modes.modeName[1]);
    if (returnCode != Widening_ReturnCode::WIDENING_RETURN_SUCCESS) {
        ALOGD("Unable to enable widening, error code: %d", returnCode);
        return false;
    }

    return true;
}

const OzoDecoderStreamInfo *
OzoDecoderWrapper::getStreamInfo() const
{
    return &this->mStreamInfo;
}

bool
OzoDecoderWrapper::decode(OzoDecoderDecodeParams_t *params, uint8_t *output, size_t *output_size)
{
    ozoaudiodecoder::OzoAudioDecoderDynamicData *inputData[OZO_MAX_DECODER_STREAMS],
        _inputData[OZO_MAX_DECODER_STREAMS];

    for (size_t i = 0; i < OZO_MAX_DECODER_STREAMS; i++) {
        inputData[i] = &_inputData[i];

        inputData[i]->enc_data.n_meta = 0;
        inputData[i]->enc_data.audio.data = params->data[i];
        inputData[i]->enc_data.audio.data_size = params->data_size[i];
    }

    ozoaudiodecoder::AudioFrameInfo frame_info;
    memset(&frame_info, 0, sizeof(ozoaudiodecoder::AudioFrameInfo));

    Mutex::Autolock autoLock(this->mLock);

    auto result = this->mOzoHandle->decode((const ozoaudiodecoder::OzoAudioDecoderDynamicData **const) inputData, frame_info);
    if (result != ozoaudiocodec::OK) {
        ALOGE("Decoding failed, error code: %d", result);
        return false;
    }

    // Apply widening to Ozo decoder output
    if (this->mWideningHandle) {
        float fOutput[1024 * 2];

        OzoLoudspeakerWidening_Process(this->mWideningHandle, (float *) frame_info.samples, fOutput, this->mStreamInfo.frameSize);

        // Back to 16-bit output format
        auto len = frame_info.n_samples / 2;
        float_2_int16(fOutput, (int16_t *) output, len);
        *output_size = len * sizeof(int16_t);
    }

    // Use decoder output as such
    else {
        *output_size = frame_info.n_samples * sizeof(int16_t);
        memcpy(output, frame_info.samples, *output_size);
    }

    return true;
}

bool
OzoDecoderWrapper::flush()
{
    ALOGD("Flush decoder");
    if (this->mOzoHandle) {
        auto result = this->mOzoHandle->flush();
        if (result != ozoaudiocodec::AudioCodecError::OK) {
            ALOGE("Unable to flush decoder, error code: %i", result);
            return false;
        }

        return true;
    }

    ALOGE("Ozo audio decoder handle not available\n");
    return false;
}

bool
OzoDecoderWrapper::setRuntimeParameters(const OzoDecoderConfigParams_t &params, uint64_t flags)
{
    Mutex::Autolock autoLock(this->mLock);

    if (flags & kOrientationRuntimeFlag) {
        ALOGD("Change orientation during runtime");
        return this->setOrientation(params.orientation);
    }

    return true;
}

bool
OzoDecoderWrapper::setOrientation(const std::string &orientation)
{
    auto ctrl = this->mOzoHandle->getControl();

    ALOGD("Set listener orientation to %s", orientation.c_str());

    std::vector<float> values;

    char *orientationLiteral = strndup(orientation.c_str(), strlen(orientation.c_str()));
    char *orientationValue = strtok(orientationLiteral, " ");
    while (orientationValue != NULL) {
        if (strlen(orientationValue) != 0) {
            values.push_back(std::stof(orientationValue));
        }
        orientationValue = strtok(NULL, " ");
    }
    free(orientationLiteral);
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;

    if (values.size()) {
        yaw = values[0];
    }
    if (values.size() > 1) {
        pitch = values[0];
    }
    if (values.size() > 2) {
        pitch = values[0];
    }

    // Set listener orientation
    if (!this->setOrientationParams(ctrl, yaw, pitch, roll)) {
        ALOGD("Unable to change listener orientation: %s", orientation.c_str());
        return false;
    }

    return true;
}

bool
OzoDecoderWrapper::setOrientationParams(ozoaudiocodec::AudioControl *ctrl, double yaw, double pitch,
    double roll)
{
    ozoaudiocodec::CodecControllerID ctrlID = ozoaudiocodec::kListenerId;
    auto result = ctrl->setState(ctrlID, (uint32_t)ozoaudiocodec::kOrientationYaw, yaw);
    if (result != ozoaudiocodec::AudioCodecError::OK) {
        ALOGE("Unable to set orientation yaw: %i", result);
        return false;
    }
    result = ctrl->setState(ctrlID, (uint32_t)ozoaudiocodec::kOrientationPitch, pitch);
    if (result != ozoaudiocodec::AudioCodecError::OK) {
        ALOGE("Unable to set orientation pitch: %i", result);
        return false;
    }
    result = ctrl->setState(ctrlID, (uint32_t)ozoaudiocodec::kOrientationRoll, roll);
    if (result != ozoaudiocodec::AudioCodecError::OK) {
        ALOGE("Unable to set orientation roll: %i", result);
        return false;
    }
    return true;
}

size_t
OzoDecoderWrapper::getInitialDelay() const
{
    return 2;
}

}  // namespace android
