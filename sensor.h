/**
 * Copyright (c) 2022 Vlad Balmos
 *
 * SPDX-License-Identifier: MIT License
 */

void sensor_init_pins(uint power_pin, uint input_pin, uint reply_pin);
void sensor_toggle_power(bool state);
int sensor_read(queue_t *in_q, queue_t *out_q);
void sensor_trigger_echo();
void sensor_measure_reply(absolute_time_t *start, absolute_time_t *end);
