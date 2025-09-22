# CC1101 driver

This driver is accessed as a singleton, as a way to give easy access to the same radio instance through the
project. This instance ought to be readied by the user of the library with at least a call to init function.

This driver works with the assumption of the existance of spi communication and 2 gd0 pins

## ccpacket

C Struct containting parameters for a specific configuration of internal radio state

## cc1101_option.h

define whose existance affects the compilation of the class. In separated file to be easier to find

## radio_pins.h

Declaration of values identifying the pin connection in esp32. Possibly to change organization later
as to indicate pins through parameters received in constructor/setter function.
