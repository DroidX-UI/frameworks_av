#include "Mi3DAudioMappingHeadphone.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "HrtfData.h"
#include <stdint.h>

namespace android {

Mi3DAudioMappingHeadphone::Mi3DAudioMappingHeadphone()
    :enable(1),
    norm(NORM_32),
    denorm(DENORM_32),
    channelInNum(HeadPhoneInChannelNum),
    channelOutNum(HeadPhoneOutChannelNum),
    limitThreshold(0.99)
{
    float sample_rate = 48000;
    float attackRatio = 1.f - expf(-2.2 / (0.0002f * sample_rate));
    float releaseRatio = 1.f - expf(-2.2 / (0.4f * sample_rate));
    for (int ii = 0; ii < channelOutNum; ii++)
    {
        limiter[ii] = CMusicLimiter_Init(attackRatio, releaseRatio, 0.005 * sample_rate + 1, 0.1, sample_rate);
    }
    limitThreshold = 0.98;

    hrtfCoeff[0][0] = hrtf_coeff_l[0];
    hrtfCoeff[1][0] = hrtf_coeff_l[1];
    hrtfCoeff[0][1] = hrtf_coeff_r[0];
    hrtfCoeff[1][1] = hrtf_coeff_r[1];
    hrtfCoeff[0][2] = hrtf_coeff_c[0];
    hrtfCoeff[1][2] = hrtf_coeff_c[1];
    hrtfCoeff[0][3] = hrtf_coeff_lfe[0];
    hrtfCoeff[1][3] = hrtf_coeff_lfe[1];
    hrtfCoeff[0][4] = hrtf_coeff_ls[0];
    hrtfCoeff[1][4] = hrtf_coeff_ls[1];
    hrtfCoeff[0][5] = hrtf_coeff_rs[0];
    hrtfCoeff[1][5] = hrtf_coeff_rs[1];

    for (int ch = 0; ch < 2; ch++)
        for (int ii = 0; ii < HeadPhoneInChannelNum; ii++)
        {
            fastconvObj[ch][ii] = new FastConv(hrtfCoeff[ch][ii], HRTFLEN);
        }

    eq[0] = new FastConv(h_eq_l, EQFIRLEN);
    eq[1] = new FastConv(h_eq_r, EQFIRLEN);

    for(int i = 0; i < HeadPhoneInChannelNum; i++){
        BuffChannelInInt[i] = NULL;
        BuffChannelIn[i] = NULL;
    }

    for(int i = 0; i < HeadPhoneWrapperOutChannelNum; i++){
        BuffChannelOutInt[i] = NULL;
        BuffChannelOut[i] = NULL;
    }
}

Mi3DAudioMappingHeadphone::Mi3DAudioMappingHeadphone(int fs_, int in_channel_num, int bit_width)
{
    enable = 1;
    if (in_channel_num == HeadPhoneInChannelNum)
    {
        channelInNum = HeadPhoneInChannelNum;
        enable = 1;
    }
    else
        enable = 0;

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

    channelOutNum = HeadPhoneOutChannelNum;

    float sample_rate = fs_;
    float attackRatio = 1.f - expf(-2.2 / (0.0002f * sample_rate));
    float releaseRatio = 1.f - expf(-2.2 / (0.4f * sample_rate));
    for (int ii = 0; ii < channelOutNum; ii++)
    {
        limiter[ii] = CMusicLimiter_Init(attackRatio, releaseRatio, 0.005 * sample_rate + 1, 0.1, sample_rate);
    }
    limitThreshold = 0.98;

    hrtfCoeff[0][0] = hrtf_coeff_l[0];
    hrtfCoeff[1][0] = hrtf_coeff_l[1];
    hrtfCoeff[0][1] = hrtf_coeff_r[0];
    hrtfCoeff[1][1] = hrtf_coeff_r[1];
    hrtfCoeff[0][2] = hrtf_coeff_c[0];
    hrtfCoeff[1][2] = hrtf_coeff_c[1];
    hrtfCoeff[0][3] = hrtf_coeff_lfe[0];
    hrtfCoeff[1][3] = hrtf_coeff_lfe[1];
    hrtfCoeff[0][4] = hrtf_coeff_ls[0];
    hrtfCoeff[1][4] = hrtf_coeff_ls[1];
    hrtfCoeff[0][5] = hrtf_coeff_rs[0];
    hrtfCoeff[1][5] = hrtf_coeff_rs[1];

    for (int ch = 0; ch < 2; ch++)
        for (int ii = 0; ii < HeadPhoneInChannelNum; ii++)
        {
            fastconvObj[ch][ii] = new FastConv(hrtfCoeff[ch][ii], HRTFLEN);
        }

    eq[0] = new FastConv(h_eq_l, EQFIRLEN);
    eq[1] = new FastConv(h_eq_r, EQFIRLEN);

    for(int i = 0; i < HeadPhoneInChannelNum; i++){
        BuffChannelInInt[i] = NULL;
        BuffChannelIn[i] = NULL;
    }

    for(int i = 0; i < HeadPhoneWrapperOutChannelNum; i++){
        BuffChannelOutInt[i] = NULL;
        BuffChannelOut[i] = NULL;
    }
}

Mi3DAudioMappingHeadphone::~Mi3DAudioMappingHeadphone()
{
    for (int ch = 0; ch < 2; ch++)
        for (int ii = 0; ii < HeadPhoneInChannelNum; ii++)
            delete fastconvObj[ch][ii];

    for (int ii = 0; ii < channelOutNum; ii++)
        CMusicLimiter_Delete(limiter[ii]);

    delete eq[0];
    delete eq[1];
}

void Mi3DAudioMappingHeadphone::setEnable(int enableValue)
{
    enable = enableValue;
}

void Mi3DAudioMappingHeadphone::process(int * output[], int * input[], int numSamples)
{
    for (int ch = 0; ch < channelInNum; ch++)
    {
        BuffChannelIn[ch] = (float *)malloc(numSamples * sizeof(float));
        for (int ii = 0; ii < numSamples; ii++)
            BuffChannelIn[ch][ii] = input[ch][ii] * norm;
    }

    for (int ch = 0; ch < channelOutNum; ch++)
        BuffChannelOut[ch] = (float *)calloc(numSamples, sizeof(float));

    float * tmpBuff = (float *)calloc(numSamples, sizeof(float));

    if (enable)
    {
        for (int cho = 0; cho < channelOutNum; cho++)
        {
            for (int chi = 0; chi < channelInNum; chi++)
            {
                memcpy(tmpBuff, BuffChannelIn[chi], numSamples * sizeof(float));
                fastconvObj[cho][chi]->process(tmpBuff, numSamples);
                for (int ii = 0; ii < numSamples; ii++)
                {
                    if(chi == 4)
                        BuffChannelOut[cho][ii] += 2*tmpBuff[ii];
                    else
                        BuffChannelOut[cho][ii] += tmpBuff[ii];
                }
            }
            memcpy(tmpBuff, BuffChannelOut[cho], numSamples * sizeof(float));
            eq[cho]->process(tmpBuff, numSamples);
            memcpy(BuffChannelOut[cho], tmpBuff, numSamples * sizeof(float));
        }

        for (int ii = 0; ii < numSamples; ii++)
            BuffChannelOut[0][ii] *= 2.0;
        for (int ii = 0; ii < numSamples; ii++)
            BuffChannelOut[1][ii] *= 2.8;
    }
    else
    {
        for (int ii = 0; ii < numSamples; ii++)
        {
            float centerPart = (BuffChannelIn[2][ii] + BuffChannelIn[3][ii])* 0.707;
            BuffChannelOut[0][ii] = (BuffChannelIn[0][ii] + BuffChannelIn[4][ii] + centerPart) * 0.6;
            BuffChannelOut[1][ii] = (BuffChannelIn[1][ii] + BuffChannelIn[5][ii] + centerPart) * 0.6;
        }
    }

    for (int ch = 0; ch < channelOutNum; ch++)
    {
        CMusicLimiter_ProcessBufferHardLimit(BuffChannelOut[ch], numSamples, limitThreshold, limiter[ch]);
        for (int ii = 0; ii < numSamples; ii++)
            output[ch][ii] = int(BuffChannelOut[ch][ii] * denorm);
    }

    // keep 2,3ch same with 0,1ch for 4 channel output
    memcpy(output[2], output[0], numSamples * sizeof(int));
    memcpy(output[3], output[1], numSamples * sizeof(int));

    for (int ch = 0; ch < channelInNum; ch++)
        free(BuffChannelIn[ch]);
    for (int ch = 0; ch < channelOutNum; ch++)
        free(BuffChannelOut[ch]);
    free(tmpBuff);

}

void Mi3DAudioMappingHeadphone::process(float * output[], float * input[], int numSamples)
{
    float * tmpBuff = (float *)calloc(numSamples, sizeof(float));

    if (enable)
    {
        for (int cho = 0; cho < channelOutNum; cho++)
        {
            for (int chi = 0; chi < channelInNum; chi++)
            {
                memcpy(tmpBuff, input[chi], numSamples * sizeof(float));
                fastconvObj[cho][chi]->process(tmpBuff, numSamples);
                for (int ii = 0; ii < numSamples; ii++)
                {
                    if (chi == 4)
                        output[cho][ii] += 2 * tmpBuff[ii];
                    else
                        output[cho][ii] += tmpBuff[ii];
                }
            }
            memcpy(tmpBuff, output[cho], numSamples * sizeof(float));
            eq[cho]->process(tmpBuff, numSamples);
            memcpy(output[cho], tmpBuff, numSamples * sizeof(float));
        }

        for (int ii = 0; ii < numSamples; ii++)
            output[0][ii] *= 2.0;
        for (int ii = 0; ii < numSamples; ii++)
            output[1][ii] *= 2.8;
    }
    else
    {
        for (int ii = 0; ii < numSamples; ii++)
        {
            float centerPart = (input[2][ii] + input[3][ii])* 0.707;
            output[0][ii] = (input[0][ii] + input[4][ii] + centerPart) * 0.6;
            output[1][ii] = (input[1][ii] + input[5][ii] + centerPart) * 0.6;
        }
    }

    for (int ch = 0; ch < channelOutNum; ch++)
    {
        CMusicLimiter_ProcessBufferHardLimit(output[ch], numSamples, limitThreshold, limiter[ch]);
    }

    // keep 2,3ch same with 0,1ch for 4 channel output
    memcpy(output[2], output[0], numSamples * sizeof(float));
    memcpy(output[3], output[1], numSamples * sizeof(float));

    free(tmpBuff);

}


void Mi3DAudioMappingHeadphone::process(void * output, const void * input, int bytenums, int bytesPerSample)
{
    int samplesNum = bytenums / bytesPerSample / HeadPhoneWrapperOutChannelNum;
    for (int ch = 0; ch < channelInNum; ch++)
        BuffChannelInInt[ch] = (int *)calloc(samplesNum, sizeof(int));
    for (int ch = 0; ch < HeadPhoneWrapperOutChannelNum; ch++)
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
                input_int_ptr[4 * ii + 1] = input_ptr[(ii*channelInNum + ch) * 3 + 0];
                input_int_ptr[4 * ii + 2] = input_ptr[(ii*channelInNum + ch) * 3 + 1];
                input_int_ptr[4 * ii + 3] = input_ptr[(ii*channelInNum + ch) * 3 + 2];
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
        for (int ch = 0; ch < HeadPhoneWrapperOutChannelNum; ch++)
        {
            for (int ii = 0; ii < samplesNum; ii++)
            {
                output_ptr[ii*HeadPhoneWrapperOutChannelNum + ch] = BuffChannelOutInt[ch][ii] >> 16;
            }
        }
        break;
    }
    case 3:
    {
        for (int ch = 0; ch < HeadPhoneWrapperOutChannelNum; ch++)
        {
            uint8_t * output_ptr = (uint8_t *)output;
            uint8_t * output_int_ptr = (uint8_t *)BuffChannelOutInt[ch];
            for (int ii = 0; ii < samplesNum; ii++)
            {
#ifndef BIG_ENDIAN
                output_ptr[(ii*HeadPhoneWrapperOutChannelNum + ch) * 3 + 0] = output_int_ptr[4 * ii + 1];
                output_ptr[(ii*HeadPhoneWrapperOutChannelNum + ch) * 3 + 1] = output_int_ptr[4 * ii + 2];
                output_ptr[(ii*HeadPhoneWrapperOutChannelNum + ch) * 3 + 2] = output_int_ptr[4 * ii + 3];
#else
                output_ptr[(ii*HeadPhoneWrapperOutChannelNum + ch) * 3 + 0] = output_int_ptr[4 * ii + 3];
                output_ptr[(ii*HeadPhoneWrapperOutChannelNum + ch) * 3 + 1] = output_int_ptr[4 * ii + 2];
                output_ptr[(ii*HeadPhoneWrapperOutChannelNum + ch) * 3 + 2] = output_int_ptr[4 * ii + 1];
#endif
            }
        }
        break;
    }
    case 4:
    {
        int32_t * output_ptr = (int32_t *)output;
        for (int ch = 0; ch < HeadPhoneWrapperOutChannelNum; ch++)
        {
            for (int ii = 0; ii < samplesNum; ii++)
            {
                output_ptr[ii*HeadPhoneWrapperOutChannelNum + ch] = BuffChannelOutInt[ch][ii];
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

void Mi3DAudioMappingHeadphone::process(float * output, const void * input, int bytenums)
{
    int samplesNum = bytenums / 4 / HeadPhoneWrapperOutChannelNum;
    for (int ch = 0; ch < channelInNum; ch++)
        BuffChannelIn[ch] = (float *)malloc(samplesNum * sizeof(float));

    for (int ch = 0; ch < HeadPhoneWrapperOutChannelNum; ch++)
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

    for (int ch = 0; ch < HeadPhoneWrapperOutChannelNum; ch++)
    {
        for (int ii = 0; ii < samplesNum; ii++)
        {
            output[ii*HeadPhoneWrapperOutChannelNum + ch] = BuffChannelOut[ch][ii];
        }
    }

    for (int ch = 0; ch < channelInNum; ch++)
        free(BuffChannelIn[ch]);
    for (int ch = 0; ch < HeadPhoneWrapperOutChannelNum; ch++)
        free(BuffChannelOut[ch]);
}

}