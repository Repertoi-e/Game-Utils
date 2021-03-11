module;

#include "../internal/context.h"
#include "../io.h"
#include "../math.h"
#include "../os.h"

module lstd.allocator;

import path;
import fmt;

#if defined BRACE_B
#error ""
#endif

#if defined BRACE_A
#error ""
#endif

LSTD_BEGIN_NAMESPACE

#if defined DEBUG_MEMORY
void DEBUG_memory_info::unlink_header(allocation_header *header) {
    thread::scoped_lock<thread::mutex> _(&Mutex);

    assert(header);
    assert(Head);

    if (header == Head) {
        Head = header->DEBUG_Next;
    }

    if (header->DEBUG_Next) {
        header->DEBUG_Next->DEBUG_Previous = header->DEBUG_Previous;
    }

    if (header->DEBUG_Previous) {
        header->DEBUG_Previous->DEBUG_Next = header->DEBUG_Next;
    }
}

void DEBUG_memory_info::add_header(allocation_header *header) {
    thread::scoped_lock<thread::mutex> _(&Mutex);

    header->DEBUG_Next = Head;
    if (Head) {
        Head->DEBUG_Previous = header;
    }
    Head = header;
}
void DEBUG_memory_info::swap_header(allocation_header *oldHeader, allocation_header *newHeader) {
    auto *prev = oldHeader->DEBUG_Previous;
    auto *next = oldHeader->DEBUG_Next;

    assert(Head);  // ?

    thread::scoped_lock<thread::mutex> _(&Mutex);
    if (prev) {
        prev->DEBUG_Next = newHeader;
        newHeader->DEBUG_Previous = prev;
    } else {
        Head = newHeader;
    }

    if (next) {
        next->DEBUG_Previous = newHeader;
        newHeader->DEBUG_Next = next;
    }
}

// Copied from test.h
//
// We check if the path contains src/ and use the rest after that.
// Otherwise we just take the file name. Possible results are:
//
//      /home/.../game/src/some_dir/a/string.cpp ---> some_dir/a/localization.cpp
//      /home/.../game/some_dir/string.cpp       ---> localization.cpp
//
constexpr string get_short_file_name(const string &str) {
    char srcData[] = {'s', 'r', 'c', OS_PATH_SEPARATOR, '\0'};
    string src = srcData;

    s64 findResult = find_substring_reverse(str, src);
    if (findResult == -1) {
        findResult = find_cp_reverse(str, OS_PATH_SEPARATOR);
        assert(findResult != str.Length - 1);
        // Skip the slash
        findResult++;
    } else {
        // Skip the src directory
        findResult += src.Length;
    }

    string result = str;
    return substring(result, findResult, result.Length);
}

void DEBUG_memory_info::report_leaks() {
    // First we check their integrity of the heap
    maybe_verify_heap();

    allocation_header **leaks;
    // We want to ignore the allocation below since it's not the user's fault and we shouldn't count it as a leak
    s64 leaksID;

    s64 leaksCount = 0;
    {
        thread::scoped_lock<thread::mutex> _(&Mutex);
        auto *it = Head;
        while (it) {
            if (!it->MarkedAsLeak) ++leaksCount;
            it = it->DEBUG_Next;
        }
    }
    // @Cleanup: We can mark this as LEAK?
    leaks = allocate_array<allocation_header *>(leaksCount);
    leaksID = ((allocation_header *) leaks - 1)->ID;

    {
        thread::scoped_lock<thread::mutex> _(&Mutex);
        auto *p = leaks;
        auto *it = Head;
        while (it) {
            if (!it->MarkedAsLeak && it->ID != leaksID) *p++ = it;
            it = it->DEBUG_Next;
        }
    }

    if (leaksCount) {
        print(">>> Warning: The module {!YELLOW}\"{}\"{!} terminated but it still had {!YELLOW}{}{!} allocations which were unfreed. Here they are:\n", os_get_current_module(), leaksCount);
    }

    For_as(i, range(leaksCount)) {
        auto *it = leaks[i];

        string file = "Unknown";

        //
        // @Cleanup D I R T Y @Cleanup @Cleanup @Cleanup
        //
        if (compare_c_string(it->FileName, "") != -1) {
            file = get_short_file_name(it->FileName);
        }

        print("    * {}:{} requested {!GRAY}{}{!} bytes, {{ID: {}, RID: {}}}\n", file, it->FileLine, it->Size, it->ID, it->RID);
    }
}

file_scope void verify_header_unlocked(allocation_header *header) {
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // If an assert fires here it means that memory was messed up in some way.
    //
    // We check for several problems here:
    //   * Accessing headers which were freed. Note: This doesn't mean that the user code attempted to modify/access
    //     memory which we marked as freed. When calling free() we unlink the header from our list but before that we
    //     fill it with DEAD_LAND_FILL. The idea is to make the memory invalid so the user code crashes if a read is attempted.
    //     Here we check if _header_ was freed but for some reason we are trying to verify it.
    //   * Alignment should not be 0, should be more than POINTER_SIZE (4 or 8) and should be a power of 2.
    //     If any of these is not true, then the header was definitely corrupted.
    //   * We store a pointer to the memory block at the end of the header, any valid header will have this pointer point after itself.
    //     Otherwise the header was definitely corrupted.
    //   * No man's land was modified. This means that you wrote before or after the allocated block.
    //     This catches programmer errors related to writing stuff to memory which is not *supposed* to be valid.
    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    char freedHeader[sizeof(allocation_header)];
    fill_memory(freedHeader, DEAD_LAND_FILL, sizeof(allocation_header));
    if (compare_memory(header, freedHeader, sizeof(allocation_header)) == -1) {
        assert(false && "Trying to access freed memory!");
    }

    assert(header->Alignment && "Alignment is zero. Definitely corrupted.");
    assert(header->Alignment >= POINTER_SIZE && "Alignment smaller than pointer size. Definitely corrupted.");
    assert(is_pow_of_2(header->Alignment) && "Alignment not a power of 2. Definitely corrupted.");

    assert(header->DEBUG_Pointer == header + 1 && "Debug pointer doesn't match. They should always match.");

    auto *user = (char *) header + sizeof(allocation_header);

    char noMansLand[NO_MANS_LAND_SIZE];
    fill_memory(noMansLand, NO_MANS_LAND_FILL, NO_MANS_LAND_SIZE);
    assert(compare_memory((char *) user - NO_MANS_LAND_SIZE, noMansLand, NO_MANS_LAND_SIZE) == -1 &&
           "No man's land was modified. This means that you wrote before the allocated block.");

    assert(compare_memory((char *) header->DEBUG_Pointer + header->Size, noMansLand, NO_MANS_LAND_SIZE) == -1 &&
           "No man's land was modified. This means that you wrote after the allocated block.");

    //
    // If one of these asserts was triggered in _maybe_verify_heap()_, this can also mean that the linked list is messed up
    // (possibly by modifying the prev/next pointers in the header).
    //
}

void DEBUG_memory_info::verify_header(allocation_header *header) {
    // We need to lock here because another thread can free a header while we are reading from it.
    thread::scoped_lock<thread::mutex> _(&Mutex);
    verify_header_unlocked(header);
}

void DEBUG_memory_info::maybe_verify_heap() {
    // We need to lock here because another thread can free a header while we are reading from it.
    thread::scoped_lock<thread::mutex> _(&Mutex);

    if (AllocationCount % MemoryVerifyHeapFrequency) return;

    auto *it = Head;
    while (it) {
        verify_header_unlocked(it);
        it = it->DEBUG_Next;
    }
}
#endif

// Calculates the required padding in bytes which needs to be added to _ptr_ in order to be aligned
u16 calculate_padding_for_pointer(void *ptr, s32 alignment) {
    assert(alignment > 0 && is_pow_of_2(alignment));
    return (u16)((((u64) ptr + (alignment - 1)) & -alignment) - (u64) ptr);
}

// Like calculate_padding_for_pointer but padding must be at least the header size
u16 calculate_padding_for_pointer_with_header(void *ptr, s32 alignment, u32 headerSize) {
    u16 padding = calculate_padding_for_pointer(ptr, alignment);
    if (padding < headerSize) {
        headerSize -= padding;
        if (headerSize % alignment) {
            padding += alignment * (1 + (headerSize / alignment));
        } else {
            padding += alignment * (headerSize / alignment);
        }
    }
    return padding;
}

file_scope void *encode_header(void *p, s64 userSize, u32 align, allocator alloc, u64 flags) {
    u32 padding = calculate_padding_for_pointer_with_header(p, align, sizeof(allocation_header));
    u32 alignmentPadding = padding - sizeof(allocation_header);

    auto *result = (allocation_header *) ((char *) p + alignmentPadding);

#if defined DEBUG_MEMORY
    result->DEBUG_Next = null;
    result->DEBUG_Previous = null;

    result->ID = (u32) DEBUG_memory_info::AllocationCount;
    atomic_inc(&DEBUG_memory_info::AllocationCount);

    result->RID = 0;
#endif

    result->Alloc = alloc;
    result->Size = userSize;

    result->Alignment = align;
    result->AlignmentPadding = alignmentPadding;

    //
    // This is now safe since we handle alignment here (and not in general_(re)allocate).
    // Before I wrote the fix the program was crashing because I was using SIMD types,
    // which require to be aligned, on memory not 16-aligned.
    // I tried allocating with specified alignment but it wasn't taking into
    // account the size of the allocation header (accounting happened before bumping
    // the resulting pointer here).
    //
    // Since I had to redo how alignment was handled I decided to remove ALLOCATE_ALIGNED
    // and REALLOCATE_ALIGNED and drastically simplify allocator implementations.
    // What we do now is request a block of memory with a size
    // that was calculated with alignment in mind.
    //                                                                              - 5.04.2020
    //
    // Since then we do this again differently because there was a bug where reallocating was having
    // issues with _AlignmentPadding_. Now we require allocators to implement RESIZE instead of REALLOCATE
    // which mustn't move the block but instead return null if resizing failed to tell us we need to allocate a new one.
    // This moves handling reallocation entirely on our side, which, again is even cleaner.
    //                                                                              - 18.05.2020
    //
    p = result + 1;
    assert((((u64) p & ~((s64) align - 1)) == (u64) p) && "Pointer wasn't properly aligned.");

#if defined DEBUG_MEMORY
    fill_memory(p, CLEAN_LAND_FILL, userSize);

    fill_memory((char *) p - NO_MANS_LAND_SIZE, NO_MANS_LAND_FILL, NO_MANS_LAND_SIZE);
    fill_memory((char *) p + userSize, NO_MANS_LAND_FILL, NO_MANS_LAND_SIZE);

    result->DEBUG_Pointer = result + 1;

    result->MarkedAsLeak = flags & LEAK;
#endif

    return p;
}

file_scope void log_file_and_line(source_location loc) {
    write(Context.Log, loc.File);
    write(Context.Log, ":");

    utf8 number[20];

    auto line = loc.Line;

    auto *numberP = number + 19;
    s64 numberSize = 0;
    {
        while (line) {
            *numberP-- = line % 10 + '0';
            line /= 10;
            ++numberSize;
        }
    }
    write(Context.Log, (const byte *) numberP + 1, numberSize);
}

void *general_allocate(allocator alloc, s64 userSize, u32 alignment, u64 options, source_location loc) {
    options |= Context.AllocOptions;

    if (alignment == 0) {
        auto contextAlignment = Context.AllocAlignment;
        assert(is_pow_of_2(contextAlignment));
        alignment = contextAlignment;
    }

#if defined DEBUG_MEMORY
    DEBUG_memory_info::maybe_verify_heap();

    s64 id = DEBUG_memory_info::AllocationCount;

    if (id == 1238) {
        s32 k = 42;
    }
#endif

    if (Context.LogAllAllocations && !Context.LoggingAnAllocation) {
        WITH_CONTEXT_VAR(LoggingAnAllocation, true) {
            write(Context.Log, ">>> Allocation made at: ");
            log_file_and_line(loc);
            write(Context.Log, "\n");
        }
    }

    alignment = alignment < POINTER_SIZE ? POINTER_SIZE : alignment;
    assert(is_pow_of_2(alignment));

    s64 required = userSize + alignment + sizeof(allocation_header) + (sizeof(allocation_header) % alignment);
#if defined DEBUG_MEMORY
    required += NO_MANS_LAND_SIZE;  // This is for the bytes after the requested block
#endif

    void *block = alloc.Function(allocator_mode::ALLOCATE, alloc.Context, required, null, 0, &options);
    auto *result = encode_header(block, userSize, alignment, alloc, options);

#if defined DEBUG_MEMORY
    auto *header = (allocation_header *) result - 1;

    header->FileName = loc.File;
    header->FileLine = loc.Line;

    DEBUG_memory_info::add_header(header);
#endif

    return result;
}

void *general_reallocate(void *ptr, s64 newUserSize, u64 options, source_location loc) {
    options |= Context.AllocOptions;

    auto *header = (allocation_header *) ptr - 1;

    if (header->Size == newUserSize) return ptr;

#if defined DEBUG_MEMORY
    DEBUG_memory_info::maybe_verify_heap();

    auto id = header->ID;
#endif

    if (Context.LogAllAllocations && !Context.LoggingAnAllocation) {
        WITH_CONTEXT_VAR(LoggingAnAllocation, true) {
            write(Context.Log, ">>> Reallocation made at: ");
            log_file_and_line(loc);
            write(Context.Log, "\n");
        }
    }

    // The header stores the size of the requested allocation
    // (so the user code can look at the header and not be confused with garbage)
    s64 extra = sizeof(allocation_header) + header->Alignment + (sizeof(allocation_header) % header->Alignment);

    s64 oldUserSize = header->Size;

    s64 oldSize = oldUserSize + extra;
    s64 newSize = newUserSize + extra;

#if defined DEBUG_MEMORY
    oldSize += NO_MANS_LAND_SIZE;
    newSize += NO_MANS_LAND_SIZE;
#endif

    auto alloc = header->Alloc;

    void *block = (char *) header - header->AlignmentPadding;
    void *p;

    // Try to resize the block, this returns null if the block can't be resized and we need to move it.
    void *newBlock = alloc.Function(allocator_mode::RESIZE, alloc.Context, newSize, block, oldSize, &options);
    if (!newBlock) {
        // Memory needs to be moved
        void *newBlock = alloc.Function(allocator_mode::ALLOCATE, alloc.Context, newSize, null, 0, &options);
        auto *newPointer = encode_header(newBlock, newUserSize, header->Alignment, alloc, options);

        auto *newHeader = (allocation_header *) newPointer - 1;

        copy_memory(newPointer, ptr, header->Size);

#if defined DEBUG_MEMORY
        newHeader->ID = id;
        newHeader->RID = header->RID + 1;

        DEBUG_memory_info::swap_header(header, newHeader);
        fill_memory(block, DEAD_LAND_FILL, oldSize);

        newHeader->FileName = loc.File;
        newHeader->FileLine = loc.Line;

        newHeader->MarkedAsLeak = header->MarkedAsLeak;
#endif
        alloc.Function(allocator_mode::FREE, alloc.Context, 0, block, oldSize, &options);

        p = (void *) (newHeader + 1);
    } else {
        // The block was resized sucessfully and it doesn't need moving
        assert(block == newBlock);  // Sanity

#if defined DEBUG_MEMORY
        ++header->RID;

        header->FileName = loc.File;
        header->FileLine = loc.Line;
#endif
        header->Size = newUserSize;

        p = (void *) (header + 1);
    }

#if defined DEBUG_MEMORY
    if (oldSize < newSize) {
        // If we are expanding the memory, fill the new stuff with CLEAN_LAND_FILL
        fill_memory((char *) p + oldUserSize, CLEAN_LAND_FILL, newSize - oldSize);
    } else {
        // If we are shrinking the memory, fill the old stuff with DEAD_LAND_FILL
        fill_memory((char *) header + oldSize, DEAD_LAND_FILL, oldSize - newSize);
    }

    // Fill the no mans land fill and check the heap for corruption
    fill_memory((char *) p + newUserSize, NO_MANS_LAND_FILL, NO_MANS_LAND_SIZE);
#endif

    return p;
}

void general_free(void *ptr, u64 options) {
    if (!ptr) return;

    options |= Context.AllocOptions;

    auto *header = (allocation_header *) ptr - 1;

    auto alloc = header->Alloc;
    void *block = (char *) header - header->AlignmentPadding;

    s64 extra = header->Alignment + sizeof(allocation_header) + (sizeof(allocation_header) % header->Alignment);
    s64 size = header->Size + extra;

#if defined DEBUG_MEMORY
    DEBUG_memory_info::maybe_verify_heap();

    auto id = header->ID;

    size += NO_MANS_LAND_SIZE;

    DEBUG_memory_info::unlink_header(header);
    fill_memory(block, DEAD_LAND_FILL, size);
#endif

    alloc.Function(allocator_mode::FREE, alloc.Context, 0, block, size, &options);
}

void free_all(allocator alloc, u64 options) {
#if defined DEBUG_MEMORY
    // Remove allocations made with the allocator from the the linked list so we don't corrupt the heap
    WITH_ALLOC(DefaultAlloc) {
        array<allocation_header *> allocations;

        auto *h = DEBUG_memory_info::Head;
        while (h) {
            if (h->Alloc == alloc) append(allocations, h);
            h = h->DEBUG_Next;
        }
        For(allocations) DEBUG_memory_info::unlink_header(it);
    }
#endif

    options |= Context.AllocOptions;

    auto result = alloc.Function(allocator_mode::FREE_ALL, alloc.Context, 0, 0, 0, &options);
    assert((result != (void *) -1) && "Allocator doesn't support FREE_ALL");
}

LSTD_END_NAMESPACE

using LSTD_NAMESPACE::general_allocate;
using LSTD_NAMESPACE::general_free;
using LSTD_NAMESPACE::source_location;
using LSTD_NAMESPACE::u32;
#define C LSTD_NAMESPACE::Context

// #if defined LSTD_DONT_DEFINE_STD
// #define L
// #define loc LSTD_NAMESPACE::source_location()
// #else
// #define L , LSTD_NAMESPACE::source_location loc
// #endif

#define L , LSTD_NAMESPACE::source_location loc

[[nodiscard]] void *operator new(size_t size L) { return general_allocate(C.Alloc, size, 0, 0, loc); }
[[nodiscard]] void *operator new[](size_t size L) { return general_allocate(C.Alloc, size, 0, 0, loc); }

[[nodiscard]] void *operator new(size_t size, align_val_t alignment L) { return general_allocate(C.Alloc, size, (u32) alignment, 0, loc); }
[[nodiscard]] void *operator new[](size_t size, align_val_t alignment L) { return general_allocate(C.Alloc, size, (u32) alignment, 0, loc); }

[[nodiscard]] void *operator new(size_t size, const std::nothrow_t &tag L) noexcept { return general_allocate(C.Alloc, size, 0, 0, loc); }
[[nodiscard]] void *operator new[](size_t size, const std::nothrow_t &tag L) noexcept { return general_allocate(C.Alloc, size, 0, 0, loc); }

[[nodiscard]] void *operator new(size_t size, align_val_t alignment, const std::nothrow_t &tag L) noexcept { return general_allocate(C.Alloc, size, (u32) alignment, 0, loc); }
[[nodiscard]] void *operator new[](size_t size, align_val_t alignment, const std::nothrow_t &tag L) noexcept { return general_allocate(C.Alloc, size, (u32) alignment, 0, loc); }

// void *operator new(size_t size, source_location loc) { return general_allocate(C.Alloc, size, 0, 0, loc); }
// void *operator new[](size_t size, source_location loc) { return general_allocate(C.Alloc, size, 0, 0, loc); }
// void *operator new(size_t size, align_val_t alignment, source_location loc) { return general_allocate(C.Alloc, size, (u32) alignment, 0, loc); }
// void *operator new[](size_t size, align_val_t alignment, source_location loc) { return general_allocate(C.Alloc, size, (u32) alignment, 0, loc); }

void operator delete(void *ptr) noexcept { general_free(ptr); }
void operator delete[](void *ptr) noexcept { general_free(ptr); }

void operator delete(void *ptr, align_val_t alignment) noexcept { general_free(ptr); }
void operator delete[](void *ptr, align_val_t alignment) noexcept { general_free(ptr); }
