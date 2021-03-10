#pragma once

/// Defines the structure of allocators in this library.
/// Provides a default thread-safe global allocator and thread local temporary allocator.

#include "../internal/common.h"
#include "../thread.h"

LSTD_BEGIN_NAMESPACE

// By default we do some extra work when allocating to make it easier to catch memory related bugs.
// That work is measureable in performance so we don't want to do it in Dist configuration.
// If you want to disable it for debug as well, define FORCE_NO_DEBUG_MEMORY.
// You can read the comments in this file (around where DEBUG_MEMORY is mentioned)
// and see what extra safety stuff we do.

#if not defined NDEBUG
#if !defined DEBUG_MEMORY && !defined FORCE_NO_DEBUG_MEMORY
#define DEBUG_MEMORY 1
#endif
#else
// Don't enable extra info when in Release configuration unless predefined
// Note: "#define DEBUG_MEMORY 0" is WRONG. We check with "#if defined DEBUG_MEMORY",
// so that means in order to disable DEBUG_MEMORY you mustn't set DEBUG_MEMORY to anything at all.
#endif

//
// Allocators:
//

// Maximum size of an allocation we will attemp to request
#define MAX_ALLOCATION_REQUEST 0xFFFFFFFFFFFFFFE0  // Around 16384 PiB

enum class allocator_mode { ALLOCATE = 0,
                            RESIZE,
                            FREE,
                            FREE_ALL };

// This is an option when allocating.
// Allocations marked explicitly as leaks don't get reported with DEBUG_memory_info::report_leaks().
// This is handled internally when passed, so allocator implementations needn't pay attention to it.
constexpr u64 LEAK = 1ull << 62;

// This specifies what the signature of each allocation function should look like.
//
// _mode_ is what we are doing currently: allocating, resizing, freeing a block or freeing everything
//      (*implementing FREE_ALL is NOT a requirement, depends on the use case of the user code)
// _context_ is used as a pointer to any data the allocator needs (state)
// _size_ is the size of the allocation
// _oldMemory_ is used when resizing or freeing a block
// _oldSize_ is the old size of memory block, used only when resizing
//
// the last pointer to u64 is reserved for options. It is a pointer to allow the allocator function
// to modify it and propagate it to the library implementation of allocate/reallocate/free.
//
// One example use is for our debug check for leaks when the library terminates.
// The temporary allocator doesn't allow freeing so there is no reason for the library to report unfreed blocks from it.
// So the temporary allocator modifies the options to include LEAK (that flag is defined just above this comment).
//
//
// !!! When called with FREE_ALL, a return value of null means success!
//     To signify that the allocator doesn't support FREE_ALL (or the operation failed) return: (void*) -1.
//
// !!! When called with RESIZE, this doesn't mean "reallocate"!
//     Only valid return here is _oldMemory_ (memory was grown/shrank in place)
//     or null - memory can't be resized and needs to be moved.
//     In the second case we allocate a new block and copy the old data there (in general_reallocate).
//
// !!! Alignment is handled internally. Allocator implementations needn't pay attention to it.
//     When an aligned allocation is being made, we send a request at least _alignment_ bytes larger,
//     so when the allocator function returns an unaligned pointer we can freely bump it.
//     The information about the alignment is saved in the header, that's how we know what
//     the old pointer was when freeing or reallocating.
//
// We do this so custom allocator implementations don't require boiler plate code.
// For examples see how we implement default allocator, temporary allocator, linked list allocator, etc.
using allocator_func_t = void *(*) (allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *);

struct allocator {
    allocator_func_t Function = null;
    void *Context = null;

    allocator(allocator_func_t function = null, void *context = null) : Function(function), Context(context) {}

    bool operator==(allocator other) const { return Function == other.Function && Context == other.Context; }
    bool operator!=(allocator other) const { return Function != other.Function || Context != other.Context; }

    operator bool() const { return Function; }
};

#if defined DEBUG_MEMORY
//
// In DEBUG we put extra stuff to make bugs more obvious. These constants are like the ones MSVC's debug CRT model uses.
// Like them, we use specific values for bytes outside the allocated range, for freed memory and for uninitialized memory.
//
// The values of the constants and the following comment is taken from MSVC:
//
// The following values are non-zero, constant, odd, large, and atypical.
// - Non-zero values help find bugs that assume zero-filled data
// - Constant values are good so that memory filling is deterministic (to help
//   make bugs reproducible). Of course, it is bad if the constant filling of
//   weird values masks a bug.
// - Mathematically odd numbers are good for finding bugs assuming a cleared
//   lower bit (e.g. properly aligned pointers to types other than char are not odd).
// - Large byte values are less typical and are useful for finding bad addresses.
// - Atypical values are good because they typically cause early detection in code.
// - For the case of the no-man's land and free blocks, if you store to any of
//   these locations, the memory integrity checker will detect it.
//

inline constexpr s64 NO_MANS_LAND_SIZE = 4;

// _NO_MANS_LAND_SIZE_ (4) extra bytes with this value before and after the allocation block
// which help detect reading out of range errors
inline constexpr u8 NO_MANS_LAND_FILL = 0xFD;

// When freeing we fill the block with this value (detects bugs when accessing memory that's freed)
inline constexpr u8 DEAD_LAND_FILL = 0xDD;

// When allocating a new block and DO_INIT_0 was not specified we fill it with this value
// (detects bugs when accessing memory before initializing it)
inline constexpr u8 CLEAN_LAND_FILL = 0xCD;
#endif

struct allocation_header {
#if defined DEBUG_MEMORY
    allocation_header *DEBUG_Next, *DEBUG_Previous;

    // Useful for debugging (you can set a breakpoint with the ID in general_allocate() in allocator.cpp).
    // Every allocation has an unique ID equal to the ID of the previous allocation + 1.
    // This is useful for debugging bugs related to allocations because (assuming your program isn't multithreaded)
    // each time you run your program the ID of each allocation is easily reproducible (assuming no randomness from the user side).
    s64 ID;

    // This ID is used to keep track of how many times this block has been reallocated.
    // When reallocate_array() is called we check if the block can be directly resized in place (using
    // allocation_mode::RESIZE). If not, we allocate a new block and transfer all the information to it there. In both
    // cases the ID above stays the same and this local ID is incremented. This starts at 0.
    s64 RID;

    // We mark the source of the allocation if such information was provided.
    // On reallocation we overwrite these with the source provided there.
    const utf8 *FileName;
    s64 FileLine;
#endif

    // The allocator used when allocating the memory. We read this when resizing/freeing.
    allocator Alloc;

    // The size of the allocation (NOT including the size of the header and padding)
    s64 Size;

#if defined DEBUG_MEMORY
    // This is another guard to check that the header is valid.
    // This points to (allocation_header *) header + 1 (the pointer we return after the allocation).
    void *DEBUG_Pointer;
#endif

    // The padding (in bytes) which was added to the pointer after allocating to make sure the resulting pointer is
    // aligned. The structure of an allocation is basically this:
    //
    // User requests allocation of _size_. The underlying allocator is called with
    //     _size_ + sizeof(allocation_header) + (sizeof(allocation_header) % alignment)
    //
    // The result:
    //   ...[..Alignment padding..][............Header............]............
    //      ^ The pointer returned by the allocator                ^ The resulting pointer (aligned)
    //
    u16 Alignment;         // We allow a maximum of 65535 bit (8191 byte) alignment
    u16 AlignmentPadding;  // Offset from the block that needs to be there in order for the result to be aligned

#if defined DEBUG_MEMORY
    // When allocating we can mark the next allocation as a leak.
    // That means that it's irrelevant if we don't free it before the end of the program (since the OS claims back the memory anyway).
    // When the Context has CheckForLeaksAtTermination set to true we log a list of unfreed allocations. Headers with this marked get skipped.
    bool MarkedAsLeak;

    // There may be padding after this (because we have modified this struct before but forgot)
    // but it's ok, we just need at least 4 bytes free. We always set the last 4 bytes of the header.
    char DEBUG_NoMansLand[NO_MANS_LAND_SIZE];
#endif

    // This header is followed by:
    // char Data[Size];
    // char NoMansLand[NO_MANS_LAND_SIZE]; (only in DEBUG)
};

// Calculates the required padding in bytes which needs to be added to _ptr_ in order to be aligned
u16 calculate_padding_for_pointer(void *ptr, s32 alignment);

// Like calculate_padding_for_pointer but padding must be at least the header size
u16 calculate_padding_for_pointer_with_header(void *ptr, s32 alignment, u32 headerSize);

// #if'd so programs don't compile when debug info shouldn't be used.
#if defined DEBUG_MEMORY
struct DEBUG_memory_info {
    inline static s64 AllocationCount = 0;

    // We keep a linked list of all allocations. You can use this list to visualize them.
    // _Head_ is the last allocation done.
    inline static allocation_header *Head = null;

    // Currently this mutex should be released in the OS implementations (e.g. windows_common.cpp).
    // We need to lock it before modifying the linked list for example.
    //
    // @TODO: @Speed: Lock-free linked list!
    inline static thread::mutex Mutex;

    // After every allocation we check the heap for corruption.
    // The problem is that this involves iterating over a (possibly) large linked list of every allocation made.
    // We use the frequency variable below to specify how often we perform that expensive operation.
    // By default we check the heap every 255 allocations, but if a problem is found you may want to decrease
    // this to 1 so you catch the corruption at just the right time.
    inline static u8 MemoryVerifyHeapFrequency = 255;

    // Set this to true to print a list of unfreed memory blocks when the library uninitializes.
    // Yes, the OS claims back all the memory the program has allocated anyway, and we are not promoting C++ style RAII
    // which make EVEN program termination slow, we are just providing this information to the programmer because they might
    // want to debug crashes/bugs related to memory. (I had to debug a bug with loading/unloading DLLs during runtime).
    inline static bool CheckForLeaksAtTermination = false;

    static void unlink_header(allocation_header *header);                                 // Removes a header from the list
    static void add_header(allocation_header *header);                                    // This adds the header to the front - making it the new head
    static void swap_header(allocation_header *oldHeader, allocation_header *newHeader);  // Replaces _oldHeader_ with _newHeader_ in the list

    // Assuming that the heap is not corrupted, this reports any unfreed allocations.
    // Yes, the OS claims back all the memory the program has allocated anyway, and we are not promoting C++ style RAII
    // which make EVEN program termination slow, we are just providing this information to the programmer because they might
    // want to debug crashes/bugs related to memory. (I had to debug a bug with loading/unloading DLLs during runtime).
    static void report_leaks();

    // Verifies the integrity of headers in all allocations (only if DEBUG_MEMORY is on).
    //
    // We call this function when a new allocation is made.
    // The problem is that it involves iterating over a linked list of every allocation made.
    // We use the frequency variable in the _Context_ to specify how often we perform that expensive operation.
    // By default we check the heap every 255 allocations, but if a problem is found you may want to decrease it to 1 so
    // your program runs way slower but you catch the corruption at just the right time.
    static void maybe_verify_heap();

    // Verifies the integrity of a single header (only if DEBUG_MEMORY is on).
    static void verify_header(allocation_header *header);
};
#endif

void *general_allocate(allocator alloc, s64 userSize, u32 alignment, u64 options = 0, source_location loc = {});

// Each pointer has a header which has information about the allocator it was allocated with.
void *general_reallocate(void *ptr, s64 newSize, u64 options = 0, source_location loc = {});

// Each pointer has a header which has information about the allocator it was allocated with.
// Calling free on a null pointer doesn't do anything.
//
// @TODO: source_location? Idea for the future: Turn on a switch to remember every freed allocation and have a way to
// display all that information in a visual way. This will help the programmer see what the program is doing with memory exactly.
void general_free(void *ptr, u64 options = 0);

// Note: Not all allocators must support this.
void free_all(allocator alloc, u64 options = 0);

//
// Default allocators:
//

// General purpose allocator.
void *default_allocator(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *);
inline allocator DefaultAlloc = {default_allocator, null};

//
// Temporary allocator:
//

struct temporary_allocator_data {
    struct page {
        byte *Storage = null;
        s64 Allocated = 0;
        s64 Used = 0;

        page *Next = null;
    };

    page Base;
    s64 TotalUsed = 0;
};

// :TemporaryAllocator:
// This allocator works like an arena allocator.
// It's super fast because it basically bumps a pointer.
// It can be used globally to allocate memory that is not meant to last long
// (e.g. return value of a function that converts utf8 to utf16 to pass to a windows call)
//
// With this allocator you don't free individual allocations, but instead free the entire thing (with FREE_ALL)
// when you are sure nobody uses memory the "temporary memory" anymore.
//
// It initializes itself the first time you allocate with it, the available space is always a multiple of 8 KiB.
// When we run out of space we allocate "overflow pages" and keep a list of them. The next time you FREE_ALL,
// these pages are merged and the default buffer is resized (new size is the combined size of all allocated buffers).
//
// Example use case: if you are programming a game and you need to calculate some stuff for a given frame,
//     using this allocator means having the freedom of dynamically allocating without performance implications.
//     At the end of the frame when the memory is no longer used you FREE_ALL and start the next frame.
void *temporary_allocator(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *options);

// Frees the memory held by the temporary allocator (if any).
// This is called automatically when closing a thread (created with our thread API).
//
// @Platform
// It's not called when that thread was created by a different API (although it still has a valid temporary allocator - see tls_init).
void release_temporary_allocator();

LSTD_END_NAMESPACE
