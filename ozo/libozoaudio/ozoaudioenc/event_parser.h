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
#include "./events.h"

namespace android {

class OzoEventParser
{
public:
    OzoEventParser(OzoAudioUtilities::EventData &event): m_event(event) {}

    int getEventsCount() const
    {
        return (m_event.event == kOzoWindNoiseEvent) ? m_event.dataSize : m_event.data[0];
    }

    bool getNotifyData(int &msg, int &ext1, int &ext2, int index) const
    {
        bool result = false;

        // WNR event data
        if (m_event.event == kOzoWindNoiseEvent) {
            msg = m_event.event;
            ext1 = m_event.timestamp;
            ext2 = m_event.data[index];
            result = true;
        }

        // Sound source event data
        // Sound source sectors data (azimuth + elevation)
        // Sound source sectors data (width + height)
        // Audio levels (L + R) data
        // Mic blocking data
        else if (m_event.event == kOzoSoundSourceEvent ||
                 m_event.event == kOzoSoundSourceSectorsEvent1 ||
                 m_event.event == kOzoSoundSourceSectorsEvent2 ||
                 m_event.event == kOzoAudioLevelEvent ||
                 m_event.event == kOzoMicBlockingEvent) {
            int *data = &m_event.data[1 + index * 2];

            msg = m_event.event;
            ext1 = data[0];
            ext2 = data[1];
            result = true;
        }

        return result;
    }

private:
    OzoAudioUtilities::EventData &m_event;
};

}  // namespace android
