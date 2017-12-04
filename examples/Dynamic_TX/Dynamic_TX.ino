/*

Demonstrates sending data packets of different length.  The receiver will check to see what size of
packet was received and act accordingly.

Radio -> Arduino

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
const static uint8_t PIN_RADIO_MOMI = 9;
const static uint8_t PIN_RADIO_SCK = 10;

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

    if (!_radio.initTwoPin(RADIO_ID, PIN_RADIO_MOMI, PIN_RADIO_SCK))
    {
        Serial.println("Cannot communicate with radio");
        while (1) {} // Wait here forever.
    }

    _radioData1.FromRadioId = RADIO_ID;
    _radioData2.FromRadioId = RADIO_ID;
}

void loop()
{
    // Pick a number from 10,000 - 60,000.
    uint16_t randomNumber = random(10000, 60001);

    if (randomNumber > 30000)
    {
        // Setup and send RadioPacket1.

        _radioData1.Counter++;

        Serial.print("Sending ");
        Serial.print(_radioData1.Counter);

        if (_radio.send(DESTINATION_RADIO_ID, &_radioData1, sizeof(_radioData1)))
        {
            Serial.println("...Success");
        }
        else
        {
            Serial.println("...Failed");
        }
    }
    else
    {
        // Setup and send RadioPacket2.

        // Create a message and assign it to the packet.
        // Strings need to be converted to a char array and note it cannot be longer
        // than 31 characters since that is the size of _radioData2.Message.
        String msg = "Hello " + String(randomNumber);
        msg.toCharArray(_radioData2.Message, msg.length() + 1);

        Serial.print("Sending '");
        Serial.print(msg);
        Serial.print("'");

        if (_radio.send(DESTINATION_RADIO_ID, &_radioData2, sizeof(_radioData2)))
        {
            Serial.println("...Success");
        }
        else
        {
            Serial.println("...Failed");
        }
    }

    delay(1000);
}