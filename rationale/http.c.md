# Rationale – `src/http.c`

This file serves the dashboard and streams live updates. Choices aim for simplicity and zero extra dependencies.

## Key choices
- **TCP server**: Standard sockets (`socket`/`bind`/`listen`/`accept`) to serve HTTP on `web_port`.
- **SSE (Server-Sent Events)**: One-way, browser-native push from server to client; simpler than WebSockets and lighter than polling for this read-only stream.
- **Thread-per-connection**: Easy to read and sufficient for a small demo; avoids complex event loops. Each client handler reads `SharedState` under a mutex (via the HTTP thread) to get consistent data.
- **Homemade static file serving**: Serving a handful of files doesn’t justify a full HTTP library.
- **Content-type guessing**: Minimal mapping (.html, .css, .js, .png, .svg) to make browsers happy.

## Walk-through
- `send_404`, `send_file`: Basic HTTP responses; `send_file` writes headers then streams file bytes. Keeps errors explicit if assets are missing.
- `guess_ctype`: Maps extensions to MIME types so browsers render correctly.
- `cat_safe`, `build_alerts`: Builds JSON alert arrays and human summaries without overflowing buffers. Used for the SSE payload.
- `json_for_current`: Formats the current `SensorData` into a JSON string (alerts, connection, via, seq) sent to clients.
- `handle_client`:
  - Parses the request line; defaults `/` to `/index.html`.
  - If `/events`, writes SSE headers and loops: checks `last_seq`, sends JSON when data changes, otherwise keepalive comments. Reads are done after locking `SharedState` in the HTTP thread to avoid torn snapshots.
  - Otherwise serves static files from `web/`.
- `http_server_thread`: Creates a listening socket, sets `SO_REUSEADDR`, binds to `INADDR_ANY:web_port`, listens, accepts, and spawns a thread per client with shared `SharedState` pointer.

## How this serves the product
- Delivers the dashboard UI (HTML/JS/CSS) and live data updates without page reloads.
- Uses SSE so changes appear immediately, matching the real-time nature of sensor readings.
- Aligns with the sensor thread via `last_seq` to avoid sending unchanged data and to keep bandwidth low.
