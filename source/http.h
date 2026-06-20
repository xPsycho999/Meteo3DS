#pragma once
#include <3ds.h>

// Simple HTTP(S) GET for the 3DS built on libctru's httpc service.
//
// Downloads the body of `url` into a freshly malloc'd, NUL-terminated buffer.
// On success returns 0, sets *out to the buffer (caller frees) and *out_len to
// the body length (excluding the terminating NUL). On failure returns a non-zero
// Result and leaves *out = NULL.
//
// Handles: chunked responses (no Content-Length), HTTP->HTTPS redirects (3xx),
// and HTTPS with certificate verification DISABLED (the 3DS CA store is too old
// for modern endpoints — see the project memory 3ds-networking-http-only).
Result httpGet(const char *url, char **out, u32 *out_len);

// Request cancellation of an in-flight httpGet (checked between download
// chunks). Set true to abort; reset to false before starting a new request.
void httpSetCancel(bool cancel);
