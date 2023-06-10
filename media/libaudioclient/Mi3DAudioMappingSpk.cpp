
#include "Mi3DAudioMappingSpk.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

namespace android {

    Mi3DAudioMappingSpk::Mi3DAudioMappingSpk()
        :enable(1),
        norm(1.0 / 2.147483648e+09),
        denorm(2.147483647e+09),
        channelInNum(InChannelNum),
        channelOutNum(OutChannelNum),
        limitThreshold(0.99)
    {
        ampChannelIn[0] = 1.4;
        ampChannelIn[1] = 1.4;
        ampChannelIn[2] = 0.707;
        ampChannelIn[3] = 0.707;
        ampChannelIn[4] = 2.0;
        ampChannelIn[5] = 2.0;

        mode = 1;

        float sample_rate = 48000;
        float attackRatio = 1.f - expf(-2.2 / (0.0002f * sample_rate));
        float releaseRatio = 1.f - expf(-2.2 / (0.4f * sample_rate));
        for (int ch = 0; ch < OutChannelNum; ch++)
            limiter[ch] = CMusicLimiter_Init(attackRatio, releaseRatio, 0.005 * sample_rate + 1, 0.1, sample_rate);

        fs = 0;
        for(int i = 0; i < InChannelNum; i++){
            BuffChannelInInt[i] = NULL;
            BuffChannelIn[i] = NULL;
        }
        for(int i = 0; i < OutChannelNum; i++){
            BuffChannelOutInt[i] = NULL;
            BuffChannelOut[i] = NULL;
        }
    }

    Mi3DAudioMappingSpk::Mi3DAudioMappingSpk(int fs_, int in_channel_num, int bit_width)
    {
        mode = 1;
        if (in_channel_num == InChannelNum)
        {
            channelInNum = InChannelNum;
            enable = 1;
        }
        else
            enable = 0;

        fs = fs_;
        switch (bit_width)
        {
        case 16:
            norm = NORM_16;
            denorm = DENORM_16;
            break;
        case 24:
            norm = NORM_24;
            denorm = DENORM_24;
            break;
        case 32:
            norm = NORM_32;
            denorm = DENORM_32;
            break;
        }

        channelOutNum = OutChannelNum;

        ampChannelIn[0] = 1.4;
        ampChannelIn[1] = 1.4;
        ampChannelIn[2] = 0.707;
        ampChannelIn[3] = 0.707;
        ampChannelIn[4] = 2.0;
        ampChannelIn[5] = 2.0;

        float sample_rate = 48000;
        float attackRatio = 1.f - expf(-2.2 / (0.0002f * sample_rate));
        float releaseRatio = 1.f - expf(-2.2 / (0.4f * sample_rate));
        for (int ch = 0; ch < OutChannelNum; ch++)
            limiter[ch] = CMusicLimiter_Init(attackRatio, releaseRatio, 0.005 * sample_rate + 1, 0.1, sample_rate);
        limitThreshold = 0.99;

        for(int i = 0; i < InChannelNum; i++){
            BuffChannelInInt[i] = NULL;
            BuffChannelIn[i] = NULL;
        }
        for(int i = 0; i < OutChannelNum; i++){
            BuffChannelOutInt[i] = NULL;
            BuffChannelOut[i] = NULL;
        }
    }

    Mi3DAudioMappingSpk::~Mi3DAudioMappingSpk()
    {
        for (int ch = 0; ch < OutChannelNum; ch++)
        {
            if (limiter[ch] != NULL)
                CMusicLimiter_Delete(limiter[ch]);
        }
    }

    void Mi3DAudioMappingSpk::process(int * output[], int * input[], int numsamples)
    {
        if (enable == 0)
            return;

        for (int ch = 0; ch < InChannelNum; ch++)
        {
            BuffChannelIn[ch] = (float *)malloc(numsamples * sizeof(float));
            for (int ii = 0; ii < numsamples; ii++)
                BuffChannelIn[ch][ii] = input[ch][ii] * norm;
        }
        for (int ch = 0; ch < channelOutNum; ch++)
        {
            BuffChannelOut[ch] = (float *)malloc(numsamples * sizeof(float));
            memset(BuffChannelOut[ch], 0, numsamples * sizeof(float));
        }

        if (mode == 1)
            for (int ii = 0; ii < numsamples; ii++)
            {
                BuffChannelOut[0][ii] = BuffChannelIn[0][ii] * ampChannelIn[0] + BuffChannelIn[2][ii] * ampChannelIn[2];
                BuffChannelOut[1][ii] = BuffChannelIn[1][ii] * ampChannelIn[1] + BuffChannelIn[2][ii] * ampChannelIn[2];
                BuffChannelOut[2][ii] = BuffChannelIn[4][ii] * ampChannelIn[4] + BuffChannelIn[3][ii] * ampChannelIn[3];
                BuffChannelOut[3][ii] = BuffChannelIn[5][ii] * ampChannelIn[5] + BuffChannelIn[3][ii] * ampChannelIn[3];
            }
        else
        {
            for (int ii = 0; ii < numsamples; ii++)
            {
                float center_part = (BuffChannelIn[2][ii] + BuffChannelIn[3][ii]) / 2;
                float left_part = (BuffChannelIn[0][ii] + BuffChannelIn[4][ii]) / 2;
                float right_part = (BuffChannelIn[1][ii] + BuffChannelIn[5][ii]) / 2;
                BuffChannelOut[0][ii] = (left_part + center_part) * 1.8;
                BuffChannelOut[1][ii] = (right_part + center_part) * 1.8;
                BuffChannelOut[2][ii] = (left_part + center_part) * 1.8;
                BuffChannelOut[3][ii] = (right_part + center_part) * 1.8;
            }
        }

        for (int ch = 0; ch < OutChannelNum; ch++)
        {
            CMusicLimiter_ProcessBufferHardLimit(BuffChannelOut[ch], numsamples, limitThreshold, limiter[ch]);
            for (int ii = 0; ii < numsamples; ii++)
                output[ch][ii] = int(BuffChannelOut[ch][ii] * denorm);
        }

        for (int ch = 0; ch < InChannelNum; ch++)
            free(BuffChannelIn[ch]);
        for (int ch = 0; ch < OutChannelNum; ch++)
            free(BuffChannelOut[ch]);

    }

    void Mi3DAudioMappingSpk::setMode(int modeValue)
    {
        mode = modeValue;
    }

    int Mi3DAudioMappingSpk::getOutputChannelsNum()
    {
        return channelOutNum;
    }

    void Mi3DAudioMappingSpk::process(void * output, const void * input, int bytenums, int bytesPerSample)
    {
        int samplesNum = bytenums / bytesPerSample / channelOutNum;
        for (int ch = 0; ch < channelInNum; ch++)
            BuffChannelInInt[ch] = (int *)calloc(samplesNum, sizeof(int));
        for (int ch = 0; ch < channelOutNum; ch++)
            BuffChannelOutInt[ch] = (int *)calloc(samplesNum, sizeof(int));

        switch (bytesPerSample)
        {
        case 2:
        {
            int16_t * input_ptr = (int16_t *)input;
            for (int ch = 0; ch < channelInNum; ch++)
            {
                for (int ii = 0; ii < samplesNum; ii++)
                {
                    BuffChannelInInt[ch][ii] = input_ptr[ii*channelInNum + ch] << 16;
                }
            }
            break;
        }
        case 3:
        {
            for (int ch = 0; ch < channelInNum; ch++)
            {
                uint8_t * input_ptr = (uint8_t *)input;
                uint8_t * input_int_ptr = (uint8_t *)BuffChannelInInt[ch];
                for (int ii = 0; ii < samplesNum; ii++)
                {
#ifndef BIG_ENDIAN
                    input_int_ptr[4*ii+1] = input_ptr[(ii*channelInNum + ch) * 3 + 0];
                    input_int_ptr[4*ii+2] = input_ptr[(ii*channelInNum + ch) * 3 + 1];
                    input_int_ptr[4*ii+3] = input_ptr[(ii*channelInNum + ch) * 3 + 2];
#else
                    input_int_ptr[4 * ii + 1] = input_ptr[(ii*channelInNum + ch) * 3 + 2];
                    input_int_ptr[4 * ii + 2] = input_ptr[(ii*channelInNum + ch) * 3 + 1];
                    input_int_ptr[4 * ii + 3] = input_ptr[(ii*channelInNum + ch) * 3 + 0];
#endif
                }
            }
            break;
        }
        case 4:
        {
            int32_t * input_ptr = (int32_t *)input;
            for (int ch = 0; ch < channelInNum; ch++)
            {
                for (int ii = 0; ii < samplesNum; ii++)
                {
                    BuffChannelInInt[ch][ii] = input_ptr[ii*channelInNum + ch];
                }
            }
            break;
        }

        }

        process(BuffChannelOutInt, BuffChannelInInt, samplesNum);

        switch (bytesPerSample)
        {
        case 2:
        {
            int16_t * output_ptr = (int16_t *)output;
            for (int ch = 0; ch < channelOutNum; ch++)
            {
                for (int ii = 0; ii < samplesNum; ii++)
                {
                    output_ptr[ii*channelOutNum + ch] = BuffChannelOutInt[ch][ii] >> 16;
                }
            }
            break;
        }
        case 3:
        {
            for (int ch = 0; ch < channelOutNum; ch++)
            {
                uint8_t * output_ptr = (uint8_t *)output;
                uint8_t * output_int_ptr = (uint8_t *)BuffChannelOutInt[ch];
                for (int ii = 0; ii < samplesNum; ii++)
                {
#ifndef BIG_ENDIAN
                    output_ptr[(ii*channelInNum + ch) * 3 + 0] = output_int_ptr[4 * ii + 1];
                    output_ptr[(ii*channelInNum + ch) * 3 + 1] = output_int_ptr[4 * ii + 2];
                    output_ptr[(ii*channelInNum + ch) * 3 + 2] = output_int_ptr[4 * ii + 3];
#else
                    output_ptr[(ii*channelInNum + ch) * 3 + 2] = output_int_ptr[4 * ii + 1];
                    output_ptr[(ii*channelInNum + ch) * 3 + 1] = output_int_ptr[4 * ii + 2];
                    output_ptr[(ii*channelInNum + ch) * 3 + 0] = output_int_ptr[4 * ii + 3];
#endif
                }
            }
            break;
        }
        case 4:
        {
            int32_t * output_ptr = (int32_t *)output;
            for (int ch = 0; ch < channelOutNum; ch++)
            {
                for (int ii = 0; ii < samplesNum; ii++)
                {
                    output_ptr[ii*channelOutNum + ch] = BuffChannelOutInt[ch][ii];
                }
            }
            break;
        }
        }

        for (int ch = 0; ch < channelInNum; ch++)
            free(BuffChannelInInt[ch]);
        for (int ch = 0; ch < channelOutNum; ch++)
            free(BuffChannelOutInt[ch]);
    }

    void Mi3DAudioMappingSpk::process(float * output[], float * input[], int numsamples)
    {
        if (enable == 0)
            return;

        if (mode == 1)
            for (int ii = 0; ii < numsamples; ii++)
            {
                output[0][ii] = input[0][ii] * ampChannelIn[0] + input[2][ii] * ampChannelIn[2];
                output[1][ii] = input[1][ii] * ampChannelIn[1] + input[2][ii] * ampChannelIn[2];
                output[2][ii] = input[4][ii] * ampChannelIn[4] + input[3][ii] * ampChannelIn[3];
                output[3][ii] = input[5][ii] * ampChannelIn[5] + input[3][ii] * ampChannelIn[3];
            }
        else
        {
            for (int ii = 0; ii < numsamples; ii++)
            {
                float center_part = (input[2][ii] + input[3][ii]) / 2;
                float left_part = (input[0][ii] + input[4][ii]) / 2;
                float right_part = (input[1][ii] + input[5][ii]) / 2;
                output[0][ii] = (left_part + center_part) * 1.8;
                output[1][ii] = (right_part + center_part) * 1.8;
                output[2][ii] = (left_part + center_part) * 1.8;
                output[3][ii] = (right_part + center_part) * 1.8;
            }
        }

        for (int ch = 0; ch < OutChannelNum; ch++)
        {
            CMusicLimiter_ProcessBufferHardLimit(output[ch], numsamples, limitThreshold, limiter[ch]);
        }

    }

    void Mi3DAudioMappingSpk::process(float * output, const void * input, int bytenums)
    {
        int samplesNum = bytenums / 4 / channelOutNum;
        for (int ch = 0; ch < channelInNum; ch++)
            BuffChannelIn[ch] = (float *)calloc(samplesNum, sizeof(float));
        for (int ch = 0; ch < channelOutNum; ch++)
            BuffChannelOut[ch] = (float *)calloc(samplesNum, sizeof(float));

        float * inputptr = (float *)input;
        for (int ch = 0; ch < channelInNum; ch++)
        {
            for (int ii = 0; ii < samplesNum; ii++)
            {
                BuffChannelIn[ch][ii] = inputptr[ii*channelInNum + ch];
            }
        }

        process(BuffChannelOut, BuffChannelIn, samplesNum);

        for (int ch = 0; ch < channelOutNum; ch++)
        {
            for (int ii = 0; ii < samplesNum; ii++)
            {
                output[ii*channelOutNum + ch] = BuffChannelOut[ch][ii];
            }
        }

        for (int ch = 0; ch < InChannelNum; ch++)
            free(BuffChannelIn[ch]);
        for (int ch = 0; ch < OutChannelNum; ch++)
            free(BuffChannelOut[ch]);
    }
}
