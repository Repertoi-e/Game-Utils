#pragma once

//
// This header provides a way to hash basic types.
// If you have a custom type you want to implement a hash for
// (for example if you want to use it as a key in a Table)
// implement like this:
//
// GU_BEGIN_NAMESPACE
// template <>
// struct Hash<my_type> {
// 		static constexpr uptr_t get(...my_type... value) { return ...; }
// }
// GU_END_NAMESPACE
//

GU_BEGIN_NAMESPACE

template <typename T>
struct Hash {
    static constexpr uptr_t get(T const& value);
};

// Partial specialization for pointers
template <typename T>
struct Hash<T*> {
    static constexpr uptr_t get(T* pointer) { return (uptr_t) pointer; }
};

// _Hashes for integer types
#define TRIVIAL_HASH(T)                                                 \
    template <>                                                         \
    struct Hash<T> {                                                    \
        static constexpr uptr_t get(T value) { return (size_t) value; } \
    }

TRIVIAL_HASH(s8);
TRIVIAL_HASH(u8);

TRIVIAL_HASH(s16);
TRIVIAL_HASH(u16);

TRIVIAL_HASH(s32);
TRIVIAL_HASH(u32);

TRIVIAL_HASH(s64);
TRIVIAL_HASH(u64);

TRIVIAL_HASH(bool);

#undef TRIVIAL_HASH

// _Hashes for floats.
// The output of these depends on the endianness of the machine
// (Because we are reinterpreting the float's bits as unsinged numbers)
template <>
struct Hash<f32> {
    static constexpr uptr_t get(f32 value) {
        // Instead of a normal reinterpret cast we use an union, because the
        // former doesn't compile on clang
        union f32_u32 {
            f32 f;
            u32 u;

            constexpr f32_u32(f32 f) : f(f) {}
        };
        return Hash<u32>::get(f32_u32(value).u);
    }
};

template <>
struct Hash<f64> {
    static constexpr uptr_t get(f64 value) {
        // Instead of a normal reinterpret cast we use an union, because the
        // former doesn't compile on clang
        union f64_u64 {
            f64 f;
            u64 u;

            constexpr f64_u64(f64 f) : f(f) {}
        };
        return Hash<u64>::get(f64_u64(value).u);
    }
};

// Hash for strings
#include "../string/string.h"
template <>
struct Hash<string> {
    static constexpr uptr_t get(const string& str) {
        uptr_t hash = 5381;
        for (size_t i = 0; i < str.Length; i++) {
            hash = ((hash << 5) + hash) + str[i];
        }
        return hash;
    }
};

GU_END_NAMESPACE
