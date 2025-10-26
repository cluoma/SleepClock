/**
* main.c
 *
 * Pico2Maple main entry point, starts the Maple bus on core1 and runs the TinyUSB stack on core0
 *
 * Copyright (c) 2024 Colin Luoma
 */

#include <chrono>
#include <stdio.h>
#include <sys/unistd.h>
#include "pico/stdlib.h"

extern "C" {
#include "ssd1306.h"
#include "utils.h"
}

#include "EddyClock.h"

int main()
{
    stdio_init_all();
    printf("Starting eddyclock\n");

    // initialize i2c
    i2c_init(i2c_default, 400 * 2000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    sleep_ms(50);

    EddyClock c(i2c_default);
    c.run();

    return 0;
}

