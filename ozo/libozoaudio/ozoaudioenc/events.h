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

#ifndef OZOCODEC_EVENTS_H_
#define OZOCODEC_EVENTS_H_

namespace android {

enum {
    // Windscreen event ID
    kOzoWindNoiseEvent = 0x7F100001,

    // Sound source event ID
    kOzoSoundSourceEvent = 0x7F100002,

    // Sound source sectors event ID (azimuth + elevation)
    kOzoSoundSourceSectorsEvent1 = 0x7F100003,

    // Sound source sectors event ID (sectors width + height)
    kOzoSoundSourceSectorsEvent2 = 0x7F100004,

    // Audio levels event ID
    kOzoAudioLevelEvent = 0x7F100005,
    kOzoAudioLevelObserverID = 26001,

    // Mic blocking event ID
    kOzoMicBlockingEvent = 0x7F100006,
};

}  // namespace android

#endif // OZOCODEC_EVENTS_H_
