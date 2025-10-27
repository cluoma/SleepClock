/**
 * ssd1306_i2c.c
 *
 * Interface to an SSD1306 OLED display using i2c.
 *
 * SSD1306 is an OLED driver chip for displays of multiple sizes. Here it is assumed the display is
 * 128x64 pixels.
 *
 * The default i2c interface is used. SDA -> GPIO 4 (PICO_DEFAULT_I2C_SDA_PIN)
 *                                    SCL -> GPIO 5 (PICO_DEFAULT_I2C_SCL_PIN)
 *
 * Copyright (c) 2024 Colin Luoma
 *
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * The 128x64 pixel screen space is split into several sections for different purposes.
 *
 * |----------------------|-----------------------------------------------|
 * | 96x64 VMU screen     | 32x16 VMU title                               |
 * |                      |-----------------------------------------------|
 * |                      | 32x16 Current VMU bank number                 |
 * |                      |-----------------------------------------------|
 * |                      | 32x16 Current USB device                      |
 * |                      |-----------------------------------------------|
 * |----------------------| 16x16 Exclamation | 16x16 Storage target      |
 *
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include <hardware/dma.h>
#include "ssd1306.h"
extern "C" {
#include "oled_static_data.h"
}

#define SSD1306_HEIGHT              64
#define SSD1306_WIDTH               128

#define SSD1306_I2C_ADDR            _u(0x3C)

// 400 is usual, but often these can be overclocked to improve display response.
// Tested at 1000 on both 32 and 84 pixel height devices and it worked.
#define SSD1306_I2C_CLK             400
//#define SSD1306_I2C_CLK             1000


// commands (see datasheet)
#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

//#define OLED_BUFFER_SIZE (7 + 8 * 128) // 7 words for screen clearing commands and data start bit before screen data
#define OLED_BUFFER_SIZE (128 *  64 / 8)

static uint16_t renderAreaBufLen(uint8_t colStart, uint8_t colEnd, uint8_t pageStart, uint8_t pageEnd)
{
    // calculate how long the flattened buffer will be for a render area
    return (colEnd - colStart + 1) * (pageEnd - pageStart + 1);
}

void SSD1306_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(const uint8_t *buf, int buflen) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    uint8_t * temp_buf = (uint8_t *)malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);

    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);

    free(temp_buf);
}

// int64_t ssd1306_enable_render_dma(alarm_id_t id, __unused void *user_data) {
//     is_render_dma_enabled = true;
//     return 0;
// }

// void SSD1306_init_dma() {
//     // set up DMA to write to i2c
//
//     /// Initialize oled buffer
//     for (uint32_t i = 0; i < OLED_BUFFER_SIZE; i++)
//     {
//         oled_buffer[i] = 0;
//     }
//     // set initial values of the oled_buffer, these always need to be sent before the actual data
//     oled_buffer[0] = (SSD1306_SET_COL_ADDR    << 8) | 0x80;
//     oled_buffer[1] = (0                       << 8) | 0x80;
//     oled_buffer[2] = ((SSD1306_WIDTH - 1)     << 8) | 0x80;
//     oled_buffer[3] = (SSD1306_SET_PAGE_ADDR   << 8) | 0x80;
//     oled_buffer[4] = (0                       << 8) | 0x80;
//     oled_buffer[5] = ((SSD1306_NUM_PAGES - 1) << 8) | 0x80;
//     oled_buffer[6] = 0x40;  // data start signal
//     // vmu bank number area
//     for (int y = 0; y < 2; y++)
//     {
//         for (int x = 96; x < 128; x++)
//         {
//             oled_buffer[7 + y * 128 + x] = (uint16_t)vmu_oled_static_bank_title[y*32 + x - 96];
//         }
//     }
//
//     /// Initialize DMA
//     dma_channel = dma_claim_unused_channel(true);
//     dma_config = dma_channel_get_default_config(dma_channel);
//
//     channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_16);
//     channel_config_set_dreq(&dma_config, i2c_get_dreq(i2c_default, true));
//     channel_config_set_read_increment(&dma_config, true);
//     channel_config_set_write_increment(&dma_config, false);
//
//     dma_channel_configure(dma_channel,
//                           &dma_config,
//                           &i2c_default->hw->data_cmd,
//                           NULL,
//                           0,
//                           false);
// }

void renderArea(const uint8_t *buf, uint8_t colStart, uint8_t colEnd, uint8_t pageStart, uint8_t pageEnd)
{
    // update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        colStart,
        colEnd,
        SSD1306_SET_PAGE_ADDR,
        pageStart,
        pageEnd
    };
    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, renderAreaBufLen(colStart, colEnd, pageStart, pageEnd));
}

void SSD1306::render()
{
    renderArea(oled_buffer, 0, 127, 0, 7);
}

void renderDigit(uint8_t digit, uint8_t colStart, uint8_t colEnd)
{
    uint8_t pageStart = 0;
    uint8_t pageEnd = 3;
    switch (digit)
    {
        case 0:
            renderArea(oled_zero, colStart, colEnd, pageStart, pageEnd);
            break;
        case 1:
            renderArea(oled_one, colStart, colEnd, pageStart, pageEnd);
            break;
        case 2:
            renderArea(oled_two, colStart, colEnd, pageStart, pageEnd);
            break;
        case 3:
            renderArea(oled_three, colStart, colEnd, pageStart, pageEnd);
            break;
        case 4:
            renderArea(oled_four, colStart, colEnd, pageStart, pageEnd);
            break;
        case 5:
            renderArea(oled_five, colStart, colEnd, pageStart, pageEnd);
            break;
        case 6:
            renderArea(oled_six, colStart, colEnd, pageStart, pageEnd);
            break;
        case 7:
            renderArea(oled_seven, colStart, colEnd, pageStart, pageEnd);
            break;
        case 8:
            renderArea(oled_eight, colStart, colEnd, pageStart, pageEnd);
            break;
        case 9:
            renderArea(oled_nine, colStart, colEnd, pageStart, pageEnd);
            break;
        default:
            break;
    }
}

void SSD1306::renderTime(uint8_t hours, uint8_t minutes)
{
    uint8_t empty[64] = {0};
    uint8_t hour_tens = 59;
    uint8_t hour_ones = hour_tens + 16;
    uint8_t colon = hour_ones + 16;
    uint8_t minute_tens = colon + 4;
    uint8_t minute_ones = minute_tens + 16;

    // Get AM/PM
    uint8_t hours_ampm;
    if (hours == 0)
        hours_ampm = 12;
    else if (hours > 12)
        hours_ampm = hours - 12;
    else
        hours_ampm = hours;

    if (hours_ampm / 10)
        renderDigit(1, hour_tens, hour_tens+15);
    else
        renderArea(empty, hour_tens, hour_tens+15, 0, 3);
    // hour ones
    renderDigit(hours_ampm % 10, hour_ones, hour_ones+15);
    // minute tens
    renderDigit(minutes / 10, minute_tens, minute_tens+15);
    // minute ones
    renderDigit(minutes % 10, minute_ones, minute_ones+15);
    renderArea(oled_colon, colon, colon+3, 0, 3);

    // am/pm
    if (hours < 12)
        renderArea(oled_am, 111, 127, 4, 4);
    else
        renderArea(oled_pm, 111, 127, 4, 4);

}

void SSD1306::renderIcon(Image i)
{
    if (i == SUN)
    {
        renderArea(oled_sun, 0, 63, 0, 7);
    }
    else if (i == MOON)
    {
        renderArea(oled_moon, 0, 63, 0, 7);
    }
}

void SSD1306::setBrightness(uint8_t brightness)
{
    uint8_t cmds[] = {
        SSD1306_SET_CONTRAST,           // set contrast control
        brightness
    };
    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

SSD1306::SSD1306(bool rotate_180)
{
    /// Run through initial chip setup
    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like
    // Some configuration values are recommended by the board manufacturer
    uint8_t cmds[] = {
        SSD1306_SET_DISP,               // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                           // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
        (uint8_t)(SSD1306_SET_SEG_REMAP | (rotate_180 ? 0x00 : 0x01)),   // set segment re-map, column address 127 is mapped to SEG0
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        (uint8_t)(SSD1306_SET_COM_OUT_DIR | (rotate_180 ? 0x00 : 0x08)), // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number.
        0x12,                           // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
        0x80,                           // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,          // set pre-charge period
        0xF1,                           // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
        0x30,                           // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST,           // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,        // set charge pump
        0x14,                           // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00,      // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01, // turn display on
    };
    SSD1306_send_cmd_list(cmds, count_of(cmds));
    sleep_ms(50);

    oled_buffer = static_cast<uint8_t *>(malloc(OLED_BUFFER_SIZE));
    memset(oled_buffer, 0x00, OLED_BUFFER_SIZE);

    // First render
    render();
}

SSD1306::~SSD1306()
{
    free(oled_buffer);
}