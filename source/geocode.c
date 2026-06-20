#include "geocode.h"
#include "http.h"
#include "i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Read the number following "key": (key includes quotes + colon) into *out.
static bool jsonNum(const char *from, const char *key, double *out) {
	const char *p = strstr(from, key);
	if (!p) return false;
	*out = strtod(p + strlen(key), NULL);
	return true;
}

// Read the string following "key":"..." into out (NUL-terminated, capped).
static bool jsonStr(const char *from, const char *key, char *out, size_t cap) {
	const char *p = strstr(from, key);
	if (!p) return false;
	p += strlen(key);
	while (*p == ' ' || *p == '\"') p++;        // skip space + opening quote
	size_t i = 0;
	while (*p && *p != '\"' && i + 1 < cap) {
		if (*p == '\\' && p[1]) p++;            // pass through escaped char
		out[i++] = *p++;
	}
	out[i] = '\0';
	return i > 0;
}

// Percent-encode `in` into `out` for use in a URL query value.
static void urlEncode(const char *in, char *out, size_t cap) {
	static const char hex[] = "0123456789ABCDEF";
	size_t o = 0;
	for (const unsigned char *p = (const unsigned char *)in; *p && o + 4 < cap; p++) {
		unsigned char c = *p;
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
		    (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
			out[o++] = (char)c;
		} else {
			out[o++] = '%';
			out[o++] = hex[c >> 4];
			out[o++] = hex[c & 0xF];
		}
	}
	out[o] = '\0';
}

bool geoLocateByIP(City *out) {
	char *body = NULL;
	u32 len = 0;
	if (httpGet("http://ip-api.com/json/", &body, &len) != 0 || !body)
		return false;

	bool ok = false;
	if (strstr(body, "\"status\":\"success\"")) {
		double lat, lon;
		if (jsonNum(body, "\"lat\":", &lat) && jsonNum(body, "\"lon\":", &lon)) {
			out->lat = lat;
			out->lon = lon;
			if (!jsonStr(body, "\"city\":", out->name, sizeof(out->name)))
				snprintf(out->name, sizeof(out->name), "%s", tr("Mein Standort", "My location"));
			ok = true;
		}
	}
	free(body);
	return ok;
}

bool geoSearchCity(const char *query, City *out) {
	char enc[256];
	urlEncode(query, enc, sizeof(enc));

	char url[512];
	snprintf(url, sizeof(url),
		"http://geocoding-api.open-meteo.com/v1/search?name=%s&count=1&language=%s&format=json",
		enc, tr("de", "en"));

	char *body = NULL;
	u32 len = 0;
	if (httpGet(url, &body, &len) != 0 || !body)
		return false;

	bool ok = false;
	const char *res = strstr(body, "\"results\":");
	if (res) {
		double lat, lon;
		if (jsonNum(res, "\"latitude\":", &lat) && jsonNum(res, "\"longitude\":", &lon)) {
			out->lat = lat;
			out->lon = lon;
			if (!jsonStr(res, "\"name\":", out->name, sizeof(out->name)))
				snprintf(out->name, sizeof(out->name), "%s", query);
			ok = true;
		}
	}
	free(body);
	return ok;
}
