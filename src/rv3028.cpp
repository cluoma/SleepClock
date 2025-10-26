//
// Created by colin on 9/13/25.
//

#include "rv3028.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <hardware/i2c.h>
#include <pico/time.h>

// The 7-bit I2C ADDRESS of the RV3028
#define RV3028_ADDR         0x52


// REGISTERS
// Clock registers
#define RV3028_SECONDS      0x00
#define RV3028_MINUTES      0x01
#define RV3028_HOURS        0x02
// Calendar registers
#define RV3028_WEEKDAY      0x03
#define RV3028_DATE         0x04
#define RV3028_MONTHS       0x05
#define RV3028_YEARS        0x06

// Alarm registers
#define RV3028_MINUTES_ALM  0x07
#define RV3028_HOURS_ALM    0x08
#define RV3028_DATE_ALM     0x09

// Periodic Countdown Timer registers
#define RV3028_TIMERVAL_0   0x0A
#define RV3028_TIMERVAL_1   0x0B
#define RV3028_TIMERSTAT_0  0x0C
#define RV3028_TIMERSTAT_1  0x0D

// Configuration registers
#define RV3028_STATUS       0x0E
#define RV3028_CTRL1        0x0F
#define RV3028_CTRL2        0x10
#define RV3028_GPBITS       0x11
#define RV3028_INT_MASK     0x12

// Eventcontrol/Timestamp registers
#define RV3028_EVENTCTRL    0x13
#define RV3028_COUNT_TS     0x14
#define RV3028_SECONDS_TS   0x15
#define RV3028_MINUTES_TS   0x16
#define RV3028_HOURS_TS     0x17
#define RV3028_DATE_TS      0x18
#define RV3028_MONTH_TS     0x19
#define RV3028_YEAR_TS      0x1A

// Unix Time registers
#define RV3028_UNIX_TIME0   0x1B
#define RV3028_UNIX_TIME1   0x1C
#define RV3028_UNIX_TIME2   0x1D
#define RV3028_UNIX_TIME3   0x1E

// RAM registers
#define RV3028_USER_RAM1    0x1F
#define RV3028_USER_RAM2    0x20

// Password registers
#define RV3028_PASSWORD0    0x21
#define RV3028_PASSWORD1    0x22
#define RV3028_PASSWORD2    0x23
#define RV3028_PASSWORD3    0x24

// EEPROM Memory Control registers
#define RV3028_EEPROM_ADDR  0x25
#define RV3028_EEPROM_DATA  0x26
#define RV3028_EEPROM_CMD   0x27

// ID register
#define RV3028_ID           0x28
#define RV3028_CHIP_ID      0x30
#define RV3028_VERSION      0x03

// EEPROM Registers
#define EEPROM_Clkout_Register   0x35
#define RV3028_EEOffset_8_1      0x36 // bits 8 to 1 of EEOffset. Bit 0 is bit 7 of register 0x37
#define EEPROM_Backup_Register   0x37


// BITS IN IMPORTANT REGISTERS

// Bits in Status Register
#define STATUS_EEBUSY_BIT     7
#define STATUS_CLKF_BIT       6
#define STATUS_BSF_BIT        5
#define STATUS_UF_BIT         4
#define STATUS_TF_BIT         3
#define STATUS_AF_BIT         2
#define STATUS_EVF_BIT        1
#define STATUS_PORF_BIT       0

// Bits in Control1 Register
#define CTRL1_TRPT        7
#define CTRL1_WADA        5 // Bit 6 not implemented
#define CTRL1_USEL        4
#define CTRL1_EERD_BIT        3
#define CTRL1_TE          2
#define CTRL1_TD1         1
#define CTRL1_TD0         0

// Bits in Control2 Register
#define CTRL2_TSE         7
#define CTRL2_CLKIE       6
#define CTRL2_UIE         5
#define CTRL2_TIE         4
#define CTRL2_AIE         3
#define CTRL2_EIE         2
#define CTRL2_12_24       1
#define CTRL2_RESET       0

// Bits in Hours register
#define HOURS_AM_PM       5

// Bits in Alarm registers
#define MINUTESALM_AE_M   7
#define HOURSALM_AE_H     7
#define DATE_AE_WD        7

// Commands for EEPROM Command Register (0x27)
#define EEPROMCMD_First          0x00
#define EEPROMCMD_Update         0x11
#define EEPROMCMD_Refresh        0x12
#define EEPROMCMD_WriteSingle    0x21
#define EEPROMCMD_ReadSingle     0x22

// Bits in EEPROM Backup Register
#define EEPROMBackup_TCE_BIT     5           // Trickle Charge Enable Bit
#define EEPROMBackup_FEDE_BIT    4           // Fast Edge Detection Enable Bit (for Backup Switchover Mode)
#define EEPROMBackup_BSM_SHIFT   2           // Backup Switchover Mode shift
#define EEPROMBackup_TCR_SHIFT   0           // Trickle Charge Resistor shift

#define EEPROMBackup_BSM_CLEAR   0b11110011  // Backup Switchover Mode clear
#define EEPROMBackup_TCR_CLEAR   0b11111100  // Trickle Charge Resistor clear
#define TCR_3K                   0b00        // Trickle Charge Resistor 3kOhm
#define TCR_5K                   0b01        // Trickle Charge Resistor 5kOhm
#define TCR_9K                   0b10        // Trickle Charge Resistor 9kOhm
#define TCR_15K                  0b11        // Trickle Charge Resistor 15kOhm


// Clock output register (0x35)
#define EEPROMClkout_CLKOE_BIT   7           // 1 = CLKOUT pin is enabled. – Default value on delivery
#define EEPROMClkout_CLKSY_BIT   6
// Bits 5 and 4 not implemented
#define EEPROMClkout_PORIE       3           // 0 = No interrupt, or canceled, signal on INT pin at POR. – Default value on delivery
                                             // 1 = An interrupt signal on INT pin at POR. Retained until the PORF flag is cleared to 0 (no automatic cancellation).
#define EEPROMClkout_FREQ_SHIFT  0           // frequency shift
#define FD_CLKOUT_32k            0b000       // 32.768 kHz – Default value on delivery
#define FD_CLKOUT_8192           0b001       // 8192 Hz
#define FD_CLKOUT_1024           0b010       // 1024 Hz
#define FD_CLKOUT_64             0b011       // 64 Hz
#define FD_CLKOUT_32             0b100       // 32 Hz
#define FD_CLKOUT_1              0b101       // 1 Hz
#define FD_CLKOUT_TIMER          0b110       // Predefined periodic countdown timer interrupt
#define FD_CLKOUT_LOW            0b111       // CLKOUT = LOW


#define IMT_MASK_CEIE            3           // Clock output when Event Interrupt bit.
#define IMT_MASK_CAIE            2           // Clock output when Alarm Interrupt bit.
#define IMT_MASK_CTIE            1           // Clock output when Periodic Countdown Timer Interrupt bit.
#define IMT_MASK_CUIE            0           // Clock output when Periodic Time Update Interrupt bit.


#define TIME_ARRAY_LENGTH 7 // Total number of writable values in device

uint16_t bcd_to_dec(const uint8_t b)
{
    return (b & 0x0F) + 10 * ((b & 0xF0) >> 4);
}

uint8_t dec_to_bcd(const uint8_t d)
{
    uint8_t tens = d / 10;
    uint8_t ones = (d % 10) & 0xF;
    return tens << 4 | ones;
}

static uint8_t read_register(i2c_inst_t * i2c, uint8_t reg)
{
    uint8_t val;
    i2c_write_blocking(i2c, RV3028_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, RV3028_I2C_ADDR, &val, 1, false);
    return val;
}

static bool write_register(i2c_inst_t * i2c, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    if (i2c_write_blocking(i2c, RV3028_I2C_ADDR, buf, 2, true) == 1)
        return true;

    return false;
}

static bool wait_for_eeprom_nobusy(i2c_inst_t * i2c) {
    // Timeout is number of loops round while - don't have access to millisecond counter
    unsigned long timeout = 500;
    while ((read_register(i2c, RV3028_STATUS) & 1 << STATUS_EEBUSY_BIT) && --timeout > 0);
    return timeout > 0;
}

rv3028::rv3028(i2c_inst_t * i2c)
{
    _i2c = i2c;
    write_register(_i2c, RV3028_STATUS, 0x00);
}

bool set_eeprom_autorefresh(i2c_inst_t * i2c, bool automatic)
{
    bool ok = wait_for_eeprom_nobusy(i2c);
    if (!ok)
        return ok;

    uint8_t ctrl1 = read_register(i2c, RV3028_CTRL1);

    if (automatic)
        ctrl1 &= ~(1 << CTRL1_EERD_BIT);
    else
        ctrl1 |= 1 << CTRL1_EERD_BIT;

    if (!write_register(i2c, RV3028_CTRL1, ctrl1))
        ok = false;

    return ok;
}

bool write_config_eeprom_ram_mirror(i2c_inst_t * i2c, uint8_t eeprom_addr, uint8_t val) {
    bool success = wait_for_eeprom_nobusy(i2c);

    // Disable auto refresh by writing 1 to EERD control bit in CTRL1 register
    set_eeprom_autorefresh(i2c, false);

    // Write Configuration RAM Register
    write_register(i2c, eeprom_addr, val);

    // Update EEPROM (All Configuration RAM -> EEPROM)
    write_register(i2c, RV3028_EEPROM_CMD, EEPROMCMD_First);
    write_register(i2c, RV3028_EEPROM_CMD, EEPROMCMD_Update);

    if(!wait_for_eeprom_nobusy(i2c))
        success = false;

    // Reenable auto refresh by writing 0 to EERD control bit in CTRL1 register
    set_eeprom_autorefresh(i2c, true);

    if(!wait_for_eeprom_nobusy(i2c))
        success = false;

    return success;
}

uint8_t read_config_eeprom_ram_mirror(i2c_inst_t * i2c, uint8_t eeprom_addr)
{
    bool success = wait_for_eeprom_nobusy(i2c);

    // Disable auto refresh by writing 1 to EERD control bit in CTRL1 register
    set_eeprom_autorefresh(i2c, false);

    // Read EEPROM Register
    write_register(i2c, RV3028_EEPROM_ADDR, eeprom_addr);
    write_register(i2c, RV3028_EEPROM_CMD, EEPROMCMD_First);
    write_register(i2c, RV3028_EEPROM_CMD, EEPROMCMD_ReadSingle);

    if(!wait_for_eeprom_nobusy(i2c))
        success = false;

    uint8_t eeprom_data = read_register(i2c, RV3028_EEPROM_DATA);
    if(!wait_for_eeprom_nobusy(i2c))
        success = false;

    // Reenable auto refresh by writing 0 to EERD control bit in CTRL1 register
    set_eeprom_autorefresh(i2c, true);

    if(!success)
        return 0xFF;

    return eeprom_data;
}

bool rv3028::setEepromRegister(uint8_t eeprom_addr, uint8_t val)
{
    if (eeprom_addr > 0x2A)  // max user-accessible EEPROM address
        return false;

    bool ret = wait_for_eeprom_nobusy(_i2c);

    // Disable auto refresh by writing 1 to EERD control bit in CTRL1 register
    set_eeprom_autorefresh(_i2c, false);

    // Write EEPROM Register
    write_register(_i2c, RV3028_EEPROM_ADDR, eeprom_addr);
    write_register(_i2c, RV3028_EEPROM_DATA, val);
    write_register(_i2c, RV3028_EEPROM_CMD, EEPROMCMD_First);
    write_register(_i2c, RV3028_EEPROM_CMD, EEPROMCMD_WriteSingle);

    if(!wait_for_eeprom_nobusy(_i2c))
        ret = false;

    // Reenable auto refresh by writing 0 to EERD control bit in CTRL1 register
    set_eeprom_autorefresh(_i2c, true);

    return ret;
}

uint8_t rv3028::getEepromRegister(uint8_t eeprom_addr)
{
    if (eeprom_addr > 0x2A)  // max user-accessible EEPROM address
        return 0xFF;

    if (!wait_for_eeprom_nobusy(_i2c))
        return 0xFF;

    // Disable auto refresh by writing 1 to EERD control bit in CTRL1 register
    set_eeprom_autorefresh(_i2c, false);

    // Write EEPROM Register
    write_register(_i2c, RV3028_EEPROM_ADDR, eeprom_addr);
    write_register(_i2c, RV3028_EEPROM_CMD, EEPROMCMD_First);
    write_register(_i2c, RV3028_EEPROM_CMD, EEPROMCMD_ReadSingle);

    if(!wait_for_eeprom_nobusy(_i2c))
        return 0xFF;

    // Reenable auto refresh by writing 0 to EERD control bit in CTRL1 register
    set_eeprom_autorefresh(_i2c, true);

    return read_register(_i2c, RV3028_EEPROM_DATA);
}

/*********************************
  0 = Switchover disabled
  1 = Direct Switching Mode
  2 = Standby Mode
  3 = Level Switching Mode
  *********************************/
bool set_backup_switchover_mode(i2c_inst_t * i2c, uint8_t val) {
    if(val > 3)
        return false;

    bool success = true;

    // Read EEPROM Backup Register (0x37)
    uint8_t eeprom_backup = read_config_eeprom_ram_mirror(i2c, EEPROM_Backup_Register);
    if(eeprom_backup == 0xFF)
        success = false;

    // Ensure FEDE Bit is set to 1
    eeprom_backup |= 1 << EEPROMBackup_FEDE_BIT;

    // Set BSM Bits (Backup Switchover Mode)
    eeprom_backup &= EEPROMBackup_BSM_CLEAR;         // Clear BSM Bits of EEPROM Backup Register
    eeprom_backup |= val << EEPROMBackup_BSM_SHIFT;  // Shift values into EEPROM Backup Register

    // Write EEPROM Backup Register
    if(!write_config_eeprom_ram_mirror(i2c, EEPROM_Backup_Register, eeprom_backup))
        success = false;

    return success;
}

void rv3028::oneTimeSetup()
{
    // Disable trickle-charging
    uint8_t eeprom_backup = read_config_eeprom_ram_mirror(_i2c, EEPROM_Backup_Register);
    eeprom_backup &= ~(1 << 5);
    write_config_eeprom_ram_mirror(_i2c, EEPROM_Backup_Register, eeprom_backup);
    sleep_ms(1000);

    // Check switchover to level-shift mode for battery backup
    set_backup_switchover_mode(_i2c, 3);
    sleep_ms(1000);
}

void rv3028::setTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    uint8_t buf[4] = {
        RV3028_SECONDS,
        dec_to_bcd(seconds),
        dec_to_bcd(minutes),
        dec_to_bcd(hours)
    };
    i2c_write_blocking(_i2c, RV3028_I2C_ADDR, buf, sizeof(buf), false);
}

void rv3028::setDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekday)
{
    uint8_t buf[5] = {
        RV3028_WEEKDAY,
        dec_to_bcd(weekday),
        dec_to_bcd(day),
        dec_to_bcd(month),
        dec_to_bcd(year)
    };
    i2c_write_blocking(_i2c, RV3028_I2C_ADDR, buf, sizeof(buf), false);
}

void rv3028::setDateTime(time_t * time)
{
    struct tm * t = gmtime(time);
    //mktime
    uint8_t buf[8] = {
        RV3028_SECONDS,
        dec_to_bcd(t->tm_sec),
        dec_to_bcd(t->tm_min),
        dec_to_bcd(t->tm_hour),
        dec_to_bcd(t->tm_wday),
        dec_to_bcd(t->tm_mday),
        dec_to_bcd(t->tm_mon),
        dec_to_bcd(t->tm_year + 100)
    };
    i2c_write_blocking(_i2c, RV3028_I2C_ADDR, buf, sizeof(buf), false);
}

void rv3028::printTime()
{
    uint8_t seconds_addr = 0x00;
    uint8_t time[3];
    i2c_write_blocking(_i2c, RV3028_I2C_ADDR, &seconds_addr, 1, true);
    i2c_read_blocking(_i2c, RV3028_I2C_ADDR, time, 3, false);
    printf("%02lu:%02lu:%02lu\n", bcd_to_dec(time[2]), bcd_to_dec(time[1]), bcd_to_dec(time[0]));
}

rv3028::rv3028_time_t rv3028::getTime()
{
    rv3028_time_t time;

    uint8_t seconds_addr = 0x00;
    i2c_write_blocking(_i2c, RV3028_I2C_ADDR, &seconds_addr, 1, true);
    i2c_read_blocking(_i2c, RV3028_I2C_ADDR, (uint8_t *)&time, 3, false);

    time.hours = bcd_to_dec(time.hours);
    time.minutes = bcd_to_dec(time.minutes);
    time.seconds = bcd_to_dec(time.seconds);

    return time;
}