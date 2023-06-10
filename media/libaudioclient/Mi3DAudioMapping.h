#ifndef MI3DAUDIOMAPPING
#define MI3DAUDIOMAPPING

#include "Mi3DAudioMappingHeadphone.h"
#include "Mi3DAudioMappingSpk.h"

namespace android{

class Mi3DAudioMapping
{
public:
    Mi3DAudioMapping(int fs_, int in_channel_num, int bit_width);
    Mi3DAudioMapping();
    void process(void * output, const void * input, int bytenums, int bit_width);
    void process(float * output, const void * input, int bytenums);
    void setMode(int modeValue);
    void setDeviceType(int type_);
    ~Mi3DAudioMapping();

private:
    int deviceType;   //0 spk, 1 headphone
    Mi3DAudioMappingSpk * mi3DAudioSpkObj;
    Mi3DAudioMappingHeadphone * mi3DAudioHeadphoneObj;
};
}


#endif
