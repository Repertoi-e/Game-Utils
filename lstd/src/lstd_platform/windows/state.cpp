#include "common.h"

#if OS == WINDOWS

LSTD_BEGIN_NAMESPACE

byte State[sizeof(win32_common_state)];

// This zeroes out the global variables (stored in State) and initializes the mutexes
void init_global_vars() {
    zero_memory(&State, sizeof(win32_common_state));

    // Init mutexes
    S->CinMutex.init();
    S->CoutMutex.init();
    S->ExitScheduleMutex.init();
    S->WorkingDirMutex.init();
#if defined DEBUG_MEMORY
    DEBUG_memory_info::Mutex.init();
#endif
}

// When our program runs, but also needs to happen when a new thread starts!
void win32_common_init_context() {
    context *c = (context *) &Context;  // @Constcast

    c->ThreadID = thread::id((u64) GetCurrentThreadId());

    c->Alloc = {default_allocator, null};

    auto startingSize = 8_KiB;
    c->TempAllocData.Base.Storage = (byte *) os_allocate_block(startingSize);  // @XXX @TODO: Allocate this with malloc (the general purpose allocator)!
    c->TempAllocData.Base.Allocated = startingSize;

    c->Temp = {temporary_allocator, &c->TempAllocData};
}

void exit_schedule(const delegate<void()> &function) {
    thread::scoped_lock _(&S->ExitScheduleMutex);

    WITH_CONTEXT_VAR(AllocOptions, Context.AllocOptions | LEAK) {
        append(S->ExitFunctions, function);
    }
}

// We supply this as API to the user if they are doing something very hacky..
void exit_call_scheduled_functions() {
    thread::scoped_lock _(&S->ExitScheduleMutex);

    For(S->ExitFunctions) it();
}

// We supply this as API to the user if they are doing something very hacky..
array<delegate<void()>> *exit_get_scheduled_functions() {
    return &S->ExitFunctions;
}

void uninit_win32_state() {
#if defined DEBUG_MEMORY
    release_temporary_allocator();

    // Now we check for memory leaks.
    // Yes, the OS claims back all the memory the program has allocated anyway, and we are not promoting C++ style RAII
    // which make even program termination slow, we are just providing this information to the user because they might
    // want to load/unload DLLs during the runtime of the application, and those DLLs might use all kinds of complex
    // cross-boundary memory stuff things, etc. This is useful for debugging crashes related to that.
    if (DEBUG_memory_info::CheckForLeaksAtTermination) {
        DEBUG_memory_info::report_leaks();
    }
#endif

    DestroyWindow(S->HelperWindowHandle);

    // Uninit mutexes
    S->CinMutex.release();
    S->CoutMutex.release();
    S->ExitScheduleMutex.release();
    S->WorkingDirMutex.release();
#if defined DEBUG_MEMORY
    DEBUG_memory_info::Mutex.release();
#endif
}

/*
// We use to do this on MSVC but since then we no longer link the CRT and do all of this ourselves. (take a look at no_crt/exe_main.cpp)
//
// This trick makes all of the above requirements work on the MSVC compiler.
//
// How it works is described in this awesome article:
// https://www.codeguru.com/cpp/misc/misc/applicationcontrol/article.php/c6945/Running-Code-Before-and-After-Main.htm#page-2
#if COMPILER == MSVC
s32 c_init() {
    init_mutexes();
    init_context();
    return 0;
}

// We need to reinit the context after the TLS initalizer fires and resets our state.. sigh.
// We can't just do it once because global variables might still use the context and TLS fires a bit later.
s32 tls_init() {
    init_context();
    return 0;
}

s32 cpp_init() {
    init_win32_state();
    return 0;
}

s32 pre_termination() {
    call_exit_functions();
    uninit_win32_state();
    return 0;
}

#pragma push_macro("allocate")
#undef allocate

typedef s32 cb(void);
#pragma const_seg(".CRT$XIU")
__declspec(allocate(".CRT$XIU")) cb *g_CInit = c_init;
#pragma const_seg()

#pragma const_seg(".CRT$XDU")
__declspec(allocate(".CRT$XDU")) cb *g_TLSInit = tls_init;
#pragma const_seg()

#pragma const_seg(".CRT$XCU")
__declspec(allocate(".CRT$XCU")) cb *g_CPPInit = cpp_init;
#pragma const_seg()

#pragma const_seg(".CRT$XPU")
__declspec(allocate(".CRT$XPU")) cb *g_PreTermination = pre_termination;
#pragma const_seg()

#pragma const_seg(".CRT$XTU")
__declspec(allocate(".CRT$XTU")) cb *g_Termination = NULL;
#pragma const_seg()

#pragma pop_macro("allocate")

#else
#error @TODO: See how this works on other compilers!
#endif
*/

// This registeres a dummy window class which is used for some os_ functions
file_scope void register_helper_window_class() {
    GUID guid;
    WIN32_CHECKHR(CoCreateGuid(&guid));
    WIN32_CHECKHR(StringFromCLSID(guid, &S->HelperClassName));

    WNDCLASSEXW wc;
    zero_memory(&wc, sizeof(wc));
    {
        wc.cbSize = sizeof(wc);
        wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = MODULE_HANDLE;
        wc.hCursor = LoadCursorW(null, IDC_ARROW);
        wc.lpszClassName = S->HelperClassName;

        // Load user-provided icon if available
        wc.hIcon = (HICON) LoadImageW(MODULE_HANDLE, L"WINDOW ICON", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
        if (!wc.hIcon) {
            // No user-provided icon found, load default icon
            wc.hIcon = (HICON) LoadImageW(null, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
        }
    }

    if (!RegisterClassExW(&wc)) {
        print("(windows_common.cpp): Failed to register helper window class\n");
        assert(false);
    }
}

void win32_common_init_global_state() {
    init_global_vars();

    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();

        // set the screen buffer to be big enough to let us scroll text
        CONSOLE_SCREEN_BUFFER_INFO cInfo;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cInfo);
        cInfo.dwSize.Y = 500;
        SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), cInfo.dwSize);
    }

    S->CinHandle = GetStdHandle(STD_INPUT_HANDLE);
    S->CoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    S->CerrHandle = GetStdHandle(STD_ERROR_HANDLE);

    if (!SetConsoleOutputCP(CP_UTF8)) {
        string warning = ">>> Warning: Couldn't set console code page to UTF8. Some characters might be messed up.\n";

        DWORD ignored;
        WriteFile(S->CerrHandle, warning.Data, (DWORD) warning.Count, &ignored, null);
    }

    // Enable ANSI escape sequences
    DWORD dw = 0;
    GetConsoleMode(S->CoutHandle, &dw);
    SetConsoleMode(S->CoutHandle, dw | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    GetConsoleMode(S->CerrHandle, &dw);
    SetConsoleMode(S->CerrHandle, dw | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Used to time stuff
    QueryPerformanceFrequency(&S->PerformanceFrequency);

    // Get the module name
    utf16 *buffer = allocate_array<utf16>(MAX_PATH, {.Alloc = Context.Temp});
    s64 reserved = MAX_PATH;

    while (true) {
        s64 written = GetModuleFileNameW(MODULE_HANDLE, buffer, (DWORD) reserved);
        if (written == reserved) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                reserved *= 2;
                buffer = allocate_array<utf16>(reserved, {.Alloc = Context.Temp});
                continue;
            }
        }
        break;
    }

    WITH_CONTEXT_VAR(AllocOptions, Context.AllocOptions | LEAK) {
        reserve(S->ModuleName, reserved * 2);  // @Bug reserved * 2 is not enough
    }

    utf16_to_utf8(buffer, const_cast<utf8 *>(S->ModuleName.Data), &S->ModuleName.Count);
    S->ModuleName.Length = utf8_length(S->ModuleName.Data, S->ModuleName.Count);

    // :UnifyPath
    replace_all(S->ModuleName, '\\', '/');

    // Get the arguments
    utf16 **argv;
    s32 argc;

    // @Cleanup: Parse the arguments ourselves and use our temp allocator
    // and don't bother with cleaning up this functions memory.
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == null) {
        string warning = ">>> Warning: Couldn't parse command line arguments, os_get_command_line_arguments() will return an empty array in all cases.\n";

        DWORD ignored;
        WriteFile(S->CerrHandle, warning.Data, (DWORD) warning.Count, &ignored, null);
    } else {
        WITH_CONTEXT_VAR(AllocOptions, Context.AllocOptions | LEAK) {
            reserve(S->Argv, argc - 1);
        }

        For(range(1, argc)) {
            auto *warg = argv[it];

            auto *arg = append(S->Argv);
            WITH_CONTEXT_VAR(AllocOptions, Context.AllocOptions | LEAK) {
                reserve(*arg, c_string_length(warg) * 2);  // @Bug c_string_length * 2 is not enough
            }
            utf16_to_utf8(warg, const_cast<utf8 *>(arg->Data), &arg->Count);
            arg->Length = utf8_length(arg->Data, arg->Count);
        }

        LocalFree(argv);
    }

    // Create helper window
    register_helper_window_class();

    {
        MSG msg;

        S->HelperWindowHandle = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, S->HelperClassName, L"LSTD Message Window", WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 1, 1, null, null, MODULE_HANDLE, null);
        if (!S->HelperWindowHandle) {
            print("(windows_monitor.cpp): Failed to create helper window\n");
            assert(false);
        }

        ShowWindow(S->HelperWindowHandle, SW_HIDE);

        // Register for HID device notifications
        {
            DEV_BROADCAST_DEVICEINTERFACE_W dbi;
            zero_memory(&dbi, sizeof(dbi));
            {
                dbi.dbcc_size = sizeof(dbi);
                dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                dbi.dbcc_classguid = GUID_DEVINTERFACE_HID;
            }
            S->DeviceNotificationHandle = RegisterDeviceNotificationW(S->HelperWindowHandle, (DEV_BROADCAST_HDR *) &dbi, DEVICE_NOTIFY_WINDOW_HANDLE);
        }

        while (PeekMessageW(&msg, S->HelperWindowHandle, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

bytes os_read_from_console() {
    DWORD read;
    ReadFile(S->CinHandle, S->CinBuffer, (DWORD) S->CONSOLE_BUFFER_SIZE, &read, null);
    return bytes(S->CinBuffer, (s64) read);
}

void *os_allocate_block(s64 size) {
    assert(size < MAX_ALLOCATION_REQUEST);
    return HeapAlloc(GetProcessHeap(), 0, size);
}

// Tests whether the allocation contraction is possible
file_scope bool is_contraction_possible(s64 oldSize) {
    // Check if object allocated on low fragmentation heap.
    // The LFH can only allocate blocks up to 16KB in size.
    if (oldSize <= 0x4000) {
        LONG heapType = -1;
        if (!HeapQueryInformation(GetProcessHeap(), HeapCompatibilityInformation, &heapType, sizeof(heapType), null)) {
            return false;
        }
        return heapType != 2;
    }

    // Contraction possible for objects not on the LFH
    return true;
}

file_scope void *try_heap_realloc(void *ptr, s64 newSize, bool *reportError) {
    void *result = null;
    __try {
        result = HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY | HEAP_GENERATE_EXCEPTIONS, ptr, newSize);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // We specify HEAP_REALLOC_IN_PLACE_ONLY, so STATUS_NO_MEMORY is a valid error.
        // We don't need to report it.
        *reportError = GetExceptionCode() != STATUS_NO_MEMORY;
    }
    return result;
}

void *os_resize_block(void *ptr, s64 newSize) {
    assert(ptr);
    assert(newSize < MAX_ALLOCATION_REQUEST);

    s64 oldSize = os_get_block_size(ptr);
    if (newSize == 0) newSize = 1;

    bool reportError = false;
    void *result = try_heap_realloc(ptr, newSize, &reportError);

    if (result) return result;

    // If a failure to contract was caused by platform limitations, just return the original block
    if (newSize < oldSize && !is_contraction_possible(oldSize)) return ptr;

    if (reportError) {
        windows_report_hresult_error(HRESULT_FROM_WIN32(GetLastError()), "HeapReAlloc", __FILE__, __LINE__);
    }
    return null;
}

s64 os_get_block_size(void *ptr) {
    s64 result = HeapSize(GetProcessHeap(), 0, ptr);
    if (result == -1) {
        windows_report_hresult_error(HRESULT_FROM_WIN32(GetLastError()), "HeapSize", __FILE__, __LINE__);
        return 0;
    }
    return result;
}

#define CREATE_MAPPING_CHECKED(handleName, call, returnOnFail)                                                  \
    HANDLE handleName = call;                                                                                   \
    if (!handleName) {                                                                                          \
        string extendedCallSite = sprint("{}\n        (the name was: {!YELLOW}\"{}\"{!GRAY})\n", #call, name);  \
        defer(free(extendedCallSite));                                                                          \
        windows_report_hresult_error(HRESULT_FROM_WIN32(GetLastError()), extendedCallSite, __FILE__, __LINE__); \
        return returnOnFail;                                                                                    \
    }

void os_write_shared_block(const string &name, void *data, s64 size) {
    // @Bug name.Length is not enough (2 wide chars for one char)
    auto *name16 = allocate_array<utf16>(name.Length + 1, {.Alloc = Context.Temp});
    utf8_to_utf16(name.Data, name.Length, name16);

    CREATE_MAPPING_CHECKED(h,
                           CreateFileMappingW(INVALID_HANDLE_VALUE, null, PAGE_READWRITE, 0, (DWORD) size, name16), );
    defer(CloseHandle(h));

    void *result = MapViewOfFile(h, FILE_MAP_WRITE, 0, 0, size);
    if (!result) {
        windows_report_hresult_error(HRESULT_FROM_WIN32(GetLastError()), "MapViewOfFile", __FILE__, __LINE__);
        return;
    }
    copy_memory(result, data, size);
    UnmapViewOfFile(result);
}

void os_read_shared_block(const string &name, void *out, s64 size) {
    // @Bug name.Length is not enough (2 wide chars for one char)
    auto *name16 = allocate_array<utf16>(name.Length + 1, {.Alloc = Context.Temp});
    utf8_to_utf16(name.Data, name.Length, name16);

    CREATE_MAPPING_CHECKED(h, OpenFileMappingW(FILE_MAP_READ, false, name16), );
    defer(CloseHandle(h));

    void *result = MapViewOfFile(h, FILE_MAP_READ, 0, 0, size);
    if (!result) {
        windows_report_hresult_error(HRESULT_FROM_WIN32(GetLastError()), "MapViewOfFile", __FILE__, __LINE__);
        return;
    }

    copy_memory(out, result, size);
    UnmapViewOfFile(result);
}

void os_free_block(void *ptr) {
    WIN32_CHECKBOOL(HeapFree(GetProcessHeap(), 0, ptr));
}

void os_exit(s32 exitCode) {
    exit_call_scheduled_functions();
    uninit_win32_state();
    ExitProcess(exitCode);
}

void os_abort() {
    ExitProcess(3);
}

time_t os_get_time() {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart;
}

f64 os_time_to_seconds(time_t time) { return (f64) time / S->PerformanceFrequency.QuadPart; }

string os_get_current_module() { return S->ModuleName; }

string os_get_working_dir() {
    thread::scoped_lock _(&S->WorkingDirMutex);

    DWORD required = GetCurrentDirectoryW(0, null);
    auto *dir16 = allocate_array<utf16>(required + 1, {.Alloc = Context.Temp});

    if (!GetCurrentDirectoryW(required + 1, dir16)) {
        windows_report_hresult_error(HRESULT_FROM_WIN32(GetLastError()), "GetCurrentDirectoryW", __FILE__, __LINE__);
        return "";
    }

    WITH_CONTEXT_VAR(AllocOptions, Context.AllocOptions | LEAK) {
        reserve(S->WorkingDir, required * 2);  // @Bug required * 2 is not enough
    }

    utf16_to_utf8(dir16, const_cast<utf8 *>(S->WorkingDir.Data), &S->WorkingDir.Count);
    S->WorkingDir.Length = utf8_length(S->WorkingDir.Data, S->WorkingDir.Count);

    // :UnifyPath
    replace_all(S->WorkingDir, '\\', '/');

    return S->WorkingDir;
}

void os_set_working_dir(const string &dir) {
    string path(dir);
    assert(path_is_absolute(path));

    thread::scoped_lock _(&S->WorkingDirMutex);

    // @Bug
    auto *dir16 = allocate_array<utf16>(dir.Length + 1, {.Alloc = Context.Temp});
    utf8_to_utf16(dir.Data, dir.Length, dir16);

    WIN32_CHECKBOOL(SetCurrentDirectoryW(dir16));

    S->WorkingDir = dir;
}

os_get_env_result os_get_env(const string &name, bool silent) {
    // @Bug name.Length is not enough (2 wide chars for one char)
    auto *name16 = allocate_array<utf16>(name.Length + 1, {.Alloc = Context.Temp});
    utf8_to_utf16(name.Data, name.Length, name16);

    DWORD bufferSize = 65535;  // Limit according to http://msdn.microsoft.com/en-us/library/ms683188.aspx

    auto *buffer = allocate_array<utf16>(bufferSize, {.Alloc = Context.Temp});
    auto r = GetEnvironmentVariableW(name16, buffer, bufferSize);

    if (r == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
        if (!silent) {
            print(">>> Warning: Couldn't find environment variable with value \"{}\"\n", name);
        }
        return {"", false};
    }

    // 65535 may be the limit but let's not take risks
    if (r > bufferSize) {
        buffer = allocate_array<utf16>(r, {.Alloc = Context.Temp});
        GetEnvironmentVariableW(name16, buffer, r);
        bufferSize = r;

        // Possible to fail a second time ?
    }

    string result;
    reserve(result, bufferSize * 2);  // @Bug bufferSize * 2 is not enough
    utf16_to_utf8(buffer, const_cast<utf8 *>(result.Data), &result.Count);
    result.Length = utf8_length(result.Data, result.Count);

    return {result, true};
}

void os_set_env(const string &name, const string &value) {
    // @Bug name.Length is not enough (2 wide chars for one char)
    auto *name16 = allocate_array<utf16>(name.Length + 1, {.Alloc = Context.Temp});
    utf8_to_utf16(name.Data, name.Length, name16);

    // @Bug value.Length is not enough (2 wide chars for one char)
    auto *value16 = allocate_array<utf16>(value.Length + 1, {.Alloc = Context.Temp});
    utf8_to_utf16(value.Data, value.Length, value16);

    if (value.Length > 32767) {
        // assert(false);
        // @Cleanup
        // The docs say windows doesn't allow that but we should test it.
    }

    WIN32_CHECKBOOL(SetEnvironmentVariableW(name16, value16));
}

void os_remove_env(const string &name) {
    // @Bug name.Length is not enough (2 wide chars for one char)
    auto *name16 = allocate_array<utf16>(name.Length + 1, {.Alloc = Context.Temp});
    utf8_to_utf16(name.Data, name.Length, name16);

    WIN32_CHECKBOOL(SetEnvironmentVariableW(name16, null));
}

string os_get_clipboard_content() {
    if (!OpenClipboard(S->HelperWindowHandle)) {
        print("(windows_monitor.cpp): Failed to open clipboard\n");
        return "";
    }
    defer(CloseClipboard());

    HANDLE object = GetClipboardData(CF_UNICODETEXT);
    if (!object) {
        print("(windows_monitor.cpp): Failed to convert clipboard to string\n");
        return "";
    }

    auto *buffer = (utf16 *) GlobalLock(object);
    if (!buffer) {
        print("(windows_monitor.cpp): Failed to lock global handle\n");
        return "";
    }
    defer(GlobalUnlock(object));

    WITH_CONTEXT_VAR(AllocOptions, Context.AllocOptions | LEAK) {
        reserve(S->ClipboardString, c_string_length(buffer) * 2);  // @Bug c_string_length * 2 is not enough
    }

    utf16_to_utf8(buffer, const_cast<utf8 *>(S->ClipboardString.Data), &S->ClipboardString.Count);
    S->ClipboardString.Length = utf8_length(S->ClipboardString.Data, S->ClipboardString.Count);

    return S->ClipboardString;
}

void os_set_clipboard_content(const string &content) {
    HANDLE object = GlobalAlloc(GMEM_MOVEABLE, content.Length * 2 * sizeof(utf16));
    if (!object) {
        print("(windows_monitor.cpp): Failed to open clipboard\n");
        return;
    }
    defer(GlobalFree(object));

    auto *buffer = (utf16 *) GlobalLock(object);
    if (!buffer) {
        print("(windows_monitor.cpp): Failed to lock global handle\n");
        return;
    }

    utf8_to_utf16(content.Data, content.Length, buffer);
    GlobalUnlock(object);

    if (!OpenClipboard(S->HelperWindowHandle)) {
        print("(windows_monitor.cpp): Failed to open clipboard\n");
        return;
    }
    defer(CloseClipboard());

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, object);
    CloseClipboard();
}

// Doesn't include the exe name.
array<string> os_get_command_line_arguments() { return S->Argv; }

u32 os_get_pid() { return (u32) GetCurrentProcessId(); }

guid guid_new() {
    GUID g;
    CoCreateGuid(&g);

    auto data = to_stack_array((byte)((g.Data1 >> 24) & 0xFF), (byte)((g.Data1 >> 16) & 0xFF),
                               (byte)((g.Data1 >> 8) & 0xFF), (byte)((g.Data1) & 0xff),

                               (byte)((g.Data2 >> 8) & 0xFF), (byte)((g.Data2) & 0xff),

                               (byte)((g.Data3 >> 8) & 0xFF), (byte)((g.Data3) & 0xFF),

                               (byte) g.Data4[0], (byte) g.Data4[1], (byte) g.Data4[2], (byte) g.Data4[3],
                               (byte) g.Data4[4], (byte) g.Data4[5], (byte) g.Data4[6], (byte) g.Data4[7]);
    return guid(data);
}

LSTD_END_NAMESPACE

#endif