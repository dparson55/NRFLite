/*

Demonstrates sending an array of data too long to fit into a single packet.  The radio hardware
supports a maximum data packet size of 32 bytes, so if you have a single piece of data that
cannot fit into a packet, the data will need to be spread across multiple packets.

In this example a fake GPS data sentence containing at least 72 bytes is created and then sent
using 3 separate packets.

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

const static uint8_t RADIO_ID = 1;
const static uint8_t DESTINATION_RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

const static uint8_t RADIO_PACKET_DATA_SIZE = 30;

struct RadioPacket
{
    uint8_t SentenceId;   // Identifier for the long string of data being sent.
    uint8_t PacketNumber; // Locator used to order the packets making up the data, e.g. 0, 1, or 2.
    uint8_t Data[RADIO_PACKET_DATA_SIZE]; // 30 since the radio's max packet size = 32 and we have 2 bytes of metadata.
};

NRFLite _radio;
uint8_t _sentenceId;
uint32_t _lastSendTime;

// Need to pre-declare these functions since they use structs.
bool sendPacket(RadioPacket packet);
void printPacket(RadioPacket packet);

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
    // Send a fake GPS sentence once every 4 seconds.
    if (millis() - _lastSendTime > 3999)
    {
        _lastSendTime = millis();

        String gpsSentence = "$GPGGA,123456.0,789.00,S,12345.00,E,6,78,9.00,12356.00,M,7.00,M,*8F," + String(millis());
        sendSentence(gpsSentence, _sentenceId);
        _sentenceId++;
    }
}

void sendSentence(String sentence, uint8_t sentenceId)
{
    Serial.println("Sending Sentence " + String(sentenceId));
    Serial.println("  " + sentence);
    Serial.println("  Sentence Length = " + String(sentence.length()));

    RadioPacket packet;
    packet.SentenceId = sentenceId;
    packet.PacketNumber = 0;
    uint8_t packetDataIndex = 0;
    uint8_t sentenceRealLength = sentence.length() + 1; // Need 1 extra byte for the string's null terminator character.

    // Loop through each character in the sentence and add them to the radio packet until the packet is full, send it,
    // then fill the packet with the next set of characters.  We'll continue until all characters, including the null
    // terminator, have been sent.
    for (uint8_t sentenceDataIndex = 0; sentenceDataIndex < sentenceRealLength; sentenceDataIndex++)
    {
        packet.Data[packetDataIndex] = sentence[sentenceDataIndex]; // Copy character from the sentence into the packet.
        bool packetIsFull = packetDataIndex + 1 == RADIO_PACKET_DATA_SIZE;
        bool endOfSentence = sentenceDataIndex + 1 == sentenceRealLength;

        if (packetIsFull || endOfSentence)
        {
            bool success = sendPacket(packet);

            if (success)
            {
                // Prepare to build the next packet.
                packet.PacketNumber++;
                packetDataIndex = 0;
            }
            else
            {
                Serial.println("  Failed to send sentence");
                return; // Abort transmission of the sentence since we failed to send this packet.
                        // The receiver will not be directly notified that we failed to send the sentence,
                        // but it will abort building any in-progress sentence when it receives a packet
                        // with PacketNumber = 0.
            }
        }
        else
        {
            // Increment the location where the next sentence character will be placed within the radio packet.
            packetDataIndex++;
        }
    }
}

bool sendPacket(RadioPacket packet)
{
    Serial.print("  Sending Packet " + String(packet.PacketNumber) + " - ");

    if (_radio.send(DESTINATION_RADIO_ID, &packet, sizeof(packet)))
    {
        printPacket(packet);
        return true;
    }
    else
    {
        Serial.println("Error");
        return false;
    }
}

void printPacket(RadioPacket packet)
{
    // Strings in C are arrays of char characters and they must end with a '\0' null character.
    // In order to print the array of data that's contained in the RadioPacket, we'll need to ensure
    // the data ends with such a character.

    char arrayToPrint[RADIO_PACKET_DATA_SIZE + 1]; // Allow 1 more character in case we need to add a null terminator.
    bool arrayIsTerminated = false;
    uint8_t i = 0;

    while (i < RADIO_PACKET_DATA_SIZE)
    {
        arrayToPrint[i] = packet.Data[i];           // Copy data from the packet into the char array.
        arrayIsTerminated = packet.Data[i] == '\0'; // See if the data is the null terminator character.

        if (arrayIsTerminated)
        {
            break;
        }
        else
        {
            i++; // Prepare to copy the next piece of data from the packet.
        }
    }

    if (!arrayIsTerminated)
    {
        // Add a null terminator so we can print the array as a string.
        arrayToPrint[i] = '\0';
    }

    Serial.println(String(arrayToPrint));
}