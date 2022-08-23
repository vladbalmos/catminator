#include <stdlib.h>
#include "pico/stdlib.h"
#include "motor.h"
#include "utils.h"

#define MOTOR_DRIVE_DURATION 690
#define MOTOR_DRIVE_REVERSAL_DURATION 200
#define MOTOR_PAUSE_BEFORE_REVERSAL 200

typedef struct {
    uint pin;
    uint drive_duration;
} motor_state;

bool motor_drive_scheduled = false;
bool motor_running = false;
uint up_pin;
uint down_pin;
alarm_id_t motor_drive_up_alarm = 0;
motor_state* mstate;

void motor_init_pins(uint upin, uint dpin) {
    up_pin = upin;
    down_pin = dpin;

    gpio_init(up_pin);
    gpio_set_dir(up_pin, GPIO_OUT);
    gpio_pull_down(up_pin);

    gpio_init(down_pin);
    gpio_set_dir(down_pin, GPIO_OUT);
    gpio_pull_down(down_pin);
}

int64_t start_drive_motor(alarm_id_t id, void *user_data) {
    if (!motor_running) {
        motor_running = true;
        status_led_toggle(true);
    }
    motor_state *state = (motor_state *) user_data;

    DEBUG("Driving motor on PIN %d\n", state->pin);
    gpio_put(state->pin, 1);
    add_alarm_in_ms(state->drive_duration, stop_drive_motor, user_data, false);

    return 0;
}

int64_t stop_drive_motor(alarm_id_t id, void *user_data) {
    motor_state *state = (motor_state *) user_data;
    gpio_put(state->pin, 0);

    if (state->pin == up_pin) {
        state->pin = down_pin;
        state->drive_duration = MOTOR_DRIVE_REVERSAL_DURATION;
        add_alarm_in_ms(MOTOR_PAUSE_BEFORE_REVERSAL, start_drive_motor, (void *)state, false);
        return 0;
    }
    free(mstate);
    mstate = NULL;
    motor_drive_scheduled = false;
    motor_running = false;
    status_led_toggle(false);
    return 0;
}

bool motor_is_drive_scheduled() {
    return motor_drive_scheduled;
}

void motor_schedule_drive(uint delay) {
    if (motor_drive_scheduled) {
        return;
    }

    mstate = malloc(sizeof(motor_state));
    mstate->pin = up_pin;
    mstate->drive_duration = MOTOR_DRIVE_DURATION;
    motor_drive_scheduled = true;
    motor_drive_up_alarm = add_alarm_in_ms(delay, start_drive_motor, (void *)mstate, false);

    if (motor_drive_up_alarm == -1) {
        // TODO: flash error LED
        return;
    }
    status_led_toggle(true);
}

void motor_cancel_drive() {
    if (!motor_drive_up_alarm) {
        return;
    }

    free(mstate);
    mstate = NULL;
    cancel_alarm(motor_drive_up_alarm);
    motor_drive_up_alarm = 0;
    motor_drive_scheduled = false;
    status_led_toggle(false);
}

bool motor_is_running() {
    return motor_running;
}
