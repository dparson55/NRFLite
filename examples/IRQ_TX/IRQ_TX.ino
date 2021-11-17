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

const static uint8_t RADIO_ID = 1;
const static uint8_t DESTINATION_RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;
const static uint8_t PIN_RADIO_IRQ = 3;

NRFLite _radio;
uint8_t _data;
uint32_t _lastSendTime;
volatile uint8_t _sendSucceeded, _sendFailed; // Note usage of volatile since the variables are used in the radio
                                              // interrupt while also being used outside the interrupt.
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
    // Send data once per second.
    if (millis() - _lastSendTime > 999)
    {
        _lastSendTime = millis();
        
        _data++;
        Serial.print("Sending ");
        Serial.print(_data);

        // Use 'startSend' rather than 'send' when using interrupts.
        // 'startSend' will not wait for transmission to complete, instead you'll
        // need to wait for the radio to notify you via the interrupt to see if
        // the send was successful.
        _radio.startSend(DESTINATION_RADIO_ID, &_data, sizeof(_data));
        
        // Note that sending data puts the radio into TX mode.  If you want to be able to receive data again,
        // call 'startRx'.  The radio will complete any pending transmissions and then switch into RX mode.
    }

    if (_sendSucceeded)
    {
        _sendSucceeded = false;
        Serial.println("...Success");
    }
    
    if (_sendFailed)
    {
        _sendFailed = false;
        Serial.println("...Failed");
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

    if (txOk)
    {
        _sendSucceeded = true;
    }
    
    if (txFail)
    {
        _sendFailed = true;
    }
}
