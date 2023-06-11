/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2019 by Dolby Laboratories,
 *                             All rights reserved.
 ******************************************************************************/

#ifndef DOLBY_VISION_DOVI_INC_PIXELPROCESSING_H
#define DOLBY_VISION_DOVI_INC_PIXELPROCESSING_H

#include <string>

#include <gui/IGraphicBufferProducer.h>
#include <gui/BufferQueue.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/BufferItem.h>
#include <gui/GLConsumer.h>

#include <utils/threads.h>
#include <utils/Vector.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <media/hardware/MetadataBufferType.h>
#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>

#define DOVI_PROCESSING_MAJOR_VERSION (5)
#define DOVI_PROCESSING_MINOR_VERSION (3)
#define DOVI_PROCESSING_PATCH_VERSION (3)
#define DOVI_PROCESSING_BUILD_VERSION (0)

namespace android {

class PixelProcessing : public ConsumerBase::FrameAvailableListener {

public:
  /* This is an internal API for setting up surface format and other details about the window */
    virtual status_t setNativeWindowSizeFormatAndUsage(
            ANativeWindow *nativeWindow /* nonnull */,
            int width, int height, int format, int rotation, int usage, bool reconnect) = 0;

    /* this is an internal function for message notifications to mediacodec for future use. */
    virtual void setNotificationMessage(const sp<AMessage> &msg) = 0;

    /*
     * updates an internal timestamp map.
     * mediaTimeUs is the timestamp for the incoming access unit
     * renderTimeNs is the timestamp for the corrosponding decoded frame
     */
    virtual status_t updateTsMap(int64_t mediaTimeUs, int64_t renderTimeNs) = 0;

    /*
     * API to parse RPU from incoming stream.
     * ptr - incoming stream
     * size - size of incoming sstream
     * timeUs - timestamp for the access unit
     */
    virtual bool parseRPU(uint8_t *ptr, size_t size, int64_t timeUs) = 0;

    /* internal function */
    virtual status_t handleMessage(const sp<AMessage> &msg) = 0;

    /* provides application surface to pixelprocessing for queuing post-processed frames. */
    virtual void setClientSurface (sp<AMessage> &msg) = 0;

    /* signal flush event from mediacodec */
    virtual status_t signalFlush() = 0;

    /* get special surface for media decoding from pixelprocessing. */
    virtual sp<Surface> getSurface() = 0;

    /* signal start event from mediacodec */
    virtual void start() = 0;

    /* signal stop event from mediacodec */
    virtual void stop() = 0;

    /* instantiate a pixelprocessing instance */
    static sp<PixelProcessing> instantiate(sp<AMessage> msg);

    /* get the version of the pixelprocessing */
    static std::string version();

protected:
    PixelProcessing() {}
    virtual ~PixelProcessing() {}

    /* Avoid copying and equating and default constructor */
    DISALLOW_EVIL_CONSTRUCTORS(PixelProcessing);
};

} /* namespace android */

#endif /* DOLBY_VISION_DOVI_INC_PIXELPROCESSING_H */
