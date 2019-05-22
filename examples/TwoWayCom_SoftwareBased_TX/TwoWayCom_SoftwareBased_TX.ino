/*

Demonstrates two-way communication by manually switching between RX and TX modes.  This is much slower
than the hardware based two-way communication shown in the 'TwoWayCom_HardwareBased' example, but this
software based approach is more flexible.

Radio    Arduino
CE    -> 9
CSN   -> 10 (Hardware SPI SS)
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
IRQ   -> No connection
VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include <SPI.h>
#include <NRFLite.h>

const static uint8_t RADIO_ID = 1;
const static uint8_t DESTINATION_RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

struct HeartbeatPacket
{
    uint8_t FromRadioId;
    uint32_t OnTimeMillis;
};

struct MessagePacket
{
    uint8_t FromRadioId;
    uint32_t OnTimeMillis;
};

NRFLite _radio;
HeartbeatPacket _heartbeatData;
MessagePacket _messageData;
uint32_t _lastHeartbeatSendTime;

void setup()
{
    Serial.begin(115200);

    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
        Serial.println("Cannot communicate with radio");
        while (1); // Wait here forever.
    }
}

void loop()
{
    // Send a heartbeat once every second.
    if (millis() - _lastHeartbeatSendTime > 999)
    {
        _lastHeartbeatSendTime = millis();

        Serial.print("Sending heartbeat");
        _heartbeatData.FromRadioId = RADIO_ID;
        _heartbeatData.OnTimeMillis = _lastHeartbeatSendTime;

        if (_radio.send(DESTINATION_RADIO_ID, &_heartbeatData, sizeof(_heartbeatData))) // 'send' puts the radio into Tx mode.
        {
            Serial.println("...Success");
        }
        else
        {
            Serial.println("...Failed");
        }
    }

    // Check to see if any messages have been received.
    while (_radio.hasData()) // 'hasData' ensures the radio is in Rx mode.  You can call '_radio.StartRx' as well.
    {
        _radio.readData(&_messageData);

        String msg = "Received message from ";
        msg += _messageData.FromRadioId;
        msg += ", ";
        msg += _messageData.OnTimeMillis;
        msg += " ms";
        Serial.println(msg);
    }
}