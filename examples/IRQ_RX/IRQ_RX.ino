/*

Demonstrates the usage of interrupts.

Radio    Arduino
CE    -> 9
CSN   -> 10 (Hardware SPI SS)
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
IRQ   -> 3  (Hardware INT1)
VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include "SPI.h"
#include "NRFLite.h"

const static uint8_t RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;
const static uint8_t PIN_RADIO_IRQ = 3;

NRFLite _radio;
volatile uint8_t _dataWasReceived; // Note usage of volatile since the variable is used in the radio interrupt
                                   // while also being used outside the interrupt.

void setup()
{
    Serial.begin(115200);

    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
        Serial.println("Cannot communicate with radio");
        while (1); // Wait here forever.
    }

    attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);
}

void loop()
{
    if (_dataWasReceived)
    {
        _dataWasReceived = false;

        // Use 'hasDataISR' rather than 'hasData' when using interrupts.
        while (_radio.hasDataISR())
        {
            uint8_t data;
            _radio.readData(&data);
            Serial.print("Received ");
            Serial.println(data);
        }
    }
}

void radioInterrupt()
{
    // Ask the radio what caused the interrupt.  This also resets the IRQ pin on the
    // radio so a new interrupt can be triggered.

    uint8_t txOk, txFail, rxReady;
    _radio.whatHappened(txOk, txFail, rxReady);

    // txOk = the radio successfully transmitted data.
    // txFail = the radio failed to transmit data.
    // rxReady = the radio received data.

    if (rxReady)
    {
        _dataWasReceived = true;
    }
}
