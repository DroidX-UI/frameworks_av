#pragma once

/*
  Copyright (C) 2018 Nokia Corporation.
  This material, including documentation and any related
  computer programs, is protected by copyright controlled by
  Nokia Corporation. All rights are reserved. Copying,
  including reproducing, storing,  adapting or translating, any
  or all of this material requires the prior written consent of
  Nokia Corporation. This material also contains confidential
  information which may not be disclosed to others without the
  prior written consent of Nokia Corporation.
*/

/**
 * @file
 * @brief WAVE format definitions.
 */

#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
  uint16_t wFormatTag;      /* format type */
  uint16_t nChannels;       /* number of channels (i.e. mono, stereo...) */
  uint32_t nSamplesPerSec;  /* sample rate */
  uint32_t nAvgBytesPerSec; /* for buffer estimation */
  uint16_t nBlockAlign;     /* block size of data */

  /* WAVEFORMAT */
  uint16_t wBitsPerSample;  /* Number of bits per sample of mono data */
  uint16_t cbSize;          /* The count in bytes of the size of extra information (after cbSize) */

  /* WAVEFORMATEX */
  union
  {
    uint16_t wValidBitsPerSample;   /* bits of precision  */
    uint16_t wSamplesPerBlock;      /* valid if wBitsPerSample==0 */
    uint16_t wReserved;             /* If neither applies, set to zero. */

  } Samples;

  uint32_t dwChannelMask;       /* which channels are */

  /* present in stream  */
  /*Note: SubFormat GUID ignored */

} WAVEFORMATEXTENSIBLE;

typedef struct
{
    FILE *fp;
    int32_t type;
    WAVEFORMATEXTENSIBLE fmt;
    uint32_t data_start_pos;
    uint32_t sample_pos;
    uint32_t total_samples;

} wav_t;

enum wav_type
{
    WAV_UNK,
    WAV_ORIG,
    WAV_E,
    WAV_EXT
};

/*-- Speaker Positions: --*/
#define SPEAKER_FRONT_LEFT              0x1
#define SPEAKER_FRONT_RIGHT             0x2
#define SPEAKER_FRONT_CENTER            0x4
#define SPEAKER_LOW_FREQUENCY           0x8
#define SPEAKER_BACK_LEFT               0x10
#define SPEAKER_BACK_RIGHT              0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER    0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER   0x80
#define SPEAKER_BACK_CENTER             0x100
#define SPEAKER_SIDE_LEFT               0x200
#define SPEAKER_SIDE_RIGHT              0x400
#define SPEAKER_TOP_CENTER              0x800
#define SPEAKER_TOP_FRONT_LEFT          0x1000
#define SPEAKER_TOP_FRONT_CENTER        0x2000
#define SPEAKER_TOP_FRONT_RIGHT         0x4000
#define SPEAKER_TOP_BACK_LEFT           0x8000
#define SPEAKER_TOP_BACK_CENTER         0x10000
#define SPEAKER_TOP_BACK_RIGHT          0x20000

/*-- WAVE form wFormatTag IDs. ,,*/
#define  WAVE_FORMAT_UNKNOWN               0x0000  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_PCM                   0x0001  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_ADPCM                 0x0002  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_IEEE_FLOAT            0x0003  /*  Microsoft Corporation  */
                    /*  IEEE754: range (+1, -1]  */
                    /*  32-bit/64-bit format as defined by */
                    /*  MSVC++ float/double type */

#define  WAVE_FORMAT_IBM_CVSD              0x0005  /*  IBM Corporation  */
#define  WAVE_FORMAT_ALAW                  0x0006  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_MULAW                 0x0007  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_OKI_ADPCM             0x0010  /*  OKI  */
#define  WAVE_FORMAT_DVI_ADPCM             0x0011  /*  Intel Corporation  */
#define  WAVE_FORMAT_IMA_ADPCM             (WAVE_FORMAT_DVI_ADPCM)  /*  Intel Corporation  */
#define  WAVE_FORMAT_MEDIASPACE_ADPCM      0x0012  /*  Videologic  */
#define  WAVE_FORMAT_SIERRA_ADPCM          0x0013  /*  Sierra Semiconductor Corp  */
#define  WAVE_FORMAT_G723_ADPCM            0x0014  /*  Antex Electronics Corporation  */
#define  WAVE_FORMAT_DIGISTD               0x0015  /*  DSP Solutions, Inc.  */
#define  WAVE_FORMAT_DIGIFIX               0x0016  /*  DSP Solutions, Inc.  */
#define  WAVE_FORMAT_DIALOGIC_OKI_ADPCM    0x0017  /*  Dialogic Corporation  */
#define  WAVE_FORMAT_MEDIAVISION_ADPCM     0x0018  /*  Media Vision, Inc. */
#define  WAVE_FORMAT_YAMAHA_ADPCM          0x0020  /*  Yamaha Corporation of America  */
#define  WAVE_FORMAT_SONARC                0x0021  /*  Speech Compression  */
#define  WAVE_FORMAT_DSPGROUP_TRUESPEECH   0x0022  /*  DSP Group, Inc  */
#define  WAVE_FORMAT_ECHOSC1               0x0023  /*  Echo Speech Corporation  */
#define  WAVE_FORMAT_AUDIOFILE_AF36        0x0024  /*    */
#define  WAVE_FORMAT_APTX                  0x0025  /*  Audio Processing Technology  */
#define  WAVE_FORMAT_AUDIOFILE_AF10        0x0026  /*    */
#define  WAVE_FORMAT_DOLBY_AC2             0x0030  /*  Dolby Laboratories  */
#define  WAVE_FORMAT_GSM610                0x0031  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_MSNAUDIO              0x0032  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_ANTEX_ADPCME          0x0033  /*  Antex Electronics Corporation  */
#define  WAVE_FORMAT_CONTROL_RES_VQLPC     0x0034  /*  Control Resources Limited  */
#define  WAVE_FORMAT_DIGIREAL              0x0035  /*  DSP Solutions, Inc.  */
#define  WAVE_FORMAT_DIGIADPCM             0x0036  /*  DSP Solutions, Inc.  */
#define  WAVE_FORMAT_CONTROL_RES_CR10      0x0037  /*  Control Resources Limited  */
#define  WAVE_FORMAT_NMS_VBXADPCM          0x0038  /*  Natural MicroSystems  */
#define  WAVE_FORMAT_CS_IMAADPCM           0x0039  /*  Crystal Semiconductor IMA ADPCM */
#define  WAVE_FORMAT_ECHOSC3               0x003A  /*  Echo Speech Corporation */
#define  WAVE_FORMAT_ROCKWELL_ADPCM        0x003B  /*  Rockwell International */
#define  WAVE_FORMAT_ROCKWELL_DIGITALK     0x003C  /*  Rockwell International */
#define  WAVE_FORMAT_XEBEC                 0x003D  /*  Xebec Multimedia Solutions Limited */
#define  WAVE_FORMAT_G721_ADPCM            0x0040  /*  Antex Electronics Corporation  */
#define  WAVE_FORMAT_G728_CELP             0x0041  /*  Antex Electronics Corporation  */
#define  WAVE_FORMAT_MPEG                  0x0050  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_MPEGLAYER3            0x0055  /*  ISO/MPEG Layer3 Format Tag */
#define  WAVE_FORMAT_CIRRUS                0x0060  /*  Cirrus Logic  */
#define  WAVE_FORMAT_ESPCM                 0x0061  /*  ESS Technology  */
#define  WAVE_FORMAT_VOXWARE               0x0062  /*  Voxware Inc  */
#define  WAVEFORMAT_CANOPUS_ATRAC          0x0063  /*  Canopus, co., Ltd.  */
#define  WAVE_FORMAT_G726_ADPCM            0x0064  /*  APICOM  */
#define  WAVE_FORMAT_G722_ADPCM            0x0065  /*  APICOM      */
#define  WAVE_FORMAT_DSAT                  0x0066  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_DSAT_DISPLAY          0x0067  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_SOFTSOUND             0x0080  /*  Softsound, Ltd.      */
#define  WAVE_FORMAT_RHETOREX_ADPCM        0x0100  /*  Rhetorex Inc  */
#define  WAVE_FORMAT_CREATIVE_ADPCM        0x0200  /*  Creative Labs, Inc  */
#define  WAVE_FORMAT_CREATIVE_FASTSPEECH8  0x0202  /*  Creative Labs, Inc  */
#define  WAVE_FORMAT_CREATIVE_FASTSPEECH10 0x0203  /*  Creative Labs, Inc  */
#define  WAVE_FORMAT_QUARTERDECK           0x0220  /*  Quarterdeck Corporation  */
#define  WAVE_FORMAT_FM_TOWNS_SND          0x0300  /*  Fujitsu Corp.  */
#define  WAVE_FORMAT_BTV_DIGITAL           0x0400  /*  Brooktree Corporation  */
#define  WAVE_FORMAT_OLIGSM                0x1000  /*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_OLIADPCM              0x1001  /*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_OLICELP               0x1002  /*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_OLISBC                0x1003  /*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_OLIOPR                0x1004  /*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_LH_CODEC              0x1100  /*  Lernout & Hauspie  */
#define  WAVE_FORMAT_NORRIS                0x1400  /*  Norris Communications, Inc.  */
#define  WAVE_FORMAT_EXTENSIBLE            0xFFFE  /*  GUID determined type */


void wav_info(const wav_t *wav);
int32_t wav_open(const char *file, wav_t *wav);

#ifdef __cplusplus
}
#endif