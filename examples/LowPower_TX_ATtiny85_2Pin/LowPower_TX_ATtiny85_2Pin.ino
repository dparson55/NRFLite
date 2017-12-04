/*

Demonstrates TX operation with an ATtiny85 using 2 pins for the radio.
The ATtiny85 and radio power up, take voltage and temperature readings, send the data, and then power down.
An LED on Arduino Pin 0 is enabled whenever the ATtiny85 and radio are powered on.

Radio circuit
* Follow the 2-Pin Hookup Guide on https://github.com/dparson55/NRFLite

Thermistor circuit
* VCC > 10K resistor > thermistor > GND

Connections
* Physical Pin 2, Arduino 3 > Connect in between 10K resistor and thermistor
* Physical Pin 3, Arduino 4 > MOMI of 2-pin circuit
* Physical Pin 5, Arduino 0 > LED > 1K resistor > GND
* Physical Pin 6, Arduino 1 > SCK of 2-pin circuit
* Physical Pin 7, Arduino 2 > No connection

*/

#include <NRFLite.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

const static uint8_t PIN_THERM = 3;
const static uint8_t PIN_RADIO_MOMI = 4;
const static uint8_t PIN_LED = 0;
const static uint8_t PIN_RADIO_SCK = 1;

const static uint8_t RADIO_ID = 1;
const static uint8_t DESTINATION_RADIO_ID = 0;

const static uint32_t POWER_DOWN_SECONDS = 3;

const static uint16_t THERM_BCOEFFICIENT = 4050;
const static uint8_t  THERM_NOMIAL_TEMP = 25;           // Thermistor nominal temperature in C.
const static uint16_t THERM_NOMINAL_RESISTANCE = 10000; // Thermistor resistance at the nominal temperature.
const static uint16_t THERM_SERIES_RESISTOR = 10000;    // Value of resistor in series with the thermistor.

struct RadioPacket
{
    uint8_t FromRadioId;
    float Temperature;
    float Voltage;
    uint32_t FailedTxCount;
};

NRFLite _radio;
RadioPacket _radioData;

ISR(WDT_vect) { wdt_disable(); } // Watchdog interrupt handler.

void setup()
{
    pinMode(PIN_LED, OUTPUT);

    if (!_radio.initTwoPin(RADIO_ID, PIN_RADIO_MOMI, PIN_RADIO_SCK, NRFLite::BITRATE250KBPS))
    {
        while (1); // Cannot communicate with radio.
    }

    _radioData.FromRadioId = RADIO_ID;
}

void loop()
{
    digitalWrite(PIN_LED, HIGH); // Indicate powered-on state.

    _radioData.Temperature = getTemp();
    _radioData.Voltage = getVcc();

    if (!_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData)))
    {
        _radioData.FailedTxCount++;
    }

    digitalWrite(PIN_LED, LOW); // Indicate powered-off state.

    powerDown(POWER_DOWN_SECONDS);
}

float getTemp()
{
    // Details available on https://learn.adafruit.com/thermistor

    float thermReading = analogRead(PIN_THERM);

    float steinhart = 1023 / thermReading - 1;
    steinhart = THERM_SERIES_RESISTOR / steinhart;
    steinhart /= THERM_NOMINAL_RESISTANCE;
    steinhart = log(steinhart);
    steinhart /= THERM_BCOEFFICIENT;
    steinhart += 1.0 / (THERM_NOMIAL_TEMP + 273.15);
    steinhart = 1.0 / steinhart;
    steinhart -= 273.15;                             // Convert to C
    steinhart = (steinhart * 9.0) / 5.0 + 32.0;      // Convert to F

    return steinhart;
}

float getVcc()
{
    // Details available on http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/

    ADMUX = _BV(MUX3) | _BV(MUX2); // Select internal 1.1V reference for measurement.
    delay(2);                      // Let voltage stabilize.
    ADCSRA |= _BV(ADSC);           // Start measuring.
    while (ADCSRA & _BV(ADSC));    // Wait for measurement to complete.
    uint16_t adcReading = ADC;
    float vcc = 1.1 * 1024.0 / adcReading; // Note the 1.1V reference can be off +/- 10%, so calibration is needed.

    return vcc;
}

void powerDown(uint32_t seconds)
{
    uint32_t totalPowerDownSeconds;
    uint8_t sleep8Seconds;

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
}