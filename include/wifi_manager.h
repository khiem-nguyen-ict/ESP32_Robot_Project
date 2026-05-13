#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/* ================================================================
 * wifi_manager.h
 * WiFi connection, AsyncWebServer, and WebSocket management.
 *
 * The underlying C++ libraries (ESPAsyncWebServer, ArduinoJson)
 * are fully contained inside wifi_manager.c; this header exposes
 * a clean C API to the rest of the application.
 * ================================================================ */

#include <stdint.h>
#include <stdbool.h>
#include "robot_types.h"

/* ----------------------------------------------------------------
 * Callback type invoked on the Protocol-CPU task whenever a valid
 * JSON message arrives from a WebSocket client.
 * 'json'   : null-terminated JSON string from the client.
 * 'length' : string length (not including the null terminator).
 * ---------------------------------------------------------------- */
typedef void (*ws_cmd_callback_t)(const char *json, size_t length);

/* ----------------------------------------------------------------
 * WiFi manager handle — opaque internals kept in wifi_manager.c
 * ---------------------------------------------------------------- */
typedef struct wifi_manager_s wifi_manager_t;

/* ----------------------------------------------------------------
 * API
 * ---------------------------------------------------------------- */

/* Allocate and return a new wifi_manager handle.
   Returns NULL if allocation fails. */
wifi_manager_t *wifi_manager_create(void);

/* Free the handle and its resources. */
void wifi_manager_destroy(wifi_manager_t *wm);

/* Register the command callback (must be called before begin). */
void wifi_manager_set_callback(wifi_manager_t *wm, ws_cmd_callback_t cb);

/* Connect to WiFi (falls back to AP mode on timeout) and start the
   web server + WebSocket endpoint.  Returns 0 on success. */
int  wifi_manager_begin(wifi_manager_t *wm);

/* Broadcast a robot_status_t snapshot as JSON to all WS clients. */
void wifi_manager_broadcast(wifi_manager_t *wm, const robot_status_t *status);

/* Remove stale WebSocket clients (call periodically). */
void wifi_manager_cleanup(wifi_manager_t *wm);

/* Query helpers */
bool   wifi_manager_is_connected(const wifi_manager_t *wm);
size_t wifi_manager_client_count(wifi_manager_t *wm);

#endif /* WIFI_MANAGER_H */
