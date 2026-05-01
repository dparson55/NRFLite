### Receiver Example
```c++
#include "SPI.h"
#include "NRFLite.h"

NRFLite _radio;
uint8_t _data;

void setup()
{
    Serial.begin(115200);
    _radio.init(0, 9, 10); // Set radio to Id = 0, along with its CE (9) and CSN (10) pins
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

### Transmitter Example
```c++
#include "SPI.h"
#include "NRFLite.h"

NRFLite _radio;
uint8_t _data;

void setup()
{
    _radio.init(1, 9, 10); // Set radio to Id = 1, along with the CE (9) and CSN (10) pins
}

void loop()
{
    _data++;
    _radio.send(0, &_data, sizeof(_data)); // Send data to the radio with Id = 0
    delay(1000);
}
```

### Features
* Very small number of public methods to learn, see [NRFLite.h](https://github.com/dparson55/NRFLite/blob/master/src/NRFLite.h) for their descriptions.
* No long radio addresses, no need to enable features like retries, auto-acknowledgment packets, dynamic packet sizes, or setting the transmit power (max is enabled by default).
* 2-pin operation on AVR microcontrollers (ATtiny/ATmega) inspired by [NerdRalph's article](http://nerdralph.blogspot.ca/2015/05/nrf24l01-control-with-2-mcu-pins-using.html).
* 4-pin operation using shared CE and CSN pins.
* Operation with or without interrupts using the radio's IRQ pin.
* Installation via the Arduino library manager (just search for ```nrflite```).
* [Compatibility](https://github.com/dparson55/NRFLite/issues/54) with the [RF24](https://github.com/nRF24/RF24) library.

### Video Tutorials
* [![Tutorial 1](http://img.youtube.com/vi/tWEgvS7Sj-8/default.jpg)](https://youtu.be/tWEgvS7Sj-8) Introduction and all basic features
* [![Tutorial 2](http://img.youtube.com/vi/URMmgQuPZVc/default.jpg)](https://youtu.be/URMmgQuPZVc) 2-pin hookup and ATtiny85 sensor walkthrough

### nRF24L01+ Pin Reference

![nRF24L01 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/nRF24L01_pinout_small.jpg)

### 2-Pin Hookup Guide
* The GPIO pins you select on the microcontroller should not share any additional components, eg. a Digispark contains an LED on PB1 and USB connections on PB3 and PB4, so do not use those pins.
* The R2 resistor does not need to be exactly 5K, anything between 4K and 6K is good. I often use two 10K resistors in parallel to create 5K.
* This mode is significantly slower than other hookup options so only use it when speed is not needed.

![2-Pin](https://github.com/dparson55/NRFLite/raw/master/extras/Two_pin_schematic.png)

### ATmega328 SPI Hookup Guide
```
Radio MISO -> Arduino 12 MISO
Radio MOSI -> Arduino 11 MOSI
Radio SCK  -> Arduino 13 SCK
Radio CE   -> Any GPIO Pin (can be same as CSN)
Radio CSN  -> Any GPIO Pin (pin 10 recommended)
Radio IRQ  -> Any GPIO Pin (optional)
```
_Arduino Pin 10 is the SPI Slave Select (SS) pin and must stay as an OUTPUT._

![ATmega328 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATmega328_pinout_small.jpg)

### ATtiny84 USI Hookup Guide
```
Radio MISO -> Physical Pin 7
Radio MOSI -> Physical Pin 8
Radio SCK  -> Physical Pin 9
Radio CE   -> Any GPIO Pin (can be same as CSN)
Radio CSN  -> Any GPIO Pin
Radio IRQ  -> Any GPIO Pin (optional)
```
_Arduino pin names (pictured in brown) using the [MIT High-Low Tech](http://highlowtech.org/?p=1695) library https://github.com/damellis/attiny._

![ATtiny84 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny84_pinout_small.png)

### ATtiny85 USI Hookup Guide
```
Radio MISO -> Physical Pin 5
Radio MOSI -> Physical Pin 6
Radio SCK  -> Physical Pin 7
Radio CE   -> Any GPIO Pin (can be same as CSN)
Radio CSN  -> Any GPIO Pin
Radio IRQ  -> Any GPIO Pin (optional)
```
_Arduino pin names (pictured in brown) using the [MIT High-Low Tech](http://highlowtech.org/?p=1695) library https://github.com/damellis/attiny._

![ATtiny85 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny85_pinout_small.png)
