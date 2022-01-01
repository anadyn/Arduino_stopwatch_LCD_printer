# Arduino_stopwatch_LCD_printer
Arduino sketch for lap time measurements using external gates as triggers.

![Overview beardboard setup.](https://github.com/anadyn/Arduino_stopwatch_LCD_printer/blob/main/overview_arduino_stopwatch_breadboard.jpg)

Features:
* Measures total time and optional split time, maximum time measured is 60 minutes. 
* LCD screen with 4x20 characters.
* The measured times are shown as minutes:seconds.hundreds of second
* Four inputs for timing gates, each can be configured for the split time measurement.
* Manual start/stop button in addition to timing gates
* Thermo printer that prints results, with switch to turn on/off
* Switch+button to set split gate 1-4 or none
* LED that blinks when measurement is active
* Timestamps are sent on the serial port (UART), on the format mmssuu, where mm=minutes, ss=seconds and uu=hundreds of seconds
* The previous two times are shown together with the latest time.
* Cool animation on the LCD while measurement is running :-)

Pin layout:

* Timing gates:  digital pin 8, 9, 10, 11
* Manual start/stop button:  digital pin 12 (cannot be used as split gate)
* LED indicator:  digital pin 13 (add 330 ohm resistor in series)
* Switch split time on/off: digital 14 (analog A0)
* Switch printer on/off:  digital 15 (analog A1)

Serial port for sending timestamps:
* RX on digital pin 0
* TX on digital pin 1

Printer:
* RX on printer to digital pin 19 (analog A5)
* TX on printer to digital pin 18 (analog A4)
(connect GND pin on printer to GND on Arduino)

LCD:
* LCD RS pin to digital pin 6
* LCD Enable pin to digital pin 7
* LCD D4 pin to digital pin 2
* LCD D5 pin to digital pin 3
* LCD D6 pin to digital pin 4
* LCD D7 pin to digital pin 5
* LCD R/W pin to ground
* 10K potentiometer:
  Outer pins to +5V and ground.
  Potentiometer wiper to LCD VO pin (LCD pin 3)

