/**
 * Copyright (c) 2022 Vlad Balmos
 *
 * SPDX-License-Identifier: MIT License
 */

#include "pico/stdlib.h"
#include "utils.h"

uint led_pin;

void status_led_init(uint lpin) {
    led_pin = lpin;

    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_pull_down(led_pin);
}

void status_led_toggle(bool state) {
    gpio_put(led_pin, state);
}

