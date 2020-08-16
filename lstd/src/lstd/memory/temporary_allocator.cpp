#include "../internal/context.h"
#include "allocator.h"
#include "array.h"

LSTD_BEGIN_NAMESPACE

void *temporary_allocator(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *options) {
#if COMPILER == MSVC
#pragma warning(push)
#pragma warning(disable : 4146)
#endif

    auto *data = (temporary_allocator_data *) context;

    // The temporary allocator should have been initialized
    assert(data->Base.Storage && data->Base.Reserved);

    switch (mode) {
        case allocator_mode::ALLOCATE: {
            auto *p = &data->Base;

            while (p->Next) {
                if (p->Used + size < p->Reserved) break;
                p = p->Next;
            }

            if (p->Used + size >= p->Reserved) {
                assert(!p->Next);
                p->Next = allocate(temporary_allocator_data::page, Malloc);

                // Random log-based growth thing I came up at the time, not real science.
                s64 loggedSize = (s64) ceil(p->Reserved * (log2(p->Reserved * 10.0) / 3));
                s64 reserveTarget = (max<s64>(ceil_pow_of_2(size * 2), ceil_pow_of_2(loggedSize)) + 8_KiB - 1) & -8_KiB;

                p->Next->Storage = allocate_array(char, reserveTarget, Malloc);
                p->Next->Reserved = reserveTarget;
                p = p->Next;
            }

            void *result = (char *) p->Storage + p->Used;
            assert(result);

            p->Used += size;
            data->TotalUsed += size;

            // We mark all allocations done with the temporary allocator as "leaks", so they don't get reported when checking for actual leaks.
            // allocator::general_(re)allocate check for this flag after calling the allocation function, so this gets propagated upwards correctly.
            *options |= LEAK;

            return result;
        }
        case allocator_mode::RESIZE: {
            auto *p = &data->Base;
            while (p->Next) {
                if (p->Used + size < p->Reserved) break;
                p = p->Next;
            }

            s64 diff = size - oldSize;

            void *possiblyThisBlock = (char *) p->Storage + p->Used - oldSize;

            // We support resizing only on the last allocation (this still covers lots of cases,
            // e.g. constructing a string and then immediately appending to it!).
            if (possiblyThisBlock == oldMemory) {
                if (p->Used + diff >= p->Reserved) return null;  // Not enough space
                p->Used += diff;
                return oldMemory;
            }
            return null;
        }
        case allocator_mode::FREE:
            // We don't free individual allocations in the temporary allocator
            return null;
        case allocator_mode::FREE_ALL: {
#if defined DEBUG_MEMORY
            // Remove our allocations from the linked list so we don't corrupt the heap after freeing the pages
            WITH_ALLOC(Malloc) {
                array<allocation_header *> toUnlink;
                defer(toUnlink.release());

                auto *h = DEBUG_memory_info::Head;
                while (h) {
                    if (h->Alloc == allocator(temporary_allocator, data)) toUnlink.append(h);
                    h = h->DEBUG_Next;
                }
                For(toUnlink) DEBUG_memory_info::unlink_header(it);
            }
#endif

            s64 targetSize = data->Base.Reserved;

            // Check if any overflow pages were used
            auto *page = data->Base.Next;
            while (page) {
                targetSize += page->Reserved;

                auto *next = page->Next;
                free(page->Storage);
                free(page);
                page = next;
            }
            data->Base.Next = null;

            // Resize _Storage_ to fit all allocations which previously required overflow pages
            if (targetSize != data->Base.Reserved) {
                free(data->Base.Storage);

                data->Base.Storage = allocate_array(char, targetSize, Malloc);
                data->Base.Reserved = targetSize;
            }

            data->TotalUsed = data->Base.Used = 0;
            // null means successful FREE_ALL
            // (void *) -1 means failure

            return null;
        }
        default:
            assert(false);
    }

#if COMPILER == MSVC
#pragma warning(pop)
#endif

    return null;
}

void release_temporary_allocator() {
    if (!Context.TempAllocData.Base.Reserved) return;

    // Free any left-over overflow pages!
    free_all(Context.Temp);

    free(Context.TempAllocData.Base.Storage);
    Context.TempAllocData = {};
}

LSTD_END_NAMESPACE
