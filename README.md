6th Semester project for ISEL.

By:
 - Rafael Conceição, student number 41486
 - Eduardo Tavares, student number 49454

# Tools Setup

## Esp-32 

### Esp to CC1101 Connection

The table below shows the wire connection to be done between the radio and the micro-controller.
Some changes such as the GDOx pin connection on the Esp side may be possible, if the code is changed to
reflect them


| CC1101 pin number | CC1101 pin Name | Esp pin name |
|:------------------|:---------------:|-------------:|
| 1                 |       GND       |          GND |
| 2                 |       VCC       |          3V3 |
| 3                 |      GDO0       |           D4 |
| 4                 |       CSN       |           D5 |
| 5                 |       SCK       |          D18 |
| 6                 |      MOSI       |          D23 |
| 7                 |      MISO       |          D19 |
| 8                 |      GDO2       |           D2 |

### Esp-32 code

Code does not require any changes. Depending on updates it may require variable changes to chosen network's name and password

### Arduino IDE 2.0 Tool

The code for the micro-controller esp-32 uses the Arduino IDE 2.0, with
wifi_firmware_esp32.ino as the main file.

Compilation for Esp-32 requires installing the boards manager esp32 by Expressif Systems,
with https://github.com/espressif/arduino-esp32 github link associated. After installation,
choose the "DOIT ESP32 DEVKIT V1" board

### Arduino-cli tool

As an alternative to using arduino IDE, the command line program arduino-cli (https://docs.arduino.cc/arduino-cli/).
Installation for esp32 board through arduino-cli is arduino-cli core install arduino:esp32 //to reconfirm last word

Arduino-cli does does not have an equivalent to the serial monitor of the IDE, allowing to see the serial communication
done with the Esp-32. Since communication in this project  is from Esp->Computer, a simple continuous read loop created
with python's pyserial will suffice.


#### Arduino-cli commands

To be run on the base of this git repository

Copmilation: arduino-cli compile --fqbn esp32:esp32:esp32doit-devkit-v1 ./wifi_firmware_esp32

Upload: arduino-cli upload -p [port name, ie: COM3] --fqbn esp32:esp32:esp32doit-devkit-v1 ./wifi_firmware_esp32

## Application

This app is a simple text interface applicaation to be used through command line. As the Esp was set up to create its own network,
as a way to have a constant IP, it is necessary to change the network on the device being used to the target ESP network before
programm initiation. 

# Future improvements

## Wifi communication update; dns service discovery with MDNS

To eliminate the need of changing networks on the device, the ESPmDNS.h library to announce the esp testbed service for the 
local network. This will allow a device to use dns to get identifying information on multiple ESP devices.

Since this requires connecting to an existing network, the MCU code will need to be updated to contain the name and password of
such network.

### Required tools

#### Esp-32

The library ESPmDNS.h is already included from the base imports for this project.

Esp mdns service search code example:

https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/examples/mDNS-SD_Extended/mDNS-SD_Extended.ino

Http server code example:

https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS/examples/mDNS_Web_Server/mDNS_Web_Server.ino

#### Application

On a linux operating system, the installation of avahi (https://wiki.archlinux.org/title/Avahi) or other software is
required.

Code example:

https://github.com/avahi/avahi/blob/master/examples/client-browse-services.c
