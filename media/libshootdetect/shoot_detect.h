/*******************************************************************
*
* Name        : shoot_detect.h
* Author      : Runyu Shi
* Version     : 1.0.0
* Date        : 2018-8-15
* Description : Header file to apply an
* 				 shoot_detect processing
* 				 feature to the input signal
*
* Update list : 1.0.0: first on-board runnable version.
********************************************************************/

#ifndef __shoot_detect_H__
#define __shoot_detect_H__

// Include files
#include <stdint.h>

#define SD_SUCCSESS          0
#define SD_ERR_BAD_INPARAM  -1
#define SD_ERR_BAD_INDATA   -2
#define SD_ERR_DATA_OVER_SIZE  -3

typedef int16_t DTYPE;
#define DATA_MAX_VAL 32768

#ifndef MAX_NCH
#define MAX_NCH 2
#endif

typedef enum {
GAME_TP_DEFAULT = 0,
GAME_TP_PUBGMHD = GAME_TP_DEFAULT, // 刺激战场
GAME_TP_HYXD,                      // 荒野行动
GAME_TP_PUBGM,                     // 全军突击
GAME_TP_MAX = GAME_TP_PUBGM,
} SHOOT_DETECT_GAME_TYPE;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/* Public func. */
uint32_t get_shoot_detect_str_size(void);
int shoot_detect_init(void* shoot_detect_st, uint32_t num_channels, uint32_t sampling_rate, unsigned int game_type);

int shoot_detect_process(void* shoot_detect_st, DTYPE* in_buf, uint32_t num_samples);
int shoot_detect_release(void* shoot_detect_st);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* __shoot_detect_H__ */

