#include "http.h"
#include <stdlib.h>
#include <string.h>

#define HTTP_USER_AGENT  "Meteo3DS/1.0"
#define HTTP_MAX_REDIR   5
#define HTTP_CHUNK       (16 * 1024)

static volatile bool s_cancel = false;

void httpSetCancel(bool cancel) { s_cancel = cancel; }

// Download the body of an already-opened context into a growing buffer.
static Result downloadBody(httpcContext *ctx, char **out, u32 *out_len) {
	u32 cap = HTTP_CHUNK;
	u32 len = 0;
	char *buf = (char *)malloc(cap);
	if (!buf) return -1;

	Result ret;
	do {
		if (len + HTTP_CHUNK + 1 > cap) {
			cap *= 2;
			char *grown = (char *)realloc(buf, cap);
			if (!grown) { free(buf); return -1; }
			buf = grown;
		}
		u32 got = 0;
		ret = httpcDownloadData(ctx, (u8 *)buf + len, HTTP_CHUNK, &got);
		len += got;
		if (s_cancel) { free(buf); return -2; }
	} while (ret == (Result)HTTPC_RESULTCODE_DOWNLOADPENDING);

	if (ret != 0) { free(buf); return ret; }

	buf[len] = '\0';
	*out = buf;
	*out_len = len;
	return 0;
}

Result httpGet(const char *url, char **out, u32 *out_len) {
	*out = NULL;
	if (out_len) *out_len = 0;

	char current[1024];
	strncpy(current, url, sizeof(current) - 1);
	current[sizeof(current) - 1] = '\0';

	for (int redirs = 0; redirs <= HTTP_MAX_REDIR; redirs++) {
		httpcContext ctx;
		Result ret = httpcOpenContext(&ctx, HTTPC_METHOD_GET, current, 1);
		if (ret != 0) return ret;

		// Modern endpoints often serve only HTTPS and the 3DS CA store can't
		// validate them — disable verification so the request still succeeds.
		httpcSetSSLOpt(&ctx, SSLCOPT_DisableVerify);
		httpcSetKeepAlive(&ctx, HTTPC_KEEPALIVE_ENABLED);
		httpcAddRequestHeaderField(&ctx, "User-Agent", HTTP_USER_AGENT);
		httpcAddRequestHeaderField(&ctx, "Connection", "Keep-Alive");

		ret = httpcBeginRequest(&ctx);
		if (ret != 0) { httpcCloseContext(&ctx); return ret; }

		u32 status = 0;
		ret = httpcGetResponseStatusCode(&ctx, &status);
		if (ret != 0) { httpcCloseContext(&ctx); return ret; }

		// Follow redirects.
		if (status >= 301 && status <= 308) {
			char location[1024];
			ret = httpcGetResponseHeader(&ctx, "Location", location, sizeof(location));
			httpcCloseContext(&ctx);
			if (ret != 0) return ret;
			strncpy(current, location, sizeof(current) - 1);
			current[sizeof(current) - 1] = '\0';
			continue;
		}

		if (status != 200) {
			httpcCloseContext(&ctx);
			return (Result)status; // surface the HTTP status as the error
		}

		ret = downloadBody(&ctx, out, out_len);
		httpcCloseContext(&ctx);
		return ret;
	}

	return -1; // too many redirects
}
