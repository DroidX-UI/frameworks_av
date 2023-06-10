#ifndef SEND_DATA_TO_XLOG_H
#define SEND_DATA_TO_XLOG_H

#include <system/audio.h>

int send_headphone_type_to_xlog(audio_devices_t type, const char* name, audio_format_t encodedFormat);
void send_projection_screen_to_xlog();

#endif
