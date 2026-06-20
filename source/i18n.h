#pragma once

// Bilingual UI. The whole app stores data language-neutrally and only picks a
// language when drawing, via tr(). Default is English; German is auto-selected
// when the console's system language is German (see i18nInit), and the user can
// override it in Settings (persisted in config.json).
typedef enum { LANG_EN = 0, LANG_DE = 1 } Lang;

extern Lang g_lang;

// Detect the console's system language (CFGU_GetSystemLanguage) and set g_lang:
// German console -> LANG_DE, everything else -> LANG_EN. A saved config can then
// override this on load. Safe to call once at startup.
void i18nInit(void);

// Pick the string for the current language: tr(german, english).
const char *tr(const char *de, const char *en);
