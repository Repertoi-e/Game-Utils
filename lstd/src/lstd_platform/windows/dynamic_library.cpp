#include "common.h"

LSTD_BEGIN_NAMESPACE

bool dynamic_library::load(const string &name) {
    // @Bug value.Length is not enough (2 wide chars for one char)
    auto *buffer = allocate_array<utf16>(name.Length + 1, {.Alloc = Context.Temp});
    utf8_to_utf16(name.Data, name.Length, buffer);
    Handle = (void *) LoadLibraryW(buffer);
    return Handle;
}

void *dynamic_library::get_symbol(const string &name) {
    auto *cString = to_c_string(name);
    defer(free(cString));
    return (void *) GetProcAddress((HMODULE) Handle, (LPCSTR) cString);
}

void dynamic_library::close() {
    if (Handle) {
        FreeLibrary((HMODULE) Handle);
        Handle = null;
    }
}

LSTD_END_NAMESPACE
