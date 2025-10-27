/**
 * ssd1306_i2c.c
 *
 * Interface to an SSD1306 OLED display using i2c.
 *
 * SSD1306 is an OLED driver chip for displays of multiple sizes. Here it is assumed the display is
 * 128x64 pixels.
 *
 * The default i2c interface is used. SDA -> GPIO 6 (PICO_DEFAULT_I2C_SDA_PIN)
 *                                    SCL -> GPIO 7 (PICO_DEFAULT_I2C_SCL_PIN)
 *
 * Copyright (c) 2024 Colin Luoma
 *
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSD1306_H
#define SSD1306_H

#include "pico/binary_info.h"

class SSD1306
{
public:

    enum Image {
        SUN,
        MOON
     };

    SSD1306(bool rotate_180);
    ~SSD1306();

    void setBrightness(uint8_t brightness);

    void render();
    void renderTime(uint8_t hours, uint8_t minutes);
    void renderIcon(Image i);
    //void renderDMA();

private:
    uint8_t * oled_buffer;
};

#endif
