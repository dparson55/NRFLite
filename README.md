**_Nov 18: 2-pin mode is now working!_**  After learning to use an oscilloscope and doing lots of troubleshooting, operation with the 2MBPS data rate is working perfectly.  1MBPS and 250KBPS data rates experience a few dropped packets in certain situations but I think they are good enough for release.  I started work on the examples and should have everything released by the end of the month.
- [x] Implement ATtiny85 @ 1MHz, ATtiny84 @ 1MHz and 8MHz, and Arduino Uno @ 16MHz.
- [ ] Create examples.
- [ ] Perform release (will be version 2.0.0).  Arduino development environments < 1.5 will no longer be supported.
- [ ] Create tutorial video.

**_Oct 9: 2-pin support is in progress!_**  Using details on <http://nerdralph.blogspot.ca/2015/05/nrf24l01-control-with-2-mcu-pins-using.html> NRFLite now has logic for an ATtiny85 @ 8MHz which multiplexes the MOSI/MISO and CE/CSN/SCK pins.  I got this working without the aid of an oscilloscope but haven't had any luck running the ATtiny @ 1 MHz, so finally have a reason to purchase one.  It should be here in a week or two. 
- [x] POC for ATtiny85 @ 8MHz.
- [x] Test main features:  bitrates, dynamic packets, ack packets, and interrupts.
- [x] Add ATtiny84 support.
- [x] Add standard Arduino support.

## NRFLite
Easily send and receive data wirelessly with less code than other libraries.

```c++
// Basic TX example

#include <SPI.h>
#include <NRFLite.h>

NRFLite _radio;
uint8_t _data;

void setup()
{
    _radio.init(0, 9, 10); // Set radio Id = 0, along with the CE and CSN pins
}

void loop()
{
    _data++;
    _radio.send(1, &_data, sizeof(_data)); // Send to the radio with Id = 1
    delay(1000);
}
```
```c++
// Basic RX example

#include <SPI.h>
#include <NRFLite.h>

NRFLite _radio;
uint8_t _data;

void setup()
{
    Serial.begin(115200);
    _radio.init(1, 9, 10); // Set this radio's Id = 1, along with its CE and CSN pins
}

void loop()
{
    while (_radio.hasData())
    {
        _radio.readData(&_data);
        Serial.println(_data);
    }
}
```

### Video Tutorials

[![Tutorial 1](http://img.youtube.com/vi/tWEgvS7Sj-8/default.jpg)](https://youtu.be/tWEgvS7Sj-8)

### Features
* 2-pin operation thanks to http://nerdralph.blogspot.ca/2015/05/nrf24l01-control-with-2-mcu-pins-using.html.
* 4-pin operation using shared CE and CSN pins.
* Operation with or without interrupts using the radio's IRQ pin.
* ATtiny84/85 support when used with the MIT High-Low Tech Arduino library http://highlowtech.org/?p=1695.
* Very easy to use:  not everything the radio supports is implemented but the library has a very small set of methods.
* No need to enable features like retries, auto-acknowledgment packets, and dynamic packet sizes; they simply work.
* No need to add delays or implement timeouts.
* No long radio addresses to manage.

### nRF24L01+ Pin Reference

![nRF24L01 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/nRF24L01_pinout_small.jpg)

### 2-Pin Hookup Guide
* Not surprising but this mode is much slower than the other hookup options which take advantage of the SPI and USI peripherals of the microcontroller.
* Keep in mind that the library temporarily disables interrupts whenever it must communicate with the radio.
* The resistor and capacitor values should only be adjusted if you know what you are doing.  After lots of experimentation and measurement, they were selected for their common values and ability to work with various microcontroller frequencies, operating voltages, radio data rates, and radio packet sizes.  Timing within the library depends upon these specific component values.

![2-Pin](https://github.com/dparson55/NRFLite/raw/master/extras/Two_pin_schematic.png)

### ATmega328 SPI Hookup Guide
* Arduino Pin 10 is the *hardware SPI slave select* pin and must stay as an *OUTPUT*.
```
Radio MISO -> Arduino 12 MISO
Radio MOSI -> Arduino 11 MOSI
Radio SCK  -> Arduino 13 SCK
Radio CE   -> Any Arduino pin
Radio CSN  -> Any Arduino pin (can be same as CE)
Radio IRQ  -> Any Arduino pin (optional)
```
![ATmega328 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATmega328_pinout_small.jpg)

### ATtiny84 USI Hookup Guide
```
Radio MISO -> Physical Pin 7, Arduino 6
Radio MOSI -> Physical Pin 8, Arduino 5
Radio SCK  -> Physical Pin 9, Arduino 4
Radio CE   -> Any Arduino pin
Radio CSN  -> Any Arduino pin (can be same as CE)
Radio IRQ  -> Any Arduino pin (optional)
```
![ATtiny84 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny84_pinout_small.png)

### ATtiny85 USI Hookup Guide
```
Radio MISO -> Physical Pin 5, Arduino 0
Radio MOSI -> Physical Pin 6, Arduino 1
Radio SCK  -> Physical Pin 7, Arduino 2
Radio CE   -> Any Arduino pin
Radio CSN  -> Any Arduino pin (can be same as CE)
Radio IRQ  -> Any Arduino pin (optional)
```
![ATtiny85 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny85_pinout_small.png)
