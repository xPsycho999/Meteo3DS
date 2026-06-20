# 3DS Weather

A polished weather app for the Nintendo 3DS, written from scratch in C with
[citro2d](https://github.com/devkitPro/citro2d). Live forecasts, animated vector
weather icons, air quality and pollen, and a touch-driven UI — fully bilingual
(English / German, auto-detected from the console language).

Powered by the free, key-less [Open-Meteo](https://open-meteo.com/) API over plain
HTTP, so it works with the 3DS's outdated certificate store.

## Features

- **Current conditions** — temperature, "feels like", humidity, wind speed +
  direction, precipitation and UV, with a crisp seven-segment temperature readout
- **Animated vector icons** and a per-condition **day/night** background theme
- **Hourly** temperature graph with precipitation bars, and a **7-day** forecast
- **Air quality & pollen** — European AQI, PM2.5 / PM10, grass / birch / mugwort
- **Multiple cities** (up to 6) — add by search or by IP geolocation, quick-switch
  with the D-pad, persisted across restarts
- **Auto location** on first launch (IP-based)
- **Bilingual** — English & German, auto-selected from the system language and
  switchable any time in Settings (along with °C/°F and km/h ↔ m/s)
- Stays responsive while loading: networking runs on a background worker thread,
  so HOME / START never freeze

## Controls

- **Touch / L / R** — switch tabs (Today · Hourly · 7 Days · Air · Cities)
- **D-pad ◄ / ►** — quick-switch the active city
- **X** — refresh · **Y** — search for a city
- **SELECT** — settings · **START** — quit

## Running

Install the `.cia` from the [Releases](../../releases) page via FBI, or copy
`3ds-weather.3dsx` to your SD card and launch it from the Homebrew Launcher on CFW.
Settings and cities are stored at `sdmc:/3ds/wetter/config.json`.

## Building

Requires [devkitARM](https://devkitpro.org/) with libctru, citro2d and citro3d.

```sh
make            # -> 3ds-weather.3dsx
```

A `.cia` can be built from the resulting `.elf` with `build-cia.ps1` (needs makerom
and bannertool).

## Credits & inspiration

Inspiration from two existing 3DS weather projects — thanks to their authors:

- **[Luma3DSWeather](https://github.com/Dzhmelyk135/Luma3DSWeather)** by Dzhmelyk135
- **[Forecast](https://github.com/NatTupper/Forecast)** by NatTupper

Weather, geocoding and air-quality data by **[Open-Meteo](https://open-meteo.com/)**.
IP geolocation by [ip-api.com](http://ip-api.com/). Built on
[citro2d](https://github.com/devkitPro/citro2d) / [devkitPro](https://devkitpro.org/).

## License

Copyright (C) 2026 xPsycho999

This program is free software: you can redistribute it and/or modify it under the
terms of the **GNU General Public License v3.0** as published by the Free Software
Foundation. See the [LICENSE](LICENSE) file for the full text, or
<https://www.gnu.org/licenses/gpl-3.0.html>.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
