# SleepClock

An alarm clock of sorts for children. The display will automatically adjust its brightness and show a unique picture to indicate when it is 'go to sleep' or 'wakeup' time.

Built using RP2350, RV3028 RTC, and SSD1309 oled screen.

![Pico2Maple three dongles](images/clock.jpg)

# Operation

In addition to keeping track of the current time of day, the clock has two special time modes that can be set independently. Which mode is active is determined by the current time of day, and the trigger times of each respective special mode.

* **Go to sleep mode** is a specific time where the display dims and a moon picture is shown, indicating it's bedtime. This mode continues until 'wakeup' time.
* **Wakeup mode** is a specific time that brightens the display and shows a sun picture, indicating it's wakeup time. This mode continues until 'go to sleep' time.

The RV3028 RTC keeps track of the current time of day. The trigger times for special modes are stored in the RV3028's non-volatile user-eeprom. Each of these can be adjusted using 4 push-buttons wired to the microcontroller.
