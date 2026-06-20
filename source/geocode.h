#pragma once
#include "weather.h"
#include <stdbool.h>

// Approximate the current location from the console's public IP (ip-api.com,
// plain HTTP). Fills `out` (name + lat/lon) on success. Best-effort only —
// IP geolocation is city-level and can be wrong behind VPNs.
bool geoLocateByIP(City *out);

// Look up a place by name via Open-Meteo's geocoding API (German results).
// Takes the first match. Fills `out` on success, false if nothing found / error.
bool geoSearchCity(const char *query, City *out);
