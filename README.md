# BinCounter

Arduino based device that uses two ultrasonic distance devices to detect which side of the bin the 
rubbish item was thrown into.

The device shows the current score via an LCD display. 


## Features
* Show 'score' on different sides of the bin.
* Record score every day in EEPROM for up to one month.
* Show date and time if the bin is turned over or is not standing
* Show date and time overnight to conserve power

## Hardware

1. Iteaduino UNO
2. ITDB02-1.8SP - LCD Display 128 x 160
3. 2 x ULTRASONIC RANGING MODULE HC-SR04
4. Real Time Clock Module (DS1307) V1.1
5. Tilt ball switch


## Build
Install the Arduino development environment, then install [ino](http://inotool.org/).

To build type
`ino build`

to install type

`ino install`
