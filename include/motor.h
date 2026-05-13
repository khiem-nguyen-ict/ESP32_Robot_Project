#ifndef MOTOR_H
#define MOTOR_H

/* ================================================================
 * motor.h
 * DC motor control via L298N driver using ESP32 LEDC (PWM).
 * Exposes a plain-C API: init, direction commands, speed control.
 * ================================================================ */

#include <stdint.h>
#include "robot_types.h"

/* ----------------------------------------------------------------
 * Motor handle — holds all hardware configuration for one driver.
 * Treat as opaque; initialise with motor_init().
 * ---------------------------------------------------------------- */
typedef struct {
    /* Motor A (left wheel) */
    uint8_t in1;
    uint8_t in2;
    uint8_t en_a;
    uint8_t ch_a;   /* LEDC channel */

    /* Motor B (right wheel) */
    uint8_t in3;
    uint8_t in4;
    uint8_t en_b;
    uint8_t ch_b;   /* LEDC channel */

    uint8_t speed;  /* current speed 0-255 */
} motor_handle_t;

/* ----------------------------------------------------------------
 * API
 * ---------------------------------------------------------------- */

/* Initialise GPIO and LEDC; must be called once before any other
   motor function.  Returns 0 on success, -1 on failure. */
int  motor_init(motor_handle_t *m,
                uint8_t in1, uint8_t in2, uint8_t en_a, uint8_t ch_a,
                uint8_t in3, uint8_t in4, uint8_t en_b, uint8_t ch_b);

/* Set global speed (clamped to SPEED_MIN – SPEED_MAX). */
void motor_set_speed(motor_handle_t *m, uint8_t speed);

/* Basic direction commands */
void motor_forward(motor_handle_t *m);
void motor_backward(motor_handle_t *m);
void motor_turn_left(motor_handle_t *m);
void motor_turn_right(motor_handle_t *m);
void motor_stop(motor_handle_t *m);

/* Execute a move_cmd_t enum value directly */
void motor_execute(motor_handle_t *m, move_cmd_t cmd);

/* Smooth ramp from current speed to target over duration_ms.
   Blocks the calling task for duration_ms using vTaskDelay(). */
void motor_ramp_to(motor_handle_t *m, uint8_t target, uint16_t duration_ms);

#endif /* MOTOR_H */
