#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "sensor.h"

uint power_pin,
     input_pin,
     reply_pin;

void sensor_init_pins(uint ppin, uint ipin, uint opin) {
    power_pin = ppin;
    input_pin = ipin;
    reply_pin = opin;

    gpio_init(power_pin);
    gpio_set_dir(power_pin, GPIO_OUT);
    gpio_pull_down(power_pin);

    gpio_init(input_pin);
    gpio_set_dir(input_pin, GPIO_OUT);
    gpio_pull_down(input_pin);

    gpio_init(reply_pin);
    gpio_set_dir(reply_pin, GPIO_IN);
    gpio_pull_down(reply_pin);
}

void sensor_toggle_power(bool state) {
    gpio_put(power_pin, state);
    sleep_ms(10); // Wait for the sensor to initialize/power down
}

int sensor_read(queue_t *in_q, queue_t *out_q) {
    bool request = true;
    int64_t sensor_response = -1;

    queue_add_blocking(in_q, &request);
    queue_remove_blocking(out_q, &sensor_response);

    if (sensor_response < 0) {
        return -1;
    }

    int64_t response = sensor_response / 58;
    return (int) response;
}

void sensor_trigger_echo() {
    gpio_put(input_pin, 1);
    sleep_us(11);
    gpio_put(input_pin, 0);
}

void sensor_measure_reply(absolute_time_t *start, absolute_time_t *end) {
    absolute_time_t max_timeout = make_timeout_time_us(100000);
    bool prev_state = false;
    bool current_state = false;

    while (absolute_time_diff_us(get_absolute_time(), max_timeout) > 0) {
        current_state = gpio_get(reply_pin);

        // No activity on reply pin
        if (!current_state && !prev_state) {
            sleep_us(1);
            continue;
        }

        // Rising edge detected on reply pin
        if (current_state && !prev_state) {
            *start = get_absolute_time();
            prev_state = current_state;
            sleep_us(1);
            continue;
        }

        // Falling edge detected on reply pin
        if (!current_state && prev_state) {
            *end = get_absolute_time();
            return;
        }

        // Reply pin is HIGH
        if (current_state && prev_state) {
            sleep_us(1);
        }
    }
}
