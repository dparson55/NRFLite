**_Nov 18: With the help of a Rigol 1054, 2-pin mode is now working!  It is flawless when using the default 2MBPS radio data rate but 1MBPS and 250KBPS rates are experience a few dropped packets in certain situations.  I think it is good enough for release though so am starting work on examples next.
**_Oct 9: 2-pin support is in progress!_**  Using details on <http://nerdralph.blogspot.ca/2015/05/nrf24l01-control-with-2-mcu-pins-using.html> NRFLite now has logic for an ATtiny85 @ 8MHz which multiplexes the MOSI/MISO and CE/CSN/SCK pins.  I got this working without the aid of an oscilloscope but haven't had any luck running the ATtiny @ 1 MHz, so finally have a reason to purchase one.  It should be here in a week or so. 
- [x] POC for ATtiny85 @ 8MHz.
- [x] Test main features:  bitrates, dynamic packets, ack packets, and interrupts.
- [x] Add ATtiny84 support.
- [x] Add standard Arduino support.
- [x] Perform testing of the ATtiny85 @ 1MHz, ATtiny84 @ 1MHz and 8MHz, and Arduino Uno @ 16MHz.
- [ ] Create examples.
- [ ] Perform release (will be version 2.0.0).  Arduino development environments < 1.5 will no longer be supported.
- [ ] Create tutorial video.

## NRFLite
Easily send dynamically-sized data packets, with or without dynamically-sized acknowledgement packets, with less code than other libraries.

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
* 4-pin and soon even 2-pin operation that behaves the same as when separate pins are used.
* Operation with or without interrupts using the radio's IRQ pin.
* Supports ATtiny84/85 when used with the MIT High-Low Tech Arduino library http://highlowtech.org/?p=1695.
* Small set of methods:  not everything the radio supports is exposed but the library is kept easy to use.
* No need to enable or disable features like retries, auto-acknowledgment packets, and dynamic packet sizes; those simply work out of the box.
* No need for calling programs to add delays or implement timeouts.
* No long radio addresses for calling programs to manage.

### Connections
###### nRF24L01+

![nRF24L01 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/nRF24L01_pinout_small.jpg)

###### 2-Pin Operation

![2-Pin](https://github.com/dparson55/NRFLite/raw/master/extras/Two_pin_schematic.png)

###### ATmega328
* Arduino Pin 10 is the *hardware SPI slave select* pin and must stay as an OUTPUT.
```
// Hardware SPI Operation
Radio SCK  -> Arduino 13 SCK
Radio MISO -> Arduino 12 MISO
Radio MOSI -> Arduino 11 MOSI
Radio CE   -> Any Arduino pin
Radio CSN  -> Any Arduino pin (can be same as CE)
Radio IRQ  -> Any Arduino pin (optional)
```
![ATmega328 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATmega328_pinout_small.jpg)

###### ATtiny84
```
// Hardware USI Operation
Radio MISO -> Physical Pin 7 PA6 Arduino 6
Radio MOSI -> Physical Pin 8 PA5 Arduino 5
Radio SCK  -> Physical Pin 9 PA4 Arduino 4
Radio CE   -> Any Arduino pin
Radio CSN  -> Any Arduino pin (can be same as CE)
Radio IRQ  -> Any Arduino pin (optional)
```
![ATtiny84 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny84_pinout_small.png)

###### ATtiny85
```
// Hardware USI Operation
Radio SCK  -> Physical Pin 7 PB2 Arduino 2
Radio MOSI -> Physical Pin 6 PB1 Arduino 1
Radio MISO -> Physical Pin 5 PB0 Arduino 0
Radio CE   -> Any Arduino pin
Radio CSN  -> Any Arduino pin (can be same as CE)
Radio IRQ  -> Any Arduino pin (optional)
```
![ATtiny85 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny85_pinout_small.png)
