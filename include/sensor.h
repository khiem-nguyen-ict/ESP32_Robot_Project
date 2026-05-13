#ifndef SENSOR_H
#define SENSOR_H

/* ================================================================
 * sensor.h
 * HC-SR04 ultrasonic sensor + SG90 servo module.
 * The servo sweeps left / center / right so the HC-SR04 can
 * measure obstacle distances in three directions for auto mode.
 * ================================================================ */

#include <stdint.h>
#include "robot_types.h"

/* ----------------------------------------------------------------
 * Sensor handle — holds GPIO and servo state.
 * Initialise with sensor_init().
 * ---------------------------------------------------------------- */
typedef struct {
    uint8_t trig_pin;
    uint8_t echo_pin;
    uint8_t servo_pin;
    /* servo object stored as void* to keep this header pure C
       (the Servo object is heap-allocated in sensor.c)           */
    void   *servo_obj;
} sensor_handle_t;

/* ----------------------------------------------------------------
 * API
 * ---------------------------------------------------------------- */

/* Initialise HC-SR04 GPIO and attach SG90 servo.
   Returns 0 on success, -1 on failure. */
int   sensor_init(sensor_handle_t *s,
                  uint8_t trig_pin, uint8_t echo_pin, uint8_t servo_pin);

/* Free resources (detach servo). */
void  sensor_deinit(sensor_handle_t *s);

/* Measure distance once (raw, no filtering). Returns cm. */
float sensor_measure_once(const sensor_handle_t *s);

/* Measure distance N times and return the median (noise filter). */
float sensor_measure(const sensor_handle_t *s, uint8_t samples);

/* Sweep servo to all three positions and fill *out.
   Blocks the calling task while the servo settles and measures. */
void  sensor_scan_all(sensor_handle_t *s, sensor_data_t *out);

/* Move servo to a specific angle (0-180 degrees). */
void  sensor_servo_angle(sensor_handle_t *s, uint8_t angle);
void  sensor_servo_center(sensor_handle_t *s);
void  sensor_servo_left(sensor_handle_t *s);
void  sensor_servo_right(sensor_handle_t *s);

#endif /* SENSOR_H */
