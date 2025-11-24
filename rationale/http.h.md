# Rationale – `include/http.h`

Declares `http_server_thread`, the HTTP/SSE server entry point.

## Why this interface
- Keeps HTTP concerns isolated from `main.c` and sensor logic.
- Passes `SharedState*` so the server can read live data without extra copying.
