#include <3ds.h>
#include <citro2d.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "weather.h"
#include "geocode.h"
#include "config.h"
#include "theme.h"
#include "icons.h"
#include "bignum.h"
#include "http.h"
#include "i18n.h"

#define SCREEN_TOP_W   400.0f
#define SCREEN_BOT_W   320.0f
#define SCREEN_H       240.0f

#define MAX_CITIES     6
#define CITY_Y0        36
#define CITY_ROWH      24

typedef enum { TAB_TODAY, TAB_HOURLY, TAB_DAILY, TAB_AIR, TAB_CITIES, TAB_COUNT } Tab;

static City        g_cities[MAX_CITIES];
static int         g_nCities = 0;
static int         g_active = 0;
static char        g_status[64] = "";
static C2D_TextBuf g_textBuf;
static Tab         g_tab = TAB_TODAY;
static bool        g_unitF = false;        // false = °C, true = °F
static bool        g_windMs = false;       // false = km/h, true = m/s
static bool        g_settingsOpen = false;

// --- Background network worker -------------------------------------------------
typedef enum { JOB_NONE, JOB_FETCH, JOB_SEARCH, JOB_IPADD } JobType;

static volatile bool    g_running      = true;
static volatile JobType g_job          = JOB_NONE;
static volatile bool    g_resultReady  = false;
static volatile bool    g_searchOk     = false;
static volatile bool    g_searchFailed = false;
static volatile bool    g_inflight     = false;
static char             g_reqQuery[64];
static City             g_workCity;
static Forecast         g_workFc;

static void workerMain(void *arg) {
	(void)arg;
	while (g_running) {
		JobType job = g_job;
		if (job != JOB_NONE) {
			g_job = JOB_NONE;
			if (job == JOB_SEARCH) {
				City found;
				if (geoSearchCity(g_reqQuery, &found)) {
					g_workCity = found;
					g_searchOk = true;
				} else {
					g_searchFailed = true;
					continue;
				}
			} else if (job == JOB_IPADD) {
				City found;
				if (geoLocateByIP(&found)) {
					g_workCity = found;
					g_searchOk = true;
				} else {
					g_searchFailed = true;
					continue;
				}
			}
			Forecast fc;
			weatherFetch((City *)&g_workCity, &fc);
			if (fc.now.ok) airFetch((City *)&g_workCity, &fc.air);
			g_workFc = fc;
			g_resultReady = true;
		}
		svcSleepThread(15ULL * 1000 * 1000);
	}
}

// --- City store actions --------------------------------------------------------
static void fetchActive(void) {
	g_status[0] = '\0';
	g_workCity = g_cities[g_active];
	g_job = JOB_FETCH;
	g_inflight = true;
}

static void cityAdd(const City *c) {
	if (g_nCities >= MAX_CITIES) { snprintf(g_status, sizeof(g_status), "%s", tr("Liste voll", "List full")); return; }
	g_cities[g_nCities] = *c;
	g_active = g_nCities;
	g_nCities++;
	configSave(g_cities, g_nCities, g_active, g_unitF, g_windMs, g_lang);
	snprintf(g_status, sizeof(g_status), "%s", tr("Stadt gespeichert", "City saved"));
}

static void citySwitch(int idx) {
	if (idx < 0 || idx >= g_nCities || idx == g_active) return;
	g_active = idx;
	configSave(g_cities, g_nCities, g_active, g_unitF, g_windMs, g_lang);
	fetchActive();
}

static void cityDelete(int idx) {
	if (idx < 0 || idx >= g_nCities || g_nCities <= 1) return;
	for (int i = idx; i < g_nCities - 1; i++) g_cities[i] = g_cities[i + 1];
	g_nCities--;
	if (g_active >= g_nCities) g_active = g_nCities - 1;
	else if (g_active > idx)   g_active--;
	configSave(g_cities, g_nCities, g_active, g_unitF, g_windMs, g_lang);
	fetchActive();
}

// --- Drawing helpers -----------------------------------------------------------
static void drawText(float x, float y, float scale, u32 color, const char *fmt, ...) {
	char tmp[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, ap);
	va_end(ap);
	C2D_Text text;
	C2D_TextParse(&text, g_textBuf, tmp);
	C2D_TextOptimize(&text);
	C2D_DrawText(&text, C2D_WithColor, x, y, 0.0f, scale, scale, color);
}

static void drawGradient(float w, float h, u32 top, u32 bottom) {
	C2D_DrawRectangle(0, 0, 0.0f, w, h, top, top, bottom, bottom);
}

// Unit-aware display helpers (data is stored metric; converted at draw time).
static int tDisp(double c)        { return (int)lround(g_unitF ? c * 9.0 / 5.0 + 32.0 : c); }
static const char *tU(void)       { return g_unitF ? "\xC2\xB0""F" : "\xC2\xB0""C"; }
static int wDisp(double kmh)      { return (int)lround(g_windMs ? kmh / 3.6 : kmh); }
static const char *wU(void)       { return g_windMs ? "m/s" : "km/h"; }

static u32 lerpColor(u32 a, u32 b, float t) {
	int ar = a & 0xFF, ag = (a >> 8) & 0xFF, ab = (a >> 16) & 0xFF;
	int br = b & 0xFF, bg = (b >> 8) & 0xFF, bb = (b >> 16) & 0xFF;
	return C2D_Color32(ar + (int)((br - ar) * t), ag + (int)((bg - ag) * t),
	                   ab + (int)((bb - ab) * t), 0xFF);
}

static void drawSpinner(float cx, float cy, float r, float anim, u32 color) {
	for (int i = 0; i < 8; i++) {
		float a = anim * 0.12f - i * (3.14159265f / 4.0f);
		float fade = 0.25f + 0.75f * (i / 7.0f);
		u32 c = C2D_Color32((color & 0xFF), (color >> 8) & 0xFF, (color >> 16) & 0xFF,
		                    (int)(0xFF * fade));
		C2D_DrawCircleSolid(cx + cosf(a) * r, cy + sinf(a) * r, 0.0f, r * 0.18f, c);
	}
}

static void fmtHM(long utc, long offset, char *out, size_t cap) {
	long l = utc + offset;
	long h = (l / 3600) % 24, m = (l / 60) % 60;
	if (h < 0) h += 24;
	if (m < 0) m += 60;
	snprintf(out, cap, "%02ld:%02ld", h, m);
}

static const char *weekday(long utc, long offset) {
	static const char *de[7] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };
	static const char *en[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	long days = (utc + offset) / 86400;
	int idx = (int)(((days + 4) % 7 + 7) % 7);
	return (g_lang == LANG_DE ? de : en)[idx];
}

// --- Top screen ----------------------------------------------------------------
static void renderTop(const CurrentWx *wx, float anim) {
	const char *cityName = g_cities[g_active].name;

	if (!wx->ok) {
		drawGradient(SCREEN_TOP_W, SCREEN_H,
			C2D_Color32(0x2b, 0x2f, 0x3a, 0xFF), C2D_Color32(0x40, 0x45, 0x52, 0xFF));
		drawText(16, 12, 0.8f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), "%s", cityName);
		drawText(16, 110, 0.7f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), "%s", tr("Fehler:", "Error:"));
		drawText(16, 134, 0.6f, C2D_Color32(0xCF, 0xD6, 0xE0, 0xFF),
			"%s", wx->err[0] ? wx->err : tr("Unbekannt", "Unknown"));
		drawText(16, 200, 0.5f, C2D_Color32(0xCF, 0xD6, 0xE0, 0xFF), "%s", tr("X = erneut versuchen", "X = retry"));
		return;
	}

	Theme th = themeFor(wx->weather_code, wx->is_day);
	drawGradient(SCREEN_TOP_W, SCREEN_H, th.skyTop, th.skyBottom);

	drawText(16, 12, 0.8f, th.textPrimary, "%s", cityName);
	if (g_nCities > 1)
		drawText(350, 14, 0.45f, th.textSecondary, "%d/%d", g_active + 1, g_nCities);
	iconDraw(wx->weather_code, wx->is_day, 312.0f, 92.0f, 52.0f, anim);

	u32 hole = lerpColor(th.skyTop, th.skyBottom, 0.29f);
	drawBigTemp(18, 52, 62.0f, th.textPrimary, hole, tDisp(wx->temp));

	drawText(20, 132, 0.8f, th.textPrimary, "%s", wmoText(wx->weather_code, wx->is_day));
	drawText(20, 164, 0.5f, th.textSecondary, tr("Gefühlt %d%s", "Feels like %d%s"), tDisp(wx->feels), tU());
	drawText(16, 194, 0.52f, th.textSecondary, tr("Luftf.: %d%%   Wind: %d %s %s", "Humidity: %d%%   Wind: %d %s %s"),
		(int)lround(wx->humidity), wDisp(wx->wind), wU(), windDir(wx->wind_dir));
	drawText(16, 216, 0.52f, th.textSecondary, tr("Regen: %.1f mm   UV: %d", "Rain: %.1f mm   UV: %d"), wx->precip, wx->uv);
}

// --- Bottom tabs ---------------------------------------------------------------
static void tabToday(const Forecast *fc) {
	const u32 white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	const u32 faint = C2D_Color32(0xB8, 0xC4, 0xD4, 0xFF);
	drawText(14, 12, 0.7f, white, "%s", tr("Heute", "Today"));
	if (fc->n_days < 1) { drawText(14, 70, 0.5f, faint, "%s", tr("Keine Daten", "No data")); return; }
	const DayWx *d = &fc->days[0];
	char a[8], b[8];
	fmtHM(d->sunrise, fc->utc_offset, a, sizeof(a));
	fmtHM(d->sunset,  fc->utc_offset, b, sizeof(b));
	drawText(14, 56,   0.55f, faint, "%s", tr("Höchst / Tiefst", "High / Low"));
	drawText(170, 56,  0.55f, white, "%d\xC2\xB0 / %d\xC2\xB0", tDisp(d->tmax), tDisp(d->tmin));
	drawText(14, 88,   0.55f, faint, "%s", tr("Regenwahrsch.", "Chance of rain"));
	drawText(170, 88,  0.55f, white, "%d%%", d->precip_prob);
	drawText(14, 120,  0.55f, faint, "%s", tr("Sonnenaufgang", "Sunrise"));
	drawText(170, 120, 0.55f, white, "%s", a);
	drawText(14, 152,  0.55f, faint, "%s", tr("Sonnenuntergang", "Sunset"));
	drawText(170, 152, 0.55f, white, "%s", b);
}

static void tabHourly(const Forecast *fc) {
	const u32 white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	const u32 faint = C2D_Color32(0xB8, 0xC4, 0xD4, 0xFF);
	const u32 rain  = C2D_Color32(0x57, 0xB2, 0xF2, 0xFF);
	const u32 bar   = C2D_Color32(0x33, 0x5d, 0x86, 0xFF);
	const u32 line  = C2D_Color32(0xFF, 0xC8, 0x4a, 0xFF);
	drawText(14, 10, 0.7f, white, "%s", tr("Stündlich", "Hourly"));

	int n = fc->n_hours; if (n > 8) n = 8;
	if (n <= 0) { drawText(14, 70, 0.5f, faint, "%s", tr("Keine Daten", "No data")); return; }

	float x0 = 26, colW = (SCREEN_BOT_W - 48) / (float)(n > 1 ? n - 1 : 1);
	float baseY = 178, graphTop = 74, graphH = 78;

	double tmn = 1e9, tmx = -1e9;
	for (int i = 0; i < n; i++) {
		if (fc->hours[i].temp < tmn) tmn = fc->hours[i].temp;
		if (fc->hours[i].temp > tmx) tmx = fc->hours[i].temp;
	}
	double range = tmx - tmn; if (range < 1.0) range = 1.0;

	for (int i = 0; i < n; i++) {
		const HourWx *h = &fc->hours[i];
		float cx = x0 + i * colW;
		float bh = (h->precip_prob / 100.0f) * 46.0f;
		if (bh > 1.0f) C2D_DrawRectSolid(cx - 7, baseY - bh, 0.0f, 14, bh, bar);
		if (h->precip_prob >= 20)
			drawText(cx - 11, baseY - bh - 14, 0.36f, rain, "%d%%", h->precip_prob);
		char hm[8];
		fmtHM(h->t, fc->utc_offset, hm, sizeof(hm));
		drawText(cx - 9, baseY + 6, 0.40f, faint, "%.2s", hm);
	}
	C2D_DrawLine(14, baseY, faint, SCREEN_BOT_W - 14, baseY, faint, 1.0f, 0.0f);

	float px = 0, py = 0;
	for (int i = 0; i < n; i++) {
		float cx = x0 + i * colW;
		float ty = graphTop + (float)(1.0 - (fc->hours[i].temp - tmn) / range) * graphH;
		if (i > 0) C2D_DrawLine(px, py, line, cx, ty, line, 2.5f, 0.0f);
		px = cx; py = ty;
	}
	for (int i = 0; i < n; i++) {
		float cx = x0 + i * colW;
		float ty = graphTop + (float)(1.0 - (fc->hours[i].temp - tmn) / range) * graphH;
		C2D_DrawCircleSolid(cx, ty, 0.0f, 3.0f, line);
		drawText(cx - 10, ty - 19, 0.40f, white, "%d\xC2\xB0", tDisp(fc->hours[i].temp));
	}
}

static void tabDaily(const Forecast *fc) {
	const u32 white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	const u32 faint = C2D_Color32(0xB8, 0xC4, 0xD4, 0xFF);
	drawText(14, 10, 0.7f, white, "%s", tr("7 Tage", "7 Days"));
	int n = fc->n_days;
	if (n <= 0) { drawText(14, 70, 0.5f, faint, "%s", tr("Keine Daten", "No data")); return; }
	float y0 = 38, rowH = (196 - y0) / 7.0f;
	for (int i = 0; i < n; i++) {
		const DayWx *d = &fc->days[i];
		float y = y0 + i * rowH;
		drawText(14, y, 0.5f, white, "%s", (i == 0) ? tr("Heute", "Today") : weekday(d->day, fc->utc_offset));
		iconDraw(d->code, 1, 96, y + rowH * 0.4f, rowH * 0.42f, 0.0f);
		drawText(150, y, 0.5f, white, "%d\xC2\xB0", tDisp(d->tmax));
		drawText(190, y, 0.5f, faint, "%d\xC2\xB0", tDisp(d->tmin));
		drawText(250, y, 0.5f, faint, "%d%%", d->precip_prob);
	}
}

static u32 aqiColor(int aqi) {
	if (aqi < 0)    return C2D_Color32(0x8a, 0x97, 0xa8, 0xFF);
	if (aqi <= 20)  return C2D_Color32(0x4c, 0xc1, 0x6a, 0xFF);
	if (aqi <= 40)  return C2D_Color32(0xa8, 0xc8, 0x4c, 0xFF);
	if (aqi <= 60)  return C2D_Color32(0xe6, 0xc2, 0x3c, 0xFF);
	if (aqi <= 80)  return C2D_Color32(0xe6, 0x8a, 0x3c, 0xFF);
	if (aqi <= 100) return C2D_Color32(0xe0, 0x4c, 0x4c, 0xFF);
	return C2D_Color32(0xa0, 0x3c, 0x8c, 0xFF);
}

static void tabAir(const Forecast *fc) {
	const u32 white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	const u32 faint = C2D_Color32(0xB8, 0xC4, 0xD4, 0xFF);
	drawText(14, 10, 0.7f, white, "%s", tr("Luftqualität", "Air quality"));
	const AirWx *a = &fc->air;
	if (!a->ok) { drawText(14, 70, 0.5f, faint, "%s", tr("Keine Luftdaten", "No air data")); return; }

	C2D_DrawRectSolid(14, 40, 0.0f, 10, 22, aqiColor(a->eu_aqi));
	if (a->eu_aqi >= 0) drawText(32, 40, 0.6f, white, "AQI %d  %s", a->eu_aqi, aqiLabel(a->eu_aqi));
	else                drawText(32, 40, 0.6f, faint, "%s", tr("AQI k. A.", "AQI n/a"));

	drawText(14, 74,  0.48f, faint, "%s", tr("Feinstaub PM2.5", "Fine dust PM2.5"));
	drawText(190, 74, 0.48f, white, "%d µg/m³", (int)lround(a->pm25));
	drawText(14, 98,  0.48f, faint, "%s", tr("Feinstaub PM10", "Fine dust PM10"));
	drawText(190, 98, 0.48f, white, "%d µg/m³", (int)lround(a->pm10));

	drawText(14, 128, 0.5f, white, "%s", tr("Pollen", "Pollen"));
	drawText(14, 152,  0.46f, faint, "%s", tr("Gräser", "Grass"));
	drawText(150, 152, 0.46f, white, "%s", pollenLevel(a->grass));
	drawText(14, 172,  0.46f, faint, "%s", tr("Birke", "Birch"));
	drawText(150, 172, 0.46f, white, "%s", pollenLevel(a->birch));
	drawText(14, 192,  0.46f, faint, "%s", tr("Beifuß", "Mugwort"));
	drawText(150, 192, 0.46f, white, "%s", pollenLevel(a->mugwort));
}

static void tabCities(void) {
	const u32 white  = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	const u32 faint  = C2D_Color32(0xB8, 0xC4, 0xD4, 0xFF);
	const u32 accent = C2D_Color32(0x6f, 0xb0, 0xf0, 0xFF);
	const u32 del    = C2D_Color32(0xe0, 0x6c, 0x6c, 0xFF);
	drawText(14, 10, 0.7f, white, "%s", tr("Städte", "Cities"));

	for (int i = 0; i < g_nCities; i++) {
		float y = CITY_Y0 + i * CITY_ROWH;
		if (i == g_active)
			C2D_DrawRectSolid(8, y - 2, 0.0f, 304, CITY_ROWH - 2, C2D_Color32(0x2f, 0x80, 0xd6, 0x55));
		drawText(16, y, 0.5f, (i == g_active) ? white : faint, "%s", g_cities[i].name);
		if (g_nCities > 1) drawText(290, y, 0.5f, del, "X");
	}
	if (g_nCities < MAX_CITIES)
		drawText(16, CITY_Y0 + g_nCities * CITY_ROWH, 0.5f, accent, "%s", tr("+ Stadt hinzufügen", "+ Add city"));
	else
		drawText(16, CITY_Y0 + g_nCities * CITY_ROWH, 0.42f, faint, tr("Liste voll (max %d)", "List full (max %d)"), MAX_CITIES);
}

static void renderSettings(void) {
	const u32 white  = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	const u32 faint  = C2D_Color32(0xB8, 0xC4, 0xD4, 0xFF);
	const u32 accent = C2D_Color32(0x6f, 0xb0, 0xf0, 0xFF);
	const u32 dim    = C2D_Color32(0x8a, 0x97, 0xa8, 0xFF);
	const u32 pill   = C2D_Color32(0x2f, 0x80, 0xd6, 0xFF);

	drawText(14, 10, 0.7f, white, "%s", tr("Einstellungen", "Settings"));

	drawText(14, 44, 0.5f, faint, "%s", tr("Temperatur", "Temperature"));
	C2D_DrawRectSolid(210, 40, 0.0f, 60, 24, pill);
	drawText(224, 44, 0.5f, white, "%s", g_unitF ? "\xC2\xB0""F" : "\xC2\xB0""C");

	drawText(14, 80, 0.5f, faint, "%s", tr("Wind", "Wind"));
	C2D_DrawRectSolid(210, 76, 0.0f, 60, 24, pill);
	drawText(218, 80, 0.5f, white, "%s", g_windMs ? "m/s" : "km/h");

	drawText(14, 116, 0.5f, faint, "%s", tr("Sprache", "Language"));
	C2D_DrawRectSolid(210, 112, 0.0f, 60, 24, pill);
	drawText(228, 116, 0.5f, white, "%s", g_lang == LANG_DE ? "DE" : "EN");

	drawText(14, 152, 0.5f, accent, "%s", tr("Standort per IP hinzufügen", "Add location by IP"));

	drawText(14, 188, 0.40f, dim, "%s", tr("Tippen zum Ändern  ·  SELECT schließt", "Tap to change  ·  SELECT closes"));
}

static void renderBottom(const Forecast *fc, bool loading, float anim) {
	const u32 white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	const u32 dim   = C2D_Color32(0x8a, 0x97, 0xa8, 0xFF);

	if (g_settingsOpen) {
		renderSettings();
	} else if (g_tab == TAB_CITIES) {
		tabCities();
	} else if (fc->now.ok) {
		switch (g_tab) {
			case TAB_TODAY:  tabToday(fc);  break;
			case TAB_HOURLY: tabHourly(fc); break;
			case TAB_DAILY:  tabDaily(fc);  break;
			case TAB_AIR:    tabAir(fc);    break;
			default: break;
		}
	} else {
		drawText(14, 12, 0.7f, white, "Meteo3DS");
		if (g_status[0]) drawText(14, 70, 0.5f, dim, "%s", g_status);
	}

	if (loading) drawSpinner(300, 16, 12, anim, white);

	float by = 208, bh = 32, bw = SCREEN_BOT_W / (float)TAB_COUNT;
	const char *labels[TAB_COUNT] = {
		tr("Heute", "Today"), tr("Stündl.", "Hourly"), tr("7 Tage", "7 Days"),
		tr("Luft", "Air"), tr("Städte", "Cities")
	};
	for (int i = 0; i < TAB_COUNT; i++) {
		bool active = (g_tab == (Tab)i);
		u32 bg = active ? C2D_Color32(0x2f, 0x80, 0xd6, 0xFF) : C2D_Color32(0x26, 0x2f, 0x3d, 0xFF);
		C2D_DrawRectSolid(i * bw + 1, by, 0.0f, bw - 2, bh, bg);
		drawText(i * bw + 5, by + 9, 0.38f, active ? white : dim, "%s", labels[i]);
	}
}

static void drawSplash(C3D_RenderTarget *top, C3D_RenderTarget *bot, const char *msg) {
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TextBufClear(g_textBuf);
	C2D_TargetClear(top, C2D_Color32(0x14, 0x1e, 0x3c, 0xFF));
	C2D_SceneBegin(top);
	drawText(16, 100, 0.7f, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF), "%s", msg);
	C2D_TargetClear(bot, C2D_Color32(0x1c, 0x24, 0x33, 0xFF));
	C2D_SceneBegin(bot);
	C3D_FrameEnd(0);
}

static void resolveStartupCities(void) {
	if (configLoad(g_cities, MAX_CITIES, &g_nCities, &g_active, &g_unitF, &g_windMs, &g_lang) && g_nCities > 0) {
		snprintf(g_status, sizeof(g_status), "%s", tr("Gespeichert", "Loaded"));
		return;
	}
	if (geoLocateByIP(&g_cities[0])) {
		g_nCities = 1; g_active = 0;
		configSave(g_cities, g_nCities, g_active, g_unitF, g_windMs, g_lang);
		snprintf(g_status, sizeof(g_status), "%s", tr("Standort per IP", "Location via IP"));
		return;
	}
	snprintf(g_cities[0].name, sizeof(g_cities[0].name), "Berlin");
	g_cities[0].lat = 52.5200; g_cities[0].lon = 13.4050;
	g_nCities = 1; g_active = 0;
	snprintf(g_status, sizeof(g_status), "%s", tr("Standardstadt", "Default city"));
}

static bool askCity(char *out, size_t cap) {
	SwkbdState swkbd;
	out[0] = '\0';
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, cap - 1);
	swkbdSetHintText(&swkbd, tr("Stadt eingeben", "Enter city"));
	SwkbdButton btn = swkbdInputText(&swkbd, out, cap);
	return btn == SWKBD_BUTTON_RIGHT && out[0] != '\0';
}

static void dispatchSearch(const char *q) {
	strncpy(g_reqQuery, q, sizeof(g_reqQuery) - 1);
	g_reqQuery[sizeof(g_reqQuery) - 1] = '\0';
	g_searchOk = false;
	g_job = JOB_SEARCH;
	g_inflight = true;
	snprintf(g_status, sizeof(g_status), tr("Suche \"%.40s\"...", "Searching \"%.40s\"..."), q);
}

int main(int argc, char **argv) {
	gfxInitDefault();
	httpcInit(0);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	g_textBuf = C2D_TextBufNew(8192);

	i18nInit();   // pick DE/EN from the console language (config may override below)
	drawSplash(top, bot, tr("Standort wird ermittelt...", "Detecting location..."));
	resolveStartupCities();

	s32 prio = 0x30;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	Thread worker = threadCreate(workerMain, NULL, 32 * 1024, prio - 1, -1, false);

	Forecast fc;
	memset(&fc, 0, sizeof(fc));
	g_workCity = g_cities[g_active];
	g_job = JOB_FETCH;
	g_inflight = true;
	float anim = 0.0f;

	while (aptMainLoop()) {
		anim += 1.0f;
		hidScanInput();
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break;
		if (kDown & KEY_SELECT) g_settingsOpen = !g_settingsOpen;

		// Tab + city navigation (disabled while the settings overlay is open).
		if (!g_settingsOpen) {
			if (kDown & KEY_R) g_tab = (Tab)((g_tab + 1) % TAB_COUNT);
			if (kDown & KEY_L) g_tab = (Tab)((g_tab + TAB_COUNT - 1) % TAB_COUNT);
			if (!g_inflight && g_nCities > 1) {
				if (kDown & KEY_DRIGHT) citySwitch((g_active + 1) % g_nCities);
				if (kDown & KEY_DLEFT)  citySwitch((g_active + g_nCities - 1) % g_nCities);
			}
		}

		// Touch handling.
		if (kDown & KEY_TOUCH) {
			touchPosition tp;
			hidTouchRead(&tp);
			if (tp.py >= 206) {                       // tab bar
				g_tab = (Tab)(tp.px / (320 / TAB_COUNT));
				g_settingsOpen = false;
			} else if (g_settingsOpen) {              // settings rows
				if (tp.py >= 40 && tp.py < 68) {
					g_unitF = !g_unitF;
					configSave(g_cities, g_nCities, g_active, g_unitF, g_windMs, g_lang);
				} else if (tp.py >= 76 && tp.py < 104) {
					g_windMs = !g_windMs;
					configSave(g_cities, g_nCities, g_active, g_unitF, g_windMs, g_lang);
				} else if (tp.py >= 112 && tp.py < 140) {
					g_lang = (g_lang == LANG_DE) ? LANG_EN : LANG_DE;
					configSave(g_cities, g_nCities, g_active, g_unitF, g_windMs, g_lang);
				} else if (tp.py >= 148 && tp.py < 174 && !g_inflight && g_nCities < MAX_CITIES) {
					g_job = JOB_IPADD;
					g_inflight = true;
					snprintf(g_status, sizeof(g_status), "%s", tr("Standort wird ermittelt...", "Detecting location..."));
				}
			} else if (g_tab == TAB_CITIES && !g_inflight) {
				int rel = (int)tp.py - CITY_Y0;
				int idx = (rel >= 0) ? rel / CITY_ROWH : -1;
				if (idx >= 0 && idx < g_nCities) {
					if (tp.px >= 284 && g_nCities > 1) cityDelete(idx);
					else                                citySwitch(idx);
				} else if (idx == g_nCities && g_nCities < MAX_CITIES) {
					char q[64];
					if (askCity(q, sizeof(q))) dispatchSearch(q);
				}
			}
		}

		if (!g_inflight && !g_settingsOpen) {
			if (kDown & KEY_X) fetchActive();
			else if (kDown & KEY_Y) {
				char q[64];
				if (askCity(q, sizeof(q))) dispatchSearch(q);
			}
		}

		// Consume worker results.
		if (g_resultReady) {
			g_resultReady = false;
			fc = g_workFc;
			g_inflight = false;
			if (g_searchOk) {
				g_searchOk = false;
				cityAdd(&g_workCity);    // append + activate + persist
			} else {
				g_status[0] = '\0';
			}
		}
		if (g_searchFailed) {
			g_searchFailed = false;
			g_inflight = false;
			snprintf(g_status, sizeof(g_status), tr("\"%.40s\" nicht gefunden", "\"%.40s\" not found"), g_reqQuery);
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TextBufClear(g_textBuf);

		C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
		C2D_SceneBegin(top);
		renderTop(&fc.now, anim);

		C2D_TargetClear(bot, C2D_Color32(0x1c, 0x24, 0x33, 0xFF));
		C2D_SceneBegin(bot);
		renderBottom(&fc, g_inflight, anim);

		C3D_FrameEnd(0);
	}

	g_running = false;
	httpSetCancel(true);
	threadJoin(worker, U64_MAX);
	threadFree(worker);

	C2D_TextBufDelete(g_textBuf);
	C2D_Fini();
	C3D_Fini();
	httpcExit();
	gfxExit();
	return 0;
}
