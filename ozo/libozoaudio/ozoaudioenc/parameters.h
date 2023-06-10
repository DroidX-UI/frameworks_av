/*
Copyright (C) 2020 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing, adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.
*/

#include <string>
#include <map>

#include "ozoaudiocommon/control.h"

namespace android {

class OzoParameterUpdater
{
public:
    OzoParameterUpdater();
    ~OzoParameterUpdater();

    // Assign new parameters
    void setParams(const char *params);

    // Update parameters to Ozo encoder
    // Parameters are in the following format: <key1>=<value1>;<key2>=<value2>;...
    bool updateParams(ozoaudiocodec::AudioControl *ctrl);

    // Clear state
    void clear();

private:
    // Return true if parameter is present in the storage
    bool hasParam(const std::string &param);

    // Update key/value based parameter. Update is done only if its value has changed
    bool updateParam(ozoaudiocodec::AudioControl *ctrl, const std::string &key, const std::string &value);

    // Update key/value parameter to Ozo encoder
    bool update(ozoaudiocodec::AudioControl *ctrl, const std::string &key, const std::string &value);

    const char *mParams;
    std::string mParamsOld;
    std::map<std::string, std::string> mStorage;
};

}  // namespace android
