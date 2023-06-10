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

#include <vector>

//#define LOG_NDEBUG 0
#define LOG_TAG "EventEmitter"
#include <log/log.h>
#include <utils/Log.h>

#include "./event_emitter.h"
#include "ozoaudiocommon/control.h"

namespace android {

WnrEventEmitter::WnrEventEmitter():
EventEmitter(),
OzoEncoderWindIndicationObserver()
{
    m_eventId = kOzoWindNoiseEvent;
}

ozoaudioencoder::ObserverMode
WnrEventEmitter::getMode() const
{
    return ozoaudioencoder::ObserverMode::WINDNOISE;
}

bool
WnrEventEmitter::process(const void *data, size_t size)
{
    auto result = ozoaudioencoder::OzoEncoderWindIndicationObserver::process(data, size);
    if (result) {
        // Pack the wind status related data into one variable
        int status = this->m_windParams.windDetected ? 1 : 0;
        int data = (((int) this->m_windParams.windLevel) << 1) | status;

        // This is to be sent to the application layer as is
        this->setEvent(data);
    }

    return result;
}


SoundSourceEmitter::SoundSourceEmitter():
EventEmitter()
{
    m_eventId = kOzoSoundSourceEvent;
}

ozoaudioencoder::ObserverMode
SoundSourceEmitter::getMode() const
{
    return ozoaudioencoder::ObserverMode::SOUND_SOURCE;
}

bool
SoundSourceEmitter::process(const void *data, size_t size)
{
    auto sources = (const ozoaudioencoder::OzoAudioSoundSource *) data;
    auto nItems = size / sizeof(ozoaudioencoder::OzoAudioSoundSource);

    this->setEvent(nItems);
    for (size_t i = 0; i < nItems; i++) {
        int ext1 = 0;

        /*
         * Bits 0-6: source strength (9-bits)
         * Bits 7-14: source elevation (8-bits)
         * Bits 15-23: source azimuth (7-bits)
         */
        ext1 |= (sources[i].azimuth + 180) << 15;
        ext1 |= (sources[i].elevation + 90) << 7;
        ext1 |= sources[i].strength;

        this->setEvent(ext1);
        this->setEvent(sources[i].id);
        //ALOGD("Received: %i %i %i", sources[i].azimuth, sources[i].elevation, sources[i].strength);
    }

    return true;
}


SoundSourceSectorsEmitter::SoundSourceSectorsEmitter(OzoEncoderWrapper *handle):
EventEmitter(),
m_handle(handle)
{}

ozoaudioencoder::ObserverMode
SoundSourceSectorsEmitter::getMode() const
{
    return (ozoaudioencoder::ObserverMode) 10000;
}

bool
SoundSourceSectorsEmitter::isSdkObserver() const
{
    return false;
}

struct SoundSourceSectors_s
{
    int id;
    int azimuth;
    int elevation;
    int width;
    int height;
};

SoundSourceSectorsEmitter1::SoundSourceSectorsEmitter1(OzoEncoderWrapper *handle):
SoundSourceSectorsEmitter(handle)
{
    m_eventId = kOzoSoundSourceSectorsEvent1;
}

bool
SoundSourceSectorsEmitter1::process(const void *, size_t)
{
    auto ctrl = this->m_handle->getOzoAudioHandle()->getControl();

    int32_t nSectors;
    auto result = ctrl->getState(ozoaudiocodec::kAnalysisSectorId, ozoaudiocodec::kAnalysisSectorCount, nSectors);
    if (result == ozoaudiocodec::AudioCodecError::OK) {
        std::vector<struct SoundSourceSectors_s> sectors;

        for (int32_t i = 0; i < nSectors; i++) {
            struct SoundSourceSectors_s data;

            ctrl->getState(ozoaudiocodec::kAnalysisSectorId, i,
                ozoaudiocodec::kAnalysisSectorQueryId, data.id);

            ctrl->getState(ozoaudiocodec::kAnalysisSectorId, i,
                ozoaudiocodec::kAnalysisSectorAzimuth, data.azimuth);

            ctrl->getState(ozoaudiocodec::kAnalysisSectorId, i,
                ozoaudiocodec::kAnalysisSectorElevation, data.elevation);

            sectors.push_back(data);
        }

        ALOGD("Number of sectors (azimuth + elevation): %i", nSectors);
        this->setEvent(nSectors);

        for (size_t i = 0; i < sectors.size(); i++) {
            int ext1;

            ALOGD("%zu: id=%i azimuth=%i elevation%i",
                i, sectors[i].id, sectors[i].azimuth, sectors[i].elevation);

            ext1 = 0;
            ext1 |= sectors[i].azimuth + 180;
            ext1 |= (sectors[i].elevation + 90) << 16;

            this->setEvent(ext1);
            this->setEvent(sectors[i].id);
        }
    }

    return false;
}

SoundSourceSectorsEmitter2::SoundSourceSectorsEmitter2(OzoEncoderWrapper *handle):
SoundSourceSectorsEmitter(handle)
{
    m_eventId = kOzoSoundSourceSectorsEvent2;
}

bool
SoundSourceSectorsEmitter2::process(const void *, size_t)
{
    auto ctrl = this->m_handle->getOzoAudioHandle()->getControl();

    int32_t nSectors;
    auto result = ctrl->getState(ozoaudiocodec::kAnalysisSectorId, ozoaudiocodec::kAnalysisSectorCount, nSectors);
    if (result == ozoaudiocodec::AudioCodecError::OK) {
        std::vector<struct SoundSourceSectors_s> sectors;

        for (int32_t i = 0; i < nSectors; i++) {
            struct SoundSourceSectors_s data;

            ctrl->getState(ozoaudiocodec::kAnalysisSectorId, i,
                ozoaudiocodec::kAnalysisSectorQueryId, data.id);

            ctrl->getState(ozoaudiocodec::kAnalysisSectorId, i,
                ozoaudiocodec::kAnalysisSectorWidth, data.width);

            ctrl->getState(ozoaudiocodec::kAnalysisSectorId, i,
                ozoaudiocodec::kAnalysisSectorHeight, data.height);

            sectors.push_back(data);
        }

        ALOGD("Number of sectors (width + height): %i", nSectors);
        this->setEvent(nSectors);

        for (size_t i = 0; i < sectors.size(); i++) {
            int ext1;

            ALOGD("%zu: id=%i width=%i height=%i",
                i, sectors[i].id, sectors[i].width, sectors[i].height);

            ext1 = 0;
            ext1 |= sectors[i].width;
            ext1 |= sectors[i].height << 16;

            this->setEvent(ext1);
            this->setEvent(sectors[i].id);
        }
    }

    return false;
}


AudioLevelsEmitter::AudioLevelsEmitter():
EventEmitter()
{
    m_eventId = kOzoAudioLevelEvent;
}

ozoaudioencoder::ObserverMode
AudioLevelsEmitter::getMode() const
{
    return (ozoaudioencoder::ObserverMode) kOzoAudioLevelObserverID;
}

bool
AudioLevelsEmitter::process(const void *data, size_t /*size*/)
{
    auto levels = (const int16_t *) data;

    this->setEvent(1);
    this->setEvent(levels[0]);
    this->setEvent(levels[1]);
    //ALOGD("Audio levels: L=%i R=%i", levels[0], levels[1]);

    return true;
}

bool
AudioLevelsEmitter::isSdkObserver() const
{
    return false;
}


MicBlockingEmitter::MicBlockingEmitter():
EventEmitter()
{
    m_eventId = kOzoMicBlockingEvent;
}

ozoaudioencoder::ObserverMode
MicBlockingEmitter::getMode() const
{
    return ozoaudioencoder::ObserverMode::MIC_BLOCKING;
}

bool
MicBlockingEmitter::process(const void *data, size_t size)
{
    auto blockingData = (const ozoaudioencoder::OzoAudioMicBlocking *) data;
    auto nItems = size / sizeof(ozoaudioencoder::OzoAudioMicBlocking);

    this->setEvent(nItems);
    //ALOGD("Mic blocking events #: %zu", nItems);

    for (size_t i = 0; i < nItems; i++) {
        this->setEvent(blockingData[i].id);
        this->setEvent(blockingData[i].value);
        //ALOGD("Mic blocking: index=%i level=%i", blockingData[i].id, blockingData[i].value);
    }

    return true;
}

}  // namespace android
