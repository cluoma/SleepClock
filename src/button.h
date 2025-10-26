//
// Created by colin on 10/24/25.
//

#ifndef EDDYCLOCK_BUTTON_H
#define EDDYCLOCK_BUTTON_H
#include <cstdint>

#include "pico/types.h"

class button {
public:
    enum State {
        IDLE,
        DEBOUNCE,
        PRESSED
     };

    enum Action {
        NONE,
        PRESS,
        RELEASE
     };

    button(uint8_t pin);
    ~button() = default;

    State update();
    Action pollAction();


private:
    uint8_t _pin;
    State _state;
    Action _action;
    bool _last_gpio_level;
    absolute_time_t _last_change;
    absolute_time_t _press_start;
};


#endif //EDDYCLOCK_BUTTON_H