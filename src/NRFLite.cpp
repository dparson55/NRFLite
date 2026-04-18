#include "NRFLite.h"

#define debug(input)   { if (_serial) _serial->print(input);   }
#define debugln(input) { if (_serial) _serial->println(input); }

#if defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    static const uint8_t USI_DI  = 6; // PA6
    static const uint8_t USI_DO  = 5; // PA5
    static const uint8_t USI_SCK = 4; // PA4
#elif defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    static const uint8_t USI_DI  = 0; // PB0
    static const uint8_t USI_DO  = 1; // PB1
    static const uint8_t USI_SCK = 2; // PB2
#else
    #include "SPI.h" // Use the normal Arduino hardware SPI library.
#endif

////////////
// Public //
////////////

void NRFLite::addAckData(void *data, uint8_t length, uint8_t removeExistingAcks)
{
    if (removeExistingAcks)
    {
        spiTransfer(WRITE_OPERATION, FLUSH_TX, NULL, 0); // Clear the TX buffer.
    }

    // Add the packet to the TX buffer for pipe 1, the pipe used to receive packets from radios that
    // send us data.  When we receive the next transmission from a radio, we'll provide this ACK data in the
    // auto-acknowledgment packet that goes back.
    spiTransfer(WRITE_OPERATION, (W_ACK_PAYLOAD | 1), data, length);
}

void NRFLite::discardData(uint8_t unexpectedDataLength)
{
    // Read data from the RX buffer.
    uint8_t data[unexpectedDataLength];
    spiTransfer(READ_OPERATION, R_RX_PAYLOAD, &data, unexpectedDataLength);

    // Clear data received flag.
    writeRegister(STATUS_NRF, _BV(RX_DR));
}

uint8_t NRFLite::hasAckData()
{
    // If we have a pipe 0 packet sitting at the top of the RX buffer, we have auto-acknowledgment data.
    // We receive ACK data from other radios using the pipe 0 address.
    if (getPipeOfFirstRxPacket() == 0)
    {
        return getRxPacketLength(); // Return the length of the data packet in the RX buffer.
    }
    else
    {
        return 0;
    }
}

uint8_t NRFLite::hasData(uint8_t usingInterrupts)
{
    static uint32_t microsSinceLastRxCheck;

    if (!_usingSeparateCeAndCsnPins)
    {
        // Shared CE and CSN pin operation requires CE to stay HIGH long enough for the radio to receive data.
        // If not using interrupts, we must rate-limit checks to prevent CE from being LOW too frequently.
        // When using interrupts we assume the calling program knows data was received, so we bypass this rate limiter.
        if (!usingInterrupts)
        {
            uint8_t giveRadioMoreRxTime = micros() - microsSinceLastRxCheck < _minRxTimeMicros;

            if (giveRadioMoreRxTime)
            {
                return 0; // Prevent calling program from forcing us to bring CE low, making the radio stop receiving.
            }

            microsSinceLastRxCheck = micros();
        }
    }

    // When the radio is initially powered on its CONFIG defaults to TX mode.
    // So verifying CONFIG is in RX mode also verifies the radio has not lost its NRFLite configuration.
    // This can occur due to a power issue that only impacts the radio and not the microcontroller.
    uint8_t notInRxModeOrNotConfigured = readRegister(CONFIG) != CONFIG_REG_FOR_RX_MODE;
    if (notInRxModeOrNotConfigured)
    {
        initRadio(_savedRadioId, _savedBitrate, _savedChannel);
    }

    // If we have a pipe 1 packet sitting at the top of the RX buffer, we have data.
    if (getPipeOfFirstRxPacket() == 1)
    {
        return getRxPacketLength(); // Return the length of the data packet in the RX buffer.
    }
    else
    {
        return 0;
    }
}

uint8_t NRFLite::hasDataISR()
{
    static const uint8_t USING_INTERRUPTS = 1;
    return hasData(USING_INTERRUPTS);
}

uint8_t NRFLite::init(uint8_t radioId, uint8_t cePin, uint8_t csnPin, Bitrates bitrate, uint8_t channel, uint8_t callSpiBegin)
{
    _cePin = cePin;
    _csnPin = csnPin;
    _useTwoPinSpiTransfer = 0;

    // Default states for the radio pins.  When CSN is LOW the radio listens to SPI communication,
    // so we operate most of the time with CSN HIGH.
    pinMode(_cePin, OUTPUT);
    pinMode(_csnPin, OUTPUT);
    digitalWrite(_csnPin, HIGH);

    // Setup the microcontroller for SPI communication with the radio.
    #if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
        pinMode(USI_DI, INPUT ); digitalWrite(USI_DI, HIGH);
        pinMode(USI_DO, OUTPUT); digitalWrite(USI_DO, LOW);
        pinMode(USI_SCK, OUTPUT); digitalWrite(USI_SCK, LOW);
    #else
        if (callSpiBegin)
        {
            // Arduino SPI makes SS (D10 on ATmega328) an output and sets it HIGH.  It must remain an output
            // for Master SPI operation, but in case it started as LOW, we'll set it back.
            uint8_t savedSS = digitalRead(SS);
            SPI.begin();
            if (_csnPin != SS) digitalWrite(SS, savedSS);
        }
    #endif

    // With the microcontroller's pins setup, we can initialize the radio.
    uint8_t success = initRadio(radioId, bitrate, channel);
    return success;
}

#if defined(__AVR__)

uint8_t NRFLite::initTwoPin(uint8_t radioId, uint8_t momiPin, uint8_t sckPin, Bitrates bitrate, uint8_t channel)
{
    _cePin = sckPin;
    _csnPin = sckPin;
    _useTwoPinSpiTransfer = 1;

    // Default states for the 2 multiplexed pins.
    pinMode(momiPin, INPUT);
    pinMode(sckPin, OUTPUT); digitalWrite(sckPin, HIGH);

    // These port and mask functions are in Arduino.h, e.g. arduino-1.8.1\hardware\arduino\avr\cores\arduino\Arduino.h
    // Direct port manipulation is used since timing is critical for the multiplexed SPI bit-banging.
    _momi_PORT = portOutputRegister(digitalPinToPort(momiPin));
    _momi_DDR = portModeRegister(digitalPinToPort(momiPin));
    _momi_PIN = portInputRegister(digitalPinToPort(momiPin));
    _momi_MASK = digitalPinToBitMask(momiPin);
    _sck_PORT = portOutputRegister(digitalPinToPort(sckPin));
    _sck_MASK = digitalPinToBitMask(sckPin);

    uint8_t success = initRadio(radioId, bitrate, channel);
    return success;
}

#endif

void NRFLite::powerDown()
{
    if (_usingSeparateCeAndCsnPins)
    {
        // Turn off RX or TX operation (enter Standby-I mode).
        digitalWrite(_cePin, LOW);
    }

    // Enter PowerDown mode.
    writeRegister(CONFIG, CONFIG_REG_FOR_RX_MODE & ~_BV(PWR_UP));
}

void NRFLite::printDetails()
{
    printRegister("CONFIG", readRegister(CONFIG));
    printRegister("EN_AA", readRegister(EN_AA));
    printRegister("EN_RXADDR", readRegister(EN_RXADDR));
    printRegister("SETUP_AW", readRegister(SETUP_AW));
    printRegister("SETUP_RETR", readRegister(SETUP_RETR));
    printRegister("RF_CH", readRegister(RF_CH));
    printRegister("RF_SETUP", readRegister(RF_SETUP));
    printRegister("STATUS", readRegister(STATUS_NRF));
    printRegister("OBSERVE_TX", readRegister(OBSERVE_TX));
    printRegister("RX_PW_P0", readRegister(RX_PW_P0));
    printRegister("RX_PW_P1", readRegister(RX_PW_P1));
    printRegister("FIFO_STATUS", readRegister(FIFO_STATUS));
    printRegister("DYNPD", readRegister(DYNPD));
    printRegister("FEATURE", readRegister(FEATURE));

    uint8_t data[5];

    String msg = "TX_ADDR ";
    readRegister(TX_ADDR, &data, 5);
    for (uint8_t i = 0; i < 4; i++) { msg += data[i]; msg += ','; }
    msg += data[4];

    msg += "\nRX_ADDR_P0 ";
    readRegister(RX_ADDR_P0, &data, 5);
    for (uint8_t i = 0; i < 4; i++) { msg += data[i]; msg += ','; }
    msg += data[4];

    msg += "\nRX_ADDR_P1 ";
    readRegister(RX_ADDR_P1, &data, 5);
    for (uint8_t i = 0; i < 4; i++) { msg += data[i]; msg += ','; }
    msg += data[4];

    debugln(msg);
}

void NRFLite::readData(void *data)
{
    // Determine length of data in the RX buffer and read it.
    uint8_t dataLength;
    spiTransfer(READ_OPERATION, R_RX_PL_WID, &dataLength, 1);
    spiTransfer(READ_OPERATION, R_RX_PAYLOAD, data, dataLength);

    // Clear data received flag.
    writeRegister(STATUS_NRF, _BV(RX_DR));
}

uint8_t NRFLite::scanChannel(uint8_t channel, uint8_t measurementCount)
{
    uint8_t strength = 0;

    // Ensure radio is configured for RX.
    uint8_t notInRxModeOrRadioNotConfigured = readRegister(CONFIG) != CONFIG_REG_FOR_RX_MODE;
    if (notInRxModeOrRadioNotConfigured)
    {
        initRadio(_savedRadioId, _savedBitrate, _savedChannel);
    }

    // Turn off radio.
    digitalWrite(_cePin, LOW);

    // Set the channel to scan.
    if (channel > MAX_NRF_CHANNEL) channel = MAX_NRF_CHANNEL;
    writeRegister(RF_CH, channel);

    while (measurementCount--) {
        // Turn on radio and wait for any signals to be received.
        digitalWrite(_cePin, HIGH);
        delayMicroseconds(400);
        digitalWrite(_cePin, LOW);

        uint8_t signalWasReceived = readRegister(CD);
        if (signalWasReceived)
        {
            strength++;
        }
    }

    return strength;
}

uint8_t NRFLite::send(uint8_t toRadioId, void *data, uint8_t length, SendType sendType)
{
    // Clear any previously asserted TX success or max retries flags.
    writeRegister(STATUS_NRF, _BV(TX_DS) | _BV(MAX_RT));

    // Ensure radio is in Standby-II mode.
    startTx(toRadioId, sendType);

    // Add data to the TX buffer, with or without an ACK request.
    if (sendType == NO_ACK) { spiTransfer(WRITE_OPERATION, W_TX_PAYLOAD_NO_ACK, data, length); }
    else                    { spiTransfer(WRITE_OPERATION, W_TX_PAYLOAD       , data, length); }

    uint8_t packetWasSent = waitForTxToComplete();
    return packetWasSent;
}

uint8_t NRFLite::startRx()
{
    // Ensure any queued packets are sent before switching into RX mode.
    waitForTxToComplete();

    // Mode transition: Standby-II -> Standby-I or PowerDown -> Standby-I -> RX.

    // When using shared CE and CSN pins the radio is turned on and off whenever
    // we interact with it via SPI, so we can't hold it in Standby-I to reconfigure
    // it for RX operation. Because of this, we'll enter PowerDown mode instead.

    if (_usingSeparateCeAndCsnPins)
    {
        digitalWrite(_cePin, LOW); // Standby-I mode.
    }
    else
    {
        powerDown(); // PowerDown mode.
    }

    writeRegister(CONFIG, CONFIG_REG_FOR_RX_MODE); // RX configuration and Power on, then Standby-I mode.
    digitalWrite(_cePin, HIGH);                    // RX mode.
    delay(POWERDOWN_TO_RXTX_MODE_MILLIS);          // Power on delay.

    uint8_t readyForRx = readRegister(CONFIG) == CONFIG_REG_FOR_RX_MODE;
    return readyForRx;
}

void NRFLite::startSend(uint8_t toRadioId, void *data, uint8_t length, SendType sendType)
{
    // Ensure radio is in Standby-II mode.
    startTx(toRadioId, sendType);

    // Add data to the TX buffer, with or without an ACK request.
    if (sendType == NO_ACK) { spiTransfer(WRITE_OPERATION, W_TX_PAYLOAD_NO_ACK, data, length); }
    else                    { spiTransfer(WRITE_OPERATION, W_TX_PAYLOAD       , data, length); }

    // It is up to the caller to determine if the packet was sent using 'whatHappened'.
}

void NRFLite::whatHappened(uint8_t &txOk, uint8_t &txFail, uint8_t &rxReady)
{
    uint8_t statusReg = readRegister(STATUS_NRF);

    txOk = (statusReg >> TX_DS) & 1;
    txFail = (statusReg >> MAX_RT) & 1;
    rxReady = (statusReg >> RX_DR) & 1;

    // When we need to see interrupt flags, we disable the logic here which clears them.
    // Programs that have an interrupt handler for the radio's IRQ pin will use 'whatHappened'
    // and if we don't disable this logic, it might not be possible for us to check these flags.
    if (_enableIrqReset)
    {
        // Clear all interrupt flags.
        writeRegister(STATUS_NRF, _BV(TX_DS) | _BV(MAX_RT) | _BV(RX_DR));

        if (txFail)
        {
            // Clear the packet that failed to send.
            spiTransfer(WRITE_OPERATION, FLUSH_TX, NULL, 0);
        }
    }
}

/////////////
// Private //
/////////////

uint8_t NRFLite::getPipeOfFirstRxPacket()
{
    // The pipe number is bits 3, 2, and 1.  So B1110 masks them and we shift right by 1 to get the pipe number.
    // 000-101 = Data Pipe Number
    //     110 = Not Used
    //     111 = RX FIFO Empty
    return (readRegister(STATUS_NRF) & 0b1110) >> 1;
}

uint8_t NRFLite::getRxPacketLength()
{
    // Read the length of the first data packet sitting in the RX buffer.
    uint8_t dataLength;
    spiTransfer(READ_OPERATION, R_RX_PL_WID, &dataLength, 1);

    // Verify the data length is valid. This method is only called if getPipeOfFirstRxPacket
    // indicates a packet exists, so the datalength should never be 0. Likewise the datalength
    // should never be > 32 since that's the largest possible packet the radio supports.
    if (dataLength == 0 || dataLength > 32)
    {
        // Clear invalid data in the RX buffer and data received flag.
        spiTransfer(WRITE_OPERATION, FLUSH_RX, NULL, 0);
        writeRegister(STATUS_NRF, _BV(RX_DR));
        return 0;
    }
    else
    {
        return dataLength;
    }
}

uint8_t NRFLite::initRadio(uint8_t radioId, Bitrates bitrate, uint8_t channel)
{
    _enableIrqReset = 1;
    _lastToRadioId = -1;
    _usingSeparateCeAndCsnPins = _cePin != _csnPin;

    // Store these in case the radio loses its register configuration (potentially
    // from a power fluctuation) that hasn't affected the microcontroller.
    // We'll check the register configuration during sends and receives and if an
    // invalid configuration is detected, we'll call initRadio with these saved values
    // to re-configure the radio.
    _savedRadioId = radioId;
    _savedBitrate = bitrate;
    _savedChannel = channel;

    static const uint8_t OFF_TO_POWERDOWN_MILLIS = 100; // Vcc > 1.9V power on reset time.
    delay(OFF_TO_POWERDOWN_MILLIS);

    // Valid channel range is 2400 - 2525 MHz, in 1 MHz increments.
    if (channel > MAX_NRF_CHANNEL) channel = MAX_NRF_CHANNEL;
    writeRegister(RF_CH, channel);

    // Transmission speed, retry times, and output power setup.
    // For 2 Mbps or 1 Mbps operation, a 500 uS retry time is necessary to support the max ACK packet size.
    // For 250 Kbps operation, a 1500 uS retry time is necessary.
    if (bitrate == BITRATE2MBPS)
    {
        writeRegister(RF_SETUP, 0b00001110);   // 2 Mbps, 0 dBm output power
        writeRegister(SETUP_RETR, 0b00011111); // 0001 =  500 uS between retries, 1111 = 15 retries
        _txRetryMicros = 600;                  // 100 uS more than the retry delay
        _minRxTimeMicros = 1200;               // Required RX time for shared CE and CSN pin operation (just a time vs speed compromise determined by experimentation).
    }
    else if (bitrate == BITRATE1MBPS)
    {
        writeRegister(RF_SETUP, 0b00000110);   // 1 Mbps, 0 dBm output power
        writeRegister(SETUP_RETR, 0b00011111); // 0001 =  500 uS between retries, 1111 = 15 retries
        _txRetryMicros = 600;                  // 100 uS more than the retry delay
        _minRxTimeMicros = 1700;
    }
    else
    {
        writeRegister(RF_SETUP, 0b00100110);   // 250 Kbps, 0 dBm output power
        writeRegister(SETUP_RETR, 0b01011111); // 0101 = 1500 uS between retries, 1111 = 15 retries
        _txRetryMicros = 1600;                 // 100 uS more than the retry delay
        _minRxTimeMicros = 5000;
    }

    // Assign this radio's address to RX pipe 1.  When another radio sends us data, this is the address
    // it will use.  We use RX pipe 1 to store our address since the address in RX pipe 0 is reserved
    // for use with auto-acknowledgment (ACK) packets.
    uint8_t address[5] = { ADDRESS_PREFIX[0], ADDRESS_PREFIX[1], ADDRESS_PREFIX[2], ADDRESS_PREFIX[3], radioId };
    writeRegister(RX_ADDR_P1, &address, 5);

    // Enable dynamically sized packets on the 2 RX pipes we use, 0 and 1.
    // RX pipe address 1 is used to for normal packets from radios that send us data.
    // RX pipe address 0 is used to for ACK packets from radios we transmit to.
    writeRegister(DYNPD, _BV(DPL_P0) | _BV(DPL_P1));

    // Enable dynamically sized payloads, ACK data packet payloads, and TX support with or without an ACK request.
    writeRegister(FEATURE, _BV(EN_DPL) | _BV(EN_ACK_PAY) | _BV(EN_DYN_ACK));

    // Ensure RX and TX buffers are empty.  Each buffer can hold 3 packets.
    spiTransfer(WRITE_OPERATION, FLUSH_RX, NULL, 0);
    spiTransfer(WRITE_OPERATION, FLUSH_TX, NULL, 0);

    // Clear any interrupts.
    writeRegister(STATUS_NRF, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));

    uint8_t success = startRx();
    return success;
}

void NRFLite::printRegister(const char name[], uint8_t reg)
{
    debug(name); debug(' ');

    for (int8_t i = 7; i >= 0; i--) {
        debug((reg >> i) & 1);
    }

    debugln();
}

void NRFLite::startTx(uint8_t toRadioId, SendType sendType)
{
    if (toRadioId != _lastToRadioId)
    {
        _lastToRadioId = toRadioId;

        // TX pipe address sets the destination radio.
        uint8_t address[5] = { ADDRESS_PREFIX[0], ADDRESS_PREFIX[1], ADDRESS_PREFIX[2], ADDRESS_PREFIX[3], toRadioId };
        writeRegister(TX_ADDR, &address, 5);

        // RX pipe 0 needs the same address in order to receive ACK packets from the destination radio.
        writeRegister(RX_ADDR_P0, &address, 5);
    }

    // We enable several features so if none are on, the radio must have lost its configuration.
    // This might occur due to a power issue that only impacts the radio and not the microcontroller.
    uint8_t radioIsNotConfigured = readRegister(FEATURE) == 0;
    if (radioIsNotConfigured)
    {
        initRadio(_savedRadioId, _savedBitrate, _savedChannel);
    }

    // Ensure radio is configured for TX.
    uint8_t readyForTx = readRegister(CONFIG) == (CONFIG_REG_FOR_RX_MODE & ~_BV(PRIM_RX));
    if (!readyForTx)
    {
        // Mode transition: RX -> Standby-I or PowerDown -> Standby-I -> Standby-II.

        // When using shared CE and CSN pins the radio is turned on and off whenever
        // we interact with it via SPI, so we can't hold it in Standby-I to reconfigure
        // it for TX operation. Because of this, we'll enter PowerDown mode instead.

        if (_usingSeparateCeAndCsnPins)
        {
            digitalWrite(_cePin, LOW); // Standby-I mode.
        }
        else
        {
            powerDown(); // PowerDown mode.
        }

        writeRegister(CONFIG, CONFIG_REG_FOR_RX_MODE & ~_BV(PRIM_RX)); // TX configuration, Power on, then Standby-I mode.
        digitalWrite(_cePin, HIGH);                                    // Standby-II mode.
        delay(POWERDOWN_TO_RXTX_MODE_MILLIS);                          // Power on delay.
    }

    uint8_t fifoReg = readRegister(FIFO_STATUS);

    // If RX buffer is full and we require an ACK, it must be cleared to receive the ACK response.
    uint8_t rxBufferIsFull = fifoReg & _BV(RX_FULL);
    if (sendType == REQUIRE_ACK && rxBufferIsFull)
    {
        spiTransfer(WRITE_OPERATION, FLUSH_RX, NULL, 0);
    }

    // If TX buffer is full, wait for all queued packets to be sent.
    uint8_t txBufferIsFull = fifoReg & _BV(FIFO_FULL);
    if (txBufferIsFull)
    {
        waitForTxToComplete();
    }
}

uint8_t NRFLite::waitForTxToComplete()
{
    // TX buffer can store 3 packets, sends retry up to 15 times, and the retry wait time is about half
    // the time necessary to send a 32 byte packet and receive a 32 byte ACK response.  3 x 15 x 2 = 90
    static const uint8_t MAX_TX_ATTEMPT_COUNT = 90;

    uint8_t txAttemptCount = MAX_TX_ATTEMPT_COUNT;
    uint8_t txBufferIsEmpty = readRegister(FIFO_STATUS) & _BV(TX_EMPTY);

    if (txBufferIsEmpty)
    {
        return 0; // There are no packets to send.
    }

    _enableIrqReset = 0; // Disable interrupt flag reset logic in 'whatHappened'.

    while (txAttemptCount--)
    {
        delayMicroseconds(_txRetryMicros); // Wait for transmission.

        txBufferIsEmpty = readRegister(FIFO_STATUS) & _BV(TX_EMPTY);

        uint8_t statusReg = readRegister(STATUS_NRF);
        uint8_t packetWasSent = statusReg & _BV(TX_DS);
        uint8_t packetCouldNotBeSent = statusReg & _BV(MAX_RT);

        if (packetWasSent)
        {
            writeRegister(STATUS_NRF, _BV(TX_DS)); // Clear TX success flag.
        }
        else if (packetCouldNotBeSent)
        {
            writeRegister(STATUS_NRF, _BV(MAX_RT)); // Clear max retry flag.
            break;
        }

        if (txBufferIsEmpty)
        {
            break;
        }
    }

    _enableIrqReset = 1; // Re-enable interrupt reset logic in 'whatHappened'.

    if (txBufferIsEmpty)
    {
        return 1; // All packets were sent.
    }
    else
    {
        // Clear the TX buffer since packets could not be sent.
        spiTransfer(WRITE_OPERATION, FLUSH_TX, NULL, 0);
        return 0;
    }
}

// Register methods

uint8_t NRFLite::readRegister(uint8_t regName)
{
    uint8_t data;
    readRegister(regName, &data, 1);
    return data;
}

void NRFLite::readRegister(uint8_t regName, void *data, uint8_t length)
{
    spiTransfer(READ_OPERATION, (R_REGISTER | (REGISTER_MASK & regName)), data, length);
}

void NRFLite::writeRegister(uint8_t regName, uint8_t data)
{
    writeRegister(regName, &data, 1);
}

void NRFLite::writeRegister(uint8_t regName, void *data, uint8_t length)
{
    spiTransfer(WRITE_OPERATION, (W_REGISTER | (REGISTER_MASK & regName)), data, length);
}

// SPI methods

void NRFLite::spiTransfer(SpiTransferType transferType, uint8_t regName, void *data, uint8_t length)
{
    uint8_t* intData = reinterpret_cast<uint8_t*>(data);

    noInterrupts(); // Prevent an interrupt from interferring with the communication.

    if (_useTwoPinSpiTransfer)
    {
        #if defined(__AVR__)
            // Signal radio to listen to SPI and allow the capacitor on CSN to discharge (CSN reaches LOW state).
            static const uint16_t CSN_DISCHARGE_MICROS = 500;
            digitalWrite(_csnPin, LOW);
            delayMicroseconds(CSN_DISCHARGE_MICROS);

            twoPinTransfer(regName);
            for (uint8_t i = 0; i < length; ++i) {
                uint8_t newData = twoPinTransfer(intData[i]);
                if (transferType == READ_OPERATION) intData[i] = newData;
            }

            // Signal radio to stop listening to SPI and allow the capacitor to recharge.
            digitalWrite(_csnPin, HIGH);
            delayMicroseconds(CSN_DISCHARGE_MICROS);
	    #endif
    }
    else
    {
        digitalWrite(_csnPin, LOW); // Signal radio to listen to SPI.

        #if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
            // ATtiny transfer with USI.
            usiTransfer(regName);
            for (uint8_t i = 0; i < length; ++i) {
                uint8_t newData = usiTransfer(intData[i]);
                if (transferType == READ_OPERATION) intData[i] = newData;
            }
        #else
            // Transfer with the Arduino SPI library.
            static const uint32_t NRF_SPICLOCK = 4000000;
            SPI.beginTransaction(SPISettings(NRF_SPICLOCK, MSBFIRST, SPI_MODE0));
            SPI.transfer(regName);
            for (uint8_t i = 0; i < length; ++i) {
                uint8_t newData = SPI.transfer(intData[i]);
                if (transferType == READ_OPERATION) intData[i] = newData;
            }
            SPI.endTransaction();
        #endif

        digitalWrite(_csnPin, HIGH); // Stop radio from listening to SPI.
    }

    interrupts();
}

#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)

uint8_t NRFLite::usiTransfer(uint8_t data)
{
    USIDR = data;
    USISR = _BV(USIOIF);

    while ((USISR & _BV(USIOIF)) == 0)
    {
        USICR = _BV(USIWM0) | _BV(USICS1) | _BV(USICLK) | _BV(USITC);
    }

    return USIDR;
}

#endif

#if defined(__AVR__)

uint8_t NRFLite::twoPinTransfer(uint8_t outputByte)
{
    uint8_t bit = 8;
    uint8_t inputByte = 0;

    // Inspired by https://nerdralph.blogspot.com/2015/05/nrf24l01-control-with-2-mcu-pins-using.html
    // NRFLite uses different capacitor and resistor values, see schematic on https://github.com/dparson55/NRFLite

    // MOMI changes between INPUT and OUTPUT during reads and writes to radio.
    // SCK remains OUTPUT and remains LOW for the majority of the time which prevents the radio's CSN from going HIGH.
    // Starting state: MOMI = INPUT  and LOW (controls radio MISO and MOSI)
    //                  SCK = OUTPUT and LOW (controls radio CE, CSN, and SCK)

    while (bit--)
    {
        // Read bit from radio.
        inputByte <<= 1;                          // Shift byte we are building to the left.
        if (*_momi_PIN & _momi_MASK) inputByte++; // Read bit on MOMI. If HIGH set bit position 0 to 1.

        // Ready bit to write.
        *_momi_DDR |= _momi_MASK;                               // Change MOMI to OUTPUT.
        if (outputByte & 0b10000000) *_momi_PORT |= _momi_MASK; // Set MOMI HIGH if bit position 7 of the byte we are sending is 1.
        outputByte <<= 1;                                       // Shift the byte we are sending to the left.

        // Pulse SCK. Radio will read the bit we prepared and set the next bit it is outputing on its MISO pin.
        *_sck_PORT |= _sck_MASK;    // Set SCK HIGH. The capacitor keeping CSN LOW will begin charging so we need to go back to LOW.
        *_sck_PORT &= ~_sck_MASK;   // Set SCK LOW. CSN will hopefully have remained LOW due to the capacitor, otherwise SPI communication will have stopped.

        // Get read to read next bit.
        *_momi_PORT &= ~_momi_MASK; // Set MOMI LOW so its PULL_UP resistor won't be enabled when we change it to INPUT.
        *_momi_DDR  &= ~_momi_MASK; // Change MOMI to INPUT.
    }

    return inputByte;
}

#endif
