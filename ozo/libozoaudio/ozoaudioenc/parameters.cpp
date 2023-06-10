/*
Copyright (C) 2019 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

#define LOG_NDEBUG 0
#define LOG_TAG "OzoParameterUpdater"
#include <utils/Log.h>

#include <tuple>
#include <stdlib.h>

#include "ozoaudiocommon/control.h"
#include "ozoaudio/ozo_tags.h"
#include "ozoaudio/utils.h"
#include "parameters.h"

namespace android {

enum {
    // Input string value is mapped to another value
    MAP2VAL = 0,

    // Input string value is converted to double value
    MAP2DOUBLE,
};

// Input string is mapped to internal control value
using OzoControlValueMapper = std::map<std::string, int32_t>;

// Control ID, selector ID, mapping mode, value mappings
using OzoControlMapper = std::tuple<ozoaudiocodec::CodecControllerID, uint32_t, uint32_t, OzoControlValueMapper>;

// Parameter range has minimum and maximum values for specified control ID
const std::map<uint32_t, std::vector<double>> OzoParameterRangeValidator {
    {ozoaudiocodec::kFocusGain, {ozoaudiocodec::kFocusZeroGain, ozoaudiocodec::kFocusMaxGain}}
};

const std::map<std::string, OzoControlMapper> OzoParameterMapper {
    // Custom processing mapping; custom=on/off
    {
        "custom",
        OzoControlMapper{
            ozoaudiocodec::kCustomId,
            ozoaudiocodec::kCustomMode,
            MAP2VAL,
            OzoControlValueMapper{
                {OZO_FEATURE_ENABLED, ozoaudiocodec::kCustomMode1Value},
                {OZO_FEATURE_DISABLED, ozoaudiocodec::kCustomMode0Value}
            }
        }
    },

    // Windscreen mapping; wnr=on/off
    {
        "wnr",
        OzoControlMapper{
            ozoaudiocodec::kWnrId,
            ozoaudiocodec::kWnrToggle,
            MAP2VAL,
            OzoControlValueMapper{
                {OZO_FEATURE_ENABLED, ozoaudiocodec::kWnrOnValue},
                {OZO_FEATURE_DISABLED, ozoaudiocodec::kWnrOffValue}
            }
        }
    },

    // Noise suppresion mapping; ns=on/smart/off
    {
        "ns",
        OzoControlMapper{
            ozoaudiocodec::kNoiseSuppressionId,
            0,
            MAP2VAL,
            OzoControlValueMapper{
                {OZO_FEATURE_ENABLED, ozoaudiocodec::kNsOnValue},
                {"smart", ozoaudiocodec::kNsSmartValue},
                {OZO_FEATURE_DISABLED, ozoaudiocodec::kNsOffValue},
            }
        }
    },

    // Zoom mapping; zoom=<double>
    {
        "zoom",
        OzoControlMapper{
            ozoaudiocodec::kFocusId,
            ozoaudiocodec::kFocusGain,
            MAP2DOUBLE,
            OzoControlValueMapper{}
        }
    },

    // Focus mapping; focus=on/off
    {
        "focus",
        OzoControlMapper{
            ozoaudiocodec::kFocusId,
            ozoaudiocodec::kFocusMode,
            MAP2VAL,
            OzoControlValueMapper{
                {OZO_FEATURE_ENABLED, ozoaudiocodec::kFocusOnMode},
                {OZO_FEATURE_DISABLED, ozoaudiocodec::kFocusOffMode}
            }
        }
    },

    // Focus azimuth mapping; focus-azimuth=<double>
    {
        "focus-azimuth",
        OzoControlMapper{
            ozoaudiocodec::kFocusId,
            ozoaudiocodec::kFocusAzimuth,
            MAP2DOUBLE,
            OzoControlValueMapper{}
        }
    },

    // Focus elevation mapping; focus-elevation=<double>
    {
        "focus-elevation",
        OzoControlMapper{
            ozoaudiocodec::kFocusId,
            ozoaudiocodec::kFocusElevation,
            MAP2DOUBLE,
            OzoControlValueMapper{}
        }
    },

    // Focus sector width mapping; focus-width=<double>
    {
        "focus-width",
        OzoControlMapper{
            ozoaudiocodec::kFocusId,
            ozoaudiocodec::kFocusSectorWidth,
            MAP2DOUBLE,
            OzoControlValueMapper{}
        }
    },

    // Focus sector height mapping; focus-height=<double>
    {
        "focus-height",
        OzoControlMapper{
            ozoaudiocodec::kFocusId,
            ozoaudiocodec::kFocusSectorHeight,
            MAP2DOUBLE,
            OzoControlValueMapper{}
        }
    },
};

OzoParameterUpdater::OzoParameterUpdater():
mParams(NULL)
{}

OzoParameterUpdater::~OzoParameterUpdater()
{
    this->clear();
}

void
OzoParameterUpdater::clear()
{
    this->mParamsOld.clear();
    this->mStorage.clear();
}

void OzoParameterUpdater::setParams(const char *params)
{
    this->mParams = params;
}

bool OzoParameterUpdater::updateParams(ozoaudiocodec::AudioControl *ctrl)
{
    bool status = true;

    if (this->mParams && this->mParamsOld.compare(this->mParams)) {
        std::vector<std::string> params = splitOzoParamString(this->mParams, ';');
        if (params.size()) {
            ALOGD("Generic parameters count: %zu (%s)", params.size(), this->mParams);
            for (size_t i = 0; i < params.size(); i++) {
                std::vector<std::string> param = splitOzoParamString(params[i], '=');
                ALOGD("%zu: key=%s, value=%s", i, param[0].c_str(), param[1].c_str());
                status = this->updateParam(ctrl, param[0], param[1]);
                if (!status)
                    break;
            }
        }

        this->mParamsOld.assign(this->mParams);
    }

    return status;
}

bool OzoParameterUpdater::hasParam(const std::string &param)
{
    auto it = this->mStorage.find(param);
    if (it != this->mStorage.end())
        return true;

    return false;
}

bool OzoParameterUpdater::updateParam(ozoaudiocodec::AudioControl *ctrl, const std::string &key, const std::string &value)
{
    bool needUpdate = false;

    // Update is done only when new parameter arrives or when parameter value changes
    if (!this->hasParam(key))
        needUpdate = true;
    else if (this->mStorage[key] != value)
        needUpdate = true;

    // Update parameter and save parameter pair on success for later update comparisons
    auto result = needUpdate ? this->update(ctrl, key, value) : true;
    if (result && needUpdate)
        this->mStorage[key] = value;

    return result;
}

bool OzoParameterUpdater::update(ozoaudiocodec::AudioControl *ctrl, const std::string &key, const std::string &value)
{
    // Find the data mapper for the key
    auto it = OzoParameterMapper.find(key);
    if (it != OzoParameterMapper.end()) {
        ozoaudiocodec::CodecControllerID ctrlID = std::get<0>(it->second);
        uint32_t selectorId = std::get<1>(it->second);
        int32_t valueMapMode = std::get<2>(it->second);

        ozoaudiocodec::AudioCodecError result;

        // Map the value to correct format
        if (valueMapMode == MAP2VAL) {
            auto mapValues = std::get<3>(it->second);

            auto itValue = mapValues.find(value);
            if (itValue == mapValues.end()) {
                ALOGE("Unsupported value: %s", value.c_str());
                return false;
            }

            int32_t mappedValue = itValue->second;

            // Assign value to encoder
            ALOGD("Ctrl ID=%i, selector ID=%i, value=%i", ctrlID, selectorId, mappedValue);
            if (selectorId == 0)
                result = ctrl->setState(ctrlID, (uint32_t) mappedValue);
            else
                result = ctrl->setState(ctrlID, selectorId, mappedValue);
        } else if (valueMapMode == MAP2DOUBLE) {
            double mappedValue = atof(value.c_str());

            // Validate range if needed
            auto rangeIt = OzoParameterRangeValidator.find(selectorId);
            if (rangeIt != OzoParameterRangeValidator.end()) {
                if (mappedValue < rangeIt->second[0] || mappedValue > rangeIt->second[1]) {
                    ALOGE("Invalid parameter value: %f", mappedValue);
                    return false;
                }
            }

            // Assign value to encoder
            ALOGD("Ctrl ID=%i, selector ID=%i, value=%f", ctrlID, selectorId, mappedValue);
            result = ctrl->setState(ctrlID, selectorId, mappedValue);
        } else {
            ALOGD("Value mapping failed, mode: %i", valueMapMode);
            return false;
        }

        // Validate result
        if (result != ozoaudiocodec::AudioCodecError::OK) {
            ALOGE("Unable to set control value: %i", result);
            return false;
        }

        return true;
    }

    ALOGE("Ozo parameter assignment failed, unknown key: %s", key.c_str());
    return false;
}

}  // namespace android
