## NRFLite
```c++
// TX example

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
// RX example

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
[![Tutorial 2](http://img.youtube.com/vi/URMmgQuPZVc/default.jpg)](https://youtu.be/URMmgQuPZVc)

### Features
* 2-pin operation thanks to [NerdRalph](http://nerdralph.blogspot.ca/2015/05/nrf24l01-control-with-2-mcu-pins-using.html).
* 4-pin operation using shared CE and CSN pins while continuing to use the high-speed SPI and USI peripherals of the supported microcontrollers.
* Operation with or without interrupts using the radio's IRQ pin.
* ATtiny84/85 support when used with the [MIT High-Low Tech](http://highlowtech.org/?p=1695) Arduino library https://github.com/damellis/attiny.  This library uses much less memory than https://github.com/SpenceKonde/ATTinyCore and is required for the [ATtiny85 sensor example](https://github.com/dparson55/NRFLite/tree/master/examples/Sensor_TX_ATtiny85_2Pin).
* Very easy to use:  not everything the radio supports is implemented but the library has a small number of methods.
* No need to enable features like retries, auto-acknowledgment packets, and dynamic packet sizes.
* No need to add delays or implement timeouts.
* No long radio addresses to manage.

### nRF24L01+ Pin Reference

![nRF24L01 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/nRF24L01_pinout_small.jpg)

### 2-Pin Hookup Guide
* This mode is much slower than the other hookup options which take advantage of the SPI and USI peripherals of the supported microcontrollers.
* Interrupts are temporarily disabled whenever the library talks to the radio in this mode.
* The resistor and capacitor values should only be adjusted if you have an oscilloscope and are comfortable changing the library.

![2-Pin](https://github.com/dparson55/NRFLite/raw/master/extras/Two_pin_schematic.png)

### ATmega328 SPI Hookup Guide
* Arduino Pin 10 is the *SPI slave select* pin and must stay as an *OUTPUT*.
```
Radio MISO -> Arduino 12 MISO
Radio MOSI -> Arduino 11 MOSI
Radio SCK  -> Arduino 13 SCK
Radio CE   -> Any GPIO Pin (can be same as CSN)
Radio CSN  -> Any GPIO Pin (pin 10 recommended)
Radio IRQ  -> Any GPIO Pin (optional)
```
![ATmega328 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATmega328_pinout_small.jpg)

### ATtiny84 USI Hookup Guide
```
Radio MISO -> Physical Pin 7, Arduino 6
Radio MOSI -> Physical Pin 8, Arduino 5
Radio SCK  -> Physical Pin 9, Arduino 4
Radio CE   -> Any GPIO Pin (can be same as CSN)
Radio CSN  -> Any GPIO Pin
Radio IRQ  -> Any GPIO Pin (optional)
```
![ATtiny84 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny84_pinout_small.png)

### ATtiny85 USI Hookup Guide
```
Radio MISO -> Physical Pin 5, Arduino 0
Radio MOSI -> Physical Pin 6, Arduino 1
Radio SCK  -> Physical Pin 7, Arduino 2
Radio CE   -> Any GPIO Pin (can be same as CSN)
Radio CSN  -> Any GPIO Pin
Radio IRQ  -> Any GPIO Pin (optional)
```
![ATtiny85 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny85_pinout_small.png)
