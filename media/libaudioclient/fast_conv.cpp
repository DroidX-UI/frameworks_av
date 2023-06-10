

#include "fast_conv.h"
#include <stdlib.h>
#include <string.h>


FastConv::FastConv(float *h, int h_len)
{
    log2n = 0;
    int h_len_check = h_len;
    while(h_len_check > 0)
    {
        log2n ++;
        h_len_check >>= 1;
    }
    nfft = 1 << log2n;

    block_len = nfft / 2;

    float * h_nfft = (float *)calloc(nfft, sizeof(float));
    memmove(h_nfft, h, h_len * sizeof(float));

    H = (complex_float *)calloc(nfft, sizeof(complex_float));
    rdftcal(nfft, h_nfft, H);

    rbuff = (float *)malloc(nfft * sizeof(float));
    memset(rbuff, 0, nfft * sizeof(float));
    cbuff = (complex_float *)calloc(nfft, sizeof(complex_float));

    zi = (float *)calloc(block_len, sizeof(float));

    free(h_nfft);
}

void FastConv::blockprocess(float * input, int samples)
{
    float real, imag = 0;
    memset(rbuff, 0, nfft * sizeof(float));
    memcpy(rbuff, zi, block_len * sizeof(float));
    memcpy(rbuff + block_len, input, samples*sizeof(float));

    memcpy(zi, zi+samples, (block_len - samples) * sizeof(float));
    memcpy(zi + block_len - samples, input, samples * sizeof(float));

    rdftcal(nfft, rbuff, cbuff);

    for (int ii = 0; ii < nfft; ii++)
    {
        real = cbuff[ii].real;
        imag = cbuff[ii].imag;
        cbuff[ii].real = real * H[ii].real - imag * H[ii].imag;
        cbuff[ii].imag = imag * H[ii].real + real * H[ii].imag;
    }

    irdftcal(nfft, cbuff, rbuff);

    memcpy(input, rbuff + block_len, samples * sizeof(float));
}

void FastConv::process(float * input, int samples)
{
    int ii = 0;
    int block_num = samples / block_len + 1;
    for(ii = 0; ii < block_num - 1; ii++)
    {
        blockprocess(input + ii * block_len, block_len);
    }
    blockprocess(input + ii * block_len, samples - ii*block_len);
}

FastConv::~FastConv()
{

    if(H != NULL)
    {
        free(H);
        H = NULL;
    }
    if (rbuff != NULL)
    {
        free(rbuff);
        rbuff = NULL;
    }
    if(cbuff != NULL)
    {
        free(cbuff);
        cbuff = NULL;
    }
    if(zi != NULL)
    {
        free(zi);
        zi = NULL;
    }
}

