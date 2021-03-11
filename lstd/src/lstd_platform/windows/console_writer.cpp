module;

#include "common.h"

module lstd.io;

LSTD_BEGIN_NAMESPACE

void console_writer::write(const byte *data, s64 size) {
    thread::mutex *mutex = null;
    if (LockMutex) mutex = &S->CoutMutex;
    thread::scoped_lock _(mutex);

    if (size > Available) {
        flush();
    }

    copy_memory(Current, data, size);

    Current += size;
    Available -= size;
}

void console_writer::flush() {
    thread::mutex *mutex = null;
    if (LockMutex) mutex = &S->CoutMutex;
    thread::scoped_lock _(mutex);

    if (!Buffer) {
        if (OutputType == console_writer::COUT) {
            Buffer = Current = S->CoutBuffer;
        } else {
            Buffer = Current = S->CerrBuffer;
        }

        BufferSize = Available = S->CONSOLE_BUFFER_SIZE;
    }

    HANDLE target = OutputType == console_writer::COUT ? S->CoutHandle : S->CerrHandle;

    DWORD ignored;
    WriteFile(target, Buffer, (DWORD)(BufferSize - Available), &ignored, null);

    Current = Buffer;
    Available = S->CONSOLE_BUFFER_SIZE;
}

// This workaround is needed in order to prevent circular inclusion of context.h
namespace internal {
writer *g_ConsoleLog = &cout;
}

LSTD_END_NAMESPACE
