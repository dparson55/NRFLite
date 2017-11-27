/*

Demonstrates simple RX and TX operation.
Please read the notes in NRFLite.h for a description of each library feature.

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

const static uint8_t RADIO_ID      = 0;  // Our radio's id.  The transmitter will send to this id.
const static uint8_t PIN_RADIO_CE  = 9;
const static uint8_t PIN_RADIO_CSN = 10;

NRFLite _radio;
uint8_t _data;

void setup()
{
	Serial.begin(115200);

	if (!_radio.init(RADIO_ID, RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
		Serial.println("Cannot communicate with radio");
		while (1) {} // Wait here forever.
	}
}

void loop()
{
	while (_radio.hasData())
    {
		_radio.readData(&_data); // Note how & must be placed in front of the variable name.
        Serial.print("Received ");
		Serial.println(_data);
	}
}
