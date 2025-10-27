//
// Created by colin on 10/24/25.
//

#include "EddyClock.h"

#include <chrono>
#include <cstdio>

#include "rv3028.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#define WAKEUP_HOURS_REGISTER 0x00
#define WAKEUP_MINUTES_REGISTER 0x01
#define GOTOSLEEP_HOURS_REGISTER 0x02
#define GOTOSLEEP_MINUTES_REGISTER 0x03

EddyClock::EddyClock(i2c_inst_t * i2c) :
    rv(i2c),
    button_hours(9),
    button_minutes(10),
    button_wakeup(11),
    button_sleep(12),
    oled(false)
{
    auto t = rv.getTime();
    last_time = t;
    wakeup_time = getWakeupTime();
    wakeup_time.seconds = 0;
    gotosleep_time = getGotoSleepTime();
    gotosleep_time.seconds = 0;
    oled.renderTime(t.hours, t.minutes);
    is_wakeup_time = true;
}

int EddyClock::compareTime(rv3028::rv3028_time_t t1, rv3028::rv3028_time_t t2)
{
    if (t1.hours > t2.hours ||
        (t1.hours == t2.hours && t1.minutes > t2.minutes) )
        return 1;

    if (t1.hours == t2.hours && t1.minutes == t2.minutes)
        return 0;

    return -1;
}

uint16_t timeToMinutes(rv3028::rv3028_time_t t)
{
    return t.hours * 60 + t.minutes;
}

bool isWakeupTime(rv3028::rv3028_time_t time, rv3028::rv3028_time_t wakeupTime, rv3028::rv3028_time_t gotoSleepTime)
{
    auto time_m = timeToMinutes(time);
    auto wakeupTime_m = timeToMinutes(wakeupTime);
    auto gotoSleepTime_m = timeToMinutes(gotoSleepTime);

    if (wakeupTime_m <= gotoSleepTime_m)
    {
        // Normal range (e.g., 08:00–17:00)
        return time_m >= wakeupTime_m && time_m < gotoSleepTime_m;
    }
    else
    {
        // Wrap-around range (e.g., 22:00–06:00)
        return time_m >= wakeupTime_m || time_m <= gotoSleepTime_m;
    }
}

int EddyClock::run()
{
    while(true)
    {
        button_hours.update();
        button_minutes.update();

        auto current_time = rv.getTime();
        wakeup_time = getWakeupTime();
        gotosleep_time = getGotoSleepTime();
        bool isWakeup = isWakeupTime(current_time, wakeup_time, gotosleep_time);
        if (is_wakeup_time != isWakeup)
        {
            is_wakeup_time = isWakeup;
            oled.setBrightness(is_wakeup_time ? 0xFF : 0x01);
        }

        // Wakeup Time
        if (button_wakeup.update() == button::PRESSED)
        {
            oled.renderIcon(SSD1306::SUN);
            auto t = getWakeupTime();
            //printf("%02u:%02u:%02u\r\n", t.hours, t.minutes, t.seconds);
            if (timeChanged(t))
                oled.renderTime(t.hours, t.minutes);

            if (button_hours.pollAction() == button::PRESS)
            {
                wakeup_time.hours = (t.hours + 1) % 24;
                wakeup_time.minutes = t.minutes;
                wakeup_time.seconds = 0;
                setWakeupTime((t.hours + 1) % 24, t.minutes);
            }
            t = getWakeupTime();
            if (button_minutes.pollAction() == button::PRESS)
            {
                wakeup_time.hours = t.hours;
                wakeup_time.minutes = (t.minutes + 1) % 60;
                wakeup_time.seconds = 0;
                setWakeupTime(t.hours, (t.minutes + 1) % 60);
            }
        }
        // Goto Sleep Time
        else if (button_sleep.update() == button::PRESSED)
        {
            oled.renderIcon(SSD1306::MOON);
            auto t = getGotoSleepTime();
            //printf("%02u:%02u:%02u\r\n", t.hours, t.minutes, t.seconds);
            if (timeChanged(t))
                oled.renderTime(t.hours, t.minutes);

            if (button_hours.pollAction() == button::PRESS)
            {
                gotosleep_time.hours = (t.hours + 1) % 24;
                gotosleep_time.minutes = t.minutes;
                gotosleep_time.seconds = 0;
                setGotoSleepTime((t.hours + 1) % 24, t.minutes);
            }
            t = getGotoSleepTime();
            if (button_minutes.pollAction() == button::PRESS)
            {
                gotosleep_time.hours = t.hours;
                gotosleep_time.minutes = (t.minutes + 1) % 60;
                gotosleep_time.seconds = 0;
                setGotoSleepTime(t.hours, (t.minutes + 1) % 60);
            }
        }
        // Regular Time
        else
        {
            //rv.printTime();
            auto t = rv.getTime();
            if ( compareTime(t, wakeup_time) == 1 &&   // current time is after wakeup time and before goto sleep time
                 compareTime(t, gotosleep_time) == -1) // if gotosleep is on the same day
                oled.renderIcon(SSD1306::SUN);
            else
                oled.renderIcon(SSD1306::MOON);

            if (timeChanged(t))
            {
                oled.renderTime(t.hours, t.minutes);
            }

            if (button_hours.pollAction() == button::PRESS)
            {
                rv.setTime((t.hours + 1) % 24, t.minutes, 0);
            }

            t = rv.getTime();
            if (button_minutes.pollAction() == button::PRESS)
            {
                rv.setTime(t.hours, (t.minutes + 1) % 60, 0);
            }
        }
    }

    return 1;
}

rv3028::rv3028_time_t EddyClock::getWakeupTime()
{
    rv3028::rv3028_time_t t;
    t.hours = rv.getEepromRegister(WAKEUP_HOURS_REGISTER);
    t.minutes = rv.getEepromRegister(WAKEUP_MINUTES_REGISTER);
    t.seconds = 0;

    return t;
}

bool EddyClock::setWakeupTime(uint16_t hours, uint16_t minutes)
{
    uint8_t h = hours % 24;
    uint8_t m = minutes % 60;

    if (rv.setEepromRegister(WAKEUP_HOURS_REGISTER, h) &&
        rv.setEepromRegister(WAKEUP_MINUTES_REGISTER, m) )
        return true;
    return false;
}

rv3028::rv3028_time_t EddyClock::getGotoSleepTime()
{
    rv3028::rv3028_time_t t;
    t.hours = rv.getEepromRegister(GOTOSLEEP_HOURS_REGISTER);
    t.minutes = rv.getEepromRegister(GOTOSLEEP_MINUTES_REGISTER);
    t.seconds = 0;

    return t;
}

bool EddyClock::setGotoSleepTime(uint16_t hours, uint16_t minutes)
{
    uint8_t h = hours % 24;
    uint8_t m = minutes % 60;

    if (rv.setEepromRegister(GOTOSLEEP_HOURS_REGISTER, h) &&
        rv.setEepromRegister(GOTOSLEEP_MINUTES_REGISTER, m) )
        return true;
    return false;
}

bool EddyClock::timeChanged(const rv3028::rv3028_time_t t)
{
    bool ret = false;

    if (t.hours != last_time.hours ||
        t.minutes != last_time.minutes)
    {
        ret = true;
    }
    last_time = t;
    return ret;
}