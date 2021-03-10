#include "../test.h"
#include "math.h"

TEST(vec_ctor) {
    vec<f32, 1, true> v1(10);

    assert_eq(v1.Data[0], 10);

    vec<f32, 2, true> v2(10);

    assert_eq(v2.Data[0], 10);
    assert_eq(v2.Data[1], 10);

    vec<f32, 3, true> v3(10);

    assert_eq(v3.Data[0], 10);
    assert_eq(v3.Data[1], 10);
    assert_eq(v3.Data[2], 10);

    vec<f32, 4, true> v4(10);

    assert_eq(v4.Data[0], 10);
    assert_eq(v4.Data[1], 10);
    assert_eq(v4.Data[2], 10);
    assert_eq(v4.Data[3], 10);

    vec<f32, 5, true> v5(10);

    assert_eq(v5.Data[0], 10);
    assert_eq(v5.Data[1], 10);
    assert_eq(v5.Data[2], 10);
    assert_eq(v5.Data[3], 10);
    assert_eq(v5.Data[4], 10);
}

TEST(ctor_array) {
    auto stack = to_stack_array(1.0, 2.0, 3.0, 4.0, 5.0);
    array_view<f64> data = stack;

    vec<f32, 1, true> v1(data);

    assert_eq(v1.Data[0], 1);

    vec<f32, 2, true> v2(data);

    assert_eq(v2.Data[0], 1);
    assert_eq(v2.Data[1], 2);

    vec<f32, 3, true> v3(data);

    assert_eq(v3.Data[0], 1);
    assert_eq(v3.Data[1], 2);
    assert_eq(v3.Data[2], 3);

    vec<f32, 4, true> v4(data);

    assert_eq(v4.Data[0], 1);
    assert_eq(v4.Data[1], 2);
    assert_eq(v4.Data[2], 3);
    assert_eq(v4.Data[3], 4);

    vec<f32, 5, true> v5(data);

    assert_eq(v5.Data[0], 1);
    assert_eq(v5.Data[1], 2);
    assert_eq(v5.Data[2], 3);
    assert_eq(v5.Data[3], 4);
    assert_eq(v5.Data[4], 5);
}

TEST(ctor_convert) {
    vec<f32, 1, true> v1 = vec<f64, 1, false>(1);

    assert_eq(v1.Data[0], 1);

    vec<f32, 2, true> v2 = vec<f64, 2, false>(1, 2);

    assert_eq(v2.Data[0], 1);
    assert_eq(v2.Data[1], 2);

    vec<f32, 3, true> v3 = vec<f64, 3, false>(1, 2, 3);

    assert_eq(v3.Data[0], 1);
    assert_eq(v3.Data[1], 2);
    assert_eq(v3.Data[2], 3);

    vec<f32, 4, true> v4 = vec<f64, 4, false>(1, 2, 3, 4);

    assert_eq(v4.Data[0], 1);
    assert_eq(v4.Data[1], 2);
    assert_eq(v4.Data[2], 3);
    assert_eq(v4.Data[3], 4);

    vec<f32, 5, true> v5 = vec<f64, 5, false>(1, 2, 3, 4, 5);

    assert_eq(v5.Data[0], 1);
    assert_eq(v5.Data[1], 2);
    assert_eq(v5.Data[2], 3);
    assert_eq(v5.Data[3], 4);
    assert_eq(v5.Data[4], 5);
}

TEST(ctor_scalar) {
    vec<f32, 1, true> v1(1);

    assert_eq(v1.Data[0], 1);

    vec<f32, 2, true> v2(1, 2);

    assert_eq(v2.Data[0], 1);
    assert_eq(v2.Data[1], 2);

    vec<f32, 3, true> v3(1, 2, 3);

    assert_eq(v3.Data[0], 1);
    assert_eq(v3.Data[1], 2);
    assert_eq(v3.Data[2], 3);

    vec<f32, 4, true> v4(1, 2, 3, 4);

    assert_eq(v4.Data[0], 1);
    assert_eq(v4.Data[1], 2);
    assert_eq(v4.Data[2], 3);
    assert_eq(v4.Data[3], 4);

    vec<f32, 5, true> v5(1, 2, 3, 4, 5);

    assert_eq(v5.Data[0], 1);
    assert_eq(v5.Data[1], 2);
    assert_eq(v5.Data[2], 3);
    assert_eq(v5.Data[3], 4);
    assert_eq(v5.Data[4], 5);
}

TEST(ctor_mixed) {
    vec<f64, 2> vd(1, 2);
    vec<f32, 2> vf(3, 4);

    vec<f32, 3, true> v3 = {vd, 3};

    assert_eq(v3.Data[0], 1);
    assert_eq(v3.Data[1], 2);
    assert_eq(v3.Data[2], 3);

    vec<f32, 4, true> v4 = {vd, vf};

    assert_eq(v4.Data[0], 1);
    assert_eq(v4.Data[1], 2);
    assert_eq(v4.Data[2], 3);
    assert_eq(v4.Data[3], 4);

    vec<f32, 5, true> v5 = {vd, 0, vf};

    assert_eq(v5.Data[0], 1);
    assert_eq(v5.Data[1], 2);
    assert_eq(v5.Data[2], 0);
    assert_eq(v5.Data[3], 3);
    assert_eq(v5.Data[4], 4);
}

TEST(ctor_mixed_swizzle) {
    vec<f64, 4> source(1, 2, 3, 4);

    vec<f32, 3, true> v3 = {source.wxy};

    assert_eq(v3.Data[0], 4);
    assert_eq(v3.Data[1], 1);
    assert_eq(v3.Data[2], 2);

    vec<f32, 5, true> v5 = {source.xy, 0, source.zw};

    assert_eq(v5.Data[0], 1);
    assert_eq(v5.Data[1], 2);
    assert_eq(v5.Data[2], 0);
    assert_eq(v5.Data[3], 3);
    assert_eq(v5.Data[4], 4);
}

TEST(cast) {
    vec<f32, 3, true> v3(0, 0, 0);
    vec<f32, 4, true> u3 = (vec<f32, 4, true>) v3;
    vec<f32, 3, true> d3 = (vec<f32, 3, true>) u3;

    assert_eq(u3, (vec<f32, 4, true>(0, 0, 0, 1)));
    assert_eq(v3, d3);

    vec<f32, 5, true> v5(0, 0, 0, 0, 0);
    vec<f32, 6, true> u5 = (vec<f32, 6, true>) v5;
    vec<f32, 5, true> d5 = (vec<f32, 5, true>) u5;

    assert_eq(u5, (vec<f32, 6, true>(0, 0, 0, 0, 0, 1)));
    assert_eq(v5, d5);
}

TEST(vec_index) {
    vec<f32, 4, true> source(0, 1, 2, 3);

    assert_eq(source[0], 0);
    assert_eq(source[1], 1);
    assert_eq(source[2], 2);
    assert_eq(source[3], 3);
}

TEST(vec_iterator) {
    vec<f32, 4, true> source(5, 6, 7, 8);

    f32 first = 5.0f;
    For(source) {
        assert_eq(it, first);
        first = first + 1.0f;
    }
}

TEST(swizzle) {
    auto v2 = vec<f32, 2, true>(1, 2);
    auto v3 = vec<f32, 3, true>(1, 2, 3);
    auto v4 = vec<f32, 4, true>(1, 2, 3, 4);

    assert_eq((vec<f32, 2, true>(v2.yx)), (vec<f32, 2, true>(2, 1)));
    assert_eq((vec<f32, 3, true>(v2.yxy)), (vec<f32, 3, true>(2, 1, 2)));
    assert_eq((vec<f32, 4, true>(v2.yxyx)), (vec<f32, 4, true>(2, 1, 2, 1)));

    assert_eq((vec<f32, 2, true>(v3.yz)), (vec<f32, 2, true>(2, 3)));
    assert_eq((vec<f32, 3, true>(v3.yzy)), (vec<f32, 3, true>(2, 3, 2)));
    assert_eq((vec<f32, 4, true>(v3.yzyx)), (vec<f32, 4, true>(2, 3, 2, 1)));

    assert_eq((vec<f32, 2, true>(v4.wz)), (vec<f32, 2, true>(4, 3)));
    assert_eq((vec<f32, 3, true>(v4.wzy)), (vec<f32, 3, true>(4, 3, 2)));
    assert_eq((vec<f32, 4, true>(v4.wzyx)), (vec<f32, 4, true>(4, 3, 2, 1)));
}

TEST(vec_add) {
    vecf<3> a3(1, 2, 3);
    vecf<3> b3(4, 5, 6);
    vecf<3> c3(5, 7, 9);

    assert_eq(a3 + b3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> b5(4, 5, 6, 7, 8);
    vecf<5> c5(5, 7, 9, 11, 13);

    assert_eq(a5 + b5, c5);
}

TEST(vec_subtract) {
    vecf<3> a3(1, 2, 3);
    vecf<3> b3(4, 5, 6);
    vecf<3> c3(-3, -3, -3);

    assert_eq(a3 - b3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> b5(4, 5, 6, 7, 8);
    vecf<5> c5(-3, -3, -3, -3, -3);

    assert_eq(a5 - b5, c5);
}

TEST(vec_multiply) {
    vecf<3> a3(1, 2, 3);
    vecf<3> b3(4, 5, 6);
    vecf<3> c3(4, 10, 18);

    assert_eq(a3 * b3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> b5(4, 5, 6, 7, 8);
    vecf<5> c5(4, 10, 18, 28, 40);

    assert_eq(a5 * b5, c5);
}

TEST(vec_divide) {
    vecf<3> a3(1, 2, 3);
    vecf<3> b3(4, 5, 6);
    vecf<3> c3(0.25f, 0.4f, 0.5f);

    assert_eq(a3 / b3, approx_vec(c3));

    vecf<5> a5(2, 6, 6, 12, 10);
    vecf<5> b5(1, 2, 3, 4, 5);
    vecf<5> c5(2, 3, 2, 3, 2);

    assert_eq(a5 / b5, approx_vec(c5));
}

TEST(vec_compound_add) {
    vecf<3> a3(1, 2, 3);
    vecf<3> b3(4, 5, 6);
    vecf<3> c3(5, 7, 9);
    a3 += b3;
    assert_eq(a3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> b5(4, 5, 6, 7, 8);
    vecf<5> c5(5, 7, 9, 11, 13);
    a5 += b5;
    assert_eq(a5, c5);
}

TEST(vec_compound_subtract) {
    vecf<3> a3(1, 2, 3);
    vecf<3> b3(4, 5, 6);
    vecf<3> c3(-3, -3, -3);
    a3 -= b3;
    assert_eq(a3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> b5(4, 5, 6, 7, 8);
    vecf<5> c5(-3, -3, -3, -3, -3);
    a5 -= b5;
    assert_eq(a5, c5);
}

TEST(vec_compound_multiply) {
    vecf<3> a3(1, 2, 3);
    vecf<3> b3(4, 5, 6);
    vecf<3> c3(4, 10, 18);
    a3 *= b3;
    assert_eq(a3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> b5(4, 5, 6, 7, 8);
    vecf<5> c5(4, 10, 18, 28, 40);
    a5 *= b5;
    assert_eq(a5, c5);
}

TEST(vec_compound_divide) {
    vecf<3> a3(1, 2, 3);
    vecf<3> b3(4, 5, 6);
    vecf<3> c3(0.25f, 0.4f, 0.5f);
    a3 /= b3;
    assert_eq(a3, approx_vec(c3));

    vecf<5> a5(2, 6, 6, 12, 10);
    vecf<5> b5(1, 2, 3, 4, 5);
    vecf<5> c5(2, 3, 2, 3, 2);
    a5 /= b5;
    assert_eq(a5, approx_vec(c5));
}

TEST(vec_scalar_add) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(5, 6, 7);

    assert_eq(a3 + b, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(5, 6, 7, 8, 9);

    assert_eq(a5 + b, c5);
}

TEST(vec_scalar_subtract) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(-3, -2, -1);

    assert_eq(a3 - b, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(-3, -2, -1, 0, 1);

    assert_eq(a5 - b, c5);
}

TEST(vec_scalar_multiply) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(4, 8, 12);

    assert_eq(a3 * b, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(4, 8, 12, 16, 20);

    assert_eq(a5 * b, c5);
}

TEST(vec_scalar_divide) {
    f32 b = 4;

    vecf<3> a3(4, 8, 12);
    vecf<3> c3(1, 2, 3);

    assert_eq(a3 / b, c3);

    vecf<5> a5(4, 8, 12, 16, 20);
    vecf<5> c5(1, 2, 3, 4, 5);

    assert_eq(a5 / b, c5);
}

TEST(vec_scalar_compound_add) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(5, 6, 7);
    a3 += b;
    assert_eq(a3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(5, 6, 7, 8, 9);
    a5 += b;
    assert_eq(a5, c5);
}

TEST(vec_scalar_compound_subtract) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(-3, -2, -1);
    a3 -= b;
    assert_eq(a3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(-3, -2, -1, 0, 1);
    a5 -= b;
    assert_eq(a5, c5);
}

TEST(vec_scalar_compound_multiply) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(4, 8, 12);
    a3 *= b;
    assert_eq(a3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(4, 8, 12, 16, 20);
    a5 *= b;
    assert_eq(a5, c5);
}

TEST(vec_scalar_compound_divide) {
    f32 b = 4;

    vecf<3> a3(4, 8, 12);
    vecf<3> c3(1, 2, 3);
    a3 /= b;
    assert_eq(a3, c3);

    vecf<5> a5(4, 8, 12, 16, 20);
    vecf<5> c5(1, 2, 3, 4, 5);
    a5 /= b;
    assert_eq(a5, c5);
}

TEST(vec_scalar_reverse_add) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(5, 6, 7);

    assert_eq(b + a3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(5, 6, 7, 8, 9);

    assert_eq(b + a5, c5);
}

TEST(vec_scalar_reverse_subtract) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(-3, -2, -1);

    assert_eq(b - a3, -c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(-3, -2, -1, 0, 1);

    assert_eq(b - a5, -c5);
}

TEST(vec_scalar_reverse_multiply) {
    f32 b = 4;

    vecf<3> a3(1, 2, 3);
    vecf<3> c3(4, 8, 12);

    assert_eq(b * a3, c3);

    vecf<5> a5(1, 2, 3, 4, 5);
    vecf<5> c5(4, 8, 12, 16, 20);

    assert_eq(b * a5, c5);
}

TEST(vec_scalar_reverse_divide) {
    f32 b = 4;

    vecf<3> a3(4, 8, 12);
    vecf<3> c3(1, 1.0 / 2.0, 1.0 / 3.0);

    assert_eq(b / a3, c3);

    vecf<5> a5(4, 8, 12, 16, 20);
    vecf<5> c5(1, 1.0f / 2.0, 1.0f / 3.0, 1.0f / 4.0, 1.0f / 5.0);

    assert_eq(b / a5, c5);
}

#define TEST_SWIZZLE_VECTOR_OP(NAME, OPERATOR) \
    TEST(swizzle_vector_##NAME) {              \
        vecf<3> v1(1, 2, 3);                   \
        vecf<3> v2(1, 4, -2);                  \
        vecf<3> r = v1.xyz OPERATOR v2;        \
        vecf<3> e = v1 OPERATOR v2;            \
        assert_eq(r, e);                       \
    }

#define TEST_VECTOR_SWIZZLE_OP(NAME, OPERATOR) \
    TEST(vector_swizzle_##NAME) {              \
        vecf<3> v1(1, 2, 3);                   \
        vecf<3> v2(1, 4, -2);                  \
        vecf<3> r = v1 OPERATOR v2.xyz;        \
        vecf<3> e = v1 OPERATOR v2;            \
        assert_eq(r, e);                       \
    }

#define TEST_VECTOR_SWIZZLE_COMPOUND_OP(NAME, OPERATOR) \
    TEST(swizzle_vector_##NAME) {                       \
        vecf<3> v1(1, 2, 3);                            \
        auto v1c = v1;                                  \
        vecf<3> v2(1, 4, -2);                           \
        v1 OPERATOR v2.xyz;                             \
        v1c OPERATOR v2;                                \
        assert_eq(v1, v1c);                             \
    }

#define TEST_SWIZZLE_VECTOR_COMPOUND_OP(NAME, OPERATOR) \
    TEST(vector_swizzle_##NAME) {                       \
        vecf<3> v1(1, 2, 3);                            \
        auto v1c = v1;                                  \
        vecf<3> v2(1, 4, -2);                           \
        v1.xyz OPERATOR v2;                             \
        v1c OPERATOR v2;                                \
        assert_eq(v1, v1c);                             \
    }

#define TEST_SWIZZLE_SWIZZLE_OP(NAME, OPERATOR) \
    TEST(swizzle_swizzle_##NAME) {              \
        vecf<3> v1(1, 2, 3);                    \
        vecf<3> v2(1, 4, -2);                   \
        vecf<3> r = v1.xyz OPERATOR v2.xyz;     \
        vecf<3> e = v1 OPERATOR v2;             \
        assert_eq(r, e);                        \
    }

#define TEST_SWIZZLE_SWIZZLE_COMPOUND_OP(NAME, OPERATOR) \
    TEST(swizzle_swizzle_##NAME) {                       \
        vecf<3> v1(1, 2, 3);                             \
        auto v1c = v1;                                   \
        vecf<3> v2(1, 4, -2);                            \
        v1.xyz OPERATOR v2.xyz;                          \
        v1c OPERATOR v2;                                 \
        assert_eq(v1, v1c);                              \
    }

#define TEST_SWIZZLE_SCALAR_COMPOUND_OP(NAME, OPERATOR) \
    TEST(swizzle_scalar_##NAME) {                       \
        vecf<3> v1(1, 2, 3);                            \
        auto v1c = v1;                                  \
        f32 b = 6;                                      \
        v1.xyz OPERATOR b;                              \
        v1c OPERATOR b;                                 \
        assert_eq(v1, v1c);                             \
    }

#define TEST_SWIZZLE_SCALAR_OP(NAME, OPERATOR) \
    TEST(swizzle_scalar_##NAME) {              \
        vecf<3> v1(1, 2, 3);                   \
        f32 b = 6;                             \
        vecf<3> r = v1.xyz OPERATOR b;         \
        vecf<3> e = v1 OPERATOR b;             \
        assert_eq(r, e);                       \
    }

#define TEST_SCALAR_SWIZZLE_OP(NAME, OPERATOR) \
    TEST(scalar_swizzle_##NAME) {              \
        vecf<3> v1(1, 2, 3);                   \
        f32 b = 6;                             \
        vecf<3> r = b OPERATOR v1.xyz;         \
        vecf<3> e = b OPERATOR v1;             \
        assert_eq(r, e);                       \
    }

// A note to tell our script to do special work because of the macros we use.
//
// :build_tests: swizzle_vector_##NAME  -> swizzle_vector_add swizzle_vector_subtract swizzle_vector_multiply swizzle_vector_divide swizzle_vector_compound_add swizzle_vector_compound_subtract swizzle_vector_compound_multiply swizzle_vector_compound_divide
// :build_tests: vector_swizzle_##NAME -> vector_swizzle_add vector_swizzle_subtract vector_swizzle_multiply vector_swizzle_divide vector_swizzle_compound_add vector_swizzle_compound_subtract vector_swizzle_compound_multiply vector_swizzle_compound_divide
//

TEST_SWIZZLE_VECTOR_OP(add, +)
TEST_SWIZZLE_VECTOR_OP(subtract, -)
TEST_SWIZZLE_VECTOR_OP(multiply, *)
TEST_SWIZZLE_VECTOR_OP(divide, /)

TEST_VECTOR_SWIZZLE_OP(add, +)
TEST_VECTOR_SWIZZLE_OP(subtract, -)
TEST_VECTOR_SWIZZLE_OP(multiply, *)
TEST_VECTOR_SWIZZLE_OP(divide, /)

TEST_VECTOR_SWIZZLE_COMPOUND_OP(compound_add, +=)
TEST_VECTOR_SWIZZLE_COMPOUND_OP(compound_subtract, -=)
TEST_VECTOR_SWIZZLE_COMPOUND_OP(compound_multiply, *=)
TEST_VECTOR_SWIZZLE_COMPOUND_OP(compound_divide, /=)

TEST_SWIZZLE_VECTOR_COMPOUND_OP(compound_add, +=)
TEST_SWIZZLE_VECTOR_COMPOUND_OP(compound_subtract, -=)
TEST_SWIZZLE_VECTOR_COMPOUND_OP(compound_multiply, *=)
TEST_SWIZZLE_VECTOR_COMPOUND_OP(compound_divide, /=)

// :build_tests: swizzle_swizzle_##NAME  -> swizzle_swizzle_add swizzle_swizzle_subtract swizzle_swizzle_multiply swizzle_swizzle_divide swizzle_swizzle_compound_add swizzle_swizzle_compound_subtract swizzle_swizzle_compound_multiply swizzle_swizzle_compound_divide

TEST_SWIZZLE_SWIZZLE_OP(add, +)
TEST_SWIZZLE_SWIZZLE_OP(subtract, -)
TEST_SWIZZLE_SWIZZLE_OP(multiply, *)
TEST_SWIZZLE_SWIZZLE_OP(divide, /)

TEST_SWIZZLE_SWIZZLE_COMPOUND_OP(compound_add, +=)
TEST_SWIZZLE_SWIZZLE_COMPOUND_OP(compound_subtract, -=)
TEST_SWIZZLE_SWIZZLE_COMPOUND_OP(compound_multiply, *=)
TEST_SWIZZLE_SWIZZLE_COMPOUND_OP(compound_divide, /=)

// :build_tests: swizzle_scalar_##NAME  -> swizzle_scalar_add swizzle_scalar_subtract swizzle_scalar_multiply swizzle_scalar_divide swizzle_scalar_compound_add swizzle_scalar_compound_subtract swizzle_scalar_compound_multiply swizzle_scalar_compound_divide

// :build_tests: scalar_swizzle_##NAME  -> scalar_swizzle_add scalar_swizzle_subtract scalar_swizzle_multiply scalar_swizzle_divide

TEST_SWIZZLE_SCALAR_OP(add, +)
TEST_SWIZZLE_SCALAR_OP(subtract, -)
TEST_SWIZZLE_SCALAR_OP(multiply, *)
TEST_SWIZZLE_SCALAR_OP(divide, /)

TEST_SCALAR_SWIZZLE_OP(add, +)
TEST_SCALAR_SWIZZLE_OP(subtract, -)
TEST_SCALAR_SWIZZLE_OP(multiply, *)
TEST_SCALAR_SWIZZLE_OP(divide, /)

TEST_SWIZZLE_SCALAR_COMPOUND_OP(compound_add, +=)
TEST_SWIZZLE_SCALAR_COMPOUND_OP(compound_subtract, -=)
TEST_SWIZZLE_SCALAR_COMPOUND_OP(compound_multiply, *=)
TEST_SWIZZLE_SCALAR_COMPOUND_OP(compound_divide, /=)

TEST(is_null_vec) {
    vecf<3> a(1, 2, 3);
    assert_eq(is_null_vector(a), false);
    vecf<3> b(0, 0, 0);
    assert_eq(is_null_vector(b), true);
}

TEST(length) {
    vecf<3> a(1, 2, 3);
    assert_eq(len(a), approx(3.7416573867));

    vecf<5> b(1, 0, 2, 0, 3);
    assert_eq(len(b), approx(3.7416573867));
}

TEST(length_precise) {
    vecf<3> a(1e-38f, 2e-38f, 3e-38f);
    assert_eq(len_precise(a), approx(3.7416573867e-38f));

    vecf<5> b(1e+37f, 0, 2e+37f, 0, 3e+37f);
    assert_eq(len_precise(b), approx(3.7416573867e+37f));
}

TEST(normalize) {
    vecf<3> a(1, 2, 3);
    a = normalize(a);
    assert_eq(len(a), approx(1));
    assert_eq(2 * a[0], approx(a[1]));
    assert_eq(3 * a[0], approx(a[2]));

    vecf<5> b(1, 0, 2, 0, 3);
    b = normalize(b);
    assert_eq(len(b), approx(1));
    assert_eq(2 * b[0], approx(b[2]));
    assert_eq(3 * b[0], approx(b[4]));
}

TEST(safe_normalize_denom) {
    vecf<3> a(0, 1e-40, 0);
    a = safe_normalize(a);
    assert_eq(len(a), approx(1));
    assert_eq(a[1], approx(1));

    vecf<5> b(0, 0, 1e-40, 0, 0);
    b = safe_normalize(b);
    assert_eq(len(b), approx(1));
    assert_eq(b[2], approx(1));
}

TEST(safe_normalize_null) {
    vecf<3> a(0, 0, 0);
    a = safe_normalize(a);
    assert_eq(len(a), approx(1));
    assert_eq(a[0], approx(1));

    vecf<5> b(0, 0, 0, 0, 0);
    b = safe_normalize(b);
    assert_eq(len(b), approx(1));
    assert_eq(a[0], approx(1));
}

TEST(safe_normalize_specific_proper) {
    vecf<3> a(1, 2, 3);
    assert_eq(approx_vec(normalize(a)), safe_normalize(a, vecf<3>(0, 1, 0)));

    vecf<5> b(1, 0, 2, 0, 3);
    assert_eq(approx_vec(normalize(b)), safe_normalize(b, vecf<5>(0, 1, 0, 0, 0)));
}

TEST(safe_normalize_specific_null) {
    vecf<3> a(0, 0, 0);
    a = safe_normalize(a, vecf<3>(0, 1, 0));
    assert_eq(len(a), approx(1));
    assert_eq(a[1], approx(1));

    vecf<5> b(0, 0, 0, 0, 0);
    b = safe_normalize(b, vecf<5>(0, 1, 0, 0, 0));
    assert_eq(len(b), approx(1));
    assert_eq(a[1], approx(1));
}

TEST(fill_vec) {
    vecf<3> a ;
    vecf<3> b(4);
    fill(a, 4);

    assert_eq(a, b);

    vecf<5> c ;
    vecf<5> d(4);
    fill(c, 4);

    assert_eq(c, d);
}

TEST(element_wise_min_max) {
    vecf<3> a(1, 2, 3);
    vecf<3> b(3, 2, 1);

    assert_eq(min(a, b), vecf<3>(1, 2, 1));
    assert_eq(max(a, b), vecf<3>(3, 2, 3));

    vecf<5> c(1, 2, 3, 4, 5);
    vecf<5> d(5, 4, 3, 2, 1);

    assert_eq(min(c, d), vecf<5>(1, 2, 3, 2, 1));
    assert_eq(max(c, d), vecf<5>(5, 4, 3, 4, 5));
}

TEST(dot) {
    vecf<3> a(1, 2, 3);
    vecf<3> b(4, 5, 6);
    auto r1 = dot(a, b);

    assert_eq(r1, 32);

    vecf<5> c(1, 2, 3, 2, 1);
    vecf<5> d(4, 5, 6, 5, 4);
    auto r2 = dot(c, d);
    assert_eq(r2, 46);
}

TEST(cross) {
    vecf<3> a(1, 2, 3);
    vecf<3> b(4, 5, 6);
    vecf<3> r = cross(a, b);
    vecf<3> rexp(-3, 6, -3);

    assert_eq(r, rexp);
}

TEST(cross_nd) {
    vecf<3> a(1, 2, 3);
    vecf<3> b(4, 5, 6);
    vecf<3> r = cross(a, b);
    vecf<3> rexp(-3, 6, -3);

    assert_eq(r, rexp);

    vecf<2> a2(1, 2);
    vecf<2> r2 = cross(a2);
    vecf<2> r2exp(-2, 1);

    assert_eq(approx_vec(r2), r2exp);

    vecf<4> a4(1, 2, 3, 4);
    vecf<4> b4(4, 2, 6, 3);
    vecf<4> c4(3, 6, 4, -9);
    vecf<4> r4 = cross(a4, b4, c4);

    auto d = lstd::abs(dot(a4, r4)) + lstd::abs(dot(b4, r4)) + lstd::abs(dot(c4, r4));
    assert_lt(d, 1e-5f);
}
