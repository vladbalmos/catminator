#include <stdio.h>
#include "pico/stdlib.h"
#include "motor.h"

const uint MOTOR_UP_PIN = 17;
const uint MOTOR_DOWN_PIN = 18;
const uint DEFAULT_LED_PIN = 19;

volatile bool motor_drive_scheduled = false;
volatile alarm_id_t motor_drive_up_alarm = 0;

typedef struct {
    uint pin;
} motor_state;

int64_t stop_drive_motor(alarm_id_t id, void *user_data) {
    motor_state *state = (motor_state *) user_data;
    gpio_put(state->pin, 0);

    printf("%d\n", state->pin);
    if (state->pin == MOTOR_UP_PIN) {
        printf("Starting new alarm for down\n");
        state->pin = MOTOR_DOWN_PIN;
        add_alarm_in_ms(500, start_drive_motor, (void *)state, false);
        return 0;
    }
    free(state);
    state = NULL;
    motor_drive_scheduled = false;
    return 0;
}

int64_t start_drive_motor(alarm_id_t id, void *user_data) {
    motor_state *state = (motor_state *) user_data;
    gpio_put(state->pin, 1);
    printf("Driving %d\n", state->pin);
    add_alarm_in_ms(1000, stop_drive_motor, user_data, false);
    return 0;
}

bool motor_is_drive_scheduled() {
    printf("Motor drive scheduled %d\n", motor_drive_scheduled);
    return motor_drive_scheduled;
}

void motor_schedule_drive(uint32_t delay) {
    if (motor_drive_scheduled) {
        return;
    }

    motor_state *state = malloc(sizeof(motor_state));
    state->pin = MOTOR_UP_PIN;
    motor_drive_scheduled = true;
    motor_drive_up_alarm = add_alarm_in_ms(delay, start_drive_motor, (void *)state, false);
    /*motor_drive_up_alarm = add_alarm_in_ms(delay, start_drive_motor, NULL, false);*/

    if (motor_drive_up_alarm == -1) {
        // TODO: flash error LED
        printf("No alarm slot available for driving the motor");
    }
    printf("Motor drive alarm %d\n", motor_drive_up_alarm);
}

void motor_cancel_drive() {
    if (!motor_drive_up_alarm) {
        return;
    }
    cancel_alarm(motor_drive_up_alarm);
    motor_drive_up_alarm = 0;
    motor_drive_scheduled = false;
}
