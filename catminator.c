/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"

const uint TRIGGER_BTN_PIN = 16;
const uint LED_PIN = 17;
const uint CANCEL_TRIGGER_BTN_PIN = 18;

const uint32_t MOTOR_DRIVE_DELAY = 2000;
const uint DEBOUNCE_DELAY = 250;

volatile uint32_t time;
volatile alarm_id_t motor_drive_alarm;
volatile bool motor_drive_scheduled = false;

int64_t drive_motor(alarm_id_t id, void *user_data) {
    gpio_put(LED_PIN, 1);
    motor_drive_scheduled = false;
    printf("Motor was driven\n");
    return 0;
}

bool valid_trigger() {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint32_t diff = now - time;

    if (diff < DEBOUNCE_DELAY) {
        return false;
    }

    time = now;
    return true;
}

void schedule_motor_drive() {
    if (!valid_trigger()) {
        return;
    }

    if (motor_drive_scheduled) {
        printf("Motor already scheduled\n");
        return;
    }

    printf("Scheduling motor drive\n");
    motor_drive_scheduled = true;
    motor_drive_alarm = add_alarm_in_ms(MOTOR_DRIVE_DELAY, drive_motor, NULL, false);

    if (motor_drive_alarm == -1) {
        // TODO: flash error LED
        printf("No alarm slot available for driving the motor");
    }
}

void cancel_motor_drive() {
    if (!valid_trigger() || !motor_drive_alarm) {
        return;
    }

    printf("Cancel motor drive %d\n", motor_drive_alarm);
    cancel_alarm(motor_drive_alarm);
    motor_drive_alarm = 0;
    motor_drive_scheduled = false;
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == TRIGGER_BTN_PIN) {
        schedule_motor_drive();
        return;
    }

    if (gpio == CANCEL_TRIGGER_BTN_PIN) {
        cancel_motor_drive();
        return;
    }
}

int main() {
    time = to_ms_since_boot(get_absolute_time());
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_drive_strength(LED_PIN, GPIO_DRIVE_STRENGTH_2MA);

    gpio_init(TRIGGER_BTN_PIN);
    gpio_set_dir(TRIGGER_BTN_PIN, GPIO_IN);
    gpio_pull_down(TRIGGER_BTN_PIN);

    gpio_init(CANCEL_TRIGGER_BTN_PIN);
    gpio_set_dir(CANCEL_TRIGGER_BTN_PIN, GPIO_IN);
    gpio_pull_down(CANCEL_TRIGGER_BTN_PIN);

    gpio_set_irq_enabled_with_callback(TRIGGER_BTN_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(CANCEL_TRIGGER_BTN_PIN, GPIO_IRQ_EDGE_RISE, true);

    printf("Starting\n");
    while (1);

    return 0;
}
