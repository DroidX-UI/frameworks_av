#pragma once
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

/**
 * @file
 * @brief Helpers for OZO Audio integration.
 */

#include <stdint.h>
#include <string.h>
#include <vector>
#include <string>
#include <sstream>

#include "ozoaudioencoder/post_utils.h"
#include "ozoaudiocommon/license_reader.h"
#include "ozoaudiocommon/capability.h"

namespace android {

static const char OZO_LICENSE_FILE[] = "/vendor/etc/ozosdk.license";

static const int MAX_EVENT_DATA_SIZE = 64;

static inline bool setOzoAudioSdkLicense()
{
    int value = 0;
    const auto &manager = ozoaudiocodec::OzoAudioCapabilityManager::create();

    manager.getValue(ozoaudiocodec::kHasLicenseSet, value);
    if (value == 0) {
        ozoaudiocodec::OzoLicenseReaderAsFile license;

        if (!license.init(OZO_LICENSE_FILE)) {
            ALOGE("Unable to read Ozo Audio SDK license file %s", OZO_LICENSE_FILE);
            return false;
        }
        ALOGD("License data size: %zu", license.size());

        if (!ozoaudiocodec::setLicense(license)) {
            ALOGE("Ozo Audio SDK license data not accepted");
            return false;
        }
    }

    return true;
}

static inline size_t copy_packet(uint8_t *dest, const uint8_t *src, size_t size, size_t sizeBytes)
{
    memcpy(dest, &size, sizeBytes);
    memcpy(dest + sizeBytes, src, size);
    return size + sizeBytes;
}

class EventEmitter : public ozoaudioencoder::OzoEncoderObserver
{
public:
    EventEmitter() : ozoaudioencoder::OzoEncoderObserver(), m_notify(false), m_eventId(-1), m_dataLen(0)
    {
        memset(m_data, 0, sizeof(m_data));
    }

    virtual ~EventEmitter() {}

    void setEvent(int data)
    {
        this->m_data[this->m_dataLen] = data;
        this->m_dataLen += 1;
        this->m_notify = true;
    }

    bool getEvent(int &eventId, int **data, int &dataLen)
    {
        eventId = this->m_eventId;
        *data = this->m_data;
        dataLen = this->m_dataLen;

        auto ret = this->m_notify;
        this->m_notify = false;
        this->m_dataLen = 0;

        return ret;
    }

    bool hasEvent() const
    {
        return this->m_notify;
    }

    virtual bool isSdkObserver() const
    {
        return true;
    }

protected:
    bool m_notify;
    int m_eventId;
    int m_data[MAX_EVENT_DATA_SIZE];
    int m_dataLen;
};

// Use explicit sizes to make sure that these remain the same in 32-bit and 64-bit environment
static const size_t DATA_SIZE = 4;
static const size_t EVENTS_COUNT = 4;
static const size_t EVENTS_ID = 4;
static const size_t EVENTS_TS = 4;
static const size_t EVENTS_DATA_LEN = 4;

class OzoAudioUtilities
{
public:
    /**
     * Event data mapping.
     */
    struct EventData
    {
        /**
         * Event ID.
         */
        int event;

        /**
         * Event timestamp in milliseconds
         */
        int32_t timestamp;

        /**
         * Event data.
         */
        int data[MAX_EVENT_DATA_SIZE];

        /**
         * Event data size.
         */
        int dataSize;
    };

    /**
     * Query number of events in the events data buffer.
     *
     * @param data Events data buffer
     * @param [out] nEvents Number of events detected
     * @return Updated data buffer pointer (points now to the actual events data)
     */
    static uint8_t *get_events_count(const uint8_t *data, size_t &nEvents)
    {
        memcpy(&nEvents, data, EVENTS_COUNT);
        return (uint8_t *)(data + EVENTS_COUNT);
    }

    /**
     * Query events data size.
     *
     * @param data Events data buffer
     * @return Data size in bytes
     */
    static size_t get_events_data_size(const uint8_t *data)
    {
        size_t nEvents;
        uint8_t *ptr = OzoAudioUtilities::get_events_count(data, nEvents);

        size_t count = 0;
        for (size_t i = 0; i < nEvents; i++) {
            struct EventData event_data;

            memcpy(&event_data.event, ptr + count, EVENTS_ID);
            count += EVENTS_ID;

            if (event_data.event) {
                count += EVENTS_TS;

                int dataSize;
                memcpy(&dataSize, ptr + count, EVENTS_DATA_LEN);
                count += EVENTS_DATA_LEN + dataSize * 4;
            }
        }

        return count ? count + EVENTS_COUNT : 0;
    }

    /**
     * Query next event data.
     *
     * @param data Events data buffer
     * @param event_data [out] Event data
     * @return Updated data buffer pointer (points now to the next event data)
     */
    static uint8_t *get_next_event_data(const uint8_t *data, struct EventData &event_data)
    {
        uint8_t *ptr = (uint8_t *)data;

        memcpy(&event_data.event, ptr, EVENTS_ID);
        ptr += EVENTS_ID;

        if (event_data.event) {
            memcpy(&event_data.timestamp, ptr, EVENTS_TS);
            ptr += EVENTS_TS;

            memcpy(&event_data.dataSize, ptr, EVENTS_DATA_LEN);
            ptr += EVENTS_DATA_LEN;

            memcpy(event_data.data, ptr, event_data.dataSize * 4);
            ptr += event_data.dataSize * 4;
        }

        return ptr;
    }

    // Combine encoded audio, raw mic data and associated control data into single buffer
    static size_t packetize_data(const uint8_t *enc_buffer,
                                 size_t enc_size,
                                 ozoaudioencoder::BasePostProcessingWriter *mic,
                                 ozoaudioencoder::BasePostProcessingWriter *ctrl,
                                 std::vector<EventEmitter *> eventEmitters,
                                 int32_t timestampInMs,
                                 uint8_t *dest)
    {
        // Add encoded data buffer
        auto nbytes = copy_packet(dest, enc_buffer, enc_size, DATA_SIZE);

        // Add mic data buffer, allow mic buffer reset
        nbytes += copy_packet(dest + nbytes, mic ? mic->data() : nullptr, mic ? mic->size() : 0,
                              DATA_SIZE);
        if (mic)
            mic->setClear(true);

        // Add ctrl data buffer, allow control buffer reset
        nbytes += copy_packet(dest + nbytes, ctrl ? ctrl->data() : nullptr, ctrl ? ctrl->size() : 0,
                              DATA_SIZE);
        if (ctrl)
            ctrl->setClear(true);


        /**
         * Event data is serialized into following format:
         *
         * | events | event-1 | event-2 | ...
         *
         * Where events describes the number of events present (EVENTS_COUNT bytes)
         *
         * The actual event data is serialized according to
         * - event id (EVENTS_ID bytes)
         * - event timestamp (EVENTS_TS bytes)
         * - event data size (EVENTS_DATA_LEN bytes)
         * - event data (varying size)
         *
         * Event ID 0 is used to indicate that no further event data is present.
         */

        // Detect if there are any actual events to be serialized
        size_t actualEvents = 0;
        for (size_t i = 0; i < eventEmitters.size(); i++)
            if (eventEmitters[i]->hasEvent()) {
                actualEvents = eventEmitters.size();
                break;
            }

        // Number of events
        memcpy(dest + nbytes, &actualEvents, EVENTS_COUNT);
        nbytes += EVENTS_COUNT;

        if (actualEvents) {
            for (size_t i = 0; i < actualEvents; i++) {
                int *data;
                int event = 0, dataLen = 0;
                if (eventEmitters[i]->getEvent(event, &data, dataLen)) {
                    // Event type
                    memcpy(dest + nbytes, &event, EVENTS_ID);
                    nbytes += EVENTS_ID;

                    // Event timestamp
                    memcpy(dest + nbytes, &timestampInMs, EVENTS_TS);
                    nbytes += EVENTS_TS;

                    // Event data size
                    memcpy(dest + nbytes, &dataLen, EVENTS_DATA_LEN);
                    nbytes += EVENTS_DATA_LEN;

                    // Event data
                    memcpy(dest + nbytes, data, dataLen * 4);
                    nbytes += dataLen * 4;
                }
                else {
                    event = 0;
                    memcpy(dest + nbytes, &event, EVENTS_ID);
                    nbytes += EVENTS_ID;
                }
            }
        }

        return nbytes;
    }

    // Unpack data sizes from specified buffer, return also data pointers for mic and control data
    static size_t depacketize_data(const uint8_t *data,
                                   size_t &enc,
                                   size_t &mic,
                                   size_t &ctrl,
                                   size_t &size_unit,
                                   uint8_t **mic_data,
                                   uint8_t **ctrl_data,
                                   uint8_t **events_data)
    {
        size_unit = DATA_SIZE;

        memcpy(&enc, data, size_unit);
        data += enc + size_unit;

        memcpy(&mic, data, size_unit);
        data += size_unit;
        *mic_data = (uint8_t *)data;
        data += mic;

        memcpy(&ctrl, data, size_unit);
        data += size_unit;
        *ctrl_data = (uint8_t *)data;
        data += ctrl;

        *events_data = (uint8_t *)data;

        return OzoAudioUtilities::get_events_data_size(*events_data);
    }
};

static inline void
calcMaxOzoAudioLevels(ozoaudioencoder::OzoEncoderObserver *observer, const int16_t *left, const int16_t *right, size_t len)
{
    int16_t maxL = 0, maxR = 0;

    for (size_t i = 0; i < len; i++, left+=2, right+=2) {
        int16_t l = abs(*left);
        int16_t r = abs(*right);

        if (maxL < l) {
            maxL = l;
        }

        if (maxR < r) {
            maxR = r;
        }
    }

    int16_t dataLR[2] = {maxL, maxR};
    observer->process(dataLR, sizeof(dataLR));
}

static inline std::vector<std::string>
splitOzoParamString(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

} // namespace android
