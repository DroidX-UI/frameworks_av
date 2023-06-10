#ifndef MI3DAUDIOMAPPINGHEADPHONE_H
#define MI3DAUDIOMAPPINGHEADPHONE_H

#include "CMusicLimiter.h"
#include "fast_conv.h"

#define HeadPhoneInChannelNum 6
#define HeadPhoneOutChannelNum 2

#define HeadPhoneWrapperOutChannelNum 4

#define NORM_16 (1.0 / 32768)
#define DENORM_16 32767.0
#define NORM_24 (1.0/8388608)
#define DENORM_24 8388607.0
#define NORM_32 (1.0 / 2.147483648000000e+09)
#define DENORM_32 (2.147483647000000e+09)

namespace android {

class Mi3DAudioMappingHeadphone
{
public:
    Mi3DAudioMappingHeadphone();
    Mi3DAudioMappingHeadphone(int fs_, int in_channel_num, int bit_width);
    void process(int * output[], int * input[], int samples);
    void process(float * output[], float * input[], int samples);
    void process(void * output, const void * input, int bytenums, int bit_width);
    void process(float * output, const void * input, int bytenums);
    void setEnable(int enableValue);
    ~Mi3DAudioMappingHeadphone();

private:
    int enable;
    float norm;
    float denorm;
    int channelInNum;
    int channelOutNum;

    int * BuffChannelInInt[HeadPhoneInChannelNum];
    int * BuffChannelOutInt[HeadPhoneWrapperOutChannelNum];
    float * BuffChannelIn[HeadPhoneInChannelNum];
    float * BuffChannelOut[HeadPhoneWrapperOutChannelNum];

    CMusicLimiter_State_t * limiter[HeadPhoneOutChannelNum];
    float limitThreshold;

    float * hrtfCoeff[2][HeadPhoneInChannelNum];
    FastConv * fastconvObj[2][HeadPhoneInChannelNum];

    FastConv * eq[2];
};
};
#endif
