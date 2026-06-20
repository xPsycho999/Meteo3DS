#pragma once
#include <3ds.h>
#include <stdbool.h>

// Current weather conditions (Open-Meteo "current" block).
typedef struct {
	double temp;        // temperature_2m (°C)
	double feels;       // apparent_temperature (°C)
	double humidity;    // relative_humidity_2m (%)
	double precip;      // precipitation (mm)
	double wind;        // wind_speed_10m (km/h)
	int    wind_dir;    // wind_direction_10m (degrees)
	int    uv;          // uv_index (rounded)
	int    weather_code;// WMO weather code
	int    is_day;      // 1 = day, 0 = night
	long   time;        // unixtime (UTC) of the observation

	bool   ok;
	char   err[96];
} CurrentWx;

// One hourly slot.
typedef struct {
	long   t;           // unixtime (UTC)
	double temp;        // °C
	int    precip_prob; // %
	int    code;        // WMO
} HourWx;

// One daily slot.
typedef struct {
	long   day;         // unixtime (UTC) of local midnight
	int    code;        // WMO
	double tmax, tmin;  // °C
	int    precip_prob; // % (daily max)
	long   sunrise, sunset; // unixtime (UTC)
} DayWx;

// A place to fetch weather for.
typedef struct {
	char   name[64];
	double lat;
	double lon;
} City;

// Air quality + pollen (Open-Meteo air-quality API). Pollen values are -1 when
// unavailable (out of season / not reported).
typedef struct {
	int    eu_aqi;     // European AQI (-1 if unknown)
	double pm25, pm10; // µg/m³
	double grass, birch, alder, mugwort; // pollen grains/m³ (-1 = n/a)
	bool   ok;
} AirWx;

// Full forecast for one location: current + next hours + 7 days + air quality.
typedef struct {
	CurrentWx now;
	long      utc_offset;     // seconds to add to a UTC unixtime for local time
	HourWx    hours[24];
	int       n_hours;
	DayWx     days[7];
	int       n_days;
	AirWx     air;
} Forecast;

// Fetch current + hourly + daily for `city` in one request. Always fills out
// (out->now.ok = false + out->now.err on failure). Returns out->now.ok.
bool weatherFetch(const City *city, Forecast *out);

// Fetch air quality + pollen into `out` (sets out->ok). Best-effort; a failure
// here should not block showing the weather. Returns out->ok.
bool airFetch(const City *city, AirWx *out);

// EU-AQI -> qualitative label in the active language. Never NULL.
const char *aqiLabel(int aqi);

// Pollen concentration (grains/m³) -> level ("none"/"low"/...) in the active language.
const char *pollenLevel(double grains);

// WMO weather code -> short description (day/night aware) in the active language.
const char *wmoText(int code, int is_day);

// 8-point compass abbreviation for a wind direction in degrees (active language).
const char *windDir(int degrees);
