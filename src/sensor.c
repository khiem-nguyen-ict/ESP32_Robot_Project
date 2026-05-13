/* ================================================================
 * sensor.c
 * Implementation of HC-SR04 ultrasonic sensor + SG90 servo module.
 *
 * The ESP32Servo library uses C++ internally; we wrap its object
 * behind a void* so the rest of the project stays pure C.
 * ================================================================ */

#include "sensor.h"
#include "config.h"

#include <Arduino.h>
#include <ESP32Servo.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>   /* malloc / free  */
#include <string.h>   /* memset         */

/* ----------------------------------------------------------------
 * Internal: cast the opaque pointer back to Servo*
 * ---------------------------------------------------------------- */
#define SERVO(s)  ((Servo *)(s)->servo_obj)

/* ----------------------------------------------------------------
 * Internal: sort a float array in-place (bubble sort, n is small)
 * ---------------------------------------------------------------- */
static void sort_floats(float *arr, uint8_t n)
{
    uint8_t i, j;
    float   tmp;
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                tmp        = arr[j];
                arr[j]     = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

/* ================================================================
 * sensor_init()
 * Configure TRIG/ECHO GPIO, allocate a Servo object and attach it.
 * ================================================================ */
int sensor_init(sensor_handle_t *s,
                uint8_t trig_pin, uint8_t echo_pin, uint8_t servo_pin)
{
    if (s == NULL) return -1;

    s->trig_pin  = trig_pin;
    s->echo_pin  = echo_pin;
    s->servo_pin = servo_pin;

    /* Configure HC-SR04 GPIO */
    pinMode(trig_pin, OUTPUT);
    pinMode(echo_pin, INPUT);
    digitalWrite(trig_pin, LOW);

    /* Allocate a Servo object on the heap so this file can stay
       compilable as C (the new/delete operators live in sensor.c
       only — nowhere else in the project). */
    Servo *srv = new Servo();
    if (srv == NULL) {
        LOGLN("[Sensor] ERROR: failed to allocate Servo object");
        return -1;
    }
    s->servo_obj = (void *)srv;

    /* ESP32Servo: allocate a hardware timer for the servo signal.
       Timer 2 is used to avoid conflicts with LEDC channels 0/1. */
    ESP32PWM::allocateTimer(2);
    srv->setPeriodHertz(50);                   /* SG90 uses 50 Hz */
    srv->attach(servo_pin, 500, 2400);         /* 500-2400 µs pulse width */

    /* Centre the servo and wait for it to settle */
    sensor_servo_center(s);
    vTaskDelay(pdMS_TO_TICKS(400));

    LOGF("[Sensor] Init OK  trig=%d echo=%d servo=%d\n",
         trig_pin, echo_pin, servo_pin);
    return 0;
}

/* ================================================================
 * sensor_deinit()
 * Detach servo and free heap memory.
 * ================================================================ */
void sensor_deinit(sensor_handle_t *s)
{
    if (s == NULL || s->servo_obj == NULL) return;
    SERVO(s)->detach();
    delete SERVO(s);
    s->servo_obj = NULL;
}

/* ================================================================
 * sensor_measure_once()
 * Fire one 10-µs TRIG pulse and measure the ECHO duration.
 * Returns distance in cm, or SENSOR_NO_ECHO on timeout.
 * ================================================================ */
float sensor_measure_once(const sensor_handle_t *s)
{
    long duration;

    /* Ensure TRIG is low before the pulse */
    digitalWrite(s->trig_pin, LOW);
    delayMicroseconds(2);

    /* 10-µs high pulse triggers the measurement */
    digitalWrite(s->trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(s->trig_pin, LOW);

    /* Measure ECHO pulse width; pulseIn returns 0 on timeout */
    duration = pulseIn(s->echo_pin, HIGH, ECHO_TIMEOUT_US);

    if (duration == 0) return SENSOR_NO_ECHO;

    /* distance = (duration_µs / 2) × 0.0343 cm/µs */
    return (float)duration * 0.01715f;
}

/* ================================================================
 * sensor_measure()
 * Take 'samples' readings, sort them and return the median.
 * A short delay between readings prevents cross-echo artefacts.
 * ================================================================ */
float sensor_measure(const sensor_handle_t *s, uint8_t samples)
{
    float   buf[8];
    uint8_t i;

    if (samples < 1) samples = 1;
    if (samples > 8) samples = 8;

    if (samples == 1) return sensor_measure_once(s);

    for (i = 0; i < samples; i++) {
        buf[i] = sensor_measure_once(s);
        vTaskDelay(pdMS_TO_TICKS(15));   /* avoid echo crosstalk */
    }

    sort_floats(buf, samples);
    return buf[samples / 2];   /* median */
}

/* ================================================================
 * sensor_scan_all()
 * Sweep the servo left → center → right, measure distance at each
 * position, then return the servo to center.
 * This function blocks the calling FreeRTOS task.
 * ================================================================ */
void sensor_scan_all(sensor_handle_t *s, sensor_data_t *out)
{
    if (s == NULL || out == NULL) return;

    /* --- Sweep LEFT (45°) --- */
    sensor_servo_left(s);
    vTaskDelay(pdMS_TO_TICKS(SERVO_SETTLE_MS));
    out->dist_left = sensor_measure(s, SENSOR_SAMPLES);

    /* --- Sweep CENTER (90°) --- */
    sensor_servo_center(s);
    vTaskDelay(pdMS_TO_TICKS(SERVO_SETTLE_MS));
    out->dist_center = sensor_measure(s, SENSOR_SAMPLES);

    /* --- Sweep RIGHT (135°) --- */
    sensor_servo_right(s);
    vTaskDelay(pdMS_TO_TICKS(SERVO_SETTLE_MS));
    out->dist_right = sensor_measure(s, SENSOR_SAMPLES);

    /* Return servo to center */
    sensor_servo_center(s);

    out->timestamp_ms = (uint32_t)millis();

    LOGF("[Sensor] L:%.1f  C:%.1f  R:%.1f cm\n",
         out->dist_left, out->dist_center, out->dist_right);
}

/* ================================================================
 * Servo position helpers
 * ================================================================ */
void sensor_servo_angle(sensor_handle_t *s, uint8_t angle)
{
    if (s == NULL || s->servo_obj == NULL) return;
    if (angle > 180) angle = 180;
    SERVO(s)->write(angle);
}

void sensor_servo_center(sensor_handle_t *s) { sensor_servo_angle(s, SERVO_CENTER); }
void sensor_servo_left  (sensor_handle_t *s) { sensor_servo_angle(s, SERVO_LEFT);   }
void sensor_servo_right (sensor_handle_t *s) { sensor_servo_angle(s, SERVO_RIGHT);  }
