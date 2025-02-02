/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <chrono>
#include <cmath>
#include <functional>
#include <string>

#include <common/trace.h>
#include <cutils/compiler.h>

namespace android {

namespace {
template <class Rep, class Period>
bool signbit(std::chrono::duration<Rep, Period> v) {
    return std::signbit(std::chrono::duration_cast<std::chrono::nanoseconds>(v).count());
}

template <typename Enum, typename std::enable_if<std::is_enum<Enum>::value>::type* = nullptr>
bool signbit(Enum e) {
    return std::signbit(static_cast<typename std::underlying_type<Enum>::type>(e));
}

template <typename T, typename std::enable_if<!std::is_enum<T>::value>::type* = nullptr>
bool signbit(T t) {
    return std::signbit(t);
}

template <typename T>
int64_t to_int64(T v) {
    return int64_t(v);
}

template <class Rep, class Period>
int64_t to_int64(std::chrono::duration<Rep, Period> v) {
    return int64_t(v.count());
}
} // namespace

template <typename T>
class TracedOrdinal {
public:
    static_assert(std::is_same<bool, T>() || (std::is_signed<T>() && std::is_integral<T>()) ||
                          std::is_same<std::chrono::nanoseconds, T>() || std::is_enum<T>(),
                  "Type is not supported. Please test it with systrace before adding "
                  "it to the list.");

    TracedOrdinal(std::string name, T initialValue)
          : mName(std::move(name)), mHasGoneNegative(signbit(initialValue)), mData(initialValue) {
        trace();
    }

    T get() const { return mData; }

    operator T() const { return get(); }

    TracedOrdinal& operator=(T other) {
        mData = other;
        mHasGoneNegative = mHasGoneNegative || signbit(mData);
        trace();
        return *this;
    }

private:
    void trace() {
        if (CC_LIKELY(!SFTRACE_ENABLED())) {
            return;
        }

        if (mNameNegative.empty()) {
            mNameNegative = mName + "Negative";
        }

        if (!signbit(mData)) {
            SFTRACE_INT64(mName.c_str(), to_int64(mData));
            if (mHasGoneNegative) {
                SFTRACE_INT64(mNameNegative.c_str(), 0);
            }
        } else {
            SFTRACE_INT64(mNameNegative.c_str(), -to_int64(mData));
            SFTRACE_INT64(mName.c_str(), 0);
        }
    }

    const std::string mName;
    std::string mNameNegative;
    bool mHasGoneNegative;
    T mData;
};

} // namespace android
