#include "i18n.h"
#include <3ds.h>

Lang g_lang = LANG_EN;

void i18nInit(void) {
	u8 sys = 0;
	if (R_SUCCEEDED(cfguInit())) {
		if (R_SUCCEEDED(CFGU_GetSystemLanguage(&sys)) && sys == CFG_LANGUAGE_DE)
			g_lang = LANG_DE;
		cfguExit();
	}
}

const char *tr(const char *de, const char *en) {
	return (g_lang == LANG_DE) ? de : en;
}
