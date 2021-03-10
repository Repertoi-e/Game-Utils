#pragma once

/// A header which provides type definitions as well as other helper macros

#include <intrin.h>

#include "debug_break.h"
#include "static_for.h"

LSTD_BEGIN_NAMESPACE

// Use this to get the location where a function was called without using macros.
// Uses built-in compiler functions. @TODO: Works on MSVC. Should work on GNU and Clang?
//
// e.g. useful for tracing where allocations came from:
//
// void my_allocate(s64 size, source_location loc = source_location::current()) { ... }
//
struct source_location {
    const char *File = "Unknown";
    const char *Function = "Unknown";
    s64 Line = 0;

    constexpr source_location() {}

    static constexpr source_location current(const char *file = __builtin_FILE(), const char *func = __builtin_FUNCTION(), s64 line = __builtin_LINE()) {
        source_location loc;
        loc.File = file;
        loc.Function = func;
        loc.Line = line;
        return loc;
    }
};

// Convenience storage literal operators, allows for specifying sizes like this:
//  s64 a = 10_MiB;

// _B For completeness
constexpr u64 operator"" _B(u64 i) { return i; }
constexpr u64 operator"" _KiB(u64 i) { return i << 10; }
constexpr u64 operator"" _MiB(u64 i) { return i << 20; }
constexpr u64 operator"" _GiB(u64 i) { return i << 30; }

constexpr u64 operator"" _thousand(u64 i) { return i * 1000; }
constexpr u64 operator"" _million(u64 i) { return i * 1000000; }
constexpr u64 operator"" _billion(u64 i) { return i * 1000000000; }

// Helper macro for, e.g flag enums
//
// enum flags {
//	Flag_1 = BIT(0),
//  Flag_1 = BIT(1)
//  Flag_1 = BIT(2)
//  ...
// };
//
#define BIT(x) (1 << (x))

// Used in macros to get "unique" variable names
#define LINE_NAME(name) _MACRO_CONCAT(name, __LINE__)
#define _MACRO_DO_CONCAT(s1, s2) s1##s2
#define _MACRO_CONCAT(s1, s2) _MACRO_DO_CONCAT(s1, s2)

// Go-style defer
//
//  defer(...);
//  defer({
//      ...;
//  });
//
// The statements inside get called on scope exit
//
#undef defer

struct Defer_Dummy {};
template <typename F>
struct Deferrer {
    F Func;
    ~Deferrer() { Func(); }
};
template <typename F>
Deferrer<F> operator*(Defer_Dummy, F func) {
    return {func};
}

#define defer(x) auto LINE_NAME(LSTD_defer) = Defer_Dummy{} * [&]() { x; }

#undef assert

#if not defined NDEBUG
#define assert(condition) (!!(condition)) ? (void) 0 : debug_break()
#else
#define assert(condition) ((void) 0)
#endif

template <typename T, typename TIter = decltype(types::declval<T>().begin()), typename = decltype(types::declval<T>().end())>
constexpr auto enumerate_impl(T &&in) {
    struct iterator {
        s64 I;
        TIter Iter;

        bool operator!=(const iterator &other) const { return Iter != other.Iter; }
        void operator++() { ++I, ++Iter; }

        struct dereference_result {
            s64 Index;
            decltype(*types::declval<TIter>()) Value;
        };

        auto operator*() const {
            return dereference_result{I, *Iter};
        }
    };

    struct iterable_wrapper {
        T Iterable;

        auto begin() { return iterator{0, Iterable.begin()}; }
        auto end() { return iterator{0, Iterable.end()}; }
    };

    return iterable_wrapper{(T &&) in};
}

// Shortcut macros for "for each" loops (really up to personal style if you want to use this)
//
//  For(array) print(it);
//
#define For_as(x, in) for (auto &&x : in)
#define For(in) For_as(it, in)

//
// Inspired from Python's enumerate().
// Example usage:
//
//    For_enumerate(data) {
//        other_data[it_index] = it + 1;
//    }
//
// .. which is the same as:
//
//    For(range(data.Count)) {
//        other_data[it] = data[it] + 1;
//    }
//
// Might not look much shorter but you don't a separate
// variable if you use data[it] more than once.
// It's just a convenience.
//
// You can change the names of the internal
// variables by using _For_enumerate_as_.
//
#define For_enumerate_as(it_index, it, in) for (auto [it_index, it] : enumerate_impl(in))
#define For_enumerate(in) For_enumerate_as(it_index, it, in)

// Base classes to reduce boiler plate code
struct non_copyable {
   protected:
    non_copyable() {}
    ~non_copyable() {}

   private:
    non_copyable(const non_copyable &) = delete;
    non_copyable &operator=(const non_copyable &) = delete;
};

struct non_movable {
   protected:
    non_movable() {}
    ~non_movable() {}

   private:
    non_movable(non_movable &&) = delete;
    non_movable &operator=(non_movable &&) = delete;
};

struct non_assignable {
   private:
    non_assignable &operator=(const non_assignable &) = delete;
    non_assignable &operator=(non_assignable &&) = delete;
};

// Python-like range functionality
// e.g.
//
//  for (auto it : range(20))        // [0, 20)
//  for (auto it : range(3, 10, 2))  // every second integer (step 2) in [3, 10)
//  for (auto it : range(10, 0, -1)) // reverse [10, 0)
//
// .. or with our For macro:
//
//  For(range(12)) {}
//
//    which is equivalent to:
//
//  For_as(it, range(12)) {}
//
struct range {
    struct iterator {
        s64 I, Step;

        constexpr iterator(s64 i, s64 step = 1) : I(i), Step(step) {}

        operator s32() const { return (s32) I; }
        operator s64() const { return I; }

        constexpr s64 operator*() const { return I; }
        constexpr iterator operator++() { return I += Step, *this; }

        constexpr iterator operator++(int) {
            iterator temp(*this);
            return I += Step, temp;
        }

        constexpr bool operator==(iterator other) const { return Step < 0 ? (I <= other.I) : (I >= other.I); }
        constexpr bool operator!=(iterator other) const { return Step < 0 ? (I > other.I) : (I < other.I); }
    };

    iterator Begin;
    iterator End;

    constexpr range(s64 start, s64 stop, s64 step) : Begin(start, step), End(stop) {}
    constexpr range(s64 start, s64 stop) : range(start, stop, 1) {}
    constexpr range(u64 stop) : range(0, stop, 1) {}

    // Checks if a value is inside the given range.
    // This also accounts for stepping.
    constexpr bool has(s64 value) const {
        if (Begin.Step > 0 ? (value >= Begin.I && value < End.I) : (value > End.I && value <= Begin.I)) {
            s64 diff = value - Begin.I;
            if (diff % Begin.Step == 0) {
                return true;
            }
        }
        return false;
    }

    constexpr iterator begin() const { return Begin; }
    constexpr iterator end() const { return End; }
};

// @Volatile: README.md
// Type policy:
//
// Aim of this policy:
// - Dramatically reduce complexity and code size (both library AND user side!) UNLESS that comes at a run-time cost
//
// - Always provide a default constructor (implicit or by "T() {}")
// - Every data member (which makes sense) should be public. Do not write useless getters/setters.
// - Strive to make classes/structures/objects (whatever you wanna call them) data oriented.
//   Programs work with data. Design your data so it makes the solution straightforward and minimize abstraction layers.
// - No user defined copy/move constructors.
// - No throwing of exceptions, .. ever, .. anywhere. No excuse.
//
// "No user defined copy/move constructors":
//   This may sound crazy if you have a type that owns memory. How would you deep copy the contents and not just the pointer when you copy the object?
//   How do you handle dangling pointers to unfreed memory of shallow copies when the original destructor fires?
//
//   _string_ is this library implemented the following way:
//     _string_ is a struct that contains a pointer to a byte buffer and 2 fields containing pre-calculated
//     utf8 code unit and code point lengths, as well as a field _Reserved_ that contains the number of
//     bytes allocated by that string (default is 0).
//
//     _string_ contains constexpr methods which deal with string manipulation (all methods which make sense and don't modify the string can be used compile-time).
//     This works because you can construct a string as a "view" with a c-style string literal.
//     constexpr functions include substrings, trimming (these work because, again, we don't do zero terminated strings, but instead a pointer and a size), searching for
//     strings or code points, etc. All operations are implemented with utf8 in mind.
//
//     We implement string length methods and comparing (lexicographically and code point by code point) for c-style strings in "string_utils.h" (included by "string.h").
//     These still work with utf8 in mind but assume the string is zero terminated.
//
//     All operations with indices on strings support negative indexing (python-style). So "-1" is translated to "str.Length - 1".
//
//     A string has either allocated memory or it has not. We make this very clear when returning strings from functions in the library.
//     If the procedure is marked as [[nodiscard]] that means that the returned string should be freed.
//
//     The object is designed with no ownership in mind. That is determined explictly in code and by the programmer.
//     _string_'s destructor is EMPTY. It doesn't free any buffers. The user is responsible for freeing the string when that is required.
//     We make this easier by providing a defer macro which runs at the end of the scope (like a destructor).
//
//     All of this allows us to skip writing copy/move constructors and assignment operators, we avoid unnecessary copies by making the programmer think harder
//     about managing the memory, while being explicit and concise, so you know what your program is doing.
//
//            string path = "./data/";      // Constructed from a zero-terminated string buffer. Doesn't allocate memory.
//
//            // _string_ includes constexpr methods but also methods which cannot be constexpr and allocate memory.
//            // It's like a mixed type between std::string_view and std::string from the STL, but with way better design and API.
//            // When a string needs to allocate memory it requests a buffer and copies the old contents of the string.
//            path.append("output.txt");
//
//            // This doesn't allocate memory but it points to the buffer in _path_.
//            // The substring is valid as long as the original string is valid.
//            string pathWithoutDot = path.substring(2, -1);
//
//            // Doesn't release the string here, but instead runs at scope exit.
//            // It runs exactly like a destructor, but it's explicit and not hidden.
//            // This style of programming makes you write code which doesn't allocate
//            // strings superfluously and doesn't rely on C++ compiler optimization
//            // (like "copy elision" when returning strings from functions).
//            defer(path.release());
//
//     String methods which allocate memory copy the contents of the old pointer if the string is still a view (hasn't yet allocated memory).
//
//     _clone(T *dest, T src)_ is a global function that ensures a deep copy of the argument passed.
//     Objects that own memory (like string) overload clone() and make sure the copy reserves a buffer and copies the data to it.
//     This is like a copy constructor but much cleaner.
//
//     ! Note: _clone_ works on all types (unless overloaded the default implementation does a shallow copy).
//     It is this library's recommended way to implement functionality normally done in copy c-tors.
//
// "No throwing of exceptions, anywhere"
//   Exceptions make your code complicated. They are a good way to handle errors in small examples,
//   but don't really help in large programs/projects. You can't be 100% sure what can throw where and when
//   thus you don't really know what your program is doing (you aren't sure it even works 100% of the time).
//   You should design code in such a way that errors can't occur (or if they do - handle them, not just bail,
//   and when even that is not possible - stop execution).
//

// Global function that is supposed to ensure a deep copy of the argument passed
// By default, a shallow copy is done (to make sure it can be called on all types)
template <typename T>
T *clone(T *dest, T src) {
    *dest = src;
    return dest;
}

template <typename T>
constexpr void swap(T &a, T &b) {
    T c = a;
    a = b;
    b = c;
}

template <typename T, s64 N>
constexpr void swap(T (&a)[N], T (&b)[N]) {
    For(range(N)) swap(a[it], b[it]);
}

//
// copy_memory, fill_memory, compare_memory and SSE optimized implementations when on x86 architecture
// (implemenations in memory/memory.cpp)
//

// In this library, copy_memory works like memmove in the std (handles overlapping buffers)
// :CopyMemory (declared in types.h also to avoid circular includes)
extern void *(*copy_memory)(void *dst, const void *src, u64 size);
constexpr void *const_copy_memory(void *dst, const void *src, u64 size) {
    auto *d = (char *) dst;
    auto *s = (const char *) src;

    if (d <= s || d >= (s + size)) {
        // Non-overlapping
        while (size--) {
            *d++ = *s++;
        }
    } else {
        // Overlapping
        d += size - 1;
        s += size - 1;

        while (size--) {
            *d-- = *s--;
        }
    }
    return dst;
}

extern void *(*fill_memory)(void *dst, char value, u64 size);
constexpr void *const_fill_memory(void *dst, char value, u64 size) {
    auto d = (char *) dst;
    while (size-- > 0) *d++ = value;
    return dst;
}

inline void *zero_memory(void *dst, u64 size) { return fill_memory(dst, 0, size); }
constexpr void *const_zero_memory(void *dst, u64 size) { return const_fill_memory(dst, 0, size); }

// compare_memory returns the index of the first byte that is different
// e.g: calling with
//		*ptr1 = 00000011
//		*ptr1 = 00100001
//	returns 2
// If the memory regions are equal, the returned value is -1
extern s64 (*compare_memory)(const void *ptr1, const void *ptr2, u64 size);
constexpr s64 const_compare_memory(const void *ptr1, const void *ptr2, u64 size) {
    auto *s1 = (const char *) ptr1;
    auto *s2 = (const char *) ptr2;

    For(range(size)) if (*s1++ != *s2++) return it;
    return -1;
}

#if COMPILER == MSVC
#pragma intrinsic(_BitScanReverse)
inline u32 msb(u32 x) {
    assert(x != 0);

    unsigned long r = 0;
    _BitScanReverse(&r, x);
    return 31 - r;
}

#pragma intrinsic(_BitScanReverse64)
inline u32 msb_64(u64 x) {
    assert(x != 0);

    unsigned long r = 0;
    _BitScanReverse64(&r, x);
    return 63 - r;
}

#pragma intrinsic(_BitScanForward)
inline u32 lsb(u32 x) {
    assert(x != 0);

    unsigned long r = 0;
    _BitScanForward(&r, x);
    return r;
}

#pragma intrinsic(_BitScanForward64)
inline u32 lsb_64(u64 x) {
    assert(x != 0);

    unsigned long r = 0;
    _BitScanForward64(&r, x);
    return r;
}
#else
#define msb(n) __builtin_clz(n)
#define msb_64(n) __builtin_clzll(n)
#define lsb(n) __builtin_ctz(n)
#define lsb_64(n) __builtin_ctzll(n)
#endif

constexpr u32 rotate_left_32(u32 x, u32 bits) { return (x << bits) | (x >> (32 - bits)); }
constexpr u64 rotate_left_64(u64 x, u32 bits) { return (x << bits) | (x >> (64 - bits)); }

constexpr u32 rotate_right_32(u32 x, u32 bits) { return (x >> bits) | (x << (32 - bits)); }
constexpr u64 rotate_right_64(u64 x, u32 bits) { return (x >> bits) | (x << (64 - bits)); }

//
// Useful: http://graphics.stanford.edu/~seander/bithacks.html#CopyIntegerSign
//

// Uses 4 operations:
#define U32_HAS_ZERO_BYTE(v) (((v) -0x01010101UL) & ~(v) &0x80808080UL)

// Uses 5 operations when n is constant:
#define U32_HAS_BYTE(x, n) (U32_HAS_ZERO_BYTE((x) ^ (~0UL / 255 * (u8)(n))))

// Uses 4 operations when n is constant:
#define U32_HAS_BYTE_LESS_THAN(x, n) (((x) - ~0UL / 255 * (u8)(n)) & ~(x) & ~0UL / 255 * 128)

// Uses 7 operations when n is constant:
#define U32_COUNT_BYTES_LESS_THAN(x, n) (((~0UL / 255 * (127 + (n)) - ((x) & ~0UL / 255 * 127)) & ~(x) & ~0UL / 255 * 128) / 128 % 255)

// Uses 3 operations when n is constant:
#define U32_HAS_BYTE_GREATER_THAN(x, n) (((x) + ~0UL / 255 * (127 - (u8)(n)) | (x)) & ~0UL / 255 * 128)

// Uses 6 operations when n is constant:
#define U32_COUNT_BYTES_GREATER_THAN(x, n) (((((x) & ~0UL / 255 * 127) + ~0UL / 255 * (127 - (u8)(n)) | (x)) & ~0UL / 255 * 128) / 128 % 255)

// Uses 7 operations when n is constant.
// Sometimes it reports false positives. Use U32_HAS_BYTE_BETWEEN for an exact answer.
// Use this as a fast pretest:
#define U32_LIKELY_HAS_BYTE_BETWEEN(x, m, n) ((((x) - ~0UL / 255 * (u8)(n)) & ~(x) & ((x) & ~0UL / 255 * 127) + ~0UL / 255 * (127 - (u8)(m))) & ~0UL / 255 * 128)

// Uses 8 operations when n is constant:
#define U32_HAS_BYTE_BETWEEN(x, m, n) ((~0UL / 255 * (127 + (u8)(n)) - ((x) & ~0UL / 255 * 127) & ~(x) & ((x) & ~0UL / 255 * 127) + ~0UL / 255 * (127 - (u8)(m))) & ~0UL / 255 * 128)

// Uses 10 operations when n is constant:
#define U32_COUNT_BYTES_BETWEEN(x, m, n) (U32_HAS_BYTE_BETWEEN(x, m, n) / 128 % 255)

#if COMPILER == MSVC
#pragma warning(push)
#pragma warning(disable : 4146)
#endif

template <types::is_integral T>
constexpr T set_bit(T *number, T bit, bool value) {
    auto enabled = (types::make_unsigned_t<T>) value;
    *number ^= (-enabled ^ *number) & bit;
}

#if COMPILER == MSVC
#pragma warning(pop)
#endif

#define POWERS_OF_10(factor) \
    factor * 10, factor * 100, factor * 1000, factor * 10000, factor * 100000, factor * 1000000, factor * 10000000, factor * 100000000, factor * 1000000000

// These are just look up tables for powers of ten. Used in the fmt module when printing arithmetic types, for example.
constexpr u32 POWERS_OF_10_32[] = {1, POWERS_OF_10(1)};
constexpr u64 POWERS_OF_10_64[] = {1, POWERS_OF_10(1), POWERS_OF_10(1000000000ull), 10000000000000000000ull};

constexpr u32 ZERO_OR_POWERS_OF_10_32[] = {0, POWERS_OF_10(1)};
constexpr u64 ZERO_OR_POWERS_OF_10_64[] = {0, POWERS_OF_10(1), POWERS_OF_10(1000000000ull), 10000000000000000000ull};
#undef POWERS_OF_10

// Returns the number of decimal digits in n. Leading zeros are not counted
// except for n == 0 in which case count_digits returns 1.
inline u32 count_digits(u64 n) {
    s32 t = (64 - msb_64(n | 1)) * 1233 >> 12;
    return (u32) t - (n < ZERO_OR_POWERS_OF_10_64[t]) + 1;
}

template <u32 Bits, types::is_integral T>
constexpr u32 count_digits(T value) {
    T n = value;
    u32 numDigits = 0;
    do {
        ++numDigits;
    } while ((n >>= Bits) != 0);
    return numDigits;
}

//
// Atomic operations for lock-free programming:
//

template <typename T>
constexpr bool is_appropriate_size_for_atomic_v = (sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

template <typename T>
concept appropriate_for_atomic = (types::is_integral<T> || types::is_enum<T> || types::is_pointer<T>) &&is_appropriate_size_for_atomic_v<T>;

#if COMPILER == MSVC

// Returns the initial value in _ptr_
template <appropriate_for_atomic T>
always_inline constexpr T atomic_inc(T *ptr) {
    if constexpr (sizeof(T) == 2) return (T) _InterlockedIncrement16((volatile short *) ptr);
    if constexpr (sizeof(T) == 4) return (T) _InterlockedIncrement((volatile long *) ptr);
    if constexpr (sizeof(T) == 8) return (T) _InterlockedIncrement64((volatile long long *) ptr);
}

// Returns the initial value in _ptr_
template <appropriate_for_atomic T>
always_inline constexpr T atomic_add(T *ptr, T value) {
    if constexpr (sizeof(T) == 2) return (T) _InterlockedExchangeAdd16((volatile short *) ptr, value);
    if constexpr (sizeof(T) == 4) return (T) _InterlockedExchangeAdd((volatile long *) ptr, value);
    if constexpr (sizeof(T) == 8) return (T) _InterlockedExchangeAdd64((volatile long long *) ptr, value);
}

// Returns the old value in _ptr_
template <appropriate_for_atomic T>
always_inline constexpr T atomic_swap(T *ptr, T value) {
    if constexpr (sizeof(T) == 2) return (T) _InterlockedExchange16((volatile short *) ptr, value);
    if constexpr (sizeof(T) == 4) return (T) _InterlockedExchange((volatile long *) ptr, value);
    if constexpr (sizeof(T) == 8) return (T) _InterlockedExchange64((volatile long long *) ptr, value);
}

// Returns the old value in _ptr_, exchanges values only if the old value is equal to comperand.
// You can use this for a safe way to read a value, e.g. atomic_compare_and_swap(&value, 0, 0)
template <appropriate_for_atomic T>
always_inline constexpr T atomic_compare_and_swap(T *ptr, T exchange, T comperand) {
    if constexpr (sizeof(T) == 2) return (T) _InterlockedCompareExchange16((volatile short *) ptr, exchange, comperand);
    if constexpr (sizeof(T) == 4) return (T) _InterlockedCompareExchange((volatile long *) ptr, exchange, comperand);
    if constexpr (sizeof(T) == 8) return (T) _InterlockedCompareExchange64((volatile long long *) ptr, exchange, comperand);
}
#else
#define atomic_inc(ptr) __sync_add_and_fetch((ptr), 1)
#define atomic_inc_64(ptr) __sync_add_and_fetch((ptr), 1)
#error Some atomic operations not implemented
#endif

// Function for swapping endianness. You can check for the endianness by using #if ENDIAN = LITTLE_ENDIAN, etc.
always_inline constexpr void byte_swap_2(void *ptr) {
    u16 x = *(u16 *) ptr;
    *(u16 *) ptr = x << 8 & 0xFF00 | x >> 8 & 0x00FF;
}

// Function for swapping endianness. You can check for the endianness by using #if ENDIAN = LITTLE_ENDIAN, etc.
always_inline constexpr void byte_swap_4(void *ptr) {
    u32 x = *(u32 *) ptr;
    *(u32 *) ptr = x << 24 & 0xFF000000 | x << 8 & 0x00FF0000 | x >> 8 & 0x0000FF00 | x >> 24 & 0x000000FF;
}

// Function for swapping endianness. You can check for the endianness by using #if ENDIAN = LITTLE_ENDIAN, etc.
always_inline constexpr void byte_swap_8(void *ptr) {
    u64 x = *(u64 *) ptr;
    x = ((x << 8) & 0xFF00FF00FF00FF00ULL) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
    x = ((x << 16) & 0xFFFF0000FFFF0000ULL) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
    *(u64 *) ptr = (x << 32) | (x >> 32);
}

template <typename T>
struct delegate;

// Schedule a function to run when this library uninitializes - before any global C++ destructors are called
// and before the CRT (if we are linking against it) uninitializes.
//
// Runs also when calling _os_exit(exitCode)_.
//
// Note: We don't try to be as general as possible. _exit_schedule_ is merely a utility that might be useful
// to ensure e.g. files are flushed or handles closed. (Don't use this for freeing memory, the OS claims it back anyway!!).
void exit_schedule(const delegate<void()> &function);

// Runs all scheduled exit functions.
// We supply this if you are doing something very weird and want to exit without calling _os_exit_.
// This is here if you are doing something very weird and hacky.
void exit_call_scheduled_functions();

template <typename T>
struct array;

// We return a pointer so you can modify the array.
// Again, this is if you are doing something very very hacky.
// .. We don't want to limit you.
array<delegate<void()>> *exit_get_scheduled_functions();

LSTD_END_NAMESPACE

#if COMPILER != MSVC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
