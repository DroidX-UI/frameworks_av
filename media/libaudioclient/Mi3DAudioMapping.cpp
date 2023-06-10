#include "Mi3DAudioMapping.h"

namespace android {

Mi3DAudioMapping::Mi3DAudioMapping(int fs_, int in_channel_num, int bit_width)
{
    deviceType = 0;
    mi3DAudioSpkObj = new Mi3DAudioMappingSpk(fs_, in_channel_num, bit_width);
    mi3DAudioHeadphoneObj = new Mi3DAudioMappingHeadphone(fs_, in_channel_num, bit_width);
}

Mi3DAudioMapping::Mi3DAudioMapping()
{
    deviceType = 0;
    mi3DAudioSpkObj = new Mi3DAudioMappingSpk();
    mi3DAudioHeadphoneObj = new Mi3DAudioMappingHeadphone();
}

Mi3DAudioMapping::~Mi3DAudioMapping()
{
    delete mi3DAudioSpkObj;
    delete mi3DAudioHeadphoneObj;
}

void Mi3DAudioMapping::process(void * output, const void * input, int bytenums, int bit_width)
{

    switch (deviceType)
    {
    case 0:
        mi3DAudioSpkObj->process(output, input, bytenums, bit_width);
        break;
    case 1:
        mi3DAudioHeadphoneObj->process(output, input, bytenums, bit_width);
        break;
    }
}

void Mi3DAudioMapping::process(float * output, const void * input, int bytenums)
{

    switch (deviceType)
    {
    case 0:
        mi3DAudioSpkObj->process(output, input, bytenums);
        break;
    case 1:
        mi3DAudioHeadphoneObj->process(output, input, bytenums);
        break;
    }
}

void Mi3DAudioMapping::setMode(int modeValue)
{
    if (deviceType)
    {
        mi3DAudioHeadphoneObj->setEnable(modeValue);
    }
    else
    {
        mi3DAudioSpkObj->setMode(modeValue);
    }
}

void Mi3DAudioMapping::setDeviceType(int type_)
{
    deviceType = type_;
}

}