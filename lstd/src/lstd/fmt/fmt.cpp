#include "../fmt.h"

LSTD_BEGIN_NAMESPACE

void fmt_default_parse_error_handler(const string &message, const string &formatString, s64 position) {
    // An error during formatting occured.
    // If you are running a debugger it has now hit a breakpoint.

    // Make escape characters appear as they would in a string literal
    string str = formatString;
    replace_all(str, '\"', "\\\"");
    replace_all(str, '\\', "\\\\");
    replace_all(str, '\a', "\\a");
    replace_all(str, '\b', "\\b");
    replace_all(str, '\f', "\\f");
    replace_all(str, '\n', "\\n");
    replace_all(str, '\r', "\\r");
    replace_all(str, '\t', "\\t");
    replace_all(str, '\v', "\\v");

    string_writer output;
    fmt_to_writer(&output, "\n\n>>> {!GRAY}An error during formatting occured: {!YELLOW}{}{!GRAY}\n", message);
    fmt_to_writer(&output, "    ... the error happened here:\n");
    fmt_to_writer(&output, "        {!}{}{!GRAY}\n", str);
    fmt_to_writer(&output, "        {: >{}} {!} \n\n", "^", position + 1);
#if defined NDEBUG
    Context.PanicHandler(output.Builder.combine(), {});
#else
    print("{}", combine(output.Builder));

    // More info has been printed to the console but here's the error message:
    auto errorMessage = message;
    assert(false);
#endif
}

LSTD_END_NAMESPACE
