**_2-pin support is in progress!_**  Using details on <http://nerdralph.blogspot.ca/2015/05/nrf24l01-control-with-2-mcu-pins-using.html> NRFLite has working code for an ATtiny85 which multiplexes the MOSI/MISO and CE/CSN/SCK pins.
- [x] POC for ATtiny85.
- [x] Test main features:  bitrates, dynamic packets, ack packets, and interrupts.
- [ ] Add ATtiny84 support.
- [ ] Add standard Arduino support.
- [ ] Create examples.
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
* 4-pin operation that behaves the same as when separate pins are used.
* Operation with or without interrupts using the radio's IRQ pin.
* Supports ATtiny84/85 when used with the MIT High-Low Tech Arduino library http://highlowtech.org/?p=1695.
  * When using an ATtiny with an older Arduino toolchain like 1.0.5, if you get a R_AVR_13_PCREL compilation error when your sketch is >4KB, a fix is available on https://github.com/TCWORLD/ATTinyCore/tree/master/PCREL%20Patch%20for%20GCC
    
### Goals
* Small set of methods:  not everything the radio supports is exposed but the library is kept easy to use.
* No need to enable or disable features like retries, auto-acknowledgment packets, and dynamic packet sizes; those simply work out of the box.
* No need for calling programs to add delays or implement timeouts.
* No long radio addresses for calling programs to manage.

### Connections
###### nRF24L01+
* IRQ provides interrupt support and is optional.
* CE and CSN can use the same pin on the microcontroller.

![nRF24L01 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/nRF24L01_pinout_small.jpg)

###### ATmega328
```
Radio SCK  -> Arduino 13 SCK
Radio MISO -> Arduino 12 MISO
Radio MOSI -> Arduino 11 MOSI
Radio CE   -> Any Arduino pin
Radio CSN  -> Any Arduino pin (can be same as CE)
Radio IRQ  -> Any Arduino pin (optional)
```
*Arduino 10 is the hardware Slave Select pin and must stay as an OUTPUT for SPI operation.*
![ATmega328 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATmega328_pinout_small.jpg)

###### ATtiny84
```
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
Radio SCK  -> Physical Pin 7 PB2 Arduino 2
Radio MOSI -> Physical Pin 6 PB1 Arduino 1
Radio MISO -> Physical Pin 5 PB0 Arduino 0
Radio CE   -> Any Arduino pin
Radio CSN  -> Any Arduino pin (can be same as CE)
```
![ATtiny85 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/ATtiny85_pinout_small.png)
