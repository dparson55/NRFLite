/*

Radio -> ATtiny84 Arduino pin

CE    -> Arduino 3         PB3 Physical Pin 10
CSN   -> Arduino 3         PB3 Physical Pin 10
MOSI  -> Arduino 5 USI_DO  PA5 Physical Pin 8
MISO  -> Arduino 6 USI_DI  PA6 Physical Pin 7
SCK   -> Arduino 4 USI_SCK PA4 Physical Pin 9

CE    -> 3
CSN   -> 3
MOSI  -> 5 (USI DO)
MISO  -> 6 (USI DI)
SCK   -> 4 (USI SCK)
IRQ   -> No connection in this example

VCC   -> 9 (ATtiny running with 3.3 volts will power the radio with this pin)
GND   -> GND

*/

#include <NRFLite.h>
#include <SoftwareSerial.h>

const static uint32_t SERIAL_SPEED = 9600;

const static uint8_t PIN_RADIO_CE       = 3;
const static uint8_t PIN_RADIO_CSN      = 3;
const static uint8_t PIN_SOFT_SERIAL_TX = 8;
const static uint8_t PIN_RADIO_POWER    = 9;

const static uint8_t RADIO_ID             = 2; // Our radio's id.  Can be any 8-bit number (0-255).
const static uint8_t DESTINATION_RADIO_ID = 0; // Id of the radio we will transmit to.

struct RadioPacket { uint8_t Data[32]; };       // We will transmit this type of data packet.

SoftwareSerial _serial(99, PIN_SOFT_SERIAL_TX); // RX,TX - and RX is a bogus pin since we won't use it.
NRFLite _radio;
RadioPacket _radioData;

uint32_t _successPacketCount, _failedPacketCount, _bitsPerSecond;
uint64_t _lastMillis;

void setup()
{
	delay(6000); // Wait for Windows to recognize COM port after programming via AVR Dragon.
	_serial.begin(SERIAL_SPEED);
	
	// Provide power to the radio.  I tried adding resistors between the radio's SPI pins and
	// ATtiny84, but no combination of resistor values or SPI pins would allow a successful program.
	// So this is an easy work-around to allow programming with the AVR Dragon.
	// The ATtiny can supply 40 mA per pin and nRF24L01+ uses no more than 13.5 mA.
	pinMode(PIN_RADIO_POWER, OUTPUT); digitalWrite(PIN_RADIO_POWER, HIGH);
	
	if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS)) {
		_serial.println("Cannot communicate with radio");
		while (1) {} // Wait here forever.
	}
}

void loop()
{
	if (millis() - _lastMillis > 999) {
		
		_bitsPerSecond = sizeof(RadioPacket) * _successPacketCount * 8;

		_serial.print(_successPacketCount);
		_serial.print(" Success ");
		_serial.print(_failedPacketCount);
		_serial.print(" Fail - ");
		_serial.print(_bitsPerSecond);
		_serial.println(" bps");
		
		_successPacketCount = 0;
		_failedPacketCount = 0;
		_lastMillis = millis();
	}

	if (_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(RadioPacket), NRFLite::REQUIRE_ACK)) {
		_successPacketCount++;
	}
	else {
		_failedPacketCount++;
	}
}