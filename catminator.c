/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "sensor.h"
#include "motor.h"

const uint COOL_OFF_PERIOD = 10 * 1000; // ms
const uint MOTOR_DRIVE_DELAY = 5000;
const uint DEFAULT_LOOP_SLEEP = 1000;
const uint MAX_TARGET_DISTANCE = 50; // cm

// Pins
const uint BTN_CANCEL_TRIGGER_PIN = 16;
const uint SENSOR_POWER_PIN = 13;
const uint SENSOR_INPUT_PIN = 14;
const uint SENSOR_REPLY_PIN = 15;
const uint MOTOR_UP_PIN = 17;
const uint MOTOR_DOWN_PIN = 18;
const uint STATUS_LED_PIN = 19;

queue_t sensor_request_q;
queue_t sensor_response_q;
bool clear_motor_drive_alarm = false;


void core1_entry() {
    while (true) {
        bool request;
        int64_t response = -1;

        queue_remove_blocking(&sensor_request_q, &request);

        if (!request) {
            sleep_ms(100);
            continue;
        }

        absolute_time_t start = nil_time;
        absolute_time_t end = nil_time;

        sensor_trigger_echo();
        sensor_measure_reply(&start, &end);

        if (start != nil_time && end != nil_time) {
            response = absolute_time_diff_us(start, end);
        }
        queue_add_blocking(&sensor_response_q, &response);
    }
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_CANCEL_TRIGGER_PIN) {
        clear_motor_drive_alarm = true;
        return;
    }
}

int main() {
    stdio_init_all();

    queue_init(&sensor_request_q, sizeof(bool), 1);
    queue_init(&sensor_response_q, sizeof(int64_t), 1);

    // GPIO init
    gpio_init(BTN_CANCEL_TRIGGER_PIN);
    gpio_set_dir(BTN_CANCEL_TRIGGER_PIN, GPIO_IN);

    sensor_init_pins(SENSOR_POWER_PIN, SENSOR_INPUT_PIN, SENSOR_REPLY_PIN);
    motor_init_pins(MOTOR_UP_PIN, MOTOR_DOWN_PIN);
    status_led_init(STATUS_LED_PIN);

    // Enable irq
    gpio_set_irq_enabled_with_callback(BTN_CANCEL_TRIGGER_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    sleep_ms(5);
    multicore_launch_core1(core1_entry);
    sleep_ms(10); // Wait a bit for the core to initialize

    printf("Initialized\n");
    absolute_time_t cool_off = nil_time;

    while (true) {
        if (cool_off != nil_time && absolute_time_diff_us(get_absolute_time(), cool_off) > 0) {
            printf("Cooling off\n");
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }

        if (clear_motor_drive_alarm && motor_is_drive_scheduled()) {
            printf("Canceling drive requested\n");
            clear_motor_drive_alarm = false;
            motor_cancel_drive();
            cool_off = make_timeout_time_ms(COOL_OFF_PERIOD);
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }


        if (queue_is_full(&sensor_request_q)) {
            printf("Request queue is full. Sleeping\n");
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }

        if (motor_is_running()) {
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }

        sensor_toggle_power(true);
        int distance = sensor_read(&sensor_request_q, &sensor_response_q);
        sensor_toggle_power(false);
        printf("Received response: %d \n", distance);

        if (distance < 0) {
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }

        if (distance < MAX_TARGET_DISTANCE && !motor_is_drive_scheduled()) {
            motor_schedule_drive(MOTOR_DRIVE_DELAY);
            printf("Target aquired. Scheduling motor drive\n");
        } else if (distance >= MAX_TARGET_DISTANCE)  {
            if (motor_is_drive_scheduled()) {
                printf("Target lost. Canceling motor drive\n");
                motor_cancel_drive();
            }
        }
        sleep_ms(DEFAULT_LOOP_SLEEP);
    }

    return 0;
}
