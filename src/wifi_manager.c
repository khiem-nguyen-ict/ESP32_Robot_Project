/* ================================================================
 * wifi_manager.c
 * WiFi (STA with AP fallback), AsyncWebServer and WebSocket.
 *
 * The C++ library objects (AsyncWebServer, AsyncWebSocket,
 * ArduinoJson) are heap-allocated and stored inside an opaque
 * struct so the rest of the project stays compilable as C.
 * ================================================================ */

#include "wifi_manager.h"
#include "config.h"
#include "webpage.h"         /* ROBOT_HTML PROGMEM string */

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* ----------------------------------------------------------------
 * Internal struct definition (hidden from other translation units)
 * ---------------------------------------------------------------- */
struct wifi_manager_s {
    AsyncWebServer   *server;
    AsyncWebSocket   *ws;
    ws_cmd_callback_t callback;
};

/* ----------------------------------------------------------------
 * Forward declarations of internal helpers
 * ---------------------------------------------------------------- */
static void  wm_connect_wifi(void);
static void  wm_setup_routes(struct wifi_manager_s *wm);
static void  wm_on_ws_event(AsyncWebSocket *srv,
                             AsyncWebSocketClient *client,
                             AwsEventType type,
                             void *arg, uint8_t *data, size_t len,
                             struct wifi_manager_s *wm);

/* ================================================================
 * wifi_manager_create()
 * Allocate the opaque handle and its C++ server objects.
 * ================================================================ */
wifi_manager_t *wifi_manager_create(void)
{
    wifi_manager_t *wm = (wifi_manager_t *)malloc(sizeof(wifi_manager_t));
    if (wm == NULL) return NULL;

    memset(wm, 0, sizeof(wifi_manager_t));

    wm->server   = new AsyncWebServer(WEB_SERVER_PORT);
    wm->ws       = new AsyncWebSocket("/ws");
    wm->callback = NULL;

    if (wm->server == NULL || wm->ws == NULL) {
        wifi_manager_destroy(wm);
        return NULL;
    }

    return wm;
}

/* ================================================================
 * wifi_manager_destroy()
 * ================================================================ */
void wifi_manager_destroy(wifi_manager_t *wm)
{
    if (wm == NULL) return;
    delete wm->server;
    delete wm->ws;
    free(wm);
}

/* ================================================================
 * wifi_manager_set_callback()
 * ================================================================ */
void wifi_manager_set_callback(wifi_manager_t *wm, ws_cmd_callback_t cb)
{
    if (wm) wm->callback = cb;
}

/* ================================================================
 * wifi_manager_begin()
 * Connect to WiFi, set up routes and start the server.
 * ================================================================ */
int wifi_manager_begin(wifi_manager_t *wm)
{
    if (wm == NULL) return -1;

    wm_connect_wifi();
    wm_setup_routes(wm);

    wm->server->begin();
    LOGLN("[WiFi] Web server started on port " TOSTRING(WEB_SERVER_PORT));
    return 0;
}

/* ================================================================
 * wifi_manager_broadcast()
 * Serialise robot_status_t → JSON and push to all WS clients.
 * ================================================================ */
void wifi_manager_broadcast(wifi_manager_t *wm, const robot_status_t *status)
{
    if (wm == NULL || status == NULL)            return;
    if (wm->ws->count() == 0)                   return;   /* no clients */

    /* Use ArduinoJson v7 stack-allocated document (256 bytes) */
    JsonDocument doc;

    doc["type"]     = "status";
    doc["mode"]     = mode_name(status->mode);
    doc["lastCmd"]  = cmd_name(status->last_cmd);
    doc["speed"]    = status->speed;
    doc["obstacle"] = status->obstacle;
    doc["uptime"]   = status->uptime_ms;
    doc["left"]     = (int)status->sensor.dist_left;
    doc["center"]   = (int)status->sensor.dist_center;
    doc["right"]    = (int)status->sensor.dist_right;

    /* Serialise to a temporary buffer on the stack */
    char buf[256];
    size_t len = serializeJson(doc, buf, sizeof(buf));

    wm->ws->textAll(buf, len);
}

/* ================================================================
 * wifi_manager_cleanup()
 * Remove disconnected WebSocket clients to free memory.
 * ================================================================ */
void wifi_manager_cleanup(wifi_manager_t *wm)
{
    if (wm) wm->ws->cleanupClients();
}

/* ================================================================
 * Query helpers
 * ================================================================ */
bool wifi_manager_is_connected(const wifi_manager_t *wm)
{
    (void)wm;
    return WiFi.status() == WL_CONNECTED;
}

size_t wifi_manager_client_count(wifi_manager_t *wm)
{
    return (wm && wm->ws) ? wm->ws->count() : 0;
}

/* ================================================================
 * Internal: wm_connect_wifi()
 * Try to connect to WIFI_SSID; fall back to AP mode on timeout.
 * ================================================================ */
static void wm_connect_wifi(void)
{
    uint32_t start;

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    LOGF("[WiFi] Connecting to \"%s\"", WIFI_SSID);

    start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           (millis() - start) < WIFI_TIMEOUT_MS) {
        vTaskDelay(pdMS_TO_TICKS(500));
        LOG(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        LOGLN("");
        LOGF("[WiFi] Connected — IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        /* Fallback: start as an Access Point */
        LOGLN("\n[WiFi] Timeout — starting AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
        LOGF("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    }
}

/* ================================================================
 * Internal: wm_setup_routes()
 * Register WebSocket event handler and HTTP route for the web UI.
 * ================================================================ */
static void wm_setup_routes(wifi_manager_t *wm)
{
    /* Store wm pointer for use inside the lambda below.
       Using a pointer-sized capture avoids heap allocation. */
    wifi_manager_t *ctx = wm;

    wm->ws->onEvent(
        [ctx](AsyncWebSocket *srv, AsyncWebSocketClient *client,
              AwsEventType type, void *arg, uint8_t *data, size_t len) {
            wm_on_ws_event(srv, client, type, arg, data, len, ctx);
        }
    );

    wm->server->addHandler(wm->ws);

    /* Serve the embedded HTML page */
    wm->server->on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send_P(200, "text/html", ROBOT_HTML);
    });

    /* 404 handler */
    wm->server->onNotFound([](AsyncWebServerRequest *req) {
        req->send(404, "text/plain", "Not found");
    });
}

/* ================================================================
 * Internal: wm_on_ws_event()
 * Handle WebSocket connect / disconnect / data events.
 * ================================================================ */
static void wm_on_ws_event(AsyncWebSocket *srv,
                            AsyncWebSocketClient *client,
                            AwsEventType type,
                            void *arg, uint8_t *data, size_t len,
                            wifi_manager_t *wm)
{
    switch (type) {

    case WS_EVT_CONNECT:
        LOGF("[WS] Client #%u connected from %s\n",
             client->id(), client->remoteIP().toString().c_str());
        break;

    case WS_EVT_DISCONNECT:
        LOGF("[WS] Client #%u disconnected\n", client->id());
        break;

    case WS_EVT_DATA: {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        /* Accept only complete, unfragmented text frames */
        if (info->final && info->index == 0 &&
            info->len == len && info->opcode == WS_TEXT) {

            /* Null-terminate the payload safely */
            char *json = (char *)malloc(len + 1);
            if (json == NULL) break;
            memcpy(json, data, len);
            json[len] = '\0';

            LOGF("[WS] RX: %s\n", json);

            if (wm->callback) wm->callback(json, len);

            free(json);
        }
        break;
    }

    case WS_EVT_ERROR:
        LOGF("[WS] Error from client #%u: %s\n",
             client->id(), (char *)data);
        break;

    default:
        break;
    }
}
