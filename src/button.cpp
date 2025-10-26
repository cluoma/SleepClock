//
// Created by colin on 10/24/25.
//

#include "button.h"

/**
 * the_button.c
 *
 * Handle button input for the 'front button' of pico2maple.
 * Does debounce, and triggers short press or long hold events.
 * Short press: button is pressed and released before the time limit.
 * Long hold: button is pressed and held until the time limit.
 *
 * Copyright (c) 2025 Colin Luoma
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define DEBOUNCE_MS 30
#define PRESSED_PIN_LEVEL 0  // This means button pressed results in pin going high

button::button(uint8_t pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);

    if (PRESSED_PIN_LEVEL == 1)
        gpio_pull_down(pin);  // assumes active-high button
    else
        gpio_pull_up(pin);


    _pin = pin;
    _state = IDLE;
    _action = NONE;
    _last_gpio_level = !static_cast<bool>(PRESSED_PIN_LEVEL);
}

button::State button::update() {
    bool level = gpio_get(_pin);

    switch (_state) {
        case IDLE:
            if (level == PRESSED_PIN_LEVEL && _last_gpio_level != PRESSED_PIN_LEVEL)
            {
                // Initial button press
                _last_change = get_absolute_time();
                _state = DEBOUNCE;
            }
            break;

        case DEBOUNCE:
            if (absolute_time_diff_us(_last_change, get_absolute_time()) > DEBOUNCE_MS)
            {
                if (level == PRESSED_PIN_LEVEL)
                {
                    // Confirmed press
                    _state = PRESSED;
                    _action = PRESS;
                }
                else
                {
                    // Bounce, return to idle
                    _state = IDLE;
                }
            }
            break;

        case PRESSED:
            if (level != PRESSED_PIN_LEVEL && _last_gpio_level == PRESSED_PIN_LEVEL) // button released
            {
                _state = IDLE;
                _action = RELEASE;
            }
            // still being held
            break;
    }
    _last_gpio_level = level;

    return _state;
}

button::Action button::pollAction()
{
    const Action ret = _action;
    _action = NONE;
    return ret;
}