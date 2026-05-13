/* ================================================================
 * motor.c
 * Implementation of the L298N motor driver module.
 *
 * Hardware wiring assumed:
 *   IN1/IN2  → direction of Motor A (left  wheel)
 *   IN3/IN4  → direction of Motor B (right wheel)
 *   ENA      → LEDC PWM signal for Motor A enable
 *   ENB      → LEDC PWM signal for Motor B enable
 * ================================================================ */

#include "motor.h"
#include "config.h"

#include <Arduino.h>        /* pinMode, digitalWrite, ledcXxx   */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* ----------------------------------------------------------------
 * Internal helpers — static so they are invisible outside this TU
 * ---------------------------------------------------------------- */

/* Set direction and PWM for Motor A */
static void drive_a(const motor_handle_t *m, bool fwd, bool bwd, uint8_t pwm)
{
    digitalWrite(m->in1, fwd ? HIGH : LOW);
    digitalWrite(m->in2, bwd ? HIGH : LOW);
    ledcWrite(m->ch_a, pwm);
}

/* Set direction and PWM for Motor B */
static void drive_b(const motor_handle_t *m, bool fwd, bool bwd, uint8_t pwm)
{
    digitalWrite(m->in3, fwd ? HIGH : LOW);
    digitalWrite(m->in4, bwd ? HIGH : LOW);
    ledcWrite(m->ch_b, pwm);
}

/* ----------------------------------------------------------------
 * motor_init()
 * Configure GPIO pins and LEDC channels.
 * Returns 0 on success.
 * ---------------------------------------------------------------- */
int motor_init(motor_handle_t *m,
               uint8_t in1, uint8_t in2, uint8_t en_a, uint8_t ch_a,
               uint8_t in3, uint8_t in4, uint8_t en_b, uint8_t ch_b)
{
    if (m == NULL) return -1;

    /* Store pin / channel configuration */
    m->in1  = in1;  m->in2  = in2;  m->en_a = en_a;  m->ch_a = ch_a;
    m->in3  = in3;  m->in4  = in4;  m->en_b = en_b;  m->ch_b = ch_b;
    m->speed = SPEED_DEFAULT;

    /* Direction pins */
    pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
    pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);

    /* LEDC setup for Motor A */
    ledcSetup(ch_a, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(en_a, ch_a);

    /* LEDC setup for Motor B */
    ledcSetup(ch_b, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(en_b, ch_b);

    /* Start with motors stopped */
    motor_stop(m);

    LOGF("[Motor] Init OK  chA=%d chB=%d\n", ch_a, ch_b);
    return 0;
}

/* ----------------------------------------------------------------
 * motor_set_speed()
 * Clamp and store the desired speed.  The new value is applied on
 * the next direction command.
 * ---------------------------------------------------------------- */
void motor_set_speed(motor_handle_t *m, uint8_t speed)
{
    if (m == NULL) return;

    if (speed < SPEED_MIN) speed = SPEED_MIN;
    if (speed > SPEED_MAX) speed = SPEED_MAX;

    m->speed = speed;
    LOGF("[Motor] Speed set to %d\n", speed);
}

/* ----------------------------------------------------------------
 * motor_forward() — both wheels drive forward at m->speed
 * ---------------------------------------------------------------- */
void motor_forward(motor_handle_t *m)
{
    drive_a(m, true,  false, m->speed);
    drive_b(m, true,  false, m->speed);
}

/* ----------------------------------------------------------------
 * motor_backward() — both wheels drive backward at m->speed
 * ---------------------------------------------------------------- */
void motor_backward(motor_handle_t *m)
{
    drive_a(m, false, true,  m->speed);
    drive_b(m, false, true,  m->speed);
}

/* ----------------------------------------------------------------
 * motor_turn_left() — spin in-place: A backward, B forward
 * ---------------------------------------------------------------- */
void motor_turn_left(motor_handle_t *m)
{
    drive_a(m, false, true,  SPEED_TURN);
    drive_b(m, true,  false, SPEED_TURN);
}

/* ----------------------------------------------------------------
 * motor_turn_right() — spin in-place: A forward, B backward
 * ---------------------------------------------------------------- */
void motor_turn_right(motor_handle_t *m)
{
    drive_a(m, true,  false, SPEED_TURN);
    drive_b(m, false, true,  SPEED_TURN);
}

/* ----------------------------------------------------------------
 * motor_stop() — electric brake: all pins LOW, PWM = 0
 * ---------------------------------------------------------------- */
void motor_stop(motor_handle_t *m)
{
    drive_a(m, false, false, 0);
    drive_b(m, false, false, 0);
}

/* ----------------------------------------------------------------
 * motor_execute() — dispatch a move_cmd_t to the right function
 * ---------------------------------------------------------------- */
void motor_execute(motor_handle_t *m, move_cmd_t cmd)
{
    switch (cmd) {
        case CMD_FORWARD:  motor_forward(m);    break;
        case CMD_BACKWARD: motor_backward(m);   break;
        case CMD_LEFT:     motor_turn_left(m);  break;
        case CMD_RIGHT:    motor_turn_right(m); break;
        default:           motor_stop(m);       break;
    }
}

/* ----------------------------------------------------------------
 * motor_ramp_to()
 * Linearly ramp PWM from the current speed to target_speed over
 * duration_ms milliseconds using 20 equal steps.
 * Blocks the calling FreeRTOS task during the ramp.
 * ---------------------------------------------------------------- */
void motor_ramp_to(motor_handle_t *m, uint8_t target, uint16_t duration_ms)
{
    if (m == NULL) return;

    uint8_t  start     = m->speed;
    int16_t  delta     = (int16_t)target - (int16_t)start;
    uint8_t  steps     = 20;
    uint16_t step_delay = duration_ms / steps;
    uint8_t  i;

    if (delta == 0) return;

    for (i = 1; i <= steps; i++) {
        m->speed = (uint8_t)(start + (delta * i / steps));
        ledcWrite(m->ch_a, m->speed);
        ledcWrite(m->ch_b, m->speed);
        vTaskDelay(pdMS_TO_TICKS(step_delay));
    }

    m->speed = target;
}
