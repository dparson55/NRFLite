#ifndef _NRFLite_h_
#define _NRFLite_h_

#include "Arduino.h"
#include "nRF24L01.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

class NRFLite {

  public:

    // Constructors
    // Optionally pass in an Arduino Serial or SoftwareSerial object for use throughout the library when debugging.
    // You can use the 'debug' and 'debugln' macros in NRFLite.cpp to use the serial object.
    NRFLite() {}
    NRFLite(Stream &serial) : _serial(&serial) {}

    enum Bitrates : uint8_t { BITRATE2MBPS, BITRATE1MBPS, BITRATE250KBPS };
    enum SendType : uint8_t { REQUIRE_ACK, NO_ACK };

    static const uint8_t MAX_NRF_CHANNEL = 125; // Maximum channel number.

    // Methods for receivers and transmitters.
    // init         = Turns the radio on and puts it into receiving mode.  Returns 0 if it cannot communicate with the radio.
    //                Channel can be 0-125 and sets the exact frequency of the radio between 2400 - 2525 MHz.
    // initTwoPin   = Same as init but with multiplexed MOSI/MISO and CE/CSN/SCK pins (only works on AVR architectures).
    //                Follow the 2-pin hookup schematic on https://github.com/dparson55/NRFLite
    // readData     = Loads a received data packet or acknowledgment data packet into the specified data parameter.
    // powerDown    = Power down the radio.  Turn the radio back on by calling one of the 'hasData' or 'send' methods.
    // printDetails = Prints many of the radio registers.  Requires a serial object in the constructor, e.g. NRFLite _radio(Serial);
    // scanChannel  = Returns a number between 0 and  measurementCount to indicate the strength of any existing signal on a channel.
    //                Radio communication will work best on channels with no existing signals, meaning a 0 is returned.
    uint8_t init(uint8_t radioId, uint8_t cePin, uint8_t csnPin, Bitrates bitrate = BITRATE2MBPS, uint8_t channel = 100, uint8_t callSpiBegin = 1);
#if defined(__AVR__)
    uint8_t initTwoPin(uint8_t radioId, uint8_t momiPin, uint8_t sckPin, Bitrates bitrate = BITRATE2MBPS, uint8_t channel = 100);
#endif
    void readData(void *data);
    void powerDown();
    void printDetails();
    uint8_t scanChannel(uint8_t channel, uint8_t measurementCount = 255);

    // Methods for transmitters.
    // send       = Puts the radio into TX mode and sends a data packet and waits for success or failure.
    //              The default REQUIRE_ACK sendType causes the radio to attempt sending the packet up to 16 times.
    //              If successful a 1 is returned.  Optionally the NO_ACK sendType can be used to transmit the packet
    //              a single time without asking the receiver to send back an acknowledgement (ACK) packet.
    // hasAckData = Checks to see if an acknowledgement data (ACK data) packet was provided by the receiver
    //              and returns its length.
    uint8_t send(uint8_t toRadioId, void *data, uint8_t length, SendType sendType = REQUIRE_ACK);
    uint8_t hasAckData();

    // Methods for receivers.
    // hasData     = Puts the radio into RX mode and checks to see if a data packet has been received and returns its length.
    // addAckData  = Enqueues an acknowledgment data packet (ACK data) for sending back to a transmitter.  Whenever the
    //               transmitter sends the next data packet, it will get this ACK data packet back as the response.
    //               The radio will store up to 3 ACK data packets and will not enqueue more if full, so you can clear
    //               any stale packets using the 'removeExistingAcks' parameter.
    // discardData = Removes the current received data packet, useful if a packet of an unexpected size is received.
    uint8_t hasData(uint8_t usingInterrupts = 0);
    void addAckData(void *data, uint8_t length, uint8_t removeExistingAcks = 0);
    void discardData(uint8_t unexpectedDataLength);

    // Methods when using the radio's IRQ pin for interrupts.
    // If interrupts are used, do not use the 'send' and 'hasData' functions above and instead use the below functions.
    // hasDataISR   = Same as hasData(1), it will greatly speed up the receive bitrate when CE and CSN share the same pin.
    // startRx      = Allows switching the radio into RX mode rather than calling 'hasDataISR'.
    //                Returns 0 if it cannot communicate with the radio.
    // startSend    = Start sending a data packet without waiting for it to complete.
    // whatHappened = Use this inside the interrupt handler to see what caused the interrupt.
    uint8_t hasDataISR();
    uint8_t startRx();
    void startSend(uint8_t toRadioId, void *data, uint8_t length, SendType sendType = REQUIRE_ACK);
    void whatHappened(uint8_t &txOk, uint8_t &txFail, uint8_t &rxReady);

  private:

    enum SpiTransferType : uint8_t { READ_OPERATION, WRITE_OPERATION };

    constexpr static uint8_t ADDRESS_PREFIX[4] = { 1, 2, 3, 4 }; // 1st 4 bytes of addresses, 5th byte will be RadioId.
    static const uint8_t CONFIG_REG_FOR_RX_MODE = _BV(PWR_UP) | _BV(PRIM_RX) | _BV(EN_CRC);
    static const uint8_t POWERDOWN_TO_RXTX_MODE_MILLIS = 5; // 4500uS to Standby + 130uS to RX or TX mode, so 5ms is enough.

    Stream *_serial;
    Bitrates _savedBitrate;
    uint8_t _savedChannel, _savedRadioId;
    uint8_t _cePin, _csnPin, _momi_MASK, _sck_MASK, _useTwoPinSpiTransfer, _usingSeparateCeAndCsnPins;
    uint16_t _minRxTimeMicros, _txRetryMicros;
    volatile uint8_t _enableIrqReset, *_momi_DDR, *_momi_PORT, *_momi_PIN, *_sck_PORT;

    uint8_t getPipeOfFirstRxPacket();
    uint8_t getRxPacketLength();
    uint8_t initRadio(uint8_t radioId, Bitrates bitrate, uint8_t channel);
    void printRegister(const char name[], uint8_t regName);
    void startTx(uint8_t toRadioId, SendType sendType);
    uint8_t waitForTxToComplete();

    uint8_t readRegister(uint8_t regName);
    void readRegister(uint8_t regName, void* data, uint8_t length);
    void writeRegister(uint8_t regName, uint8_t data);
    void writeRegister(uint8_t regName, void* data, uint8_t length);
    void spiTransfer(SpiTransferType transferType, uint8_t regName, void* data, uint8_t length);

#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    uint8_t usiTransfer(uint8_t data);
#endif

#if defined(__AVR__)
    uint8_t twoPinTransfer(uint8_t data);
#endif

};

#endif
