/**
 * This product contains one or more programs protected under international
 * and U.S. copyright laws as unpublished works.  They are confidential and
 * proprietary to Dolby Laboratories.  Their reproduction or disclosure, in
 * whole or in part, or the production of derivative works therefrom without
 * the express permission of Dolby Laboratories is prohibited.
 * Copyright 2011 - 2019 by Dolby Laboratories.
 * All rights reserved.
 */

#ifndef _IDOLBYVISION_DMESSAGEUSAGE_H
#define _IDOLBYVISION_DMESSAGEUSAGE_H

/*
 * Description of key-value pairs that are understood by SWIDK.
 *
 * For OPENGL
 * -----------------------------------------------------------------------------
 *  string                type      actual type                 Additional Info
 * -----------------------------------------------------------------------------
 *  config-buffer         pointer     char*
 *  config-mode           int32       int
 *  protected-mode        int32       int
 *  profile               int32       int                    Dolby vision profile
 *  timestamp-us          int64       int64_t
 *  input-texId           int32       GLuint
 *  transform-matrix      pointer     float*
 *  width                 int32       int
 *  height                int32       int
 *  is-sdr                int32       int
 *  Tmax                  float       float
 *  Tmin                  float       float
 *  Tgamma                float       float
 *  DBrightness           float       float
 *  DSaturation           float       float
 *  DContrast             float       float
 *  EnLowPowerTrims       int32       int
 *  LpSlope               float       float
 *  LpOffset              float       float
 *  LpPower               float       float
 *  LpChromaWeight        float       float
 *  LpSaturationGain      float       float
 *  LpTMidContrast        float       float
 *  LpClipTrim            float       float
 *
 * ----------------------------------------------------------------------------------------------------------------------------------------------------
 *  IFunctions            string                  required      Accepted values                                   Additional Info
 * ----------------------------------------------------------------------------------------------------------------------------------------------------
 *  Instantiate           is-sdr                  no
 *
 *  Init()                config-buffer           yes
 *                        config-mode             yes
 *                        profile                 no
 *                        protected-mode          no
 *  ProcessFrame()        input-texId             yes
 *                        transform-matrix        yes
 *                        timestamp-us            yes

 *. SetParam              -                       yes                                                      What() property determines param type.
 *                        -                       yes          IDMessage::DMSG_VIDEO_RESOLUTION            Needs width, height
 *                        -                       no           IDMessage::DMSG_LOW_POWER_MODE              No additional params needed
 *                                                no           IDMessage::DMSG_CONFIG_PARAMS               Used with Tmax, Tmin, Tgamma, DBrightness, DSaturation,
 *                                                                                                         DContrast, EnLowPowerTrims, LpSlope, LpOffset, LpPower,
 *                                                                                                         LpChromaWeight, LpSaturationGain, LpTMidContrast, LpClipTrim
 *
 *
 *
 *
 *
 *
 * For Metal
 * -----------------------------------------------------------------------------------------------------------
 *  key string                    value type      actual type     Additional Info
 * -----------------------------------------------------------------------------------------------------------
 *  timestamp-us                    int64           int64_t
 *  transform-matrix                pointer         float*
 *  width                           int32           int
 *  height                          int32           int
 *  config-buffer                   pointer         char*
 *  config-mode                     int32           int
 *  command-queue                   pointer         void*          Pointer to MTLCommandQueue
 *  pixel-format                    Int64           int64_t        Render target pixel format (MTLPixelFormat)
 *  mtl-texture-y                   pointer         void*          Pointer to Y metal texture
 *  mtl-texture-uv                  pointer         void*          Pointer to UV metal texture
 *  view-port-val                   pointer         double[4]      originX, originY, width, height
 *  render-pass-descriptor-val      pointer         void*          Pointer to MTLRenderPassDescriptor
 *
 *
 * ---------------------------------------------------------------------------------------------------------------------------------------------------------
 *   Functions            string                        required      Accepted values                           Additional Info
 * ---------------------------------------------------------------------------------------------------------------------------------------------------------
 *  Init()                command-queue                 yes
 *                        pixel-format                  yes
 *                        config-buffer                 yes
 *                        config-mode                   yes
 *  ProcessFrame()        mtl-texture-y                 yes
 *                        mtl-texture-uv                yes
 *                        timestamp-us                  yes
 *                        transform-matrix              no
 *  SetParam                                            -                                                         What() property determines param type.
 *                                                      yes           IDMessage::DMSG_VIDEO_RESOLUTION            Needs width, height
 *                                                                    IDMessage::DMSG_VIEW_PORT                   Needs view-port-val
 *                                                                    IDMessage::DMSG_RENDER_PASS_DESCRIPTOR      Needs render-pass-descriptor-val
 *
 *
 *
 *
 */


#endif /* end of include guard: _IDOLBYVISION_DMESSAGEUSAGE_H */
