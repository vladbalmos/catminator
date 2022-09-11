/**
 * Copyright (c) 2022 Vlad Balmos
 *
 * SPDX-License-Identifier: MIT License
 */

#include "hardware/adc.h"
#include "battery.h"
#include "utils.h"

#define ADC_CONVERSION_FACTOR 3.3f / (1 << 12)

#define ADC_PIN 26
#define ADC_INPUT 0
#define LOW_VOLTAGE_THRESHOLD 3.2

void init_battery_readings() {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_INPUT);
    adc_set_clkdiv(9600); // 5khz
}

bool low_battery() {
    uint16_t result = adc_read();
    float voltage = result * ADC_CONVERSION_FACTOR * 2 + .15; // .15 accounts for resistor divider inaccuracy
    DEBUG("Battery voltage: %f V\n", voltage);
    return voltage < LOW_VOLTAGE_THRESHOLD;
}

