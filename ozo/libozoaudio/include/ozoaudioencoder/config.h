#pragma once
/*
  Copyright (C) 2016 Nokia Corporation.
  This material, including documentation and any related
  computer programs, is protected by copyright controlled by
  Nokia Corporation. All rights are reserved. Copying,
  including reproducing, storing, adapting or translating, any
  or all of this material requires the prior written consent of
  Nokia Corporation. This material also contains confidential
  information which may not be disclosed to others without the
  prior written consent of Nokia Corporation.
*/

#include <stdio.h> // SEEK_SET
#include <stdint.h>
#include <stddef.h>

#include "ozoaudiocommon/ozo_audio_common.h"

namespace ozoaudioencoder {

/**
 * Identifier for binaural/stereo meta
 */
static const char *const kBinauralTag = "binaural";

/**
 * OZO Audio encoder input configuration. Contains general information regarding the input and
 * output signals.
 */
typedef struct OzoAudioEncoderInitConfig
{
    /**
     * Number of input channels.
     */
    int16_t n_in_channels;

    /**
     * Input channel layout.
     */
    const char *in_layout;

    /**
     * Input sample format.
     */
    ozoaudiocodec::SampleFormat in_sample_format;

    /**
     * Input sample rate, in Hz.
     */
    int32_t sample_rate;

    /**
     * Number of output channels.
     */
    int16_t n_out_channels;

    /**
     * Output channel layout.
     */
    const char *out_layout;

    /**
     * Output sample format.
     */
    ozoaudiocodec::SampleFormat out_sample_format;

    /**
     * Capturing device as UUID (Universally Unique IDentifier) format. The device name will be
     * communicated separately for each use case and no UUIDs are documented here.
     */
    const char *device;

} OzoAudioEncoderInitConfig;


/**
 * OZO Audio encoder codec configuration. Contains general information regarding the codec
 * operation.
 */
typedef struct OzoAudioEncoderCodecConfig
{
    /**
     * Encoding bitrate for audio (covering all channels), in kbps.
     */
    int16_t bitrate;

} OzoAudioEncoderCodecConfig;


/**
 * OZO Audio encoder output configuration. This data is filled by the encoder and is valid only on
 * successful initialization.
 */
typedef struct OzoAudioEncoderOutConfig
{
    /**
     * Input frame length, in samples per channel.
     */
    int32_t frame_length;

    /**
     * Accepted input sample rate, in Hz. This may differ from OzoAudioEncoderInitConfig::sample
     * rate. There may be situations where encoder requires lower sample rate to the used for
     * specified input configuration. An example is where low encoding bitrate is to be used.
     */
    int32_t sample_rate;

    /**
     * Configuration data for the container format. The configuration consists of audio and meta
     * data which describe the corresponding decoder configuration (such as number of channels,
     * sample rate, various constants to be used throughout the decoding etc) for each item. These
     * configurations are to be included to container format (e.g., MP4) and in the playback side
     * they are first extracted from the container format and then passed to the Ozo audio decoder
     * initialization method. Depending on encoder mode, meta data configuration may not be always
     * present; for example ambisonics mode does not produce any meta data configuration.
     *
     * For ozoaudio and ambisonics encoding modes the audio configuration data contains the decoder
     * specific configuration that has been standardized for the codec in question. For example,
     * if the audio encoder that is passed to the init method is AAC (Advanced Audio Encoding)
     * encoder, then the configuration data is what has been standardized for AAC in ISO/IEC
     * 14496-3.
     *
     * The meta data configuration follows the SPAC config syntax which has been specified in the
     * Nokia MP4VR specification.
     */
    ozoaudiocodec::AudioDataContainer vr_config;

    /**
     * Channel layout (chnl box) as described in ISO/IEC 14496-12 (ISO base media file format).
     */
    ozoaudiocodec::CodecData chnl;

    /**
     * Input output delay caused by spatial audio processing, audio encoding and audio decoding.
     * Value -1 indicates N/A.
     */
    int32_t delay;

} OzoAudioEncoderOutConfig;


/**
 * OZO Audio encoder configuration. This data structure is passed to the encoder initialization
 * method. Input and codec related data is set by the user of the API, whereas output configuration
 * data is set by the encoder.
 */
typedef struct OzoAudioEncoderConfig
{
    const OzoAudioEncoderInitConfig *in_config;
    const OzoAudioEncoderCodecConfig *codec_config;
    OzoAudioEncoderOutConfig out_config;

} OzoAudioEncoderConfig;

/**
 * OZO Audio encoder sound source related localization data.
 */
typedef struct OzoAudioSoundSource
{
    /**
     * Constructor.
     */
    OzoAudioSoundSource(int16_t azimuth, int8_t elevation, uint8_t strength, uint32_t id) :
        azimuth(azimuth),
        elevation(elevation),
        strength(strength),
        id(id)
    {}

    /**
     * Source azimuth in degrees, zero in front and positive values to the left. Range -180..179.
     */
    int16_t azimuth;

    /**
     * Source elevation in degrees, zero in front and positive values up. Range -90..90.
     */
    int8_t elevation;

    /**
     * Source strength. Range 0..100.
     */
    uint8_t strength;

    /**
     * Source id.
     */
    uint32_t id;

} OzoAudioSoundSource;

/**
 * OZO Audio encoder blocked microphone indication data.
 */
typedef struct OzoAudioMicBlocking
{
    /**
     * Constructor.
     */
    OzoAudioMicBlocking(uint16_t id, uint16_t value) : id(id), value(value)
    {}

    /**
     * Microphone channel index.
     */
    uint16_t id;

    /**
     * Mic blocking value. Range 0..100. (0 = not blocked)
     */
    uint16_t value;

} OzoAudioMicBlocking;

/**
 * Observer modes for the encoder.
 */
enum ObserverMode
{
    /**
     * Microphone data.
     */
    MICROPHONE = 0,

    /**
     * Encoder control data.
     */
    CONTROL = 1,

    /**
     * Wind noise indication data.
     */
    WINDNOISE = 2,

    /**
     * Sound source localization data.
     */
    SOUND_SOURCE = 3,

    /**
     * Level indication data.
     */
    LEVEL_INDICATION = 4,

    /**
     * Mic blocking indication data.
     */
    MIC_BLOCKING = 5,
};

/**
 * Interface for requesting capturing related data from encoder.
 */
class OzoEncoderObserver
{
public:
    /**
     * Return observer mode.
     */
    virtual ObserverMode getMode() const = 0;

    /**
     * Capturing related data from the encoder.
     *
     * @param [out] data Encoder data. The data is opaque and it is up to the caller to
     * handle the data accordingly. For example, if observer mode is
     * ozoaudioencoder::SOUND_SOURCE, data is pointer to OzoAudioSoundSource
     * and size indicates the number of sound sources available.
     * @param [out] size Size of data.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual bool process(const void *data, size_t size) = 0;

protected:
    /** @cond PRIVATE */
    OzoEncoderObserver() {}
    virtual ~OzoEncoderObserver() {}
    /** @endcond */
};


typedef struct WindIndicationParams
{
    /**
     * When noticeable wind level has been detected this value is true, othervice it is false.
     */
    bool windDetected;

    /**
     * Wind level value has range 0-100.
     */
    uint8_t windLevel;

} WindIndicationParams;

/**
 * Base class for an observer receiving wind indication data from encoder.
 */
class OzoEncoderWindIndicationObserver : public OzoEncoderObserver
{
public:
    /**
     * Return observer mode.
     */
    virtual ObserverMode getMode() const override
    {
        return ObserverMode::WINDNOISE;
    }

    /**
     * Capturing related data from the encoder.
     *
     * @param [out] data Encoder data.
     * @param [out] size Size of data.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual bool process(const void *data, size_t size) override
    {
        if (data == nullptr || size != sizeof(WindIndicationParams))
            return false;

        m_windParams = *reinterpret_cast<const WindIndicationParams *>(data);
        return true;
    }

protected:
    /** @cond PRIVATE */
    OzoEncoderWindIndicationObserver() : m_windParams{false, 0} {}
    virtual ~OzoEncoderWindIndicationObserver() = 0;
    /** @endcond */

protected:
    WindIndicationParams m_windParams;
};

inline OzoEncoderWindIndicationObserver::~OzoEncoderWindIndicationObserver() {}


typedef struct LevelIndicationParams
{
    /**
     * Peak level values for stereo output channels have range 0-32768.
     */
    uint16_t level[2];

} LevelIndicationParams;

/**
 * Base class for an observer receiving level indication data from encoder.
 */
class OzoEncoderLevelIndicationObserver : public OzoEncoderObserver
{
public:
    /**
     * Return observer mode.
     */
    virtual ObserverMode getMode() const override
    {
        return ObserverMode::LEVEL_INDICATION;
    }

    /**
     * Capturing related data from the encoder.
     *
     * @param [out] data Encoder data.
     * @param [out] size Size of data.
     * @return ozoaudiocodec::AudioCodecError::OK on success, otherwise an error code.
     */
    virtual bool process(const void *data, size_t size) override
    {
        if (data == nullptr || size != sizeof(LevelIndicationParams))
            return false;

        m_levelParams = *reinterpret_cast<const LevelIndicationParams *>(data);
        return true;
    }

protected:
    /** @cond PRIVATE */
    OzoEncoderLevelIndicationObserver() : m_levelParams{0} {}
    virtual ~OzoEncoderLevelIndicationObserver() = 0;
    /** @endcond */

protected:
    LevelIndicationParams m_levelParams;
};

inline OzoEncoderLevelIndicationObserver::~OzoEncoderLevelIndicationObserver() {}

/**
 * Interface for abstracting file write and read access.
 */
class FileWriterReader
{
public:
    /**
     * Write data to file.
     *
     * @param data Write data.
     * @param size Size of data to be written.
     * @return Number of items written
     */
    virtual size_t write(const void *data, size_t size) = 0;

    /**
     * Read data from file.
     *
     * @param [out] data Read data.
     * @param size Size of data to be read.
     * @return Number of items read.
     */
    virtual size_t read(void *data, size_t size) = 0;
    /**
     * Change file position.
     *
     * @param offset New position in bytes with respect to the reference position.
     * @param whence File position reference (SEEK_SET, SEEK_CUR, SEEK_END)
     * @return true on success, false on error.
     */
    virtual bool seek(size_t offset, int whence = SEEK_SET) = 0;

    /**
     * Read newline terminated string from file.
     *
     * @param [out] data String data.
     * @param len Size of input data buffer.
     * @return Length of string.
     */
    virtual size_t readLine(char *data, size_t len) const = 0;

    /**
     * Return current file position.
     */
    virtual size_t position() const = 0;

    /**
     * Close file object.
     */
    virtual void close() = 0;

protected:
    /** @cond PRIVATE */
    FileWriterReader() {}
    virtual ~FileWriterReader() {}
    /** @endcond */
};

} // namespace ozoaudioencoder
