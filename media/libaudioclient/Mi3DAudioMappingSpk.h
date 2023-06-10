
#ifndef MI3DAUDIOMAPPINGSPK_H
#define MI3DAUDIOMAPPINGSPK_H

#include "CMusicLimiter.h"

#define InChannelNum 6
#define OutChannelNum 4

#define NORM_16 (1.0 / 32768)
#define DENORM_16 32767.0
#define NORM_24 (1.0/8388608)
#define DENORM_24 8388607.0
#define NORM_32 (1.0 / 2.147483648000000e+09)
#define DENORM_32 (2.147483647000000e+09)

namespace android {

    class Mi3DAudioMappingSpk
    {
    public:
        Mi3DAudioMappingSpk();
        Mi3DAudioMappingSpk(int fs_, int in_channel_num, int bit_width);
        void process(int * output[], int * input[], int samples);
        void process(void * output, const void * input, int bytenums, int bit_width);
        void process(float * output, const void * input, int bytenums);
        void process(float * output[], float * input[], int numsamples);
        void setMode(int modeValue);
        ~Mi3DAudioMappingSpk();
        int getOutputChannelsNum();

    private:
        int enable;
        int fs;
        int mode;
        float norm;
        float denorm;
        int channelInNum;
        int channelOutNum;

        int * BuffChannelInInt[InChannelNum];
        int * BuffChannelOutInt[OutChannelNum];
        float * BuffChannelIn[InChannelNum];
        float * BuffChannelOut[OutChannelNum];

        float ampChannelIn[InChannelNum];

        CMusicLimiter_State_t * limiter[OutChannelNum];

        float limitThreshold;
    };
}; // namespace android
#endif
