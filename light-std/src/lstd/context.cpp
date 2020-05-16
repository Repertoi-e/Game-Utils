#include "context.h"

#include "io/fmt.h"
#include "storage/array.h"

LSTD_BEGIN_NAMESPACE

void default_unexpected_exception_handler(string message, array<os_function_call> callStack) {
    fmt::print("\n{!}(context.cpp / default_crash_handler): An exception occured and the program must terminate.\n");
    fmt::print("{!GRAY}        Error: {!RED}{}{!}\n\n", message);
    fmt::print("        ... and here is the call stack:\n");
    For(callStack) {
        fmt::print("        {!YELLOW}{}{!}\n", it.Name);
        fmt::print("          in file: {}:{}\n", it.File, it.LineNumber);
    }
    fmt::print("\n\n");
}

void implicit_context::release_temporary_allocator() const {
    if (!TemporaryAllocData.Base.Reserved) return;

    // Free any left-over overflow pages!
    TemporaryAlloc.free_all();

    delete[] TemporaryAllocData.Base.Storage;
    *const_cast<temporary_allocator_data *>(&TemporaryAllocData) = {};
}

LSTD_END_NAMESPACE