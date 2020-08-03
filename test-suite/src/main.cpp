#include "test.h"

void run_tests() {
    fmt::print("\n");
    for (auto [fileName, tests] : g_TestTable) {
        fmt::print("{}:\n", *fileName);

        u32 sucessfulProcs = 0;
        For(*tests) {
            auto length = min<s64>(30, it.Name.Length);
            fmt::print("        {:.{}} {:.^{}} ", it.Name, length, "", 35 - length);

            auto failedAssertsStart = asserts::GlobalFailed.Count;

            // Run the test
            if (it.Function) {
                it.Function();
            } else {
                fmt::print("{!RED}FAILED {!GRAY}(Function pointer is null){!}\n");
                continue;
            }

            // Check if test has failed asserts
            if (failedAssertsStart == asserts::GlobalFailed.Count) {
                fmt::print("{!GREEN}OK{!}\n");
                sucessfulProcs++;
            } else {
                fmt::print("{!RED}FAILED{!}\n");

                auto it = asserts::GlobalFailed.begin() + failedAssertsStart;
                for (; it != asserts::GlobalFailed.end(); ++it) {
                    fmt::print("          {!GRAY}>>> {}{!}\n", *it);
                }
                fmt::print("\n");
            }
        }

        f32 successRate = (f32) sucessfulProcs / (f32) tests->Count;
        fmt::print("{!GRAY}{:.2%} success ({} out of {} procs)\n{!}\n", successRate, sucessfulProcs, tests->Count);
    }
    fmt::print("\n\n");

    s64 calledCount = asserts::GlobalCalledCount;
    s64 failedCount = asserts::GlobalFailed.Count;
    s64 successCount = calledCount - failedCount;

    f32 successRate = calledCount ? (f32) successCount / (f32) calledCount : 0.0f;
    fmt::print("[Test Suite] {:.3%} success ({}/{} test asserts)\n", successRate, successCount, calledCount);

    if (failedCount) {
        fmt::print("[Test Suite] Failed asserts:\n");
        For(asserts::GlobalFailed) { fmt::print("    >>> {!RED}FAILED:{!GRAY} {}{!}\n", it); }
    }
    fmt::print("\n{!}");

    // These need to be reset in case we rerun the tests (we may spin this function up in a while loop a bunch of times when looking for rare bugs).
    asserts::GlobalCalledCount = 0;
    asserts::GlobalFailed.release();
}

io::string_builder_writer logger;

void write_output_to_file() {
    auto out = file::handle(file::path("output.txt"));
    out.write_to_file(logger.Builder.combine(), file::handle::Overwrite_Entire);
}

void test_exit() {
    fmt::print("Hello, world!\n");
}

s32 main() {
    Context.CheckForLeaksAtTermination = true;
    // Context.Log = &logger;

    run_at_exit(test_exit);
    os_exit(1);

    time_t start = os_get_time();

    // WITH_CONTEXT_VAR(Alloc, Context.TemporaryAlloc)
    {
        while (true) {
            run_tests();
            Context.TemporaryAlloc.free_all();
            break;
        }
    }

    fmt::print("\nFinished tests, time taken: {:f} seconds\n\n", os_time_to_seconds(os_get_time() - start));
    run_at_exit(write_output_to_file);

#if defined DEBUG_MEMORY
    // These get reported as leaks otherwise and we were looking for actual problems...
    for (auto [k, v] : g_TestTable) {
        v->release();
    }
    g_TestTable.release();
    Context.release_temporary_allocator();
#endif

    return 0;
}