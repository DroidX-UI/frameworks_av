/*******************************************************************
 *
 * Name        : CMusicLimiter.cpp
 * Author      : Xie Shaoduo
 * Version     : 1.0.0
 * Date        : 2017-07-12
 * Description : A hard limiter module. Introducing block_length delay.
 *
 ********************************************************************/

#include <math.h>
#include "CMusicLimiter.h"
#include <stdlib.h>
#include <memory.h>

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

CMusicLimiter_State* CMusicLimiter_Init(CMusic_Sample_Type attack_coeff, CMusic_Sample_Type release_coeff, int delay_len, CMusic_Sample_Type releaseTime, CMusic_Sample_Type samplerate) {

    CMusicLimiter_State *state = (CMusicLimiter_State*)malloc(sizeof(CMusicLimiter_State));;

    state->attack_coeff = attack_coeff;
    state->release_coeff = release_coeff;
    state->delay_index = 0;
    state->delay_length = delay_len;
    state->xpeak = 0;
    state->g = 1;
    state->delay_line = (CMusic_Sample_Type*)malloc(sizeof(CMusic_Sample_Type) * delay_len);
    memset(state->delay_line, 0, sizeof(CMusic_Sample_Type) * delay_len);
    //hard limiter params
    state->diff_index = 0;
    state->lastDiffNumberIndex = 0;
    state->diff_number = (CMusic_Sample_Type*)malloc(sizeof(CMusic_Sample_Type) * delay_len);
    int i;
    for (i = 0; i < delay_len; ++i) {
        state->diff_number[i] = 1.f;
    }
    state->numSamplesRelease = samplerate * releaseTime;
    state->releaseTime = releaseTime;
//    CMusicLimiter_Reset(state);

    return state;
}

void CMusicLimiter_Delete(CMusicLimiter_State *state)
{
    if (state != NULL) {
        if (state->delay_line != NULL) {
            free(state->delay_line);
            state->delay_line = NULL;
        }
        if (state->diff_number != NULL) {
            free(state->diff_number);
            state->diff_number = NULL;
        }
        free(state);
        state = NULL;
    }
}

void CMusicLimiter_Reset(CMusicLimiter_State *state)
{
    if (state == NULL)
        return;
    if (state->delay_line == NULL)
        return;
    if (state->diff_number == NULL)
        return;

    memset(state->delay_line, 0, sizeof(CMusic_Sample_Type) * state->delay_length);
    int i;
    for (i = 0; i < state->delay_length; ++i) {
        state->diff_number[i] = 1.f;
    }
}

void CMusicLimiter_ProcessBuffer(CMusic_Sample_Type *signal, int block_length, CMusic_Sample_Type threshold, CMusicLimiter_State *state)
{
    int i = 0;
    for (i = 0; i < block_length; i++) {
        CMusic_Sample_Type coeff = 0.f;

        CMusic_Sample_Type a = fabsf(signal[i]);

        if (a > state->xpeak)
            coeff = state->attack_coeff;
        else
            coeff = state->release_coeff;

        state->xpeak = (1 - coeff) * state->xpeak + coeff * a;
        CMusic_Sample_Type f = MIN(1, threshold / state->xpeak);

        if (f < state->g)
            coeff = state->attack_coeff;
        else
            coeff = state->release_coeff;

        state->g = (1 - coeff) * state->g + coeff * f;

        state->delay_line[state->delay_index] = signal[i];

        state->delay_index++;
        if (state->delay_index >= state->delay_length)
            state->delay_index = 0;

        signal[i] = state->g * state->delay_line[state->delay_index];
    }
}

CMusic_Sample_Type CMusicLimiter_ProcessSample(CMusic_Sample_Type x, CMusic_Sample_Type threshold, CMusicLimiter_State *state)
{
    CMusic_Sample_Type coeff = 0.f;

    CMusic_Sample_Type a = fabsf(x);

    if (a > state->xpeak)
        coeff = state->attack_coeff;
    else
        coeff = state->release_coeff;

    state->xpeak = (1 - coeff) * state->xpeak + coeff * a;
    CMusic_Sample_Type f = MIN(1, threshold / state->xpeak);

    if (f < state->g)
        coeff = state->attack_coeff;
    else
        coeff = state->release_coeff;

    state->g = (1 - coeff) * state->g + coeff * f;

    state->delay_line[state->delay_index] = x;

    state->delay_index++;
    if (state->delay_index >= state->delay_length)
        state->delay_index = 0;

    return state->g * state->delay_line[state->delay_index];
}

//======================================================
void CMusicLimiter_ProcessBufferHardLimit(CMusic_Sample_Type *signal, int block_length, CMusic_Sample_Type threshold, CMusicLimiter_State *state)
{
    int i = 0;
    for (i = 0; i < block_length; i++) {

        state->xpeak = fabsf(signal[i]);
        CMusic_Sample_Type f = MIN(1, threshold / state->xpeak);

        state->lastDiffNumberIndex = state->diff_index - 1;
        if (state->lastDiffNumberIndex < 0)
            state->lastDiffNumberIndex += state->delay_length;
        CMusic_Sample_Type last_written_g = state->diff_number[state->lastDiffNumberIndex];

        CMusic_Sample_Type increment = 0;
        int d;
        if (f < last_written_g) {
            increment = (f - state->g) / state->delay_length;
            for (d = 0; d < state->delay_length; ++d) {
                int diff_index_to_set = state->diff_index - d;
                if (diff_index_to_set < 0)
                    diff_index_to_set += state->delay_length;

                CMusic_Sample_Type newG = f - increment * d;
                if (diff_index_to_set == state->diff_index)
                    state->diff_number[diff_index_to_set] = newG;
                else {
                    if (newG < state->diff_number[diff_index_to_set])
                        state->diff_number[diff_index_to_set] = newG;
                }
            }
        }
        else if (f > last_written_g) {
            increment = (1 - last_written_g) / state->numSamplesRelease;
            state->diff_number[state->diff_index] = last_written_g + increment;
        }
        else {
            state->diff_number[state->diff_index] = last_written_g;
        }

        state->diff_index++;
        if (state->diff_index >= state->delay_length)
            state->diff_index = 0;
        state->g = state->diff_number[state->diff_index];

        state->delay_line[state->delay_index] = signal[i];

        state->delay_index++;
        if (state->delay_index >= state->delay_length)
            state->delay_index = 0;

        signal[i] = state->g * state->delay_line[state->delay_index];
    }
}
