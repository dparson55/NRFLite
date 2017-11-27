/*

Demonstrates transmitting data with an ATtiny84.  ATtiny's have a Universal Serial Interface 
peripheral (USI) that can be used for SPI communication, and NRFLite takes advantage of this capability.

Radio -> ATtiny84

CE    -> Physical Pin 10, Arduino 3         
CSN   -> Physical Pin 10, Arduino 3         
MOSI  -> Physical Pin 8 , Arduino 5 (Hardware USI_DO)
MISO  -> Physical Pin 7 , Arduino 6 (Hardware USI_DI)
SCK   -> Physical Pin 9 , Arduino 4 (Hardware USI_SCK)
IRQ   -> No connection in this example
VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include <NRFLite.h>
#include <SoftwareSerial.h>

const static uint32_t SERIAL_SPEED = 9600;

const static uint8_t RADIO_ID             = 3;
const static uint8_t DESTINATION_RADIO_ID = 2;
const static uint8_t PIN_RADIO_CE         = 3;
const static uint8_t PIN_RADIO_CSN        = 3;
const static uint8_t PIN_SOFT_SERIAL_TX   = 8;

struct RadioPacket
{
	uint8_t Data[32];
};

NRFLite _radio;
RadioPacket _radioData;
SoftwareSerial _serial(99, PIN_SOFT_SERIAL_TX); // RX,TX - and RX is a bogus pin since we won't use it.

uint32_t _successPacketCount, _failedPacketCount, _bitsPerSecond, _lastMillis

void setup()
{
	_serial.begin(SERIAL_SPEED);
	
	if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
		_serial.println("Cannot communicate with radio");
		while (1) {} // Wait here forever.
	}
}

void loop()
{
    // Once a second report the number of successful and failed packet sends.
	if (millis() - _lastMillis > 999)
    {
		_bitsPerSecond = sizeof(_radioData) * _successPacketCount * 8;

		_serial.print(_successPacketCount);
		_serial.print(" Success ");
		_serial.print(_failedPacketCount);
		_serial.print(" Failed - ");
		_serial.print(_bitsPerSecond);
		_serial.println(" bps");
		
		_successPacketCount = 0;
		_failedPacketCount = 0;
		_lastMillis = millis();
	}

	if (_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData)))
    {
		_successPacketCount++;
	}
	else
    {
		_failedPacketCount++;
	}
}