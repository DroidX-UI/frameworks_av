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

#ifndef OZOAUDIO_SIMPLE_C2_INTERFACE_UTILS_H_
#define OZOAUDIO_SIMPLE_C2_INTERFACE_UTILS_H_

#include <util/C2InterfaceHelper.h>

namespace android {

template<typename T, typename ...Args>
std::shared_ptr<T> AllocSharedString(const Args(&... args), const char *str) {
    size_t len = strlen(str) + 1;
    std::shared_ptr<T> ret = T::AllocShared(len, args...);
    strcpy(ret->m.value, str);
    return ret;
}

template<typename T, typename ...Args>
std::shared_ptr<T> AllocSharedString(const Args(&... args), const std::string &str) {
    std::shared_ptr<T> ret = T::AllocShared(str.length() + 1, args...);
    strcpy(ret->m.value, str.c_str());
    return ret;
}

template <typename T>
struct Setter {
    typedef typename std::remove_reference<T>::type type;

    static C2R NonStrictValueWithNoDeps(
            bool mayBlock, C2InterfaceHelper::C2P<type> &me) {
        (void)mayBlock;
        return me.F(me.v.value).validatePossible(me.v.value);
    }

    static C2R NonStrictValuesWithNoDeps(
            bool mayBlock, C2InterfaceHelper::C2P<type> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();
        for (size_t ix = 0; ix < me.v.flexCount(); ++ix) {
            res.plus(me.F(me.v.m.values[ix]).validatePossible(me.v.m.values[ix]));
        }
        return res;
    }

    static C2R StrictValueWithNoDeps(
            bool mayBlock,
            const C2InterfaceHelper::C2P<type> &old,
            C2InterfaceHelper::C2P<type> &me) {
        (void)mayBlock;
        if (!me.F(me.v.value).supportsNow(me.v.value)) {
            me.set().value = old.v.value;
        }
        return me.F(me.v.value).validatePossible(me.v.value);
    }
};

}  // namespace android

#endif  // OZOAUDIO_SIMPLE_C2_INTERFACE_UTILS_H_
