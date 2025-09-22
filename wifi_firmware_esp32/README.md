# Running code

Installation of platformio ide is advised, but it is theoretically possible to use only the core.

A basic look into this folder's structure can be found [here](https://github.com/platformio/platformio-examples/tree/develop/unit-testing/calculator).

No upload port is specified, so it will default to search for one. If multiple are connected to the computer
at upload time, use [this guide](https://docs.platformio.org/en/latest/projectconf/sections/env/options/upload/upload_port.html)
to indicate the desired port

## Testing

Tests in /test directory are unit tests ran as the default in platformio. Tests regarding 2 devices, as a lot of features
regarding the communication between radios, will possibly to this folder if possible. Right now it can be found in
[here](/fixed_role_communication/radio_communication.cpp) source code to be ran as the main file. This can be acheived by
either placing this code on the [main file](/src/wifi_firmware_esp32.cpp), or by adding a 
[src_dir option](https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/src_dir.html).
