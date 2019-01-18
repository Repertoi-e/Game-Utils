#pragma once

#include "../memory/memory.hpp"
#include "../memory/memory_view.hpp"

#include "string_utils.hpp"

CPPU_BEGIN_NAMESPACE

struct string;

// This object represents a non-owning pointer to
// to a utf-8 string. It also contains the amount of bytes stored there,
// as well as the amount of codepoints. (utf8 length != byte length)
// string_view is useful when working with literal strings or when
// you don't want to allocate memory for a new string (eg. a substring)
struct string_view {
   private:
    struct Iterator {
       private:
        const byte *Current;

        // Returns a pointer to Current + _n_ utf-8 characters
        constexpr const byte *get_current_after(s64 n) const {
            const byte *result = Current;
            if (n > 0) {
                For(range(n)) result += get_size_of_code_point(result);
            }
            if (n < 0) {
                For(range(n, 0)) {
                    do {
                        result--;
                    } while ((*result & 0xC0) == 0x80);
                }
            }
            return result;
        }

       public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = char32_t;
        using difference_type = ptr_t;
        using pointer = char32_t *;
        using reference = char32_t &;

        constexpr Iterator(const byte *data) : Current(data) {}

        constexpr Iterator &operator+=(s64 amount) {
            Current = get_current_after(amount);
            return *this;
        }
        constexpr Iterator &operator-=(s64 amount) {
            Current = get_current_after(-amount);
            return *this;
        }
        constexpr Iterator &operator++() { return *this += 1; }
        constexpr Iterator &operator--() { return *this -= 1; }
        constexpr Iterator operator++(s32) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }
        constexpr Iterator operator--(s32) {
            Iterator temp = *this;
            --(*this);
            return temp;
        }

        constexpr s64 operator-(const Iterator &other) const {
            s64 difference = 0;
            const byte *lesser = Current, *greater = other.Current;
            if (lesser > greater) {
                lesser = other.Current;
                greater = Current;
            }
            while (lesser != greater) {
                lesser += get_size_of_code_point(lesser);
                difference++;
            }
            return Current <= other.Current ? -difference : difference;
        }

        constexpr Iterator operator+(s64 amount) const { return Iterator(get_current_after(amount)); }
        constexpr Iterator operator-(s64 amount) const { return Iterator(get_current_after(-amount)); }

        constexpr friend inline Iterator operator+(s64 amount, const Iterator &it) { return it + amount; }
        constexpr friend inline Iterator operator-(s64 amount, const Iterator &it) { return it - amount; }

        constexpr bool operator==(const Iterator &other) const { return Current == other.Current; }
        constexpr bool operator!=(const Iterator &other) const { return Current != other.Current; }
        constexpr bool operator>(const Iterator &other) const { return Current > other.Current; }
        constexpr bool operator<(const Iterator &other) const { return Current < other.Current; }
        constexpr bool operator>=(const Iterator &other) const { return Current >= other.Current; }
        constexpr bool operator<=(const Iterator &other) const { return Current <= other.Current; }

        constexpr char32_t operator*() const { return decode_code_point(Current); }

        constexpr const byte *to_pointer() const { return Current; }
    };

   public:
    using iterator = Iterator;

    const byte *Data = null;
    size_t ByteLength = 0;

    // Length of the string in code points
    size_t Length = 0;

    constexpr string_view() {}
    // Construct from a null-terminated c-style string.
    constexpr string_view(const byte *str) : string_view(str, str ? cstring_strlen(str) : 0) {}

    // Construct from a c-style string and a size (in code units, not code points)
    constexpr string_view(const byte *str, size_t size) {
        Data = str;
        ByteLength = size;
        if (Data) {
            Length = utf8_strlen(Data, ByteLength);
        }
    }

    constexpr string_view(const char *str) : string_view((const byte *) str) {}
    constexpr string_view(const char *str, size_t size) : string_view((const byte *) str, size) {}

    constexpr string_view(const Memory_View &memView) : string_view(memView.Data, memView.ByteLength) {}

    constexpr string_view(const string_view &other) = default;
    constexpr string_view(string_view &&other) = default;

    // Gets the _index_'th code point in the string
    // Allows negative reversed indexing which begins at
    // the end of the string, so -1 is the last character
    // -2 the one before that, etc. (Python-style)
    constexpr char32_t get(s64 index) const {
        return decode_code_point(get_pointer_to_code_point_at(Data, Length, index));
    }

    // Gets [begin, end) range of characters
    // Allows negative reversed indexing which begins at
    // the end of the string, so -1 is the last character
    // -2 the one before that, etc. (Python-style)
    // Note that the string returned is a view into _str_.
    //    It's not actually an allocated string, so it _str_ gets
    //    destroyed, then the returned string will be pointing to
    //      invalid memory. Copy the returned string explicitly if
    //      you intend to use it longer than this string.
    constexpr string_view substring(s64 begin, s64 end) const {
        // Convert to absolute [begin, end)
        size_t beginIndex = translate_index(begin, Length);
        size_t endIndex = translate_index(end - 1, Length) + 1;

        const byte *beginPtr = Data;
        for (size_t i = 0; i < beginIndex; i++) beginPtr += get_size_of_code_point(beginPtr);
        const byte *endPtr = beginPtr;
        for (size_t i = beginIndex; i < endIndex; i++) endPtr += get_size_of_code_point(endPtr);

        string_view result;
        result.Data = beginPtr;
        result.ByteLength = (uptr_t)(endPtr - beginPtr);
        result.Length = endIndex - beginIndex;
        return result;
    }

    // Find the first occurence of _ch_
    constexpr size_t find(char32_t ch) const {
        assert(Data);
        for (size_t i = 0; i < Length; ++i)
            if (get(i) == ch) return i;
        return npos;
    }

    // Find the first occurence of _other_
    constexpr size_t find(const string_view &other) const {
        assert(Data);
        assert(other.Data);

        for (size_t haystack = 0; haystack < Length; ++haystack) {
            size_t i = haystack;
            size_t n = 0;
            while (n < other.Length && get(i) == other.get(n)) {
                ++n;
                ++i;
            }
            if (n == other.Length) {
                return haystack;
            }
        }
        return npos;
    }

    // Find the last occurence of _ch_
    constexpr size_t find_last(char32_t ch) const {
        assert(Data);
        for (size_t i = Length - 1; i >= 0; --i)
            if (get(i) == ch) return i;
        return npos;
    }

    // Find the last occurence of _other_
    constexpr size_t find_last(const string_view &other) const {
        assert(Data);
        assert(other.Data);

        for (size_t haystack = Length - 1; haystack >= 0; --haystack) {
            size_t i = haystack;
            size_t n = 0;
            while (n < other.Length && get(i) == other.get(n)) {
                ++n;
                ++i;
            }
            if (n == other.Length) {
                return haystack;
            }
        }
        return npos;
    }

    constexpr bool has(char32_t ch) const { return find(ch) != npos; }
    constexpr bool has(const string_view &other) const { return find(other) != npos; }

    // Moves the beginning forwards by n characters.
    constexpr void remove_prefix(size_t n) {
        assert(Data);
        assert(n <= Length);

        const byte *newData = get_pointer_to_code_point_at(Data, Length, n);
        ByteLength -= newData - Data;
        Length -= n;

        Data = newData;
    }

    // Moves the end backwards by n characters.
    constexpr void remove_suffix(size_t n) {
        assert(Data);
        assert(n <= Length);

        ByteLength = get_pointer_to_code_point_at(Data, Length, -(s64) n) - Data;
        Length -= n;
    }

    constexpr string_view trim() const { return trim_start().trim_end(); }
    constexpr string_view trim_start() const {
        assert(Data);

        string_view result = *this;
        For(*this) {
            if (!is_space(it)) break;

            size_t codePointSize = get_size_of_code_point(it);
            result.Data += codePointSize;
            result.ByteLength -= codePointSize;
            result.Length--;
        }
        return result;
    }

    constexpr string_view trim_end() const {
        assert(Data);

        string_view result = *this;
        for (s64 i = 1; (size_t) i <= Length; i++) {
            char32_t ch = get(-i);
            if (!is_space(ch)) break;

            size_t codePointSize = get_size_of_code_point(ch);
            result.ByteLength -= codePointSize;
            result.Length--;
        }
        return result;
    }

    bool begins_with(char32_t ch) const { return get(0) == ch; }
    bool begins_with(const string_view &other) const { return compare_memory(Data, other.Data, other.ByteLength) == 0; }

    bool ends_with(char32_t ch) const { return get(-1) == ch; }
    bool ends_with(const string_view &other) const {
        return compare_memory(Data + ByteLength - other.ByteLength, other.Data, other.ByteLength) == 0;
    }

    constexpr void swap(string_view &other) {
        {
            auto temp = Data;
            Data = other.Data;
            other.Data = temp;
        }
        {
            auto temp = ByteLength;
            ByteLength = other.ByteLength;
            other.ByteLength = temp;
        }
        {
            auto temp = Length;
            Length = other.Length;
            other.Length = temp;
        }
    }

    constexpr iterator begin() const { return iterator(Data); }
    constexpr iterator end() const { return iterator(Data + ByteLength); }

    // Compares the string view to _other_ lexicographically.
    // The result is less than 0 if this string_view sorts before the other,
    // 0 if they are equal, and greater than 0 otherwise.
    constexpr s32 compare(const string_view &other) const {
        // If the memory and the lengths are the same, the views are equal!
        if (Data == other.Data && ByteLength == other.ByteLength) return 0;

        if (Length == 0 && other.Length == 0) return 0;
        if (Length == 0) return -((s32) other.get(0));
        if (other.Length == 0) return get(0);

        auto s1 = begin(), s2 = other.begin();
        while (*s1 == *s2) {
            ++s1, ++s2;
            if (s1 == end() && s2 == other.end()) return 0;
            if (s1 == end()) return -((s32) other.get(0));
            if (s2 == other.end()) return get(0);
        }
        return ((s32) *s1 - (s32) *s2);
    }

    constexpr s32 compare(const byte *other) const { return compare(string_view(other)); }
    constexpr s32 compare(const char *other) const { return compare(string_view(other)); }

    // Compares the string view to _other_ lexicographically. Case insensitive.
    // The result is less than 0 if this string_view sorts before the other,
    // 0 if they are equal, and greater than 0 otherwise.
    constexpr s32 compare_ignore_case(const string_view &other) const {
        // If the memory and the lengths are the same, the views are equal!
        if (Data == other.Data && ByteLength == other.ByteLength) return 0;
        if (Length == 0 && other.Length == 0) return 0;
        if (Length == 0) return -((s32) to_lower(other.get(0)));
        if (other.Length == 0) return to_lower(get(0));

        auto s1 = begin(), s2 = other.begin();
        while (to_lower(*s1) == to_lower(*s2)) {
            ++s1, ++s2;
            if (s1 == end() && s2 == other.end()) return 0;
            if (s1 == end()) return -((s32) to_lower(other.get(0)));
            if (s2 == other.end()) return to_lower(get(0));
        }
        return ((s32) to_lower(*s1) - (s32) to_lower(*s2));
    }

    constexpr s32 compare_ignore_case(const byte *other) const { return compare_ignore_case(string_view(other)); }
    constexpr s32 compare_ignore_case(const char *other) const { return compare_ignore_case(string_view(other)); }

    // Check two string views for equality
    constexpr bool operator==(const string_view &other) const { return compare(other) == 0; }
    constexpr bool operator!=(const string_view &other) const { return !(*this == other); }
    constexpr bool operator<(const string_view &other) const { return compare(other) < 0; }
    constexpr bool operator>(const string_view &other) const { return compare(other) > 0; }
    constexpr bool operator<=(const string_view &other) const { return !(*this > other); }
    constexpr bool operator>=(const string_view &other) const { return !(*this < other); }

    constexpr string_view &operator=(const string_view &other) = default;
    constexpr string_view &operator=(string_view &&other) = default;

    // Read-only [] operator
    constexpr const char32_t operator[](s64 index) const { return get(index); }

    operator bool() const { return Length != 0; }
    operator Memory_View() const { return Memory_View((const byte *) Data, ByteLength); }

    // Substring operator
    constexpr string_view operator()(s64 begin, s64 end) const { return substring(begin, end); }
};

constexpr bool operator==(const byte *one, const string_view &other) { return other.compare(one) == 0; }
constexpr bool operator!=(const byte *one, const string_view &other) { return !(one == other); }
constexpr bool operator<(const byte *one, const string_view &other) { return other.compare(one) > 0; }
constexpr bool operator>(const byte *one, const string_view &other) { return other.compare(one) < 0; }
constexpr bool operator<=(const byte *one, const string_view &other) { return !(one > other); }
constexpr bool operator>=(const byte *one, const string_view &other) { return !(one < other); }

constexpr bool operator==(const char *one, const string_view &other) { return other.compare(one) == 0; }
constexpr bool operator!=(const char *one, const string_view &other) { return !(one == other); }
constexpr bool operator<(const char *one, const string_view &other) { return other.compare(one) > 0; }
constexpr bool operator>(const char *one, const string_view &other) { return other.compare(one) < 0; }
constexpr bool operator<=(const char *one, const string_view &other) { return !(one > other); }
constexpr bool operator>=(const char *one, const string_view &other) { return !(one < other); }

CPPU_END_NAMESPACE
