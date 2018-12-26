#pragma once

#include "../memory/memory.hpp"

#include "string.hpp"

CPPU_BEGIN_NAMESPACE

struct String_Builder {
    static constexpr size_t BUFFER_SIZE = 4_KiB;

    struct Buffer {
        char Data[BUFFER_SIZE];
        size_t Occupied = 0;
        Buffer *Next = null;
    };

    // Counts how many extra buffers have been dynamically allocated.
    size_t IndirectionCount = 0;

    Buffer _BaseBuffer;
    Buffer *CurrentBuffer = &_BaseBuffer;

    // The allocator used for allocating new buffers past the first one (which is stack allocated).
    // This value is null until this object allocates memory or the user sets it manually.
    Allocator_Closure Allocator;

    // Append a string to the builder
    void append(const char *str);
    void append(const string_view &str);
    void append(const string &str);

    // Append a non encoded character to a string
    void append(char32_t codePoint);

    // Append a null terminated utf-8 cstyle string.
    void append_cstring(const char *str);

    // Append _size_ bytes of string contained in _data_
    void append_pointer_and_size(const char *data, size_t size);

    // Merges all buffers and returns a single string.
    inline string combine() const {
        string result;
        result.reserve((IndirectionCount + 1) * BUFFER_SIZE);

        const String_Builder::Buffer *buffer = &_BaseBuffer;
        while (buffer) {
            result.append_pointer_and_size(buffer->Data, buffer->Occupied);
            buffer = buffer->Next;
        }
        return result;
    }

    // Don't deallocate, just move cursor to 0
    void reset();

    // Free the entire builder
    void release();

    ~String_Builder();
};

CPPU_END_NAMESPACE