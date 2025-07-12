# Project Setup

## Hardware 


The table below shows the wire connection to be done between the radio and the micro-controller.
Some changes such as the GDOx pin connection on the ESP side may be possible, if the code is changed to reflect them.


| CC1101 pin number | CC1101 pin Name | ESP pin name |
|:------------------|:---------------:|-------------:|
| 1                 |       GND       |          GND |
| 2                 |       VCC       |          3V3 |
| 3                 |      GDO0       |           D4 |
| 4                 |       CSN       |           D5 |
| 5                 |       SCK       |          D18 |
| 6                 |      MOSI       |          D23 |
| 7                 |      MISO       |          D19 |
| 8                 |      GDO2       |           D2 |

## Setup Environment and Run

Steps to run this project:

1. Clone this repo into your machine
2. Install Arduino IDE 2.0
3. In the IDE, install ["esp32" by Espressif Systems](https://github.com/espressif/arduino-esp32) in Boards Manager
4. Open the `wifi_firmware_esp32` folder from project in the IDE
5. Connect the ESP32 to your machine
6. At the top of the IDE:
- Select the correct board (DOIT ESP32 DEVKIT V1)
- Select the port where the ESP is connected
7. Press upload

To view debug output, you must uncomment line #31 in the file `wifi_firmware_esp32.ino`, and then in the IDE go into **Tools >  Serial Monitor**. 

### Arduino-cli tool

As an alternative to using arduino IDE, the command line program [arduino-cli](https://docs.arduino.cc/arduino-cli/) can be used.
Installation for esp32 board through arduino-cli is arduino-cli core install arduino:esp32

Arduino-cli does does not have an equivalent to the serial monitor of the IDE, allowing to see the serial communication
done with the Esp-32. Since communication in this project  is from Esp->Computer, a simple continuous read loop created
with python's pyserial will suffice.


#### Arduino-cli commands

To be run on the base of this git repository

Copmilation: arduino-cli compile --fqbn esp32:esp32:esp32doit-devkit-v1 ./wifi_firmware_esp32

Upload: arduino-cli upload -p [port name, ie: COM3] --fqbn esp32:esp32:esp32doit-devkit-v1 ./wifi_firmware_esp32

## Application

This terminal based application allows for the configuration of the individual ESPs. It was built and tested on a Linux environment.

### Connect to the ESP

As the ESPs are set up as access points, you must first connect to the ESP you wish to configure using throuh your machines network manager.

- The SSID will start with `ESP32-` and be followed by the MAC address of the ESP.

- The password is defined in [`wifi_config.h`](wifi_firmware_esp32/src/wifi_config/wifi_config.h); the default value is `ESP32-firmware`.

### Build and Run

To build an run the application:

1. Go into the `terminal_app` directory.

2. Run the makefile with:

    make

3. Run the binary with:

    ./bin/config_esp

### Usage

- Type `h` to see all available options.
- Type `x` to close the connection and terminate the application.
