/*

Radio -> Arduino

CE    -> 9 or 10 depending on test condition
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

const static uint8_t RADIO_ID             = 0;
const static uint8_t PIN_RADIO_CE         = 10; // 9 or 10 depending on test condition
const static uint8_t PIN_RADIO_CSN        = 10;

#define SERIAL_SPEED 115200
#define debug(input)            { Serial.print(input);   }
#define debugln(input)          { Serial.println(input); }
#define debugln2(input1,input2) { Serial.print(input1); Serial.println(input2); }

enum RadioStates { StartSync, RunDemos };

struct RadioPacket     { uint8_t Counter; RadioStates RadioState; uint8_t Data[29]; };
struct RadioAckPacketA { uint8_t Counter; uint8_t Data[31]; };
struct RadioAckPacketB { uint8_t Counter; uint8_t Data[30]; };

NRFLite _radio;
RadioPacket _radioData;
RadioAckPacketA _radioDataAckA;
RadioAckPacketB _radioDataAckB;

const uint16_t DEMO_LENGTH_MILLIS = 4250;
const uint16_t DEMO_INTERVAL_MILLIS = 250;

uint8_t _showMessageInInterrupt, _addAckInInterrupt;
uint32_t _bitsPerSecond, _packetCount, _ackAPacketCount, _ackBPacketCount;
uint64_t _currentMillis, _lastMillis, _endMillis;

void setup() {
	
	Serial.begin(SERIAL_SPEED);
	delay(500); // Needed this to allow serial output to show the first message.
	
	if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS)) {
		debugln("Cannot communicate with radio");
		while (1) {} // Sit here forever.
	}
}

void loop() {
	
	debugln();
	debugln("250KBPS bitrate");
	_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS);
	runDemos();
	
	debugln();
	debugln("1MBPS bitrate");
	_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE1MBPS);
	runDemos();
	
	debugln();
	debugln("2MBPS bitrate");
	_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS);
	runDemos();
}

void radioInterrupt() {
	
	uint8_t tx_ok, tx_fail, rx_ready;
	
	_radio.whatHappened(tx_ok, tx_fail, rx_ready);
	
	if (rx_ready) {

		while (_radio.hasDataISR()) {
			
			_packetCount++;
			
			_radio.readData(&_radioData);
			if (_showMessageInInterrupt) debugln2("  Received ", _radioData.Counter);
			
			if (_addAckInInterrupt) {
				
				// Randomly pick the type of ACK packet to enqueue. 2/3 will be A, 1/3 will be B.
				if (random(3) == 1) {
					_radio.addAckData(&_radioDataAckB, sizeof(RadioAckPacketB));
					_ackBPacketCount++;
				}
				else {
					_radio.addAckData(&_radioDataAckA, sizeof(RadioAckPacketA));
					_ackAPacketCount++;
				}
			}
		}
	}
}

void runDemos() {
	startSync();
	demoPolling();
	demoInterrupts();
	demoAckPayload();
	demoPollingBitrate();
	demoInterruptsBitrate();
	demoPollingBitrateNoAck();
	demoInterruptsBitrateNoAck();
	demoPollingBitrateAckPayload();
	demoInterruptsBitrateAckPayload();
	demoPollingBitrateAllPacketSizes();
	demoPollingBitrateAllAckSizes();
}

void startSync() {
	
	debugln("  Starting sync");
	
	detachInterrupt(1);
	
	while (true) {
		if (_radio.hasData()) {
			_radio.readData(&_radioData);
			if (_radioData.RadioState == StartSync) break;
		}
	}
}

void demoPolling() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Polling");
	
	_endMillis = millis() + DEMO_LENGTH_MILLIS;
	
	while (millis() < _endMillis) {
		while (_radio.hasData()) {
			_radio.readData(&_radioData);
			debugln2("  Received ", _radioData.Counter);
		}
	}
}

void demoInterrupts() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Interrupts");
	
	attachInterrupt(1, radioInterrupt, FALLING);
	_showMessageInInterrupt = 1;
	
	delay(DEMO_LENGTH_MILLIS);
	
	detachInterrupt(1);
}

void demoAckPayload() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("ACK payloads");
	
	_endMillis = millis() + DEMO_LENGTH_MILLIS;
	_lastMillis = millis();	

	while (millis() < _endMillis) {

		_currentMillis = millis();
		
		while (_radio.hasData()) {
			_radio.readData(&_radioData);
			debug("  Received ");
			debugln(_radioData.Counter);
		}
		
		if (_currentMillis - _lastMillis > 2000) {
			
			_lastMillis = _currentMillis;
			
			if (random(2) == 1) {
				_radioDataAckB.Counter--;
				debug("    Add ACK B ");
				debugln(_radioDataAckB.Counter);
				_radio.addAckData(&_radioDataAckB, sizeof(RadioAckPacketB));
			}
			else {
				_radioDataAckA.Counter++;
				debug("    Add ACK A ");
				debugln(_radioDataAckA.Counter);
				_radio.addAckData(&_radioDataAckA, sizeof(RadioAckPacketA));
			}
		}
	}
}

void demoPollingBitrate() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Polling bitrate");
	
	_packetCount = 0;
	_endMillis = millis() + DEMO_LENGTH_MILLIS;
	_lastMillis = millis();
	
	while (millis() < _endMillis) {
		
		while (_radio.hasData()) {
			_packetCount++;
			_radio.readData(&_radioData);
		}
		
		_currentMillis = millis();
		
		if (_currentMillis - _lastMillis > 999) {
			_bitsPerSecond = sizeof(RadioPacket) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
			_lastMillis = _currentMillis;
			debug("  ");
			debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
			_packetCount = 0;
		}
	}
}

void demoPollingBitrateNoAck() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Polling bitrate NO_ACK");
	
	_packetCount = 0;
	_endMillis = millis() + DEMO_LENGTH_MILLIS;
	_lastMillis = millis();

	while (millis() < _endMillis) {
		
		while (_radio.hasData()) {
			_packetCount++;
			_radio.readData(&_radioData);
		}
		
		_currentMillis = millis();
		
		if (_currentMillis - _lastMillis > 999) {
			_bitsPerSecond = sizeof(RadioPacket) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
			_lastMillis = _currentMillis;
			debug("  ");
			debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
			_packetCount = 0;
		}
	}
}

void demoInterruptsBitrate() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Interrupts bitrate");
	
	_showMessageInInterrupt = 0;
	attachInterrupt(1, radioInterrupt, FALLING);
	_packetCount = 0;
	
	_endMillis = millis() + DEMO_LENGTH_MILLIS;
	_lastMillis = millis();
	
	while (millis() < _endMillis) {
		
		_currentMillis = millis();
		
		if (_currentMillis - _lastMillis > 999) {
			_bitsPerSecond = sizeof(RadioPacket) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
			_lastMillis = _currentMillis;
			debug("  ");
			debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
			_packetCount = 0;
		}
	}
	
	detachInterrupt(1);
}

void demoInterruptsBitrateNoAck() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Interrupts bitrate NO_ACK");
	
	_showMessageInInterrupt = 0;
	attachInterrupt(1, radioInterrupt, FALLING);
	_packetCount = 0;
	
	_endMillis = millis() + DEMO_LENGTH_MILLIS;
	_lastMillis = millis();
	
	while (millis() < _endMillis) {
		
		_currentMillis = millis();
		
		if (_currentMillis - _lastMillis > 999) {
			_bitsPerSecond = sizeof(RadioPacket) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
			_lastMillis = _currentMillis;
			debug("  ");
			debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
			_packetCount = 0;
		}
	}
	
	detachInterrupt(1);
}

void demoPollingBitrateAckPayload() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Polling bitrate ACK payload");
	
	_endMillis = millis() + DEMO_LENGTH_MILLIS;
	_lastMillis = millis();
	
	_packetCount = 0;
	_ackAPacketCount = 0;
	_ackBPacketCount = 0;

	while (millis() < _endMillis) {
		
		while (_radio.hasData()) {
			
			_packetCount++;
			_radio.readData(&_radioData);
			
			// Randomly pick the type of ACK packet to enqueue.  2/3 will be A, 1/3 will be B.
			if (random(3) == 1) {
				_radio.addAckData(&_radioDataAckB, sizeof(RadioAckPacketB));
				_ackBPacketCount++;
			}
			else {
				_radio.addAckData(&_radioDataAckA, sizeof(RadioAckPacketA));
				_ackAPacketCount++;
			}
		}
		
		_currentMillis = millis();
		
		if (_currentMillis - _lastMillis > 999) {
			_bitsPerSecond = sizeof(RadioPacket) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
			_lastMillis = _currentMillis;
			debug("  ");
			debug(_packetCount); debug(" packets A");
			debug(_ackAPacketCount);
			debug(" B");
			debug(_ackBPacketCount);
			debug(" ");
			debug(_bitsPerSecond); 
			debugln(" bps");
			_packetCount = 0;
			_ackAPacketCount = 0;
			_ackBPacketCount = 0;
		}
	}
}

void demoInterruptsBitrateAckPayload() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Interrupts bitrate ACK payload");
	
	_endMillis = millis() + DEMO_LENGTH_MILLIS;
	_lastMillis = millis();
	
	_addAckInInterrupt = 1;
	_showMessageInInterrupt = 0;
	attachInterrupt(1, radioInterrupt, FALLING);
	_packetCount = 0;
	_ackAPacketCount = 0;
	_ackBPacketCount = 0;

	while (millis() < _endMillis) {
		
		_currentMillis = millis();
		
		if (_currentMillis - _lastMillis > 999) {
			_bitsPerSecond = sizeof(RadioPacket) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
			_lastMillis = _currentMillis;
			debug("  ");
			debug(_packetCount); debug(" packets A");
			debug(_ackAPacketCount);
			debug(" B");
			debug(_ackBPacketCount);
			debug(" ");
			debug(_bitsPerSecond);
			debugln(" bps");
			_packetCount = 0;
			_ackAPacketCount = 0;
			_ackBPacketCount = 0;
		}
	}
	
	detachInterrupt(1);
	_addAckInInterrupt = 0;
}

void demoPollingBitrateAllPacketSizes() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Polling bitrate all packet sizes");
	
	_packetCount = 0;
	_lastMillis = millis();
	
	uint8_t packet[32];
	uint8_t lastPacketSize = 1;
	uint8_t packetLength;
	
	while (1) {
		
		packetLength = _radio.hasData();
		
		if (packetLength > 0) {
			lastPacketSize = packetLength; // Store size for display.
			_packetCount++;
			_radio.readData(&packet);
		}
	
		_currentMillis = millis();
	
		if (_currentMillis - _lastMillis > 2999) { // Receive 3 seconds worth of packets for each size.
			
			_bitsPerSecond = lastPacketSize * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
			_lastMillis = _currentMillis;
			
			debug("  Size "); debug(lastPacketSize); debug(" ");
			debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
			
			_packetCount = 0;
			
			if (lastPacketSize == 32) break; // Stop demo when we reach max packet size.
			
			delay(DEMO_INTERVAL_MILLIS);
		}
	}
}

void demoPollingBitrateAllAckSizes() {
	
	delay(DEMO_INTERVAL_MILLIS);
	
	debugln("Polling bitrate all ACK sizes");
	
	_packetCount = 0;
	_lastMillis = millis();
	
	uint8_t ackPacket[32];
	uint8_t ackPacketSize = 1;
	
	_radio.addAckData(&ackPacket, ackPacketSize);
	
	while (1) {
		
		while (_radio.hasData()) {
			_packetCount++;
			_radio.readData(&_radioData);
			_radio.addAckData(&ackPacket, ackPacketSize);
		}
		
		_currentMillis = millis();
		
		if (_currentMillis - _lastMillis > 2999) { // Receive 3 seconds worth of packets for each size.
			
			_bitsPerSecond = sizeof(RadioPacket) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
			
			debug("  ACK Size "); debug(ackPacketSize); debug(" ");
			debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
			
			_packetCount = 0;
			_lastMillis = _currentMillis;
			
			if (ackPacketSize == 32) break; // Stop demo when we reach max packet size.
			
			ackPacketSize++; // Increase the size of the ACK to send back.
			delay(DEMO_INTERVAL_MILLIS);
		}
	}
}