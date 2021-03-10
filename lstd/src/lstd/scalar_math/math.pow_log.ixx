module;

#include "../internal/floating_point.h"
#include "../types/numeric_info.h"

export module math.pow_log;

//
// Cephes Math Library Release 2.8: June, 2000
// Copyright 1985, 1995, 2000 by Stephen L. Moshier
//

import math.basic;
import math.frexp_ldexp;
import math.poleval;
import math.constants;

LSTD_BEGIN_NAMESPACE

export {
    constexpr f64 pow(f64 a, f64 x);                                   // Returns a ** x
    constexpr f64 pow(f64 a, types::is_integral auto x) { return 0; }  // Returns a ** x (where x is an integer)
    constexpr f64 exp(f64 x) { return 0; }                             // Returns e ** x
    constexpr f64 exp(types::is_integral auto x);                      // Returns e ** x (where x is an integer)
    constexpr f64 sqrt(f64 x) { return 0; }                            // Returns the square root of x
    constexpr f64 ln(f64 x) { return 0; }                              // Returns the natural logarithm (log_e)  of x
    constexpr f64 log2(f64 x) { return 0; }                            // Returns the log_2 of x
    constexpr f64 log10(f64 x) { return 0; }                           // Returns the log_10 of x

    always_inline constexpr f32 pow(f32 a, f32 x) { return (f32) pow((f64) a, (f64) x); }
    always_inline constexpr f32 pow(f32 a, types::is_integral auto x) { return (f32) pow((f64) a, x); }
    always_inline constexpr f32 exp(f32 x) { return (f32) exp((f64) x); }
    always_inline constexpr f32 exp(types::is_integral auto x) { return (f32) exp(x); }
    always_inline constexpr f32 sqrt(f32 x) { return (f32) sqrt((f64) x); }
    always_inline constexpr f32 log2(f32 x) { return (f32) log2((f64) x); }
    always_inline constexpr f32 log10(f32 x) { return (f32) log10((f64) x); }
}

constexpr f64 P[] = {
    4.97778295871696307983711449197E-1,
    3.73336776063286857052503364685E0,
    7.69994162726912545480217886507E0,
    4.66651806774358490770282514859E0};
constexpr f64 Q[] = {
    9.33340916416696231294736207929E0,
    2.79999886606328409754951280775E1,
    3.35994905342304406303810537793E1,
    1.39995542032307547231084754458E1};
constexpr f64 A[] = {
    1.0E0,
    9.57603280698573700036035916128E-1,
    9.17004043204671215328005473566E-1,
    8.78126080186649726755376832443E-1,
    8.40896415253714502036075373326E-1,
    8.0524516597462714173616404878E-1,
    7.71105412703970372056971882557E-1,
    7.38413072969749673113426524651E-1,
    7.07106781186547572737310929369E-1,
    6.77127773468446325644265471055E-1,
    6.48419777325504820275625661452E-1,
    6.20928906036742001006700775179E-1,
    5.94603557501360513448673827952E-1,
    5.69394317378345782287851761794E-1,
    5.45253866332628844837415726943E-1,
    5.2213689121370687740153471168E-1,
    5.0E-1};
constexpr f64 B[] = {
    0.0E0,
    1.6415536121228136017570290838E-17,
    4.09950501029074826006362195521E-17,
    3.97491740484881042808051941469E-17,
    -4.83364665672645672552734986488E-17,
    1.26912513974441574796455125396E-17,
    1.99100761573282305549014827378E-17,
    -1.52339103990623557348275585271E-17,
    0.0E0};
constexpr f64 R[] = {
    1.49664108433729299969762102651E-5,
    1.54010762792771896791468866361E-4,
    1.3333547696409771710773783937E-3,
    9.61812908476554311032469257725E-3,
    5.55041086645832321133653408651E-2,
    2.40226506959099778137911584963E-1,
    6.93147180559945286226763982995E-1};

export constexpr f64 pow(f64 x, f64 y) {
    // Find a multiple of 1/16 that is within 1/16 of x
    auto reduce = [](f64 x) -> f64 {
        f64 t = load_exponent(x, 4);
        t = floor(t);
        t = load_exponent(t, -4);
        return t;
    };

    // double w, z, W, Wa, Wb, ya, yb, u;
    /* double F, Fa, Fb, G, Ga, Gb, H, Ha, Hb */
    // double aw, ay, wy;
    // int e, i, nflg, iyflg, yoddint;

    if (y == 0.0) [[unlikely]]
        return 1.0;
    if (is_nan(x)) [[unlikely]]
        return x;
    if (is_nan(y)) [[unlikely]]
        return y;
    if (y == 1.0) [[unlikely]]
        return x;

    if (!is_finite(y) && (x == 1.0 || x == -1.0)) [[unlikely]] {
        // assert(false && "Domain");
        return numeric_info<f64>::signaling_NaN();
    }

    if (x == 1.0) [[unlikely]]
        return 1.0;

    if (y >= F64_MAX) [[unlikely]] {
        if (x > 1.0) return numeric_info<f64>::infinity();
        if (x > 0.0 && x < 1.0) return 0.0;
        if (x < -1.0) return numeric_info<f64>::infinity();
        if (x > -1.0 && x < 0.0) return 0.0;
    }

    if (y <= -F64_MAX) [[unlikely]] {
        if (x > 1.0) return 0.0;
        if (x > 0.0 && x < 1.0) return numeric_info<f64>::infinity();
        if (x < -1.0) return 0.0;
        if (x > -1.0 && x < 0.0) return numeric_info<f64>::infinity();
    }

    if (x >= F64_MAX) [[unlikely]] {
        if (y > 0.0) return numeric_info<f64>::infinity();
        return 0.0;
    }

    // Set flag if y is an integer
    f64 w = floor(y);
    bool integerY = w == y;

    // Test for odd integer y
    bool oddIntegerY = false;
    if (integerY) [[unlikely]] {
        f64 ya = floor(0.5 * abs(y));
        f64 yb = 0.5 * abs(w);
        if (ya != yb) oddIntegerY = true;
    }

    if (x <= -F64_MAX) [[unlikely]] {
        if (y > 0.0) {
            if (oddIntegerY) return -numeric_info<f64>::infinity();
            return numeric_info<f64>::infinity();
        }

        if (y < 0.0) {
            if (oddIntegerY) return -0.0;
            return 0.0;
        }
    }

    bool nxRaisedToInteger = false;  // Flag if x < 0 is raised to integer power
    if (x <= 0.0) {
        if (x == 0.0) [[unlikely]] {
            if (y < 0.0) {
                if (sign_bit(x) && oddIntegerY) return -numeric_info<f64>::infinity();
                return numeric_info<f64>::infinity();
            }
            if (y > 0.0) {
                if (sign_bit(x) && oddIntegerY) return -0.0;
                return 0.0;
            }
            return 1.0;
        } else {
            if (integerY == 0) {
                // Noninteger power of negative number
                // assert(false && "Domain");
                return numeric_info<f64>::signaling_NaN();
            }
            nxRaisedToInteger = true;
        }
    }

    if (integerY) [[unlikely]] {
        w = floor(x);
        if ((w == x) && (abs(y) < 32768.0)) {
            return pow(x, (s32) y);  // Call the integer exponent overload
        }
    }

    if (nxRaisedToInteger) [[unlikely]]
        x = abs(x);

    f64 z;  // Stores the result

    // For results close to 1, use a series expansion.
    w = x - 1.0;
    f64 aw = abs(w);
    f64 ay = abs(y);
    f64 wy = w * y;
    f64 ya = abs(wy);
    if ((aw <= 1.0e-3 && ay <= 1.0) || (ya <= 1.0e-3 && ay >= 1.0)) {
        z = (((((w * (y - 5.) / 720. + 1. / 120.) * w * (y - 4.) + 1. / 24.) * w * (y - 3.) + 1. / 6.) * w * (y - 2.) + 0.5) * w * (y - 1.)) * wy + wy + 1.;
    } else {
        // Separate significand from exponent
        auto [xx, e] = fraction_exponent(x);
        x = xx;

        // Find significand of x in antilog table A[]
        s32 i = 1;
        if (x <= A[9]) i = 9;
        if (x <= A[i + 4]) i += 4;
        if (x <= A[i + 2]) i += 2;
        if (x >= A[1]) i = -1;
        i += 1;

        // Find (x - A[i])/A[i]
        // in order to compute log(x/A[i]):
        //
        // log(x) = log( a x/a ) = log(a) + log(x/a)
        //
        // log(x/a) = log(1+v),  v = x/a - 1 = (x-a)/a
        x -= A[i];
        x -= B[i / 2];
        x /= A[i];

        // Rational approximation for log(1+v):
        //
        // log(1+v)  =  v  -  v**2/2  +  v**3 P(v) / Q(v)
        //
        z = x * x;
        w = x * (z * poleval<3>(x, P) / poleval_1<4>(x, Q));

        w = w - load_exponent(z, -1);  // w - 0.5 * z

        // Convert to base 2 logarithm:
        // multiply by log2(e)
        w = w + LOG2_E_MINUS_1 * w;

        // Note: x was not yet added in
        // to above rational approximation,
        // so do it now, while multiplying
        // by log2(e).
        z = w + LOG2_E_MINUS_1 * x;
        z = z + x;

        // Compute exponent term of the base 2 logarithm
        w = -i;
        w = load_exponent(w, -4);  // Divide by 16
        w += e;

        // Now base 2 log of x is w + z.
        // Multiply base 2 log by y, in extended precision.

        // Separate y into large part _ya_ and small part _yb_ less than 1/16
        f64 ya = reduce(y);
        f64 yb = y - ya;

        f64 F = z * y + w * yb;
        f64 Fa = reduce(F);
        f64 Fb = F - Fa;

        f64 G = Fa + w * ya;
        f64 Ga = reduce(G);
        f64 Gb = G - Ga;

        f64 H = Fb + Gb;
        f64 Ha = reduce(H);
        w = load_exponent(Ga + Ha, 4);

        // Test the power of 2 for overflow
        if (w > F64_MAX_EXP) {
            if (nxRaisedToInteger && oddIntegerY) return -numeric_info<f64>::infinity();
            return numeric_info<f64>::infinity();
        }

        if (w < (F64_MIN_EXP - 1)) {
            if (nxRaisedToInteger && oddIntegerY) return -0.0;
            return 0.0;
        }

        e = (s32) w;

        f64 Hb = H - Ha;
        if (Hb > 0.0) {
            e += 1;
            Hb -= 0.0625;
        }

        // Now the product y * log2(x)  =  Hb + e/16.0.
        //
        // Compute base 2 exponential of Hb,
        // where -0.0625 <= Hb <= 0.
        z = Hb * poleval<6>(Hb, R);  // z  =  2**Hb - 1

        // Express e/16 as an integer plus a negative number of 16ths.
        // Find lookup table entry for the fractional power of 2.

        i = e >= 0;

        i = e / 16 + i;
        e = 16 * i - e;
        w = A[e];

        z = w + w * z;            //    2**-e * ( 1 + (2**Hb-1) )
        z = load_exponent(z, i);  // Multiply by integer power of 2
    }

    // Negate if odd integer power of negative number
    if (nxRaisedToInteger && oddIntegerY) [[unlikely]] {
        if (z == 0.0) {
            z = -0.0;
        } else {
            z = -z;
        }
    }
    return z;
}
LSTD_END_NAMESPACE
