## NRFLite
Arduino nRF24L01+ library with the goal of being easy to operate.  You can send any size of data packet supported by the radio
(up to 32 bytes), with or without acknowledgment, and send back any size of acknowledgment packet, all without having to manage
the various radio settings, TX and RX pipes, or FIFO buffers.

### Features
* Supports standard Arduino's.
* Supports ATtiny84/85 when used with the MIT High-Low Tech Arduino library http://highlowtech.org/?p=1695.
* Supports CE and CSN on the same pin.
* Supports operation with or without interrupts using the radio's IRQ pin.
    
### Goals
* Small set of methods:  not everything the radio supports is exposed but the library is kept easy to use.
* No dealing with enabling or disabling of features like retries, auto-acknowledgment packets, and dynamic packet sizes.
* Shared CE and CSN pin operation that behaves the same when separate pins are used.
* No need for calling programs to add delays or implement timeouts.
* No complicated radio addresses for calling programs to assign.
* No need for calling programs to deal with the radio's TX and RX pipes.
* No dealing with the radio's RX and TX FIFO buffers.

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
MOSI -> Arduino 5 USI_DO  PA5
MISO -> Arduino 6 USI_DI  PA6
SCK  -> Arduino 4 USI_SCK PA4
```

###### ATtiny85
```
MOSI -> Arduino 1 USI_DO  PB1
MISO -> Arduino 0 USI_DI  PB0
SCK  -> Arduino 2 USI_SCK PB2
```
