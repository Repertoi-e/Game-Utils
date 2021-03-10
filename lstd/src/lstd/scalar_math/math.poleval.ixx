module;

#include "../internal/static_for.h"

export module math.poleval;

LSTD_BEGIN_NAMESPACE

// Cephes Math Library Release 2.8: June, 2000
// Copyright 1985, 1995, 2000 by Stephen L. Moshier

// Modified versions to unroll loops at compile-time

export template <s64 N>
constexpr f64 poleval(f64 x, const f64 c[]) {
    f64 result = c[0];
    static_for<1, N>([&](auto i) {
        result = result * x + c[i];
    });
    return result;
}

// Same as poleval but treats the coeff in front of x^n as 1.
export template <s64 N>
constexpr f64 poleval_1(f64 x, const f64 c[]) {
    f64 result = x + c[1];
    static_for<2, N>([&](auto i) {
        result = result * x + c[i];
    });
    return result;
}

LSTD_END_NAMESPACE
