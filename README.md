# Pico2Maple

A Dreamcast Maple bus emulator for the RP2350

# Required Hardware

* Raspberry Pi Pico 2
* Micro-USB to female USB-A adapter
* A Dreamcast controller cable
* Steam Controller with USB dongle

<div style="text-align: center"><a href="https://www.cluoma.com/images/pico2maple_needed_hardware.jpg"><img src="https://www.cluoma.com/images/pico2maple_needed_hardware.jpg" style="margin: auto; max-width: 100%"></a><p></p></div>

# Controls

Controls are mapped as you would expect. The exception is the left touchpad of the Steam Controller which must be clicked to be read as DPad input on the Dreamcast.

# Download

[Download the pico2maple uf2 firmware file](pico2maple.uf2).

Or [Download the steamcontroller2maple uf2 firmware file](steamcontroller2maple.uf2) for trackpad support as a second analog stick. Note that this may break controller compatibility with some games.

Holding down the *BOOTSEL* button while connecting the Pico to a PC should make it appear as a USB storage device. Then simply copy the pico2maple uf2 file over and the Pico should reboot itself with the new firmware.

# Construction

Use a multi-meter to check which wires on the controller cable correspond to the following pins on the controller plug.

<div style="text-align: center"><a href="https://www.cluoma.com/images/dc_controller_plug.jpg"><img src="https://www.cluoma.com/images/dc_controller_plug.jpg" style="margin: auto; max-width: 100%"></a><p></p></div>

Connect the wires to the labelled pins on the Pico below by soldering or otherwise.

<div style="text-align: center"><a href="https://www.cluoma.com/images/pico2maple_pinout.jpg"><img src="https://www.cluoma.com/images/pico2maple_pinout.jpg" style="margin: auto; max-width: 100%"></a><p></p></div>

With everything wired up, it's simply a matter of plugging in the Steam Controller dongle to the Pico using the USB-A to Mini-USB adapter and plugging the controller cable into the Dreamcast.

Once the Dreamcast is powered on, power on the Steam Controller and it should connect and be a usable Dreamcast controller!

# Code Acknowledgements

* [pico-sdk](https://github.com/raspberrypi/pico-sdk) - [BSD 3-Clause](https://github.com/raspberrypi/pico-sdk/blob/master/LICENSE.TXT)
* [tinyusb](https://github.com/hathach/tinyusb) - MIT
* [tusb_xinput](https://github.com/Ryzee119/tusb_xinput) - MIT
* [no-OS-FatFS-SD-SDIO-SPI-RPi-Pico](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico) - Apache 2.0, GPL-based ZuluSCSI SDIO code should be removed
* [btstack](https://github.com/bluekitchen/btstack) - [non-commercial](https://github.com/bluekitchen/btstack/blob/master/LICENSE), [pico commercial exception](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_btstack/LICENSE.RP) 
* [cyw43-driver](https://github.com/georgerobotics/cyw43-driver) - [License](https://github.com/georgerobotics/cyw43-driver/blob/main/LICENSE.RP)
* [bluepad32](https://github.com/ricardoquesada/bluepad32) - Apache 2.0
