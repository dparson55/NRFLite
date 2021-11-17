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

const static uint8_t RADIO_ID = 0;
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
char _sentenceAsCharArray[RADIO_PACKET_DATA_SIZE * 3]; // The 3 packets we receive will be placed into this array.

// Need to pre-declare these functions since they use structs.
void addPacketToSentence(RadioPacket packet);
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
    while (_radio.hasData())
    {
        RadioPacket packet;
        _radio.readData(&packet);

        if (packet.PacketNumber == 0)
        {
            Serial.println("Receiving Sentence " + String(packet.SentenceId));
            addPacketToSentence(packet);
        }
        else if (packet.PacketNumber == 1)
        {
            addPacketToSentence(packet);
        }
        else if (packet.PacketNumber == 2)
        {
            addPacketToSentence(packet); // The sentence should now be complete.
            
            String sentence = String(_sentenceAsCharArray); // Convert the char array we were building into a string.
            Serial.println("  Complete - " + sentence);
            Serial.println("  Sentence Length = " + String(sentence.length()));
        }
    }
}

void addPacketToSentence(RadioPacket packet)
{
    Serial.print("  Packet " + String(packet.PacketNumber) + " - ");
    printPacket(packet);
    
    // Determine location to place data within the sentence char array.
    // If it is the first packet, i.e. PacketNumber = 0, then the location = 0.
    // If PacketNumber = 1, then the location = 30.
    // If PacketNumber = 2, then the location = 60.
    uint8_t sentenceDataIndex = packet.PacketNumber * RADIO_PACKET_DATA_SIZE;

    // Copy the data from the packet into the char array, starting at the location we determined above.
    for (uint8_t packetDataIndex = 0; packetDataIndex < RADIO_PACKET_DATA_SIZE; packetDataIndex++)
    {
        _sentenceAsCharArray[sentenceDataIndex] = packet.Data[packetDataIndex];
        sentenceDataIndex++;
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