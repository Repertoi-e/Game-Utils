#pragma once

#include "types.h"

LSTD_BEGIN_NAMESPACE

// @Avoid#include
template <typename T>
struct array;

// @Avoid#include
struct string;

/// General functions related to platform specific tasks - implemented in platform files accordingly

// Allocates memory by calling OS functions
void *os_alloc(size_t size);

// Frees a memory block allocated by _os_alloc()_
void os_free(void *ptr);

// Exits the application with the given exit code
void os_exit(s32 exitCode = 0);

// Returns a time stamp that can be used for time-interval measurements
time_t os_get_time();

// Converts a time stamp acquired by _os_get_time()_ to seconds
f64 os_time_to_seconds(time_t time);

string os_get_clipboard_content();
void os_set_clipboard_content(string content);

// Sleep for _ms_ milliseconds
// void os_sleep(f64 ms);

// Returns the path of the current exe (full dir + name)
string os_get_exe_name();

// Get the value of an environment variable, returns true if found. 
// If not found and silent is false, logs error to cerr.
bool os_get_env(string *out, string name, bool silent = false);

// Sets a variable (creates if it doesn't exist yet) in this process' environment
void os_set_env(string name, string value);

// Delete a variable from the current process' environment
void os_remove_env(string name);

// Get a list of parsed command line arguments excluding the first one (the exe name - you can get that with os_get_exe_name())
array<string> os_get_command_line_arguments();

// Returns an ID which uniquely identifies the current process on the system
u32 os_get_pid();

// Utility to report hresult errors produces by calling windows functions.
// Shouldn't be used on other platforms
#if OS == WINDOWS

// Logs a formatted error message.
void windows_report_hresult_error(long hresult, string call, string file, s32 line);

// CHECKHR checks the return value of _call_ and if the returned HRESULT is less than zero, reports an error.
#define CHECKHR(call)                                                                    \
    {                                                                                    \
        long result = call;                                                              \
        if (result < 0) windows_report_hresult_error(result, #call, __FILE__, __LINE__); \
    }

// DXCHECK is used for checking e.g. directx calls. The difference is that
// in Dist configuration, the macro expands to just the call (no error checking)
// in order to save on performance.
#if defined DEBUG || defined RELEASE
#define DXCHECK(call) CHECKHR(call)
#else
#define DXCHECK(call) call
#endif

#define SAFE_RELEASE(x) \
    if (x) {            \
        x->Release();   \
        x = null;       \
    }

#endif  // OS == WINDOWS

LSTD_END_NAMESPACE
