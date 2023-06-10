/**
 * This product contains one or more programs protected under international
 * and U.S. copyright laws as unpublished works.  They are confidential and
 * proprietary to Dolby Laboratories.  Their reproduction or disclosure, in
 * whole or in part, or the production of derivative works therefrom without
 * the express permission of Dolby Laboratories is prohibited.
 * Copyright 2011 - 2019 by Dolby Laboratories.
 * All rights reserved.
 */

#ifndef LIBS_DATA_PATH_INC_OTT_IDMESSAGE_H
#define LIBS_DATA_PATH_INC_OTT_IDMESSAGE_H

#include "DvControlExport.h"

namespace dovi
{

class DOVI_OTT_EXPORT IDMessage
{
public:
    static const unsigned int dmessage_major = 0;
    static const unsigned int dmessage_minor = 1;

    enum {
        MSG_TYPE_COMPOSER = 1,
        MSG_TYPE_DM = 2,
    };
    enum DMSG_TYPE{
        //Common
        DMSG_WHAT_INVALID = -1,
        DMSG_VIDEO_RESOLUTION = 1,
        DMSG_MAX_BRIGHTNESS = 41,
        DMSG_AMBIENT_LUX = 42,
        DMSG_RESET_TRACKED_PARAMS = 43,
        DMSG_PTS_TO_MILLISECONDS_FACTOR = 44,

        //OpenGL specific
        DMSG_EGL_PARAMS = 2,
        DMSG_LOW_POWER_MODE = 3,
        DMSG_METADATA = 4,
        DMSG_CONFIG_PARAMS = 5,
        DMSG_EGL_STATE = 40,

        //Debug Specific
        DMSG_DEBUG=300,

        //Metal specific
        DMSG_VIEW_PORT=100,
        DMSG_RENDER_PASS_DESCRIPTOR=101,

        DMSG_WHAT_MAX = 0xFFFFFFFF
    };

    IDMessage() = default;
    virtual ~IDMessage() = default;

    /* Get the value identified with key */
    virtual bool GetInt32(const char *key, int *val) = 0;
    virtual bool GetInt64(const char *key, long long *val) = 0;
    virtual bool GetPointer(const char *key, void **val) = 0;
    virtual bool GetFloat(const char *key, float *val) = 0;

    /* Set the value identified with key */
    virtual bool SetInt32(const char *key, int val) = 0;
    virtual bool SetInt64(const char *key, long long val) = 0;
    virtual bool SetPointer(const char *key, void *val) = 0;
    virtual bool SetFloat(const char *key, float val) = 0;

    /* Clears all entries in DMessage */
    virtual void clear() = 0;
    //Set Message Type
    virtual bool SetWhat(int what) = 0;
    //Get Message Type
    virtual int What() = 0;

    /* Print contents in DMessage */
    virtual void PrintMessage() = 0;
};

/*
 * Create and return a IDMessage instance
 */
  DOVI_OTT_EXPORT IDMessage *CreateDMessage(unsigned int version_major = IDMessage::dmessage_major,
                          unsigned int version_minor = IDMessage::dmessage_minor);

}; /* namespace dovi */

#endif /* end of include guard: LIBS_DATA_PATH_INC_OTT_IDMESSAGE_H */
