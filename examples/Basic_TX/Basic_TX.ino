/* Demonstrates the most basic example of RX and TX operation.

Radio -> Arduino

CE    -> 9
CSN   -> 10 (Hardware SPI SS)
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
IRQ   -> No connection in this example

VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include <SPI.h>
#include <NRFLite.h>

NRFLite _radio;
uint8_t _data;

void setup()
{
	Serial.begin(115200);
	_radio.init(1, 9, 10); // radio id, CE pin, CSN pin
}

void loop()
{
	_data++;
	Serial.print("Send "); Serial.println(_data);
	_radio.send(0, &_data, sizeof(_data)); // send _data to radio id 0
	delay(1000);
}