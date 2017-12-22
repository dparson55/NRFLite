/* 

Demonstrates the usage of interrupts while sending and receiving acknowledgement data.

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

#include <SPI.h>
#include <NRFLite.h>

const static uint8_t RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;
const static uint8_t PIN_RADIO_IRQ = 3;

NRFLite _radio;
uint8_t _data;

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

void loop() {}

void radioInterrupt()
{
    // Ask the radio what caused the interrupt.
    // txOk = the radio successfully transmitted data.
    // txFail = the radio failed to transmit data.
    // rxReady = the radio has received data.
    uint8_t txOk, txFail, rxReady;
    _radio.whatHappened(txOk, txFail, rxReady);
    
    if (rxReady)
    {
        // Use 'hasDataISR' rather than 'hasData' when using interrupts.
        while (_radio.hasDataISR())
        {
            _radio.readData(&_data);
            uint8_t ackData = _data + 100;
            
            Serial.print("Received ");
            Serial.print(_data);
            Serial.print("...Added Ack ");
            Serial.println(ackData);
            
            // Add the Ack data packet to the radio.  The next time we receive data,
            // this Ack packet will be sent back.
            _radio.addAckData(&ackData, sizeof(ackData));
        }
    }
}
