module;

#include "internal/context.h"
#include "io.h"
#include "math.h"
#include "memory/guid.h"

export module fmt;

//
// Format specification:
//
// The formatting engine in this library is similar to how python handles advanced string formatting.
//
// Format strings consist of characters and fields encoded in utf8.
// Fields define how an argument gets formatted to the output while the rest of the characters get transfered unchanged.
//
// Fields are defined with curly braces, like so:
//    print("This is an {}", "example") -> "This is an example"
// Braces can be escaped by doubling:
//    print("Hey there :-{{}}")         -> "Hey there :-{}"
//
// You can specify which argument a field refers to by index or by name.
//    print("{0} {1} {0}", "first", "second") -> "first second first"
//   Note: If you leave the braces without an index, by default it is automatically incremented
//         with each new field. However you may not switch between automatic and manual indexing
//         throughout the formatting string.
//
// Format specifiers follow the index, with ":" separating the two:
//    print("{:<8}", "Jon")  -> "     Jon"
//
// The general form of a standard format specifier is:
//     [[fill]align][sign][#][0][width][.precision][type]
// The brackets ([]) indicate an optional element.
//
//     The optional align flag is one of the following:
//       '<' - Forces the field to be left-aligned within the available space (default)
//       '>' - Forces the field to be right-aligned within the available space.
//       '=' - Forces the padding to be placed after the sign (if any)
//             but before the digits.  This is used for printing fields
//             in the form '+000000120'. This alignment option is only
//             valid for numeric types.
//       '^' - Forces the field to be centered within the available space
//
//     Note that unless a minimum field width is defined, the field width will always be the same size as the data to
//     fill it, so that the alignment option has no meaning in this case.
//
//     The optional 'fill' code point defines the code point to be used to pad the field to the minimum width.
//     The fill code point, if present, must be followed by an alignment flag.
//     The fill element can be multiple bytes and must be encoded in utf8.
//
//     The 'sign' option is only valid for numeric types, and can be one of the following:
//       '+'  - Indicates that a sign should be used for both positive as well as negative numbers
//       '-'  - Indicates that a sign should be used only for negative numbers (default)
//       ' '  - Indicates that a leading space should be used on positive numbers
//
//     If the '#' character is present, integers use the 'alternate form' for formatting.
//     This means that binary, octal, and hexadecimal output will be prefixed with '0b', '0o', and '0x', respectively.
//
//     'width' is a decimal integer defining the minimum field width.
//     If not specified, then the field width will be determined by the content.
//
//     If the width field is preceded by a zero('0') character, this enables zero - padding.
//     This is equivalent to an alignment type of '=' and a fill character of '0'.
//
//     The 'precision' is a decimal number indicating how many digits should be displayed after the decimal point in a
//     floating point conversion. For non-numeric types the field indicates the maximum field size - in other words, how
//     many characters will be used from the field content. The precision is ignored for integer conversions.
//
//     Finally, the 'type' byte determines how the data should be presented:
//
//      Integers:
//       'b' - Binary integer. Outputs the number in base 2.
//       'c' - Code point. Converts the integer to the corresponding unicode code point before printing.
//       'd' - Decimal integer. Outputs the number in base 10.
//       'o' - Octal format. Outputs the number in base 8.
//       'x' - Hex format. Outputs the number in base 16, using lower-case letters for the digits above 9.
//       'X' - Hex format. Outputs the number in base 16, using upper-case letters for the digits above 9.
//       'n' - Number. This is the same as 'd', except that it inserts thousands separator
//             (currently a dot that doesn't get determined by the locale)
//       '' (None) - the same as 'd'
//
//      Floats:
//       'e' - Exponent notation. Prints the number in scientific notation using 'e' for the exponent.
//       'E' - Uppercase version of 'e'.
//       'f' - Fixed point. Displays the number as a fixed-point number.
//       'F' - Uppercase version of 'f'.
//       'g' - General format. This prints the number as a fixed-point number, unless the number is too large,
//             in which case it switches to 'e' exponent notation.
//       'G' - Uppercase version of 'g'.
//       '%' - Percentage. Multiplies the number by 100 and displays in fixed ('f') format, followed by a percent sign.
//       '' (None) - similar to 'g', except that it prints at least one
//             digit after the decimal point.
//
//      Pointers:
//       'p' - Outputs a 'const void *' formatted in hexadecimal. Example: 0xab5c8fea84
//       '' (None) - the same as 'p'
//
//      C-Style string:
//       'p' - Treats the argument as a pointer.
//       's' - Outputs it as an utf8 encoded string.
//       '' (None) - the same as 's'
//
//      Strings:
//       's' - Outputs it as an utf8 encoded string.
//       '' (None) - the same as 's'
//
//      Guid:
//       'n' - 00000000000000000000000000000000
//       'N' - Uppercase version of 'n'
//       'd' - 00000000-0000-0000-0000-000000000000
//       'D' - Uppercase version of 'd'
//       'b' - {00000000-0000-0000-0000-000000000000}
//       'B' - Uppercase version of 'b'
//       'p' - (00000000-0000-0000-0000-000000000000)
//       'P' - Uppercase version of 'p'
//       'x' - {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
//       'X' - Uppercase version of 'x'
//       '' (None) - the same as 'd'
//
//
// There is also a way to specify text styles directly in the format string. Without requiring an argument.
// Text styles are defined by a opening curly brace ('{') followed by '!', then the text style and a closing brace ('}')
// An empty text style resets any foreground/background color and text emphasis.
//    print("{!}")
//
// There are 3 ways to define a text color. The color is optional but it must be the first thing after '!'.
//    1) Using the name of a color, e.g. {!CORNFLOWER_BLUE}
//       A full list of recognized colors is available in "lstd/io/fmt/colors.def"
//       and programatically in the color enum
//	        print("{!DARK_MAGENTA}")
//    2) Using the name of a "terminal" color. Use these colors if the console you are printing to doesn't support
//       24 bit true color. These are the most basic colors supported by almost any console.
//           BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE
//           and the bright versions: BRIGHT_BLACK, BRIGHT_RED, ...
//       In order to distinguish between 24-bit color and 4-bit, a leading 't' is required after the '!'
//	        print("{!tBRIGHT_CYAN}")
//    3) Using RGB. This gives you full control and ability to specify any 24-bit color.
//       Channels are parsed in the order red - green - blue and must be separated by ';'
//	        print("{!50;230;170}")
//       Values must be in the range [0-255]
//
// After the text color you can mark is as background using "BG":
//	        print("{!WHITE;BG}")
// That means the color applies to the background and not the foreground.
//
// If you didn't mark the color as background, you can specify a series of
// characters that define the emphasis style of the text.
//	        print("{!WHITE;BIUS}")
//    Here "BIUS" specifies all the types of emphasis:
//      B (bold), I (italic, rarely supported by consoles though), U (underline) and S (strikethrough)
//    They can be in any order and are optional.
//    e.g. valid emphasis strings are: "BI", "U", "B", "SU", "SB", etc...
//       Note: When parsing, if we fail to find the name of a color, e.g. {!IMAGINARYCOLOR}, we treat
//             the series of characters as emphasis, although any character encountered that is not a
//             valid emphasis gets reported as an error. This allows specifying emphasis without color:
//                 print("{!BU}");
//
// You can disable text styles with _Context.FmtDisableAnsiCodes_.
// That is useful when logging to a file and not a console. The ansi escape codes look like garbage in files.
//

export import fmt.arg;
export import fmt.parse_context;
export import fmt.context;
export import fmt.text_style;

LSTD_BEGIN_NAMESPACE

export {
    //
    // These are the most common functions.
    // If you are doing something specific, you can look
    // into the implementation details further down this file.
    //

    // Formats to a writer.
    template <typename... Args>
    void fmt_to_writer(writer * out, const string &fmtString, Args &&...arguments);

    // Formats to a counting writer and returns the result - how many bytes would be written with the given format string and args.
    template <typename... Args>
    s64 fmt_calculate_length(const string &fmtString, Args &&...arguments);

    // Formats to a string. The caller is responsible for freeing.
    template <typename... Args>
    [[nodiscard("Leak")]] string sprint(const string &fmtString, Args &&...arguments);

    // Formats to a string. Uses the temporary allocator.
    template <typename... Args>
    [[nodiscard("Leak")]] string tsprint(const string &fmtString, Args &&...arguments);

    // Calls fmt_to_writer on Context.Log - which is usually pointing to the console
    // but that can be changed to redirect the output!
    template <typename... Args>
    void print(const string &fmtString, Args &&...arguments);
}

fmt_type get_type(fmt_args ars, s64 index) {
    u64 shift = (u64) index * 4;
    return (fmt_type)((ars.Types & (0xfull << shift)) >> shift);
}

template <typename FC>
fmt_arg<FC> get_arg(fmt_args ars, s64 index) {
    if (index >= ars.Count) return {};

    if (!(ars.Types & fmt_internal::IS_UNPACKED_BIT)) {
        if (index > fmt_internal::MAX_PACKED_ARGS) return {};

        auto type = get_type(ars, index);
        if (type == fmt_type::None) return {};

        fmt_arg<FC> result;
        result.Type = type;
        result.Value = ((fmt_value<FC> *) ars.Data)[index];
        return result;
    }
    return ((fmt_arg<FC> *) ars.Data)[index];
}

struct width_checker {
    fmt_context *F;

    template <typename T>
    u32 operator()(T value) {
        if constexpr (types::is_integral<T>) {
            if (sign_bit(value)) {
                F->on_error("Negative width");
                return (u32) -1;
            } else if ((u64) value > numeric_info<s32>::max()) {
                F->on_error("Width value is too big");
                return (u32) -1;
            }
            return (u32) value;
        } else {
            F->on_error("Width was not an integer");
            return (u32) -1;
        }
    }
};

struct precision_checker {
    fmt_context *F;

    template <typename T>
    s32 operator()(T value) {
        if constexpr (types::is_integral<T>) {
            if (sign_bit(value)) {
                F->on_error("Negative precision");
                return -1;
            } else if ((u64) value > numeric_info<s32>::max()) {
                F->on_error("Precision value is too big");
                return -1;
            }
            return (s32) value;
        } else {
            F->on_error("Precision was not an integer");
            return -1;
        }
    }
};

export {
    // Returns an argument from index and reports an error if it is out of bounds.
    // Doesn't support negative indexing! (@Robustness ?)
    template <typename FC>
    fmt_arg<FC> fmt_get_arg_from_index(FC * f, s64 index) {
        if (index < f->Args.Count) {
            return get_arg<FC>(f->Args, index);
        }
        f->on_error("Argument index out of range");
        return {};
    }

    bool fmt_handle_dynamic_specs(fmt_context * f) {
        assert(f->Specs);

        if (f->Specs->WidthIndex != -1) {
            auto width = fmt_get_arg_from_index(f, f->Specs->WidthIndex);
            if (width.Type != fmt_type::None) {
                f->Specs->Width = fmt_visit_fmt_arg(width_checker{f}, width);
                if (f->Specs->Width == (u32) -1) return false;
            }
        }
        if (f->Specs->PrecisionIndex != -1) {
            auto precision = fmt_get_arg_from_index(f, f->Specs->PrecisionIndex);
            if (precision.Type != fmt_type::None) {
                f->Specs->Precision = fmt_visit_fmt_arg(precision_checker{f}, precision);
                if (f->Specs->Precision == numeric_info<s32>::min()) return false;
            }
        }

        return true;
    }

    void fmt_parse_and_format(fmt_context * f) {
        fmt_parse_context *p = &f->Parse;

        auto write_until = [&](const utf8 *end) {
            if (!p->It.Count) return;
            while (true) {
                auto searchString = string(p->It.Data, end - p->It.Data);

                s64 bracket = find_cp(searchString, '}');
                if (bracket == -1) {
                    write_no_specs(f, p->It.Data, end - p->It.Data);
                    return;
                }

                auto *pbracket = get_cp_at_index(searchString.Data, searchString.Length, bracket);
                if (*(pbracket + 1) != '}') {
                    f->on_error("Unmatched \"}\" in format string - if you want to print it use \"}}\" to escape", pbracket - f->Parse.FormatString.Data);
                    return;
                }

                write_no_specs(f, p->It.Data, pbracket - p->It.Data);
                write_no_specs(f, "}");

                s64 advance = pbracket + 2 - p->It.Data;
                p->It.Data += advance, p->It.Count -= advance;
            }
        };

        fmt_arg<fmt_context> currentArg;

        while (p->It.Count) {
            s64 bracket = find_cp(p->It, '{');
            if (bracket == -1) {
                write_until(p->It.Data + p->It.Count);
                return;
            }

            auto *pbracket = get_cp_at_index(p->It.Data, p->It.Length, bracket);
            write_until(pbracket);

            s64 advance = pbracket + 1 - p->It.Data;
            p->It.Data += advance, p->It.Count -= advance;

            if (!p->It.Count) {
                f->on_error("Invalid format string");
                return;
            }
            if (p->It[0] == '}') {
                // Implicit {} means "get the next argument"
                currentArg = fmt_get_arg_from_index(f, p->next_arg_id());
                if (currentArg.Type == fmt_type::None) return;  // The error was reported in _f->get_arg_from_ref_

                fmt_visit_fmt_arg(fmt_context_visitor(f), currentArg);
            } else if (p->It[0] == '{') {
                // {{ means we escaped a {.
                write_until(p->It.Data + 1);
            } else if (p->It[0] == '!') {
                ++p->It.Data, --p->It.Count;  // Skip the !

                text_style style = {};
                bool success = parse_text_style(p, &style);
                if (!success) return;
                if (!p->It.Count || p->It[0] != '}') {
                    f->on_error("\"}\" expected");
                    return;
                }

                if (!Context.FmtDisableAnsiCodes) {
                    utf8 ansiBuffer[7 + 3 * 4 + 1];
                    auto *ansiEnd = fmt_internal::color_to_ansi(ansiBuffer, style);
                    write_no_specs(f, ansiBuffer, ansiEnd - ansiBuffer);

                    u8 emphasis = (u8) style.Emphasis;
                    if (emphasis) {
                        assert(!style.Background);
                        ansiEnd = fmt_internal::emphasis_to_ansi(ansiBuffer, emphasis);
                        write_no_specs(f, ansiBuffer, ansiEnd - ansiBuffer);
                    }
                }
            } else {
                // Parse integer specified or a named argument
                s64 argId = parse_arg_id(p);
                if (argId == -1) return;

                currentArg = fmt_get_arg_from_index(f, argId);
                if (currentArg.Type == fmt_type::None) return;  // The error was reported in _f->get_arg_from_ref_

                utf8 c = p->It.Count ? p->It[0] : 0;
                if (c == '}') {
                    fmt_visit_fmt_arg(fmt_context_visitor(f), currentArg);
                } else if (c == ':') {
                    ++p->It.Data, --p->It.Count;  // Skip the :

                    dynamic_format_specs specs = {};
                    bool success = parse_fmt_specs(p, currentArg.Type, &specs);
                    if (!success) return;
                    if (!p->It.Count || p->It[0] != '}') {
                        f->on_error("\"}\" expected");
                        return;
                    }

                    f->Specs = &specs;
                    success = fmt_handle_dynamic_specs(f);
                    if (!success) return;

                    fmt_visit_fmt_arg(fmt_context_visitor(f), currentArg);

                    f->Specs = null;
                } else {
                    f->on_error("\"}\" expected");
                    return;
                }
            }
            ++p->It.Data, --p->It.Count;  // Go to the next byte
        }
    }

    template <typename... Args>
    void fmt_to_writer(writer * out, const string &fmtString, Args &&...arguments) {
        // @TODO: Can we remove this? (the fmt_context{})
        auto args = fmt_args_on_the_stack(fmt_context{}, ((types::remove_reference_t<Args> &&) arguments)...);  // This needs to outlive _parse_fmt_string_
        auto f = fmt_context(out, fmtString, args);

        fmt_parse_and_format(&f);
        f.flush();
    }

    // Formats to a counting writer and returns the result - how many bytes would be written with the given format string and args.
    template <typename... Args>
    s64 fmt_calculate_length(const string &fmtString, Args &&...arguments) {
        counting_writer writer;
        fmt_to_writer(&writer, fmtString, ((Args &&) arguments)...);
        return writer.Count;
    }

    // Formats to a string. The caller is responsible for freeing.
    template <typename... Args>
    [[nodiscard("Leak")]] string sprint(const string &fmtString, Args &&...arguments) {
        auto writer = string_writer();
        fmt_to_writer(&writer, fmtString, ((Args &&) arguments)...);

        string combined = combine(writer.Builder);
        free(writer);

        return combined;
    }

    // Formats to a string. Uses the temporary allocator.
    template <typename... Args>
    string tsprint(const string &fmtString, Args &&...arguments) {
        WITH_ALLOC(Context.Temp) {
            return sprint(fmtString, ((Args &&) arguments)...);
        }
    }

    // Calls fmt_to_writer on Context.Log - which is usually pointing to the console
    template <typename... Args>
    void print(const string &fmtString, Args &&...arguments) {
        fmt_to_writer(Context.Log, fmtString, ((Args &&) arguments)...);
    }
}

LSTD_END_NAMESPACE
