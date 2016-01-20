DCCCommander
===========

DCCCommander is an embedded library written in C/C++ that provides the foundation for implementing an NMRA DCC command station.

DCCCommander provides functions for various train and accessory related settings, including: setting a locomotive’s speed, activating functions, switching turnouts, and programming DCC decoders. 

DCCCommander translates these commands into NMRA-compatible DCC packets for transmitting over the tracks. Packets are prioritzed dependent on their type and DCCCommander keeps track of packets that require long-term repeating in the background. 

An interrupt service routine attached to TIMER1 (AVR) or MCPWM0 (ARM) takes these packets and injects the highest priority packet into the precision DCC waveform. This signal is suitable for amplification with your booster/power station/motor-controller.

**Please note that the Arduino outputs are not capable of driving trains directly.**

Currently, DCCCommander supports various Arduino devices including atmega168/328 variants, as well as the Yun and attiny.

Installation
------------

To install, see the general instructions for Arduino library installation here:
http://arduino.cc/en/Guide/Environment#libraries


Origins
-------

DCCCommander is a heavily modified fork to CmdrArduino - https://github.com/Railstars/CmdrArduino
