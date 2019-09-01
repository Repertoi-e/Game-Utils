#pragma once

#include "../os.h"
#include "../storage/array.h"
#include "../storage/signal.h"
#include "../storage/string.h"

struct HMONITOR__;

LSTD_BEGIN_NAMESPACE

struct window;

struct display_mode {
    // Use this on RGB bits or refresh rate when setting the display mode for a monitor
    static constexpr s32 DONT_CARE = -1;

    s32 Width = 0, Height = 0;
    s32 RedBits = 0, GreenBits = 0, BlueBits = 0;
    s32 RefreshRate = 0;

    s32 compare(const display_mode &other) const {
        s32 bpp = RedBits + GreenBits + BlueBits;
        s32 otherBPP = other.RedBits + other.GreenBits + other.BlueBits;

        // First sort on color bits per pixel
        if (bpp != otherBPP) return (bpp > otherBPP) - (otherBPP > bpp);

        s32 area = Width * Height;
        s32 otherArea = other.Width * other.Height;

        // Then sort on screen area
        if (area != otherArea) return (area > otherArea) - (otherArea > area);

        return (RefreshRate > other.RefreshRate) - (other.RefreshRate - RefreshRate);
    }

    bool operator==(const display_mode &other) const { return compare(other) == 0; }
    bool operator!=(const display_mode &other) const { return !(*this == other); }
    bool operator>(const display_mode &other) const { return compare(other) == 1; }
    bool operator<(const display_mode &other) const { return compare(other) == -1; }
};

struct monitor {
    union platform_data {
        struct {
            HMONITOR__ *hMonitor = null;
            
			// 32 matches the static size of DISPLAY_DEVICE.DeviceName
            wchar_t AdapterName[32]{}, DisplayName[32]{};
            char PublicAdapterName[32]{}, PublicDisplayName[32]{};

            bool ModesPruned = false, ModeChanged = false;
        } Win32;
    } PlatformData {};

    string Name;

    // Physical dimensions in millimeters
    s32 WidthMM = 0, HeightMM = 0;

    // The window whose video mode is current on this monitor
    window *Window;

    array<display_mode> DisplayModes;
    display_mode CurrentMode;
};

struct monitor_event {
    enum action { CONNECTED, DISCONNECTED };

    monitor *Monitor = null;
    action Action;
};

// Connect a callback to this signal for monitor connect/disconnect events.
inline signal<void(const monitor_event &)> g_MonitorEvent;

// @TODO: Move these to _monitor_
// :
display_mode os_get_current_display_mode(monitor *mon);

// Work area is the screen excluding taskbar and other docked bars
rect os_get_work_area(monitor *mon);

bool os_set_display_mode(monitor *mon, display_mode desired);
void os_restore_display_mode(monitor *mon);

vec2i os_get_monitor_pos(monitor *mon);
vec2 os_get_monitor_content_scale(monitor *mon);
//:

monitor *os_monitor_from_window(window *win);

array<monitor *> os_get_monitors();
monitor *os_get_primary_monitor();

LSTD_END_NAMESPACE