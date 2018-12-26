#pragma once

#include "../context.hpp"
#include "memory.hpp"

#include "hash.hpp"

#include <iterator>
#include <tuple>

CPPU_BEGIN_NAMESPACE

template <typename Key, typename Value>
struct Table_Iterator;

// Table means hash-map/undordered_map etc.
template <typename Key, typename Value>
struct Table {
    using Key_Type = Key;
    using Value_Type = Value;

    static const size_t MINIMUM_SIZE = 32;
    size_t Count = 0;
    size_t Reserved = 0;

    // By default, the value that gets returned if a value is not found
    // is a default constructed Value_Type. This value can be changed if
    // special behaviour is desired.
    Value_Type UnfoundValue = Value_Type();

    // The allocator used for expanding the table.
    // This value is null until this object allocates memory or the user sets it manually.
    Allocator_Closure Allocator;

    // We store slots as SOA to minimize cache misses.
    bool *_OccupancyMask = null;
    Key_Type *_Keys = null;
    Value_Type *_Values = null;
    uptr_t *_Hashes = null;

    Table() {}
    Table(const Table &other) {
        Count = other.Count;
        Reserved = other.Reserved;
        UnfoundValue = other.UnfoundValue;

        _OccupancyMask = New_and_ensure_allocator<bool>(Reserved, Allocator);
        _Keys = New<Key_Type>(Reserved, Allocator);
        _Values = New<Value_Type>(Reserved, Allocator);
        _Hashes = New<uptr_t>(Reserved, Allocator);

        copy_elements(_OccupancyMask, other._OccupancyMask, Reserved);
        copy_elements(_Keys, other._Keys, Reserved);
        copy_elements(_Values, other._Values, Reserved);
        copy_elements(_Hashes, other._Hashes, Reserved);
    }

    Table(Table &&other) { other.swap(*this); }
    ~Table() { release(); }

    void release() {
        if (Reserved) {
            Delete(_OccupancyMask, Reserved, Allocator);
            Delete(_Keys, Reserved, Allocator);
            Delete(_Values, Reserved, Allocator);
            Delete(_Hashes, Reserved, Allocator);

            _OccupancyMask = null;
            _Keys = null;
            _Values = null;
            _Hashes = null;
            Reserved = 0;
            Count = 0;
        }
    }

    // Copies the key and the value into the table.
    void put(const Key_Type &key, const Value_Type &value) {
        uptr_t hash = Hash<Key_Type>::get(key);
        s32 index = _find_index(key, hash);
        if (index == -1) {
            if (Count >= Reserved) {
                _expand();
            }
            assert(Count <= Reserved);

            index = (s32)(hash % Reserved);
            while (_OccupancyMask[index]) {
                // Resolve collision
                index++;
                if (index >= Reserved) {
                    index = 0;
                }
            }

            Count++;
        }

        _OccupancyMask[index] = true;
        _Keys[index] = key;
        _Values[index] = value;
        _Hashes[index] = hash;
    }

    // Returns a tuple of the value and a bool (true if found). Doesn't return the value by reference so
    // modifying it doesn't update it in the table, use pointers if you want that kind of behaviour.
    std::tuple<Value_Type &, bool> find(const Key_Type &key) {
        uptr_t hash = Hash<Key_Type>::get(key);

        s32 index = _find_index(key, hash);
        if (index == -1) {
            return {UnfoundValue, false};
        }

        return std::forward_as_tuple(_Values[index], true);
    }

    bool has(const Key_Type &key) {
        auto [_, found] = find(key);
        return found;
    }

    Table_Iterator<Key, Value> begin() { return Table_Iterator<Key, Value>(*this); }
    Table_Iterator<Key, Value> end() { return Table_Iterator<Key, Value>(*this, Reserved); }
    const Table_Iterator<Key, Value> begin() const { return Table_Iterator<Key, Value>(*this); }
    const Table_Iterator<Key, Value> end() const { return Table_Iterator<Key, Value>(*this, Reserved); }

    void swap(Table &other) {
        std::swap(Count, other.Count);
        std::swap(Allocator, other.Allocator);
        std::swap(Reserved, other.Reserved);
        std::swap(UnfoundValue, other.UnfoundValue);

        std::swap(_OccupancyMask, other._OccupancyMask);
        std::swap(_Keys, other._Keys);
        std::swap(_Values, other._Values);
        std::swap(_Hashes, other._Hashes);
    }

    Table &operator=(const Table &other) {
        release();

        Table(other).swap(*this);
        return *this;
    }

    Table &operator=(Table &&other) {
        release();

        Table(std::move(other)).swap(*this);
        return *this;
    }

    // This is a very internal function. Basically don't call this unless
    // you absolutely know what you are doing. This doesn't free the old memory
    // before allocating new, so that's up to the caller to do. Trust me
    // there is a reason why this function does what it does, it makes _expand()
    // much much simpler.
    void _reverse(size_t size) {
        Reserved = size;

        _OccupancyMask = New_and_ensure_allocator<bool>(size, Allocator);
        _Keys = New<Key>(size, Allocator);
        _Values = New<Value>(size, Allocator);
        _Hashes = New<uptr_t>(size, Allocator);
    }

    s32 _find_index(const Key_Type &key, uptr_t hash) {
        if (!Reserved) {
            return -1;
        }

        size_t index = hash % Reserved;
        while (_OccupancyMask[index]) {
            if (_Hashes[index] == hash) {
                if (_Keys[index] == key) {
                    return (s32) index;
                }
            }

            index++;
            if (index >= Reserved) {
                index = 0;
            }
        }

        return -1;
    }

    // Double's the size of the table and copies the elements to their new location.
    void _expand() {
        // I love C++
        // I love C++
        // I love C++
        size_t oldReserved = Reserved;
        auto oldOccupancyMask = _OccupancyMask;
        auto oldKeys = _Keys;
        auto oldValues = _Values;
        auto oldHashes = _Hashes;

        size_t newSize = Reserved * 2;
        if (newSize < MINIMUM_SIZE) {
            newSize = MINIMUM_SIZE;
        }

        _reverse(newSize);

        for (size_t i = 0; i < oldReserved; i++) {
            if (oldOccupancyMask[i]) {
                put(oldKeys[i], oldValues[i]);
            }
        }

        if (oldReserved) {
            Delete(oldOccupancyMask, oldReserved, Allocator);
            Delete(oldKeys, oldReserved, Allocator);
            Delete(oldValues, oldReserved, Allocator);
            Delete(oldHashes, oldReserved, Allocator);
        }
    }
};

template <typename Key, typename Value>
struct Table_Iterator : public std::iterator<std::forward_iterator_tag, std::tuple<Key &, Value &>> {
    const Table<Key, Value> &Table;
    s64 SlotIndex = -1;

    explicit Table_Iterator(const ::Table<Key, Value> &table, s64 index = -1) : Table(table), SlotIndex(index) {
        // Find the first pair
        ++(*this);
    }

    Table_Iterator &operator++() {
        while (SlotIndex < (s64) Table.Reserved) {
            SlotIndex++;
            if (Table._OccupancyMask && Table._OccupancyMask[SlotIndex]) {
                break;
            }
        }
        return *this;
    }

    Table_Iterator operator++(int) {
        Table_Iterator pre = *this;
        ++(*this);
        return pre;
    }

    bool operator==(Table_Iterator other) const { return SlotIndex == other.SlotIndex; }
    bool operator!=(Table_Iterator other) const { return !(*this == other); }

    std::tuple<Key &, Value &> operator*() const {
        return std::forward_as_tuple(Table._Keys[SlotIndex], Table._Values[SlotIndex]);
    }
};

CPPU_END_NAMESPACE