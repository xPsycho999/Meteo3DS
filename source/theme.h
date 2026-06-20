#pragma once
#include <3ds.h>

// Colour palette derived from the current weather + time of day. Drives the
// top-screen background gradient and text colours.
typedef struct {
	u32 skyTop;        // background gradient (top edge)
	u32 skyBottom;     // background gradient (bottom edge)
	u32 textPrimary;   // headings, big temperature
	u32 textSecondary; // labels, secondary info
} Theme;

// Pick a theme for a WMO weather code and day/night flag.
Theme themeFor(int weather_code, int is_day);
