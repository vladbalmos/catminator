/**
 * Copyright (c) 2022 Vlad Balmos
 *
 * SPDX-License-Identifier: MIT License
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/xosc.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"
#include "utils.h"
#include "sensor.h"
#include "motor.h"
#include "battery.h"

#define COOL_OFF_PERIOD 10 * 1000 // seconds
#define MOTOR_DRIVE_DELAY 5000
#define DEFAULT_LOOP_SLEEP 1000
#define MAX_TARGET_DISTANCE 50 // cm

// Pins
#define SENSOR_POWER_PIN 13
#define SENSOR_INPUT_PIN 14
#define SENSOR_REPLY_PIN 15
#define BTN_CANCEL_TRIGGER_PIN 16
#define MOTOR_UP_PIN 17
#define MOTOR_DOWN_PIN 18
#define STATUS_LED_PIN 19
#define BOOST_5V_ENABLE_PIN 11

queue_t sensor_request_q;
queue_t sensor_response_q;
absolute_time_t cool_off;
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

bool is_cooling_off() {
    return cool_off != nil_time && absolute_time_diff_us(get_absolute_time(), cool_off) > 0;
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_CANCEL_TRIGGER_PIN && !is_cooling_off()) {
        clear_motor_drive_alarm = true;
        return;
    }
}

void measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("------------------- FREQ-------------\n");
    printf("\tll_sys  = %dkHz\n", f_pll_sys);
    printf("\tpll_usb  = %dkHz\n", f_pll_usb);
    printf("\trosc     = %dkHz\n", f_rosc);
    printf("\tclk_sys  = %dkHz\n", f_clk_sys);
    printf("\tclk_peri = %dkHz\n", f_clk_peri);
    printf("\tclk_usb  = %dkHz\n", f_clk_usb);
    printf("\tclk_adc  = %dkHz\n", f_clk_adc);
    printf("\tclk_rtc  = %dkHz\n", f_clk_rtc);
    printf("------------------- END FREQ---------\n");
}

void stop() {
    gpio_put(BOOST_5V_ENABLE_PIN, 0);
    gpio_put(SENSOR_POWER_PIN, 0);
    if (debug_mode()) {
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
    }

    clock_configure(clk_ref,
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH,
                    0,
                    1 * MHZ,
                    1 * MHZ);
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,
                    0,
                    1 * MHZ,
                    1 * MHZ);
    // Stop clocks
    xosc_disable();
    clock_stop(clk_peri);
    clock_stop(clk_usb);
    clock_stop(clk_adc);
    clock_stop(clk_rtc);
    pll_deinit(pll_sys);
    pll_deinit(pll_usb);
    clock_stop(clk_sys);
    clock_stop(clk_ref);
    panic("Stoping\n");
}

int main() {
    bool cool_off_after_motor_drive = false;

    stdio_init_all();

    if (!debug_mode()) {
        set_sys_clock_pll(756 * MHZ, 7, 6);
        clock_stop(clk_peri);
        clock_stop(clk_usb);
    }

    init_battery_readings();

    queue_init(&sensor_request_q, sizeof(bool), 1);
    queue_init(&sensor_response_q, sizeof(int64_t), 1);

    bool default_led_state = true;
    if (debug_mode()) {
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    }

    // GPIO init
    gpio_init(BTN_CANCEL_TRIGGER_PIN);
    gpio_set_dir(BTN_CANCEL_TRIGGER_PIN, GPIO_IN);

    gpio_init(BOOST_5V_ENABLE_PIN);
    gpio_set_dir(BOOST_5V_ENABLE_PIN, GPIO_OUT);
    gpio_put(BOOST_5V_ENABLE_PIN, 1);

    sensor_init_pins(SENSOR_POWER_PIN, SENSOR_INPUT_PIN, SENSOR_REPLY_PIN);
    motor_init_pins(MOTOR_UP_PIN, MOTOR_DOWN_PIN);
    status_led_init(STATUS_LED_PIN);

    // Enable irq
    gpio_set_irq_enabled_with_callback(BTN_CANCEL_TRIGGER_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    sleep_ms(5);
    multicore_launch_core1(core1_entry);
    sleep_ms(10); // Wait a bit for the core to initialize

    DEBUG("Initialized\n");
    cool_off = nil_time;

    while (true) {
        if (cool_off_after_motor_drive && !motor_is_running()) {
            cool_off = make_timeout_time_ms(COOL_OFF_PERIOD);
            cool_off_after_motor_drive = false;
            DEBUG("Cooling off after motor drive\n");
        }

        if (debug_mode()) {
            measure_freqs();
        }

        if (low_battery()) {
            DEBUG("Please recharge battery\n");
            stop();
        }

        if (debug_mode()) {
            gpio_put(PICO_DEFAULT_LED_PIN, default_led_state);
            default_led_state = !default_led_state;
        }
        if (is_cooling_off()) {
            DEBUG("Cooling off\n");
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }

        cool_off = nil_time;

        if (clear_motor_drive_alarm && motor_is_drive_scheduled()) {
            DEBUG("Canceling drive requested\n");
            clear_motor_drive_alarm = false;
            motor_cancel_drive();
            cool_off = make_timeout_time_ms(COOL_OFF_PERIOD);
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }


        if (queue_is_full(&sensor_request_q)) {
            DEBUG("Request queue is full. Sleeping\n");
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }

        if (motor_is_running()) {
            sleep_ms(DEFAULT_LOOP_SLEEP);
            cool_off_after_motor_drive = true;
            continue;
        }

        sensor_toggle_power(true);
        int distance = sensor_read(&sensor_request_q, &sensor_response_q);
        sensor_toggle_power(false);
        DEBUG("Target distance: %dcm \n", distance);

        if (distance < 0) {
            sleep_ms(DEFAULT_LOOP_SLEEP);
            continue;
        }

        if (distance < MAX_TARGET_DISTANCE && !motor_is_drive_scheduled()) {
            motor_schedule_drive(MOTOR_DRIVE_DELAY);
            DEBUG("Target aquired. Scheduling motor drive\n");
        } else if (distance >= MAX_TARGET_DISTANCE)  {
            if (motor_is_drive_scheduled()) {
                DEBUG("Target lost. Canceling motor drive\n");
                motor_cancel_drive();
            }
        }
        sleep_ms(DEFAULT_LOOP_SLEEP);
    }

    return 0;
}
