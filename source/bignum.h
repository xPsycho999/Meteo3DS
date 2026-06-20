#pragma once
#include <3ds.h>

// Draw a temperature value as crisp seven-segment-style vector digits, with a
// degree ring, starting at top-left (x, y). `height` is the digit height in
// pixels; width scales from it. `hole` is the colour used to punch the centre
// of the degree ring (use the background colour behind the number).
// Returns the total width drawn.
float drawBigTemp(float x, float y, float height, u32 color, u32 hole, int value);
