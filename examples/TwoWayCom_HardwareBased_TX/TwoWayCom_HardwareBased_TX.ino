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

enum MessagePacketType
{
    Normal,
    BeginGetData,
    EndGetData
};

struct MessagePacket
{
    MessagePacketType MessageType;
    uint8_t FromRadioId;
};

struct AckPacket
{
    uint32_t ReceiverOnTimeMillis;
};

NRFLite _radio;
MessagePacket _messagePacket;
AckPacket _ackPacket;
uint32_t _lastSendMessageTime, _lastRequestDataTime;

void setup()
{
    Serial.begin(115200);

    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
        Serial.println("Cannot communicate with radio");
        while (1); // Wait here forever.
    }

    _messagePacket.FromRadioId = RADIO_ID;
}

void loop()
{
    // Send a message once every 2 seconds.
    if (millis() - _lastSendMessageTime > 1999)
    {
        _lastSendMessageTime = millis();

        Serial.println("Sending message");
        _messagePacket.MessageType = Normal;
        _radio.send(DESTINATION_RADIO_ID, &_messagePacket, sizeof(_messagePacket));
    }

    // Request data once every 5 seconds.
    if (millis() - _lastRequestDataTime > 4999)
    {
        _lastRequestDataTime = millis();

        Serial.println("Requesting data");
        Serial.println("  Sending BeginGetData");
        _messagePacket.MessageType = BeginGetData; // When the receiver sees this packet type, it will load an ACK data packet.
        _radio.send(DESTINATION_RADIO_ID, &_messagePacket, sizeof(_messagePacket));

        Serial.println("  Sending EndGetData");
        _messagePacket.MessageType = EndGetData; // When the receiver replies to this packet, we will get the ACK data that was loaded.
        _radio.send(DESTINATION_RADIO_ID, &_messagePacket, sizeof(_messagePacket));

            while (_radio.hasAckData())
            {
                _radio.readData(&_ackPacket);
                Serial.print("  Received ");
                Serial.println(_ackPacket.ReceiverOnTimeMillis);
            }
    }
}