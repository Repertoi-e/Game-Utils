module;

#include "../internal/floating_point.h"
#include "../types/numeric_info.h"

export module math.sin_cos;

/* 
* SIN, DESCRIPTION:
*
* Range reduction is into intervals of pi/4. The reduction
* error is nearly eliminated by contriving an extended precision
* modular arithmetic.
*
* Two polynomial approximating functions are employed.
* Between 0 and pi/4 the sine is approximated by
*      x  +  x**3 P(x**2).
* Between pi/4 and pi/2 the cosine is represented as
*      1  -  x**2 Q(x**2).
*
*
* ACCURACY:
*
*                      Relative error:
* arithmetic   domain      # trials      peak         rms
*    DEC       0, 10       150000       3.0e-17     7.8e-18
*    IEEE -1.07e9,+1.07e9  130000       2.1e-16     5.4e-17
* 
* ERROR MESSAGES:
*
*   message           condition        value returned
* sin total loss   x > 1.073741824e9      0.0
*
* Partial loss of accuracy begins to occur at x = 2**30
* = 1.074e9.  The loss is not gradual, but jumps suddenly to
* about 1 part in 10e7.  Results may be meaningless for
* x > 2**49 = 5.6e14.  The routine as implemented flags a
* TLOSS error for x > 2**30 and returns 0.0.
*
* 
* COS, DESCRIPTION:
*
* Range reduction is into intervals of pi/4. The reduction
* error is nearly eliminated by contriving an extended precision
* modular arithmetic.
*
* Two polynomial approximating functions are employed.
* Between 0 and pi/4 the cosine is approximated by
*      1  -  x**2 Q(x**2).
* Between pi/4 and pi/2 the sine is represented as
*      x  +  x**3 P(x**2).
*
*
* ACCURACY:
*
*                      Relative error:
* arithmetic   domain      # trials      peak         rms
*    IEEE -1.07e9,+1.07e9  130000       2.1e-16     5.4e-17
*    DEC        0,+1.07e9   17000       3.0e-17     7.2e-18
*/

//
// Cephes Math Library Release 2.8: June, 2000
// Copyright 1985, 1995, 2000 by Stephen L. Moshier
//

import math.basic;
import math.frexp_ldexp;
import math.poleval;
import math.constants;

LSTD_BEGIN_NAMESPACE

constexpr f64 SIN_COEF[] =
    {1.58962301576546568060e-10,
     -2.50507477628578072866e-8,
     2.75573136213857245213e-6,
     -1.98412698295895385996e-4,
     8.33333333332211858878e-3,
     -1.66666666666666307295e-1};

constexpr f64 COS_COEF[] = {
    -1.13585365213876817300e-11,
    2.08757008419747316778e-9,
    -2.75573141792967388112e-7,
    2.48015872888517045348e-5,
    -1.38888888888730564116e-3,
    4.16666666666665929218e-2};

constexpr f64 DP1 = 7.85398125648498535156e-1;
constexpr f64 DP2 = 3.77489470793079817668e-8;
constexpr f64 DP3 = 2.69515142907905952645e-15;
constexpr f64 LOSSTH = 1.073741824e9;

template <bool Sin>
always_inline constexpr f64 sin_or_cos(f64 x) {
    if constexpr (Sin) {
        if (x == 0.0) [[unlikely]]
            return x;
    }

    if (is_nan(x)) [[unlikely]]
        return x;

    if (!is_finite(x)) [[unlikely]] {
        // assert(false && "Domain");  // Should we use asserts?
        return numeric_info<f64>::signaling_NaN();
    }

    ieee754_f64 ieee;
    ieee.F = x;

    s32 sign = 1;

    // If doing _sin_, save the sign
    if constexpr (Sin) {
        if (ieee.ieee.S) sign = -1;
    }

    // Make _x_ positive.
    ieee.ieee.S = 0;
    x = ieee.F;

    if (x > LOSSTH) [[unlikely]] {
        // assert(false && "Input too large, result may be meaningless. Try passing a smaller value (Note: sin and cos are periodic functions)");
        // return 0.0;
        return numeric_info<f64>::signaling_NaN();
    }

    // Integer part of x / PIO4
    f64 y = floor(x / (PI / 4));

    // Strip high bits of integer part to prevent integer overflow
    f64 z = load_exponent(y, -4);
    z = floor(z);                 // Integer part of y/8
    z = y - load_exponent(z, 4);  // y - 16 * (y/16)

    // Convert to integer for tests on the phase angle
    s32 j = (s32) z;

    // Map zeros to origin
    if (j & 1) {
        j += 1;
        y += 1.0;
    }

    j = j & 7;  // Octant modulo 360 degrees

    // Reflect in x axis
    if (j > 3) {
        sign = -sign;
        j -= 4;
    }

    if constexpr (!Sin) {
        if (j > 1) sign = -sign;
    }

    // Extended precision modular arithmetic
    z = ((x - y * DP1) - y * DP2) - y * DP3;

    f64 zz = z * z;
    if ((j == 1) || (j == 2)) {
        if constexpr (Sin) {
            y = 1.0 - load_exponent(zz, -1) + zz * zz * poleval<5>(zz, COS_COEF);
        } else {
            y = z + z * (zz * poleval<5>(zz, SIN_COEF));
        }
    } else {
        if constexpr (Sin) {
            y = z + z * (zz * poleval<5>(zz, SIN_COEF));
        } else {
            y = 1.0 - load_exponent(zz, -1) + zz * zz * poleval<5>(zz, COS_COEF);
        }
    }

    return y * sign;
}

export {
    constexpr f64 sin(f64 x) { return sin_or_cos<true>(x); }
    constexpr f64 cos(f64 x) { return sin_or_cos<false>(x); }

    constexpr f64 asin(f64 x) { return 0; }
    constexpr f64 acos(f64 x) { return 0; }

    always_inline constexpr f32 sin(f32 x) { return (f32) sin((f64) x); }
    always_inline constexpr f32 cos(f32 x) { return (f32) cos((f64) x); }
    always_inline constexpr f32 asin(f32 x) { return (f32) asin((f64) x); }
    always_inline constexpr f32 acos(f32 x) { return (f32) acos((f64) x); }
}

LSTD_END_NAMESPACE
