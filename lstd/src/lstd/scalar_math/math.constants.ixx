module;

#include "../internal/floating_point.h"

export module math.constants;

LSTD_BEGIN_NAMESPACE

// Doubles have ~15 digits of precision, here we provide 20 - it should get rounded automatically.
// This should be way more than enough for normal cases.
//
// @TODO: In the future maybe we should look into arbitrary precision math?
// i.e. big integers and no limit on real number precision. It would be very useful e.g. for calculators/graphing.
// I don't think there are good C++ libraries that do that, so we will maybe interface with a C one, instead of porting from zero.
constexpr f64 TAU = (f64) 6.28318530717958647692;
constexpr f64 PI = (f64) 3.14159265358979323846;
constexpr f64 EULER = (f64) 2.71828182845904523536;

constexpr f64 SQRT2 = (f64) 1.41421356237309504880;
constexpr f64 ISQRT2 = (f64) 0.70710678118654752440;  // sqrt(2) / 2 == 1 / sqrt(2)

constexpr f64 LN2 = (f64) 0.693147180559945309417;    //  ln(2)
constexpr f64 ILN2 = (f64) 1.4426950408889634073599;  //  1 / ln(2)

constexpr f64 LOG2_E_MINUS_1 = (f64) 0.44269504088896340736;  // log_2(e) - 1

LSTD_END_NAMESPACE
