#include "weather.h"
#include "http.h"
#include "i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void buildUrl(const City *c, char *url, size_t cap) {
	snprintf(url, cap,
		"http://api.open-meteo.com/v1/forecast"
		"?latitude=%.4f&longitude=%.4f"
		"&current=temperature_2m,apparent_temperature,relative_humidity_2m,"
		"precipitation,weather_code,wind_speed_10m,wind_direction_10m,is_day,uv_index"
		"&hourly=temperature_2m,precipitation_probability,weather_code"
		"&daily=weather_code,temperature_2m_max,temperature_2m_min,"
		"precipitation_probability_max,sunrise,sunset"
		"&timezone=auto&timeformat=unixtime&forecast_days=7",
		c->lat, c->lon);
}

// Read the number after "key": (key includes quotes + colon), searching from `from`.
static bool getNum(const char *from, const char *key, double *out) {
	const char *p = strstr(from, key);
	if (!p) return false;
	*out = strtod(p + strlen(key), NULL);
	return true;
}

// Parse the numeric array after "key":[ ... ] into out[], up to maxN. Returns count.
static int getArray(const char *from, const char *key, double *out, int maxN) {
	const char *p = strstr(from, key);
	if (!p) return 0;
	p += strlen(key);
	while (*p && *p != '[') p++;
	if (*p != '[') return 0;
	p++;
	int n = 0;
	while (*p && *p != ']' && n < maxN) {
		while (*p == ' ' || *p == ',') p++;
		if (*p == ']') break;
		if (*p == 'n') {                 // "null"
			out[n++] = 0.0;
			while (*p && *p != ',' && *p != ']') p++;
			continue;
		}
		char *end;
		double v = strtod(p, &end);
		if (end == p) { p++; continue; }
		out[n++] = v;
		p = end;
	}
	return n;
}

bool weatherFetch(const City *city, Forecast *out) {
	memset(out, 0, sizeof(*out));

	char url[640];
	buildUrl(city, url, sizeof(url));

	char *body = NULL;
	u32 len = 0;
	Result ret = httpGet(url, &body, &len);
	if (ret != 0 || !body) {
		snprintf(out->now.err, sizeof(out->now.err),
			tr("Abruf fehlgeschlagen (0x%08lX)", "Fetch failed (0x%08lX)"), (unsigned long)ret);
		if (body) free(body);
		return false;
	}

	double v;
	if (getNum(body, "\"utc_offset_seconds\":", &v)) out->utc_offset = (long)v;

	// --- current ---
	const char *cur = strstr(body, "\"current\":");
	if (!cur) {
		snprintf(out->now.err, sizeof(out->now.err), "%s", tr("Antwort unlesbar", "Unreadable response"));
		free(body);
		return false;
	}
	CurrentWx *n = &out->now;
	if (getNum(cur, "\"time\":", &v))                 n->time = (long)v;
	if (getNum(cur, "\"temperature_2m\":", &v))       n->temp = v;
	if (getNum(cur, "\"apparent_temperature\":", &v)) n->feels = v;
	if (getNum(cur, "\"relative_humidity_2m\":", &v)) n->humidity = v;
	if (getNum(cur, "\"precipitation\":", &v))        n->precip = v;
	if (getNum(cur, "\"wind_speed_10m\":", &v))       n->wind = v;
	if (getNum(cur, "\"wind_direction_10m\":", &v))   n->wind_dir = (int)lround(v);
	if (getNum(cur, "\"uv_index\":", &v))             n->uv = (int)lround(v);
	if (getNum(cur, "\"weather_code\":", &v))         n->weather_code = (int)lround(v);
	if (getNum(cur, "\"is_day\":", &v))               n->is_day = (int)lround(v);

	// --- hourly (take 24 starting from the current hour) ---
	const char *hr = strstr(body, "\"hourly\":");
	if (hr) {
		static double t[168], tmp[168], pp[168], wc[168];
		int nt  = getArray(hr, "\"time\":", t, 168);
		int ntm = getArray(hr, "\"temperature_2m\":", tmp, 168);
		int npp = getArray(hr, "\"precipitation_probability\":", pp, 168);
		int nwc = getArray(hr, "\"weather_code\":", wc, 168);
		int cnt = nt;
		if (ntm < cnt) cnt = ntm;

		int start = 0;
		for (int i = 0; i < cnt; i++) {
			if ((long)t[i] >= n->time - 1800) { start = i; break; }
		}
		int k = 0;
		for (int i = start; i < cnt && k < 24; i++, k++) {
			out->hours[k].t           = (long)t[i];
			out->hours[k].temp        = tmp[i];
			out->hours[k].precip_prob = (i < npp) ? (int)lround(pp[i]) : 0;
			out->hours[k].code        = (i < nwc) ? (int)lround(wc[i]) : 0;
		}
		out->n_hours = k;
	}

	// --- daily (7 days) ---
	const char *dy = strstr(body, "\"daily\":");
	if (dy) {
		static double dt[7], dc[7], mx[7], mn[7], dpp[7], sr[7], ss[7];
		int nd = getArray(dy, "\"time\":", dt, 7);
		getArray(dy, "\"weather_code\":", dc, 7);
		getArray(dy, "\"temperature_2m_max\":", mx, 7);
		getArray(dy, "\"temperature_2m_min\":", mn, 7);
		getArray(dy, "\"precipitation_probability_max\":", dpp, 7);
		getArray(dy, "\"sunrise\":", sr, 7);
		getArray(dy, "\"sunset\":", ss, 7);
		for (int i = 0; i < nd && i < 7; i++) {
			out->days[i].day         = (long)dt[i];
			out->days[i].code        = (int)lround(dc[i]);
			out->days[i].tmax        = mx[i];
			out->days[i].tmin        = mn[i];
			out->days[i].precip_prob = (int)lround(dpp[i]);
			out->days[i].sunrise     = (long)sr[i];
			out->days[i].sunset      = (long)ss[i];
		}
		out->n_days = (nd < 7) ? nd : 7;
	}

	free(body);
	n->ok = true;
	n->err[0] = '\0';
	return true;
}

bool airFetch(const City *city, AirWx *out) {
	memset(out, 0, sizeof(*out));
	out->eu_aqi = -1;
	out->grass = out->birch = out->alder = out->mugwort = -1.0;

	char url[512];
	snprintf(url, sizeof(url),
		"http://air-quality-api.open-meteo.com/v1/air-quality"
		"?latitude=%.4f&longitude=%.4f"
		"&current=european_aqi,pm2_5,pm10,grass_pollen,birch_pollen,alder_pollen,mugwort_pollen"
		"&timezone=auto",
		city->lat, city->lon);

	char *body = NULL;
	u32 len = 0;
	if (httpGet(url, &body, &len) != 0 || !body) return false;

	const char *cur = strstr(body, "\"current\":");
	if (!cur) { free(body); return false; }

	double v;
	if (getNum(cur, "\"european_aqi\":", &v)) out->eu_aqi = (int)lround(v);
	if (getNum(cur, "\"pm2_5\":", &v))        out->pm25 = v;
	if (getNum(cur, "\"pm10\":", &v))         out->pm10 = v;
	if (getNum(cur, "\"grass_pollen\":", &v))   out->grass = v;
	if (getNum(cur, "\"birch_pollen\":", &v))   out->birch = v;
	if (getNum(cur, "\"alder_pollen\":", &v))   out->alder = v;
	if (getNum(cur, "\"mugwort_pollen\":", &v)) out->mugwort = v;

	free(body);
	out->ok = true;
	return true;
}

const char *aqiLabel(int aqi) {
	if (aqi < 0)   return tr("k. A.", "n/a");
	if (aqi <= 20) return tr("Gut", "Good");
	if (aqi <= 40) return tr("Annehmbar", "Fair");
	if (aqi <= 60) return tr("Mäßig", "Moderate");
	if (aqi <= 80) return tr("Schlecht", "Poor");
	if (aqi <= 100) return tr("Sehr schlecht", "Very poor");
	return tr("Extrem schlecht", "Extremely poor");
}

const char *pollenLevel(double grains) {
	if (grains < 0)   return tr("k. A.", "n/a");
	if (grains < 1)   return tr("keine", "none");
	if (grains < 20)  return tr("niedrig", "low");
	if (grains < 50)  return tr("mäßig", "moderate");
	if (grains < 100) return tr("hoch", "high");
	return tr("sehr hoch", "very high");
}

const char *wmoText(int code, int is_day) {
	switch (code) {
		case 0:  return is_day ? tr("Klar", "Clear") : tr("Klare Nacht", "Clear night");
		case 1:  return tr("Überwiegend klar", "Mainly clear");
		case 2:  return tr("Teilweise bewölkt", "Partly cloudy");
		case 3:  return tr("Bedeckt", "Overcast");
		case 45: return tr("Nebel", "Fog");
		case 48: return tr("Reifnebel", "Rime fog");
		case 51: return tr("Leichter Niesel", "Light drizzle");
		case 53: return tr("Niesel", "Drizzle");
		case 55: return tr("Starker Niesel", "Heavy drizzle");
		case 56: return tr("Gefrierender Niesel", "Freezing drizzle");
		case 57: return tr("Starker gefr. Niesel", "Heavy freezing drizzle");
		case 61: return tr("Leichter Regen", "Light rain");
		case 63: return tr("Regen", "Rain");
		case 65: return tr("Starker Regen", "Heavy rain");
		case 66: return tr("Gefrierender Regen", "Freezing rain");
		case 67: return tr("Starker gefr. Regen", "Heavy freezing rain");
		case 71: return tr("Leichter Schneefall", "Light snow");
		case 73: return tr("Schneefall", "Snow");
		case 75: return tr("Starker Schneefall", "Heavy snow");
		case 77: return tr("Schneegriesel", "Snow grains");
		case 80: return tr("Leichte Schauer", "Light showers");
		case 81: return tr("Schauer", "Showers");
		case 82: return tr("Heftige Schauer", "Violent showers");
		case 85: return tr("Leichte Schneeschauer", "Light snow showers");
		case 86: return tr("Schneeschauer", "Snow showers");
		case 95: return tr("Gewitter", "Thunderstorm");
		case 96: return tr("Gewitter mit Hagel", "Thunderstorm with hail");
		case 99: return tr("Schweres Gewitter", "Severe thunderstorm");
		default: return tr("Unbekannt", "Unknown");
	}
}

const char *windDir(int degrees) {
	static const char *de[8] = { "N", "NO", "O", "SO", "S", "SW", "W", "NW" };
	static const char *en[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
	int idx = (int)lround(degrees / 45.0) & 7;
	return (g_lang == LANG_DE ? de : en)[idx];
}
