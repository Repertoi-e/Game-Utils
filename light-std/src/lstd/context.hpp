#pragma once

#include "common.hpp"
#include "memory/allocator.hpp"

#include <functional>

LSTD_BEGIN_NAMESPACE

// A function that gets called when an assert in the program is called.
// If you don't specify one in the context a default one is provided,
// which on failure prints the information to the log writer and stops the program.
//
// The message is the condition in string form. This allows the assert user to
// add extra information that will get printed, for example:
//
//      assert(index < size && "Index out of bounds.");
//
using Assert_Function = std::function<void(const char *file, int line, const char *condition)>;

namespace io {
class Writer;
}

namespace internal {
extern io::Writer *ConsoleLog;
}

// When allocating you should use the context's allocator
// This makes it so when users call your functions, they
// can specify an allocator beforehand by pushing a new context,
// without having to pass you anything as a parameter for example.
//
// The idea for this comes from the implicit context in Jai.
struct Implicit_Context {
    Allocator_Closure Allocator = MALLOC;

    // This variable is useful when you redirect all logging output
    // (provided that the code that logs stuff uses the context!).
    // By default the writer outputs to the console.
    io::Writer *Log = internal::ConsoleLog;
    Assert_Function AssertFailed = os_assert_failed;
};

// Immutable context available everywhere
// TODO: This should be thread local
inline const Implicit_Context Context;

#define OLD_CONTEXT_VAR_GEN_(x, LINE) LSTD_NAMESPACE_NAME##_lstd_context##x##LINE
#define OLD_CONTEXT_VAR_GEN(x, LINE) OLD_CONTEXT_VAR_GEN_(x, LINE)

// This is a helper macro to safely modify the implicit context in a block of code.
// Usage:
//    PUSH_CONTEXT(myContext) {
//        ... code with new context ...
//    }
//    ... old context is restored ...
//
// Don't pass a pointer as a parameter!
#define PUSH_CONTEXT(newContext)                                                                    \
    Implicit_Context OLD_CONTEXT_VAR_GEN(context, __LINE__) = Context;                              \
    bool OLD_CONTEXT_VAR_GEN(restored, __LINE__) = false;                                           \
    defer {                                                                                         \
        if (!OLD_CONTEXT_VAR_GEN(restored, __LINE__)) {                                             \
            *const_cast<Implicit_Context *>(&Context) = OLD_CONTEXT_VAR_GEN(context, __LINE__);     \
        }                                                                                           \
    };                                                                                              \
    if (true) {                                                                                     \
        *const_cast<Implicit_Context *>(&Context) = newContext;                                     \
        goto body;                                                                                  \
    } else                                                                                          \
        while (true)                                                                                \
            if (true) {                                                                             \
                *const_cast<Implicit_Context *>(&Context) = OLD_CONTEXT_VAR_GEN(context, __LINE__); \
                OLD_CONTEXT_VAR_GEN(restored, __LINE__) = true;                                     \
                break;                                                                              \
            } else                                                                                  \
            body:

#define CONTEXT_ALLOC Context.Allocator

LSTD_END_NAMESPACE