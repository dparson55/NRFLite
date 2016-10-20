/*

Radio -> ATtiny85 Arduino pin

CE    -> Arduino 3         PB3 Physical Pin 2
CSN   -> Arduino 3         PB3 Physical Pin 2
MOSI  -> Arduino 1 USI_DO  PB1 Physical Pin 6
MISO  -> Arduino 0 USI_DI  PB0 Physical Pin 5
SCK   -> Arduino 2 USI_SCK PB2 Physical Pin 7
IRQ   -> No connection in this example
      
VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include <NRFLite.h>
#include <SoftwareSerial.h>

const static uint32_t SERIAL_SPEED = 9600;

const static uint8_t PIN_RADIO_CE       = 3;
const static uint8_t PIN_RADIO_CSN      = 3;
const static uint8_t PIN_SOFT_SERIAL_TX = 4;

const static uint8_t RADIO_ID = 0;        // Our radio's id.  The transmitter will send to this id.

struct RadioPacket { uint8_t Data[32]; }; // Transmitter will send us these types of data packets.

NRFLite _radio;
RadioPacket _radioData;
SoftwareSerial _serial(99, PIN_SOFT_SERIAL_TX); // RX,TX - and RX is a bogus pin since we won't use it.
uint32_t _packetCount = 0;
uint64_t _lastMillis = 0;

void setup()
{
	delay(6000); // Wait for Windows to recognize COM port after programming via AVR Dragon.
	_serial.begin(SERIAL_SPEED);
	
	if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS)) {
		_serial.println("Cannot communicate with radio");
		while (1) {} // Wait here forever.
	}
}

void loop()
{
	while (_radio.hasData()) {
		_radio.readData(&_radioData);
		_packetCount++;
	}
	
	if (millis() - _lastMillis > 999) {
		
		uint32_t bitsPerSecond = sizeof(RadioPacket) * _packetCount * 8 / (float)(millis() - _lastMillis) * 1000;
		
		_serial.print(_packetCount); 
		_serial.print(" packets "); 
		_serial.print(bitsPerSecond);
		_serial.println(" bps");
		
		_packetCount = 0;
		_lastMillis = millis();
	}
}