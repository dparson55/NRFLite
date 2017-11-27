/*

Demonstrates receiving data with an ATtiny85.  ATtiny's have a Universal Serial Interface
peripheral (USI) that can be used for SPI communication, and NRFLite takes advantage of this capability.

Radio -> ATtiny85

CE    -> Physical Pin 2, Arduino 3         
CSN   -> Physical Pin 2, Arduino 3         
MOSI  -> Physical Pin 6, Arduino 1 (Hardware USI_DO)  
MISO  -> Physical Pin 5, Arduino 0 (Hardware USI_DI)  
SCK   -> Physical Pin 7, Arduino 2 (Hardware USI_SCK) 
IRQ   -> No connection in this example
VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include <NRFLite.h>
#include <SoftwareSerial.h>

const static uint32_t SERIAL_SPEED = 9600;

const static uint8_t RADIO_ID           = 2;
const static uint8_t PIN_RADIO_CE       = 3;
const static uint8_t PIN_RADIO_CSN      = 3;
const static uint8_t PIN_SOFT_SERIAL_TX = 4;


struct RadioPacket
{
	uint8_t Data[32];
};

NRFLite _radio;
RadioPacket _radioData;
SoftwareSerial _serial(99, PIN_SOFT_SERIAL_TX); // RX,TX - and RX is a bogus pin since we won't use it.

uint32_t _packetCount, _bitsPerSecond, _lastMillis;

void setup()
{
	_serial.begin(SERIAL_SPEED);
	
	if (!_radio.init(RADIO_ID, RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
		_serial.println("Cannot communicate with radio");
		while (1) {} // Wait here forever.
	}
}

void loop()
{
	while (_radio.hasData())
    {
		_radio.readData(&_radioData);
		_packetCount++;
	}
	
    // Once a second report how many packets we've received.
	if (millis() - _lastMillis > 999)
    {
		bitsPerSecond = sizeof(_radioData) * _packetCount * 8 / (float)(millis() - _lastMillis) * 1000;
		
		_serial.print(_packetCount); 
		_serial.print(" packets "); 
		_serial.print(bitsPerSecond);
		_serial.println(" bps");
		
		_packetCount = 0;
		_lastMillis = millis();
	}
}