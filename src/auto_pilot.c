/* ================================================================
 * auto_pilot.c
 * Autonomous obstacle-avoidance algorithm.
 *
 * The servo sweeps to three positions so the HC-SR04 can build a
 * simple front-arc map.  Decisions are made purely from the three
 * distance readings.
 * ================================================================ */

#include "auto_pilot.h"
#include "config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

/* ================================================================
 * auto_pilot_run()
 * One full cycle: scan → decide → manoeuvre.
 * ================================================================ */
void auto_pilot_run(motor_handle_t *m, sensor_handle_t *s, sensor_data_t *out)
{
    bool left_clear, right_clear;

    /* --- Step 1: scan three directions with servo + HC-SR04 --- */
    sensor_scan_all(s, out);

    left_clear  = (out->dist_left   > OBSTACLE_DIST_CM);
    right_clear = (out->dist_right  > OBSTACLE_DIST_CM);

    /* --- Step 2: decide and execute manoeuvre --- */

    if (out->dist_center > OBSTACLE_DIST_CM) {
        /* Path ahead is clear — drive forward */
        motor_forward(m);
        vTaskDelay(pdMS_TO_TICKS(AUTO_FWD_MS));

    } else {
        /* Obstacle ahead — stop first */
        motor_stop(m);
        vTaskDelay(pdMS_TO_TICKS(80));

        if (left_clear && right_clear) {
            /* Both sides are free — choose the side with more space */
            if (out->dist_left >= out->dist_right) {
                LOGLN("[Auto] Obstacle — turning LEFT (more space)");
                motor_turn_left(m);
            } else {
                LOGLN("[Auto] Obstacle — turning RIGHT (more space)");
                motor_turn_right(m);
            }
            vTaskDelay(pdMS_TO_TICKS(AUTO_TURN_MS));

        } else if (left_clear) {
            LOGLN("[Auto] Obstacle — turning LEFT");
            motor_turn_left(m);
            vTaskDelay(pdMS_TO_TICKS(AUTO_TURN_MS));

        } else if (right_clear) {
            LOGLN("[Auto] Obstacle — turning RIGHT");
            motor_turn_right(m);
            vTaskDelay(pdMS_TO_TICKS(AUTO_TURN_MS));

        } else {
            /* Blocked on all three sides — back up then spin 180° */
            LOGLN("[Auto] All blocked — backing up and spinning");
            motor_backward(m);
            vTaskDelay(pdMS_TO_TICKS(AUTO_BACK_MS));
            motor_turn_right(m);
            vTaskDelay(pdMS_TO_TICKS(AUTO_SPIN_MS));
        }

        motor_stop(m);
    }
}
