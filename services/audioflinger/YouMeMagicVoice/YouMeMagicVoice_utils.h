#ifndef YouMeMagicVoice_utils_hpp
#define YouMeMagicVoice_utils_hpp
#include <stdlib.h>
#include "YouMeMagicVoiceConstDefine.h"

int YouMeMagicVoice_init();
void YouMeMagicVoice_deinit();

void* engine_createMagicVoiceEngine();
void  engine_releaseMagicVoiceEngine(void* engine);
int   engine_setMagicVoiceInfo(const char *effectInfo);
int   engine_clearMagicVoiceInfo() ;
int   engine_setMagicVoiceAdjust(double dFS, double dSemitones);

#endif
