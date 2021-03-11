module;

#include "internal/common.h"
#include "memory/string_builder.h"

export module lstd.io;

LSTD_BEGIN_NAMESPACE

export {
    // Provides a way to write types and bytes with a simple extension API.
    // Subclasses of this must override the write/flush methods depending on the output (console, files, buffers, etc.)
    //
    // Note: One specific kind of writer is fmt_context.
    // You can use it to convert arbitrary types to strings (custom types are formatted using a custom formatter).
    struct writer {
        writer() {}
        virtual ~writer() {}

        virtual void write(const byte *data, s64 count) = 0;
        virtual void flush() {}
    };

    //
    // Use these overloads instead of calling writer->write() directly.
    //

    void write(writer * w, const byte *data, s64 size) { w->write(data, size); }
    void write(writer * w, const bytes &data) { w->write(data.Data, data.Count); }
    void write(writer * w, const string &str) { w->write((byte *) str.Data, str.Count); }

    void write(writer * w, utf32 cp) {
        utf8 data[4];
        encode_cp(data, cp);
        w->write((byte *) data, get_size_of_cp(data));
    }

    void flush(writer * w) { w->flush(); }

    //
    // String writer:
    //

    // string_builder is a type that has a large stack buffer for writing out characters before allocating dynamic memory.
    struct string_writer : writer {
        string_builder Builder;

        string_writer() {}

        void write(const byte *data, s64 size) override {
            //
            // @Robustness: Optional utf8 validation would be good here?
            //
            append_pointer_and_size(Builder, (const utf8 *) data, size);
        }
    };

    // You must free the builder (in case it has allocated extra buffers)
    void free(string_writer & writer) {
        free(writer.Builder);
    }

    // This writer counts how many bytes would have been written. The actual data is discarded.
    struct counting_writer : writer {
        s64 Count = 0;
        void write(const byte *, s64 size) override { Count += size; }
    };

    //
    // Console writer:
    //
    struct console_writer : writer {
        // By default, we are thread-safe.
        // If you don't use seperate threads and aim for maximum console output performance, set this to false.
        bool LockMutex = true;

        byte *Buffer, *Current;
        s64 Available = 0, BufferSize = 0;

        enum output_type {
            COUT,
            CERR
        };

        output_type OutputType;

        console_writer(output_type type) : OutputType(type) {}

        // @Platform Defined in e.g. windows_common.cpp
        void write(const byte *data, s64 size) override;
        void flush() override;
    };

    auto cout = console_writer(console_writer::COUT);
    auto cerr = console_writer(console_writer::CERR);
}

LSTD_END_NAMESPACE
