Final *Software and Computer Engineering* bachelor's project for **Insituto Superior de Engenharia de Lisboa**.

Developed By:
 - Rafael Conceição, student number 41486
 - Eduardo Tavares, student number 49454

# Wi-Fi firmware for ESP32
## A Low-Cost Experimental Platform for Dense Wireless Networks

This repository contains the firmware for a wireless Medium Access Control (MAC) protocol implementation, developed for an ESP32 microcontroller development board (DOIT ESP32 DEVKIT V1) paired with a CC1101 radio module.


Hardware used in this project:
- CC1101 radio ![C1101 ](./docs/cc1101.png "Radio with antenna")

- [Esp 32 controller](https://www.flux.ai/blog/esp32-pinout-everything-you-need-to-know)

- Female to female cables connecting radio and Esp32, usb cable to connect to computer

Key software components include:

- MAC protocol implementation based on CSMA/CA, with RTS/CTS and NAV.

- Traffic generator for producing periodic network traffic.

- Terminal application for remote configuration and monitoring of the ESP32 nodes.

## Documentation

For information on how to set up the project, see the documentation [here](docs/).
