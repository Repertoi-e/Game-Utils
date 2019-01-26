#pragma once

#include "../common.hpp"

#include "range.hpp"

#include <algorithm>

LSTD_BEGIN_NAMESPACE

template <typename T, size_t Size>
struct Array {
    T Data[Size];
    static constexpr size_t Count = Size;

    using iterator = T *;
    using const_iterator = const T *;

    constexpr iterator begin() { return Data; }
    constexpr iterator end() { return Data + Count; }
    constexpr const_iterator begin() const { return Data; }
    constexpr const_iterator end() const { return Data + Count; }

    // Find the index of the first occuring _item_ in the array, npos if it's not found
    constexpr size_t find(const T &item) const {
        const T *index = Data;
        For(range(Count)) {
            if (*index++ == item) {
                return it;
            }
        }
        return npos;
    }

    // Find the index of the last occuring _item_ in the array, npos if it's not found
    constexpr size_t find_last(const T &item) const {
        const T *index = Data + Count - 1;
        For(range(Count)) {
            if (*index-- == item) {
                return Count - it - 1;
            }
        }
        return npos;
    }

    constexpr void sort() { std::sort(begin(), end()); }
    template <typename Pred>

    constexpr void sort(Pred &&predicate) {
        std::sort(begin(), end(), predicate);
    }

    constexpr bool has(const T &item) const { return find(item) != npos; }

    constexpr T &get(size_t index) { return Data[index]; }
    constexpr const T &get(size_t index) const { return Data[index]; }

    constexpr T &operator[](size_t index) { return get(index); }
    constexpr const T &operator[](size_t index) const { return get(index); }

    constexpr bool operator==(const Array &other) {
        if (Count != other.Count) return false;
        For(range(Count)) {
            if (Data[it] != other.Data[it]) {
                return false;
            }
        }
        return true;
    }

    constexpr bool operator!=(const Array &other) { return !(*this == other); }
};

template <typename... T>
constexpr auto to_array(T &&... values)
    -> Array<typename std::decay_t<typename std::common_type_t<T...>>, sizeof...(T)> {
    return Array<typename std::decay_t<typename std::common_type_t<T...>>, sizeof...(T)>{std::forward<T>(values)...};
}

LSTD_END_NAMESPACE