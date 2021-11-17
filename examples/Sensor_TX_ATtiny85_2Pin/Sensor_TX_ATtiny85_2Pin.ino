/*

Requires the use of https://github.com/damellis/attiny for the ATtiny85 rather than
https://github.com/SpenceKonde/ATTinyCore since ATTinyCore takes up too much memory.

Demonstrates TX operation with an ATtiny85 using 2 pins for the radio.
* The ATtiny powers up, takes sensor readings, sends the data, and then powers down.
* Current consumption is about 5 microamps when powered down.
* The watchdog timer is used to wake the the ATtiny at a configurable interval.
* The receiver can change certain settings by providing an acknowledgement NewSettingsPacket packet, see
  the 'Sensor_RX' example for more detail.  New settings are saved in eeprom.
* Informational messages are sent to the receiver for certain events, like when the eeprom is loaded.
  This logic can be used for remote debugging, which is very cool.
* Physical pin 5 (Arduino 0) acts as a power switch.  The radio and sensors all have VCC connected,
  and physical pin 5 provides their connection to GND.  So when the ATtiny wakes from sleep, it makes
  physical pin 5 an OUTPUT and sets it LOW in order to power-on the radio and sensors.
* Follow the 2-Pin Hookup Guide on https://github.com/dparson55/NRFLite to create the MOMI and SCK
  connections for the radio.
* There is an image of the circuit in the same folder as this .ino file.

ATtiny Connections
* Physical pin 1 > Reset button > GND
* Physical pin 2 > Connect between 10K resistor and thermistor
* Physical pin 3 > MOMI of radio 2-pin circuit
* Physical pin 4 > GND
* Physical pin 5 > Connect to the radio GND and the GND for the thermistor, LED, and LDR circuits
* Physical pin 6 > SCK of radio 2-pin circuit
* Physical pin 7 > Connect between LDR and 1K resistor
* Physical pin 8 > VCC (no more than 3.6 volts since that is the max for the radio)

*/

#include "NRFLite.h"
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

const static uint8_t PIN_RADIO_MOMI = 4;
const static uint8_t PIN_RADIO_SCK = 1;
const static uint8_t PIN_LDR = A1;
const static uint8_t PIN_THERM = A3;
const static uint8_t PIN_POWER_BUS = 0;

const static uint8_t DESTINATION_RADIO_ID = 0;

const static uint16_t THERM_BCOEFFICIENT = 4050;
const static uint8_t  THERM_NOMIAL_TEMP = 25;           // Thermistor nominal temperature in C.
const static uint16_t THERM_NOMINAL_RESISTANCE = 10000; // Thermistor resistance at the nominal temperature.
const static uint16_t THERM_SERIES_RESISTOR = 10000;    // Value of resistor in series with the thermistor.

const uint8_t EEPROM_SETTINGS_VERSION = 1;

struct EepromSettings
{
    uint8_t RadioId;
    uint32_t SleepIntervalSeconds;
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
    uint32_t NewSleepIntervalSeconds;
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
    // Enable the power bus.  This provides power to all the sensors and radio, and we'll turn it off when we sleep to conserve power.
    pinMode(PIN_POWER_BUS, OUTPUT);
    digitalWrite(PIN_POWER_BUS, LOW);

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
        while (1); // Cannot communicate with the radio so stop all processing.
    }
}

void loop()
{
    // Put the microcontroller, sensors, and radio into a low power state.  Processing stops here until the watchdog timer wakes us up.
    sleep(_settings.SleepIntervalSeconds);

    // Now that we are awake, collect all the sensor readings.
    RadioPacket radioData;
    radioData.FromRadioId = _settings.RadioId;
    radioData.FailedTxCount = _failedTxCount;
    radioData.Brightness = analogRead(PIN_LDR);
    radioData.Temperature = getTemperature();
    radioData.TemperatureType = _settings.TemperatureType;
    radioData.Voltage = getVcc();

    // Try sending the sensor data.
    if (_radio.send(DESTINATION_RADIO_ID, &radioData, sizeof(radioData)))
    {
        // If we are here it means the data was sent successful.
      
        // Check to see if the receiver provided an acknowledgement data packet.
        // It will do this if it wants us to change one of our settings.
        if (_radio.hasAckData())
        {
            NewSettingsPacket newSettingsData;
            _radio.readData(&newSettingsData);

            // Confirm the settings we received are meant for us (maybe it was trying to change the settings for a different transmitter).
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

    // Convert the message string into an array of bytes.
    msg.getBytes((unsigned char*)messageData.message, msg.length() + 1);
  
    _radio.send(DESTINATION_RADIO_ID, &messageData, sizeof(messageData));
}

void sleep(uint32_t seconds)
{
    uint32_t totalPowerDownSeconds = 0;
    uint8_t canSleep8Seconds;

    _radio.powerDown();            // Put the radio into a low power state.
    pinMode(PIN_POWER_BUS, INPUT); // Disconnect power bus.
    ADCSRA &= ~_BV(ADEN);          // Disable ADC to save power.

    while (totalPowerDownSeconds < seconds)
    {
        canSleep8Seconds = seconds - totalPowerDownSeconds >= 8;

        if (canSleep8Seconds)
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

    ADCSRA |= _BV(ADEN);            // Re-enable ADC.
    pinMode(PIN_POWER_BUS, OUTPUT); // Re-enable power bus.  It will already be LOW since it was configured in 'setup()'.
    setupRadio();                   // Re-initialize the radio.
}
