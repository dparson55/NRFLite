### Transmitter Example
See [Basic_TX.ino](https://github.com/dparson55/NRFLite/blob/master/examples/Basic_TX/Basic_TX.ino) for a more complete example.
```c++
#include "SPI.h"
#include "NRFLite.h"

NRFLite _radio;
uint8_t _data;

void setup()
{
    _radio.init(1, 9, 10); // Set radio to Id = 1, along with the CE and CSN pins
}

void loop()
{
    _data++;
    _radio.send(0, &_data, sizeof(_data)); // Send data to the radio with Id = 0
    delay(1000);
}
```

### Receiver Example
See [Basic_RX.ino](https://github.com/dparson55/NRFLite/blob/master/examples/Basic_RX/Basic_RX.ino) for a more complete example.
```c++
#include "SPI.h"
#include "NRFLite.h"

NRFLite _radio;
uint8_t _data;

void setup()
{
    Serial.begin(115200);
    _radio.init(0, 9, 10); // Set radio to Id = 0, along with its CE and CSN pins
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

### PA+LNA nRF24L01 Limitation
* Issues [44](https://github.com/dparson55/NRFLite/issues/44), [56](https://github.com/dparson55/NRFLite/issues/56), [63](https://github.com/dparson55/NRFLite/issues/63), [66](https://github.com/dparson55/NRFLite/issues/66), [77](https://github.com/dparson55/NRFLite/issues/77) have been raised about PA+LNA nRF24L01+ modules not working correctly when using the automatic acknowledgement (ACK) feature of the radio.  The nRF24L01+ chip itself does not provide settings to solve this incompatibiity so a work around is to implement ACK manually in software.  The `TwoWayCom_SoftwareBased` examples can be used as a starting point.
* Consider using [RFM69](https://github.com/LowPowerLab/RFM69) modules rather than PA+LNA nRF24L01+ modules when needing a long range, low power solution.  The nRF24L01+ chip is well suited for short range, high bitrate projects while the RFM69 excels in longer range, lower bitrate applications.

### Video Tutorials
* [![Tutorial 1](http://img.youtube.com/vi/tWEgvS7Sj-8/default.jpg)](https://youtu.be/tWEgvS7Sj-8) Introduction and all basic features
* [![Tutorial 2](http://img.youtube.com/vi/URMmgQuPZVc/default.jpg)](https://youtu.be/URMmgQuPZVc) 2-pin Operation and ATtiny sensor walkthrough

### Installation
* Start the Arduino IDE.
* Open the Library Manager by selecting the menu item Sketch > Include library > Manage Libraries.
* Search for 'nrflite'.
* Select the latest version and click the Install button.
* View examples in the menu File > Examples > NRFLite.

### Features
* 2-pin operation on many ATtiny and ATmega microcontrollers thanks to [NerdRalph](http://nerdralph.blogspot.ca/2015/05/nrf24l01-control-with-2-mcu-pins-using.html).
* 4-pin operation using shared CE and CSN pins while continuing to use the high-speed SPI and USI peripherals of the supported microcontrollers.
* Operation with or without interrupts using the radio's IRQ pin.
* ATtiny84/85 support when used with the [MIT High-Low Tech](http://highlowtech.org/?p=1695) library https://github.com/damellis/attiny.  This library uses much less memory than https://github.com/SpenceKonde/ATTinyCore and is required for the [ATtiny85 sensor example](https://github.com/dparson55/NRFLite/tree/master/examples/Sensor_TX_ATtiny85_2Pin).
* Small number of public methods.  Please see [NRFLite.h](https://github.com/dparson55/NRFLite/blob/master/src/NRFLite.h) for their descriptions.
* No need to enable features like retries, auto-acknowledgment packets, and dynamic packet sizes.
* No need to add delays or implement timeouts.
* No long radio addresses to manage.
* No need to set the transmit power (max is enabled by default).
* [Compatibility with RF24 library](https://github.com/dparson55/NRFLite/issues/54)

### nRF24L01+ Pin Reference

![nRF24L01 Pinout](https://github.com/dparson55/NRFLite/raw/master/extras/nRF24L01_pinout_small.jpg)

### 2-Pin Hookup Guide
* This mode is much slower than the other hookup options which take advantage of the SPI and USI peripherals of the supported microcontrollers.
* The GPIO pins you select on the microcontroller should not share any additional components, e.g. a Digispark board contains an LED on PB1 and USB connections on PB3 and PB4, so do not use these pins.
* The R2 resistor does not need to be exactly 5K, anything between 4K and 6K is good.

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
