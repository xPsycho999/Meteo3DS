#include "bignum.h"
#include <citro2d.h>
#include <stdio.h>
#include <string.h>

// Segment bits.
enum { SA = 1, SB = 2, SC = 4, SD = 8, SE = 16, SF = 32, SG = 64 };

static const unsigned char DIGIT[10] = {
	/*0*/ SA|SB|SC|SD|SE|SF,
	/*1*/ SB|SC,
	/*2*/ SA|SB|SG|SE|SD,
	/*3*/ SA|SB|SG|SC|SD,
	/*4*/ SF|SG|SB|SC,
	/*5*/ SA|SF|SG|SC|SD,
	/*6*/ SA|SF|SG|SE|SC|SD,
	/*7*/ SA|SB|SC,
	/*8*/ SA|SB|SC|SD|SE|SF|SG,
	/*9*/ SA|SB|SC|SD|SF|SG,
};

// Horizontal segment between x0..x1 at vertical centre y, with rounded caps.
static void hseg(float x0, float x1, float y, float t, u32 col) {
	C2D_DrawRectSolid(x0, y - t / 2, 0.0f, x1 - x0, t, col);
	C2D_DrawCircleSolid(x0, y, 0.0f, t / 2, col);
	C2D_DrawCircleSolid(x1, y, 0.0f, t / 2, col);
}

// Vertical segment between y0..y1 at horizontal centre x, with rounded caps.
static void vseg(float x, float y0, float y1, float t, u32 col) {
	C2D_DrawRectSolid(x - t / 2, y0, 0.0f, t, y1 - y0, col);
	C2D_DrawCircleSolid(x, y0, 0.0f, t / 2, col);
	C2D_DrawCircleSolid(x, y1, 0.0f, t / 2, col);
}

static void drawDigit(float dx, float dy, float w, float h, float t, unsigned char mask, u32 col) {
	float left = dx, right = dx + w;
	float top = dy, mid = dy + h / 2, bot = dy + h;
	float pad = t * 0.7f;
	if (mask & SA) hseg(left + pad, right - pad, top, t, col);
	if (mask & SG) hseg(left + pad, right - pad, mid, t, col);
	if (mask & SD) hseg(left + pad, right - pad, bot, t, col);
	if (mask & SF) vseg(left,  top + pad, mid - pad, t, col);
	if (mask & SB) vseg(right, top + pad, mid - pad, t, col);
	if (mask & SE) vseg(left,  mid + pad, bot - pad, t, col);
	if (mask & SC) vseg(right, mid + pad, bot - pad, t, col);
}

float drawBigTemp(float x, float y, float height, u32 color, u32 hole, int value) {
	float h   = height;
	float w   = h * 0.55f;
	float t   = h * 0.15f;
	float gap = h * 0.22f;

	char buf[16];
	snprintf(buf, sizeof(buf), "%d", value);

	float cx = x;
	for (const char *p = buf; *p; p++) {
		if (*p == '-') {
			hseg(cx + t * 0.4f, cx + w - t * 0.4f, y + h / 2, t, color);
			cx += w * 0.7f + gap;
		} else if (*p >= '0' && *p <= '9') {
			drawDigit(cx, y, w, h, t, DIGIT[*p - '0'], color);
			cx += w + gap;
		}
	}

	// Degree: a clear thin ring just past the last digit, near the top.
	float rightEdge = cx - gap;        // cx overshot by one gap after the loop
	float r     = h * 0.13f;
	float ringT = r * 0.55f;           // ring thickness -> obvious hole
	float dxc   = rightEdge + r + h * 0.20f;
	float dyc   = y + r + t * 0.10f;
	C2D_DrawCircleSolid(dxc, dyc, 0.0f, r, color);
	C2D_DrawCircleSolid(dxc, dyc, 0.0f, r - ringT, hole);

	return (dxc + r) - x;
}
