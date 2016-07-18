/* Demonstrates sending data packets of different length.  The receiver will check to see what size of
   packet was received and act according based on the type of packet.

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

const static uint8_t RADIO_ID      = 0;  // Our radio's id.  The transmitter will send to this id.
const static uint8_t PIN_RADIO_CE  = 9;
const static uint8_t PIN_RADIO_CSN = 10;

struct RadioPacket1 {
	uint8_t FromRadioId; // We'll load this with the Id of the radio who sent the packet.
	uint8_t Counter;
};

struct RadioPacket2 {
	uint8_t FromRadioId; // We'll load this with the Id of the radio who sent the packet.
	char Message[31];    // Note the max total packet size is 32 so 31 is all we can use here.
};

NRFLite _radio;
RadioPacket1 _radioData1;
RadioPacket2 _radioData2;

void setup()
{
	delay(500); // Give the serial monitor a little time to get ready so it shows all serial output.
	
	Serial.begin(115200);

	if (_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN)) {
		Serial.println("Radio initialized");
	}
	else {
		Serial.println("Cannot communicate with radio");
		while (1) {} // Wait here forever.
	}
}

void loop()
{
	uint8_t packetSize = _radio.hasData(); // hasData returns 0 if there is data packet.
	
	if (packetSize == sizeof(RadioPacket1)) {
		
		_radio.readData(&_radioData1); // Loads data into the _radioData1 variable.  The & passes the variable by reference.
		
		Serial.print("Received ");
		Serial.print(_radioData1.Counter);
		Serial.print(" from radio ");
		Serial.println(_radioData1.FromRadioId);
	}
	else if (packetSize == sizeof(RadioPacket2)) {

		_radio.readData(&_radioData2); // Loads data into the _radioData2 variable.  The & passes the variable by reference.
		
		String msg = String(_radioData2.Message);

		Serial.print("Received ");
		Serial.print(msg);
		Serial.print(" from radio ");
		Serial.println(_radioData2.FromRadioId);		
	}
}