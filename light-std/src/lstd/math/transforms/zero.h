#pragma once

#include "../mat_util.h"
#include "../vec_func.h"

LSTD_BEGIN_NAMESPACE

struct zero_helper : non_copyable {
    template <typename T, s64 Dim, bool Packed>
    operator vec<T, Dim, Packed>() const {
        vec<T, Dim, Packed> v = {no_init};
        fill(v, T(0));
        return v;
    }

    template <typename T, s64 R, s64 C, bool Packed>
    operator mat<T, R, C, Packed>() const {
        mat<T, R, C, Packed> m = {no_init};
        For(range(m.StripeCount)) fill(m.Stripes[it], T(0));
        return m;
    }
};

inline auto zero() { return zero_helper{}; }

LSTD_END_NAMESPACE
