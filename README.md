CmdrArduino
===========

CmdrArduino is an embedded library written in C(++) that provides the foundation for implementing an NMRA DCC command station.

CmdrArduino presents classes and methods for, among other things, setting a locomotive’s speed, activating functions, switching turnouts, and programming DCC decoders. CmdrArduino translates these commands into DCC packets. The packets are carefully prioritzed, and CmdrArduino keeps track of packets that require repeating in the background. An interrupt service routine attached to TIMER1 (AVR) or MCPWM0 (ARM) takes these packets and injects the highest priority packet into the precision DCC waveform. This signal is suitable for amplification with your booster/power station. Please note that the Arduino outputs are not themselves capable of driving trains directly.

Currently, CmdrArduino supports various Arduino devices including atmega168/328 variants, as well as the Yun and attiny.

Installation
------------

To install, see the general instructions for Arduino library installation here:
http://arduino.cc/en/Guide/Environment#libraries

