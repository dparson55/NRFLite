/*

Demonstrates simple RX and TX operation using 2 pins for the radio.  This greatly slows down the communication speed between
the microcontroller and radio compared to using the SPI and USI peripherials of the ATmega and ATtiny microcontrollers.
Only AVR architectures (ATMega/ATtiny) support 2 pin operation.

Any of the Basic_RX examples can be used as a receiver.

Radio circuit
* Follow the 2-Pin Hookup Guide on https://github.com/dparson55/NRFLite

Connections
* Arduino 9  > MOMI of 2-pin circuit
* Arduino 10 > SCK of 2-pin circuit

*/

#include "NRFLite.h"

const static uint8_t RADIO_ID = 2;
const static uint8_t DESTINATION_RADIO_ID = 0;
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

    _radioData.FromRadioId = RADIO_ID;
}

void loop()
{
    _radioData.OnTimeMillis = millis();

    Serial.print("Sending ");
    Serial.print(_radioData.OnTimeMillis);
    Serial.print(" ms");

    if (_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData)))
    {
        Serial.println("...Success");
    }
    else
    {
        Serial.println("...Failed");
        _radioData.FailedTxCount++;
    }

    delay(1000);
}
