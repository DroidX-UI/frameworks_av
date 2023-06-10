#ifndef __THREAD__
#define __THREAD__

#include <log/log.h>
#include <utils/Errors.h>

namespace android {
    String8 magicVoiceInfo;
    float semitonesadjust = 0;
    float fsadjust = 0;
    bool magicVoiceState = false;
  typedef enum {
    NONEEFFECT = 0,
    MEN,
    LADY,
    LOLI,
    CARTOON,
    ROBOT,
} voice_mode;

};
#endif

