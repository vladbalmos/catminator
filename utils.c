#include "pico/stdlib.h"
#include "utils.h"

uint led_pin;
bool led_busy = true;

void status_led_init(uint lpin) {
    led_pin = lpin;

    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_pull_down(led_pin);
}

bool status_led_is_busy() {
    return led_busy;
}

void status_led_show_countdown(uint timeout) {
}

void status_led_stop_countdown() {
}
