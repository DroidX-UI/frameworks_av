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
#define LOG_TAG "C2SoftOzoDec"
#include <log/log.h>

#include <inttypes.h>
#include <math.h>
#include <numeric>

#include <cutils/properties.h>
#include <media/stagefright/foundation/MediaDefs.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaErrors.h>
#include <utils/misc.h>

#include <C2ComponentFactory.h>
#include <SimpleC2Interface.h>

#include "C2SoftOzoDec.h"
#include "decoder.h"
#include "../OzoC2ConfigDec.h"
#include "OzoC2VendorSupport.h"

namespace android {

namespace {
constexpr char COMPONENT_NAME[] = "c2.nokia.ozoaudio.decoder";
}  // namespace

static const int16_t MAX_CHANNEL_COUNT = 2;
static const int16_t DEFAULT_CHANNELS = 2;
static const int32_t DEFAULT_SAMPLERATE = 48000;

class C2SoftOzoDec::IntfImpl :
    public SimpleInterface<void>::BaseParams,
    public C2SoftOzoDecBaseParams
{
public:
    explicit IntfImpl(const std::shared_ptr<C2ReflectorHelper> &helper):
    SimpleInterface<void>::BaseParams(
                helper,
                COMPONENT_NAME,
                C2Component::KIND_DECODER,
                C2Component::DOMAIN_AUDIO,
                MEDIA_MIMETYPE_AUDIO_OZOAUDIO),
    C2SoftOzoDecBaseParams()
    {
        noPrivateBuffers();
        noInputReferences();
        noOutputReferences();
        noInputLatency();
        noTimeStretch();

        addParameter(
                DefineParam(mActualOutputDelay, C2_PARAMKEY_OUTPUT_DELAY)
                .withConstValue(new C2PortActualDelayTuning::output(2u))
                .build());

        OZODEC_INJECT_PARAMS();
    }
};


C2SoftOzoDec::C2SoftOzoDec(const char *name, c2_node_id_t id, const std::shared_ptr<IntfImpl> &intfImpl):
SimpleC2Component(std::make_shared<SimpleInterface<IntfImpl>>(name, id, intfImpl)),
mIntf(intfImpl),
mSignalledError(false),
mDecoder(nullptr),
mParams(nullptr),
mDelayedFrames(0),
mTsIndex(0)
{
    ALOGD("Birth");

    memset(&mConfig, 0, sizeof(struct OzoConfig));
}

C2SoftOzoDec::~C2SoftOzoDec()
{
    ALOGD("Death");
    onRelease();
}

c2_status_t
C2SoftOzoDec::onInit()
{
    ALOGD("onInit()");
    return C2_OK;
}

c2_status_t
C2SoftOzoDec::onStop()
{
    ALOGD("onStop()");
    mSignalledError = false;
    mTsIndex = 0;
    return C2_OK;
}

void
C2SoftOzoDec::onReset()
{
    ALOGD("onReset()");
    onStop();
}

void
C2SoftOzoDec::onRelease()
{
    ALOGD("onRelease()");

    delete this->mDecoder;
    this->mDecoder = NULL;

    delete this->mParams;
    this->mParams = NULL;
}

status_t
C2SoftOzoDec::initDecoder(const uint8_t *csd, size_t size)
{
    ALOGD("initDecoder()");

    this->mDecoder = new OzoDecoderWrapper();
    this->mStreamInfo = this->mDecoder->getStreamInfo();

    this->mDelayedFrames = 0;

    this->mParams = new OzoDecoderConfigParams_t();
    this->mParams->track_type = ozoaudiodecoder::TrackType::OZO_TRACK;
    this->mParams->guidance = mIntf->getRenderingGuidance();

    int32_t numChannels = DEFAULT_CHANNELS;
    int32_t sampleRate = DEFAULT_SAMPLERATE;
    OzoDecoderWrapper::readCSD(sampleRate, numChannels, csd, size);

    ozoaudiodecoder::TrackType track_type = ozoaudiodecoder::TrackType::OZO_TRACK;
    if (numChannels == 4)
        track_type = ozoaudiodecoder::TrackType::AMBISONICS_TRACK;

    // Decoder specific config data as input to the decoder
    this->mParams->config[0] = const_cast<uint8_t *>(csd);
    this->mParams->config_size[0] = static_cast<int32_t>(size);
    this->mParams->track_type = track_type;

    this->mParams->lsWidening = mIntf->isLsWidening();

    // Initialize Ozo decoder
    auto status = this->mDecoder->init(this->mParams);
    if (!status) {
        ALOGE("Unable to initialize Ozo decoder");
        mSignalledError = true;
        return BAD_VALUE;
    }

    return OK;
}

void
C2SoftOzoDec::updateRuntimeParameters()
{
    uint64_t flags = 0;

    // Dynamic (runtime) configuration settings, these can be set after decoder initialization

    // Listener orientation change request
    if (strcmp(mIntf->getListenerOrientation(), mParams->orientation.c_str())) {
        flags |= OzoDecoderWrapper::kOrientationRuntimeFlag;
        ALOGD("Set listener orientation: %s", mIntf->getListenerOrientation());
    }
    mParams->orientation = std::string(mIntf->getListenerOrientation());

    // Runtime assignment of the changes
    if (flags)
        this->mDecoder->setRuntimeParameters(*this->mParams, flags);
}

void
C2SoftOzoDec::process(const std::unique_ptr<C2Work> &work, const std::shared_ptr<C2BlockPool> &pool)
{
    work->result = C2_OK;
    work->workletsProcessed = 0u;
    work->worklets.front()->output.configUpdate.clear();

    if (mSignalledError) {
        return;
    }

    size_t size = 0u;
    C2ReadView view = mDummyReadView;
    if (!work->input.buffers.empty()) {
        view = work->input.buffers[0]->data().linearBlocks().front().map().get();
        size = view.capacity();
    }

    bool eos = (work->input.flags & C2FrameData::FLAG_END_OF_STREAM) != 0;
    bool codecConfig = (work->input.flags & C2FrameData::FLAG_CODEC_CONFIG) != 0;

    ALOGV("process(): %i %zu", eos, size);

    if (codecConfig && size > 0u)
    {
        // Save decoder config data in case re-initialization is needed
        mConfig.configSize = size;
        memcpy(mConfig.configData, view.data(), size);

        // Initialize decoding path
        if (this->initDecoder(view.data(), size) != OK)
            return;

        C2StreamSampleRateInfo::output sampleRateInfo(0u, mStreamInfo->sampleRate);
        C2StreamChannelCountInfo::output channelCountInfo(0u, mStreamInfo->numChannels);

        std::vector<std::unique_ptr<C2SettingResult>> failures;
        c2_status_t err = mIntf->config({ &sampleRateInfo, &channelCountInfo }, C2_MAY_BLOCK, &failures);
        if (err == OK) {
            C2FrameData &output = work->worklets.front()->output;
            output.configUpdate.push_back(C2Param::Copy(sampleRateInfo));
            output.configUpdate.push_back(C2Param::Copy(channelCountInfo));
        }

        work->worklets.front()->output.flags = work->input.flags;
        work->worklets.front()->output.ordinal = work->input.ordinal;
        work->worklets.front()->output.buffers.clear();
        work->workletsProcessed = 1u;
        return;
    }

    size_t initialDelay = this->mDecoder->getInitialDelay();

    if (size > 0u) {
        size_t output_size = 0;
        uint8_t output[2048 * MAX_CHANNEL_COUNT];
        OzoDecoderDecodeParams_t decode_params = {
            {(uint8_t *) view.data()}, {static_cast<int32_t>(size)}
        };

        // Check if decoder re-initialization is required
        if (mConfig.widening != mIntf->isLsWidening()) {
            ALOGD("Decoder re-init required: %i", mIntf->isLsWidening());

            this->onRelease();
            if (this->initDecoder(mConfig.configData, mConfig.configSize) != OK) {
                ALOGD("Failed to re-initialize decoder");
                return;
            }
        }
        mConfig.widening = mIntf->isLsWidening();

        // Decode input frame
        //ALOGD("Incoming buffer size: %zu bytes", size);
        if (!this->mDecoder->decode(&decode_params, output, &output_size)) {
            ALOGE("Unable to decode frame");
            mSignalledError = true;
            return;
        }

        // Update runtime parameters, if any
        this->updateRuntimeParameters();

        // Compensate initial decoding delay
        if (initialDelay && this->mDelayedFrames < initialDelay) {
            ALOGD("Skipping decoded frame: %zu (out of %zu frames)", this->mDelayedFrames, initialDelay);
            this->mTs[this->mDelayedFrames % 2] = work->input.ordinal.timestamp.peeku();
            this->mDelayedFrames++;
            output_size = 0;
        }

        C2FrameData &c2Output = work->worklets.front()->output;
        c2Output.flags = work->input.flags;
        c2Output.buffers.clear();
        c2Output.ordinal = work->input.ordinal;

        if (output_size) {
            std::shared_ptr<C2LinearBlock> block;

            size_t numOutBytes = this->mStreamInfo->frameSize * sizeof(int16_t) * this->mStreamInfo->numChannels;
            C2MemoryUsage usage = { C2MemoryUsage::CPU_READ, C2MemoryUsage::CPU_WRITE };
            c2_status_t err = pool->fetchLinearBlock(numOutBytes, usage, &block);
            if (err != C2_OK) {
                ALOGD("Failed to fetch a linear block (%d)", err);
                mSignalledError = true;
                return;
            }

            memcpy(block->map().get().data(), output, numOutBytes);
            c2Output.buffers.push_back(createLinearBuffer(block));

            if (initialDelay) {
                c2Output.ordinal.timestamp = this->mTs[this->mTsIndex];

                this->mTs[this->mTsIndex] = work->input.ordinal.timestamp.peeku();
                this->mTsIndex = (this->mTsIndex + 1) % 2;
            }
        }
        else
            c2Output.ordinal.timestamp = 0;

        work->workletsProcessed = 1u;
        work->result = C2_OK;
    }
    else if (eos) {
        auto fillEmptyWork = [](const std::unique_ptr<C2Work> &work, int64_t timestamp) {
            work->worklets.front()->output.flags = work->input.flags;
            work->worklets.front()->output.buffers.clear();
            work->worklets.front()->output.ordinal = work->input.ordinal;
            work->worklets.front()->output.ordinal.timestamp = timestamp;
            work->workletsProcessed = 1u;
        };

        auto ts = initialDelay ? this->mTs[this->mTsIndex] : work->input.ordinal.timestamp.peeku();
        fillEmptyWork(work, ts);
    }
}

c2_status_t
C2SoftOzoDec::drain(uint32_t /*drainMode*/, const std::shared_ptr<C2BlockPool> & /*pool*/)
{
    ALOGD("drain()");
    this->mDecoder->flush();
    mDelayedFrames = 0;
    mTsIndex = 0;
    return C2_OK;
}

c2_status_t
C2SoftOzoDec::onFlush_sm()
{
    ALOGD("onFlush_sm()");
    mDelayedFrames = 0;
    mTsIndex = 0;
    return C2_OK;
}

class C2SoftOzoDecFactory: public C2ComponentFactory
{
public:
    C2SoftOzoDecFactory():
    mHelper(std::static_pointer_cast<C2ReflectorHelper>(GetOzoAudioCodec2VendorComponentStore()->getParamReflector()))
    {}

    virtual c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component>* const component,
        std::function<void(C2Component*)> deleter) override
    {
        *component = std::shared_ptr<C2Component>(
                new C2SoftOzoDec(COMPONENT_NAME, id, std::make_shared<C2SoftOzoDec::IntfImpl>(mHelper)),
                deleter
        );

        return C2_OK;
    }

    virtual c2_status_t createInterface(
        c2_node_id_t id, std::shared_ptr<C2ComponentInterface>* const interface,
        std::function<void(C2ComponentInterface*)> deleter) override
    {
        *interface = std::shared_ptr<C2ComponentInterface>(
            new SimpleInterface<C2SoftOzoDec::IntfImpl>(
                COMPONENT_NAME, id, std::make_shared<C2SoftOzoDec::IntfImpl>(mHelper)),
                deleter
            );

        return C2_OK;
    }

    virtual ~C2SoftOzoDecFactory() override = default;

private:
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

}  // namespace android

extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    ALOGV("in %s", __func__);
    return new ::android::C2SoftOzoDecFactory();
}

extern "C" void DestroyCodec2Factory(::C2ComponentFactory* factory) {
    ALOGV("in %s", __func__);
    delete factory;
}
