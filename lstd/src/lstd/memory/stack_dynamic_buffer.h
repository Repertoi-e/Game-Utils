#pragma once

#include "../internal/context.h"
#include "array.h"
#include "string.h"

LSTD_BEGIN_NAMESPACE

// A buffer that uses a stack allocated buffer before dynamically allocating.
// StackSize - the amount of bytes used on the stack
template <s64 StackSize>
struct stack_dynamic_buffer : non_copyable, non_movable, non_assignable {
    byte StackData[StackSize]{};
    byte *Data = StackData;

    s64 Allocated = 0;
    s64 Count = 0;

    stack_dynamic_buffer() {}

    // We no longer use destructors for deallocation.
    // ~stack_dynamic_buffer() { free(); }

    //
    // Iterator:
    //
    using iterator = byte *;
    using const_iterator = const byte *;

    iterator begin() { return Data; }
    iterator end() { return Data + Count; }

    const_iterator begin() const { return Data; }
    const_iterator end() const { return Data + Count; }

    //
    // Operators:
    //
    operator array<byte>() { return array<byte>(Data, Count); }
    explicit operator bool() const { return Count; }

    // Read/write [] operator
    byte &operator[](s64 index) { return get(*this, index); }
    // Read-only [] operator
    byte operator[](s64 index) const { return get(*this, index); }
};

template <typename T>
struct is_stack_dynamic_buffer : types::false_t {};

template <s64 StackSize>
struct is_stack_dynamic_buffer<stack_dynamic_buffer<StackSize>> : types::true_t {};

template <typename T>
concept any_stack_dynamic_buffer = is_stack_dynamic_buffer<T>::value;

// Makes sure buffer has reserved enough space for at least n bytes.
// Note that it may reserve way more than required.
// Reserves space equal to the next power of two bigger than _size_, starting at 8.
//
// ! Reserves only if there is not enough space on the stack
template <any_stack_dynamic_buffer T>
void reserve(T &buffer, s64 targetCount) {
    if (targetCount < sizeof(buffer.StackData)) return;
    if (buffer.Count + targetCount < buffer.Allocated) return;

    targetCount = max(ceil_pow_of_2(targetCount + buffer.Count + 1), 8);

    if (buffer.Allocated) {
        buffer.Data = reallocate_array(buffer.Data, targetCount);
    } else {
        auto *oldData = buffer.Data;
        buffer.Data = allocate_array<byte>(targetCount);
        if (buffer.Count) copy_memory(buffer.Data, oldData, buffer.Count);
    }
    buffer.Allocated = targetCount;
}

// Releases the memory allocated by this buffer.
// If this buffer doesn't own the memory it points to, this function does nothing.
template <any_stack_dynamic_buffer T>
void free(T &buffer) {
    if (buffer.Allocated) free(buffer.Data);
    buffer.Data = null;
    buffer.Count = buffer.Allocated = 0;
}

// Don't free the buffer, just move cursor to 0
template <any_stack_dynamic_buffer T>
void reset(T &buffer) { buffer.Count = 0; }

// Allows negative indexing which is reversed and begins at the end
template <any_stack_dynamic_buffer T>
byte &get(T &buffer, s64 index) { return buffer.Data[translate_index(index, buffer.Count)]; }

template <any_stack_dynamic_buffer T>
byte get(const T &buffer, s64 index) { return buffer.Data[translate_index(index, buffer.Count)]; }

// Sets the _index_'th byte in the string
template <any_stack_dynamic_buffer T>
void set(T &buffer, s64 index, byte b) { buffer.Data[translate_index(index, buffer.Count)] = b; }

// Insert a byte at a specified index
// _unsafe_ - avoid reserving (may attempt to write past buffer if there is not enough space!)
template <any_stack_dynamic_buffer T>
void insert(T &buffer, s64 index, byte b, bool unsafe = false) {
    if (!unsafe) reserve(buffer, buffer.Count + 1);

    auto *target = buffer.Data + translate_index(index, buffer.Count, true);
    u64 offset = (u64)(target - buffer.Data);
    copy_memory((byte *) buffer.Data + offset + 1, target, buffer.Count - (target - buffer.Data));
    *target = b;

    ++buffer.Count;
}

// Insert data after a specified index
// _unsafe_ - avoid reserving (may attempt to write past buffer if there is not enough space!)
template <any_stack_dynamic_buffer T>
void insert_array(T &buffer, s64 index, const array<byte> &arr, bool unsafe = false) {
    insert_pointer_and_size(buffer, index, arr.Data, arr.Count, unsafe);
}

// Insert data after a specified index
// _unsafe_ - avoid reserving (may attempt to write past buffer if there is not enough space!)
template <any_stack_dynamic_buffer T>
void insert_array(T &buffer, s64 index, const initializer_list<byte> &list, bool unsafe = false) {
    insert_pointer_and_size(buffer, index, list.begin(), list.size(), unsafe);
}

// Insert a buffer of bytes at a specified index
// _unsafe_ - avoid reserving (may attempt to write past buffer if there is not enough space!)
template <any_stack_dynamic_buffer T>
void insert_pointer_and_size(T &buffer, s64 index, const byte *data, s64 count, bool unsafe = false) {
    if (!unsafe) reserve(buffer, buffer.Count + count);

    auto *target = buffer.Data + translate_index(index, buffer.Count, true);
    u64 offset = (u64)(target - buffer.Data);
    copy_memory((byte *) buffer.Data + offset + count, target, buffer.Count - (target - buffer.Data));
    copy_memory(target, data, count);

    buffer.Count += count;
}

// Remove byte at specified index
template <any_stack_dynamic_buffer T>
void remove(T &buffer, s64 index) {
    auto *targetBegin = buffer.Data + translate_index(index, buffer.Count);
    u64 offset = (u64)(targetBegin - buffer.Data);
    copy_memory((byte *) buffer.Data + offset, targetBegin + 1, buffer.Count - offset - 1);

    --buffer.Count;
}

// Remove a range of bytes.
// [begin, end)
template <any_stack_dynamic_buffer T>
void remove_range(T &buffer, s64 begin, s64 end) {
    auto *targetBegin = buffer.Data + translate_index(begin, buffer.Count);
    auto *targetEnd = buffer.Data + translate_index(begin, buffer.Count, true);

    assert(targetEnd > targetBegin);

    s64 bytes = targetEnd - targetBegin;
    u64 offset = (u64)(targetBegin - buffer.Data);
    copy_memory((byte *) buffer.Data + offset, targetEnd, buffer.Count - offset - bytes);

    buffer.Count -= bytes;
}

// Append a byte
// _unsafe_ - avoid reserving (may attempt to write past buffer if there is not enough space!)
template <any_stack_dynamic_buffer T>
void append(T &buffer, byte b, bool unsafe = false) { insert(buffer, buffer.Count, b, unsafe); }

// Append _count_ bytes of string contained in _data_
// _unsafe_ - avoid reserving (may attempt to write past buffer if there is not enough space!)
template <any_stack_dynamic_buffer T>
void append_pointer_and_size(T &buffer, const byte *data, s64 count, bool unsafe = false) {
    insert_pointer_and_size(buffer, buffer.Count, data, count, unsafe);
}

// Append one view to another
// _unsafe_ - avoid reserving (may attempt to write past buffer if there is not enough space!)
template <any_stack_dynamic_buffer T>
void append_array(T &buffer, const array<byte> &view, bool unsafe = false) {
    append_pointer_and_size(buffer, view.Data, view.Count, unsafe);
}

// Append a list
// _unsafe_ - avoid reserving (may attempt to write past buffer if there is not enough space!)
template <any_stack_dynamic_buffer T>
void append_list(T &buffer, const initializer_list<byte> &list, bool unsafe = false) {
    append_pointer_and_size(buffer, list.begin(), list.size(), unsafe);
}

LSTD_END_NAMESPACE