//
// Created by colin on 10/24/25.
//

#ifndef EDDYCLOCK_CLOCK_H
#define EDDYCLOCK_CLOCK_H

#include "button.h"
#include "rv3028.h"
#include "ssd1306.h"

class EddyClock {
public:
    explicit EddyClock(i2c_inst_t * i2c);
    ~EddyClock() = default;

    int run();

private:

    rv3028::rv3028_time_t getWakeupTime();
    bool setWakeupTime(uint16_t hours, uint16_t minutes);
    rv3028::rv3028_time_t getGotoSleepTime();
    bool setGotoSleepTime(uint16_t hours, uint16_t minutes);

    bool timeChanged(rv3028::rv3028_time_t t);
    int compareTime(rv3028::rv3028_time_t t1, rv3028::rv3028_time_t t2);

    rv3028 rv;
    rv3028::rv3028_time_t last_time;
    rv3028::rv3028_time_t wakeup_time;
    rv3028::rv3028_time_t gotosleep_time;

    button button_hours;
    button button_minutes;

    button button_wakeup;
    button button_sleep;

    SSD1306 oled;
};


#endif //EDDYCLOCK_CLOCK_H