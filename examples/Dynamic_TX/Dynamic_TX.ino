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

const static uint8_t RADIO_ID             = 1; // Our radio's id.  Can be any 8-bit number (0-255).
const static uint8_t DESTINATION_RADIO_ID = 0; // Id of the radio we will transmit to.
const static uint8_t PIN_RADIO_CE         = 9;
const static uint8_t PIN_RADIO_CSN        = 10;

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
	
	_radioData1.FromRadioId = RADIO_ID;
	_radioData2.FromRadioId = RADIO_ID;
}

void loop()
{
	// Pick a number from 10,000 - 60,000.
	uint16_t randomNumber = random(10000, 60001); 
	
	// If it is > 30000 we send a data packet with an 8-bit integer,
	// otherwise we send a data packet containing a string.
	if (randomNumber > 30000 ) {
		
		_radioData1.Counter++;
		
		Serial.print("Send ");
		Serial.print(_radioData1.Counter);
		
		// Note how we specify the size of the RadioData1 packet.
		if (_radio.send(DESTINATION_RADIO_ID, &_radioData1, sizeof(RadioPacket1))) {
			Serial.println("...Success");
		}
		else {
			Serial.println("...Failed");
		}
		
	} else {
		
		String msg = "random string " + String(randomNumber);
		
		// Convert the string to a char array, writing the array to _radioData2.Message.
		// Note the string cannot be longer than 31 characters since that is the length of _radioData2.Message.
		msg.toCharArray(_radioData2.Message, msg.length() + 1);

		Serial.print("Send ");
		Serial.print(msg);
		
		// Note how we specify the size of the RadioPacket2 packet.
		if (_radio.send(DESTINATION_RADIO_ID, &_radioData2, sizeof(RadioPacket2))) {
			Serial.println("...Success");
		}
		else {
			Serial.println("...Failed");
		}
	}
	
	delay(1000);
}