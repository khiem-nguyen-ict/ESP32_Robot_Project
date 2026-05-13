#ifndef CONFIG_H
#define CONFIG_H

/* ================================================================
 * config.h
 * Central configuration file for the ESP32 WiFi Mobile Robot.
 * Edit values here without touching any logic code.
 * ================================================================ */

/* ----------------------------------------------------------------
 * WiFi credentials
 * ---------------------------------------------------------------- */
#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASSWORD     "YOUR_WIFI_PASSWORD"
#define WIFI_AP_SSID      "ESP32_Robot_AP"   /* fallback AP name  */
#define WIFI_AP_PASS      "robot1234"         /* fallback AP pass (>=8 chars) */
#define WIFI_TIMEOUT_MS   15000               /* connection timeout in ms */

/* ----------------------------------------------------------------
 * Motor driver (L298N) pin mapping
 * Motor A = LEFT wheel, Motor B = RIGHT wheel
 * ---------------------------------------------------------------- */
#define PIN_MOTOR_A_IN1   25
#define PIN_MOTOR_A_IN2   26
#define PIN_MOTOR_A_EN    27    /* PWM enable A */

#define PIN_MOTOR_B_IN3   14
#define PIN_MOTOR_B_IN4   12
#define PIN_MOTOR_B_EN    13    /* PWM enable B */

/* ----------------------------------------------------------------
 * Ultrasonic sensor HC-SR04 pin mapping
 * NOTE: ECHO outputs 5 V — use a 1kΩ/2kΩ voltage divider
 *       before connecting to GPIO18 (max 3.3 V).
 * ---------------------------------------------------------------- */
#define PIN_TRIG          5
#define PIN_ECHO          18

/* ----------------------------------------------------------------
 * Servo SG90 pin mapping
 * ---------------------------------------------------------------- */
#define PIN_SERVO         19

/* ----------------------------------------------------------------
 * LEDC (PWM) configuration for motor speed control
 * ---------------------------------------------------------------- */
#define PWM_FREQ          5000   /* Hz */
#define PWM_RESOLUTION    8      /* bits → range 0-255 */
#define LEDC_CHANNEL_A    0      /* LEDC channel for motor A */
#define LEDC_CHANNEL_B    1      /* LEDC channel for motor B */

/* ----------------------------------------------------------------
 * Motor speed constants (0-255)
 * ---------------------------------------------------------------- */
#define SPEED_DEFAULT     180
#define SPEED_TURN        150
#define SPEED_MIN         80
#define SPEED_MAX         255

/* ----------------------------------------------------------------
 * Ultrasonic sensor parameters
 * ---------------------------------------------------------------- */
#define OBSTACLE_DIST_CM  30      /* detection threshold in cm   */
#define SENSOR_NO_ECHO    999.0f  /* returned when echo times out */
#define ECHO_TIMEOUT_US   25000   /* ~425 cm max range           */
#define SENSOR_SAMPLES    3       /* samples per median reading  */

/* ----------------------------------------------------------------
 * Servo scan angles (degrees)
 * ---------------------------------------------------------------- */
#define SERVO_LEFT        45
#define SERVO_CENTER      90
#define SERVO_RIGHT       135
#define SERVO_SETTLE_MS   350     /* wait for servo to reach position */

/* ----------------------------------------------------------------
 * FreeRTOS task configuration
 * ---------------------------------------------------------------- */
#define TASK_MOTION_STACK     4096
#define TASK_MOTION_PRIORITY  2
#define TASK_MOTION_CORE      1   /* App CPU  */

#define TASK_WEB_STACK        8192
#define TASK_WEB_PRIORITY     1
#define TASK_WEB_CORE         0   /* Protocol CPU */

/* ----------------------------------------------------------------
 * Timing intervals (ms)
 * ---------------------------------------------------------------- */
#define PERIOD_MOTION_MS      100   /* motion task loop period   */
#define PERIOD_BROADCAST_MS   250   /* WebSocket broadcast rate  */
#define PERIOD_WS_CLEANUP_MS  5000  /* stale client cleanup rate */

/* Auto-mode manoeuvre durations (ms) */
#define AUTO_FWD_MS           200
#define AUTO_TURN_MS          450
#define AUTO_BACK_MS          500
#define AUTO_SPIN_MS          750

/* ----------------------------------------------------------------
 * Web server
 * ---------------------------------------------------------------- */
#define WEB_SERVER_PORT   80

/* ----------------------------------------------------------------
 * Debug logging macros
 * Set DEBUG_ENABLED to 0 to strip all log output from the binary.
 * ---------------------------------------------------------------- */
#define DEBUG_ENABLED  1

#if DEBUG_ENABLED
  #define LOG(x)     Serial.print(x)
  #define LOGLN(x)   Serial.println(x)
  #define LOGF(...)  Serial.printf(__VA_ARGS__)
#else
  #define LOG(x)
  #define LOGLN(x)
  #define LOGF(...)
#endif

#endif /* CONFIG_H */
