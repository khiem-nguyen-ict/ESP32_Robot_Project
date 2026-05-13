#ifndef AUTO_PILOT_H
#define AUTO_PILOT_H

/* ================================================================
 * auto_pilot.h
 * Autonomous obstacle-avoidance decision logic.
 *
 * The algorithm reads three distance values (left / center / right)
 * measured by the HC-SR04 and decides the next manoeuvre:
 *
 *   center clear            → drive FORWARD
 *   center blocked, both sides clear → turn toward the farther side
 *   center blocked, only left clear  → turn LEFT
 *   center blocked, only right clear → turn RIGHT
 *   all three blocked               → BACK UP + spin 180°
 * ================================================================ */

#include "motor.h"
#include "sensor.h"
#include "robot_types.h"

/* ================================================================
 * auto_pilot_run()
 * Execute one decision cycle for autonomous mode.
 *
 *   m   : motor handle (must be initialised)
 *   s   : sensor handle (must be initialised)
 *   out : filled with the distances measured during this cycle
 *
 * Blocks the calling FreeRTOS task for the duration of sensor
 * scanning + manoeuvre execution.
 * ================================================================ */
void auto_pilot_run(motor_handle_t *m, sensor_handle_t *s, sensor_data_t *out);

#endif /* AUTO_PILOT_H */
