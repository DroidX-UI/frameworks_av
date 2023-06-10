/*
 * fftsg.h
 *
 *  Created on: 2020Äê12ÔÂ6ÈÕ
 *      Author: dingge
 */

#ifndef SRC_FFTSG_H_
#define SRC_FFTSG_H_

#include <math.h>

typedef struct complex_float
{
    float real;
    float imag;
}complex_float;

//#ifdef __cplusplus
//extern "C" {
//#endif

     void rdft(int n, int isgn, float *a, int *ip, float *w);
     void rdftcal(int nfft, float *input, complex_float *output);
     void irdftcal(int nfft, complex_float *input, float *output);

//#ifdef __cplusplus
//}
//#endif

#endif /* SRC_FFTSG_H_ */
