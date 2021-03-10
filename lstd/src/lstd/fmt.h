#pragma once

#include "io.h"
#include "math.h"
#include "memory/guid.h"
#include "memory/string_builder.h"

//
// This file includes useful formatters for some complex types:
//     guid, arrays, math types, etc.
//
// This means you can use them as format arguments directly.
//
// The reason it is a header file is because on 04.02.2021, VS has 
// a bug where template specializations aren't picked up from
// separate modules. Hopefully this gets fixed in the future.
//

import fmt;

LSTD_BEGIN_NAMESPACE

template <>
struct formatter<string_builder> {
    void format(const string_builder &src, fmt_context *f) {
        auto *buffer = &src.BaseBuffer;
        while (buffer) {
            write_no_specs(f, buffer->Data, buffer->Occupied);
            buffer = buffer->Next;
        }
    }
};

// Formats GUID in the following way: 00000000-0000-0000-0000-000000000000
// Allows specifiers:
//   'n' - 00000000000000000000000000000000
//   'N' - Uppercase version of 'n'
//   'd' - 00000000-0000-0000-0000-000000000000
//   'D' - Uppercase version of 'd'
//   'b' - {00000000-0000-0000-0000-000000000000}
//   'B' - Uppercase version of 'b'
//   'p' - (00000000-0000-0000-0000-000000000000)
//   'P' - Uppercase version of 'p'
//   'x' - {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
//   'X' - Uppercase version of 'x'
// The default format is the same as 'd'.
template <>
struct formatter<guid> {
    void format(const guid &src, fmt_context *f) {
        char type = 'd';
        if (f->Specs) {
            type = f->Specs->Type;
        }

        bool upper = is_upper(type);
        type = (char) to_lower(type);

        if (type != 'n' && type != 'd' && type != 'b' && type != 'p' && type != 'x') {
            f->on_error("Invalid type specifier for a guid", f->Parse.It.Data - f->Parse.FormatString.Data - 1);
            return;
        }

        utf32 openParenthesis = 0, closedParenthesis = 0;
        bool hyphen = true;

        if (type == 'n') {
            hyphen = false;
        } else if (type == 'b') {
            openParenthesis = '{';
            closedParenthesis = '}';
        } else if (type == 'p') {
            openParenthesis = '(';
            closedParenthesis = ')';
        } else if (type == 'x') {
            auto *old = f->Specs;
            f->Specs = null;

            u8 *p = (u8 *) src.Data.Data;
            if (upper) {
                fmt_to_writer(f, "{{{:#04X}{:02X}{:02X}{:02X},{:#04X}{:02X},{:#04X}{:02X},{{{:#04X},{:#04X},{:#04X},{:#04X},{:#04X},{:#04X},{:#04X},{:#04X}}}}}",
                              p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[16]);
            } else {
                fmt_to_writer(f, "{{{:#04x}{:02x}{:02x}{:02x},{:#04x}{:02x},{:#04x}{:02x},{{{:#04x},{:#04x},{:#04x},{:#04x},{:#04x},{:#04x},{:#04x},{:#04x}}}}}",
                              p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[16]);
            }

            f->Specs = old;
            return;
        }

        if (openParenthesis) write_no_specs(f, openParenthesis);

        auto *old = f->Specs;
        f->Specs = null;

        const byte *p = src.Data.Data;
        For(range(16)) {
            if (hyphen && (it == 4 || it == 6 || it == 8 || it == 10)) {
                write_no_specs(f, (utf32) '-');
            }
            if (upper) {
                fmt_to_writer(f, "{:02X}", (u8) *p);
            } else {
                fmt_to_writer(f, "{:02x}", (u8) *p);
            }
            ++p;
        }
        f->Specs = old;

        if (closedParenthesis) write_no_specs(f, closedParenthesis);
    }
};

// Formatts array in the following way: [1, 2, ...]
template <typename T>
struct formatter<array<T>> {
    void format(const array<T> &src, fmt_context *f) { format_list(f).entries(src.Data, src.Count)->finish(); }
};

// Formatts stack array in the following way: [1, 2, ...]
template <typename T, s64 N>
struct formatter<stack_array<T, N>> {
    void format(const stack_array<T, N> &src, fmt_context *f) { format_list(f).entries(src.Data, src.Count)->finish(); }
};

template <>
struct formatter<thread::id> {
    void format(thread::id src, fmt_context *f) { write(f, src.Value); }
};

//
// Formatters for math types:
//

// Formats vector in the following way: [1, 2, ...]
template <typename T, s32 Dim, bool Packed>
struct formatter<vec<T, Dim, Packed>> {
    void format(const vec<T, Dim, Packed> &src, fmt_context *f) {
        format_list(f).entries(src.Data, src.DIM)->finish();
    }
};

// Formats in the following way: [ 1, 2, 3; 4, 5, 6; 7, 8, 9]
// Alternate (using # specifier):
// [  1,   2,   3
//    3,  41,   5
//  157,   8,   9]
template <typename T, s64 R, s64 C, bool Packed>
struct formatter<mat<T, R, C, Packed>> {
    void format(const mat<T, R, C, Packed> &src, fmt_context *f) {
        write(f, "[");

        bool alternate = f->Specs && f->Specs->Hash;
        s64 max = 0;
        if (alternate) {
            for (s32 i = 0; i < src.Height; ++i) {
                for (s32 j = 0; j < src.Width; ++j) {
                    s64 s;
                    if constexpr (types::is_floating_point<T>) {
                        s = fmt_calculate_length("{:f}", src(i, j));
                    } else {
                        s = fmt_calculate_length("{}", src(i, j));
                    }
                    if (s > max) max = s;
                }
            }
        }

        auto *old = f->Specs;
        f->Specs = null;
        for (s32 i = 0; i < src.Height; ++i) {
            for (s32 j = 0; j < src.Width; ++j) {
                if (alternate) {
                    if constexpr (types::is_floating_point<T>) {
                        fmt_to_writer(f, "{0:<{1}f}", src(i, j), max);
                    } else {
                        fmt_to_writer(f, "{0:<{1}}", src(i, j), max);
                    }
                } else {
                    if constexpr (types::is_floating_point<T>) {
                        fmt_to_writer(f, "{0:f}", src(i, j));
                    } else {
                        fmt_to_writer(f, "{0:}", src(i, j));
                    }
                }
                if (j != src.Width - 1) write(f, ", ");
            }
            if (i < src.R - 1) write(f, alternate ? "\n " : "; ");
        }
        f->Specs = old;
        write(f, "]");
    }
};

// (This is for mat views)
// Formats in the following way: [ 1, 2, 3; 4, 5, 6; 7, 8, 9]
// Alternate (using # specifier):
// [  1,   2,   3
//    3,  41,   5
//  157,   8,   9]
template <typename T, s64 R, s64 C, bool Packed, s64 SR, s64 SC>
struct formatter<mat_view<mat<T, R, C, Packed>, SR, SC>> {
    void format(const mat_view<mat<T, R, C, Packed>, SR, SC> &src, fmt_context *f) {
        mat<T, SR, SC, Packed> v = src;
        fmt_to_writer(f, "{}", v);  // yES. We are lazy.
    }
};

// Formats in the following way: quat(1, 0, 0, 0)
// Alternate (using # specifier): [ 60 deg @ [0, 1, 0] ] (rotation in degrees around axis)
template <typename T, bool Packed>
struct formatter<tquat<T, Packed>> {
    void format(const tquat<T, Packed> &src, fmt_context *f) {
        bool alternate = f->Specs && f->Specs->Hash;
        if (alternate) {
            write(f, "[");
            fmt_to_writer(f, "{.f}", src.angle() / TAU * 360);
            write(f, " deg @ ");
            fmt_to_writer(f, "{}", src.axis());
            write(f, "]");
        } else {
            format_tuple(f, "quat").field(src.s)->field(src.i)->field(src.j)->field(src.k)->finish();
        }
    }
};

LSTD_END_NAMESPACE
