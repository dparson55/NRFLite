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

const static uint8_t RADIO_ID = 0;
const static uint8_t DESTINATION_RADIO_ID = 1;
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
uint32_t _lastMessageSendTime;

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
    while (_radio.hasData())
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
        else if (radioData.PacketType == BeginGetData)
        {
            Serial.println("Received data request, adding ACK packet");

            RadioPacket ackData;
            ackData.PacketType = ReceiverData;
            ackData.FromRadioId = RADIO_ID;
            ackData.OnTimeMillis = millis();

            // Add the data to send back to the transmitter.
            // The next packet we receive will be acknowledged with this data.
            _radio.addAckData(&ackData, sizeof(ackData));
        }
        else if (radioData.PacketType == EndGetData)
        {
            // The transmitter hopefully received the acknowledgement data packet at this point.
        }
    }
}