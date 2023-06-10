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

#define LOG_TAG "OzoC2Store"
#define LOG_NDEBUG 0
#include <utils/Log.h>

#include <C2AllocatorGralloc.h>
#include <C2AllocatorIon.h>
#include <C2BufferPriv.h>
#include <C2BqBufferPriv.h>
#include <C2Component.h>
#include <C2Config.h>
#include <C2PlatformStorePluginLoader.h>
#include <C2ComponentFactory.h>
#include <OzoC2VendorSupport.h>
#include <util/C2InterfaceHelper.h>
#include <media/stagefright/foundation/MediaDefs.h>

#include <dlfcn.h>
#include <unistd.h> // getpagesize

#include <map>
#include <memory>
#include <mutex>

#include "../codecs/OzoC2ConfigEnc.h"
#include "../codecs/OzoC2ConfigDec.h"
#include "../codecs/base/include/SimpleC2Utils.h"

namespace android {

struct IonSetter {
    static C2R setIonUsage(bool /* mayBlock */, C2InterfaceHelper::C2P<C2StoreIonUsageInfo> &me) {
        me.set().heapMask = ~0;
        me.set().allocFlags = 0;
        me.set().minAlignment = 0;
        return C2R::Ok();
    }
};


/**
 * Platform allocator store IDs
 */
class OzoAllocatorStore : public C2AllocatorStore {
public:
    enum : id_t {
        /**
         * ID of the ion backed platform allocator.
         *
         * C2Handle consists of:
         *   fd  shared ion buffer handle
         *   int size (lo 32 bits)
         *   int size (hi 32 bits)
         *   int magic '\xc2io\x00'
         */
        ION = PLATFORM_START,

        /**
         * ID of the gralloc backed platform allocator.
         *
         * C2Handle layout is not public. Use C2AllocatorGralloc::UnwrapNativeCodec2GrallocHandle
         * to get the underlying gralloc handle from a C2Handle, and WrapNativeCodec2GrallocHandle
         * to create a C2Handle from a gralloc handle - for C2Allocator::priorAllocation.
         */
        GRALLOC,

        /**
         * ID of indicating the end of platform allocator definition.
         *
         * \note always put this macro in the last place.
         *
         * Extended platform store plugin should use this macro as the start ID of its own allocator
         * types.
         */
        PLATFORM_END,
    };
};


/**
 * The platform allocator store provides basic allocator-types for the framework based on ion and
 * gralloc. Allocators are not meant to be updatable.
 *
 * \todo Provide allocator based on ashmem
 * \todo Move ion allocation into its HIDL or provide some mapping from memory usage to ion flags
 * \todo Make this allocator store extendable
 */
class OzoAllocatorStoreImpl : public OzoAllocatorStore {
public:
    OzoAllocatorStoreImpl();

    virtual c2_status_t fetchAllocator(
            id_t id, std::shared_ptr<C2Allocator> *const allocator) override;

    virtual std::vector<std::shared_ptr<const C2Allocator::Traits>> listAllocators_nb()
            const override {
        return std::vector<std::shared_ptr<const C2Allocator::Traits>>(); /// \todo
    }

    virtual C2String getName() const override {
        return "ozoaudio.allocator-store";
    }

    void setComponentStore(std::shared_ptr<C2ComponentStore> store);

    ~OzoAllocatorStoreImpl() override = default;

private:
    /// returns a shared-singleton ion allocator
    std::shared_ptr<C2Allocator> fetchIonAllocator();

    /// returns a shared-singleton gralloc allocator
    std::shared_ptr<C2Allocator> fetchGrallocAllocator();

    /// component store to use
    std::mutex _mComponentStoreSetLock; // protects the entire updating _mComponentStore and its
                                        // dependencies
    std::mutex _mComponentStoreReadLock; // must protect only read/write of _mComponentStore
    std::shared_ptr<C2ComponentStore> _mComponentStore;
};

OzoAllocatorStoreImpl::OzoAllocatorStoreImpl() {
}

c2_status_t OzoAllocatorStoreImpl::fetchAllocator(
        id_t id, std::shared_ptr<C2Allocator> *const allocator) {
    allocator->reset();
    switch (id) {
    // TODO: should we implement a generic registry for all, and use that?
    case OzoAllocatorStore::ION:
    case C2AllocatorStore::DEFAULT_LINEAR:
        *allocator = fetchIonAllocator();
        break;

    case OzoAllocatorStore::GRALLOC:
    case C2AllocatorStore::DEFAULT_GRAPHIC:
        *allocator = fetchGrallocAllocator();
        break;

    default:
        // Try to create allocator from platform store plugins.
        c2_status_t res =
                C2PlatformStorePluginLoader::GetInstance()->createAllocator(id, allocator);
        if (res != C2_OK) {
            return res;
        }
        break;
    }
    if (*allocator == nullptr) {
        return C2_NO_MEMORY;
    }
    return C2_OK;
}


namespace {

std::mutex gIonAllocatorMutex;
std::weak_ptr<C2AllocatorIon> gIonAllocator;

void UseComponentStoreForIonAllocator(
        const std::shared_ptr<C2AllocatorIon> allocator,
        std::shared_ptr<C2ComponentStore> store) {
    C2AllocatorIon::UsageMapperFn mapper;
    uint64_t minUsage = 0;
    uint64_t maxUsage = C2MemoryUsage(C2MemoryUsage::CPU_READ, C2MemoryUsage::CPU_WRITE).expected;
    size_t blockSize = getpagesize();

    // query min and max usage as well as block size via supported values
    C2StoreIonUsageInfo usageInfo;
    std::vector<C2FieldSupportedValuesQuery> query = {
        C2FieldSupportedValuesQuery::Possible(C2ParamField::Make(usageInfo, usageInfo.usage)),
        C2FieldSupportedValuesQuery::Possible(C2ParamField::Make(usageInfo, usageInfo.capacity)),
    };
    c2_status_t res = store->querySupportedValues_sm(query);
    if (res == C2_OK) {
        if (query[0].status == C2_OK) {
            const C2FieldSupportedValues &fsv = query[0].values;
            if (fsv.type == C2FieldSupportedValues::FLAGS && !fsv.values.empty()) {
                minUsage = fsv.values[0].u64;
                maxUsage = 0;
                for (C2Value::Primitive v : fsv.values) {
                    maxUsage |= v.u64;
                }
            }
        }
        if (query[1].status == C2_OK) {
            const C2FieldSupportedValues &fsv = query[1].values;
            if (fsv.type == C2FieldSupportedValues::RANGE && fsv.range.step.u32 > 0) {
                blockSize = fsv.range.step.u32;
            }
        }

        mapper = [store](C2MemoryUsage usage, size_t capacity,
                         size_t *align, unsigned *heapMask, unsigned *flags) -> c2_status_t {
            if (capacity > UINT32_MAX) {
                return C2_BAD_VALUE;
            }
            C2StoreIonUsageInfo usageInfo = { usage.expected, capacity };
            std::vector<std::unique_ptr<C2SettingResult>> failures; // TODO: remove
            c2_status_t res = store->config_sm({&usageInfo}, &failures);
            if (res == C2_OK) {
                *align = usageInfo.minAlignment;
                *heapMask = usageInfo.heapMask;
                *flags = usageInfo.allocFlags;
            }
            return res;
        };
    }

    allocator->setUsageMapper(mapper, minUsage, maxUsage, blockSize);
}

}

void OzoAllocatorStoreImpl::setComponentStore(std::shared_ptr<C2ComponentStore> store) {
    // technically this set lock is not needed, but is here for safety in case we add more
    // getter orders
    std::lock_guard<std::mutex> lock(_mComponentStoreSetLock);
    {
        std::lock_guard<std::mutex> lock(_mComponentStoreReadLock);
        _mComponentStore = store;
    }
    std::shared_ptr<C2AllocatorIon> allocator;
    {
        std::lock_guard<std::mutex> lock(gIonAllocatorMutex);
        allocator = gIonAllocator.lock();
    }
    if (allocator) {
        UseComponentStoreForIonAllocator(allocator, store);
    }
}

std::shared_ptr<C2Allocator> OzoAllocatorStoreImpl::fetchIonAllocator() {
    std::lock_guard<std::mutex> lock(gIonAllocatorMutex);
    std::shared_ptr<C2AllocatorIon> allocator = gIonAllocator.lock();
    if (allocator == nullptr) {
        std::shared_ptr<C2ComponentStore> componentStore;
        {
            std::lock_guard<std::mutex> lock(_mComponentStoreReadLock);
            componentStore = _mComponentStore;
        }
        allocator = std::make_shared<C2AllocatorIon>(OzoAllocatorStore::ION);
        UseComponentStoreForIonAllocator(allocator, componentStore);
        gIonAllocator = allocator;
    }
    return allocator;
}

std::shared_ptr<C2Allocator> OzoAllocatorStoreImpl::fetchGrallocAllocator() {
    static std::mutex mutex;
    static std::weak_ptr<C2Allocator> grallocAllocator;
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<C2Allocator> allocator = grallocAllocator.lock();
    if (allocator == nullptr) {
        allocator = std::make_shared<C2AllocatorGralloc>(OzoAllocatorStore::GRALLOC);
        grallocAllocator = allocator;
    }
    return allocator;
}


struct OzoCodecInterface:
    public C2InterfaceHelper,
    public C2SoftOzoEncBaseParams,
    public C2SoftOzoDecBaseParams
{
    std::shared_ptr<C2StoreIonUsageInfo> mIonUsageInfo;

    std::shared_ptr<C2ApiLevelSetting> mApiLevel;
    std::shared_ptr<C2ApiFeaturesSetting> mApiFeatures;

    std::shared_ptr<C2PlatformLevelSetting> mPlatformLevel;
    std::shared_ptr<C2PlatformFeaturesSetting> mPlatformFeatures;

    std::shared_ptr<C2ComponentNameSetting> mName;
    std::shared_ptr<C2ComponentAliasesSetting> mAliases;
    std::shared_ptr<C2ComponentKindSetting> mKind;
    std::shared_ptr<C2ComponentDomainSetting> mDomain;
    std::shared_ptr<C2ComponentAttributesSetting> mAttrib;
    std::shared_ptr<C2ComponentTimeStretchTuning> mTimeStretch;

    std::shared_ptr<C2PortMediaTypeSetting::input> mInputMediaType;
    std::shared_ptr<C2PortMediaTypeSetting::output> mOutputMediaType;
    std::shared_ptr<C2StreamBufferTypeSetting::input> mInputFormat;
    std::shared_ptr<C2StreamBufferTypeSetting::output> mOutputFormat;

    std::shared_ptr<C2PortRequestedDelayTuning::input> mRequestedInputDelay;
    std::shared_ptr<C2PortRequestedDelayTuning::output> mRequestedOutputDelay;
    std::shared_ptr<C2RequestedPipelineDelayTuning> mRequestedPipelineDelay;

    std::shared_ptr<C2PortActualDelayTuning::input> mActualInputDelay;
    std::shared_ptr<C2PortActualDelayTuning::output> mActualOutputDelay;
    std::shared_ptr<C2ActualPipelineDelayTuning> mActualPipelineDelay;

    std::shared_ptr<C2StreamMaxReferenceAgeTuning::input> mMaxInputReferenceAge;
    std::shared_ptr<C2StreamMaxReferenceCountTuning::input> mMaxInputReferenceCount;
    std::shared_ptr<C2StreamMaxReferenceAgeTuning::output> mMaxOutputReferenceAge;
    std::shared_ptr<C2StreamMaxReferenceCountTuning::output> mMaxOutputReferenceCount;
    std::shared_ptr<C2MaxPrivateBufferCountTuning> mMaxPrivateBufferCount;

    std::shared_ptr<C2PortStreamCountTuning::input> mInputStreamCount;
    std::shared_ptr<C2PortStreamCountTuning::output> mOutputStreamCount;

    std::shared_ptr<C2SubscribedParamIndicesTuning> mSubscribedParamIndices;
    std::shared_ptr<C2PortSuggestedBufferCountTuning::input> mSuggestedInputBufferCount;
    std::shared_ptr<C2PortSuggestedBufferCountTuning::output> mSuggestedOutputBufferCount;

    std::shared_ptr<C2CurrentWorkTuning> mCurrentWorkOrdinal;
    std::shared_ptr<C2LastWorkQueuedTuning::input> mLastInputQueuedWorkOrdinal;
    std::shared_ptr<C2LastWorkQueuedTuning::output> mLastOutputQueuedWorkOrdinal;

    std::shared_ptr<C2PortAllocatorsTuning::input> mInputAllocators;
    std::shared_ptr<C2PortAllocatorsTuning::output> mOutputAllocators;
    std::shared_ptr<C2PrivateAllocatorsTuning> mPrivateAllocators;
    std::shared_ptr<C2PortBlockPoolsTuning::output> mOutputPoolIds;
    std::shared_ptr<C2PrivateBlockPoolsTuning> mPrivatePoolIds;

    std::shared_ptr<C2TrippedTuning> mTripped;
    std::shared_ptr<C2OutOfMemoryTuning> mOutOfMemory;

    std::shared_ptr<C2PortConfigCounterTuning::input> mInputConfigCounter;
    std::shared_ptr<C2PortConfigCounterTuning::output> mOutputConfigCounter;
    std::shared_ptr<C2ConfigCounterTuning> mDirectConfigCounter;

    OzoCodecInterface(std::shared_ptr<C2ReflectorHelper> reflector):
    C2InterfaceHelper(reflector),
    C2SoftOzoEncBaseParams(),
    C2SoftOzoDecBaseParams()
    {
        setDerivedInstance(this);

        addParameter(
            DefineParam(mIonUsageInfo, "ion-usage")
            .withDefault(new C2StoreIonUsageInfo())
            .withFields({
                C2F(mIonUsageInfo, usage).flags({C2MemoryUsage::CPU_READ | C2MemoryUsage::CPU_WRITE}),
                C2F(mIonUsageInfo, capacity).inRange(0, UINT32_MAX, 1024),
                C2F(mIonUsageInfo, heapMask).any(),
                C2F(mIonUsageInfo, allocFlags).flags({}),
                C2F(mIonUsageInfo, minAlignment).equalTo(0)
            })
            .withSetter(IonSetter::setIonUsage)
            .build());

        addParameter(
            DefineParam(mInputStreamCount, C2_PARAMKEY_INPUT_STREAM_COUNT)
            .withConstValue(new C2PortStreamCountTuning::input(1))
            .build());

        addParameter(
            DefineParam(mOutputStreamCount, C2_PARAMKEY_OUTPUT_STREAM_COUNT)
            .withConstValue(new C2PortStreamCountTuning::output(1))
            .build());

        addParameter(
            DefineParam(mRequestedInputDelay, C2_PARAMKEY_INPUT_DELAY_REQUEST)
            .withConstValue(new C2PortRequestedDelayTuning::input(0u))
            .build());

        addParameter(
            DefineParam(mActualInputDelay, C2_PARAMKEY_INPUT_DELAY)
            .withConstValue(new C2PortActualDelayTuning::input(0u))
            .build());

        addParameter(
            DefineParam(mRequestedOutputDelay, C2_PARAMKEY_OUTPUT_DELAY_REQUEST)
            .withConstValue(new C2PortRequestedDelayTuning::output(0u))
            .build());

        addParameter(
            DefineParam(mActualOutputDelay, C2_PARAMKEY_OUTPUT_DELAY)
            .withConstValue(new C2PortActualDelayTuning::output(0u))
            .build());

        addParameter(
            DefineParam(mRequestedPipelineDelay, C2_PARAMKEY_PIPELINE_DELAY_REQUEST)
            .withConstValue(new C2RequestedPipelineDelayTuning(0u))
            .build());

        addParameter(
            DefineParam(mActualPipelineDelay, C2_PARAMKEY_PIPELINE_DELAY)
            .withConstValue(new C2ActualPipelineDelayTuning(0u))
            .build());

        addParameter(
            DefineParam(mPrivateAllocators, C2_PARAMKEY_PRIVATE_ALLOCATORS)
            .withConstValue(C2PrivateAllocatorsTuning::AllocShared(0u))
            .build());

        addParameter(
            DefineParam(mMaxPrivateBufferCount, C2_PARAMKEY_MAX_PRIVATE_BUFFER_COUNT)
            .withConstValue(C2MaxPrivateBufferCountTuning::AllocShared(0u))
            .build());

        addParameter(
            DefineParam(mPrivatePoolIds, C2_PARAMKEY_PRIVATE_BLOCK_POOLS)
            .withConstValue(C2PrivateBlockPoolsTuning::AllocShared(0u))
            .build());

        addParameter(
            DefineParam(mMaxInputReferenceAge, C2_PARAMKEY_INPUT_MAX_REFERENCE_AGE)
            .withConstValue(new C2StreamMaxReferenceAgeTuning::input(0u))
            .build());

        addParameter(
            DefineParam(mMaxInputReferenceCount, C2_PARAMKEY_INPUT_MAX_REFERENCE_COUNT)
            .withConstValue(new C2StreamMaxReferenceCountTuning::input(0u))
            .build());

        addParameter(
            DefineParam(mMaxOutputReferenceAge, C2_PARAMKEY_OUTPUT_MAX_REFERENCE_AGE)
            .withConstValue(new C2StreamMaxReferenceAgeTuning::output(0u))
            .build());

        addParameter(
            DefineParam(mMaxOutputReferenceCount, C2_PARAMKEY_OUTPUT_MAX_REFERENCE_COUNT)
            .withConstValue(new C2StreamMaxReferenceCountTuning::output(0u))
            .build());

        addParameter(
            DefineParam(mTimeStretch, C2_PARAMKEY_TIME_STRETCH)
            .withConstValue(new C2ComponentTimeStretchTuning(1.f))
            .build());

        addParameter(
            DefineParam(mSubscribedParamIndices, C2_PARAMKEY_SUBSCRIBED_PARAM_INDICES)
            .withDefault(C2SubscribedParamIndicesTuning::AllocShared(0u))
            .withFields({ C2F(mSubscribedParamIndices, m.values[0]).any(),
                        C2F(mSubscribedParamIndices, m.values).any() })
            .withSetter(Setter<C2SubscribedParamIndicesTuning>::NonStrictValuesWithNoDeps)
            .build());

        addParameter(
            DefineParam(mInputFormat, C2_PARAMKEY_INPUT_STREAM_BUFFER_TYPE)
            .withConstValue(new C2StreamBufferTypeSetting::input(
                    0u, C2BufferData::LINEAR))
            .build());

        addParameter(
            DefineParam(mOutputFormat, C2_PARAMKEY_OUTPUT_STREAM_BUFFER_TYPE)
            .withConstValue(new C2StreamBufferTypeSetting::output(
                    0u, C2BufferData::LINEAR))
            .build());


        C2Allocator::id_t inputAllocators[1] = { C2AllocatorStore::DEFAULT_LINEAR };
        C2Allocator::id_t outputAllocators[1] = { C2AllocatorStore::DEFAULT_LINEAR };
        C2BlockPool::local_id_t outputPoolIds[1] = { C2BlockPool::BASIC_LINEAR };
        addParameter(
            DefineParam(mInputAllocators, C2_PARAMKEY_INPUT_ALLOCATORS)
            .withDefault(C2PortAllocatorsTuning::input::AllocShared(inputAllocators))
            .withFields({ C2F(mInputAllocators, m.values[0]).any(),
                        C2F(mInputAllocators, m.values).inRange(0, 1) })
            .withSetter(Setter<C2PortAllocatorsTuning::input>::NonStrictValuesWithNoDeps)
            .build());

        addParameter(
            DefineParam(mOutputAllocators, C2_PARAMKEY_OUTPUT_ALLOCATORS)
            .withDefault(C2PortAllocatorsTuning::output::AllocShared(outputAllocators))
            .withFields({ C2F(mOutputAllocators, m.values[0]).any(),
                        C2F(mOutputAllocators, m.values).inRange(0, 1) })
            .withSetter(Setter<C2PortAllocatorsTuning::output>::NonStrictValuesWithNoDeps)
            .build());

        addParameter(
            DefineParam(mOutputPoolIds, C2_PARAMKEY_OUTPUT_BLOCK_POOLS)
            .withDefault(C2PortBlockPoolsTuning::output::AllocShared(outputPoolIds))
            .withFields({ C2F(mOutputPoolIds, m.values[0]).any(),
                        C2F(mOutputPoolIds, m.values).inRange(0, 1) })
            .withSetter(Setter<C2PortBlockPoolsTuning::output>::NonStrictValuesWithNoDeps)
            .build());

        addParameter(
            DefineParam(mName, C2_PARAMKEY_COMPONENT_NAME)
            .withConstValue(AllocSharedString<C2ComponentNameSetting>("ozoaudio"))
            .build());

        addParameter(
            DefineParam(mAttrib, C2_PARAMKEY_COMPONENT_ATTRIBUTES)
            .withConstValue(new C2ComponentAttributesSetting(
                C2Component::ATTRIB_IS_TEMPORAL))
            .build());

        addParameter(
            DefineParam(mKind, C2_PARAMKEY_COMPONENT_KIND)
            .withConstValue(new C2ComponentKindSetting(C2Component::KIND_ENCODER))
            .build());

        addParameter(
            DefineParam(mDomain, C2_PARAMKEY_COMPONENT_DOMAIN)
            .withConstValue(new C2ComponentDomainSetting(C2Component::DOMAIN_AUDIO))
            .build());

        addParameter(
            DefineParam(mOutputMediaType, C2_PARAMKEY_OUTPUT_MEDIA_TYPE)
            .withConstValue(AllocSharedString<C2PortMediaTypeSetting::output>(MEDIA_MIMETYPE_AUDIO_OZOAUDIO))
            .build()
        );

        OZOENC_INJECT_PARAMS();
        OZODEC_INJECT_PARAMS();
    }
};


class OzoAudioC2Store : public C2ComponentStore {
public:
    virtual std::vector<std::shared_ptr<const C2Component::Traits>> listComponents() override;
    virtual std::shared_ptr<C2ParamReflector> getParamReflector() const override;
    virtual C2String getName() const override;
    virtual c2_status_t querySupportedValues_sm(
            std::vector<C2FieldSupportedValuesQuery> &fields) const override;
    virtual c2_status_t querySupportedParams_nb(
            std::vector<std::shared_ptr<C2ParamDescriptor>> *const params) const override;
    virtual c2_status_t query_sm(
            const std::vector<C2Param*> &stackParams,
            const std::vector<C2Param::Index> &heapParamIndices,
            std::vector<std::unique_ptr<C2Param>> *const heapParams) const override;
    virtual c2_status_t createInterface(
            C2String name, std::shared_ptr<C2ComponentInterface> *const interface) override;
    virtual c2_status_t createComponent(
            C2String name, std::shared_ptr<C2Component> *const component) override;
    virtual c2_status_t copyBuffer(
            std::shared_ptr<C2GraphicBuffer> src, std::shared_ptr<C2GraphicBuffer> dst) override;
    virtual c2_status_t config_sm(
            const std::vector<C2Param*> &params,
            std::vector<std::unique_ptr<C2SettingResult>> *const failures) override;
    OzoAudioC2Store();

    virtual ~OzoAudioC2Store() override = default;

private:

    /**
     * An object encapsulating a loaded component module.
     *
     * \todo provide a way to add traits to known components here to avoid loading the .so-s
     * for listComponents
     */
    struct ComponentModule : public C2ComponentFactory,
            public std::enable_shared_from_this<ComponentModule> {
        virtual c2_status_t createComponent(
                c2_node_id_t id, std::shared_ptr<C2Component> *component,
                ComponentDeleter deleter = std::default_delete<C2Component>()) override;
        virtual c2_status_t createInterface(
                c2_node_id_t id, std::shared_ptr<C2ComponentInterface> *interface,
                InterfaceDeleter deleter = std::default_delete<C2ComponentInterface>()) override;

        /**
         * \returns the traits of the component in this module.
         */
        std::shared_ptr<const C2Component::Traits> getTraits();

        /**
         * Creates an uninitialized component module.
         *
         * \param name[in]  component name.
         *
         * \note Only used by ComponentLoader.
         */
        ComponentModule()
            : mInit(C2_NO_INIT),
              mLibHandle(nullptr),
              createFactory(nullptr),
              destroyFactory(nullptr),
              mComponentFactory(nullptr) {
        }

        /**
         * Initializes a component module with a given library path. Must be called exactly once.
         *
         * \note Only used by ComponentLoader.
         *
         * \param libPath[in] library path
         *
         * \retval C2_OK        the component module has been successfully loaded
         * \retval C2_NO_MEMORY not enough memory to loading the component module
         * \retval C2_NOT_FOUND could not locate the component module
         * \retval C2_CORRUPTED the component module could not be loaded (unexpected)
         * \retval C2_REFUSED   permission denied to load the component module (unexpected)
         * \retval C2_TIMED_OUT could not load the module within the time limit (unexpected)
         */
        c2_status_t init(std::string libPath);

        virtual ~ComponentModule() override;

    protected:
        std::recursive_mutex mLock; ///< lock protecting mTraits
        std::shared_ptr<C2Component::Traits> mTraits; ///< cached component traits

        c2_status_t mInit; ///< initialization result

        void *mLibHandle; ///< loaded library handle
        C2ComponentFactory::CreateCodec2FactoryFunc createFactory; ///< loaded create function
        C2ComponentFactory::DestroyCodec2FactoryFunc destroyFactory; ///< loaded destroy function
        C2ComponentFactory *mComponentFactory; ///< loaded/created component factory
    };

    /**
     * An object encapsulating a loadable component module.
     *
     * \todo make this also work for enumerations
     */
    struct ComponentLoader {
        /**
         * Load the component module.
         *
         * This method simply returns the component module if it is already currently loaded, or
         * attempts to load it if it is not.
         *
         * \param module[out] pointer to the shared pointer where the loaded module shall be stored.
         *                    This will be nullptr on error.
         *
         * \retval C2_OK        the component module has been successfully loaded
         * \retval C2_NO_MEMORY not enough memory to loading the component module
         * \retval C2_NOT_FOUND could not locate the component module
         * \retval C2_CORRUPTED the component module could not be loaded
         * \retval C2_REFUSED   permission denied to load the component module
         */
        c2_status_t fetchModule(std::shared_ptr<ComponentModule> *module) {
            c2_status_t res = C2_OK;
            std::lock_guard<std::mutex> lock(mMutex);
            std::shared_ptr<ComponentModule> localModule = mModule.lock();
            if (localModule == nullptr) {
                localModule = std::make_shared<ComponentModule>();
                res = localModule->init(mLibPath);
                if (res == C2_OK) {
                    mModule = localModule;
                }
            }
            *module = localModule;
            return res;
        }

        /**
         * Creates a component loader for a specific library path (or name).
         */
        ComponentLoader(std::string libPath)
            : mLibPath(libPath) {}

    private:
        std::mutex mMutex; ///< mutex guarding the module
        std::weak_ptr<ComponentModule> mModule; ///< weak reference to the loaded module
        std::string mLibPath; ///< library path
    };

    /**
     * Retrieves the component module for a component.
     *
     * \param module pointer to a shared_pointer where the component module will be stored on
     *               success.
     *
     * \retval C2_OK        the component loader has been successfully retrieved
     * \retval C2_NO_MEMORY not enough memory to locate the component loader
     * \retval C2_NOT_FOUND could not locate the component to be loaded
     * \retval C2_CORRUPTED the component loader could not be identified due to some modules being
     *                      corrupted (this can happen if the name does not refer to an already
     *                      identified component but some components could not be loaded due to
     *                      bad library)
     * \retval C2_REFUSED   permission denied to find the component loader for the named component
     *                      (this can happen if the name does not refer to an already identified
     *                      component but some components could not be loaded due to lack of
     *                      permissions)
     */
    c2_status_t findComponent(C2String name, std::shared_ptr<ComponentModule> *module);

    /**
     * Loads each component module and discover its contents.
     */
    void visitComponents();

    std::mutex mMutex; ///< mutex guarding the component lists during construction
    bool mVisited; ///< component modules visited
    std::map<C2String, ComponentLoader> mComponents; ///< path -> component module
    std::map<C2String, C2String> mComponentNameToPath; ///< name -> path
    std::vector<std::shared_ptr<const C2Component::Traits>> mComponentList;

    std::shared_ptr<C2ReflectorHelper> mReflector;
    OzoCodecInterface mInterface;
};

c2_status_t OzoAudioC2Store::ComponentModule::init(std::string libPath)
{
    ALOGV("in %s", __func__);
    ALOGV("loading dll");
    mLibHandle = dlopen(libPath.c_str(), RTLD_NOW|RTLD_NODELETE);
    LOG_ALWAYS_FATAL_IF(mLibHandle == nullptr,
            "could not dlopen %s: %s", libPath.c_str(), dlerror());

    createFactory =
        (C2ComponentFactory::CreateCodec2FactoryFunc)dlsym(mLibHandle, "CreateCodec2Factory");
    LOG_ALWAYS_FATAL_IF(createFactory == nullptr,
            "createFactory is null in %s", libPath.c_str());

    destroyFactory =
        (C2ComponentFactory::DestroyCodec2FactoryFunc)dlsym(mLibHandle, "DestroyCodec2Factory");
    LOG_ALWAYS_FATAL_IF(destroyFactory == nullptr,
            "destroyFactory is null in %s", libPath.c_str());

    mComponentFactory = createFactory();
    if (mComponentFactory == nullptr) {
        ALOGD("could not create factory in %s", libPath.c_str());
        mInit = C2_NO_MEMORY;
    } else {
        mInit = C2_OK;
    }

    if (mInit != C2_OK) {
        return mInit;
    }

    std::shared_ptr<C2ComponentInterface> intf;
    c2_status_t res = createInterface(0, &intf);
    if (res != C2_OK) {
        ALOGD("failed to create interface: %d", res);
        return mInit;
    }

    std::shared_ptr<C2Component::Traits> traits(new (std::nothrow) C2Component::Traits);
    if (traits) {
        traits->name = intf->getName();

        C2ComponentKindSetting kind;
        C2ComponentDomainSetting domain;
        res = intf->query_vb({ &kind, &domain }, {}, C2_MAY_BLOCK, nullptr);
        bool fixDomain = res != C2_OK;
        if (res == C2_OK) {
            traits->kind = kind.value;
            traits->domain = domain.value;
        } else {
            // TODO: remove this fall-back
            ALOGD("failed to query interface for kind and domain: %d", res);

            traits->kind =
                (traits->name.find("encoder") != std::string::npos) ? C2Component::KIND_ENCODER :
                (traits->name.find("decoder") != std::string::npos) ? C2Component::KIND_DECODER :
                C2Component::KIND_OTHER;
        }

        uint32_t mediaTypeIndex =
                traits->kind == C2Component::KIND_ENCODER ? C2PortMediaTypeSetting::output::PARAM_TYPE
                : C2PortMediaTypeSetting::input::PARAM_TYPE;
        std::vector<std::unique_ptr<C2Param>> params;
        res = intf->query_vb({}, { mediaTypeIndex }, C2_MAY_BLOCK, &params);
        if (res != C2_OK) {
            ALOGD("failed to query interface: %d", res);
            return mInit;
        }
        if (params.size() != 1u) {
            ALOGD("failed to query interface: unexpected vector size: %zu", params.size());
            return mInit;
        }
        C2PortMediaTypeSetting *mediaTypeConfig = C2PortMediaTypeSetting::From(params[0].get());
        if (mediaTypeConfig == nullptr) {
            ALOGD("failed to query media type");
            return mInit;
        }
        traits->mediaType =
            std::string(mediaTypeConfig->m.value,
                        strnlen(mediaTypeConfig->m.value, mediaTypeConfig->flexCount()));

        if (fixDomain) {
            if (strncmp(traits->mediaType.c_str(), "audio/", 6) == 0) {
                traits->domain = C2Component::DOMAIN_AUDIO;
            } else if (strncmp(traits->mediaType.c_str(), "video/", 6) == 0) {
                traits->domain = C2Component::DOMAIN_VIDEO;
            } else if (strncmp(traits->mediaType.c_str(), "image/", 6) == 0) {
                traits->domain = C2Component::DOMAIN_IMAGE;
            } else {
                traits->domain = C2Component::DOMAIN_OTHER;
            }
        }

        // TODO: get this properly from the store during emplace
        switch (traits->domain) {
        case C2Component::DOMAIN_AUDIO:
            traits->rank = 8;
            break;
        default:
            traits->rank = 512;
        }

        params.clear();
        res = intf->query_vb({}, { C2ComponentAliasesSetting::PARAM_TYPE }, C2_MAY_BLOCK, &params);
        if (res == C2_OK && params.size() == 1u) {
            C2ComponentAliasesSetting *aliasesSetting =
                C2ComponentAliasesSetting::From(params[0].get());
            if (aliasesSetting) {
                // Split aliases on ','
                // This looks simpler in plain C and even std::string would still make a copy.
                char *aliases = ::strndup(aliasesSetting->m.value, aliasesSetting->flexCount());
                ALOGD("'%s' has aliases: '%s'", intf->getName().c_str(), aliases);

                for (char *tok, *ptr, *str = aliases; (tok = ::strtok_r(str, ",", &ptr));
                        str = nullptr) {
                    traits->aliases.push_back(tok);
                    ALOGD("adding alias: '%s'", tok);
                }
                free(aliases);
            }
        }
    }
    mTraits = traits;

    return mInit;
}

OzoAudioC2Store::ComponentModule::~ComponentModule() {
    ALOGV("in %s", __func__);
    if (destroyFactory && mComponentFactory) {
        destroyFactory(mComponentFactory);
    }
    if (mLibHandle) {
        ALOGV("unloading dll");
        dlclose(mLibHandle);
    }
}

c2_status_t OzoAudioC2Store::ComponentModule::createInterface(
        c2_node_id_t id, std::shared_ptr<C2ComponentInterface> *interface,
        std::function<void(::C2ComponentInterface*)> deleter) {
    interface->reset();
    if (mInit != C2_OK) {
        return mInit;
    }
    std::shared_ptr<ComponentModule> module = shared_from_this();
    c2_status_t res = mComponentFactory->createInterface(
            id, interface, [module, deleter](C2ComponentInterface *p) mutable {
                // capture module so that we ensure we still have it while deleting interface
                deleter(p); // delete interface first
                module.reset(); // remove module ref (not technically needed)
    });
    return res;
}

c2_status_t OzoAudioC2Store::ComponentModule::createComponent(
        c2_node_id_t id, std::shared_ptr<C2Component> *component,
        std::function<void(::C2Component*)> deleter) {
    component->reset();
    if (mInit != C2_OK) {
        return mInit;
    }
    std::shared_ptr<ComponentModule> module = shared_from_this();
    c2_status_t res = mComponentFactory->createComponent(
            id, component, [module, deleter](C2Component *p) mutable {
                // capture module so that we ensure we still have it while deleting component
                deleter(p); // delete component first
                module.reset(); // remove module ref (not technically needed)
    });
    return res;
}

std::shared_ptr<const C2Component::Traits> OzoAudioC2Store::ComponentModule::getTraits() {
    std::unique_lock<std::recursive_mutex> lock(mLock);
    return mTraits;
}

OzoAudioC2Store::OzoAudioC2Store():
    mVisited(false),
    mReflector(std::make_shared<C2ReflectorHelper>()),
    mInterface(mReflector)
{
    auto emplace = [this](const char *libPath) {
        mComponents.emplace(libPath, libPath);
    };

    emplace("libcodec2_soft_ozoenc.so");
    emplace("libcodec2_soft_ozodec.so");
}

c2_status_t OzoAudioC2Store::copyBuffer(
        std::shared_ptr<C2GraphicBuffer> /*src*/, std::shared_ptr<C2GraphicBuffer> /*dst*/) {
    return C2_OMITTED;
}

c2_status_t OzoAudioC2Store::query_sm(
        const std::vector<C2Param*> &stackParams,
        const std::vector<C2Param::Index> &heapParamIndices,
        std::vector<std::unique_ptr<C2Param>> *const heapParams) const {
    return mInterface.query(stackParams, heapParamIndices, C2_MAY_BLOCK, heapParams);
}

c2_status_t OzoAudioC2Store::config_sm(
        const std::vector<C2Param*> &params,
        std::vector<std::unique_ptr<C2SettingResult>> *const failures) {
    return mInterface.config(params, C2_MAY_BLOCK, failures);
}

void OzoAudioC2Store::visitComponents() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mVisited) {
        return;
    }
    for (auto &pathAndLoader : mComponents) {
        const C2String &path = pathAndLoader.first;
        ComponentLoader &loader = pathAndLoader.second;
        std::shared_ptr<ComponentModule> module;
        if (loader.fetchModule(&module) == C2_OK) {
            std::shared_ptr<const C2Component::Traits> traits = module->getTraits();
            if (traits) {
                mComponentList.push_back(traits);
                mComponentNameToPath.emplace(traits->name, path);
                for (const C2String &alias : traits->aliases) {
                    mComponentNameToPath.emplace(alias, path);
                }
            }
        }
    }
    mVisited = true;
}

std::vector<std::shared_ptr<const C2Component::Traits>> OzoAudioC2Store::listComponents() {
    // This method SHALL return within 500ms.
    visitComponents();
    return mComponentList;
}

c2_status_t OzoAudioC2Store::findComponent(
        C2String name, std::shared_ptr<ComponentModule> *module) {
    (*module).reset();
    visitComponents();

    auto pos = mComponentNameToPath.find(name);
    if (pos != mComponentNameToPath.end()) {
        return mComponents.at(pos->second).fetchModule(module);
    }
    return C2_NOT_FOUND;
}

c2_status_t OzoAudioC2Store::createComponent(
        C2String name, std::shared_ptr<C2Component> *const component) {
    // This method SHALL return within 100ms.
    component->reset();
    std::shared_ptr<ComponentModule> module;
    c2_status_t res = findComponent(name, &module);
    if (res == C2_OK) {
        // TODO: get a unique node ID
        res = module->createComponent(0, component);
    }
    return res;
}

c2_status_t OzoAudioC2Store::createInterface(
        C2String name, std::shared_ptr<C2ComponentInterface> *const interface) {
    // This method SHALL return within 100ms.
    interface->reset();
    std::shared_ptr<ComponentModule> module;
    c2_status_t res = findComponent(name, &module);
    if (res == C2_OK) {
        // TODO: get a unique node ID
        res = module->createInterface(0, interface);
    }
    return res;
}

c2_status_t OzoAudioC2Store::querySupportedParams_nb(
        std::vector<std::shared_ptr<C2ParamDescriptor>> *const params) const {
    return mInterface.querySupportedParams(params);
}

c2_status_t OzoAudioC2Store::querySupportedValues_sm(
        std::vector<C2FieldSupportedValuesQuery> &fields) const {
    return mInterface.querySupportedValues(fields, C2_MAY_BLOCK);
}

C2String OzoAudioC2Store::getName() const {
    return "ozoaudio";
}

std::shared_ptr<C2ParamReflector> OzoAudioC2Store::getParamReflector() const {
    return mReflector;
}


namespace {
    std::mutex gPreferredComponentStoreMutex;
    std::shared_ptr<C2ComponentStore> gPreferredComponentStore;

    std::mutex gPlatformAllocatorStoreMutex;
    std::weak_ptr<OzoAllocatorStoreImpl> gPlatformAllocatorStore;
}

std::shared_ptr<C2ComponentStore> GetPreferredOzoComponentStore() {
    std::lock_guard<std::mutex> lock(gPreferredComponentStoreMutex);
    return gPreferredComponentStore ? gPreferredComponentStore : GetOzoAudioCodec2VendorComponentStore();
}

std::shared_ptr<C2AllocatorStore> GetOzoAllocatorStore() {
    std::lock_guard<std::mutex> lock(gPlatformAllocatorStoreMutex);
    std::shared_ptr<OzoAllocatorStoreImpl> store = gPlatformAllocatorStore.lock();
    if (store == nullptr) {
        store = std::make_shared<OzoAllocatorStoreImpl>();
        store->setComponentStore(GetPreferredOzoComponentStore());
        gPlatformAllocatorStore = store;
    }
    return store;
}


namespace {

class _C2BlockPoolCache {
public:
    _C2BlockPoolCache() {}

    c2_status_t createBlockPool(
            OzoAllocatorStore::id_t allocatorId,
            std::shared_ptr<const C2Component> component,
            C2BlockPool::local_id_t poolId,
            std::shared_ptr<C2BlockPool> *pool
    ) {
        std::shared_ptr<C2AllocatorStore> allocatorStore = GetOzoAllocatorStore();
        std::shared_ptr<C2Allocator> allocator;
        c2_status_t res = C2_NOT_FOUND;

        switch(allocatorId) {
            case OzoAllocatorStore::ION:
            case C2AllocatorStore::DEFAULT_LINEAR:
                res = allocatorStore->fetchAllocator(
                        C2AllocatorStore::DEFAULT_LINEAR, &allocator);
                if (res == C2_OK) {
                    std::shared_ptr<C2BlockPool> ptr =
                            std::make_shared<C2PooledBlockPool>(
                                    allocator, poolId);
                    *pool = ptr;
                    mBlockPools[poolId] = ptr;
                    mComponents[poolId] = component;
                }
                break;
            case OzoAllocatorStore::GRALLOC:
            case C2AllocatorStore::DEFAULT_GRAPHIC:
                res = allocatorStore->fetchAllocator(
                        C2AllocatorStore::DEFAULT_GRAPHIC, &allocator);
                if (res == C2_OK) {
                    std::shared_ptr<C2BlockPool> ptr =
                        std::make_shared<C2PooledBlockPool>(allocator, poolId);
                    *pool = ptr;
                    mBlockPools[poolId] = ptr;
                    mComponents[poolId] = component;
                }
                break;
            default:
                // Try to create block pool from platform store plugins.
                std::shared_ptr<C2BlockPool> ptr;
                res = C2PlatformStorePluginLoader::GetInstance()->createBlockPool(
                        allocatorId, poolId, &ptr);
                if (res == C2_OK) {
                    *pool = ptr;
                    mBlockPools[poolId] = ptr;
                    mComponents[poolId] = component;
                }
                break;
        }

        return res;
    }

    bool getBlockPool(
            C2BlockPool::local_id_t blockPoolId,
            std::shared_ptr<const C2Component> component,
            std::shared_ptr<C2BlockPool> *pool) {
        // TODO: use one iterator for multiple blockpool type scalability.
        std::shared_ptr<C2BlockPool> ptr;
        auto it = mBlockPools.find(blockPoolId);
        if (it != mBlockPools.end()) {
            ptr = it->second.lock();
            if (!ptr) {
                mBlockPools.erase(it);
                mComponents.erase(blockPoolId);
            } else {
                auto found = mComponents.find(blockPoolId);
                if (component == found->second.lock()) {
                    *pool = ptr;
                    return true;
                }
            }
        }
        return false;
    }

private:
    std::map<C2BlockPool::local_id_t, std::weak_ptr<C2BlockPool>> mBlockPools;
    std::map<C2BlockPool::local_id_t, std::weak_ptr<const C2Component>> mComponents;
};

static std::unique_ptr<_C2BlockPoolCache> sBlockPoolCache = std::make_unique<_C2BlockPoolCache>();
static std::mutex sBlockPoolCacheMutex;

} // anynymous namespace


c2_status_t
GetOzoCodec2BlockPool(
        C2BlockPool::local_id_t id,
        std::shared_ptr<const C2Component> component,
        std::shared_ptr<C2BlockPool> *pool
) {
    pool->reset();
    std::lock_guard<std::mutex> lock(sBlockPoolCacheMutex);
    std::shared_ptr<C2AllocatorStore> allocatorStore = GetOzoAllocatorStore();
    std::shared_ptr<C2Allocator> allocator;
    c2_status_t res = C2_NOT_FOUND;

    if (id >= C2BlockPool::PLATFORM_START) {
        if (sBlockPoolCache->getBlockPool(id, component, pool)) {
            return C2_OK;
        }
    }

    switch (id) {
    case C2BlockPool::BASIC_LINEAR:
        res = allocatorStore->fetchAllocator(C2AllocatorStore::DEFAULT_LINEAR, &allocator);
        if (res == C2_OK) {
            *pool = std::make_shared<C2BasicLinearBlockPool>(allocator);
        }
        break;
    case C2BlockPool::BASIC_GRAPHIC:
        res = allocatorStore->fetchAllocator(C2AllocatorStore::DEFAULT_GRAPHIC, &allocator);
        if (res == C2_OK) {
            *pool = std::make_shared<C2BasicGraphicBlockPool>(allocator);
        }
        break;
    default:
        res = sBlockPoolCache->createBlockPool(C2AllocatorStore::DEFAULT_LINEAR, component, id, pool);
        break;
    }

    return res;
}


std::shared_ptr<C2ComponentStore> GetOzoAudioCodec2VendorComponentStore() {
    static std::mutex mutex;
    static std::weak_ptr<C2ComponentStore> ozoStore;
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<C2ComponentStore> store = ozoStore.lock();
    if (store == nullptr) {
        store = std::make_shared<OzoAudioC2Store>();
        ozoStore = store;
    }

    return store;
}

} // namespace android
