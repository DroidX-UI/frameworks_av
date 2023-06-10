/*
Copyright (C) 2020 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

#pragma once

#include <dlfcn.h>

#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/MediaDefs.h>

#include "ozoaudiodecoder/ozo_audio_decoder.h"

// Location of OZO Audio library
#if defined(__arm__)
#define OZOPATH "/system_ext/lib/libozoaudio.so"
#else // defined(__aarch64__)
#define OZOPATH "/system_ext/lib64/libozoaudio.so"
#endif

namespace android {

static inline bool
setOzoMimeType(const uint8_t *csd, size_t size, AString &mime)
{
    bool isOzo = false;
    int32_t sample_rate, num_channels;

    // Check if AAC related MIME type should be converted to OZO Audio
    if (mime == MEDIA_MIMETYPE_AUDIO_AAC) {
        void *ozoLib = dlopen(OZOPATH, RTLD_NOW);
        ALOGD("Open libozoaudio library = %p (%s)", ozoLib, OZOPATH);
        if (ozoLib == NULL) {
            ALOGW("Could not open OZO Audio library: %s", dlerror());
        } else {
            bool (*detectOzoAudio)(const char *codec, const uint8_t *decoder_config, size_t size);
            bool (*readCSD)(const uint8_t *decoder_config, size_t size, int32_t &sample_rate, int32_t &num_channels);

            detectOzoAudio = (bool (*)(const char *, const uint8_t *, size_t)) dlsym(ozoLib, "_ZN15ozoaudiodecoder14detectOzoAudioEPKcPKhj");
            readCSD = (bool (*)(const uint8_t *decoder_config, size_t size, int32_t &sample_rate, int32_t &num_channels)) dlsym(ozoLib, "_ZN15ozoaudiodecoder7readCSDEPKhjRiS2_");

            if (detectOzoAudio && readCSD) {
                if (detectOzoAudio("aac", csd, size)) {
                    isOzo = true;
                    mime = MEDIA_MIMETYPE_AUDIO_OZOAUDIO;
                    ALOGD("OZO Audio detected, mime type changed to %s", MEDIA_MIMETYPE_AUDIO_OZOAUDIO);
                } else if (readCSD(csd, size, sample_rate, num_channels)) {
                    ALOGD("CSD config data: channels=%i, samplerate=%i", num_channels, sample_rate);
                    if (num_channels == 4)
                        mime = MEDIA_MIMETYPE_AUDIO_OZOAUDIO;
                }
            }

            dlclose(ozoLib);
        }
    }

    return isOzo;
}

}  // namespace android
