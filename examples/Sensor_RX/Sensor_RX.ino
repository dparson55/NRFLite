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

#include <SPI.h>
#include <NRFLite.h>

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
    uint16_t NewSleepIntervalSeconds;
    float NewTemperatureCorrection;
    uint8_t NewTemperatureType;
    float NewVoltageCorrection;
};

NRFLite _radio;
RadioPacket _radioData;
MessagePacket _messageData;
NewSettingsPacket _newSettingsData;

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
}

void radioInterrupt()
{
    uint8_t txOk, txFail, rxReady;
    _radio.whatHappened(txOk, txFail, rxReady);

    if (rxReady)
    {
        while (_radio.hasDataISR()) // Note the use of 'hasDataISR' rather than 'hasData' when using interrupts.
        {
            uint8_t packetSize = _radio.hasDataISR();

            if (packetSize == sizeof(_radioData))
            {
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

void processSettingsChange(String input)
{
    input.toUpperCase();

    if (input.startsWith("CID"))
    {
        // CID 1 2
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeRadioId;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewRadioId = newValue.toInt();

        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
    else if (input.startsWith("CSI"))
    {
        // CSI 1 4
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeSleepInterval;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewSleepIntervalSeconds = newValue.toInt();

        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
    else if (input.startsWith("CTC"))
    {
        // CTC 1 1.5
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeTemperatureCorrection;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewTemperatureCorrection = newValue.toFloat();

        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
    else if (input.startsWith("CTT"))
    {
        // CTT 1 0
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeTemperatureType;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewTemperatureType = newValue.toInt();

        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
    else if (input.startsWith("CVC"))
    {
        // CVC 1 -0.3
        uint8_t spaceIndex = input.indexOf(' ', 4);
        String forRadioId = input.substring(4, spaceIndex);
        String newValue = input.substring(spaceIndex + 1);

        _newSettingsData.ChangeType = ChangeVoltageCorrection;
        _newSettingsData.ForRadioId = forRadioId.toInt();
        _newSettingsData.NewVoltageCorrection = newValue.toFloat();

        _radio.addAckData(&_newSettingsData, sizeof(_newSettingsData));
    }
}
