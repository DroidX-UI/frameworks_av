/*******************************************************************
 *
 * Name        : CMusicLimiter.h
 * Author      : Xie Shaoduo
 * Version     : 1.0.0
 * Date        : 2017-07-12
 * Description : A hard limiter module. Introducing block_length delay.
 *
 ********************************************************************/

#ifndef __C_CMUSICLIMITER_H__
#define __C_CMUSICLIMITER_H__

#define CMusic_Sample_Type float

typedef struct CMusicLimiter_State_t {
    int delay_index;
    int delay_length;
    CMusic_Sample_Type *delay_line;
    CMusic_Sample_Type attack_coeff;
    CMusic_Sample_Type release_coeff;
    CMusic_Sample_Type xpeak;
    CMusic_Sample_Type g;
    //hard limiter params
    int diff_index;
    CMusic_Sample_Type *diff_number;
    int lastDiffNumberIndex;
    int numSamplesRelease;
    CMusic_Sample_Type releaseTime;

} CMusicLimiter_State;

#ifdef __cplusplus
extern "C" {
#endif

    extern CMusicLimiter_State* CMusicLimiter_Init(CMusic_Sample_Type attack_factor, CMusic_Sample_Type release_factor, int delay_len, CMusic_Sample_Type releaseTime, CMusic_Sample_Type samplerate);
    extern void CMusicLimiter_Delete(CMusicLimiter_State *state);
    extern void CMusicLimiter_Reset(CMusicLimiter_State *state);

    //buffer version
    extern void CMusicLimiter_ProcessBuffer(CMusic_Sample_Type *signal, int block_length, CMusic_Sample_Type threshold, CMusicLimiter_State *state);

    //buffer hard limit
    extern void CMusicLimiter_ProcessBufferHardLimit(CMusic_Sample_Type *signal, int block_length, CMusic_Sample_Type threshold, CMusicLimiter_State *state);

    //sample by sample version
    extern CMusic_Sample_Type CMusicLimiter_ProcessSample(CMusic_Sample_Type x, CMusic_Sample_Type threshold, CMusicLimiter_State *state);

#ifdef __cplusplus
}
#endif

#endif
