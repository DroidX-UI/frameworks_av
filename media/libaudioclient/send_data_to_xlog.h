#ifndef SEND_DATA_TO_XLOG_H
#define SEND_DATA_TO_XLOG_H


void send_audiotrack_parameter_to_xlog(int playbackTime, const char* clientName, const char* usage, const char* flags, int channelMask, const char* format, int sampleRate);

#endif