/* Demonstrates the usage of interrupts while sending and receiving acknowledgement data.

Radio -> Arduino

CE    -> 9
CSN   -> 10 (Hardware SPI SS)
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
IRQ   -> 3  (Hardware INT1)

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
	attachInterrupt(1, radioInterrupt, FALLING); // INT1 = digital pin 3
}

void loop()
{
	_data++;
	Serial.print("Send ");
	Serial.print(_data);
	_radio.startSend(0, &_data, sizeof(_data)); // start the send of _data to radio id 0
	delay(1000);
}

void radioInterrupt()
{
	uint8_t tx_ok, tx_fail, rx_ready;
	_radio.whatHappened(tx_ok, tx_fail, rx_ready);
	
	if (tx_ok) {
		if (_radio.hasAckData()) {
			uint8_t ackData;
			_radio.readData(&ackData);
			Serial.print("...Received Ack ");
			Serial.println(ackData);
		}
	}
	
	if (tx_fail) {
		Serial.println("...Fail");
	}
}
