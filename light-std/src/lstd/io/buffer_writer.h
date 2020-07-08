#pragma once

#include "../memory/stack_dynamic_buffer.h"
#include "writer.h"

LSTD_BEGIN_NAMESPACE

namespace io {

template <s64 N>
void buffer_writer_write(writer *w, const char *data, s64 count);

template <s64 N>
void buffer_writer_flush(writer *w);

template <s64 N>
struct buffer_writer : writer {
    stack_dynamic_buffer<N> *StackDynamicBuffer;

    buffer_writer(stack_dynamic_buffer<N> *buffer)
        : writer(buffer_writer_write<N>, buffer_writer_flush<N>), StackDynamicBuffer(buffer) {
        Buffer = Current = buffer->Data;
        BufferSize = Available = sizeof(buffer->StackData);
    }
};

template <s64 N>
void buffer_writer_write(writer *w, const char *data, s64 count) {
    auto *bw = (buffer_writer<N> *) w;

    if (count > bw->Available) {
        w->write(data, bw->Available);
        data += bw->Available;
        count -= bw->Available;

        bw->flush();
    }

    copy_memory(bw->Current, data, count);
    bw->Current += count;
    bw->Available -= count;
}

template <s64 N>
void buffer_writer_flush(writer *w) {
    auto *bw = (buffer_writer<N> *) w;

    auto *dynBuf = bw->StackDynamicBuffer;
    dynBuf->append_pointer_and_size(bw->Buffer, bw->BufferSize - bw->Available);
    bw->Buffer = bw->Current = dynBuf->Data + dynBuf->ByteLength;

    if (dynBuf->Reserved) {
        bw->Available = dynBuf->Reserved - dynBuf->ByteLength;
    } else {
        bw->Available = sizeof(dynBuf->StackData) - dynBuf->ByteLength;
    }
    bw->BufferSize = bw->Available;
}

}  // namespace io

LSTD_END_NAMESPACE
