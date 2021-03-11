#pragma once

#include "lstd/internal/common.h"

#if OS == WINDOWS

#include "../pch.h"
#include "lstd/io.h"
#include "lstd/memory/dynamic_library.h"
#include "lstd/memory/guid.h"
#include "lstd/os.h"

import path;
import fmt;

LSTD_BEGIN_NAMESPACE

extern "C" IMAGE_DOS_HEADER __ImageBase;
#define MODULE_HANDLE ((HMODULE) &__ImageBase)

struct win32_common_state {
    utf16 *HelperClassName;

    HWND HelperWindowHandle;
    HDEVNOTIFY DeviceNotificationHandle;

    static constexpr s64 CONSOLE_BUFFER_SIZE = 1_KiB;

    byte CinBuffer[CONSOLE_BUFFER_SIZE];
    byte CoutBuffer[CONSOLE_BUFFER_SIZE];
    byte CerrBuffer[CONSOLE_BUFFER_SIZE];

    HANDLE CinHandle, CoutHandle, CerrHandle;
    thread::mutex CoutMutex, CinMutex;

    array<delegate<void()>> ExitFunctions;  // Stores any functions to be called before the program terminates (naturally or by os_exit(exitCode))
    thread::mutex ExitScheduleMutex;        // Used when modifying the ExitFunctions array

    LARGE_INTEGER PerformanceFrequency;  // Used to time stuff

    string ModuleName;  // Caches the module name (retrieve this with os_get_current_module())
    string WorkingDir;  // Caches the working dir (query/modify this with os_get_working_dir(), os_set_working_dir())
    thread::mutex WorkingDirMutex;

    array<string> Argv;

    string ClipboardString;
};

// We create a byte array which large enough to hold all global variables because
// that avoids C++ constructors erasing the state we initialize before any C++ constructors are called.
// Some further explanation... We need to initialize this before main is run. We need to initialize this
// before even constructors for global variables (refered to as C++ constructors) are called (which may
// rely on e.g. the Context being initialized). This is analogous to the stuff CRT runs before main is called.
// Except that we don't link against the CRT (that's why we even have to "call" the constructors ourselves,
// using linker magic - take a look at the exe_main.cpp in no_crt).
extern byte State[sizeof(win32_common_state)];

// Short-hand macro for sanity
#define S ((win32_common_state *) &State[0])

LSTD_END_NAMESPACE

#endif
