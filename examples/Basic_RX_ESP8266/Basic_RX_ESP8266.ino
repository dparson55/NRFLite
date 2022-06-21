/*

Demonstrates simple RX operation with an ESP8266.
Any of the Basic_TX examples can be used as a transmitter.

ESP's require the use of '__attribute__((packed))' on the RadioPacket data structure
to ensure the bytes within the structure are aligned properly in memory.

Radio    ESP8266 ESP-12E module
CE    -> 4
CSN   -> 8 (and connect 8 to GND via a 3-5K resistor)
MOSI  -> 7
MISO  -> 6
SCK   -> 5
IRQ   -> No connection
VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include "SPI.h"  // You can also try SPISlave.h if this does not work.
#include "NRFLite.h"

const static uint8_t RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 4;
const static uint8_t PIN_RADIO_CSN = 8;

struct __attribute__((packed)) RadioPacket // Note the packed attribute.
{
    uint8_t FromRadioId;
    uint32_t OnTimeMillis;
    uint32_t FailedTxCount;
};

NRFLite _radio;
RadioPacket _radioData;

void setup()
{
    Serial.begin(115200);

    uint8_t initWasSuccessful = _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN);

    if (initWasSuccessful)
    {
        Serial.println("Successfully configured the registers within the nRF24L01");
    }
    else
    {
        Serial.println("Failed to validate the register assignments within the nRF24L01");

        while (1)     // Wait here forever since there is some problem with the ESP8266 and nRF24L01 communication.
        {
            yield();  // Allow the ESP8266 to boot successfully by allowing its background tasks to execute.
        }
    }
}

void loop()
{
    while (_radio.hasData())
    {
        _radio.readData(&_radioData);

        String msg = "Radio ";
        msg += _radioData.FromRadioId;
        msg += ", ";
        msg += _radioData.OnTimeMillis;
        msg += " ms, ";
        msg += _radioData.FailedTxCount;
        msg += " Failed TX";

        Serial.println(msg);
    }
}
