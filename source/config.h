#pragma once
#include "weather.h"
#include "i18n.h"
#include <stdbool.h>

// Persisted settings live at sdmc:/3ds/Meteo3DS/config.json:
//   {"active":0,"cities":[{"name":"...","lat":..,"lon":..}, ...]}
// Old single-city files {"name","lat","lon"} are still read (migrated on save).

// Load the saved city list into arr (up to maxN). Sets *count, *active and the
// unit prefs (*unitF = use °F, *windMs = wind in m/s). *lang is only overwritten
// if the file stores one — otherwise the caller's auto-detected value is kept.
// Returns false if no usable config exists (caller falls back to geolocation).
bool configLoad(City *arr, int maxN, int *count, int *active, bool *unitF, bool *windMs, Lang *lang);

// Save the city list + active index + unit/language prefs. Best-effort (creates dir).
void configSave(const City *arr, int count, int active, bool unitF, bool windMs, Lang lang);
