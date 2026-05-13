/* ================================================================
 * main.c
 * ESP32 WiFi Mobile Robot — main entry point.
 *
 * Architecture overview
 * ─────────────────────
 *
 *   Core 0 (Protocol CPU)            Core 1 (App CPU)
 *   ┌──────────────────────┐         ┌──────────────────────┐
 *   │  task_web            │         │  task_motion         │
 *   │  Priority : 1        │         │  Priority : 2        │
 *   │  Stack    : 8192 B   │         │  Stack    : 4096 B   │
 *   │                      │         │                      │
 *   │  • Broadcast status  │         │  • Execute move cmd  │
 *   │    JSON via WS       │         │  • Run auto-pilot    │
 *   │  • Cleanup WS clients│         │  • Read HC-SR04      │
 *   └──────────────────────┘         └──────────────────────┘
 *            │                                  │
 *            └──────── shared_state_t ──────────┘
 *                   (protected by mutex)
 *
 * Module dependencies
 * ───────────────────
 *   main.c
 *     ├── motor.h / motor.c         L298N driver (LEDC PWM)
 *     ├── sensor.h / sensor.c       HC-SR04 + SG90 servo
 *     ├── wifi_manager.h / .c       WiFi, WebServer, WebSocket
 *     ├── command_parser.h / .c     JSON → state transitions
 *     ├── auto_pilot.h / .c         Obstacle avoidance algorithm
 *     ├── config.h                  All pin defs and constants
 *     ├── robot_types.h             Enums, structs, helpers
 *     └── webpage.h                 Embedded HTML/CSS/JS (PROGMEM)
 * ================================================================ */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "config.h"
#include "robot_types.h"
#include "motor.h"
#include "sensor.h"
#include "wifi_manager.h"
#include "command_parser.h"
#include "auto_pilot.h"

/* ================================================================
 * Global hardware handles
 * ================================================================ */
static motor_handle_t   g_motor;
static sensor_handle_t  g_sensor;
static wifi_manager_t  *g_wifi = NULL;

/* ================================================================
 * Shared state — accessed from both FreeRTOS tasks.
 * All accesses MUST be protected by g_mutex.
 * ================================================================ */
static shared_state_t  g_state = {
    .mode   = MODE_MANUAL,
    .cmd    = CMD_STOP,
    .sensor = {999.0f, 999.0f, 999.0f, 0},
    .speed  = SPEED_DEFAULT
};

static SemaphoreHandle_t g_mutex = NULL;

/* ================================================================
 * WebSocket command callback
 * Called from the Protocol-CPU task (Core 0) inside the WS event
 * handler.  Must be fast; no blocking calls allowed here.
 * ================================================================ */
static void on_ws_command(const char *json, size_t len)
{
    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        cmd_parse(json, len, &g_state);
        /* Apply speed change immediately to the motor handle */
        motor_set_speed(&g_motor, g_state.speed);
        xSemaphoreGive(g_mutex);
    }
}

/* ================================================================
 * task_motion  (Core 1 — App CPU, priority 2)
 *
 * Responsibilities:
 *   • MANUAL mode : execute move commands from g_state.cmd
 *   • AUTO   mode : run one auto_pilot_run() cycle per iteration
 *   • Write fresh sensor data back into g_state.sensor
 * ================================================================ */
static void task_motion(void *param)
{
    TickType_t   wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(PERIOD_MOTION_MS);

    robot_mode_t local_mode;
    move_cmd_t   local_cmd;
    sensor_data_t new_sensor;

    LOGF("[Motion] Task started on core %d\n", xPortGetCoreID());

    for (;;) {
        /* --- Snapshot shared state under mutex --- */
        if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            local_mode = g_state.mode;
            local_cmd  = g_state.cmd;
            xSemaphoreGive(g_mutex);
        } else {
            /* Could not acquire mutex; skip this cycle */
            vTaskDelayUntil(&wake, period);
            continue;
        }

        /* -------------------------------------------------------- *
         * MANUAL mode
         * -------------------------------------------------------- */
        if (local_mode == MODE_MANUAL) {
            /* Execute the requested move command */
            motor_execute(&g_motor, local_cmd);

            /* Quick single center reading (servo stays at 90°) */
            new_sensor.dist_center = sensor_measure(&g_sensor, 2);
            new_sensor.dist_left   = SENSOR_NO_ECHO;
            new_sensor.dist_right  = SENSOR_NO_ECHO;
            new_sensor.timestamp_ms = (uint32_t)millis();

            /* Write sensor result back */
            if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                g_state.sensor = new_sensor;
                xSemaphoreGive(g_mutex);
            }

            vTaskDelayUntil(&wake, period);
        }

        /* -------------------------------------------------------- *
         * AUTO mode
         * -------------------------------------------------------- */
        else {
            /* auto_pilot_run() blocks for servo settle + measure +
               manoeuvre duration — it manages its own timing.      */
            auto_pilot_run(&g_motor, &g_sensor, &new_sensor);

            /* Write results back under mutex */
            if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                g_state.sensor = new_sensor;
                xSemaphoreGive(g_mutex);
            }

            /* Reset wake time to "now" so the next manual-mode
               delay is not immediately overdue after a long auto
               manoeuvre.                                           */
            wake = xTaskGetTickCount();
        }
    }
}

/* ================================================================
 * task_web  (Core 0 — Protocol CPU, priority 1)
 *
 * Responsibilities:
 *   • Broadcast robot_status_t JSON to all WebSocket clients
 *   • Periodically clean up disconnected clients
 * ================================================================ */
static void task_web(void *param)
{
    TickType_t   broadcast_wake  = xTaskGetTickCount();
    TickType_t   cleanup_wake    = xTaskGetTickCount();
    robot_status_t status;

    LOGF("[Web] Task started on core %d\n", xPortGetCoreID());

    for (;;) {
        uint32_t now = (uint32_t)xTaskGetTickCount();

        /* --- Broadcast sensor + status --- */
        if ((now - broadcast_wake) >= pdMS_TO_TICKS(PERIOD_BROADCAST_MS)) {
            broadcast_wake = now;

            if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                status.sensor   = g_state.sensor;
                status.mode     = g_state.mode;
                status.last_cmd = g_state.cmd;
                status.speed    = g_state.speed;
                status.obstacle = (g_state.sensor.dist_center < OBSTACLE_DIST_CM);
                status.uptime_ms = (uint32_t)millis();
                xSemaphoreGive(g_mutex);
            }

            wifi_manager_broadcast(g_wifi, &status);
        }

        /* --- Cleanup stale WebSocket clients --- */
        if ((now - cleanup_wake) >= pdMS_TO_TICKS(PERIOD_WS_CLEANUP_MS)) {
            cleanup_wake = now;
            wifi_manager_cleanup(g_wifi);
        }

        vTaskDelay(pdMS_TO_TICKS(20));   /* yield to other Core-0 tasks */
    }
}

/* ================================================================
 * setup()
 * Called once by the Arduino runtime on Core 1 before the
 * FreeRTOS scheduler takes over.
 * ================================================================ */
void setup(void)
{
    Serial.begin(115200);
    delay(400);

    LOGLN("\n================================================");
    LOGLN("  ESP32 WiFi Mobile Robot — Booting...");
    LOGLN("================================================");

    /* --- Initialise motor driver --- */
    motor_init(&g_motor,
               PIN_MOTOR_A_IN1, PIN_MOTOR_A_IN2, PIN_MOTOR_A_EN, LEDC_CHANNEL_A,
               PIN_MOTOR_B_IN3, PIN_MOTOR_B_IN4, PIN_MOTOR_B_EN, LEDC_CHANNEL_B);
    motor_set_speed(&g_motor, SPEED_DEFAULT);

    /* --- Initialise ultrasonic sensor + servo --- */
    sensor_init(&g_sensor, PIN_TRIG, PIN_ECHO, PIN_SERVO);

    /* --- Create shared mutex --- */
    g_mutex = xSemaphoreCreateMutex();
    if (g_mutex == NULL) {
        LOGLN("[FATAL] Failed to create mutex — rebooting");
        ESP.restart();
    }

    /* --- Initialise WiFi manager and register command callback --- */
    g_wifi = wifi_manager_create();
    if (g_wifi == NULL) {
        LOGLN("[FATAL] Failed to create WiFi manager — rebooting");
        ESP.restart();
    }
    wifi_manager_set_callback(g_wifi, on_ws_command);
    wifi_manager_begin(g_wifi);

    /* --- Create FreeRTOS tasks --- */

    /* Task: motion control + sensor reading (Core 1, priority 2) */
    BaseType_t ret_motion = xTaskCreatePinnedToCore(
        task_motion,
        "task_motion",
        TASK_MOTION_STACK,
        NULL,
        TASK_MOTION_PRIORITY,
        NULL,
        TASK_MOTION_CORE
    );

    /* Task: web server broadcast + cleanup (Core 0, priority 1) */
    BaseType_t ret_web = xTaskCreatePinnedToCore(
        task_web,
        "task_web",
        TASK_WEB_STACK,
        NULL,
        TASK_WEB_PRIORITY,
        NULL,
        TASK_WEB_CORE
    );

    if (ret_motion != pdPASS || ret_web != pdPASS) {
        LOGLN("[FATAL] Task creation failed — rebooting");
        ESP.restart();
    }

    LOGLN("[Setup] All tasks created successfully.");
    LOGLN("[Setup] System ready.\n");
    LOGLN("================================================\n");
}

/* ================================================================
 * loop()
 * Intentionally empty — all work is done inside FreeRTOS tasks.
 * The Arduino loop() runs on the idle task of Core 1; we yield
 * back immediately to avoid starving other tasks.
 * ================================================================ */
void loop(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
