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
 * @brief WAVE file interface implementations supporting multichannel configurations.
 */

#define LOG_TAG "WaveReader"

#include <errno.h>
#include <string.h>
#include <media/stagefright/foundation/ADebug.h>

#include "wavformat.h"

typedef struct
{
    char id[4];
    uint32_t size;

} riff_ck_t;


/*
* General waveform format structure (information common to all formats).
*/
typedef struct /*waveformat_tag*/
{
    uint16_t wFormatTag;      /* format type */
    uint16_t nChannels;       /* number of channels (i.e. mono, stereo...) */
    uint32_t nSamplesPerSec;  /* sample rate */
    uint32_t nAvgBytesPerSec; /* for buffer estimation */
    uint16_t nBlockAlign;     /* block size of data */

} WAVEFORMAT;


typedef struct tWAVEFORMATEX
{
    uint16_t wFormatTag;      /* format type */
    uint16_t nChannels;       /* number of channels (i.e. mono, stereo...) */
    uint32_t nSamplesPerSec;    /* sample rate */
    uint32_t nAvgBytesPerSec;   /* for buffer estimation */
    uint16_t nBlockAlign;     /* block size of data */
    uint16_t wBitsPerSample;  /* Number of bits per sample of mono data */
    uint16_t cbSize;          /* The count in bytes of the size of extra information (after cbSize) */

} WAVEFORMATEX;



typedef struct
{
  uint32_t id;
  const char *name;

} id_name_t;

#define END_ID (0xffffffff)

id_name_t speaker_pos[] = {
  {SPEAKER_FRONT_LEFT, "FRONT_LEFT"},
  {SPEAKER_FRONT_RIGHT, "FRONT_RIGHT"},
  {SPEAKER_FRONT_CENTER, "FRONT_CENTER"},
  {SPEAKER_LOW_FREQUENCY, "LOW_FREQUENCY"},
  {SPEAKER_BACK_LEFT, "BACK_LEFT"},
  {SPEAKER_BACK_RIGHT, "BACK_RIGHT"},
  {SPEAKER_FRONT_LEFT_OF_CENTER, "FRONT_LEFT_OF_CENTER"},
  {SPEAKER_FRONT_RIGHT_OF_CENTER, "FRONT_RIGHT_OF_CENTER"},
  {SPEAKER_BACK_CENTER, "BACK_CENTER"},
  {SPEAKER_SIDE_LEFT, "SIDE_LEFT"},
  {SPEAKER_SIDE_RIGHT, "SIDE_RIGHT"},
  {SPEAKER_TOP_CENTER, "TOP_CENTER"},
  {SPEAKER_TOP_FRONT_LEFT, "TOP_FRONT_LEFT"},
  {SPEAKER_TOP_FRONT_CENTER, "TOP_FRONT_CENTER"},
  {SPEAKER_TOP_FRONT_RIGHT, "TOP_FRONT_RIGHT"},
  {SPEAKER_TOP_BACK_LEFT, "TOP_BACK_LEFT"},
  {SPEAKER_TOP_BACK_CENTER, "TOP_BACK_CENTER"},
  {SPEAKER_TOP_BACK_RIGHT, "TOP_BACK_RIGHT"},
  {END_ID, "UNKNOWN"}
};

id_name_t wav_fmt[] = {
  {WAVE_FORMAT_UNKNOWN, "UNKNOWN"},
  {WAVE_FORMAT_PCM, "PCM"},
  {WAVE_FORMAT_ADPCM, "ADPCM"},
  {WAVE_FORMAT_IEEE_FLOAT, "IEEE_FLOAT"},
  {WAVE_FORMAT_IBM_CVSD, "IBM_CVSD"},
  {WAVE_FORMAT_ALAW, "ALAW"},
  {WAVE_FORMAT_MULAW, "MULAW"},
  {WAVE_FORMAT_OKI_ADPCM, "OKI_ADPCM"},
  {WAVE_FORMAT_DVI_ADPCM, "DVI_ADPCM"},
  {WAVE_FORMAT_IMA_ADPCM, "IMA_ADPCM"},
  {WAVE_FORMAT_MEDIASPACE_ADPCM, "MEDIASPACE_ADPCM"},
  {WAVE_FORMAT_SIERRA_ADPCM, "SIERRA_ADPCM"},
  {WAVE_FORMAT_G723_ADPCM, "G723_ADPCM"},
  {WAVE_FORMAT_DIGISTD, "DIGISTD"},
  {WAVE_FORMAT_DIGIFIX, "DIGIFIX"},
  {WAVE_FORMAT_DIALOGIC_OKI_ADPCM, "DIALOGIC_OKI_ADPCM"},
  {WAVE_FORMAT_MEDIAVISION_ADPCM, "MEDIAVISION_ADPCM"},
  {WAVE_FORMAT_YAMAHA_ADPCM, "YAMAHA_ADPCM"},
  {WAVE_FORMAT_SONARC, "SONARC"},
  {WAVE_FORMAT_DSPGROUP_TRUESPEECH, "DSPGROUP_TRUESPEECH"},
  {WAVE_FORMAT_ECHOSC1, "ECHOSC1"},
  {WAVE_FORMAT_AUDIOFILE_AF36, "AUDIOFILE_AF36"},
  {WAVE_FORMAT_APTX, "APTX"},
  {WAVE_FORMAT_AUDIOFILE_AF10, "AUDIOFILE_AF10"},
  {WAVE_FORMAT_DOLBY_AC2, "DOLBY_AC2"},
  {WAVE_FORMAT_GSM610, "GSM610"},
  {WAVE_FORMAT_MSNAUDIO, "MSNAUDIO"},
  {WAVE_FORMAT_ANTEX_ADPCME, "ANTEX_ADPCME"},
  {WAVE_FORMAT_CONTROL_RES_VQLPC, "CONTROL_RES_VQLPC"},
  {WAVE_FORMAT_DIGIREAL, "DIGIREAL"},
  {WAVE_FORMAT_DIGIADPCM, "DIGIADPCM"},
  {WAVE_FORMAT_CONTROL_RES_CR10, "CONTROL_RES_CR10"},
  {WAVE_FORMAT_NMS_VBXADPCM, "NMS_VBXADPCM"},
  {WAVE_FORMAT_CS_IMAADPCM, "CS_IMAADPCM"},
  {WAVE_FORMAT_ECHOSC3, "ECHOSC3"},
  {WAVE_FORMAT_ROCKWELL_ADPCM, "ROCKWELL_ADPCM"},
  {WAVE_FORMAT_ROCKWELL_DIGITALK, "ROCKWELL_DIGITALK"},
  {WAVE_FORMAT_XEBEC, "XEBEC"},
  {WAVE_FORMAT_G721_ADPCM, "G721_ADPCM"},
  {WAVE_FORMAT_G728_CELP, "G728_CELP"},
  {WAVE_FORMAT_MPEG, "MPEG"},
  {WAVE_FORMAT_MPEGLAYER3, "MPEGLAYER3"},
  {WAVE_FORMAT_CIRRUS, "CIRRUS"},
  {WAVE_FORMAT_ESPCM, "ESPCM"},
  {WAVE_FORMAT_VOXWARE, "VOXWARE"},
  {WAVEFORMAT_CANOPUS_ATRAC, "WAVEFORMAT_CANOPUS_ATRAC"},
  {WAVE_FORMAT_G726_ADPCM, "G726_ADPCM"},
  {WAVE_FORMAT_G722_ADPCM, "G722_ADPCM"},
  {WAVE_FORMAT_DSAT, "DSAT"},
  {WAVE_FORMAT_DSAT_DISPLAY, "DSAT_DISPLAY"},
  {WAVE_FORMAT_SOFTSOUND, "SOFTSOUND"},
  {WAVE_FORMAT_RHETOREX_ADPCM, "RHETOREX_ADPCM"},
  {WAVE_FORMAT_CREATIVE_ADPCM, "CREATIVE_ADPCM"},
  {WAVE_FORMAT_CREATIVE_FASTSPEECH8, "CREATIVE_FASTSPEECH8"},
  {WAVE_FORMAT_CREATIVE_FASTSPEECH10, "CREATIVE_FASTSPEECH10"},
  {WAVE_FORMAT_QUARTERDECK, "QUARTERDECK"},
  {WAVE_FORMAT_FM_TOWNS_SND, "FM_TOWNS_SND"},
  {WAVE_FORMAT_BTV_DIGITAL, "BTV_DIGITAL"},
  {WAVE_FORMAT_OLIGSM, "OLIGSM"},
  {WAVE_FORMAT_OLIADPCM, "OLIADPCM"},
  {WAVE_FORMAT_OLICELP, "OLICELP"},
  {WAVE_FORMAT_OLISBC, "OLISBC"},
  {WAVE_FORMAT_OLIOPR, "OLIOPR"},
  {WAVE_FORMAT_LH_CODEC, "LH_CODEC"},
  {WAVE_FORMAT_NORRIS, "NORRIS"},
  {WAVE_FORMAT_EXTENSIBLE, "EXTENSIBLE"},
  {END_ID, "UNKNOWN"}
};

int32_t
wav_open(const char *file, wav_t *wav)
{
  int32_t type = WAV_UNK;
  riff_ck_t ck;
  char wav_str[4];
  uint32_t ttl_sz, rem = 0;
  size_t ret;

  wav->fp = fopen(file, "rb");
  if(wav->fp == 0)
  {
    ALOGD("Failed opening file '%s' %s", file, strerror(errno));
    return (0);
  }

  ret = fread((uint8_t *) &ck, 1, sizeof (riff_ck_t), wav->fp);
  if(ret != sizeof(riff_ck_t) || strncmp ("RIFF", ck.id, 4))
  {
    ALOGD("Not RIFF format!\n");
    return 0;
  }

  ttl_sz = ck.size;
  ret = fread(wav_str, sizeof (wav_str), 1, wav->fp);
  if(ret != 1 || strncmp ("WAVE", wav_str, 4))
  {
    ALOGD("Not WAVE format!\n");
    return 0;
  }

  ret = fread (&ck, sizeof (riff_ck_t), 1, wav->fp);
  if(ret != 1 || strncmp ("fmt ", ck.id, 4))
  {
    ALOGD("fmt field expected\n");
    return 0;
  }

  if(ck.size >= sizeof(WAVEFORMATEXTENSIBLE))
  {
    rem = ck.size - sizeof (WAVEFORMATEXTENSIBLE);
    ret = fread(&wav->fmt, sizeof (WAVEFORMATEXTENSIBLE), 1, wav->fp);
    if (ret == 1)
        type = WAV_EXT;
  }
  else if(ck.size >= sizeof (WAVEFORMATEX))
  {
    rem = ck.size - sizeof (WAVEFORMATEX);
    ret = fread(&wav->fmt, sizeof (WAVEFORMATEX), 1, wav->fp);
    if (ret == 1)
        type = WAV_E;
  }
  else if(ck.size >= sizeof (WAVEFORMAT))
  {
    rem = ck.size - sizeof (WAVEFORMAT);
    ret = fread(&wav->fmt, sizeof (WAVEFORMAT), 1, wav->fp);
    if (ret == 1)
        type = WAV_ORIG;
  }

  if (type == WAV_UNK) {
    ALOGD("Unknown WAVE header!\n");
    return 0;
  }

  if(rem) fseek (wav->fp, rem, SEEK_CUR);

  ret = fread (&ck, sizeof (riff_ck_t), 1, wav->fp);
  if (ret != 1) {
      ALOGD("Data chunk expected before loop\n");
      return 0;
  }

  while(strncmp ("data", ck.id, 4))
  {
    if(ck.size % 4)
      ck.size += 4 - ck.size % 4;   /* force 4 byte alignment */
    if(fseek (wav->fp, ck.size, SEEK_CUR))
      break;
    if(fread (&ck, sizeof (riff_ck_t), 1, wav->fp) != 1)
      break;
  }

  if(strncmp ("data", ck.id, 4))
  {
    ck.size = 0;        /* terminate the string */
    ALOGD("Data chunk expected %s\n", ck.id);
    return 0;
  }

  wav->type = type;
  wav->sample_pos = 0;
  wav->total_samples = ck.size / wav->fmt.nBlockAlign;

  /*-- We are now at the start of waveform data. --*/
  wav->data_start_pos = (uint32_t)ftell(wav->fp);

  return (1L);
}

static const char *
get_name(uint32_t id, const id_name_t * map)
{
  int16_t i = 0;

  while(map[i].id != END_ID)
  {
    if(id == map[i].id)
      return map[i].name;

    if(id < map[i].id)
      break;            /* assuming increasing order */

    ++i;
  }

  return "UNKNOWN";
}

void
wav_info(const wav_t *wav)
{
  ALOGD("WAVE details:\n-------------\nFormat: 0x%x %s\nChannels: %d\nSample rate: %ld\nBlock align: %d\n",
        wav->fmt.wFormatTag, get_name(wav->fmt.wFormatTag, wav_fmt),
        wav->fmt.nChannels, (long int) wav->fmt.nSamplesPerSec,
        wav->fmt.nBlockAlign);

  if(wav->type == WAV_E || wav->type == WAV_EXT)
    ALOGD("Bits/sample: %d\n", wav->fmt.wBitsPerSample);

  if(wav->type == WAV_EXT)
  {
    uint32_t n, i = 0;

    if(wav->fmt.wBitsPerSample)
      ALOGD("Valid bits/sample: %d\n", wav->fmt.Samples.wValidBitsPerSample);
    else
      ALOGD("Samples per block: %d\n", wav->fmt.Samples.wSamplesPerBlock);

    ALOGD("Speaker map 0x%lx: ", (long unsigned int) wav->fmt.dwChannelMask);

    i = 1;
    for(n = 0; n < 32; ++n)
    {
      if(i & wav->fmt.dwChannelMask)
        ALOGD(" %s,", get_name (i, speaker_pos));
      i <<= 1;
    }
    ALOGD("\n");
  }

  ALOGD("Total samples %lu: (%.2fs)\n",
    (long unsigned int) wav->total_samples, (float) wav->total_samples / wav->fmt.nSamplesPerSec);
}