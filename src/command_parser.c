/* ================================================================
 * command_parser.c
 * Deserialise WebSocket JSON into shared_state_t updates.
 * ArduinoJson v7 is used for parsing (stack-allocated document).
 * ================================================================ */

#include "command_parser.h"
#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

/* ================================================================
 * cmd_parse()
 * ================================================================ */
void cmd_parse(const char *json, size_t len, shared_state_t *state)
{
    if (json == NULL || state == NULL || len == 0) return;

    /* Stack-allocated JSON document — 192 bytes is enough for all
       our message types.                                          */
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json, len);

    if (err) {
        LOGF("[Parser] JSON error: %s\n", err.c_str());
        return;
    }

    const char *type = doc["type"];
    if (type == NULL) return;

    /* ------------------------------------------------------------ *
     * "move" — update movement command (manual mode only)
     * ------------------------------------------------------------ */
    if (strcmp(type, "move") == 0) {
        if (state->mode != MODE_MANUAL) return;

        const char *cmd_str = doc["cmd"];
        if (cmd_str == NULL) return;

        if      (strcmp(cmd_str, "forward")  == 0) state->cmd = CMD_FORWARD;
        else if (strcmp(cmd_str, "backward") == 0) state->cmd = CMD_BACKWARD;
        else if (strcmp(cmd_str, "left")     == 0) state->cmd = CMD_LEFT;
        else if (strcmp(cmd_str, "right")    == 0) state->cmd = CMD_RIGHT;
        else                                        state->cmd = CMD_STOP;

        LOGF("[Parser] Move -> %s\n", cmd_name(state->cmd));
    }

    /* ------------------------------------------------------------ *
     * "mode" — switch between MANUAL and AUTO
     * ------------------------------------------------------------ */
    else if (strcmp(type, "mode") == 0) {
        const char *val = doc["value"];
        if (val == NULL) return;

        if (strcmp(val, "auto") == 0) {
            state->mode = MODE_AUTO;
            state->cmd  = CMD_STOP;   /* stop immediately on switch */
            LOGLN("[Parser] Mode -> AUTO");
        } else {
            state->mode = MODE_MANUAL;
            state->cmd  = CMD_STOP;
            LOGLN("[Parser] Mode -> MANUAL");
        }
    }

    /* ------------------------------------------------------------ *
     * "speed" — update motor speed (0-255)
     * ------------------------------------------------------------ */
    else if (strcmp(type, "speed") == 0) {
        uint8_t spd = (uint8_t)doc["value"].as<int>();
        if (spd < SPEED_MIN) spd = SPEED_MIN;
        if (spd > SPEED_MAX) spd = SPEED_MAX;
        state->speed = spd;
        LOGF("[Parser] Speed -> %d\n", spd);
    }

    else {
        LOGF("[Parser] Unknown type: %s\n", type);
    }
}
