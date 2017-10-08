## NRFLite
Easily send dynamically-sized data packets, with or without dynamically-sized acknowledgement packets, with less code than other libraries.

**_2-pin control support is nearly ready for release!_**  Using details on <http://nerdralph.blogspot.ca/2015/05/nrf24l01-control-with-2-mcu-pins-using.html> I have working code for an ATtiny85 which multiplexes the MOSI/MISO and CE/CSN/SCK pins.  I need to finish adding ATtiny84 and standard Arduino support, finish the examples, and create a tutorial video.

YouTube video

[![YouTube Video](http://img.youtube.com/vi/tWEgvS7Sj-8/default.jpg)](https://youtu.be/tWEgvS7Sj-8)

### Features
* No need to enable or disable features like retries, auto-acknowledgment packets, and dynamic packet sizes; those all just work out of the box.
* 4-pin operation that behaves the same as when separate pins are used.
* Supports operation with or without interrupts using the radio's IRQ pin.
* Supports ATtiny84/85 when used with the MIT High-Low Tech Arduino library http://highlowtech.org/?p=1695.
  * When using an ATtiny with an older Arduino toolchain like 1.0.5, if you get a R_AVR_13_PCREL compilation error when your sketch is >4KB, a fix is available on https://github.com/TCWORLD/ATTinyCore/tree/master/PCREL%20Patch%20for%20GCC
    
### Goals
* Small set of methods:  not everything the radio supports is exposed but the library is kept easy to use.
* No need for calling programs to add delays or implement timeouts.
* No long radio addresses for calling programs to manage.

### Connections
* CE, CSN, and IRQ are configurable.
* CE and CSN can use the same pin.
* IRQ provides interrupt support and is optional.

###### ATmega328
```
MOSI -> Arduino 11 MOSI
MISO -> Arduino 12 MISO
SCK  -> Arduino 13 SCK
```
*Arduino 10 is the hardware Slave Select pin and must stay as an OUTPUT for SPI operation.  You don't need to use pin 10
with the radio's CSN pin but you must keep it an OUTPUT.  This goes for all hardware SPI use on the ATmega328, if you set it
as an INPUT, SPI operation stops.*

###### ATtiny84
```
MOSI -> Arduino 5 USI_DO  PA5 Physical Pin 8
MISO -> Arduino 6 USI_DI  PA6 Physical Pin 7
SCK  -> Arduino 4 USI_SCK PA4 Physical Pin 9
```

###### ATtiny85
```
MOSI -> Arduino 1 USI_DO  PB1 Physical Pin 6
MISO -> Arduino 0 USI_DI  PB0 Physical Pin 5
SCK  -> Arduino 2 USI_SCK PB2 Physical Pin 7
```
