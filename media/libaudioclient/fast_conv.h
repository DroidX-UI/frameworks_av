#ifndef MISOUND4_FASTCONV_H
#define MISOUND4_FASTCONV_H


#include "fftsg.h"

class FastConv {
public:
    FastConv(float * h, int h_len);
    ~FastConv();
    void process(float * input, int samples);
    void blockprocess(float * input, int samples);

private:
    int nfft;
    int log2n;
    int block_len;

    complex_float * H;
    float * rbuff;
    complex_float * cbuff;
    float * zi;

};
#endif

