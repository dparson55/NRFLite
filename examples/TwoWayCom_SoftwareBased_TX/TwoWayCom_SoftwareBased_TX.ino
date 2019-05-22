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

enum RadioPacketType
{
    Heartbeat,
    ReceiverData
};

struct RadioPacket
{
    RadioPacketType PacketType;
    uint8_t FromRadioId;
    uint32_t OnTimeMillis;
};

NRFLite _radio;
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
        RadioPacket radioData;
        radioData.PacketType = Heartbeat;
        radioData.FromRadioId = RADIO_ID;
        radioData.OnTimeMillis = _lastHeartbeatSendTime;

        if (_radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData))) // 'send' puts the radio into Tx mode.
        {
            Serial.println("...Success");
        }
        else
        {
            Serial.println("...Failed");
        }
    }

    // Check to see if any data has been received.
    while (_radio.hasData()) // 'hasData' ensures the radio is in Rx mode.  You can call '_radio.StartRx' as well.
    {
        RadioPacket radioData;
        _radio.readData(&radioData);

        String msg = "Received data from ";
        msg += radioData.FromRadioId;
        msg += ", ";
        msg += radioData.OnTimeMillis;
        msg += " ms";
        Serial.println(msg);
    }
}