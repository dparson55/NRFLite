/*

Demonstrates RX operation with an Arduino using 2 pins for the radio.
This is the receiver for the 'LowPower_TX_ATtiny85_2Pin' example where an ATtiny85
sends voltage and temperature data.

Radio circuit
* Follow the 2-Pin Hookup Guide on https://github.com/dparson55/NRFLite

Connections
* Arduino 9  > MOMI of 2-pin circuit
* Arduino 10 > SCK of 2-pin circuit

*/

#include <NRFLite.h>

const static uint8_t PIN_RADIO_MOMI = 9;
const static uint8_t PIN_RADIO_SCK = 10;

const static uint8_t RADIO_ID = 0;

struct RadioPacket
{
    uint8_t FromRadioId;
    float Temperature;
    float Voltage;
    uint32_t FailedTxCount;
};

NRFLite _radio;
RadioPacket _radioData;

void setup()
{
    Serial.begin(115200);

    if (!_radio.initTwoPin(RADIO_ID, PIN_RADIO_MOMI, PIN_RADIO_SCK, NRFLite::BITRATE250KBPS))
    {
        Serial.println("Cannot communicate with radio");
        while (1);
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
        msg += _radioData.Temperature;
        msg += " F, ";
        msg += _radioData.Voltage;
        msg += " V, ";
        msg += _radioData.FailedTxCount;
        msg += " Failed TX";

        Serial.println(msg);
    }
}