#include <NRFLite.h>

#define debug(input)   { if (_serial) _serial->print(input);   }
#define debugln(input) { if (_serial) _serial->println(input); }

#if defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    const static uint8_t USI_DI  = 6; // PA6
    const static uint8_t USI_DO  = 5; // PA5
    const static uint8_t USI_SCK = 4; // PA4
#elif defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    const static uint8_t USI_DI  = 0; // PB0
    const static uint8_t USI_DO  = 1; // PB1
    const static uint8_t USI_SCK = 2; // PB2
#else
    #include <SPI.h> // Use the normal Arduino hardware SPI library.
#endif

////////////////////
// Public methods //
////////////////////

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

    // With the microcontroller's pins setup, we can now initialize the radio.
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
    uint8_t statusReg = readRegister(STATUS_NRF);
    if (statusReg & _BV(RX_DR))
    {
        writeRegister(STATUS_NRF, statusReg | _BV(RX_DR));
    }
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
    // If using the same pins for CE and CSN, we need to ensure CE is left HIGH long enough to receive data.
    // If we don't limit the calling program, CE could be LOW so often that no data packets can be received.
    if (!_usingSeparateCeAndCsnPins)
    {
        if (usingInterrupts)
        {
            // Since the calling program is using interrupts, we can trust that it only calls hasData when an
            // interrupt is received, meaning only when data is received.  So we don't need to limit it.
        }
        else
        {
            uint8_t giveRadioMoreTimeToReceive = micros() - _microsSinceLastDataCheck < _maxHasDataIntervalMicros;
            if (giveRadioMoreTimeToReceive)
            {
                return 0; // Prevent the calling program from forcing us to bring CE low, making the radio stop receiving.
            }
            else
            {
                _microsSinceLastDataCheck = micros();
            }
        }
    }
    
    uint8_t notInRxMode = readRegister(CONFIG) != CONFIG_REG_SETTINGS_FOR_RX_MODE;
    if (notInRxMode)
    {
        startRx();
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
    // This method should be used inside an interrupt handler for the radio's IRQ pin to bypass
    // the limit on how often the radio is checked for data.  This optimization greatly increases
    // the receiving bitrate when CE and CSN share the same pin.
    return hasData(1);
}

void NRFLite::readData(void *data)
{
    // Determine length of data in the RX buffer and read it.
    uint8_t dataLength;
    spiTransfer(READ_OPERATION, R_RX_PL_WID, &dataLength, 1);
    spiTransfer(READ_OPERATION, R_RX_PAYLOAD, data, dataLength);
    
    // Clear data received flag.
    uint8_t statusReg = readRegister(STATUS_NRF);
    if (statusReg & _BV(RX_DR))
    {
        writeRegister(STATUS_NRF, statusReg | _BV(RX_DR));
    }
}

uint8_t NRFLite::startRx()
{
    waitForTxToComplete();

    // Put radio into Standby-I mode in order to transition into RX mode.
    digitalWrite(_cePin, LOW);

    // Configure the radio for receiving.
    writeRegister(CONFIG, CONFIG_REG_SETTINGS_FOR_RX_MODE);

    // Put radio into RX mode.
    digitalWrite(_cePin, HIGH);

    // Wait for the transition into RX mode.
    delay(POWERDOWN_TO_RXTX_MODE_MILLIS);

    uint8_t inRxMode = readRegister(CONFIG) == CONFIG_REG_SETTINGS_FOR_RX_MODE;
    return inRxMode;
}

uint8_t NRFLite::send(uint8_t toRadioId, void *data, uint8_t length, SendType sendType)
{
    prepForTx(toRadioId, sendType);

    // Clear any previously asserted TX success or max retries flags.
    writeRegister(STATUS_NRF, _BV(TX_DS) | _BV(MAX_RT));
    
    // Add data to the TX buffer, with or without an ACK request.
    if (sendType == NO_ACK) { spiTransfer(WRITE_OPERATION, W_TX_PAYLOAD_NO_ACK, data, length); }
    else                    { spiTransfer(WRITE_OPERATION, W_TX_PAYLOAD       , data, length); }

    uint8_t result = waitForTxToComplete();
    return result;
}

void NRFLite::startSend(uint8_t toRadioId, void *data, uint8_t length, SendType sendType)
{
    prepForTx(toRadioId, sendType);
    
    // Add data to the TX buffer, with or without an ACK request.
    if (sendType == NO_ACK) { spiTransfer(WRITE_OPERATION, W_TX_PAYLOAD_NO_ACK, data, length); }
    else                    { spiTransfer(WRITE_OPERATION, W_TX_PAYLOAD       , data, length); }
    
    // Start transmission.
    // If we have separate pins for CE and CSN, CE will be LOW and we must pulse it to send the packet.
    if (_usingSeparateCeAndCsnPins)
    {
        digitalWrite(_cePin, HIGH);
        delayMicroseconds(CE_TRANSMISSION_MICROS);
        digitalWrite(_cePin, LOW);
    }
}

void NRFLite::whatHappened(uint8_t &txOk, uint8_t &txFail, uint8_t &rxReady)
{
    uint8_t statusReg = readRegister(STATUS_NRF);
    
    txOk = statusReg & _BV(TX_DS);
    txFail = statusReg & _BV(MAX_RT);
    rxReady = statusReg & _BV(RX_DR);
    
    // When we need to see interrupt flags, we disable the logic here which clears them.
    // Programs that have an interrupt handler for the radio's IRQ pin will use 'whatHappened'
    // and if we don't disable this logic, it's not possible for us to check these flags.
    if (_resetInterruptFlags)
    {
        writeRegister(STATUS_NRF, _BV(TX_DS) | _BV(MAX_RT) | _BV(RX_DR));
    }
}

void NRFLite::powerDown()
{
    // If we have separate CE and CSN pins, we can gracefully transition into Power Down mode by first entering Standby-I mode.
    if (_usingSeparateCeAndCsnPins)
    {
        digitalWrite(_cePin, LOW);
    }
    
    // Turn off the radio.
    writeRegister(CONFIG, readRegister(CONFIG) & ~_BV(PWR_UP));
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

uint8_t NRFLite::scanChannel(uint8_t channel, uint8_t measurementCount)
{
    uint8_t strength = 0;

    // Put radio into Standby-I mode.
    digitalWrite(_cePin, LOW);

    // Set the channel.
    writeRegister(RF_CH, channel);

    // Take a bunch of measurements.
    do
    {
        // Put the radio into RX mode and wait a little time for a signal to be received.
        digitalWrite(_cePin, HIGH);
        delayMicroseconds(400);
        digitalWrite(_cePin, LOW);

        uint8_t signalWasReceived = readRegister(CD);
        if (signalWasReceived)
        {
            strength++;
        }
    } while (measurementCount--);
    
    return strength;
}

/////////////////////
// Private methods //
/////////////////////

uint8_t NRFLite::getPipeOfFirstRxPacket()
{
    // The pipe number is bits 3, 2, and 1.  So B1110 masks them and we shift right by 1 to get the pipe number.
    // 000-101 = Data Pipe Number
    //     110 = Not Used
    //     111 = RX FIFO Empty
    return (readRegister(STATUS_NRF) & B1110) >> 1;
}

uint8_t NRFLite::getRxPacketLength()
{
    // Read the length of the first data packet sitting in the RX buffer.
    uint8_t dataLength;
    spiTransfer(READ_OPERATION, R_RX_PL_WID, &dataLength, 1);

    // Verify the data length is valid (0 - 32 bytes).
    if (dataLength > 32)
    {
        spiTransfer(WRITE_OPERATION, FLUSH_RX, NULL, 0); // Clear invalid data in the RX buffer.
        writeRegister(STATUS_NRF, readRegister(STATUS_NRF) | _BV(TX_DS) | _BV(MAX_RT) | _BV(RX_DR));
        return 0;
    }
    else
    {
        return dataLength;
    }
}

uint8_t NRFLite::initRadio(uint8_t radioId, Bitrates bitrate, uint8_t channel)
{
    _lastToRadioId = -1;
    _resetInterruptFlags = 1;
    _usingSeparateCeAndCsnPins = _cePin != _csnPin;

    delay(OFF_TO_POWERDOWN_MILLIS);

    // Valid channel range is 2400 - 2525 MHz, in 1 MHz increments.
    if (channel > MAX_NRF_CHANNEL) { channel = MAX_NRF_CHANNEL; }
    writeRegister(RF_CH, channel);

    // Transmission speed, retry times, and output power setup.
    // For 2 Mbps or 1 Mbps operation, a 500 uS retry time is necessary to support the max ACK packet size.
    // For 250 Kbps operation, a 1500 uS retry time is necessary.
    if (bitrate == BITRATE2MBPS)
    {
        writeRegister(RF_SETUP, B00001110);   // 2 Mbps, 0 dBm output power
        writeRegister(SETUP_RETR, B00011111); // 0001 =  500 uS between retries, 1111 = 15 retries
        _transmissionRetryWaitMicros = 600;   // 100 more than the retry delay
        _maxHasDataIntervalMicros = 1200;
    }
    else if (bitrate == BITRATE1MBPS)
    {
        writeRegister(RF_SETUP, B00000110);   // 1 Mbps, 0 dBm output power
        writeRegister(SETUP_RETR, B00011111); // 0001 =  500 uS between retries, 1111 = 15 retries
        _transmissionRetryWaitMicros = 600;   // 100 more than the retry delay
        _maxHasDataIntervalMicros = 1700;
    }
    else
    {
        writeRegister(RF_SETUP, B00100110);   // 250 Kbps, 0 dBm output power
        writeRegister(SETUP_RETR, B01011111); // 0101 = 1500 uS between retries, 1111 = 15 retries
        _transmissionRetryWaitMicros = 1600;  // 100 more than the retry delay
        _maxHasDataIntervalMicros = 5000;
    }

    // Assign this radio's address to RX pipe 1.  When another radio sends us data, this is the address
    // it will use.  We use RX pipe 1 to store our address since the address in RX pipe 0 is reserved
    // for use with auto-acknowledgment packets.
    uint8_t address[5] = { 1, 2, 3, 4, radioId };
    writeRegister(RX_ADDR_P1, &address, 5);

    // Enable dynamically sized packets on the 2 RX pipes we use, 0 and 1.
    // RX pipe address 1 is used to for normal packets from radios that send us data.
    // RX pipe address 0 is used to for auto-acknowledgment packets from radios we transmit to.
    writeRegister(DYNPD, _BV(DPL_P0) | _BV(DPL_P1));

    // Enable dynamically sized payloads, ACK payloads, and TX support with or without an ACK request.
    writeRegister(FEATURE, _BV(EN_DPL) | _BV(EN_ACK_PAY) | _BV(EN_DYN_ACK));

    // Ensure RX and TX buffers are empty.  Each buffer can hold 3 packets.
    spiTransfer(WRITE_OPERATION, FLUSH_RX, NULL, 0);
    spiTransfer(WRITE_OPERATION, FLUSH_TX, NULL, 0);

    // Clear any interrupts.
    writeRegister(STATUS_NRF, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));

    uint8_t success = startRx();
    return success;
}

void NRFLite::prepForTx(uint8_t toRadioId, SendType sendType)
{
    if (_lastToRadioId != toRadioId)
    {
        _lastToRadioId = toRadioId;

        // TX pipe address sets the destination radio for the data.
        // RX pipe 0 is special and needs the same address in order to receive ACK packets from the destination radio.
        uint8_t address[5] = { 1, 2, 3, 4, toRadioId };
        writeRegister(TX_ADDR, &address, 5);
        writeRegister(RX_ADDR_P0, &address, 5);
    }

    // Ensure radio is ready for TX operation.
    uint8_t configReg = readRegister(CONFIG);
    uint8_t readyForTx = configReg == (CONFIG_REG_SETTINGS_FOR_RX_MODE & ~_BV(PRIM_RX));
    if (!readyForTx)
    {
        // Put radio into Standby-I mode in order to transition into TX mode.
        digitalWrite(_cePin, LOW);
        configReg = CONFIG_REG_SETTINGS_FOR_RX_MODE & ~_BV(PRIM_RX);
        writeRegister(CONFIG, configReg);
        delay(POWERDOWN_TO_RXTX_MODE_MILLIS);
    }

    uint8_t fifoReg = readRegister(FIFO_STATUS);

    // If RX buffer is full and we require an ACK, clear it so we can receive the ACK response.
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
    _resetInterruptFlags = 0; // Disable interrupt flag reset logic in 'whatHappened'.

    uint8_t fifoReg, statusReg;
    uint8_t txBufferIsEmpty;
    uint8_t packetWasSent, packetCouldNotBeSent;
    uint8_t txAttemptCount = 0;
    uint8_t result = 0; // Default to indicating a failure.

    // TX buffer can store 3 packets, sends retry up to 15 times, and the retry wait time is about half
    // the time necessary to send a 32 byte packet and receive a 32 byte ACK response.  3 x 15 x 2 = 90
    const static uint8_t MAX_TX_ATTEMPT_COUNT = 90;

    while (txAttemptCount++ < MAX_TX_ATTEMPT_COUNT)
    {
        fifoReg = readRegister(FIFO_STATUS);
        txBufferIsEmpty = fifoReg & _BV(TX_EMPTY);

        if (txBufferIsEmpty)
        {
            result = 1; // Indicate success.
            break;
        }

        // If we have separate pins for CE and CSN, CE will be LOW so we must toggle it to send a packet.
        if (_usingSeparateCeAndCsnPins)
        {
            digitalWrite(_cePin, HIGH);
            delayMicroseconds(CE_TRANSMISSION_MICROS);
            digitalWrite(_cePin, LOW);
        }

        delayMicroseconds(_transmissionRetryWaitMicros);
        
        statusReg = readRegister(STATUS_NRF);
        packetWasSent = statusReg & _BV(TX_DS);
        packetCouldNotBeSent = statusReg & _BV(MAX_RT);

        if (packetWasSent)
        {
            writeRegister(STATUS_NRF, _BV(TX_DS));   // Clear TX success flag.
        }
        else if (packetCouldNotBeSent)
        {
            spiTransfer(WRITE_OPERATION, FLUSH_TX, NULL, 0); // Clear TX buffer.
            writeRegister(STATUS_NRF, _BV(MAX_RT));          // Clear max retry flag.
            break;
        }
    }

    _resetInterruptFlags = 1; // Re-enable interrupt reset logic in 'whatHappened'.

    return result;
}

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

void NRFLite::spiTransfer(SpiTransferType transferType, uint8_t regName, void *data, uint8_t length)
{
    uint8_t* intData = reinterpret_cast<uint8_t*>(data);

    noInterrupts(); // Prevent an interrupt from interferring with the communication.

    if (_useTwoPinSpiTransfer)
    {
        digitalWrite(_csnPin, LOW);              // Signal radio to listen to the SPI bus.
        delayMicroseconds(CSN_DISCHARGE_MICROS); // Allow capacitor on CSN pin to discharge.
        twoPinTransfer(regName);
        for (uint8_t i = 0; i < length; ++i) {
            uint8_t newData = twoPinTransfer(intData[i]);
            if (transferType == READ_OPERATION) { intData[i] = newData; }
        }
        digitalWrite(_csnPin, HIGH);             // Stop radio from listening to the SPI bus.
        delayMicroseconds(CSN_DISCHARGE_MICROS); // Allow capacitor on CSN pin to recharge.
    }
    else
    {
        digitalWrite(_csnPin, LOW); // Signal radio to listen to the SPI bus.

        #if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
            // ATtiny transfer with USI.
            usiTransfer(regName);
            for (uint8_t i = 0; i < length; ++i) {
                uint8_t newData = usiTransfer(intData[i]);
                if (transferType == READ_OPERATION) { intData[i] = newData; }
            }
        #else
            // Transfer with the Arduino SPI library.
            SPI.beginTransaction(SPISettings(NRF_SPICLOCK, MSBFIRST, SPI_MODE0));
            SPI.transfer(regName);
            for (uint8_t i = 0; i < length; ++i) {
                uint8_t newData = SPI.transfer(intData[i]);
                if (transferType == READ_OPERATION) { intData[i] = newData; }
            }
            SPI.endTransaction();
        #endif

        digitalWrite(_csnPin, HIGH); // Stop radio from listening to the SPI bus.
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

uint8_t NRFLite::twoPinTransfer(uint8_t data)
{
    uint8_t byteFromRadio = 0;
    uint8_t currentBitIndex = 8;
    
    do
    {
        byteFromRadio <<= 1; // Shift the byte we are building to the left.
        
        if (*_momi_PIN & _momi_MASK) { byteFromRadio++; } // Read bit from radio on MOMI pin.  If HIGH, set bit position 0 of our byte to 1.
        *_momi_DDR |= _momi_MASK;                         // Change MOMI to be an OUTPUT pin.
        
        if (data & 0x80) { *_momi_PORT |=  _momi_MASK; }  // Set MOMI HIGH if bit position 7 of the byte we are sending is 1.

        *_sck_PORT |= _sck_MASK;  // Set SCK HIGH to transfer the bit to the radio.  CSN will remain LOW while the capacitor begins charging.
        *_sck_PORT &= ~_sck_MASK; // Set SCK LOW.  CSN will have remained LOW due to the capacitor.
        
        *_momi_PORT &= ~_momi_MASK; // Set MOMI LOW.
        *_momi_DDR &= ~_momi_MASK;  // Change MOMI back to an INPUT.  Since we previously ensured it was LOW, its PULLUP resistor will never 
                                    // be enabled which would prevent MOMI from fully reaching a LOW state.

        data <<= 1; // Shift the byte we are sending to the left.
    }
    while (--currentBitIndex);
    
    return byteFromRadio;
}

void NRFLite::printRegister(const char name[], uint8_t reg)
{
    String msg = name;
    msg += " ";

    uint8_t i = 8;
    do
    {
        msg += bitRead(reg, --i);
    }
    while (i);

    debugln(msg);
}
