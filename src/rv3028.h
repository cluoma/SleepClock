//
// Created by colin on 9/13/25.
//

#ifndef RV3028_H
#define RV3028_H

#define RV3028_I2C_ADDR 0x52
#include <ctime>
#include <stdint.h>
#include "hardware/i2c.h"

class rv3028 {
public:
    rv3028(i2c_inst_t * i2c);
    ~rv3028() = default;

    typedef struct {
        uint8_t seconds;
        uint8_t minutes;
        uint8_t hours;
    } rv3028_time_t;

    void oneTimeSetup();
    void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void setDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekday);
    void setDateTime(time_t * time);
    bool setEepromRegister(uint8_t eeprom_addr, uint8_t val);
    uint8_t getEepromRegister(uint8_t eeprom_addr);
    rv3028_time_t getTime();
    void printTime();

private:
    i2c_inst_t * _i2c;
};

#endif //RV3028_H
