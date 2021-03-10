#pragma once

#include "../memory/allocator.h"
#include "hash.h"

LSTD_BEGIN_NAMESPACE

// This returns the type of the _Keys_ member of an hash table
template <typename HashTableT>
using hash_table_key_t = typename types::remove_pointer_t<decltype(HashTableT::Keys)>;

// This returns the type of the _Values_ member of an hash table
template <typename HashTableT>
using hash_table_value_t = typename types::remove_pointer_t<decltype(HashTableT::Values)>;

// #undefed at the end of the file
#define key_t hash_table_key_t
#define value_t hash_table_value_t

template <typename T>
struct key_value_pair {
    key_t<T> *Key;
    value_t<T> *Value;
};

// This hash table stores all entries in a contiguous array, for good performance when looking up things. Some tables
// work by storing linked lists of entries, but that can lead to many more cache misses.
//
// We store 3 arrays, one for the values, one for the keys and one for the hashed keys.
// Read the comment above reserve() for more information on how the arrays get allocated.
//
// When storing a value, we map its hash to a slot index and if that slot is free, we put the key and value there,
// otherwise we keep incrementing the slot index until we find an empty slot. Because the hash table can never be full, we
// are guaranteed to find a slot eventually.
//
// When looking up a value we perform the same process to find the correct slot.
//
// We use hash values to indicate whether slots are empty of removed. A hash of 0 means that slot is not used, so new
// values an be put there. A hash of 1 means that slot used to be valid, but has been removed. A hash of 2 or h igher
// (FIRST_VALID_HASH) means this is a currently used slot.
//
// Whether we hash a key, if the result is less than 2, we just add 2 to it to put it in the valid range.
// This leads to possibly more collisions, but it's a small price to pay.
//
// The template parameter _BlockAlloc_ specifies whether the hashes, keys and values arrays are allocated
// all contiguously or by seperate allocation calls. You want to allocate them next to each other because that's good for the cache,
// but if the hash table is too large then the block won't fit in the cache anyways so you should consider setting this to false to reduce
// the size of the allocation request.
template <typename K_, typename V_, bool BlockAlloc = true>
struct hash_table {
    using K = K_;
    using V = V_;
    static constexpr bool BLOCK_ALLOC = BlockAlloc;

    static constexpr s64 MINIMUM_SIZE = 32;
    static constexpr s64 FIRST_VALID_HASH = 2;

    // Number of valid items
    s64 Count = 0;

    // Number of slots allocated
    s64 Allocated = 0;

    // Number of slots that can't be used (valid + removed items)
    s64 SlotsFilled = 0;

    u64 *Hashes = null;
    K *Keys = null;
    V *Values = null;

    hash_table() {}

    // We don't use destructors for freeing memory anymore.
    // ~has_table() { free(); }

    //
    // Iterator:
    //
    template <bool Const>
    struct iterator_ {
        using hash_table_t = types::select_t<Const, const hash_table<K, V>, hash_table<K, V>>;

        hash_table_t *Parent;
        s64 Index;

        iterator_(hash_table_t *parent, s64 index = 0) : Parent(parent), Index(index) {
            assert(parent);

            // Find the first pair
            skip_empty_slots();
        }

        iterator_ &operator++() {
            ++Index;
            skip_empty_slots();
            return *this;
        }

        iterator_ operator++(int) {
            iterator_ pre = *this;
            ++(*this);
            return pre;
        }

        bool operator==(const iterator_ &other) const { return Parent == other.Parent && Index == other.Index; }
        bool operator!=(const iterator_ &other) const { return !(*this == other); }

        key_value_pair<hash_table_t> operator*() {
            return {Parent->Keys + Index, Parent->Values + Index};
        }

       private:
        void skip_empty_slots() {
            for (; Index < Parent->Allocated; ++Index) {
                if (Parent->Hashes[Index] < FIRST_VALID_HASH) continue;
                break;
            }
        }
    };

    using iterator = iterator_<false>;
    using const_iterator = iterator_<true>;

    iterator begin() { return iterator(this); }
    iterator end() { return iterator(this, Allocated); }
    const_iterator begin() const { return const_iterator(this); }
    const_iterator end() const { return const_iterator(this, Allocated); }

    //
    // Operators:
    //

    // Returns a pointer to the value associated with _key_.
    // If the key doesn't exist, this adds a new element and returns it.
    V *operator[](const K &key) {
        auto [kp, vp] = find(*this, key);
        if (vp) return vp;
        return add(*this, key, V()).Value;
    }
};

template <typename T>
struct is_hash_table : types::false_t {};

template <typename K, typename V, bool BlockAlloc>
struct is_hash_table<hash_table<K, V, BlockAlloc>> : types::true_t {};

template <typename T>
concept any_hash_table = is_hash_table<T>::value;

// Makes sure the hash table has reserved enough space for at least n elements.
// Note that it may reserve way more than required.
// Reserves space equal to the next power of two bigger than _size_, starting at _MINIMUM_SIZE_.
//
// Allocates a buffer if the hash table doesn't already point to allocated memory (using the Context's allocator).
// If _BLOCK_ALLOC_ is true it ensures that the allocated arrays are next to each other.
//
// You don't need to call this before using the hash table.
// The first time an element is added to the hash table, it reserves with _MINIMUM_SIZE_ and no specified alignment.
// You can call this before using the hash table to initialize the arrays with a custom alignment (if that's required).
//
// This is also called when adding an element and the hash table is more than half full (SlotsFilled * 2 >= Allocated).
// In that case the _target_ is exactly _SlotsFilled_.
// You may want to call this manually if you are adding a bunch of items and causing the hash table to reallocate a lot.
template <any_hash_table T>
void reserve(T &table, s64 target, u32 alignment = 0) {
    using K = key_t<T>;
    using V = value_t<T>;

    if (table.SlotsFilled + target < table.Allocated) return;
    target = max(ceil_pow_of_2(target + table.SlotsFilled + 1), table.MINIMUM_SIZE);

    auto allocateNewBlock = [&]() {
        if constexpr (table.BLOCK_ALLOC) {
            s64 padding1 = 0, padding2 = 0;
            if (alignment != 0) {
                padding1 = (target * sizeof(u64)) % alignment;
                padding2 = (target * sizeof(V)) % alignment;
            }

            s64 sizeInBytes = target * (sizeof(u64) + sizeof(K) + sizeof(V)) + padding1 + padding2;

            byte *block = allocate_array<byte>(sizeInBytes, {.Alignment = alignment});
            table.Hashes = (u64 *) block;
            table.Keys = (K *) (block + target * sizeof(u64) + padding1);
            table.Values = (V *) (block + target * (sizeof(u64) + sizeof(K)) + padding2);
            zero_memory(table.Hashes, target * sizeof(u64));
        } else {
            table.Hashes = allocate_array<u64>(target, {.Alignment = alignment});
            table.Keys = allocate_array<K>(target, {.Alignment = alignment});
            table.Values = allocate_array<V>(target, {.Alignment = alignment});
            zero_memory(table.Hashes, target * sizeof(u64));
        }
    };

    if (table.Allocated) {
        auto oldAlignment = ((allocation_header *) table.Hashes - 1)->Alignment;
        if (alignment == 0) {
            alignment = oldAlignment;
        } else {
            assert(alignment == oldAlignment && "Reserving with an alignment but the object already has arrays with a different alignment. Specify alignment 0 to automatically use the old one.");
        }

        auto *oldHashes = table.Hashes;
        auto *oldKeys = table.Keys;
        auto *oldValues = table.Values;
        auto oldAllocated = table.Allocated;

        allocateNewBlock();

        // Add the old items
        For(range(oldAllocated)) {
            if (oldHashes[it] >= table.FIRST_VALID_HASH) add_prehashed(table, oldHashes[it], oldKeys[it], oldValues[it]);
        }

        free(oldHashes);

        if constexpr (!table.BLOCK_ALLOC) {
            free(oldKeys);
            free(oldValues);
        }
    } else {
        // It's impossible to have a view into a hash table (currently).
        // So there were no previous elements.
        assert(!table.Count);
        allocateNewBlock();
    }
    table.Allocated = target;
}

// Free any memory allocated by this object and reset count
template <any_hash_table T>
void free(T &table) {
    if (table.Allocated) {
        free(table.Hashes);
        if constexpr (!table.BLOCK_ALLOC) {
            free(table.Keys);
            free(table.Values);
        }
    }
    table.Hashes = null;
    table.Keys = null;
    table.Values = null;
    table.Count = table.SlotsFilled = table.Allocated = 0;
}

// Don't free the hash table, just destroy contents and reset count
template <any_hash_table T>
void reset(T &table) {
    // PODs may have destructors, although the C++ standard's definition forbids them to have non-trivial ones.
    if (table.Allocated) {
        // @TODO: Factor this into uninitialize_block() function and use it in free() as well
        auto *p = table.Hashes, *end = table.Hashes + table.Allocated;
        s64 index = 0;
        while (p != end) {
            if (*p) {
                table.Keys[index].~key_t();
                table.Values[index].~value_t();
                *p = 0;
            }
            ++index, ++p;
        }
    }
    table.Count = table.SlotsFilled = 0;
}

// Looks for key in the hash table using the given hash.
// In normal _find_ we calculate the hash of the key using the global get_hash() specialized functions.
// This method is useful if you have cached the hash.
template <any_hash_table T>
key_value_pair<T> find_prehashed(const T &table, u64 hash, const key_t<T> &key) {
    if (!table.Count) return {null, null};

    s64 index = hash & (table.Allocated - 1);
    For(range(table.Allocated)) {
        if (table.Hashes[index] == hash) {
            if (table.Keys[index] == key) {
                return {table.Keys + index, table.Values + index};
            }
        }

        ++index;
        if (index >= table.Allocated) {
            index = 0;
        }
    }
    return {null, null};
}

// We calculate the hash of the key using the global get_hash() specialized functions.
template <any_hash_table T>
key_value_pair<T> find(const T &table, const key_t<T> &key) {
    return find_prehashed(table, get_hash(key), key);
}

// Adds key and value to the hash table using the given hash.
// In normal _add_ we calculate the hash of the key using the global get_hash() specialized functions.
// This method is useful if you have cached the hash.
// Returns pointers to the added key and value.
template <any_hash_table T>
key_value_pair<T> add_prehashed(T &table, u64 hash, const key_t<T> &key, const value_t<T> &value) {
    // The + 1 here handles the case when the hash table size is 1 and you add the first item.
    if ((table.SlotsFilled + 1) * 2 >= table.Allocated) reserve(table, table.SlotsFilled);  // Make sure the hash table is never more than 50% full

    assert(table.SlotsFilled < table.Allocated);

    if (hash < table.FIRST_VALID_HASH) hash += table.FIRST_VALID_HASH;

    s64 index = hash & (table.Allocated - 1);
    while (table.Hashes[index]) {
        ++index;
        if (index >= table.Allocated) index = 0;
    }

    ++table.Count;
    ++table.SlotsFilled;

    table.Hashes[index] = hash;
    new (table.Keys + index) key_t<T>(key);
    new (table.Values + index) value_t<T>(value);
    return {table.Keys + index, table.Values + index};
}

// Inserts an empty value at a specified key and returns pointers to the key and value in the buffers.
//
// This is useful for the following way to clone add an object (because by default we just shallow-copy the object).
//
// value_t toBeCloned = ...;
// auto [kp, vp] = table.add(key);
// clone(vp, toBeCloned);
//
// Because _add_ returns a pointer where the object is placed, clone() can place the deep copy there directly.
//
// We calculate the hash of the key using the global get_hash() specialized functions.
template <any_hash_table T>
key_value_pair<T> add(T &table, const key_t<T> &key) { return add(table, key, value_t<T>()); }

// Inserts an empty key/value pair with a given hash.
// Use the returned pointers to fill out the slots.
// This is useful if you want to clone() the key and value and not just shallow copy them.
template <any_hash_table T>
key_value_pair<T> add(T &table, u64 hash) {
    return add_prehashed(table, hash, key_t<T>(), value_t<T>());
}

// We calculate the hash of the key using the global get_hash() specialized functions.
// Returns pointers to the added key and value.
template <any_hash_table T>
key_value_pair<T> add(T &table, const key_t<T> &key, const value_t<T> &value) {
    return add_prehashed(table, get_hash(key), key, value);
}

// In normal _set_ we calculate the hash of the key using the global get_hash() specialized functions.
// This method is useful if you have cached the hash.
template <any_hash_table T>
key_value_pair<T> set_prehashed(T &table, u64 hash, const key_t<T> &key, const value_t<T> &value) {
    auto [kp, vp] = find_prehashed(table, hash, key);
    if (vp) {
        *vp = value;
        return {kp, vp};
    }
    return add(table, key, value);
}

// We calculate the hash of the key using the global get_hash() specialized functions.
template <any_hash_table T>
key_value_pair<T> set(T &table, const key_t<T> &key, const value_t<T> &value) {
    return set_prehashed(table, get_hash(key), key, value);
}

// Returns true if the key was found and removed.
// In normal _remove_ we calculate the hash of the key using the global get_hash() specialized functions.
// This method is useful if you have cached the hash.
template <any_hash_table T>
bool remove_prehashed(T &table, u64 hash, const key_t<T> &key) {
    auto *ptr = find(table, hash, key);
    if (ptr) {
        s64 index = ptr - table.Values;
        table.Hashes[index] = 1;
        return true;
    }
    return false;
}

// Returns true if the key was found and removed.
// We calculate the hash of the key using the global get_hash() specialized functions.
template <any_hash_table T>
bool remove(T &table, const key_t<T> &key) {
    return remove_prehashed(table, get_hash(key), key);
}

// Returns true if the hash table has the given key.
// We calculate the hash of the key using the global get_hash() specialized functions.
template <any_hash_table T>
bool has(const T &table, const key_t<T> &key) { return find(table, key) != null; }

// Returns true if the hash table has the given key.
// In normal _hash_ we calculate the hash of the key using the global get_hash() specialized functions.
// This method is useful if you have cached the hash.
template <any_hash_table T>
bool has_prehashed(const T &table, u64 hash, const key_t<T> &key) { return find_prehashed(table, hash, key) != null; }

#undef key_t
#undef value_t

template <typename K, typename V, bool BlockAlloc>
hash_table<K, V, BlockAlloc> *clone(hash_table<K, V, BlockAlloc> *dest, const hash_table<K, V, BlockAlloc> &src) {
    free(*dest);
    for (auto [k, v] : src) add(*dest, *k, *v);
    return dest;
}

LSTD_END_NAMESPACE
