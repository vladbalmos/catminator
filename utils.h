/**
 * Copyright (c) 2022 Vlad Balmos
 *
 * SPDX-License-Identifier: MIT License
 */

#ifdef DEBUG_MODE
    #define DEBUG(...) printf(__VA_ARGS__);
#else
    #define DEBUG(...) do {} while (0)
#endif
void status_led_init(uint lpin);
void status_led_toggle(bool state);

inline bool debug_mode() {
#ifdef DEBUG_MODE
    return true;
#else
    return false;
#endif
}
