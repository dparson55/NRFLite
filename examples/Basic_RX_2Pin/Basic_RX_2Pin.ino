/*

Demonstrates simple RX and TX operation using 2 pins for the radio.
Any of the Basic_TX examples can be used as a transmitter.
Only AVR architectures (ATtiny/ATmega) support 2 pin operation.

Radio circuit
* Follow the 2-Pin Hookup Guide on https://github.com/dparson55/NRFLite

Connections
* Arduino 9  > MOMI of 2-pin circuit
* Arduino 10 > SCK of 2-pin circuit

*/

#include "NRFLite.h"

const static uint8_t RADIO_ID = 0;
const static uint8_t PIN_RADIO_MOMI = 9;
const static uint8_t PIN_RADIO_SCK = 10;

struct RadioPacket
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

    if (!_radio.initTwoPin(RADIO_ID, PIN_RADIO_MOMI, PIN_RADIO_SCK)) // Note usage of 'initTwoPin' rather than 'init'.
    {
        Serial.println("Cannot communicate with radio");
        while (1); // Wait here forever.
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
