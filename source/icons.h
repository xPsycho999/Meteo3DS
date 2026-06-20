#pragma once

// Draw an animated vector weather icon centred at (cx, cy) with overall radius
// `r`, using only citro2d primitives. `anim` is a continuously increasing
// frame counter that drives the animation (rotating rays, falling drops, ...).
void iconDraw(int weather_code, int is_day, float cx, float cy, float r, float anim);
