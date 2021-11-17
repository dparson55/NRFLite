/*

Demonstrates two-way communication without using acknowledgement data packets.  This is much slower than
the hardware-based, acknowledgement data packet approach shown in the 'TwoWayCom_HardwareBased' example,
but it is more flexible.

It is important to keep in mind the radio cannot send and receive data at the same time, instead, the
radio will either be in receiver mode or transmitter mode.  NRFLite switches the mode of the radio in
hopefully a natural way based on the methods being called, e.g. 'send' puts the radio into transmitter
mode while 'hasData' puts it into receiver mode.  But you may need to be aware of this mode switching
behavior when doing two-way communication.  For example, you may need to minimize the amount of time the
radio is in transmitter mode if you would like it to participate in two-way communication with another
radio.  An approach to minimize the amount of time the radio is a transmitter is shown in this example.

Software-based two-way communication is slow compared to the two-way communication support implemented
by the designers of the radio module in hardware.  So check out the hardware-based two-way communication
example if you would like a faster solution.

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

#include "SPI.h"
#include "NRFLite.h"

const static uint8_t RADIO_ID = 0;
const static uint8_t DESTINATION_RADIO_ID = 1;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

enum RadioPacketType
{
    Heartbeat
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
    // Send a heartbeat once every 4 seconds.
    if (millis() - _lastHeartbeatSendTime > 3999)
    {
        _lastHeartbeatSendTime = millis();
        sendHeartbeat();        
    }

    // Show any received data.
    checkRadio();
}

void sendHeartbeat()
{
    Serial.print("Sending heartbeat");

    RadioPacket radioData;
    radioData.PacketType = Heartbeat;
    radioData.FromRadioId = RADIO_ID;
    radioData.OnTimeMillis = millis();

    if (_radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData))) // 'send' puts the radio into Tx mode.
    {
        Serial.println("...Success");
    }
    else
    {
        Serial.println("...Failed");
    }
}

void checkRadio()
{
    while (_radio.hasData()) // 'hasData' puts the radio into Rx mode.
    {
        RadioPacket radioData;
        _radio.readData(&radioData);

        if (radioData.PacketType == Heartbeat)
        {
            String msg = "Heartbeat from ";
            msg += radioData.FromRadioId;
            msg += ", ";
            msg += radioData.OnTimeMillis;
            msg += " ms";
            Serial.println(msg);
        }
    }
}