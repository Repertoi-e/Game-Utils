#pragma once

#include "array.h"

import lstd.allocator;

LSTD_BEGIN_NAMESPACE

// UTF-8 string
// This string doesn't guarantee a null termination at the end.
//
// Documentation for array is relevant here.
//
// Contains the pre-calculated amount of code points in the string.
//
// Functions on this object allow negative reversed indexing which begins at
// the end of the string, so -1 is the last code point -2 the one before that, etc. (Python-style)
struct string : public array_view<utf8> {
    struct code_point_ref {
        string *Parent;
        s64 Index;

        code_point_ref() {}
        code_point_ref(string *parent, s64 index) : Parent(parent), Index(index) {}

        code_point_ref &operator=(utf32 other);
        operator utf32() const;
    };

    s64 Length = 0;  // Length of the string in unicode code points
    s64 Allocated = 0;

    constexpr string() {}

    // Create a string from a null terminated c-string.
    // Note that this constructor doesn't validate if the passed in string is valid utf8.
    constexpr string(const utf8 *str) : array_view<utf8>((utf8 *) str, c_string_length(str)), Length(utf8_length(str, Count)) {}

    // This constructor allows constructing from the utf8 encoded u8"..." string literals.
    constexpr string(const char8_t *str) : array_view<utf8>((utf8 *) str, c_string_length(str)), Length(utf8_length((utf8 *) str, Count)) {}

    // Create a string from a buffer and a length.
    // Note that this constructor doesn't validate if the passed in string is valid utf8.
    constexpr string(const utf8 *str, s64 size) : array_view<utf8>((utf8 *) str, size), Length(utf8_length(str, size)) {}

    // Create a string from a buffer and a length.
    // Note that this constructor doesn't validate if the passed in string is valid utf8.
    constexpr string(const byte *str, s64 size) : array_view<utf8>((utf8 *) str, size), Length(utf8_length(Data, size)) {}

    constexpr string(const array_view<utf8> &arr) : array_view<utf8>(arr), Length(utf8_length(Data, Count)) {}
    constexpr string(const array_view<byte> &arr) : array_view<utf8>((utf8 *) arr.Data, arr.Count), Length(utf8_length(Data, Count)) {}

    // Allocates a buffer (using the Context's allocator by default)
    string(utf32 codePoint, s64 repeat);

    // Allocates a buffer (using the Context's allocator by default)
    string(utf16 codePoint, s64 repeat) : string((utf32) codePoint, repeat) {}

    // Converts a null-terminated utf16 string to utf8.
    // Allocates a buffer (using the Context's allocator by default)
    explicit string(const utf16 *str);

    // Converts a null-terminated utf32 string to utf8.
    // Allocates a buffer (using the Context's allocator by default)
    explicit string(const utf32 *str);

    // We no longer use destructors for deallocation. Call release() explicitly (take a look at the defer macro!).
    // ~string() { free(); }

    //
    // Iterator:
    //
   private:
    template <bool Const>
    struct string_iterator {
        using string_t = types::select_t<Const, const string, string>;

        string_t *Parent;
        s64 Index;

        string_iterator() {}
        string_iterator(string_t *parent, s64 index) : Parent(parent), Index(index) {}

        string_iterator &operator+=(s64 amount) { return Index += amount, *this; }
        string_iterator &operator-=(s64 amount) { return Index -= amount, *this; }
        string_iterator &operator++() { return *this += 1; }
        string_iterator &operator--() { return *this -= 1; }
        string_iterator operator++(s32) {
            string_iterator temp = *this;
            return ++(*this), temp;
        }

        string_iterator operator--(s32) {
            string_iterator temp = *this;
            return --(*this), temp;
        }

        s64 operator-(const string_iterator &other) const {
            s64 lesser = Index, greater = other.Index;
            if (lesser > greater) {
                lesser = other.Index;
                greater = Index;
            }
            s64 difference = greater - lesser;
            return Index <= other.Index ? difference : -difference;
        }

        string_iterator operator+(s64 amount) const { return string_iterator(Parent, Index + amount); }
        string_iterator operator-(s64 amount) const { return string_iterator(Parent, Index - amount); }

        friend string_iterator operator+(s64 amount, const string_iterator &it) { return it + amount; }
        friend string_iterator operator-(s64 amount, const string_iterator &it) { return it - amount; }

        bool operator==(const string_iterator &other) const { return Index == other.Index; }
        bool operator!=(const string_iterator &other) const { return Index != other.Index; }
        bool operator>(const string_iterator &other) const { return Index > other.Index; }
        bool operator<(const string_iterator &other) const { return Index < other.Index; }
        bool operator>=(const string_iterator &other) const { return Index >= other.Index; }
        bool operator<=(const string_iterator &other) const { return Index <= other.Index; }

        auto operator*() { return (*Parent)[Index]; }

        operator const utf8 *() const { return get_cp_at_index(Parent->Data, Parent->Length, Index, true); }
    };

   public:
    using iterator = string_iterator<false>;
    using const_iterator = string_iterator<true>;

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, Length); }

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, Length); }

    //
    // Operators:
    //

    // Returns true if the string contains any code points
    constexpr explicit operator bool() const { return Length; }

    constexpr operator array_view<utf8>() const { return array_view<utf8>(Data, Count); }
    constexpr operator array_view<byte>() const { return array_view<byte>((byte *) Data, Count); }

    // The non-const version allows to modify the character by simply =.
    code_point_ref operator[](s64 index) { return code_point_ref(this, translate_index(index, Length)); }
    constexpr utf32 operator[](s64 index) const { return decode_cp(get_cp_at_index(Data, Length, index)); }

    // Substring operator:
    constexpr string operator()(s64 begin, s64 end) const;
};

// Makes sure string has reserved enough space for at least n bytes.
// Note that it may reserve way more than required.
// Reserves space equal to the next power of two bigger than _size_, starting at 8.
//
// Allocates a buffer if the string doesn't already point to reserved memory.
// Allocations are done using the Context's allocator and aligned if there is a specified
// preferred alignment in the Context.
void reserve(string &s, s64 size);

// Resets Count and Length without freeing memory
void reset(string &s);

// Releases the memory allocated by this string and resets its members
void free(string &s);

// Allocates a buffer, copies the string's contents and also appends a zero terminator.
// The caller is responsible for freeing.
[[nodiscard("Leak")]] utf8 *to_c_string(const string &s, allocator alloc = {});

// Gets the _index_'th code point in the string.
constexpr utf32 get(const string &s, s64 index) { return decode_cp(get_cp_at_index(s.Data, s.Length, index)); }

//
// String modification:
//
// Sets the _index_'th code point in the string.
void set(string &s, s64 index, utf32 codePoint);

// Insert a code point at a specified index.
void insert(string &s, s64 index, utf32 codePoint);

// Insert a buffer of bytes at a specified index.
void insert_pointer_and_size(string &s, s64 index, const utf8 *str, s64 size);

// Insert a string at a specified index.
inline void insert_string(string &s, s64 index, const string &str) { return insert_pointer_and_size(s, index, str.Data, str.Count); }

// Remove the first occurence of a code point.
void remove(string &s, utf32 cp);

// Remove code point at specified index.
void remove_at_index(string &s, s64 index);

// Remove a range of code points. [begin, end)
void remove_range(string &s, s64 begin, s64 end);

// Append a non encoded character to a string.
inline void append_cp(string &s, utf32 codePoint) { return insert(s, s.Length, codePoint); }

// Append _size_ bytes of string contained in _data_.
inline void append_pointer_and_size(string &s, const utf8 *str, s64 size) { return insert_pointer_and_size(s, s.Length, str, size); }

// Append one string to another.
inline void append_string(string &s, const string &str) { return append_pointer_and_size(s, str.Data, str.Count); }

// Copy this string's contents and append them _n_ times.
void repeat(string &s, s64 n);

// @Cleanup: Combine remove_all and replace_all into one implementation with concept magic?

// Removes all occurences of _cp_.
void remove_all(string &s, utf32 cp);

// Remove all occurences of _str_.
void remove_all(string &s, const string &str);

// Replace all occurences of _oldCp_ with _newCp_.
void replace_all(string &s, utf32 oldCp, utf32 newCp);

// Replace all occurences of _oldStr_ with _newStr_.
void replace_all(string &s, const string &oldStr, const string &newStr);

// Replace all occurences of _oldCp_ with _newStr_.
void replace_all(string &s, utf32 oldCp, const string &newStr);

// Replace all occurences of _oldStr_ with _newCp_.
void replace_all(string &s, const string &oldStr, utf32 newCp);

//
// Comparison and searching:
//

// Compares two utf8 encoded strings and returns the index
// of the code point at which they are different or _-1_ if they are the same.
constexpr s64 compare(const string &s, const string &other) {
    if (s.Length == 0 && other.Length == 0) return -1;
    if (s.Length == 0 || other.Length == 0) return 0;

    auto *p1 = s.Data, *p2 = other.Data;
    auto *e1 = p1 + s.Count, *e2 = p2 + other.Count;

    s64 index = 0;
    while (decode_cp(p1) == decode_cp(p2)) {
        p1 += get_size_of_cp(p1);
        p2 += get_size_of_cp(p2);
        if (p1 == e1 && p2 == e2) return -1;
        if (p1 == e1 || p2 == e2) return index;
        ++index;
    }
    return index;
}

// Compares two utf8 encoded strings while ignoring case and returns the index
// of the code point at which they are different or _-1_ if they are the same.
constexpr s64 compare_ignore_case(const string &s, const string &other) {
    if (s.Length == 0 && other.Length == 0) return -1;
    if (s.Length == 0 || other.Length == 0) return 0;

    auto *p1 = s.Data, *p2 = other.Data;
    auto *e1 = p1 + s.Count, *e2 = p2 + other.Count;

    s64 index = 0;
    while (to_lower(decode_cp(p1)) == to_lower(decode_cp(p2))) {
        p1 += get_size_of_cp(p1);
        p2 += get_size_of_cp(p2);
        if (p1 == e1 && p2 == e2) return -1;
        if (p1 == e1 || p2 == e2) return index;
        ++index;
    }
    return index;
}

// Compares two utf8 encoded strings lexicographically and returns:
//  -1 if _a_ is before _b_
//   0 if a == b
//   1 if _b_ is before _a_
constexpr s32 compare_lexicographically(const string &a, const string &b) {
    if (a.Length == 0 && b.Length == 0) return 0;
    if (a.Length == 0) return -1;
    if (b.Length == 0) return 1;

    auto *p1 = a.Data, *p2 = b.Data;
    auto *e1 = p1 + a.Count, *e2 = p2 + b.Count;

    s64 index = 0;
    while (decode_cp(p1) == decode_cp(p2)) {
        p1 += get_size_of_cp(p1);
        p2 += get_size_of_cp(p2);
        if (p1 == e1 && p2 == e2) return 0;
        if (p1 == e1) return -1;
        if (p2 == e2) return 1;
        ++index;
    }
    return ((s64) decode_cp(p1) - (s64) decode_cp(p2)) < 0 ? -1 : 1;
}

// Compares two utf8 encoded strings lexicographically while ignoring case and returns:
//  -1 if _a_ is before _b_
//   0 if a == b
//   1 if _b_ is before _a_
constexpr s32 compare_lexicographically_ignore_case(const string &a, const string &b) {
    if (a.Length == 0 && b.Length == 0) return 0;
    if (a.Length == 0) return -1;
    if (b.Length == 0) return 1;

    auto *p1 = a.Data, *p2 = b.Data;
    auto *e1 = p1 + a.Count, *e2 = p2 + b.Count;

    s64 index = 0;
    while (to_lower(decode_cp(p1)) == to_lower(decode_cp(p2))) {
        p1 += get_size_of_cp(p1);
        p2 += get_size_of_cp(p2);
        if (p1 == e1 && p2 == e2) return 0;
        if (p1 == e1) return -1;
        if (p2 == e2) return 1;
        ++index;
    }
    return ((s64) to_lower(decode_cp(p1)) - (s64) to_lower(decode_cp(p2))) < 0 ? -1 : 1;
}

// Searches for the first occurence of a substring which is after a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_substring(const string &haystack, const string &needle, s64 start = 0) {
    assert(needle.Data && needle.Length);

    if (haystack.Length == 0) return -1;

    if (start >= haystack.Length || start <= -haystack.Length) return -1;

    auto *p = get_cp_at_index(haystack.Data, haystack.Length, translate_index(start, haystack.Length));
    auto *end = haystack.Data + haystack.Count;

    auto *needleEnd = needle.Data + needle.Count;

    while (p != end) {
        while (end - p > 4) {
            if (U32_HAS_BYTE(*(u32 *) p, *needle.Data)) break;
            p += 4;
        }

        while (p != end) {
            if (*p == *needle.Data) break;
            ++p;
        }

        if (p == end) return -1;

        auto *search = p + 1;
        auto *progress = needle.Data + 1;
        while (search != end && progress != needleEnd && *search == *progress) ++search, ++progress;
        if (progress == needleEnd) return utf8_length(haystack.Data, p - haystack.Data);
        ++p;
    }
    return -1;
}

// Searches for the first occurence of a code point which is after a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_cp(const string &haystack, utf32 cp, s64 start = 0) {
    utf8 encoded[4]{};
    encode_cp(encoded, cp);
    return find_substring(haystack, string(encoded, get_size_of_cp(encoded)), start);
}

// Searches for the last occurence of a substring which is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_substring_reverse(const string &haystack, const string &needle, s64 start = 0) {
    assert(needle.Data && needle.Length);

    if (haystack.Length == 0) return -1;

    if (start >= haystack.Length || start <= -haystack.Length) return -1;
    if (start == 0) start = haystack.Length;

    auto *p = get_cp_at_index(haystack.Data, haystack.Length, translate_index(start, haystack.Length, true) - 1);
    auto *end = haystack.Data + haystack.Count;

    auto *needleEnd = needle.Data + needle.Count;

    while (p > haystack.Data) {
        while (p - haystack.Data > 4) {
            if (U32_HAS_BYTE(*((u32 *) (p - 3)), *needle.Data)) break;
            p -= 4;
        }

        while (p != haystack.Data) {
            if (*p == *needle.Data) break;
            --p;
        }

        if (*p != *needle.Data && p == haystack.Data) return -1;

        auto *search = p + 1;
        auto *progress = needle.Data + 1;
        while (search != end && progress != needleEnd && *search == *progress) ++search, ++progress;
        if (progress == needleEnd) return utf8_length(haystack.Data, p - haystack.Data);
        --p;
    }
    return -1;
}

// Searches for the last occurence of a code point which is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_cp_reverse(const string &haystack, utf32 cp, s64 start = 0) {
    utf8 encoded[4]{};
    encode_cp(encoded, cp);
    return find_substring_reverse(haystack, string(encoded, get_size_of_cp(encoded)), start);
}

// Searches for the first occurence of a substring which is different from _eat_ and is after a specified _start_ index.
// Returns -1 if no index was found.
//
//  e.g.
//   find_substring_not("../../../../user/stuff", "../")
//   returns:                        ^
//
constexpr s64 find_substring_not(const string &s, const string &eat, s64 start = 0) {
    assert(eat.Data && eat.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;

    auto *p = get_cp_at_index(s.Data, s.Length, translate_index(start, s.Length));
    auto *end = s.Data + s.Count;

    auto *eatEnd = eat.Data + eat.Count;
    while (p != end) {
        while (p != end) {
            if (*p != *eat.Data) break;
            ++p;
        }

        if (p == end) return -1;

        auto *search = p + 1;
        auto *progress = eat.Data + 1;
        while (search != end && progress != eatEnd && *search != *progress) ++search, ++progress;
        if (progress == eatEnd) return utf8_length(s.Data, p - s.Data);
        ++p;
    }
    return -1;
}

// Searches for the first occurence of a code point which is not _cp_ and is after a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_cp_not(const string &s, utf32 cp, s64 start = 0) {
    utf8 encoded[4]{};
    encode_cp(encoded, cp);
    return find_substring_not(s, string(encoded, get_size_of_cp(encoded)), start);
}

// Searches for the last occurence of a substring which is different from _eat_ and is before a specified _start_ index.
// Returns -1 if no index was found.
//
//  e.g.
//   find_substring_reverse_not("user/stuff/file.txtGARBAGEGARBAGEGARBAGE", "GARBAGE")
//   returns:                                      ^
//
constexpr s64 find_substring_reverse_not(const string &s, const string &eat, s64 start = 0) {
    assert(eat.Data && eat.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;
    if (start == 0) start = s.Length;

    auto *p = get_cp_at_index(s.Data, s.Length, translate_index(start, s.Length, true) - 1);
    auto *end = s.Data + s.Count;

    auto *eatEnd = eat.Data + eat.Count;

    while (p > s.Data) {
        while (p != s.Data) {
            if (*p != *eat.Data) break;
            --p;
        }

        if (*p == *eat.Data && p == s.Data) return -1;

        auto *search = p + 1;
        auto *progress = eat.Data + 1;
        while (search != end && progress != eatEnd && *search != *progress) ++search, ++progress;
        if (progress == eatEnd) return utf8_length(s.Data, p - s.Data);
        --p;
    }
    return -1;
}

// Searches for the last occurence of a code point which is different from _cp_ and is before a specified _start_ index.
// Returns -1 if no index was found.
//
//  e.g.
//   find_cp_reverse_not("user/stuff/file.txtCCCCCC", 'C')
//   returns:                               ^
//
constexpr s64 find_cp_reverse_not(const string &s, utf32 cp, s64 start = 0) {
    utf8 encoded[4]{};
    encode_cp(encoded, cp);
    return find_substring_reverse_not(s, string(encoded, get_size_of_cp(encoded)), start);
}

// Searches for the first occurence of a code point which is also present in _anyOfThese_ and is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_any_of(const string &s, const string &anyOfThese, s64 start = 0) {
    assert(anyOfThese.Data && anyOfThese.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;

    start = translate_index(start, s.Length);
    auto *p = get_cp_at_index(s.Data, s.Length, start);

    For(range(start, s.Length)) {
        if (find_cp(anyOfThese, decode_cp(p)) != -1) return utf8_length(s.Data, p - s.Data);
        p += get_size_of_cp(p);
    }
    return -1;
}

// Searches for the last occurence of a code point which is also present in _anyOfThese_ and is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_reverse_any_of(const string &s, const string &anyOfThese, s64 start = 0) {
    assert(anyOfThese.Data && anyOfThese.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;
    if (start == 0) start = s.Length;

    start = translate_index(start, s.Length, true) - 1;
    auto *p = get_cp_at_index(s.Data, s.Length, start);

    For(range(start, -1, -1)) {
        if (find_cp(anyOfThese, decode_cp(p)) != -1) return utf8_length(s.Data, p - s.Data);
        p -= get_size_of_cp(p);
    }
    return -1;
}

// Searches for the first occurence of a code point which is NOT present in _anyOfThese_ and is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_not_any_of(const string &s, const string &anyOfThese, s64 start = 0) {
    assert(anyOfThese.Data && anyOfThese.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;

    start = translate_index(start, s.Length);
    auto *p = get_cp_at_index(s.Data, s.Length, start);

    For(range(start, s.Length)) {
        if (find_cp(anyOfThese, decode_cp(p)) == -1) return utf8_length(s.Data, p - s.Data);
        p += get_size_of_cp(p);
    }
    return -1;
}

// Searches for the last occurence of a code point which is NOT present in _anyOfThese_ and is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_reverse_not_any_of(const string &s, const string &anyOfThese, s64 start = 0) {
    assert(anyOfThese.Data && anyOfThese.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;
    if (start == 0) start = s.Length;

    start = translate_index(start, s.Length, true) - 1;
    auto *p = get_cp_at_index(s.Data, s.Length, start);

    For(range(start, -1, -1)) {
        if (find_cp(anyOfThese, decode_cp(p)) == -1) return utf8_length(s.Data, p - s.Data);
        p -= get_size_of_cp(p);
    }
    return -1;
}

// Counts the number of occurences of _cp_
constexpr s64 count(const string &s, utf32 cp) {
    s64 result = 0, index = 0;
    while ((index = find_cp(s, cp, index)) != -1) {
        ++result, ++index;
        if (index >= s.Length) break;
    }
    return result;
}

// Counts the number of occurences of _str_
constexpr s64 count(const string &s, const string &str) {
    s64 result = 0, index = 0;
    while ((index = find_substring(s, str, index)) != -1) {
        ++result, ++index;
        if (index >= s.Length) break;
    }
    return result;
}

// Returns true if the string contains _cp_ anywhere
constexpr bool has(const string &s, utf32 cp) { return find_cp(s, cp) != -1; }

// Returns true if the string contains _str_ anywhere
constexpr bool has(const string &s, const string &str) { return find_substring(s, str) != -1; }

//
// Substring and trimming:
//

// Gets [begin, end) range of characters into a new string object.
// This function doesn't allocate, but just returns a "view".
// We can do this because we don't store strings with a zero terminator.
constexpr string substring(const string &s, s64 begin, s64 end) {
    s64 beginIndex = translate_index(begin, s.Length);
    s64 endIndex = translate_index(end, s.Length, true);

    const utf8 *beginPtr = get_cp_at_index(s.Data, s.Length, beginIndex);
    const utf8 *endPtr = beginPtr;
    For(range(beginIndex, endIndex)) endPtr += get_size_of_cp(endPtr);

    return string(beginPtr, endPtr - beginPtr);
}

// Returns true if _s_ begins with _str_
constexpr bool match_beginning(const string &s, const string &str) {
    if (str.Count > s.Count) return false;
    return compare_memory(s.Data, str.Data, str.Count) == -1;
}

// Returns true if _s_ ends with _str_
constexpr bool match_end(const string &s, const string &str) {
    if (str.Count > s.Count) return false;
    return compare_memory(s.Data + s.Count - str.Count, str.Data, str.Count) == -1;
}

// Returns a substring with white space removed at the start.
// This function doesn't allocate, but just returns a "view".
// We can do this because we don't store strings with a zero terminator.
constexpr string trim_start(const string &s) { return substring(s, find_not_any_of(s, " \n\r\t\v\f"), s.Length); }

// Returns a substring with white space removed at the end.
// This function doesn't allocate, but just returns a "view".
// We can do this because we don't store strings with a zero terminator.
constexpr string trim_end(const string &s) { return substring(s, 0, find_reverse_not_any_of(s, " \n\r\t\v\f") + 1); }

// Returns a substring with white space removed from both sides.
// This function doesn't allocate, but just returns a "view".
// We can do this because we don't store strings with a zero terminator.
constexpr string trim(const string &s) { return trim_end(trim_start(s)); }

//
// Operators:
//

constexpr bool operator==(const string &one, const string &other) { return compare_lexicographically(one, other) == 0; }
constexpr bool operator!=(const string &one, const string &other) { return !(one == other); }
constexpr bool operator<(const string &one, const string &other) { return compare_lexicographically(one, other) < 0; }
constexpr bool operator>(const string &one, const string &other) { return compare_lexicographically(one, other) > 0; }
constexpr bool operator<=(const string &one, const string &other) { return !(one > other); }
constexpr bool operator>=(const string &one, const string &other) { return !(one < other); }

// @TODO:
// constexpr auto operator<=>(const string &one, const string &other) { return compare_lexicographically(one, other); }

// operator<=> doesn't work on these for some reason...
constexpr bool operator==(const utf8 *one, const string &other) { return compare_lexicographically(one, other) == 0; }
constexpr bool operator!=(const utf8 *one, const string &other) { return !(one == other); }
constexpr bool operator<(const utf8 *one, const string &other) { return compare_lexicographically(one, other) < 0; }
constexpr bool operator>(const utf8 *one, const string &other) { return compare_lexicographically(one, other) > 0; }
constexpr bool operator<=(const utf8 *one, const string &other) { return !(one > other); }
constexpr bool operator>=(const utf8 *one, const string &other) { return !(one < other); }

// Substring operator:
constexpr string string::operator()(s64 begin, s64 end) const { return substring(*this, begin, end); }

// Be careful not to call this with _dest_ pointing to _src_!
// Returns just _dest_.
string *clone(string *dest, const string &src);

// Hash for strings
inline u64 get_hash(const string &value) {
    u64 hash = 5381;
    For(value) hash = ((hash << 5) + hash) + it;
    return hash;
}

LSTD_END_NAMESPACE
