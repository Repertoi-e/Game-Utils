module;

#include "../internal/floating_point.h"
#include "../types/numeric_info.h"

export module math.basic;

import math.constants;

LSTD_BEGIN_NAMESPACE


export {
    constexpr bool sign_bit(types::is_signed_integral auto x) { return x < 0; }
    constexpr bool sign_bit(types::is_unsigned_integral auto) { return false; }

    constexpr bool sign_bit(f32 x) { return ieee754_f32{x}.ieee.S; }
    constexpr bool sign_bit(f64 x) { return ieee754_f64{x}.ieee.S; }

    // Returns -1 if x is negative, 1 otherwise
    constexpr s32 sign_no_zero(types::is_scalar auto x) { return sign_bit(x) ? -1 : 1; }

    // Returns -1 if x is negative, 1 if positive, 0 otherwise
    constexpr s32 sign(types::is_scalar auto x) {
        if (x == decltype(x)(0)) return 0;
        return sign_no_zero(x);
    }

    template <types::is_floating_point T>
    constexpr T copy_sign(T x, T y) {
        if constexpr (sizeof(x) == 4) {
            ieee754_f32 formatx = {x}, formaty = {y};
            formatx.ieee.S = formaty.ieee.S;
            return formatx.F;
        } else {
            ieee754_f64 formatx = {x}, formaty = {y};
            formatx.ieee.S = formaty.ieee.S;
            return formatx.F;
        }
    }

    constexpr bool is_nan(types::is_floating_point auto x) {
        if constexpr (sizeof(x) == 4) {
            ieee754_f32 format = {x};
            return format.ieee.E == F32_MAX_EXP && format.ieee.M != 0;
        } else {
            ieee754_f64 format = {x};
            return format.ieee.E == F64_MAX_EXP && (format.ieee.M0 != 0 || format.ieee.M1 != 0);
        }
    }

    constexpr bool is_signaling_nan(types::is_floating_point auto x) {
        if constexpr (sizeof(x) == 4)
            return is_nan(x) && ieee754_f32{x}.ieee_nan.N == 0;
        else
            return is_nan(x) && ieee754_f64{x}.ieee_nan.N == 0;
    }

    constexpr bool is_infinite(types::is_floating_point auto x) {
        if constexpr (sizeof(x) == 4) {
            ieee754_f32 format = {x};
            return format.ieee.E == F32_MAX_EXP && format.ieee.M == 0;
        } else {
            ieee754_f64 format = {x};
            return format.ieee.E == F64_MAX_EXP && format.ieee.M0 == 0 && format.ieee.M1 == 0;
        }
    }

    constexpr bool is_finite(types::is_floating_point auto x) {
        if constexpr (sizeof(x) == 4)
            return ieee754_f32{x}.ieee.E != F32_MAX_EXP;
        else
            return ieee754_f64{x}.ieee.E != F64_MAX_EXP;
    }
}

template <typename T, typename U>
concept are_same_signage = (types::is_signed_integral<T> && types::is_signed_integral<U>) || (types::is_unsigned_integral<T> && types::is_unsigned_integral<U>);

export template <typename T, typename U>
requires(types::is_scalar<T> &&types::is_scalar<U>) constexpr T cast_numeric_safe(U y) {
    if constexpr (types::is_floating_point<T>) {
        static_assert((types::is_integral<U> || sizeof(T) >= sizeof(U)), "T is a float. U must be a float of the same size or smaller, or an integer. Otherwise information may be lost when casting.");
    } else if constexpr (types::is_integral<T> && types::is_integral<U>) {
        static_assert(sizeof(T) > sizeof(U) || (sizeof(T) == sizeof(U) && are_same_signage<T, U>), "Both T and U are integers. T must be larger than U, or if they have the same size, they must have the same signage. Otherwise information may be lost when casting.");
    } else {
        static_assert(false, "T was an integer, but U was a floating point. Information may be lost when casting.");
    }
    return (T) y;
}

constexpr auto min_(auto x, auto y) {
    auto y_casted = cast_numeric_safe<decltype(x)>(y);
    if constexpr (types::is_floating_point<decltype(x)>) {
        if (is_nan(x) || is_nan(y_casted)) return x + y_casted;
    }
    return x < y_casted ? x : y_casted;
}

constexpr auto max_(auto x, auto y) {
    auto y_casted = cast_numeric_safe<decltype(x)>(y);
    if constexpr (types::is_floating_point<decltype(x)>) {
        if (is_nan(x) || is_nan(y_casted)) return x + y_casted;
    }
    return x > y_casted ? x : y_casted;
}

export {
    template <types::is_scalar... Args>
    constexpr auto min(types::is_scalar auto x, Args... rest) {
        auto result = x;
        ((void) (result = min_(result, rest)), ...);
        return result;
    }

    template <types::is_scalar... Args>
    constexpr auto max(types::is_scalar auto x, Args... rest) {
        auto result = x;
        ((void) (result = max_(result, rest)), ...);
        return result;
    }

    // Returns lower if x < lower, return upper if x > upper, returns x otherwise
    constexpr always_inline auto clamp(auto x, auto lower, auto upper) { return max(lower, min(upper, x)); }

    constexpr auto abs(types::is_scalar auto x) {
        if constexpr (types::is_floating_point<decltype(x)>) {
            if constexpr (sizeof(x) == 4) {
                ieee754_f32 u = {x};
                u.ieee.S = 0;
                return u.F;
            } else {
                ieee754_f64 u = {x};
                u.ieee.S = 0;
                return u.F;
            }
        } else {
            if constexpr (types::is_unsigned_integral<decltype(x)>) {
                return x;  // Unsigned integrals are always positive
            } else {
                return x < 0 ? -x : x;
            }
        }
    }

    // Checks if x is a power of 2
    constexpr bool is_pow_of_2(types::is_integral auto x) { return (x & (x - 1)) == 0; }

    // Returns the smallest power of 2 bigger or equal to x.
    template <types::is_integral T>
    constexpr auto ceil_pow_of_2(T x) {
        if (x <= 1) return T(1);

        T power = 2;
        --x;
        while (x >>= 1) power <<= 1;
        return power;
    }

    // Returns 10 ** exp at compile-time. Uses recursion.
    template <typename T>
    constexpr T const_exp10(s32 exp) { return exp == 0 ? T(1) : T(10) * const_exp10<T>(exp - 1); }
}

LSTD_END_NAMESPACE
