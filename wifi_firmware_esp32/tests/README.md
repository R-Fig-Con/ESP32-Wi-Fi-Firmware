# Testing

This directory contains code related to testing  the c1101 driver module. 
The usefulness in these tests is found more in having ways to test
the conditions of the used hardware than verifying the correctness of this
project's code.

To test your esp32 hardware by itself, see  [here.](https://github.com/espressif/arduino-esp32/tree/master/tests)



## Organization
Since creating separate executables as would generally be done in code for
computers would require uploading each executable to the MCU, a single
main file with the ability to choose the test through serial communication
was created.

Note: added mklink /J src ..\src for copy of header folder; mklink cc1101_driver.ino ..\cc1101_driver.ino
for copy of radio implementation. Folder seems to be hard copied in git, to find a variation of command
that does not do such a thing or leave in this readme instructions for testers

## Openhtf (TODO)

To create Openhtf pyhon scripts for each test in the .ino