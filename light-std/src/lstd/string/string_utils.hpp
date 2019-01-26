#pragma once

#include "../memory/range.hpp"

#include <iterator>

LSTD_BEGIN_NAMESPACE

// This is a constexpr function for working with cstyle strings at compile time
// Retrieve the length of a standard cstyle string (==strlen)
// Doesn't care about encoding.
// Note that this calculation does not include the null byte.
// This function can also be used to determine the size in
// bytes of a null terminated utf-8 string.
constexpr size_t cstring_strlen(const byte *str) {
    if (str == null) return 0;

    size_t length = 0;
    while (*str++) length++;
    return length;
}

// This is a constexpr function for working with utf8 strings at compile time
// Retrieve the length of a valid utf8 string
constexpr size_t utf8_strlen(const byte *str, size_t size) {
    size_t length = 0;
    For(range(size)) {
        if (!((*str++ & 0xc0) == 0x80)) ++length;
    }
    return length;
}

// These functions only work for ascii
constexpr bool is_digit(char32_t x) { return x >= '0' && x <= '9'; }
// These functions only work for ascii
constexpr bool is_hexadecimal_digit(char32_t x) { return (x >= '0' && x <= '9') || (x >= 'a' && x <= 'f'); }

// These functions only work for ascii
constexpr bool is_space(char32_t x) { return (x >= 9 && x <= 13) || x == 32; }
// These functions only work for ascii
constexpr bool is_blank(char32_t x) { return x == 9 || x == 32; }

// These functions only work for ascii
constexpr bool is_alpha(char32_t x) { return (x >= 65 && x <= 90) || (x >= 97 && x <= 122); }
// These functions only work for ascii
constexpr bool is_alphanumeric(char32_t x) { return is_alpha(x) || is_digit(x); }

constexpr bool is_identifier_start(char32_t x) { return is_alpha(x) || x == '_'; }

// These functions only work for ascii
constexpr bool is_print(char32_t x) { return x > 31 && x != 127; }

//
// Utility utf-8 functions:
//

// Convert code point to uppercase
constexpr char32_t to_upper(char32_t cp) {
    if (((0x0061 <= cp) && (0x007a >= cp)) || ((0x00e0 <= cp) && (0x00f6 >= cp)) ||
        ((0x00f8 <= cp) && (0x00fe >= cp)) || ((0x03b1 <= cp) && (0x03c1 >= cp)) ||
        ((0x03c3 <= cp) && (0x03cb >= cp))) {
        return cp - 32;
    }
    if (((0x0100 <= cp) && (0x012f >= cp)) || ((0x0132 <= cp) && (0x0137 >= cp)) ||
        ((0x014a <= cp) && (0x0177 >= cp)) || ((0x0182 <= cp) && (0x0185 >= cp)) ||
        ((0x01a0 <= cp) && (0x01a5 >= cp)) || ((0x01de <= cp) && (0x01ef >= cp)) ||
        ((0x01f8 <= cp) && (0x021f >= cp)) || ((0x0222 <= cp) && (0x0233 >= cp)) ||
        ((0x0246 <= cp) && (0x024f >= cp)) || ((0x03d8 <= cp) && (0x03ef >= cp))) {
        return cp & ~0x1;
    }
    if (((0x0139 <= cp) && (0x0148 >= cp)) || ((0x0179 <= cp) && (0x017e >= cp)) ||
        ((0x01af <= cp) && (0x01b0 >= cp)) || ((0x01b3 <= cp) && (0x01b6 >= cp)) ||
        ((0x01cd <= cp) && (0x01dc >= cp))) {
        return (cp - 1) | 0x1;
    }
    if (cp == 0x00ff) return 0x0178;
    if (cp == 0x0180) return 0x0243;
    if (cp == 0x01dd) return 0x018e;
    if (cp == 0x019a) return 0x023d;
    if (cp == 0x019e) return 0x0220;
    if (cp == 0x0292) return 0x01b7;
    if (cp == 0x01c6) return 0x01c4;
    if (cp == 0x01c9) return 0x01c7;
    if (cp == 0x01cc) return 0x01ca;
    if (cp == 0x01f3) return 0x01f1;
    if (cp == 0x01bf) return 0x01f7;
    if (cp == 0x0188) return 0x0187;
    if (cp == 0x018c) return 0x018b;
    if (cp == 0x0192) return 0x0191;
    if (cp == 0x0199) return 0x0198;
    if (cp == 0x01a8) return 0x01a7;
    if (cp == 0x01ad) return 0x01ac;
    if (cp == 0x01b0) return 0x01af;
    if (cp == 0x01b9) return 0x01b8;
    if (cp == 0x01bd) return 0x01bc;
    if (cp == 0x01f5) return 0x01f4;
    if (cp == 0x023c) return 0x023b;
    if (cp == 0x0242) return 0x0241;
    if (cp == 0x037b) return 0x03fd;
    if (cp == 0x037c) return 0x03fe;
    if (cp == 0x037d) return 0x03ff;
    if (cp == 0x03f3) return 0x037f;
    if (cp == 0x03ac) return 0x0386;
    if (cp == 0x03ad) return 0x0388;
    if (cp == 0x03ae) return 0x0389;
    if (cp == 0x03af) return 0x038a;
    if (cp == 0x03cc) return 0x038c;
    if (cp == 0x03cd) return 0x038e;
    if (cp == 0x03ce) return 0x038f;
    if (cp == 0x0371) return 0x0370;
    if (cp == 0x0373) return 0x0372;
    if (cp == 0x0377) return 0x0376;
    if (cp == 0x03d1) return 0x03f4;
    if (cp == 0x03d7) return 0x03cf;
    if (cp == 0x03f2) return 0x03f9;
    if (cp == 0x03f8) return 0x03f7;
    if (cp == 0x03fb) return 0x03fa;
    // No upper case!
    return cp;
}

// Convert code point to lowercase
constexpr char32_t to_lower(char32_t cp) {
    if (((0x0041 <= cp) && (0x005a >= cp)) || ((0x00c0 <= cp) && (0x00d6 >= cp)) ||
        ((0x00d8 <= cp) && (0x00de >= cp)) || ((0x0391 <= cp) && (0x03a1 >= cp)) ||
        ((0x03a3 <= cp) && (0x03ab >= cp))) {
        return cp + 32;
    }
    if (((0x0100 <= cp) && (0x012f >= cp)) || ((0x0132 <= cp) && (0x0137 >= cp)) ||
        ((0x014a <= cp) && (0x0177 >= cp)) || ((0x0182 <= cp) && (0x0185 >= cp)) ||
        ((0x01a0 <= cp) && (0x01a5 >= cp)) || ((0x01de <= cp) && (0x01ef >= cp)) ||
        ((0x01f8 <= cp) && (0x021f >= cp)) || ((0x0222 <= cp) && (0x0233 >= cp)) ||
        ((0x0246 <= cp) && (0x024f >= cp)) || ((0x03d8 <= cp) && (0x03ef >= cp))) {
        return cp | 0x1;
    }
    if (((0x0139 <= cp) && (0x0148 >= cp)) || ((0x0179 <= cp) && (0x017e >= cp)) ||
        ((0x01af <= cp) && (0x01b0 >= cp)) || ((0x01b3 <= cp) && (0x01b6 >= cp)) ||
        ((0x01cd <= cp) && (0x01dc >= cp))) {
        return (cp + 1) & ~0x1;
    }
    if (cp == 0x0178) return 0x00ff;
    if (cp == 0x0178) return 0x00ff;
    if (cp == 0x0243) return 0x0180;
    if (cp == 0x018e) return 0x01dd;
    if (cp == 0x023d) return 0x019a;
    if (cp == 0x0220) return 0x019e;
    if (cp == 0x01b7) return 0x0292;
    if (cp == 0x01c4) return 0x01c6;
    if (cp == 0x01c7) return 0x01c9;
    if (cp == 0x01ca) return 0x01cc;
    if (cp == 0x01f1) return 0x01f3;
    if (cp == 0x01f7) return 0x01bf;
    if (cp == 0x0187) return 0x0188;
    if (cp == 0x018b) return 0x018c;
    if (cp == 0x0191) return 0x0192;
    if (cp == 0x0198) return 0x0199;
    if (cp == 0x01a7) return 0x01a8;
    if (cp == 0x01ac) return 0x01ad;
    if (cp == 0x01af) return 0x01b0;
    if (cp == 0x01b8) return 0x01b9;
    if (cp == 0x01bc) return 0x01bd;
    if (cp == 0x01f4) return 0x01f5;
    if (cp == 0x023b) return 0x023c;
    if (cp == 0x0241) return 0x0242;
    if (cp == 0x03fd) return 0x037b;
    if (cp == 0x03fe) return 0x037c;
    if (cp == 0x03ff) return 0x037d;
    if (cp == 0x037f) return 0x03f3;
    if (cp == 0x0386) return 0x03ac;
    if (cp == 0x0388) return 0x03ad;
    if (cp == 0x0389) return 0x03ae;
    if (cp == 0x038a) return 0x03af;
    if (cp == 0x038c) return 0x03cc;
    if (cp == 0x038e) return 0x03cd;
    if (cp == 0x038f) return 0x03ce;
    if (cp == 0x0370) return 0x0371;
    if (cp == 0x0372) return 0x0373;
    if (cp == 0x0376) return 0x0377;
    if (cp == 0x03f4) return 0x03d1;
    if (cp == 0x03cf) return 0x03d7;
    if (cp == 0x03f9) return 0x03f2;
    if (cp == 0x03f7) return 0x03f8;
    if (cp == 0x03fa) return 0x03fb;
    // No lower case!
    return cp;
}

constexpr bool is_upper(char32_t ch) { return ch != to_lower(ch); }
constexpr bool is_lower(char32_t ch) { return ch != to_upper(ch); }

// Returns the size in bytes of the code point that _str_ points to.
// This function reads the first byte and returns that result.
// If the byte pointed by _str_ is a countinuation utf-8 byte, this
// function returns 0
constexpr size_t get_size_of_code_point(const byte *str) {
    if (!str) return 0;
    if ((*str & 0xc0) == 0x80) return 0;

    if (0xf0 == (0xf8 & str[0])) {
        return 4;
    } else if (0xe0 == (0xf0 & str[0])) {
        return 3;
    } else if (0xc0 == (0xe0 & str[0])) {
        return 2;
    } else {
        return 1;
    }
}

// Returns the size that the code point would be if it were encoded.
constexpr size_t get_size_of_code_point(char32_t codePoint) {
    if (((s32) 0xffffff80 & codePoint) == 0) {
        return 1;
    } else if (((s32) 0xfffff800 & codePoint) == 0) {
        return 2;
    } else if (((s32) 0xffff0000 & codePoint) == 0) {
        return 3;
    } else {
        return 4;
    }
}

// Encodes code point at _str_, assumes there is enough space.
constexpr void encode_code_point(byte *str, char32_t codePoint) {
    size_t size = get_size_of_code_point(codePoint);
    if (size == 1) {
        // 1-byte/7-bit ascii
        // (0b0xxxxxxx)
        str[0] = (byte) codePoint;
    } else if (size == 2) {
        // 2-byte/11-bit utf-8 code point
        // (0b110xxxxx 0b10xxxxxx)
        str[0] = 0xc0 | (byte)(codePoint >> 6);
        str[1] = 0x80 | (byte)(codePoint & 0x3f);
    } else if (size == 3) {
        // 3-byte/16-bit utf-8 code point
        // (0b1110xxxx 0b10xxxxxx 0b10xxxxxx)
        str[0] = 0xe0 | (byte)(codePoint >> 12);
        str[1] = 0x80 | (byte)((codePoint >> 6) & 0x3f);
        str[2] = 0x80 | (byte)(codePoint & 0x3f);
    } else {
        // 4-byte/21-bit utf-8 code point
        // (0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx)
        str[0] = 0xf0 | (byte)(codePoint >> 18);
        str[1] = 0x80 | (byte)((codePoint >> 12) & 0x3f);
        str[2] = 0x80 | (byte)((codePoint >> 6) & 0x3f);
        str[3] = 0x80 | (byte)(codePoint & 0x3f);
    }
}

// Decodes a code point from a data pointer
constexpr char32_t decode_code_point(const byte *str) {
    if (0xf0 == (0xf8 & str[0])) {
        // 4 byte utf-8 code point
        return ((0x07 & str[0]) << 18) | ((0x3f & str[1]) << 12) | ((0x3f & str[2]) << 6) | (0x3f & str[3]);
    } else if (0xe0 == (0xf0 & str[0])) {
        // 3 byte utf-8 code point
        return ((0x0f & str[0]) << 12) | ((0x3f & str[1]) << 6) | (0x3f & str[2]);
    } else if (0xc0 == (0xe0 & str[0])) {
        // 2 byte utf-8 code point
        return ((0x1f & str[0]) << 6) | (0x3f & str[1]);
    } else {
        // 1 byte utf-8 code point
        return str[0];
    }
}

// This function translates an index that may be negative to an actual index.
// For example 5 maps to 5
// but -5 maps to length - 5
// This function is used to support python-like negative indexing.
constexpr size_t translate_index_unchecked(s64 index, size_t length) {
    if (index < 0) return (size_t)(length + index);
    return (size_t) index;
}

// This function checks if the index is in range
constexpr size_t translate_index(s64 index, size_t length) {
    if (index < 0) {
        s64 actual = length + index;
        assert(actual >= 0);
        assert((size_t) actual < length);
        return (size_t) actual;
    }
    assert((size_t) index < length);
    return (size_t) index;
}

// This returns str advanced to point to the code point at a specified index in a string with a given length.
constexpr const byte *get_pointer_to_code_point_at(const byte *str, size_t length, s64 index) {
    For(range(translate_index(index, length))) str += get_size_of_code_point(str);
    return str;
}

LSTD_END_NAMESPACE