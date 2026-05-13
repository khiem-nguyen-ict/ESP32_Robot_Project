#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

/* ================================================================
 * command_parser.h
 * Parse JSON commands received from the WebSocket client and
 * apply them to the shared_state_t.
 *
 * Supported JSON message formats:
 *
 *   Move command (manual mode only):
 *     {"type":"move","cmd":"forward"}
 *     {"type":"move","cmd":"backward"}
 *     {"type":"move","cmd":"left"}
 *     {"type":"move","cmd":"right"}
 *     {"type":"move","cmd":"stop"}
 *
 *   Mode switch:
 *     {"type":"mode","value":"manual"}
 *     {"type":"mode","value":"auto"}
 *
 *   Speed change:
 *     {"type":"speed","value":200}
 * ================================================================ */

#include <stddef.h>
#include "robot_types.h"

/* ----------------------------------------------------------------
 * cmd_parse()
 * Parse a null-terminated JSON string and update *state.
 * Calls motor_stop() on the provided motor handle when switching
 * to AUTO mode or receiving a STOP command.
 * Thread-safe note: the caller must hold any necessary mutex
 * before calling this function.
 * ---------------------------------------------------------------- */
void cmd_parse(const char *json, size_t len, shared_state_t *state);

#endif /* COMMAND_PARSER_H */
