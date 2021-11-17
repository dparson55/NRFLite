/*

Demonstrates sending and receiving packets of different length.

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

struct RadioPacket1
{
    uint8_t FromRadioId;
    uint8_t Counter;
};

struct RadioPacket2
{
    uint8_t FromRadioId;
    char Message[31];    // Note the max packet size is 32, so 31 is all we can use here.
};

NRFLite _radio;
RadioPacket1 _radioData1;
RadioPacket2 _radioData2;

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
    // 'hasData' returns the size of the packet that was received, or 0 if there is no data.
    uint8_t packetSize = _radio.hasData();

    if (packetSize > 0)
    {
        if (packetSize == sizeof(RadioPacket1))
        {
            _radio.readData(&_radioData1);

            Serial.print("Received ");
            Serial.print(_radioData1.Counter);
            Serial.print(" from radio ");
            Serial.println(_radioData1.FromRadioId);
        }
        else if (packetSize == sizeof(RadioPacket2))
        {
            _radio.readData(&_radioData2);

            String msg = String(_radioData2.Message);

            Serial.print("Received '");
            Serial.print(msg);
            Serial.print("' from radio ");
            Serial.println(_radioData2.FromRadioId);
        }
        else
        {
            // We have a packet with an unexpected size, either an invalid packet or packet
            // that was sent from a radio that we cannot handle, so remove it from the radio.

            Serial.print("Received packet with length '");
            Serial.print(packetSize);
            Serial.println("', it will be discarded");

            _radio.discardData(packetSize);
        }
    }
}