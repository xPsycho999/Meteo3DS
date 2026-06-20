#include "icons.h"
#include <citro2d.h>
#include <math.h>

#define PI 3.14159265f

// Natural icon colours (independent of the background theme for legibility).
#define COL_SUN     C2D_Color32(0xFF, 0xCB, 0x3a, 0xFF)
#define COL_MOON    C2D_Color32(0xEC, 0xF1, 0xFA, 0xFF)
#define COL_CRATER  C2D_Color32(0xCB, 0xD4, 0xE3, 0xFF)
#define COL_CLOUD   C2D_Color32(0xF4, 0xF6, 0xFA, 0xFF)
#define COL_CLOUD_D C2D_Color32(0xC9, 0xD1, 0xDD, 0xFF)
#define COL_SHADE   C2D_Color32(0xD7, 0xDE, 0xE8, 0xFF)
#define COL_RAIN    C2D_Color32(0x57, 0xB2, 0xF2, 0xFF)
#define COL_SNOW    C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define COL_BOLT    C2D_Color32(0xFF, 0xD2, 0x35, 0xFF)
#define COL_FOG     C2D_Color32(0xE3, 0xE8, 0xEF, 0xFF)

typedef enum {
	FAM_CLEAR, FAM_PARTLY, FAM_CLOUD, FAM_FOG, FAM_RAIN, FAM_SNOW, FAM_THUNDER
} WxFamily;

// Scale a colour's RGB by `f` (clamped), keeping it opaque.
static u32 scaleCol(u32 c, float f) {
	int r = (int)((c & 0xFF) * f), g = (int)(((c >> 8) & 0xFF) * f), b = (int)(((c >> 16) & 0xFF) * f);
	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	return C2D_Color32(r, g, b, 0xFF);
}

static WxFamily familyOf(int code) {
	switch (code) {
		case 0:                                                  return FAM_CLEAR;
		case 1: case 2:                                          return FAM_PARTLY;
		case 3:                                                  return FAM_CLOUD;
		case 45: case 48:                                        return FAM_FOG;
		case 51: case 53: case 55: case 56: case 57:
		case 61: case 63: case 65: case 66: case 67:
		case 80: case 81: case 82:                               return FAM_RAIN;
		case 71: case 73: case 75: case 77: case 85: case 86:    return FAM_SNOW;
		case 95: case 96: case 99:                               return FAM_THUNDER;
		default:                                                 return FAM_CLOUD;
	}
}

static bool isHeavy(int code) {
	return code == 65 || code == 67 || code == 82 || code == 75 ||
	       code == 86 || code == 99 || code == 95 || code == 96;
}

static void drawSun(float cx, float cy, float r, float anim) {
	float base = anim * 0.012f;
	float pulse = 1.0f + 0.06f * sinf(anim * 0.06f);   // gentle breathing
	for (int i = 0; i < 8; i++) {
		float a = base + i * (PI / 4.0f);
		float c = cosf(a), s = sinf(a);
		C2D_DrawLine(cx + c * r * 0.78f, cy + s * r * 0.78f, COL_SUN,
		             cx + c * r * (1.10f + 0.06f * pulse), cy + s * r * (1.10f + 0.06f * pulse),
		             COL_SUN, 3.0f, 0.0f);
	}
	C2D_DrawCircleSolid(cx, cy, 0.0f, r * 0.72f * pulse, C2D_Color32(0xFF, 0xCB, 0x3a, 0x3a)); // glow
	C2D_DrawCircleSolid(cx, cy, 0.0f, r * 0.60f * pulse, COL_SUN);
}

static void drawMoon(float cx, float cy, float r) {
	C2D_DrawCircleSolid(cx, cy, 0.0f, r * 0.62f, COL_MOON);
	C2D_DrawCircleSolid(cx - r * 0.18f, cy - r * 0.10f, 0.0f, r * 0.12f, COL_CRATER);
	C2D_DrawCircleSolid(cx + r * 0.20f, cy + r * 0.18f, 0.0f, r * 0.09f, COL_CRATER);
	C2D_DrawCircleSolid(cx + r * 0.05f, cy - r * 0.28f, 0.0f, r * 0.06f, COL_CRATER);
}

// Fluffy two-tone cloud centred at (cx, cy); `s` is the lump radius unit.
static void drawCloud(float cx, float cy, float s, u32 col) {
	u32 shade = scaleCol(col, 0.86f);
	u32 hi    = scaleCol(col, 1.06f);
	// Darker underside for depth (offset down).
	C2D_DrawRectSolid(cx - 1.55f * s, cy + 0.12f * s, 0.0f, 3.25f * s, 0.80f * s, shade);
	C2D_DrawCircleSolid(cx - 0.95f * s, cy + 0.18f * s, 0.0f, 0.70f * s, shade);
	C2D_DrawCircleSolid(cx + 0.10f * s, cy - 0.30f * s, 0.0f, 0.95f * s, shade);
	C2D_DrawCircleSolid(cx + 1.05f * s, cy + 0.18f * s, 0.0f, 0.72f * s, shade);
	// Main body.
	C2D_DrawRectSolid(cx - 1.55f * s, cy, 0.0f, 3.25f * s, 0.78f * s, col);
	C2D_DrawCircleSolid(cx - 0.95f * s, cy + 0.05f * s, 0.0f, 0.70f * s, col);
	C2D_DrawCircleSolid(cx + 0.10f * s, cy - 0.45f * s, 0.0f, 0.95f * s, col);
	C2D_DrawCircleSolid(cx + 1.05f * s, cy + 0.05f * s, 0.0f, 0.72f * s, col);
	// Soft highlight on the big lump.
	C2D_DrawCircleSolid(cx - 0.05f * s, cy - 0.60f * s, 0.0f, 0.40f * s, hi);
}

static void drawRain(float cx, float cy, float s, float anim, bool heavy) {
	float top = cy + 0.85f * s;
	int n = heavy ? 7 : 5;
	float span = (heavy ? 1.8f : 1.5f) * s;
	for (int i = 0; i < n; i++) {
		float x = cx - 1.0f * s + i * (2.0f * s / (n - 1));
		float speed = 0.045f + (i % 3) * 0.011f;       // varied fall speed
		float ph = fmodf(anim * speed + i * 0.27f, 1.0f);
		float y = top + ph * span;
		C2D_DrawLine(x, y, COL_RAIN, x - 0.12f * s, y + 0.42f * s, COL_RAIN, 2.2f, 0.0f);
	}
}

static void drawSnow(float cx, float cy, float s, float anim) {
	float top = cy + 0.85f * s;
	int n = 6;
	for (int i = 0; i < n; i++) {
		float speed = 0.022f + (i % 3) * 0.006f;
		float ph = fmodf(anim * speed + i * 0.21f, 1.0f);
		float sway = sinf(anim * 0.035f + i * 1.3f) * 0.22f * s;
		float x = cx - 1.0f * s + i * (2.0f * s / (n - 1)) + sway;
		float y = top + ph * 1.6f * s;
		float rad = (0.10f + (i % 2) * 0.05f) * s;
		C2D_DrawCircleSolid(x, y, 0.0f, rad, COL_SNOW);
	}
}

static void drawFog(float cx, float cy, float s, float anim) {
	for (int i = 0; i < 4; i++) {
		float y = cy - 0.3f * s + i * 0.45f * s;
		float sway = sinf(anim * 0.03f + i * 0.8f) * 0.25f * s;
		C2D_DrawLine(cx - 1.4f * s + sway, y, COL_FOG,
		             cx + 1.4f * s + sway, y, COL_FOG, 0.32f * s, 0.0f);
	}
}

static void drawBolt(float cx, float cy, float s) {
	float t = cy + 0.6f * s;
	C2D_DrawTriangle(cx - 0.15f * s, t,            COL_BOLT,
	                 cx + 0.25f * s, t,            COL_BOLT,
	                 cx - 0.10f * s, t + 0.7f * s, COL_BOLT, 0.0f);
	C2D_DrawTriangle(cx + 0.20f * s, t + 0.2f * s, COL_BOLT,
	                 cx - 0.25f * s, t + 1.3f * s, COL_BOLT,
	                 cx + 0.05f * s, t + 0.55f * s, COL_BOLT, 0.0f);
}

void iconDraw(int code, int is_day, float cx, float cy, float r, float anim) {
	WxFamily fam = familyOf(code);
	float s = r * 0.46f;                                  // cloud lump unit
	float drift = sinf(anim * 0.012f) * (r * 0.05f);     // gentle cloud sway
	float cd = cx + drift;                               // drifted x for clouds
	bool heavy = isHeavy(code);

	switch (fam) {
		case FAM_CLEAR:
			if (is_day) drawSun(cx, cy, r * 0.62f, anim);
			else        drawMoon(cx, cy, r);
			break;

		case FAM_PARTLY:
			if (is_day) drawSun(cx + 0.5f * r, cy - 0.45f * r, r * 0.42f, anim);
			else        drawMoon(cx + 0.5f * r, cy - 0.45f * r, r * 0.7f);
			drawCloud(cd - 0.15f * r, cy + 0.2f * r, s * 0.95f, COL_CLOUD);
			break;

		case FAM_CLOUD:
			drawCloud(cd, cy - 0.1f * r, s, COL_CLOUD_D);
			drawCloud(cd + 0.15f * r, cy + 0.05f * r, s * 0.85f, COL_CLOUD);
			break;

		case FAM_FOG:
			drawCloud(cd, cy - 0.35f * r, s * 0.9f, COL_CLOUD);
			drawFog(cx, cy + 0.45f * r, s, anim);
			break;

		case FAM_RAIN:
			drawCloud(cd, cy - 0.25f * r, s, COL_CLOUD_D);
			drawRain(cd, cy - 0.25f * r, s, anim, heavy);
			break;

		case FAM_SNOW:
			drawCloud(cd, cy - 0.25f * r, s, COL_CLOUD);
			drawSnow(cd, cy - 0.25f * r, s, anim);
			break;

		case FAM_THUNDER:
			drawCloud(cd, cy - 0.3f * r, s, COL_CLOUD_D);
			drawBolt(cd, cy - 0.3f * r, s);
			if (heavy) drawRain(cd + 0.6f * s, cy - 0.3f * r, s, anim, false);
			break;
	}
}
