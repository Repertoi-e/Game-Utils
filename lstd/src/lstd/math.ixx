export module math;

//
// This file defines common functions that work on scalars (integrals or floating point types).
//
// Includes functions that normally require the C runtime library. There is a point to be made on why that's not very good -
// different platforms have different implementations of the standard library, that means that your sin function suddenly changes
// when you switch from Windows to Linux... We implement our own functions in order to not rely on the runtime library at all,
// but this also means that you have one implementation for all platforms.
//
// !!!
// The implementations of some functions are based on the Cephes library.
// The changes are documented in the source files of each function respectively.
//
// Cephes Math Library Release 2.8: June, 2000
// Copyright 1985, 1995, 2000 by Stephen L. Moshier
// !!!
//
//
// Here is the list of functions included by this header:
//
// *return type* | *name* (equivalent in CRT - if it exists) | -> *note*
// --------------|-------------------------------------------|-----------------
// bool                    sign_bit            (signbit)       -> works for both integers and floats
// s32                     sign_no_zero                        -> returns -1 if x is negative, 1 otherwise
// s32                     sign                                -> returns -1 if x is negative, 1 if positive, 0 otherwise
//
// f64                     copy_sign                           -> copies the sign of y to x; only portable way to manipulate NaNs
//
// bool                    is_nan,             (isnan)
// bool                    is_signaling_nan
// bool                    is_infinite,        (isinf)
// bool                    is_finite,          (isfinite)      -> !is_infinite(x) && !is_nan(x)
//
// T                       min *                               -> also supports variable number of arguments
// T                       max *                               -> also supports variable number of arguments
// T                       clamp *                             -> clamps x between two values
// T                       abs *
//
// bool                    is_pow_of_2                         -> checks if x is a power of 2
// T                       ceil_pow_of_2                       -> returns the smallest power of 2 bigger or equal to x
//
// T                       const_exp10                         -> evaluates 10 ** exp at compile-time, using recursion
//
// f64                     pow *
// f64                     sqrt *
// f64                     ln (log)                            -> returns the natural logarithm of x
//                         ^^^^^^^^ Function is called ln instead of just log, because that name
//                                  is more logical to me (and follows math notation).
//
// f64                     log2 *
// f64                     log10 *
// f64                     sin *
// f64                     cos *
// f64                     asin *
// f64                     acos *
//
// f64                     ceil *
// f64                     floor *
// f64                     round *
//
// decompose_float_result  fraction_exponent                   -> decomposes a float into a normalized fraction and an integer exponent
// f64                     load_exponent                       -> multiplies a float by 2 ** exp
//                         ^^^^^^^^^^^^^^^^^
//                         These two functions are inverses of each other and are used to modify floats without messing with the bits.
//
//
// * These functions share names with the C++ standard library.
//   We are using a namespace by default so this should not cause unresolvable name conflicts.
//
//
// THIS IS NOT A COMPLETE REPLACEMENT FOR ALL FUNCTIONS IN THE STANDARD LIBRARY.
// When we need a certain function, we implement is as we go.
// If we attempt to make a super general math library that has all the functions in the world, we will never complete it.
// This also forces us to think which functions are *very* useful and get used a lot and which are just boilerplate that
// may get used once in one remote corner in a program in a very specific case.
//

//
// Note: A quick reminder on how IEEE 754 works.
//
// Here is how a 32 bit float looks in memory:
//
//      31
//      |
//      | 30    23 22                    0
//      | |      | |                     |
// -----+-+------+-+---------------------+
// qnan 0 11111111 10000000000000000000000
// snan 0 11111111 01000000000000000000000
//  inf 0 11111111 00000000000000000000000
// -inf 1 11111111 00000000000000000000000
// -----+-+------+-+---------------------+
//      | |      | |                     |
//      | +------+ +---------------------+
//      |    |               |
//      |    v               v
//      | exponent        mantissa
//      |
//      v
//      sign
//
// Inf is defined to be any x with exponent == 0xFF and mantissa == 0
// NaN is defined to be any x with exponent == 0xFF and mantissa != 0
//
// "SNaNs (signaling NaNs) are typically used to trap or invoke an exception handler.
//  They must be inserted by software; that is, the processor never generates an SNaN as a result of a floating-point operation."
//                                                          - Intel manual, 4.8.3.4
//
// QNans (quiet NaNs) and SNanS are differentiated by the 22th bit. For this particular example the 23th bit
// for sNaN is set because otherwise the mantissa would be 0 (and that is interpreted as infinity by definition).
//

export import math.basic;
export import math.constants;
export import math.pow_log;
export import math.sin_cos;
export import math.ceil_floor_round;
export import math.frexp_ldexp;
