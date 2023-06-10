/*******************************************************************
*
* Name        : utils.h
* Version     : 1.0.0
* Date        : 2018-12-01
* Description : Header file for utils tool
*
********************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <system/audio.h>


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

bool checkGameSoundVolumeGapSwitch();
bool checkGameSound4DUISwitch();
bool isAppSupportedBySound4D(pid_t processID, audio_format_t format,
              audio_stream_type_t streamType,  uint32_t sampleRate, audio_channel_mask_t channelMask, unsigned int* appID);
bool isAppNeedtoChangeStreamType(pid_t processID, audio_format_t format,
                                       audio_stream_type_t streamType,  uint32_t sampleRate, audio_channel_mask_t channelMask);
bool isAppNeedtoChangeInputSource(pid_t processID, audio_format_t format,
                                       audio_source_t inputSource,  uint32_t sampleRate, audio_channel_mask_t channelMask);

#ifdef __cplusplus
        }
#endif //__cplusplus

#endif /* __UTILS_H__ */
