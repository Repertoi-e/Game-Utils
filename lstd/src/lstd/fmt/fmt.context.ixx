module;

#include "../memory/string_builder.h"
#include "../types/numeric_info.h"
#include "format_float.inl"

export module fmt.context;

import fmt.arg;
import fmt.specs;
import fmt.parse_context;

import lstd.io;

LSTD_BEGIN_NAMESPACE

export {
    // This writer is kinda specific.
    // We have a pointer (_Out_) to a writer that gets the writes with certain formatting behaviour.
    //
    // We implement write() to take format specs into account
    // (width, fill char, padding direction etc.) but we provide _write_no_specs(...)_
    // which directly call Out->write(...).
    //
    // This object also has functions for writing pointers, integers and floats.
    // You can use this without a format string but just directly to write formatted stuff to an output writer.
    //
    // The reason we combine the writer and the "context" (the arguments and the parsed specs) is
    // because it's convenient to have everything in one place (e.g. when writing custom formatters).
    struct fmt_context : writer {
        writer *Out;  // The real output

        fmt_parse_context Parse;  // Holds the format string (and how much we've parsed) and some
                                  // state about the argument ids (when using automatic indexing).
        fmt_args Args;            // Storage for the arguments (gets set by the constructor)

        // null if no specs were parsed.
        // When writing a custom formatter use this for checking specifiers.
        // e.g.
        //     if (f->Specs && f->Specs->Hash) { ... }
        //
        // These are "dynamic" format specs because width or precision might have
        // been specified by another argument (instead of being a literal in the format string).
        dynamic_format_specs *Specs = null;

        fmt_context(writer *out = null, const string &fmtString = "", const fmt_args &args = {}) : Out(out), Args(args), Parse(fmtString) {}

        void write(const byte *data, s64 count) override;
        void flush() override { Out->flush(); }

        // The position tells where to point the caret in the format string, so it is clear where exactly the error happened.
        // If left as -1 we calculate using the current Parse.It.
        //
        // (We may want to pass a different position if we are in the middle of parsing and the It is not pointing at the right place).
        //
        // This is only used to provide useful error messages.
        inline void on_error(const string &message, s64 position = -1) { Parse.on_error(message, position); }
    };

    // Writes an unsigned integer with given formatting specs.
    // We format signed integers by writing "-" if the integer is negative and then calling this routine as if the integer is positive.
    void write_u64(fmt_context * f, u64 value, bool negative, fmt_specs specs);

    // Writes a float with given formatting specs.
    // The float formatting routine we used is based on stb_sprintf.
    void write_f64(fmt_context * f, f64 value, fmt_specs specs);

    // We need this overload for fmt_context because otherwise the pointer overload
    // of write_no_specs gets chosen (utf8* gets casted automatically to void*.. sigh!)
    inline void write(fmt_context * f, const utf8 *str) { f->write((const byte *) str, c_string_length(str)); }
    inline void write(fmt_context * f, const char8_t *str) { f->write((const byte *) str, c_string_length(str)); }

    // General formatting routimes which take specifiers into account:
    template <types::is_integral T>
    void write(fmt_context * f, T value);
    template <types::is_floating_point T>
    void write(fmt_context * f, T value);
    void write(fmt_context * f, bool value);
    void write(fmt_context * f, const void *value);

    // These routines write the value directly, without looking at formatting specs.
    // Useful when writing a custom formatter and there were specifiers but they
    // shouldn't propagate downwards when printing simpler types.
    template <types::is_integral T>
    void write_no_specs(fmt_context * f, T value);
    template <types::is_floating_point T>
    void write_no_specs(fmt_context * f, T value);
    inline void write_no_specs(fmt_context * f, bool value);
    inline void write_no_specs(fmt_context * f, const void *value);

    inline void write_no_specs(fmt_context * f, const string &str) { write(f->Out, *((array<byte> *) &str)); }

    // We need this overload for fmt_context because otherwise the pointer overload
    // of write_no_specs gets chosen (utf8* gets casted automatically to void*.. sigh!)
    inline void write_no_specs(fmt_context * f, const utf8 *str) { write(f->Out, (const byte *) str, c_string_length(str)); }
    inline void write_no_specs(fmt_context * f, const char8_t *str) { write(f->Out, (const byte *) str, c_string_length(str)); }

    inline void write_no_specs(fmt_context * f, const utf8 *str, s64 size) { write(f->Out, (const byte *) str, size); }
    inline void write_no_specs(fmt_context * f, utf32 cp) { write(f->Out, cp); }

    struct format_struct_helper;
    struct format_tuple_helper;
    struct format_list_helper;

    //
    // The following three classes are used to quickly collect elements and then output them in a pretty way.
    //
    // e.g. usage for a custom quaternion formatter:
    //     ...
    //     fmt_tuple(f, "quat").field(src.s)->field(src.i)->field(src.j)->field(src.k)->finish();
    // Outputs: "quat(1.00, 2.00, 3.00, 4.00)"
    //
    // These are inspired by Rust's API <3

    // Outputs in the following format: *name* { field1: value, field2: value, ... }
    // e.g.     vector3(x: 1.00, y: 4.00, z: 9.00)
    template <typename FC>
    struct format_struct {
        struct field_entry {
            string Name;
            fmt_arg<FC> Arg;
        };

        fmt_context *F;
        string Name;
        array<field_entry> Fields;
        bool NoSpecs;  // Write the result without taking into account specs for individual arguments

        format_struct(fmt_context *f, const string &name, bool noSpecs = false) : F(f), Name(name), NoSpecs(noSpecs) {}

        // I know we are against hidden freeing but having this destructor is fine because it helps with code conciseness.
        ~format_struct() { free(Fields); }

        template <typename T>
        format_struct *field(const string &name, const T &value) {
            append(Fields, {name, fmt_make_arg<FC>(value)});
            return this;
        }

        void finish();
    };

    // Outputs in the following format: *name*(element1, element2, ...)
    // e.g.     read_file_result("Hello world!", true)
    template <typename FC>
    struct format_tuple {
        FC *F;
        string Name;
        array<fmt_arg<FC>> Fields;
        bool NoSpecs;  // Write the result without taking into account specs for individual arguments

        format_tuple(FC *f, const string &name, bool noSpecs = false) : F(f), Name(name), NoSpecs(noSpecs) {}

        // I know we are against hidden freeing but having this destructor is fine because it helps with code conciseness.
        ~format_tuple() { free(Fields); }

        template <typename T>
        format_tuple *field(const T &value) {
            append(Fields, fmt_make_arg<FC>(value));
            return this;
        }

        void finish();
    };

    // Outputs in the following format: [element1, element2, ...]
    // e.g.     ["This", "is", "an", "array", "of", "strings"]
    template <typename FC>
    struct format_list {
        FC *F;
        array<fmt_arg<FC>> Fields;
        bool NoSpecs;  // Write the result without taking into account specs for individual arguments

        format_list(FC *f, bool noSpecs = false) : F(f), NoSpecs(noSpecs) {}

        // I know we are against hidden freeing but having this destructor is fine because it helps with code conciseness.
        ~format_list() { free(Fields); }

        template <typename T>
        format_list *entries(const array_view<T> &values) {
            For(values) append(Fields, fmt_make_arg<FC>(it));
            return this;
        }

        template <typename T>
        format_list *entries(T *begin, T *end) {
            return entries(array_view<T>(begin, end - begin));
        }

        template <typename T>
        format_list *entries(T *begin, s64 count) {
            return entries(array_view<T>(begin, count));
        }

        void finish();
    };

    //
    // The implementations of the above functions follow:
    //

    template <types::is_integral T>
    void write(fmt_context * f, T value) {
        u64 absValue = (u64) value;
        bool negative = sign_bit(value);
        if (negative) absValue = 0 - absValue;

        if (f->Specs) {
            write_u64(f, absValue, negative, *f->Specs);
        } else {
            write_u64(f, absValue, negative, {});
        }
    }

    template <types::is_floating_point T>
    void write(fmt_context * f, T value) {
        if (f->Specs) {
            write_f64(f, (f64) value, *f->Specs);
        } else {
            write_f64(f, (f64) value, {});
        }
    }

    template <types::is_integral T>
    void write_no_specs(fmt_context * f, T value) {
        u64 absValue = (u64) value;
        bool negative = sign_bit(value);
        if (negative) absValue = 0 - absValue;
        write_u64(f, absValue, negative, {});
    }

    template <types::is_floating_point T>
    void write_no_specs(fmt_context * f, T value) {
        write_f64(f, (f64) value, {});
    }

    inline void write_no_specs(fmt_context * f, bool value) { write_no_specs(f, value ? 1 : 0); }

    inline void write_no_specs(fmt_context * f, const void *value) {
        auto *old = f->Specs;
        f->Specs = null;
        write(f, value);
        f->Specs = old;
    }

    // Used to dispatch values to write/write_no_specs functions. Used in conjunction with fmt_visit_fmt_arg.
    template <typename FC>
    struct fmt_context_visitor {
        FC *F;
        bool NoSpecs;

        fmt_context_visitor(FC *f, bool noSpecs = false) : F(f), NoSpecs(noSpecs) {}

        void operator()(s32 value) { NoSpecs ? write_no_specs(F, value) : write(F, value); }
        void operator()(u32 value) { NoSpecs ? write_no_specs(F, value) : write(F, value); }
        void operator()(s64 value) { NoSpecs ? write_no_specs(F, value) : write(F, value); }
        void operator()(u64 value) { NoSpecs ? write_no_specs(F, value) : write(F, value); }
        void operator()(bool value) { NoSpecs ? write_no_specs(F, value) : write(F, value); }
        void operator()(f64 value) { NoSpecs ? write_no_specs(F, value) : write(F, value); }
        void operator()(const string &value) { NoSpecs ? write_no_specs(F, value) : write(F, value); }
        void operator()(const void *value) { NoSpecs ? write_no_specs(F, value) : write(F, value); }
        void operator()(const typename fmt_value<FC>::custom &custom) { custom.FormatFunc(custom.Data, F); }

        void operator()(types::unused) {
            F->on_error("Internal error while formatting");
            assert(false);
        }
    };

    template <typename FC>
    void format_struct<FC>::finish() {
        auto write_field = [&](field_entry *entry) {
            write_no_specs(F, entry->Name);
            write_no_specs(F, ": ");
            fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), entry->Arg);
        };

        write_no_specs(F, Name);
        write_no_specs(F, " {");

        auto *begin = Fields.begin();
        if (begin != Fields.end()) {
            write_no_specs(F, " ");
            write_field(begin);
            ++begin;
            while (begin != Fields.end()) {
                write_no_specs(F, ", ");
                write_field(begin);
                ++begin;
            }
        }
        write_no_specs(F, " }");
    }

    template <typename FC>
    void format_tuple<FC>::finish() {
        write_no_specs(F, Name);
        write_no_specs(F, "(");

        auto *begin = Fields.begin();
        if (begin != Fields.end()) {
            fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), *begin);
            ++begin;
            while (begin != Fields.end()) {
                write_no_specs(F, ", ");
                fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), *begin);
                ++begin;
            }
        }
        write_no_specs(F, ")");
    }

    template <typename FC>
    void format_list<FC>::finish() {
        write_no_specs(F, "[");

        auto *begin = Fields.begin();
        if (begin != Fields.end()) {
            fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), *begin);
            ++begin;
            while (begin != Fields.end()) {
                write_no_specs(F, ", ");
                fmt_visit_fmt_arg(fmt_context_visitor(F, NoSpecs), *begin);
                ++begin;
            }
        }
        write_no_specs(F, "]");
    }
}

utf8 DIGITS[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

template <typename UInt>
utf8 *format_uint_decimal(utf8 *buffer, UInt value, s64 formattedSize, const string &thousandsSep = "") {
    u32 digitIndex = 0;

    buffer += formattedSize;
    while (value >= 100) {
        u32 index = (u32)(value % 100) * 2;
        value /= 100;
        *--buffer = DIGITS[index + 1];
        if (++digitIndex % 3 == 0) {
            buffer -= thousandsSep.Count;
            copy_memory(buffer, thousandsSep.Data, thousandsSep.Count);
        }
        *--buffer = DIGITS[index];
        if (++digitIndex % 3 == 0) {
            buffer -= thousandsSep.Count;
            copy_memory(buffer, thousandsSep.Data, thousandsSep.Count);
        }
    }

    if (value < 10) {
        *--buffer = (utf8)('0' + value);
        return buffer;
    }

    u32 index = (u32) value * 2;
    *--buffer = DIGITS[index + 1];
    if (++digitIndex % 3 == 0) {
        buffer -= thousandsSep.Count;
        copy_memory(buffer, thousandsSep.Data, thousandsSep.Count);
    }
    *--buffer = DIGITS[index];

    return buffer;
}

template <u32 BASE_BITS, typename UInt>
utf8 *format_uint_base(utf8 *buffer, UInt value, s64 formattedSize, bool upper = false) {
    buffer += formattedSize;
    do {
        const utf8 *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
        u32 digit = (value & ((1 << BASE_BITS) - 1));
        *--buffer = (utf8)(BASE_BITS < 4 ? (utf8)('0' + digit) : digits[digit]);
    } while ((value >>= BASE_BITS) != 0);
    return buffer;
}

// Writes pad code points and the actual contents with f(),
// _fSize_ needs to be the size of the output from _f_ in code points (in order to calculate padding properly)
template <typename F>
void write_padded_helper(fmt_context *f, const fmt_specs &specs, F &&func, s64 fSize) {
    u32 padding = (u32)(specs.Width > fSize ? specs.Width - fSize : 0);
    if (specs.Align == fmt_alignment::RIGHT) {
        For(range(padding)) write_no_specs(f, specs.Fill);
        func();
    } else if (specs.Align == fmt_alignment::CENTER) {
        u32 leftPadding = padding / 2;
        For(range(leftPadding)) write_no_specs(f, specs.Fill);
        func();
        For(range(padding - leftPadding)) write_no_specs(f, specs.Fill);
    } else {
        func();
        For(range(padding)) write_no_specs(f, specs.Fill);
    }
}

void write_helper(fmt_context *f, const byte *data, s64 size) {
    if (!f->Specs) {
        write_no_specs(f, (const utf8 *) data, size);
        return;
    }

    if (f->Specs->Type) {
        if (f->Specs->Type == 'p') {
            write(f, (const void *) data);
            return;
        }
        if (f->Specs->Type != 's') {
            f->on_error("Invalid type specifier for a string", f->Parse.It.Data - f->Parse.FormatString.Data - 1);
            return;
        }
    }

    // 'p' wasn't specified, not treating as formatting a pointer
    s64 length = utf8_length((const utf8 *) data, size);

    // Adjust size for specified precision
    if (f->Specs->Precision != -1) {
        assert(f->Specs->Precision >= 0);
        length = f->Specs->Precision;
        size = get_cp_at_index((const utf8 *) data, length, length, true) - (const utf8 *) data;
    }
    write_padded_helper(
        f, *f->Specs, [&]() { write_no_specs(f, (const utf8 *) data, size); }, length);
}

void fmt_context::write(const byte *data, s64 count) { write_helper(this, data, count); }

// @ThreadSafety ???
utf8 U64_FORMAT_BUFFER[numeric_info<u64>::digits10 + 1];

void write(fmt_context *f, bool value) {
    if (f->Specs && f->Specs->Type) {
        write(f, value ? 1 : 0);
    } else {
        write(f, value ? "true" : "false");
    }
}

void write(fmt_context *f, const void *value) {
    if (f->Specs && f->Specs->Type && f->Specs->Type != 'p') {
        f->on_error("Invalid type specifier for a pointer", f->Parse.It.Data - f->Parse.FormatString.Data - 1);
        return;
    }

    auto uptr = bit_cast<u64>(value);
    u32 numDigits = count_digits<4>(uptr);

    auto func = [&, f]() {
        write_no_specs(f, U'0');
        write_no_specs(f, U'x');

        utf8 formatBuffer[numeric_info<u64>::digits / 4 + 2];
        auto *p = format_uint_base<4>(formatBuffer, uptr, numDigits);
        write_no_specs(f, p, formatBuffer + numDigits - p);
    };

    if (!f->Specs) {
        func();
        return;
    }

    fmt_specs specs = *f->Specs;
    if (specs.Align == fmt_alignment::NONE) specs.Align = fmt_alignment::RIGHT;
    write_padded_helper(f, specs, func, numDigits + 2);
}

void write_u64(fmt_context *f, u64 value, bool negative, fmt_specs specs) {
    utf8 type = specs.Type;
    if (!type) type = 'd';

    s64 numDigits;
    if (type == 'd' || type == 'n') {
        numDigits = count_digits(value);
    } else if (to_lower(type) == 'b') {
        numDigits = count_digits<1>(value);
    } else if (type == 'o') {
        numDigits = count_digits<3>(value);
    } else if (to_lower(type) == 'x') {
        numDigits = count_digits<4>(value);
    } else if (type == 'c') {
        if (specs.Align == fmt_alignment::NUMERIC || specs.Sign != fmt_sign::NONE || specs.Hash) {
            f->on_error("Invalid format specifier(s) for code point - code points can't have numeric alignment, signs or #", f->Parse.It.Data - f->Parse.FormatString.Data);
            return;
        }
        auto cp = (utf32) value;
        write_padded_helper(
            f, specs, [&]() { write_no_specs(f, cp); }, get_size_of_cp(cp));
        return;
    } else {
        f->on_error("Invalid type specifier for an integer", f->Parse.It.Data - f->Parse.FormatString.Data - 1);
        return;
    }

    utf8 prefixBuffer[4];
    utf8 *prefixPointer = prefixBuffer;

    if (negative) {
        *prefixPointer++ = '-';
    } else if (specs.Sign == fmt_sign::PLUS) {
        *prefixPointer++ = '+';
    } else if (specs.Sign == fmt_sign::SPACE) {
        *prefixPointer++ = ' ';
    }

    if ((to_lower(type) == 'x' || to_lower(type) == 'b') && specs.Hash) {
        *prefixPointer++ = '0';
        *prefixPointer++ = type;
    }

    // Octal prefix '0' is counted as a digit,
    // so only add it if precision is not greater than the number of digits.
    if (type == 'o' && specs.Hash) {
        if (specs.Precision == -1 || specs.Precision > numDigits) *prefixPointer++ = '0';
    }

    auto prefix = string(prefixBuffer, prefixPointer - prefixBuffer);

    s64 formattedSize = prefix.Length + numDigits;
    s64 padding = 0;
    if (specs.Align == fmt_alignment::NUMERIC) {
        if (specs.Width > formattedSize) {
            padding = specs.Width - formattedSize;
            formattedSize = specs.Width;
        }
    } else if (specs.Precision > (s32) numDigits) {
        formattedSize = (u32) prefix.Length + (u32) specs.Precision;
        padding = (u32) specs.Precision - numDigits;
        specs.Fill = '0';
    }
    if (specs.Align == fmt_alignment::NONE) specs.Align = fmt_alignment::RIGHT;

    type = (utf8) to_lower(type);
    if (type == 'd') {
        write_padded_helper(
            f, specs,
            [&]() {
                if (prefix.Length) write_no_specs(f, prefix);
                For(range(padding)) write_no_specs(f, specs.Fill);
                auto *p = format_uint_decimal(U64_FORMAT_BUFFER, value, numDigits);
                write_no_specs(f, p, U64_FORMAT_BUFFER + numDigits - p);
            },
            formattedSize);
    } else if (type == 'b') {
        write_padded_helper(
            f, specs,
            [&]() {
                if (prefix.Length) write_no_specs(f, prefix);
                For(range(padding)) write_no_specs(f, specs.Fill);
                auto *p = format_uint_base<1>(U64_FORMAT_BUFFER, value, numDigits);
                write_no_specs(f, p, U64_FORMAT_BUFFER + numDigits - p);
            },
            formattedSize);
    } else if (type == 'o') {
        write_padded_helper(
            f, specs,
            [&]() {
                if (prefix.Length) write_no_specs(f, prefix);
                For(range(padding)) write_no_specs(f, specs.Fill);
                auto *p = format_uint_base<3>(U64_FORMAT_BUFFER, value, numDigits);
                write_no_specs(f, p, U64_FORMAT_BUFFER + numDigits - p);
            },
            formattedSize);
    } else if (type == 'x') {
        write_padded_helper(
            f, specs,
            [&]() {
                if (prefix.Length) write_no_specs(f, prefix);
                For(range(padding)) write_no_specs(f, specs.Fill);
                auto *p = format_uint_base<4>(U64_FORMAT_BUFFER, value, numDigits, is_upper(specs.Type));
                write_no_specs(f, p, U64_FORMAT_BUFFER + numDigits - p);
            },
            formattedSize);
    } else if (type == 'n') {
        formattedSize += ((numDigits - 1) / 3);
        write_padded_helper(
            f, specs,
            [&]() {
                if (prefix.Length) write_no_specs(f, prefix);
                For(range(padding)) write_no_specs(f, specs.Fill);
                auto *p = format_uint_decimal(U64_FORMAT_BUFFER, value, formattedSize, "," /*@Locale*/);
                write_no_specs(f, p, U64_FORMAT_BUFFER + formattedSize - p);
            },
            formattedSize);
    } else {
        assert(false && "Invalid type");  // sanity
    }
}

// Writes a float with given formatting specs
void write_f64(fmt_context *f, f64 value, fmt_specs specs) {
    utf8 type = specs.Type;
    if (type) {
        utf8 lower = (utf8) to_lower(type);
        if (lower != 'g' && lower != 'e' && lower != '%' && lower != 'f' && lower != 'a') {
            f->on_error("Invalid type specifier for a float", f->Parse.It.Data - f->Parse.FormatString.Data - 1);
            return;
        }
    } else {
        type = 'g';
    }

    bool percentage = specs.Type == '%';

    utf32 sign = 0;

    ieee754_f64 bits;
    bits.F = value;

    // Check the sign bit instead of value < 0 since the latter is always false for NaN
    if (bits.ieee.S) {
        sign = '-';
        value = -value;
    } else if (specs.Sign == fmt_sign::PLUS) {
        sign = '+';
    } else if (specs.Sign == fmt_sign::SPACE) {
        sign = ' ';
    }

    // Handle INF or NAN
    if (bits.ieee.E == 2047) {
        write_padded_helper(
            f, specs,
            [&]() {
                if (sign) write_no_specs(f, sign);
                write_no_specs(f, (bits.DW & ((1ll << 52) - 1)) ? (is_upper(specs.Type) ? "NAN" : "nan") : (is_upper(specs.Type) ? "INF" : "inf"));
                if (percentage) write_no_specs(f, U'%');
            },
            3 + (sign ? 1 : 0) + (percentage ? 1 : 0));
        return;
    }

    if (percentage) {
        value *= 100;
        type = 'f';
    }

    // @Locale The decimal point written in _internal::format_float_ should be locale-dependent.
    // Also if we decide to add a thousands separator we should do it inside _format_float_
    string_builder formatBuffer;
    defer(free(formatBuffer));

    format_float(
        [](void *user, utf8 *buf, s64 length) {
            auto *sb = (string_builder *) user;
            sb->BaseBuffer.Occupied += length;
            if (sb->BaseBuffer.Occupied > string_builder::BUFFER_SIZE) return (utf8 *) null;
            return buf + length;
        },
        &formatBuffer, formatBuffer.BaseBuffer.Data, type, value, specs.Precision);

    // Note: We set _type_ to 'g' if it's zero, but here we check specs.Type (which we didn't modify)
    // This is because '0' is similar to 'g', except that it prints at least one digit after the decimal point,
    // which we do here (python-like formatting)
    if (!specs.Type) {
        auto *p = formatBuffer.BaseBuffer.Data;
        auto *end = p + formatBuffer.BaseBuffer.Occupied;
        while (p < end && is_digit(*p)) ++p;
        if (p < end && to_lower(*p) != 'e') {
            ++p;
            if (*p == '0') ++p;
            while (p != end && *p >= '1' && *p <= '9') ++p;

            utf8 *where = p;
            while (p != end && *p == '0') ++p;

            if (p == end || !is_digit(*p)) {
                if (p != end) copy_memory(where, p, (s64)(end - p));
                formatBuffer.BaseBuffer.Occupied -= (s64)(p - where);
            }
        } else if (p == end) {
            // There was no dot at all
            append_pointer_and_size(formatBuffer, ".0", 2);
        }
    }

    if (percentage) append_cp(formatBuffer, '%');

    if (specs.Align == fmt_alignment::NUMERIC) {
        if (sign) {
            write_no_specs(f, sign);
            sign = 0;
            if (specs.Width) --specs.Width;
        }
        specs.Align = fmt_alignment::RIGHT;
    } else if (specs.Align == fmt_alignment::NONE) {
        specs.Align = fmt_alignment::RIGHT;
    }

    auto formattedSize = formatBuffer.BaseBuffer.Occupied + (sign ? 1 : 0);
    write_padded_helper(
        f, specs,
        [&]() {
            if (sign) write_no_specs(f, sign);
            write_no_specs(f, (utf8 *) formatBuffer.BaseBuffer.Data, formatBuffer.BaseBuffer.Occupied);
        },
        formattedSize);
}

LSTD_END_NAMESPACE
