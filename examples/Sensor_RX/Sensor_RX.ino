/*

The receiver for the 'Sensor_TX_ATtiny85_2Pin' example where an ATtiny85 sends various sensor data.
Sensor settings can be changed by entering a specially formatted string into the Serial Monitor.
This allows you to modify a few things such as how often the sensor sends its data, calibrate sensor readings, etc.
If you have multiple sensors, it is possible that the wrong sensor will receive the setting change request but
if this happens, the sensor will ignore it.  This problem is due to a simple acknowledgement data packet being
used to send settings, so whichever sensor contacts the receiver immediately after you enter the setting change
string will receive the setting change request.  To work around this, just enter the setting change string
again until the correct sensor receives the change request.  Sensors will send informational messages about
setting change requests when they receive them:  they will either let you know they are ignoring the request
or process it.

Examples:
'CID 1 2'    change sensor with Radio Id = 1 to Radio Id = 2
'CSI 1 60'   change sensor with Radio Id = 1 to have Sleep Interval = 60 seconds
'CTC 1 1.5'  change sensor with Radio Id = 1 to have Temperature Correction = 1.5
'CTT 1 0'    change sensor with Radio Id = 1 to have Temperature Type = 0 (0 = Celsius, 1 = Fahrenheit)
'CVC 1 -0.3' change sensor with Radio Id = 1 to have Voltage Correction = -0.3

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

struct RadioPacket
{
    uint8_t FromRadioId;
    uint32_t FailedTxCount;
    uint16_t Brightness;
    float Temperature;
    uint8_t TemperatureType; // 0 = Celsius, 1 = Fahrenheit
    float Voltage;
};

struct MessagePacket
{
    uint8_t FromRadioId;
    uint8_t message[31]; // Can hold a 30 character string + the null terminator.
};

enum ChangeType
{
    ChangeRadioId,
    ChangeSleepInterval,
    ChangeTemperatureCorrection,
    ChangeTemperatureType,
    ChangeVoltageCorrection
};

struct NewSettingsPacket
{
    ChangeType ChangeType;
    uint8_t ForRadioId;
    uint8_t NewRadioId;
    uint32_t NewSleepIntervalSeconds; // Max is about 4,000,000,000 seconds (126 years) due to Serial conversion from float.
    float NewTemperatureCorrection;
    uint8_t NewTemperatureType;
    float NewVoltageCorrection;
};

NRFLite _radio;
RadioPacket _radioData;
MessagePacket _messageData;
NewSettingsPacket _newSettingsData;

volatile uint8_t _hadRadioInterrupt; // Note usage of volatile for the global variable being changed in the radio interrupt.

void setup()
{
    Serial.begin(115200);

    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS))
    {
        Serial.println("Cannot communicate with radio");
        while (1); // Wait here forever.
    }

    attachInterrupt(digitalPinToInterrupt(PIN_RADIO_IRQ), radioInterrupt, FALLING);
}

void loop()
{
    if (Serial.available())
    {
        String input = Serial.readString();
        processSettingsChange(input);
    }

    if (_hadRadioInterrupt)
    {
        _hadRadioInterrupt = 0;

        while (_radio.hasDataISR()) // Note the use of 'hasDataISR' rather than 'hasData' when using interrupts.
        {
            uint8_t packetSize = _radio.hasDataISR();

            if (packetSize == sizeof(_radioData))
            {
                // Show the received sensor data.
                
                _radio.readData(&_radioData);

                String msg = "Radio ";
                msg += _radioData.FromRadioId;
                msg += ", ";
                msg += _radioData.FailedTxCount;
                msg += " Failed TX, ";
                msg += _radioData.Brightness;
                msg += " Brightness, ";
                msg += _radioData.Temperature;
                msg += _radioData.TemperatureType == 0 ? " C, " : " F, ";
                msg += _radioData.Voltage;
                msg += " V";

                Serial.println(msg);
            }
            else if (packetSize == sizeof(_messageData))
            {
                // Show the message string sent by a sensor.
                
                _radio.readData(&_messageData);

                String msg = "Radio ";
                msg += _messageData.FromRadioId;
                msg += ", ";
                msg += String((char*)_messageData.message);

                Serial.println(msg);
            }
        }
    }
}

void radioInterrupt()
{
    uint8_t txOk, txFail, rxReady;
    _radio.whatHappened(txOk, txFail, rxReady);

    if (rxReady)
    {
        _hadRadioInterrupt = 1;
    }        
}

void processSettingsChange(String input)
{
    input.toUpperCase();
    String msg = "Enqueued ACK packet to change ";

    if (input.startsWith("CID"))
    {
        // CID 1 2 (change id of radio 1 to radio id 2)
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeRadioId;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewRadioId = newValue.toInt();
        
        msg += "id of radio " + forRadioId;
        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
    else if (input.startsWith("CSI"))
    {
        // CSI 1 4 (change sleep interval for radio 1 to 4 seconds)
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeSleepInterval;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewSleepIntervalSeconds = (uint32_t)newValue.toFloat();

        msg += "sleep interval for radio " + forRadioId;
        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
    else if (input.startsWith("CTC"))
    {
        // CTC 1 1.5 (change temperature correction for radio 1 to positive 1.5 degrees)
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeTemperatureCorrection;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewTemperatureCorrection = newValue.toFloat();

        msg += "temperature correction for radio " + forRadioId;
        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
    else if (input.startsWith("CTT"))
    {
        // CTT 1 1 (change temperature type for radio 1 to Fahrenheit)
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeTemperatureType;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewTemperatureType = newValue.toInt();

        msg += "temperature type for radio " + forRadioId;
        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
    else if (input.startsWith("CVC"))
    {
        // CVC 1 -0.3 (change voltage correction for radio 1 to negative 0.3 volts)
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeVoltageCorrection;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewVoltageCorrection = newValue.toFloat();

        msg += "voltage correction for radio " + forRadioId;
        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }

    Serial.println(msg);
}
