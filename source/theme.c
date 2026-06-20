#include "theme.h"
#include <citro2d.h>

// Coarse weather "family" used for theming and icon selection.
typedef enum {
	FAM_CLEAR, FAM_PARTLY, FAM_CLOUD, FAM_FOG,
	FAM_RAIN, FAM_SNOW, FAM_THUNDER
} WxFamily;

static WxFamily familyOf(int code) {
	switch (code) {
		case 0:                          return FAM_CLEAR;
		case 1: case 2:                  return FAM_PARTLY;
		case 3:                          return FAM_CLOUD;
		case 45: case 48:                return FAM_FOG;
		case 51: case 53: case 55:
		case 56: case 57:
		case 61: case 63: case 65:
		case 66: case 67:
		case 80: case 81: case 82:       return FAM_RAIN;
		case 71: case 73: case 75:
		case 77: case 85: case 86:       return FAM_SNOW;
		case 95: case 96: case 99:       return FAM_THUNDER;
		default:                         return FAM_CLOUD;
	}
}

Theme themeFor(int code, int is_day) {
	Theme t;
	t.textPrimary   = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	t.textSecondary = C2D_Color32(0xD7, 0xE0, 0xEC, 0xFF);

	WxFamily fam = familyOf(code);

	if (!is_day) {
		// Night: deep blues, a touch darker for bad weather.
		switch (fam) {
			case FAM_CLEAR:
				t.skyTop = C2D_Color32(0x0b, 0x1b, 0x44, 0xFF);
				t.skyBottom = C2D_Color32(0x1d, 0x2e, 0x5e, 0xFF); break;
			case FAM_THUNDER:
				t.skyTop = C2D_Color32(0x16, 0x14, 0x28, 0xFF);
				t.skyBottom = C2D_Color32(0x2a, 0x27, 0x3f, 0xFF); break;
			case FAM_RAIN: case FAM_SNOW: case FAM_FOG: case FAM_CLOUD:
			default:
				t.skyTop = C2D_Color32(0x13, 0x1c, 0x33, 0xFF);
				t.skyBottom = C2D_Color32(0x26, 0x30, 0x49, 0xFF); break;
		}
		t.textSecondary = C2D_Color32(0xC2, 0xCC, 0xDC, 0xFF);
		return t;
	}

	// Day palettes.
	switch (fam) {
		case FAM_CLEAR:
			t.skyTop = C2D_Color32(0x2f, 0x80, 0xd6, 0xFF);
			t.skyBottom = C2D_Color32(0x8f, 0xc7, 0xf0, 0xFF); break;
		case FAM_PARTLY:
			t.skyTop = C2D_Color32(0x4c, 0x84, 0xbf, 0xFF);
			t.skyBottom = C2D_Color32(0x9a, 0xc0, 0xdc, 0xFF); break;
		case FAM_CLOUD:
			t.skyTop = C2D_Color32(0x6b, 0x79, 0x8c, 0xFF);
			t.skyBottom = C2D_Color32(0xa7, 0xb2, 0xc1, 0xFF); break;
		case FAM_FOG:
			t.skyTop = C2D_Color32(0x82, 0x8b, 0x96, 0xFF);
			t.skyBottom = C2D_Color32(0xbe, 0xc4, 0xcb, 0xFF); break;
		case FAM_RAIN:
			t.skyTop = C2D_Color32(0x44, 0x55, 0x6b, 0xFF);
			t.skyBottom = C2D_Color32(0x77, 0x88, 0x9e, 0xFF); break;
		case FAM_SNOW:
			t.skyTop = C2D_Color32(0x7d, 0x8f, 0xa6, 0xFF);
			t.skyBottom = C2D_Color32(0xc9, 0xd6, 0xe4, 0xFF); break;
		case FAM_THUNDER:
			t.skyTop = C2D_Color32(0x39, 0x36, 0x52, 0xFF);
			t.skyBottom = C2D_Color32(0x67, 0x63, 0x82, 0xFF); break;
	}
	return t;
}
