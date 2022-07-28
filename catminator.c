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

const uint DEBOUNCE_DELAY = 250;

// Pins
const uint LED_PIN = 17;
const uint BTN_CANCEL_TRIGGER_PIN = 20;
const uint SENSOR_POWER_PIN = 13;
const uint SENSOR_INPUT_PIN = 14;
const uint SENSOR_REPLY_PIN = 15;
volatile uint32_t time;

queue_t sensor_request_q;
queue_t sensor_response_q;


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

// Debounce button inputs
bool valid_trigger() {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint32_t diff = now - time;

    if (diff < DEBOUNCE_DELAY) {
        return false;
    }

    time = now;
    return true;
}

int main() {
    stdio_init_all();

    time = to_ms_since_boot(get_absolute_time());
    queue_init(&sensor_request_q, sizeof(bool), 1);
    queue_init(&sensor_response_q, sizeof(int64_t), 1);

    // GPIO init
    sensor_init_pins(SENSOR_POWER_PIN, SENSOR_INPUT_PIN, SENSOR_REPLY_PIN);

    sleep_ms(5);
    multicore_launch_core1(core1_entry);
    sleep_ms(10); // Wait a bit for the core to initialize

    printf("Initialized\n");

    while (true) {
        if (queue_is_full(&sensor_request_q)) {
            printf("Request queue is full. Sleeping\n");
            sleep_ms(1000);
            continue;
        }

        sensor_toggle_power(true);
        int distance = sensor_read(&sensor_request_q, &sensor_response_q);
        printf("Received response: %d \n", distance);
        sensor_toggle_power(false);
        sleep_ms(1000);
    }

    return 0;
}
