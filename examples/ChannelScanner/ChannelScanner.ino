/*

Run this to help identify a free channel to use with the radio.  It shows a graph in the serial montior
which allows you to visualize channels that already have a signal.  You should configure the radio to use
a channel with no existing signals.

By default, 'init' configures the radio to use channel 100 (channels 0-125 are valid).
Both the RX and TX radios must have the same channel to communicate with each other.
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

    Serial.println("Use a channel without existing signals.");
    Serial.println("Each X indicates a signal was received.");
    Serial.println();

    for (uint8_t channel = 0; channel <= NRFLite::MAX_NRF_CHANNEL; channel++)
    {
        uint8_t signalStrength = _radio.scanChannel(channel);

        // Build the message about the channel, e.g. 'Channel 125 XXXXXXXXX'
        String channelMsg = "Channel ";

        if (channel < 10) { channelMsg += "  "; }
        else if (channel < 100) { channelMsg += " "; }
        channelMsg += channel;
        channelMsg += "  ";

        while (signalStrength--)
        {
            channelMsg += "X";
        }

        // Print the channel message.
        Serial.println(channelMsg);

#if defined(ESP8266) || defined(ESP32)
        yield();
#endif
    }
}

void loop() {}
