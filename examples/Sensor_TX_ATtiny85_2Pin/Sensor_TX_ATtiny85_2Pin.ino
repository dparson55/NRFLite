/*

Requires the use of https://github.com/damellis/attiny for the ATtiny85 rather than
https://github.com/SpenceKonde/ATTinyCore since ATTinyCore takes up too much memory.

Demonstrates TX operation with an ATtiny85 using 2 pins for the radio.
The ATtiny85 powers up, takes various sensor readings, sends the data, and then powers down.
Physical Pin 5 (Arduino 0) powers the sensors to reduce current consumption.
Messages are sent to the receiver for debugging and other informational purposes.
The receiver can change certain settings by providing an acknowledgement NewSettingsPacket packet, see
the 'Sensor_RX' example for more detail.  Settings are stored in eeprom.

Radio circuit
* Follow the 2-Pin Hookup Guide on https://github.com/dparson55/NRFLite

There is a jpeg of the circuit in the same folder as this .ino file.

LED circuit
* Physical Pin 5 > LED > 1K resistor > GND

Light dependent resistor (LDR) circuit
* Physical Pin 5 > LDR > 1K resistor > GND

Thermistor circuit
* Physical Pin 5 > 10K resistor > thermistor > GND

Connections
* Physical Pin 2, Arduino A3 > Connect between 10K resistor and thermistor
* Physical Pin 3, Arduino 4  > MOMI of 2-pin circuit
* Physical Pin 5, Arduino 0  > LED > 1K resistor > GND
*                              LDR > 1K resistor > GND
*                              10K resistor > Thermistor > GND
* Physical Pin 6, Arduino 1  > SCK of 2-pin circuit
* Physical Pin 7, Arduino A1 > Connect between LDR and 1K resistor

*/

#include <NRFLite.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

const static uint8_t PIN_RADIO_MOMI = 4;
const static uint8_t PIN_RADIO_SCK = 1;
const static uint8_t PIN_LDR = A1;
const static uint8_t PIN_THERM = A3;
const static uint8_t PIN_SENSOR_POWER = 0;

const static uint8_t DESTINATION_RADIO_ID = 0;

const static uint16_t THERM_BCOEFFICIENT = 4050;
const static uint8_t  THERM_NOMIAL_TEMP = 25;           // Thermistor nominal temperature in C.
const static uint16_t THERM_NOMINAL_RESISTANCE = 10000; // Thermistor resistance at the nominal temperature.
const static uint16_t THERM_SERIES_RESISTOR = 10000;    // Value of resistor in series with the thermistor.

const uint8_t EEPROM_SETTINGS_VERSION = 1;

struct EepromSettings
{
    uint8_t RadioId;
    uint16_t SleepIntervalSeconds;
    float TemperatureCorrection;
    uint8_t TemperatureType;
    float VoltageCorrection;
    uint8_t Version;
};

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
    uint16_t NewSleepIntervalSeconds; // Max value = 65535 seconds = 45 days
    float NewTemperatureCorrection;
    uint8_t NewTemperatureType;
    float NewVoltageCorrection;
};

NRFLite _radio;
EepromSettings _settings;
uint32_t _failedTxCount;

#define eepromBegin() eeprom_busy_wait(); noInterrupts() // Details on https://youtu.be/_yOcKwu7mQA
#define eepromEnd()   interrupts()

void processSettingsChange(NewSettingsPacket newSettings); // Need to pre-declare this function since it uses a custom struct as a parameter (or use a .h file instead).

void setup()
{
    // Load settings from eeprom.
    eepromBegin();
    eeprom_read_block(&_settings, 0, sizeof(_settings));
    eepromEnd();

    if (_settings.Version == EEPROM_SETTINGS_VERSION)
    {
        setupRadio();
        sendMessage(F("Loaded settings")); // Save memory using F() for strings.  Details on https://learn.adafruit.com/memories-of-an-arduino/optimizing-sram
    }
    else
    {
        // The settings version in eeprom is not what we expect, so assign default values.
        _settings.RadioId = 1;
        _settings.SleepIntervalSeconds = 2;
        _settings.TemperatureCorrection = 0;
        _settings.TemperatureType = 0;
        _settings.VoltageCorrection = 0;
        _settings.Version = EEPROM_SETTINGS_VERSION;

        setupRadio();
        sendMessage(F("Eeprom old, using defaults"));
        saveSettings();
    }
}

void setupRadio()
{
    if (!_radio.initTwoPin(_settings.RadioId, PIN_RADIO_MOMI, PIN_RADIO_SCK, NRFLite::BITRATE250KBPS))
    {
        while (1); // Cannot communicate with radio.
    }
}

void loop()
{
    sleep(_settings.SleepIntervalSeconds);

    RadioPacket radioData;
    radioData.FromRadioId = _settings.RadioId;
    radioData.FailedTxCount = _failedTxCount;
    radioData.Brightness = analogRead(PIN_LDR);
    radioData.Temperature = getTemperature();
    radioData.TemperatureType = _settings.TemperatureType;
    radioData.Voltage = getVcc();

    if (_radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData)))
    {
        if (_radio.hasAckData())
        {
            NewSettingsPacket newSettingsData;
            _radio.readData(&newSettingsData);

            if (newSettingsData.ForRadioId == _settings.RadioId)
            {
                processSettingsChange(newSettingsData);
            }
            else
            {
                sendMessage(F("Ignoring settings change"));
                String msg = F(" request for Radio ");
                msg += String(newSettingsData.ForRadioId);
                sendMessage(msg);
                sendMessage(F(" Please try again"));
            }
        }
    }
    else
    {
        _failedTxCount++;
    }
}

ISR(WDT_vect) // Watchdog interrupt handler.
{
    wdt_disable();
}

float getTemperature()
{
    // Details on https://learn.adafruit.com/thermistor

    float thermReading = analogRead(PIN_THERM);

    float steinhart = 1023 / thermReading - 1;
    steinhart = THERM_SERIES_RESISTOR / steinhart;
    steinhart /= THERM_NOMINAL_RESISTANCE;
    steinhart = log(steinhart);
    steinhart /= THERM_BCOEFFICIENT;
    steinhart += 1.0 / (THERM_NOMIAL_TEMP + 273.15);
    steinhart = 1.0 / steinhart;
    steinhart -= 273.15;                            // Convert to C

    if (_settings.TemperatureType == 1)
    {
        steinhart = (steinhart * 9.0) / 5.0 + 32.0; // Convert to F
    }

    return steinhart + _settings.TemperatureCorrection;
}

float getVcc()
{
    // Details on http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/

    ADMUX = _BV(MUX3) | _BV(MUX2); // Select internal 1.1V reference for measurement.
    delay(2);                      // Let voltage stabilize.
    ADCSRA |= _BV(ADSC);           // Start measuring.
    while (ADCSRA & _BV(ADSC));    // Wait for measurement to complete.
    uint16_t adcReading = ADC;
    float vcc = 1.1 * 1024.0 / adcReading; // Note the 1.1V reference can be off +/- 10%, so calibration is needed.

    return vcc + _settings.VoltageCorrection;
}

void processSettingsChange(NewSettingsPacket newSettings)
{
    String msg;

    if (newSettings.ChangeType == ChangeRadioId)
    {
        msg = F("Changing Id to ");
        msg += newSettings.NewRadioId;
        sendMessage(msg);

        _settings.RadioId = newSettings.NewRadioId;
        setupRadio();
    }
    else if (newSettings.ChangeType == ChangeSleepInterval)
    {
        sendMessage(F("Changing sleep interval"));
        _settings.SleepIntervalSeconds = newSettings.NewSleepIntervalSeconds;
    }
    else if (newSettings.ChangeType == ChangeTemperatureCorrection)
    {
        sendMessage(F("Changing temperature"));
        sendMessage(F(" correction"));
        _settings.TemperatureCorrection = newSettings.NewTemperatureCorrection;
    }
    else if (newSettings.ChangeType == ChangeTemperatureType)
    {
        sendMessage(F("Changing temperature type"));
        _settings.TemperatureType = newSettings.NewTemperatureType;
    }
    else if (newSettings.ChangeType == ChangeVoltageCorrection)
    {
        sendMessage(F("Changing voltage correction"));
        _settings.VoltageCorrection = newSettings.NewVoltageCorrection;
    }

    saveSettings();
}

void saveSettings()
{
    EepromSettings settingsCurrentlyInEeprom;

    eepromBegin();
    eeprom_read_block(&settingsCurrentlyInEeprom, 0, sizeof(settingsCurrentlyInEeprom));
    eepromEnd();

    // Do not waste 1 of the 100,000 guaranteed eeprom writes if settings have not changed.
    if (memcmp(&settingsCurrentlyInEeprom, &_settings, sizeof(_settings)) == 0)
    {
        sendMessage(F("Skipped eeprom save, no change"));
    }
    else
    {
        sendMessage(F("Saving settings"));
        eepromBegin();
        eeprom_write_block(&_settings, 0, sizeof(_settings));
        eepromEnd();
    }
}

void sendMessage(String msg)
{
    MessagePacket messageData;
    messageData.FromRadioId = _settings.RadioId;

    // Ensure the message is not too large for the MessagePacket.
    if (msg.length() > sizeof(messageData.message) - 1)
    {
        msg = msg.substring(0, sizeof(messageData.message) - 1);
    }

    msg.getBytes((unsigned char*)messageData.message, msg.length() + 1);
    _radio.send(DESTINATION_RADIO_ID, &messageData, sizeof(messageData));
}

void sleep(uint16_t seconds)
{
    uint16_t totalPowerDownSeconds;
    uint8_t sleep8Seconds;

    digitalWrite(PIN_SENSOR_POWER, LOW); // Disconnect power from the sensors.
    pinMode(PIN_SENSOR_POWER, INPUT);
    _radio.powerDown();   // Put the radio into a low power state.  It will be powered back on by the library when needed.
    ADCSRA &= ~_BV(ADEN); // Disable ADC to save power.

    while (totalPowerDownSeconds < seconds)
    {
        sleep8Seconds = seconds - totalPowerDownSeconds >= 8;

        if (sleep8Seconds)
        {
            WDTCR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0); // Enable watchdog and set 8 second interrupt time.
            totalPowerDownSeconds += 8;
        }
        else
        {
            WDTCR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1); // 1 second interrupt time.
            totalPowerDownSeconds++;
        }

        wdt_reset();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();

        //MCUCR |= _BV(BODS) | _BV(BODSE);   // Disable brown-out detection (only supported on ATtiny85 RevC and higher).
        //MCUCR = _BV(BODS);

        sleep_cpu();
        sleep_disable();
    }

    ADCSRA |= _BV(ADEN); // Re-enable ADC.
    pinMode(PIN_SENSOR_POWER, OUTPUT); // Provide power to the sensors.
    digitalWrite(PIN_SENSOR_POWER, HIGH);
}
