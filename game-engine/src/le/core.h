#pragma once

#include <lstd/context.h>

// LE_GAME_API is used to export functions from the game dll
#if OS == WINDOWS
#if defined LE_BUILDING_GAME
#define LE_GAME_API __declspec(dllexport)
#else
#define LE_GAME_API __declspec(dllimport)
#endif
#else
#if COMPILER == GCC || COMPILER == CLANG
#if defined LE_BUILD_DLL
#define LE_GAME_API __attribute__((visibility("default")))
#else
#define LE_GAME_API
#endif
#else
#define LE_GAME_API
#pragma warning Unknown dynamic link import / export semantics.
#endif
#endif

namespace le {
// Used for keyboard/mouse input
enum : u32 {
    Modifier_Shift = BIT(0),
    Modifier_Control = BIT(1),
    Modifier_Alt = BIT(2),
    Modifier_Super = BIT(3),
};
}  // namespace le

// 'x' needs to have dll-interface to be used by clients of struct 'y'
// This will never be a problem since nowhere do we change struct sizes based on debug/release/whatever conditions
#if COMPILER == MSVC
#pragma warning(disable : 4251)
#endif