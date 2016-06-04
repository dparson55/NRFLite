/*

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

#include <SPI.h> // NRFLite uses Arduino SPI when used with an ATmega328.  This include is not necessary when using an ATtiny84/85.
#include <NRFLite.h>

const static uint8_t RADIO_ID             = 1; // Our radio's id.  Can be any 8-bit number (0-255).
const static uint8_t DESTINATION_RADIO_ID = 0; // Id of the radio we will transmit to.
const static uint8_t PIN_RADIO_CE         = 9;
const static uint8_t PIN_RADIO_CSN        = 10;

struct RadioPacket { uint8_t Counter; };       // We will transmit this type of data packet.

NRFLite _radio;
RadioPacket _radioData;

void setup()
{
	delay(500); // Give the serial monitor a little time to get ready so it shows all serial output.
	
	Serial.begin(115200);
	
	if (_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS)) {
		Serial.println("Radio initialized successfully");
	}
	else {
		Serial.println("Cannot communicate with radio");
		while (1) {} // Wait here forever.
	}
}

void loop()
{
	_radioData.Counter++;
	
	Serial.print("Send ");
	Serial.print(_radioData.Counter);
	
	if (_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(RadioPacket))) {
		Serial.println("...Success");
	}
	else {
		Serial.println("...Failed");
	}
	
	delay(1000);
}