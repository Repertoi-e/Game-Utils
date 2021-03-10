module;

#include "../internal/floating_point.h"
#include "../types/numeric_info.h"

export module math.ceil_floor_round;

/* Based on a version which carries the following copyright:  */

/* Round double to integer away from zero.
   Copyright (C) 2011-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 2011.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* Which itself is based on a version which carries the following copyright:  */

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

export {
    // Returns the smallest integer greater than or equal to x.
    // It truncates toward +Inf.
    constexpr f64 ceil(f64 x) {
        ieee754_f64 u = {x};

        u32 j0 = u.ieee.E - 0x3ff;
        if (j0 <= 51) {
            if (j0 < 0) {
                // return 0 * sign(x) if |x| < 1
                if (u.SDW < 0) {
                    u.SDW = 0x8000000000000000ll;
                } else if (u.SDW != 0) {
                    u.SDW = 0x3ff0000000000000ll;
                }
            } else {
                u64 i = 0x000fffffffffffffll >> j0;
                if ((u.SDW & i) == 0) return x;  // x is integral
                if (u.SDW > 0) u.SDW += 0x0010000000000000ull >> j0;
                u.SDW &= (~i);
            }
        } else {
            if (j0 == 0x400) {
                return x + x;  // Inf or NaN
            } else {
                return x;  // x is integral
            }
        }
        return u.F;
    }

    // Returns the largest integer less than or equal to x.
    // It truncates toward -Inf.
    constexpr f64 floor(f64 x) {
        ieee754_f64 u = {x};

        u32 j0 = u.ieee.E - 0x3ff;
        if (j0 < 52) [[likely]] {
            if (j0 < 0) {
                // return 0 * sign(x) if |x| < 1
                if (u.SDW >= 0) {
                    u.SDW = 0;
                } else if ((u.SDW & 0x7fffffffffffffffl) != 0) {
                    u.SDW = 0xbff0000000000000l;
                }
            } else {
                u64 i = (0x000fffffffffffffl) >> j0;
                if ((u.SDW & i) == 0) return x;  // x is integral
                if (u.SDW < 0) u.SDW += (0x0010000000000000l) >> j0;
                u.SDW &= (~i);
            }
        } else if (j0 == 0x400) {
            // Inf or NaN
            return x + x;
        }
        return u.F;
    }

    // Returns the nearest integer to x
    constexpr f64 round(f64 x) {
        ieee754_f64 u = {x};

        u32 j0 = u.ieee.E - 0x3ff;
        if (j0 < 52) [[likely]] {
            if (j0 < 0) {
                u.SDW &= 0x8000000000000000ull;
                if (j0 == -1) {
                    u.SDW |= 0x3ff0000000000000ull;
                }
            } else {
                u64 i = 0x000fffffffffffffull >> j0;
                if ((u.SDW & i) == 0) return x;  // X is integral

                u.SDW += 0x0008000000000000ull >> j0;
                u.SDW &= ~i;
            }
        } else {
            if (j0 == 0x400) {
                return x + x;  // Inf or NaN.
            } else {
                return x;  // X is integral
            }
        }
        return u.F;
    }

    always_inline constexpr f32 ceil(f32 x) { return (f32) ceil((f64) x); }
    always_inline constexpr f32 floor(f32 x) { return (f32) floor((f64) x); }
    always_inline constexpr f32 round(f32 x) { return (f32) round((f64) x); }
}

LSTD_END_NAMESPACE
