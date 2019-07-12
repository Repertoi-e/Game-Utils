#pragma once

/// A header which provides type definitions as well as other helper macros

#include "intrinsics/debug_break.h"
#include "types.h"

// Convenience storage literal operators, allows for specifying sizes like this:
//  size_t a = 10_MiB;

// _B For completeness
constexpr size_t operator"" _B(u64 i) { return (size_t)(i); }
constexpr size_t operator"" _KiB(u64 i) { return (size_t)(i) << 10; }
constexpr size_t operator"" _MiB(u64 i) { return (size_t)(i) << 20; }
constexpr size_t operator"" _GiB(u64 i) { return (size_t)(i) << 30; }

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

LSTD_BEGIN_NAMESPACE
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
LSTD_END_NAMESPACE

#define DEFER_INTERNAL_(LINE) LSTD_defer##LINE
#define DEFER_INTERNAL(LINE) DEFER_INTERNAL_(LINE)
#define defer(x) auto DEFER_INTERNAL(__LINE__) = LSTD_NAMESPACE ::Defer_Dummy{} * [&]() { x; }

#undef assert

#if !defined NDEBUG
#define assert(condition) (!!(condition)) ? (void) 0 : debug_break()
#else
#define assert(condition) ((void) 0)
#endif

// Shortcut macros for "for each" loops (really up to personal style if you want to use this)
//
//  For(array) print(it);
//
#define For_as(x, in) for (const auto &x : in)
#define For(in) For_as(it, in)

LSTD_BEGIN_NAMESPACE

// Base classes to reduce boiler plate code
struct non_copyable {
   protected:
    non_copyable() = default;
    ~non_copyable() = default;

   private:
    non_copyable(const non_copyable &) = delete;
    non_copyable &operator=(const non_copyable &) = delete;
};

struct non_movable {
   protected:
    non_movable() = default;
    ~non_movable() = default;

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
struct range {
    struct iterator {
        s64 I;
        s64 Step;

        constexpr iterator(s64 i, s64 step = 1) : I(i), Step(step) {}

        operator s32() const { return (s32) I; }
        operator s64() const { return I; }

        constexpr s64 operator*() const { return (s32) I; }
        constexpr const iterator &operator++() { return I += Step, *this; }

        constexpr iterator operator++(int) {
            iterator temp(*this);
            return I += Step, temp;
        }

        constexpr bool operator==(const iterator &other) const { return Step < 0 ? (I <= other.I) : (I >= other.I); }
        constexpr bool operator!=(const iterator &other) const { return Step < 0 ? (I > other.I) : (I < other.I); }
    };

    iterator _Begin;
    iterator _End;

    constexpr range(s64 start, s64 stop, s64 step) : _Begin(start, step), _End(stop) {}
    constexpr range(s64 start, s64 stop) : range(start, stop, 1) {}
    constexpr range(u64 stop) : range(0, stop, 1) {}

    // Checks if a value is inside the given range.
    // This also accounts for stepping.
    constexpr bool has(s64 value) const {
        if (_Begin.Step > 0 ? (value >= _Begin.I && value < _End.I) : (value > _End.I && value <= _Begin.I)) {
            s64 diff = value - _Begin.I;
            if (diff % _Begin.Step == 0) {
                return true;
            }
        }
        return false;
    }

    constexpr iterator begin() const { return _Begin; }
    constexpr iterator end() const { return _End; }
};

// @Volatile: README.md
// Type policy:
//
// Aim of this policy:
// - dramatically reduce (or keep the same) complexity and code size (both library AND user side!) UNLESS that comes at
// a cost of run-time overhead
//
// - Always provide a default constructor (implicit or by "T() = default")
// - Every data member should have the same access control (everything should be public or private or protected)
// - No user defined copy/move constructors
// - No virtual or overriden functions
// - No throwing of exceptions, anywhere
//
// Most of the above requirements above are to clasify the type as POD. Templated containers and functions
// in this library assert that the type is POD. If your type doesn't classify as POD (has bit fields,
// non-default but parameterless constructor or is a subclass with extended members) you can
// declare it explictly with DECLARE_IS_POD(type, true).
// Declaring your type as POD explictly means you are sure the type CAN be TREATED as POD without bugs.
//
// "Every data member should have the same access control" in order to qualify the type as POD (plain old data)
//   This also provides freedom saves frustration of your type having a limited API.
//   If you really want you can prefix your "private" members with _.
//
// "No user defined copy/move constructors":
//   This may sound crazy if you have a type that owns memory (how would you deep copy the contents and not 
//   just the pointer when you copy the object?)
//   This library implements _string_ the following way:
//     _string_ is a struct that contains a pointer to a byte buffer and 2 fields containing precalculated 
//     utf8 code unit and code point lengths, as well as a field _Reserved_ that contains the number of 
//     bytes allocated by that string (default is 0).
//	   
//     A string may own its allocated memory or it may not, which is determined by encoding the _this_ pointer
//     before the byte buffer when the string reserves memory.
//     That way when you shallow copy the string, the _this_ pointer is obviously different (because it is a
//     different object) and when the copied string gets destructed it doesn't free the memory (it doesn't own it).
//     Only when the original string gets destructed does the memory get freed and any shallow copies of it
//     are invalidated (they point to freed memory).
//     
//     When a string gets contructed from a literal it doesn't allocate memory.
//     _Reserved_ is 0 and the object works like a view.
//     
//     When you call modifying methods (like appending, inserting code points, etc.):
//       If the string points to an allocated buffer but from a different string, the method asserts.
//       If the string was constructed from a literal or a byte buffer that wasn't allocated by a different string,
//       the string allocates a buffer, copies the old one and becomes an owner.
//
//	   
//     _clone(T *dest, T src)_ is a global function that ensures a deep copy of the argument passed.
//     There is a string overload for clone() that deep copies the contents to _dest_ (_dest_ now has allocated 
//     memory and the byte buffer contains the string from _src_).
//     
//     _move(T *dest, T *src)_ global function that transfers ownership.
//     The buffer in _src_ (iff _src_ owns it) is now owned by _dest_ (_src_ becomes simply a view into _dest_).
//     So _move_ is cheaper than _clone_ when you don't need the old string to remain an owner.
//     
//     ! Note: _clone_ and _move_ work on all types and are the required way to implement functionality 
//     normally present in copy/move c-tors.
//
//     ! Note: In c++ the default assignment operator doesn't call the destructor, 
//     so assigning to a string that owns a buffer will cause a leak.
//	   
//   Types that manage memory in this library follow similar design to string and helper functions (as well as an example)
//   are provided in _storage/owner_pointers.h_.
//
// "No virtual or overriden functions":
//   They bring a slight run-time overhead and aren't really a good design (in most cases they can be avoided).
//   I recommend striving away from a design that requires inheritance and overloading (OOP in such sense in general)
//   but I came up with a possible work-around that is best shown with an example:
//   - Using virtual functions:
//         struct writer {
//             virtual void write(string str) { /*may also be pure virtual*/
//             }
//         };
//
//         struct console_writer : writer {
//             void write(string str) override { /*...*/
//             }
//         };
//
//   - Work-around:
//         struct writer {
//             using write_func_t = void(writer *context, string *str);
//
//             static void default_write(writer *context, string *str) {}
//             write_func_t *WriteFunction = default_write; /*may also be null by default (simulate pure virtual)*/
//
//             void write(string *str) { WriteFunction(this, str); }
//         };
//
//         struct console_writer : writer {
//             static void console_write(writer *context, string *str) {
//                 auto *consoleWriter = (console_writer *) console_write;
//                 /*...*/
//             }
//
//             console_writer() { WriteFunction = console_write; }
//         };
//
// "No throwing of exceptions, anywhere"
//   Exceptions make your code complicated. They are a good way to handle errors in small examples,
//   but don't really help in large programs/projects. You can't be 100% sure what can throw where and when
//   thus you don't really know what your program is doing (you aren't sure it even works 100% of the time).
//   You should design code in such a way that errors can't occur (or if they do - handle them, not just bail,
//   and when even that is not possible - stop execution).
//
// Every type in this library complies with this policy

// Global function that is supposed to ensure a deep copy of the argument passed
// By default, a shallow copy is done (to make sure it can be called on all types)
template <typename T>
T *clone(T *dest, T src) {
    *dest = src;
    return dest;
}

// Global function that is supposed to ensure transfer of ownership without the overhead of cloning
// By default, a normal copy is done (to make sure it can be called on all types)
// Returns _dest_
template <typename T>
T *move(T *dest, T *src) {
    *dest = *src;
    return dest;
}

//
// copy_memory, fill_memory, compare_memory and SSE optimized implementations when on x86 architecture
// (implemenations in memory/memory.cpp)
//

// In this library, copy_memory works like memmove in the std (handles overlapping buffers)
// :CopyMemory (declared in types.h also to avoid circular includes)
extern void (*copy_memory)(void *dest, const void *src, size_t num);
constexpr void copy_memory_constexpr(void *dest, const void *src, size_t num) {
    auto *d = (byte *) dest;
    auto *s = (const byte *) src;

    if (d <= s || d >= (s + num)) {
        // Non-overlapping
        while (num--) {
            *d++ = *s++;
        }
    } else {
        // Overlapping
        d += num - 1;
        s += num - 1;

        while (num--) {
            *d-- = *s--;
        }
    }
}

extern void (*fill_memory)(void *dest, byte value, size_t num);
constexpr void fill_memory_constexpr(void *dest, byte value, size_t num) {
    auto d = (byte *) dest;
    while (num-- > 0) *d++ = value;
}

// compare_memory returns the index of the first byte that is different
// e.g: calling with
//		*ptr1 = 00000011
//		*ptr1 = 00100001
//	returns 2
// If the memory regions are equal, the returned value is npos (-1)
extern size_t (*compare_memory)(const void *ptr1, const void *ptr2, size_t num);
constexpr size_t compare_memory_constexpr(const void *ptr1, const void *ptr2, size_t num) {
    auto *s1 = (const byte *) ptr1;
    auto *s2 = (const byte *) ptr2;

    for (size_t index = 0; --num; ++index) {
        if (*s1++ != *s2++) return index;
    }
    return npos;
}

LSTD_END_NAMESPACE

#if COMPILER != MSVC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

#endif