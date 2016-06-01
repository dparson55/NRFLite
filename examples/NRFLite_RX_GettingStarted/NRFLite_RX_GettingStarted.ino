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

const static uint32_t SERIAL_SPEED = 115200;

const static uint8_t PIN_RADIO_CE  = 9;
const static uint8_t PIN_RADIO_CSN = 10;

const static uint8_t RADIO_ID = 0;       // Our radio's id.  The transmitter will send to this id.

struct RadioPacket { uint8_t Counter; }; // Transmitter will send us these types of data packets.

NRFLite _radio;
RadioPacket _radioData;

void setup()
{
	delay(500); // Give the serial monitor a little time to get read so it shows all serial output.
	
	Serial.begin(SERIAL_SPEED);

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
	while (_radio.hasData()) {        // Returns the size of any received data packet.
		
		_radio.readData(&_radioData); // Loads data into the _radioData variable.  The & passes the variable by reference.
		
		Serial.print("Received ");
		Serial.println(_radioData.Counter);
	}
}