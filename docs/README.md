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

This branch has changed to use platformio extension in vscode.
See 
[this tutorial](https://docs.platformio.org/en/latest/integration/ide/vscode.html) for more guided information

Steps to run this project:

1. Clone this repo into your machine
2. Install platformio ide extension (extension id platformio.platformio-ide)
3. Open a platformio project with [this folder](../wifi_firmware_esp32/) as base, the configuration ini
file in base installation of board should be automatic
4. Connect the ESP32 to your machine
6. At the bottom of the vs code IDE press platformio upload. 

To view debug output, you must uncomment #define MONITOR_DEBUG_MODE on [this file](../wifi_firmware_esp32/include/arduinoGlue.h),
and then in the IDE go into **Tools >  Serial Monitor**.  May be changed to be on [inii configuration file](../wifi_firmware_esp32/platformio.ini)
on the build_flags options. 

# GUI application

This app uses a combination of fltk (version 1.4.4) found [here](https://github.com/fltk/fltk), installed with cmake as instructed
on their README.md

## Connect to the ESP

Esp's were changeed to be part of the normal network, which will make unfeasable the knowledge of their ip addresses a priori. Check if
your esp device is capable of using 5G networks before trying to connect them to one.

As a way to find their addresses, avahi client is to be installed on a linux system with the command 
sudo apt install libavahi-client-dev libavahi-common-dev

- The name and password are defined in [`this folder`](../wifi_firmware_esp32/src/wifi_config/); Recreate the file
hidden by its gitignore to add your values.

### Build and Run

To build an run the application:

1. Go into the [fltk](../fltk_app/) directory.

2. Run the makefile with:

    make

3. Run the binary with:

    ./bin/app

### Usage

In development. TODO update
