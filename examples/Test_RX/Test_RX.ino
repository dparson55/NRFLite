/*

This is used for internal testing of the library.

Goal is to test all bitrates, all packet sizes, all ACK packet sizes, with and without
interrupts, while measuring bitrates.

Manually adjusted for shared CE/CSN operation, and 2-pin operation.

*/

#include "SPI.h"
#include "NRFLite.h"

const static uint8_t RADIO_ID = 0;
const static uint8_t DESTINATION_RADIO_ID = 1;
const static uint8_t PIN_RADIO_CE = 9; // 9 or 10 depending on test condition
const static uint8_t PIN_RADIO_CSN = 10;
const static uint8_t PIN_RADIO_IRQ = 3;

#define SERIAL_SPEED 115200
#define debug(input)            { Serial.print(input);   }
#define debugln(input)          { Serial.println(input); }
#define debugln2(input1,input2) { Serial.print(input1); Serial.println(input2); }

enum RadioStates { StartSync, RunDemos };

struct RadioPacket { uint8_t Counter; RadioStates RadioState; uint8_t Data[29]; };
struct RadioAckPacketA { uint8_t Counter; uint8_t Data[31]; };
struct RadioAckPacketB { uint8_t Counter; uint8_t Data[30]; };

NRFLite _radio(Serial);
RadioPacket _radioData;
RadioAckPacketA _radioDataAckA;
RadioAckPacketB _radioDataAckB;

const uint16_t DEMO_LENGTH_MILLIS = 4250;
const uint16_t DEMO_INTERVAL_MILLIS = 250;

volatile uint8_t _hadRxReadyInterrupt;
uint32_t _bitsPerSecond, _packetCount, _ackAPacketCount, _ackBPacketCount;
uint32_t _currentMillis, _lastMillis, _endMillis;

void setup()
{
    Serial.begin(SERIAL_SPEED);

    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS))
    {
        debugln("Cannot communicate with radio");
        while (1); // Wait here forever.
    }
}

void loop()
{
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

void radioInterrupt()
{
    uint8_t txOk, txFail, rxReady;
    _radio.whatHappened(txOk, txFail, rxReady);

    if (rxReady)
    {
        _hadRxReadyInterrupt = 1;
    }
}

void runDemos()
{
    startSync();

    demoRxTxSwitching();
    demoPowerOff();

    demoPolling();
    demoInterrupts();
    demoAckPayload();

    demoPollingBitrate();
    demoPollingBitrateNoAck();
    demoPollingBitrateAckPayload();

    demoInterruptsBitrate();
    demoInterruptsBitrateNoAck();
    demoInterruptsBitrateAckPayload();

    demoPollingBitrateAllPacketSizes();
    demoPollingBitrateAllAckSizes();
}

void startSync()
{
    debugln("  Starting sync");

    detachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ));

    while (1)
    {
        if (_radio.hasData())
        {
            _radio.readData(&_radioData);
            if (_radioData.RadioState == StartSync) break;
        }
    }
}

void demoPolling()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Polling");

    while (millis() < _endMillis)
    {
        while (_radio.hasData())
        {
            _radio.readData(&_radioData);
            debugln2("  Received ", _radioData.Counter);
        }
    }
}

void demoInterrupts()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Interrupts");
    radioInterrupt(); // Clear any previously asserted interrupt.
    attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);

    while (millis() < _endMillis)
    {
        if (_hadRxReadyInterrupt)
        {
            _hadRxReadyInterrupt = 0;

            while (_radio.hasDataISR())
            {
                _radio.readData(&_radioData);
                debugln2("  Received ", _radioData.Counter);
            }
        }
    }

    detachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ));
}

void demoAckPayload()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("ACK payloads");

    while (millis() < _endMillis)
    {
        _currentMillis = millis();

        while (_radio.hasData())
        {
            _radio.readData(&_radioData);
            debug("  Received ");
            debug(_radioData.Counter);

            if (random(2) == 1)
            {
                _radioDataAckB.Counter--;
                debug(", Added ACK B ");
                debugln(_radioDataAckB.Counter);
                _radio.addAckData(&_radioDataAckB, sizeof(_radioDataAckB));
            }
            else
            {
                _radioDataAckA.Counter++;
                debug(", Added ACK A ");
                debugln(_radioDataAckA.Counter);
                _radio.addAckData(&_radioDataAckA, sizeof(_radioDataAckA));
            }
        }
    }
}

void demoRxTxSwitching()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("RxTx switching");
    radioInterrupt(); // Clear any previously asserted interrupt.
    attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);
    _lastMillis = millis() + 1500;

    while (millis() < _lastMillis)
    {
        if (_hadRxReadyInterrupt)
        {
            _hadRxReadyInterrupt = 0;
            _radio.readData(&_radioData);
            debugln2("  Received ", _radioData.Counter);
        }
    }

    detachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ));
    delay(500); // Wait for transmitter to become a receiver.

                // Send back the last received count, plus 1.  The primary transmitter will verify this value.
    _radioData.Counter++;
    debug("  Send ");
    debug(_radioData.Counter);

    if (_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData)))
    {
        debugln("...Success");
    }
    else
    {
        debugln("...Failed");
    }

    _radio.hasData(); // Switch back to RX mode to allow the next demo to start.

    while (millis() < _endMillis)
    {
        delay(1);
    }
}

void demoPowerOff()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Power off");

    debugln("  Radio power down");
    _radio.powerDown();

    while (millis() < _endMillis)
    {
        // Verify the call to 'hasData' powers the radio on.
        while (_radio.hasData())
        {
            _radio.readData(&_radioData);
            debugln2("  Received ", _radioData.Counter);
        }
    }
}

void demoPollingBitrate() {

    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Polling bitrate");
    _packetCount = 0;
    _lastMillis = millis();

    while (millis() < _endMillis)
    {
        _currentMillis = millis();

        while (_radio.hasData())
        {
            _packetCount++;
            _radio.readData(&_radioData);
        }

        if (_currentMillis - _lastMillis > 999)
        {
            _bitsPerSecond = sizeof(_radioData) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
            _lastMillis = _currentMillis;
            debug("  ");
            debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
            _packetCount = 0;
        }
    }
}

void demoPollingBitrateNoAck()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Polling bitrate NO_ACK");
    _packetCount = 0;
    _lastMillis = millis();

    while (millis() < _endMillis)
    {
        _currentMillis = millis();

        while (_radio.hasData())
        {
            _packetCount++;
            _radio.readData(&_radioData);
        }

        if (_currentMillis - _lastMillis > 999)
        {
            _bitsPerSecond = sizeof(_radioData) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
            _lastMillis = _currentMillis;
            debug("  ");
            debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
            _packetCount = 0;
        }
    }
}

void demoInterruptsBitrate()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Interrupts bitrate");
    radioInterrupt(); // Clear any previously asserted interrupt.
    attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);
    _packetCount = 0;

    _lastMillis = millis();

    while (millis() < _endMillis)
    {
        _currentMillis = millis();

        if (_hadRxReadyInterrupt)
        {
            _hadRxReadyInterrupt = 0;

            while (_radio.hasDataISR())
            {
                _packetCount++;
                _radio.readData(&_radioData);
            }
        }

        if (_currentMillis - _lastMillis > 999)
        {
            _bitsPerSecond = sizeof(_radioData) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
            _lastMillis = _currentMillis;
            debug("  ");
            debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
            _packetCount = 0;
        }
    }

    detachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ));
}

void demoInterruptsBitrateNoAck()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Interrupts bitrate NO_ACK");
    radioInterrupt(); // Clear any previously asserted interrupt.
    attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);
    _packetCount = 0;
    _lastMillis = millis();

    while (millis() < _endMillis)
    {
        _currentMillis = millis();

        if (_hadRxReadyInterrupt)
        {
            _hadRxReadyInterrupt = 0;

            while (_radio.hasDataISR())
            {
                _packetCount++;
                _radio.readData(&_radioData);
            }
        }

        if (_currentMillis - _lastMillis > 999)
        {
            _bitsPerSecond = sizeof(_radioData) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
            _lastMillis = _currentMillis;
            debug("  ");
            debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");
            _packetCount = 0;
        }
    }

    detachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ));
}

void demoPollingBitrateAckPayload()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Polling bitrate ACK payload");
    _lastMillis = millis();
    _packetCount = 0;
    _ackAPacketCount = 0;
    _ackBPacketCount = 0;

    while (millis() < _endMillis)
    {
        _currentMillis = millis();

        while (_radio.hasData())
        {
            _packetCount++;
            _radio.readData(&_radioData);

            // Randomly pick the type of ACK packet to enqueue.  2/3 will be A, 1/3 will be B.
            if (random(3) == 1)
            {
                _radio.addAckData(&_radioDataAckB, sizeof(_radioDataAckB));
                _ackBPacketCount++;
            }
            else
            {
                _radio.addAckData(&_radioDataAckA, sizeof(_radioDataAckA));
                _ackAPacketCount++;
            }
        }

        if (_currentMillis - _lastMillis > 999)
        {
            _bitsPerSecond = sizeof(_radioData) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
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

void demoInterruptsBitrateAckPayload()
{
    delay(DEMO_INTERVAL_MILLIS);
    _endMillis = millis() + DEMO_LENGTH_MILLIS;

    debugln("Interrupts bitrate ACK payload");
    radioInterrupt(); // Clear any previously asserted interrupt.
    attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);
    _lastMillis = millis();
    _packetCount = 0;
    _ackAPacketCount = 0;
    _ackBPacketCount = 0;

    while (millis() < _endMillis)
    {
        _currentMillis = millis();

        if (_hadRxReadyInterrupt)
        {
            _hadRxReadyInterrupt = 0;

            while (_radio.hasDataISR())
            {
                _packetCount++;
                _radio.readData(&_radioData);

                if (random(3) == 1)
                {
                    _radio.addAckData(&_radioDataAckB, sizeof(_radioDataAckB));
                    _ackBPacketCount++;
                }
                else
                {
                    _radio.addAckData(&_radioDataAckA, sizeof(_radioDataAckA));
                    _ackAPacketCount++;
                }
            }
        }

        if (_currentMillis - _lastMillis > 999)
        {
            _bitsPerSecond = sizeof(_radioData) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
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

    detachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ));
}

void demoPollingBitrateAllPacketSizes()
{
    delay(DEMO_INTERVAL_MILLIS);

    debugln("Polling bitrate all packet sizes");
    _packetCount = 0;
    _lastMillis = millis();

    uint8_t packet[32];
    uint8_t lastPacketSize = 1;
    uint8_t packetLength;

    while (1)
    {
        _currentMillis = millis();
        packetLength = _radio.hasData();

        if (packetLength > 0)
        {
            lastPacketSize = packetLength; // Store size for display.
            _packetCount++;
            _radio.readData(&packet);
        }

        if (_currentMillis - _lastMillis > 1999) // Receive 2 seconds worth of packets for each size.
        {
            _bitsPerSecond = lastPacketSize * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;
            _lastMillis = _currentMillis;

            debug("  Size "); debug(lastPacketSize); debug(" ");
            debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");

            if (lastPacketSize == 32) break; // Stop demo when we reach the max packet size.

            _packetCount = 0;
            delay(DEMO_INTERVAL_MILLIS);
        }
    }
}

void demoPollingBitrateAllAckSizes()
{
    // Receives a 32 byte data packet and returns ACK packets that increase in length.

    delay(DEMO_INTERVAL_MILLIS);

    debugln("Polling bitrate all ACK sizes");
    _packetCount = 0;
    _lastMillis = millis();
    uint8_t ackPacket[32];
    uint8_t ackPacketSize = 1; // Start with this packet size and work our way up.

    _radio.addAckData(&ackPacket, ackPacketSize);

    while (1)
    {
        _currentMillis = millis();

        while (_radio.hasData())
        {
            _packetCount++;
            _radio.readData(&_radioData);
            _radio.addAckData(&ackPacket, ackPacketSize);
        }

        if (_currentMillis - _lastMillis > 1999) // Receive 2 seconds worth of packets for each size.
        {
            _bitsPerSecond = sizeof(_radioData) * _packetCount * 8 / (float)(_currentMillis - _lastMillis) * 1000;

            debug("  ACK Size "); debug(ackPacketSize); debug(" ");
            debug(_packetCount); debug(" packets "); debug(_bitsPerSecond); debugln(" bps");

            if (ackPacketSize == 32) break; // Stop demo when we reach the max packet size.

            _packetCount = 0;
            _lastMillis = _currentMillis;
            ackPacketSize++; // Increase the size of the ACK to send back.
            delay(DEMO_INTERVAL_MILLIS);
        }
    }
}