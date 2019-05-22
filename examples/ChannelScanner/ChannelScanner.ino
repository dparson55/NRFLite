/*

Run this to select a free channel to use with the radio.  A graph will be shown in the serial montior
that shows any existing signals on the channels supported by the radio.

By default, 'init' configures the radio to use a 2MBPS bitrate on channel 100 (channels 0-125 are valid).
Both the RX and TX radios must have the same bitrate and channel to communicate with each other.
You can assign a different channel as shown below.
_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS, 0);   // Channel 0
_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS, 75);  // Channel 75
_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS, 100); // Channel 100, THE DEFAULT

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

#include <SPI.h>
#include <NRFLite.h>

const static uint8_t RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

NRFLite _radio(Serial); // Provide a serial object so the radio can print to the serial monitor.

void setup()
{
    Serial.begin(115200);

    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
        Serial.println("Cannot communicate with radio");
        while (1); // Wait here forever.
    }

    Serial.println();
    Serial.println("Use a channel without existing signals.");
    _radio.printChannels();
}

void loop() {}