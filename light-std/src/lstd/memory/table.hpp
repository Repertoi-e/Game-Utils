#pragma once

#include "../context.hpp"
#include "memory.hpp"

#include "hash.hpp"

#include <tuple>

LSTD_BEGIN_NAMESPACE

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

    Table() {}
    Table(const Table &other) {
        Count = other.Count;
        Reserved = other.Reserved;
        UnfoundValue = other.UnfoundValue;

        OccupancyMask = new (&Allocator, ensure_allocator) bool[Reserved];
        Keys = new (Allocator) Key_Type[Reserved];
        Values = new (Allocator) Value_Type[Reserved];
        Hashes = new (Allocator) uptr_t[Reserved];

        copy_elements(OccupancyMask, other.OccupancyMask, Reserved);
        copy_elements(Keys, other.Keys, Reserved);
        copy_elements(Values, other.Values, Reserved);
        copy_elements(Hashes, other.Hashes, Reserved);
    }

    Table(Table &&other) { other.swap(*this); }
    ~Table() { release(); }

    void release() {
        if (Reserved) {
            delete[] OccupancyMask;
            delete[] Keys;
            delete[] Values;
            delete[] Hashes;

            OccupancyMask = null;
            Keys = null;
            Values = null;
            Hashes = null;
            Reserved = 0;
            Count = 0;
        }
    }

    // Copies the key and the value into the table.
    void put(const Key_Type &key, const Value_Type &value) {
        uptr_t hash = Hash<Key_Type>::get(key);
        s32 index = find_index(key, hash);
        if (index == -1) {
            if (Count >= Reserved) {
                grow();
            }
            assert(Count <= Reserved);

            index = (s32)(hash % Reserved);
            while (OccupancyMask[index]) {
                // Resolve collision
                index++;
                if (index >= Reserved) {
                    index = 0;
                }
            }

            Count++;
        }

        OccupancyMask[index] = true;
        new (&Keys[index]) Key_Type(key);
        new (&Values[index]) Value_Type(value);
        Hashes[index] = hash;
    }

    // Returns a tuple of the value and a bool (true if found). Doesn't return the value by reference so
    // modifying it doesn't update it in the table, use pointers if you want that kind of behaviour.
    std::tuple<Value_Type &, bool> find(const Key_Type &key) {
        uptr_t hash = Hash<Key_Type>::get(key);

        s32 index = find_index(key, hash);
        if (index == -1) {
            return {UnfoundValue, false};
        }

        return std::forward_as_tuple(Values[index], true);
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

        std::swap(OccupancyMask, other.OccupancyMask);
        std::swap(Keys, other.Keys);
        std::swap(Values, other.Values);
        std::swap(Hashes, other.Hashes);
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

   private:
    // This doesn't free the old memory before allocating new, so that's up to the caller to do.
    void reverse(size_t size) {
        Reserved = size;

        OccupancyMask = new (&Allocator, ensure_allocator) bool[size];
        Keys = new (Allocator) Key[size];
        Values = new (Allocator) Value[size];
        Hashes = new (Allocator) uptr_t[size];
    }

    s32 find_index(const Key_Type &key, uptr_t hash) {
        if (!Reserved) {
            return -1;
        }

        size_t index = hash % Reserved;
        while (OccupancyMask[index]) {
            if (Hashes[index] == hash) {
                if (Keys[index] == key) {
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
    void grow() {
        size_t oldReserved = Reserved;
        auto oldOccupancyMask = OccupancyMask;
        auto oldKeys = Keys;
        auto oldValues = Values;
        auto oldHashes = Hashes;

        size_t newSize = Reserved * 2;
        if (newSize < MINIMUM_SIZE) {
            newSize = MINIMUM_SIZE;
        }

        reverse(newSize);

        For(range(oldReserved)) {
            if (oldOccupancyMask[it]) {
                put(oldKeys[it], oldValues[it]);
            }
        }

        if (oldReserved) {
            delete[] oldOccupancyMask;
            delete[] oldKeys;
            delete[] oldValues;
            delete[] oldHashes;
        }
    }

    // We store slots as SOA to minimize cache misses.
    bool *OccupancyMask = null;
    Key_Type *Keys = null;
    Value_Type *Values = null;
    uptr_t *Hashes = null;

    friend struct Table_Iterator<Key_Type, Value_Type>;
};

template <typename Key, typename Value>
struct Table_Iterator {
    const Table<Key, Value> &ParentTable;
    s64 SlotIndex = 0;

    explicit Table_Iterator(const Table<Key, Value> &table, s64 index = -1) : ParentTable(table), SlotIndex(index) {
        // Find the first pair
        ++(*this);
    }

    Table_Iterator &operator++() {
        while (SlotIndex < (s64) ParentTable.Reserved) {
            SlotIndex++;
            if (SlotIndex == ParentTable.Reserved) break;
            if (ParentTable.OccupancyMask && ParentTable.OccupancyMask[SlotIndex]) {
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
        return std::forward_as_tuple(ParentTable.Keys[SlotIndex], ParentTable.Values[SlotIndex]);
    }
};

LSTD_END_NAMESPACE