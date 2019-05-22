/*

Demonstrates two-way communication using hardware features of the radio.  This is limited in that the
hardware has an idea of a primary transmitter and primary receiver, and it is the transmitter that must
initiate the communication.  The receiver can only provide data back to the transmitter via acknowledgement
data packets.  So the transmitter can send data to the receiver, and the receiver can reply back with data
to the transmitter, but the receiver cannot initiate the communication.

Another limitation is the receiver doesn't get the opportunity to process a received packet and then load an
acknowledgement data packet to send back, instead the acknowledgement packet must to be loaded prior to receiving
a packet from the transmitting radio.  An approach to work around this limitation is demonstrated in this example.

If you need the ability to initiate communication from either radio and do not require the high-speed,
hardware based two-way communication, see the 'TwoWayCom_SoftwareBased' example instead.

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
    BeginGetData,
    EndGetData,
    ReceiverData
};

struct RadioPacket
{
    RadioPacketType PacketType;
    uint8_t FromRadioId;
    uint32_t OnTimeMillis;
};

NRFLite _radio;
uint32_t _lastHeartbeatSendTime, _lastMessageRequestTime;

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

        if (_radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData)))
        {
            Serial.println("...Success");
        }
        else
        {
            Serial.println("...Failed");
        }
    }

    // Request data from the primary receiver once every 4 seconds.
    if (millis() - _lastMessageRequestTime > 3999)
    {
        _lastMessageRequestTime = millis();

        Serial.println("Requesting data");
        Serial.println("  Sending BeginGetData");
        RadioPacket radioData;
        radioData.PacketType = BeginGetData; // When the receiver sees this packet type, it will load an ACK data packet.
        _radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData));

        Serial.println("  Sending EndGetData");
        radioData.PacketType = EndGetData; // When the receiver replies to this packet, we will get the ACK data that was loaded.
        _radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData));
        
        while (_radio.hasAckData())
        {
            RadioPacket ackData;
            _radio.readData(&ackData);

            String msg = "  Received data from ";
            msg += ackData.FromRadioId;
            msg += ", ";
            msg += ackData.OnTimeMillis;
            msg += " ms";
            Serial.println(msg);
        }
    }
}