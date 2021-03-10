module;

#include "../internal/floating_point.h"
#include "../types/numeric_info.h"

export module math.frexp_ldexp;

/* Based on a version which carries the following copyright:  
 * https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/sysdeps/ieee754/dbl-64/wordsize-64/s_scalbn.c
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

import math.basic;
import math.constants;

LSTD_BEGIN_NAMESPACE

// Decomposes given floating point value into a normalized fraction
// and an integral power of two such that x = _Fraction_ * 2 ** _Exponent_.
// _Fraction_ is in the range (-1; -0.5] or [0.5; 1).
//
// If the arg was 0, both returned values are 0.
// If the arg was not finite, it is returned in _Fraction_ and _Exponent_ is unspecified.
struct decompose_float_result {
    f64 Fraction;
    s32 Exponent;
};

// (frexp) The inverse of load_exponent.
// Decomposes a float into a fraction and an exponent. See note above _decompose_float_result_.
//
// These two functions are used to modify floats without messing with the bits.
export constexpr decompose_float_result fraction_exponent(f64 x) {
    ieee754_f64 u = {x};

    s32 ex = u.ieee.E;
    s32 e = 0;
    if (ex != 0x7ff && x != 0.0) [[likely]] {
        // Not zero and finite
        e = ex - 1022;
        if (ex == 0) [[unlikely]] {
            // Subnormal.
            u.F *= 0x1p54;
            ex = u.ieee.E;
            e = ex - 1022 - 54;
        }
        u.ieee.E = 1022;
    } else {
        // Quiet signaling NaNs
        u.F += u.F;
    }
    return {u.F, e};
}

constexpr f64 TWO54 = 1.80143985094819840000e+16;   // 0x43500000, 0x00000000
constexpr f64 TWOM54 = 5.55111512312578270212e-17;  // 0x3C900000, 0x00000000

// (ldexp) The reverse of _fraction_exponent_.
// Multiplies x by 2 to the power of _exp_.
//
// These two functions are used to modify floats without messing with the bits.
export constexpr f64 load_exponent(f64 x, s32 n) {
    ieee754_f64 u = {x};

    s64 k = u.ieee.E;
    if (k == 0) [[unlikely]] {
        // 0 or subnormal
        if (u.ieee.M0 == 0 && u.ieee.M1 == 0) return x;  // +- 0

        u.F *= TWO54;
        k = u.ieee.E - 54;
    }

    if (k == 0x7ff) [[unlikely]] {
        return x + x;  // NaN or Inf
    }

    if (n < -50000) [[unlikely]] {
        return 1.0e-300 * copy_sign(1.0e-300, x);  // Underflow
    }

    if (n > 50000 || (k + n) > 0x7fe) [[unlikely]] {
        return 1.0e+300 * copy_sign(1.0e+300, x);  // Overflow
    }

    // Now k and n are bounded and we know that k = k + n does not overflow.
    k += n;

    if (k > 0) [[likely]] {
        u.ieee.E = k;
        return u.F;
    }

    if (k <= -54) {
        return 1.0e-300 * copy_sign(1.0e-300, x);  // Underflow
    } else {
        // Subnormal result
        k += 54;
        u.ieee.E = k;
        return u.F * TWOM54;
    }
}

LSTD_END_NAMESPACE
