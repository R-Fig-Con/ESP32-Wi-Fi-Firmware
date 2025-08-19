# Testing

This directory contains code related to testing modules contained in the
Esp32. The usefulness in these tests is found more in having ways to test
the conditions of the used hardware than verifying the correctness of this
project's code

## Organization
Since creating separate executables as would generally be done in code for
computers would require uploading each executable to the MCU, a single
main file with the ability to choose the test through serial communication
was created.

## Openhtf (TODO)

To create Openhtf pyhon scripts for each test in the .ino