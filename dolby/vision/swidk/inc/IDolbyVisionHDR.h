/****************************************************************************
 * This program is protected under international and U.S. copyright laws as
 * an unpublished work. This program is confidential and proprietary to the
 * copyright owners. Reproduction or disclosure, in whole or in part, or the
 * production of derivative works therefrom without the express permission of
 * the copyright owners is prohibited.
 *
 * Copyright (C) 2011 - 2019 by Dolby Laboratories,
 * All rights reserved.
 ******************************************************************************/
#ifndef _IDOLBYVISIONHDR_H
#define _IDOLBYVISIONHDR_H

#include <stdint.h>
#include <stdlib.h>
#include "IDMessage.h"

class DOVI_OTT_EXPORT IDolbyVisionHDR
{

public:
    virtual ~IDolbyVisionHDR() { }

    /*
     *  API to parse RPU from incoming stream
     *  API will parse and copy the RPU NAL. it does not change input stream buffer
     *  @param[in] au_buf   single access unit from incoming stream.
     *  @param[in] size     size of au_buf
     *  @param[in] timeUs   timestamp for the access unit
     */
    virtual bool ParseRPU(uint8_t *au_buf, size_t size, int64_t timeUs) = 0;

    /*
     * API to invoke Dolby Processing on the decoded frame. Dolby Vision
     * processing is done in GPU using fragment shader and compute shader.
     *  @param[in] *message   Pointer to DMessage. See IDMessageUsage.h for details
     *  @return
     *   true
     *   false
     */
    virtual bool ProcessFrame(dovi::IDMessage *message) = 0;

    /*
     * API to configure the instance with a specific PQ Mode.
     *  @param[in] configBuffer Buffer containing tuning params
     *  @param[in] config_id    The config id(picture mode id) to choose for this instance
     *  @return
     *   true
     *   false
     */
    virtual bool SetPQMode(const char *configBuffer, int config_id) = 0;

    /*
     * API to set a param
     *  @param[in] *message   Pointer to DMessage. See IDMessageUsage.h for details
     */
    virtual bool SetParam(dovi::IDMessage *message) = 0;

    /*
     * signal flush event during seeking
     *  @return
     *   true
     *   false
     */
    virtual bool SignalFlush() = 0;

    /* This function should be called before start of playback.
     *  @param[in] *message     Pointer to DMessage. See IDMessageUsage.h for details
     *
     *  @return
     *   true
     *   false
     */
    virtual bool Init(dovi::IDMessage *message) = 0;

    /*
     *  signal stop event from player to indicate the very first stop of
     *  the processing
     */
    virtual void Teardown() = 0;

    /*
     * This may be blocking, Can be use for setting states within the library
     */
    virtual bool SetState(dovi::IDMessage *state) = 0;

    /*
     *  instantiate a DolbyHDR instance
     *  key     - Authorization key for dolby vision library
     *. message - Pointer to DMessage.  See IDMessageUsage.h for details
     */
    static IDolbyVisionHDR *Instantiate(const char *key, dovi::IDMessage *message = nullptr);

    /*
     *  Get the version of the software
     */
    static const char *Version();

protected:
    IDolbyVisionHDR() { }
};

#endif //_IDOLBYVISIONHDR_H
