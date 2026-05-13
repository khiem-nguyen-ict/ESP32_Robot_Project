#ifndef ROBOT_TYPES_H
#define ROBOT_TYPES_H

/* ================================================================
 * robot_types.h
 * Shared data types used across all modules.
 * Written in C — no classes, no templates.
 * ================================================================ */

#include <stdint.h>
#include <stdbool.h>

/* ----------------------------------------------------------------
 * Robot operating mode
 * ---------------------------------------------------------------- */
typedef enum {
    MODE_MANUAL = 0,   /* remote control via WebSocket */
    MODE_AUTO   = 1    /* autonomous obstacle avoidance */
} robot_mode_t;

/* ----------------------------------------------------------------
 * Movement command
 * ---------------------------------------------------------------- */
typedef enum {
    CMD_STOP     = 0,
    CMD_FORWARD  = 1,
    CMD_BACKWARD = 2,
    CMD_LEFT     = 3,
    CMD_RIGHT    = 4
} move_cmd_t;

/* ----------------------------------------------------------------
 * Sensor reading from HC-SR04 (three directions)
 * ---------------------------------------------------------------- */
typedef struct {
    float    dist_left;    /* distance to the left  (cm) */
    float    dist_center;  /* distance straight ahead (cm) */
    float    dist_right;   /* distance to the right (cm) */
    uint32_t timestamp_ms; /* millis() at time of reading */
} sensor_data_t;

/* ----------------------------------------------------------------
 * Complete robot status snapshot broadcast to WebSocket clients
 * ---------------------------------------------------------------- */
typedef struct {
    sensor_data_t sensor;
    robot_mode_t  mode;
    move_cmd_t    last_cmd;
    uint8_t       speed;       /* 0-255 */
    bool          obstacle;    /* true when path is blocked */
    uint32_t      uptime_ms;   /* millis() since boot */
} robot_status_t;

/* ----------------------------------------------------------------
 * Shared state block — protected by a FreeRTOS mutex.
 * Both tasks read/write through this single structure.
 * ---------------------------------------------------------------- */
typedef struct {
    volatile robot_mode_t mode;
    volatile move_cmd_t   cmd;
    sensor_data_t         sensor;   /* guarded by sensor_mutex */
    uint8_t               speed;
} shared_state_t;

/* ----------------------------------------------------------------
 * Helper: command name string (for logging)
 * ---------------------------------------------------------------- */
static inline const char *cmd_name(move_cmd_t c) {
    switch (c) {
        case CMD_FORWARD:  return "FORWARD";
        case CMD_BACKWARD: return "BACKWARD";
        case CMD_LEFT:     return "LEFT";
        case CMD_RIGHT:    return "RIGHT";
        default:           return "STOP";
    }
}

/* Helper: mode name string */
static inline const char *mode_name(robot_mode_t m) {
    return (m == MODE_AUTO) ? "auto" : "manual";
}

#endif /* ROBOT_TYPES_H */
