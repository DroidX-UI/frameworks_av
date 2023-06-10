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

#include "ozoaudio/utils.h"
#include "ozoaudioencoder/config.h"
#include "./ozo_wrapper.h"
#include "./events.h"

namespace android {

class WnrEventEmitter: public EventEmitter, public ozoaudioencoder::OzoEncoderWindIndicationObserver
{
public:
    WnrEventEmitter();

    virtual ozoaudioencoder::ObserverMode getMode() const override;
    virtual bool process(const void *data, size_t size) override;
};

class SoundSourceEmitter: public EventEmitter
{
public:
    SoundSourceEmitter();

    virtual ozoaudioencoder::ObserverMode getMode() const override;
    virtual bool process(const void *data, size_t size) override;
};

class SoundSourceSectorsEmitter: public EventEmitter
{
public:
    SoundSourceSectorsEmitter(OzoEncoderWrapper *handle);

    virtual ozoaudioencoder::ObserverMode getMode() const override;
    virtual bool isSdkObserver() const override;

protected:
    OzoEncoderWrapper *m_handle;
};

class SoundSourceSectorsEmitter1: public SoundSourceSectorsEmitter
{
public:
    SoundSourceSectorsEmitter1(OzoEncoderWrapper *handle);

    virtual bool process(const void *data, size_t size) override;
};

class SoundSourceSectorsEmitter2: public SoundSourceSectorsEmitter
{
public:
    SoundSourceSectorsEmitter2(OzoEncoderWrapper *handle);

    virtual bool process(const void *data, size_t size) override;
};

class AudioLevelsEmitter: public EventEmitter
{
public:
    AudioLevelsEmitter();

    virtual ozoaudioencoder::ObserverMode getMode() const override;
    virtual bool process(const void *data, size_t size) override;
    virtual bool isSdkObserver() const override;
};

class MicBlockingEmitter: public EventEmitter
{
public:
    MicBlockingEmitter();

    virtual ozoaudioencoder::ObserverMode getMode() const override;
    virtual bool process(const void *data, size_t size) override;
};

}  // namespace android
