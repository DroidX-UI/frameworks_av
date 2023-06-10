/*
Copyright (C) 2019,2020 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

#define LOG_NDEBUG 0
#define LOG_TAG "C2SoftOzoEnc"
#include <utils/Log.h>

#include <inttypes.h>

#include <C2ComponentFactory.h>
#include <SimpleC2Interface.h>
#include <media/stagefright/foundation/MediaDefs.h>
#include <media/stagefright/foundation/hexdump.h>

#include "C2SoftOzoEnc.h"
#include "ozo_wrapper.h"
#include "ozoaudio/ozo_tags.h"
#include "event_emitter.h"
#include "aacenc_lib.h"
#include "../OzoC2ConfigEnc.h"
#include "OzoC2VendorSupport.h"

namespace android {

namespace {
constexpr char COMPONENT_NAME[] = "c2.nokia.ozoaudio.encoder";
}  // namespace

class C2SoftOzoEnc::IntfImpl:
    public SimpleInterface<void>::BaseParams,
    public C2SoftOzoEncBaseParams
{
public:
    explicit IntfImpl(const std::shared_ptr<C2ReflectorHelper> &helper):
    SimpleInterface<void>::BaseParams(
        helper,
        COMPONENT_NAME,
        C2Component::KIND_ENCODER,
        C2Component::DOMAIN_AUDIO,
        MEDIA_MIMETYPE_AUDIO_OZOAUDIO
    ),
    C2SoftOzoEncBaseParams()
    {
        noPrivateBuffers();
        noInputReferences();
        noOutputReferences();
        noInputLatency();
        noTimeStretch();
        setDerivedInstance(this);

        addParameter(
                DefineParam(mAttrib, C2_PARAMKEY_COMPONENT_ATTRIBUTES)
                .withConstValue(new C2ComponentAttributesSetting(
                    C2Component::ATTRIB_IS_TEMPORAL))
                .build());

        // Override the output media type to be AAC
        addParameter(
            DefineParam(mOutputMediaType, C2_PARAMKEY_OUTPUT_MEDIA_TYPE)
            .withConstValue(AllocSharedString<C2PortMediaTypeSetting::output>(
                    MEDIA_MIMETYPE_AUDIO_AAC))
            .build());

        OZOENC_INJECT_PARAMS();
    }
};

C2SoftOzoEnc::C2SoftOzoEnc(const char *name, c2_node_id_t id, const std::shared_ptr<IntfImpl> &intfImpl):
SimpleC2Component(std::make_shared<SimpleInterface<IntfImpl>>(name, id, intfImpl)),
mIntf(intfImpl),
mNumBytesPerInputFrame(0u),
mOutBufferSize(0),
mSentCodecSpecificData(false),
mInputSize(0),
mInputFrame(nullptr),
mInputTimeUs(-1ll),
mSignalledError(false),
mOzoInitialized(false),
mDelayedFrames(0),
mBytesPerSample(2),
mEncoder(nullptr),
mParams(new OzoEncoderParams_t),
mParamUpdater(new OzoParameterUpdater()),
mFrames(0)
{
    ALOGD("Birth");

    init_default_ozoparams(this->mParams);
}

C2SoftOzoEnc::~C2SoftOzoEnc()
{
    ALOGD("Death");

    onStop();
    this->destroy();

    delete this->mParams;
    this->mParams = NULL;

    delete this->mParamUpdater;
    this->mParamUpdater = NULL;
}

c2_status_t
C2SoftOzoEnc::onInit()
{
    ALOGD("onInit()");
    status_t err = this->initEncoder();
    return err == OK ? C2_OK : C2_CORRUPTED;
}

status_t
C2SoftOzoEnc::initEncoder()
{
    ALOGD("initEncoder()");

    this->mParams->bitrate = mIntf->getBitrate();
    this->mParams->samplerate = mIntf->getSampleRate();
    this->mParams->profile = AOT_AAC_LC;

    this->mParams->encoderMode = std::string(mIntf->getEncMode());

    auto deviceId = mIntf->getDeviceId();
    if (strlen(deviceId)) {
        this->mParams->deviceId = std::string(deviceId);
        this->mParams->channels = mIntf->getChannelCount();
    }

    this->mParams->sampledepth = mIntf->getSampleDepth();
    this->mBytesPerSample = this->mParams->sampledepth / 8;

    this->mParams->postData = mIntf->getPostMode();

    mNumBytesPerInputFrame = this->mParams->channels * OzoEncoderWrapper::kNumSamplesPerFrame * this->mBytesPerSample;

    // Hold at least encoded output + wnr event
    auto maxEncChannels = (this->mParams->encoderMode == ozoaudioencoder::OZOSDK_OZOAUDIO_ID) ? 2 : 4;
    mOutBufferSize = maxEncChannels * 768 + 128;

    this->updateRuntimeParameters();

    this->mEncoder = new OzoEncoderWrapper();
    this->mInputFrame = new uint8_t[mNumBytesPerInputFrame];

    // WNR notifications towards application UI enabled
    if (mIntf->getWnrNotification()) {
        ALOGD("Enabling wind level notifications");
        this->mParams->eventEmitters.push_back(new WnrEventEmitter());
    }

    // Sound source localization notifications towards application UI enabled
    SoundSourceSectorsEmitter1 *sourceSectorsEmitter1 = nullptr;
    SoundSourceSectorsEmitter2 *sourceSectorsEmitter2 = nullptr;
    if (mIntf->getSSLocNotification()) {
        ALOGD("Enabling sound source notifications");
        sourceSectorsEmitter1 = new SoundSourceSectorsEmitter1(this->mEncoder);
        this->mParams->eventEmitters.push_back(sourceSectorsEmitter1);
        sourceSectorsEmitter2 = new SoundSourceSectorsEmitter2(this->mEncoder);
        this->mParams->eventEmitters.push_back(sourceSectorsEmitter2);

        this->mParams->eventEmitters.push_back(new SoundSourceEmitter());
    }

    if (mIntf->getAudioLevelsNotification()) {
        ALOGD("Enabling audio levels notifications");
        this->mParams->eventEmitters.push_back(new AudioLevelsEmitter());
    }

    if (mIntf->getMicBlockingNotification()) {
        ALOGD("Enabling mic blocking notifications");
        this->mParams->eventEmitters.push_back(new MicBlockingEmitter());
    }

    // Initialize Ozo encoder
    if (!this->mEncoder->init(*this->mParams, *this->mParamUpdater)) {
        this->mSignalledError = true;
        return BAD_VALUE;
    }
    this->mOzoInitialized = true;

    // Process the localization sectors data to events (azimuth + elevation)
    if (sourceSectorsEmitter1)
        sourceSectorsEmitter1->process(nullptr, 0);

    // Process the localization sectors data to events (width + height)
    if (sourceSectorsEmitter2)
        sourceSectorsEmitter2->process(nullptr, 0);

    size_t initialDelay = this->mEncoder->getInitialDelay();
    this->mDelayTimeUs = (initialDelay * OzoEncoderWrapper::kNumSamplesPerFrame * 1000000ll) / mIntf->getSampleRate();
    this->mFrames = 0;

    return OK;
}

// Dynamic (runtime) configuration settings, these can be set both before
// and after encoder initialization
void
C2SoftOzoEnc::updateRuntimeParameters()
{
    // Update generic parameters
    this->mParamUpdater->setParams(mIntf->getGenericParam());

    // Runtime parameter assignment only if encoder already initialized
    if (this->mOzoInitialized)
        this->mEncoder->setRuntimeParameters(*this->mParamUpdater);
}

class FillWork {
public:
    FillWork(uint32_t flags, C2WorkOrdinalStruct ordinal, const std::shared_ptr<C2Buffer> &buffer):
    mFlags(flags),
    mOrdinal(ordinal),
    mBuffer(buffer)
    {}

    ~FillWork() = default;

    void operator()(const std::unique_ptr<C2Work> &work) {
        work->worklets.front()->output.flags = (C2FrameData::flags_t) mFlags;
        work->worklets.front()->output.buffers.clear();
        work->worklets.front()->output.ordinal = mOrdinal;
        work->workletsProcessed = 1u;
        work->result = C2_OK;
        if (mBuffer) {
            work->worklets.front()->output.buffers.push_back(mBuffer);
        }
        /*
        ALOGV("timestamp = %lld, index = %lld, w/%s buffer",
                mOrdinal.timestamp.peekll(),
                mOrdinal.frameIndex.peekll(),
                mBuffer ? "" : "o");
                */
    }

private:
    const uint32_t mFlags;
    const C2WorkOrdinalStruct mOrdinal;
    const std::shared_ptr<C2Buffer> mBuffer;
};

void
C2SoftOzoEnc::process(const std::unique_ptr<C2Work> &work, const std::shared_ptr<C2BlockPool> &pool)
{
    //ALOGV("process()");

    work->result = C2_OK;
    work->workletsProcessed = 0u;

    if (mSignalledError || !mOzoInitialized)
        return;

    bool eos = (work->input.flags & C2FrameData::FLAG_END_OF_STREAM) != 0;

    if (!mSentCodecSpecificData) {
        // The very first thing we want to output is the codec specific
        // data. It does not require any input data but we will need an
        // output buffer to store it in.
        auto config = this->mEncoder->getConfig();
        ALOGD("Config size: %i", config.data_size);

        std::unique_ptr<C2StreamInitDataInfo::output> csd = C2StreamInitDataInfo::output::AllocUnique(config.data_size, 0u);
        memcpy(csd->m.value, config.data, config.data_size);
        work->worklets.front()->output.configUpdate.push_back(std::move(csd));

        mSentCodecSpecificData = true;
        mInputTimeUs = work->input.ordinal.timestamp;
    }

    uint8_t temp[1];
    size_t capacity = 0u;
    const uint8_t *data = temp;
    C2ReadView view = mDummyReadView;

    // Query input stream
    if (!work->input.buffers.empty()) {
        view = work->input.buffers[0]->data().linearBlocks().front().map().get();
        data = view.data();
        capacity = view.capacity();
    }

    // End of stream signaled, fill frame with silence to get complete audio frame
    if (eos) {
        size_t copy = mNumBytesPerInputFrame - this->mInputSize;
        if (copy > capacity)
            copy = capacity;

        memset(this->mInputFrame + this->mInputSize, 0, copy);
        this->mInputSize += copy;
    }

    std::shared_ptr<C2LinearBlock> block;
    std::unique_ptr<C2WriteView> wView;
    uint8_t *outPtr = temp;

    // Update runtime parameters, if any
    this->updateRuntimeParameters();

    size_t offset = 0;
    bool doProcess = true;
    ozoaudiocodec::AudioDataContainer output;
    size_t initialDelay = this->mEncoder->getInitialDelay();

    output.audio.data_size = 0;

    C2WorkOrdinalStruct outOrdinal = work->input.ordinal;
    uint64_t inputIndex = work->input.ordinal.frameIndex.peeku();

    // Input timestamp with encoder delay compensation
    mInputTimeUs = work->input.ordinal.timestamp;
    mInputTimeUs -= this->mDelayTimeUs;
    uint64_t timestamp = mInputTimeUs.peeku();

    // Amount of data available in the buffer from previous call, measured in microseconds
    size_t bufTsOffset = ((this->mInputSize * 1000000ll) / (mIntf->getChannelCount() * mBytesPerSample)) / mIntf->getSampleRate();

    bool framesEncoded = false;

    do {
        if (this->mInputSize == mNumBytesPerInputFrame) {
            // Encode input to OZO Audio
            if (!this->mEncoder->encode(this->mInputFrame, output)) {
                this->mSignalledError = true;
                ALOGD("Encode failed");
                return;
            }

            // Process output data only if available
            if (output.audio.data_size) {
                if (initialDelay && this->mDelayedFrames < initialDelay) {
                    ALOGD("Skipping encoded frame: %zu (out of %zu frames)", this->mDelayedFrames, initialDelay);
                    this->mDelayedFrames++;
                    output.audio.data_size = 0;
                } else {
                    // Compensate with the data initially present in the buffer
                    timestamp -= bufTsOffset;
                    bufTsOffset = 0;

                    // Advance timestamp by encoded amount
                    timestamp += ((this->mInputSize * 1000000ll) / (mIntf->getChannelCount() * mBytesPerSample)) / mIntf->getSampleRate();

                    // Provide encoded output
                    size_t nOutputBytes = 0;
                    bool packetize = this->mEncoder->mMicWriter || this->mParams->eventEmitters.size();

                    size_t bufferSize = mOutBufferSize;
                    if (this->mEncoder->mMicWriter)
                        bufferSize += 128 + this->mEncoder->mMicWriter->size() + this->mEncoder->mCtrlWriter->size();

                    // Request data block for encoded output
                    C2MemoryUsage usage = { C2MemoryUsage::CPU_READ, C2MemoryUsage::CPU_WRITE };
                    c2_status_t err = pool->fetchLinearBlock(bufferSize, usage, &block);
                    if (err != C2_OK) {
                        ALOGE("Memory pool fetch failed: %d", err);
                        return;
                    }

                    wView.reset(new C2WriteView(block->map().get()));
                    outPtr = wView->data();

                    if (packetize) {
                        /*
                        ALOGD("Post capture data bytes: %zu %zu %zu",
                            this->mEncoder->mMicWriter ? this->mEncoder->mMicWriter->size() : 0,
                            this->mEncoder->mCtrlWriter ? this->mEncoder->mCtrlWriter->size() : 0,
                            bufferSize
                        );
                        */

                        // Pack encoded data + post capture data into one buffer
                        nOutputBytes = OzoAudioUtilities::packetize_data(
                            output.audio.data, output.audio.data_size,
                            this->mEncoder->mMicWriter,
                            this->mEncoder->mCtrlWriter,
                            this->mParams->eventEmitters,
                            timestamp / 1000,
                            outPtr
                        );
                    } else {
                        nOutputBytes = output.audio.data_size;
                        memcpy(outPtr, output.audio.data, nOutputBytes);
                    }

                    C2FrameData::flags_t flags = (C2FrameData::flags_t) 0;
                    if (eos) flags = C2FrameData::FLAG_END_OF_STREAM;

                    outOrdinal.frameIndex = this->mFrames++;
                    outOrdinal.timestamp = timestamp;

                    ALOGD("Encoded %zu bytes, index: %zu, ts: %lld", nOutputBytes, this->mFrames, outOrdinal.timestamp.peekll());

                    auto buffer = createLinearBuffer(block, 0, nOutputBytes);
                    cloneAndSend(inputIndex, work, FillWork(flags, outOrdinal, buffer));
                    buffer.reset();

                    framesEncoded = true;
                }
            }

            this->mInputSize = 0;
        }

        // Add any pending input data to internal input buffer
        if (capacity) {
            size_t copy = mNumBytesPerInputFrame - this->mInputSize;
            if (copy > capacity)
                copy = capacity;

            memcpy(this->mInputFrame + this->mInputSize, data + offset, copy);
            this->mInputSize += copy;
            capacity -= copy;
            offset += copy;
        }
        else {
            // We are done
            doProcess = false;
        }

    } while (doProcess);

    if (!framesEncoded) {
        uint64_t timestamp = mInputTimeUs.peeku() - bufTsOffset;
        timestamp += ((this->mInputSize * 1000000ll) / (mIntf->getChannelCount() * mBytesPerSample)) / mIntf->getSampleRate();

        C2FrameData::flags_t flags = (C2FrameData::flags_t) 0;
        if (eos) flags = C2FrameData::FLAG_END_OF_STREAM;

        work->worklets.front()->output.flags = flags;
        work->worklets.front()->output.buffers.clear();
        work->worklets.front()->output.ordinal = work->input.ordinal;
        work->worklets.front()->output.ordinal.timestamp = timestamp;
        work->workletsProcessed = 1u;
    }
}

c2_status_t
C2SoftOzoEnc::onStop()
{
    ALOGD("onStop()");
    mSentCodecSpecificData = false;
    mInputSize = 0u;
    mInputTimeUs = -1ll;
    mSignalledError = false;
    mFrames = 0;
    return C2_OK;
}

void
C2SoftOzoEnc::destroy()
{
    delete this->mEncoder;
    this->mEncoder = NULL;

    for (size_t i = 0; i < this->mParams->eventEmitters.size(); i++)
        delete this->mParams->eventEmitters[i];

    delete[] this->mInputFrame;
    this->mInputFrame = NULL;

    this->mParamUpdater->clear();
    this->mOzoInitialized = false;
}

void
C2SoftOzoEnc::onReset()
{
    ALOGD("onReset()");
    (void) onStop();

    this->destroy();
    this->initEncoder();
}

void
C2SoftOzoEnc::onRelease()
{}

c2_status_t
C2SoftOzoEnc::onFlush_sm()
{
    ALOGD("onFlush_smn()");
    mSentCodecSpecificData = false;
    mInputSize = 0u;
    mDelayedFrames = 0;
    mFrames = 0;

    return C2_OK;
}

c2_status_t
C2SoftOzoEnc::drain(uint32_t drainMode, const std::shared_ptr<C2BlockPool> & /*pool*/)
{
    ALOGV("drain()");

    switch (drainMode) {
        case DRAIN_COMPONENT_NO_EOS:
            [[fallthrough]];
        case NO_DRAIN:
            // no-op
            return C2_OK;
        case DRAIN_CHAIN:
            return C2_OMITTED;
        case DRAIN_COMPONENT_WITH_EOS:
            break;
        default:
            return C2_BAD_VALUE;
    }

    mSentCodecSpecificData = false;
    mInputSize = 0u;
    mDelayedFrames = 0;
    mFrames = 0;

    return C2_OK;
}


class C2SoftOzoEncFactory: public C2ComponentFactory
{
public:
    C2SoftOzoEncFactory():
    mHelper(std::static_pointer_cast<C2ReflectorHelper>(GetOzoAudioCodec2VendorComponentStore()->getParamReflector()))
    {}

    virtual c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component>* const component,
        std::function<void(C2Component*)> deleter) override
    {
        *component = std::shared_ptr<C2Component>(
            new C2SoftOzoEnc(COMPONENT_NAME, id, std::make_shared<C2SoftOzoEnc::IntfImpl>(mHelper)),
            deleter
        );

        return C2_OK;
    }

    virtual c2_status_t createInterface(
            c2_node_id_t id, std::shared_ptr<C2ComponentInterface>* const interface,
            std::function<void(C2ComponentInterface*)> deleter) override
    {
        *interface = std::shared_ptr<C2ComponentInterface>(
            new SimpleInterface<C2SoftOzoEnc::IntfImpl>(
                COMPONENT_NAME, id, std::make_shared<C2SoftOzoEnc::IntfImpl>(mHelper)),
                deleter
            );

        return C2_OK;
    }

    virtual ~C2SoftOzoEncFactory() override = default;

private:
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

}  // namespace android

extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    ALOGV("in %s", __func__);
    return new ::android::C2SoftOzoEncFactory();
}

extern "C" void DestroyCodec2Factory(::C2ComponentFactory* factory) {
    ALOGV("in %s", __func__);
    delete factory;
}
