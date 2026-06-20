#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CFG_DIR  "sdmc:/3ds/wetter"
#define CFG_PATH CFG_DIR "/config.json"

static bool jsonNum(const char *from, const char *key, double *out) {
	const char *p = strstr(from, key);
	if (!p) return false;
	*out = strtod(p + strlen(key), NULL);
	return true;
}

static bool jsonStr(const char *from, const char *key, char *out, size_t cap) {
	const char *p = strstr(from, key);
	if (!p) return false;
	p += strlen(key);
	while (*p == ' ' || *p == '\"') p++;
	size_t i = 0;
	while (*p && *p != '\"' && i + 1 < cap) {
		if (*p == '\\' && p[1]) p++;
		out[i++] = *p++;
	}
	out[i] = '\0';
	return i > 0;
}

bool configLoad(City *arr, int maxN, int *count, int *active, bool *unitF, bool *windMs, Lang *lang) {
	*count = 0;
	*active = 0;
	*unitF = false;
	*windMs = false;
	// *lang intentionally NOT reset: keep the caller's auto-detected default
	// unless this file explicitly stores a language.

	FILE *f = fopen(CFG_PATH, "rb");
	if (!f) return false;
	char buf[4096];
	size_t n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[n] = '\0';

	double v;
	if (jsonNum(buf, "\"active\":", &v)) *active = (int)v;
	if (jsonNum(buf, "\"unitF\":", &v))  *unitF = (v != 0);
	if (jsonNum(buf, "\"windMs\":", &v)) *windMs = (v != 0);
	if (jsonNum(buf, "\"lang\":", &v))   *lang = (v != 0) ? LANG_DE : LANG_EN;

	const char *cities = strstr(buf, "\"cities\":");
	if (cities) {
		const char *p = strchr(cities, '[');
		if (p) {
			p++;
			while (*count < maxN) {
				const char *ob = strchr(p, '{');
				const char *eb = strchr(p, ']');
				if (!ob || (eb && ob > eb)) break;
				const char *oe = strchr(ob, '}');
				if (!oe) break;

				City c;
				memset(&c, 0, sizeof(c));
				double lat, lon;
				if (jsonStr(ob, "\"name\":", c.name, sizeof(c.name)) &&
				    jsonNum(ob, "\"lat\":", &lat) && jsonNum(ob, "\"lon\":", &lon)) {
					c.lat = lat;
					c.lon = lon;
					arr[(*count)++] = c;
				}
				p = oe + 1;
			}
		}
	} else {
		// Legacy single-city format.
		double lat, lon;
		if (jsonNum(buf, "\"lat\":", &lat) && jsonNum(buf, "\"lon\":", &lon)) {
			City c;
			memset(&c, 0, sizeof(c));
			if (!jsonStr(buf, "\"name\":", c.name, sizeof(c.name)))
				snprintf(c.name, sizeof(c.name), "%s", tr("Standort", "Location"));
			c.lat = lat;
			c.lon = lon;
			arr[(*count)++] = c;
		}
	}

	if (*count == 0) return false;
	if (*active < 0 || *active >= *count) *active = 0;
	return true;
}

void configSave(const City *arr, int count, int active, bool unitF, bool windMs, Lang lang) {
	mkdir("sdmc:/3ds", 0777);
	mkdir(CFG_DIR, 0777);

	FILE *f = fopen(CFG_PATH, "wb");
	if (!f) return;
	fprintf(f, "{\"active\":%d,\"unitF\":%d,\"windMs\":%d,\"lang\":%d,\"cities\":[",
		active, unitF ? 1 : 0, windMs ? 1 : 0, (lang == LANG_DE) ? 1 : 0);
	for (int i = 0; i < count; i++) {
		fprintf(f, "%s{\"name\":\"%s\",\"lat\":%.4f,\"lon\":%.4f}",
			i ? "," : "", arr[i].name, arr[i].lat, arr[i].lon);
	}
	fprintf(f, "]}\n");
	fclose(f);
}
