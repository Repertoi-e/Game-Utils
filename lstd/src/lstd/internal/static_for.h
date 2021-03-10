#pragma once

#include "../types/type_info.h"

LSTD_BEGIN_NAMESPACE

//
// The math module uses this, but the module is itself imported in common.h, 
// so we can't put this in common.h
//

// This template function unrolls a loop at compile-time.
// The lambda should take "auto it" as a parameter and
// that can be used as a compile-time constant index.
//
// This is useful for when you can just write a for-loop
// instead of using template functional recursive programming.
template <s64 First, s64 Last, typename Lambda>
void static_for(Lambda &&f) {
    if constexpr (First < Last) {
        f(types::integral_constant<s64, First>{});
        static_for<First + 1, Last>(f);
    }
}

LSTD_END_NAMESPACE
