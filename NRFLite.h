#ifndef _NRFLite_h_
#define _NRFLite_h_

#include <Arduino.h>
#include <nRF24L01.h>

class NRFLite {
    
    public:
    
    // Constructors
    // You can pass in an Arduino Serial or SoftwareSerial object for use throughout the library when debugging.
	// This approach allows both Serial and SoftwareSerial support so debugging on ATtinys is easy.
    NRFLite() {}
    NRFLite(Stream& serial) : _serial(&serial) {}
    
    enum Bitrates { BITRATE2MBPS, BITRATE1MBPS, BITRATE250KBPS };
    enum SendType { REQUIRE_ACK, NO_ACK };
    
    // Methods for receivers and transmitters.
    // init      = Turns the radio on and puts it into receiving mode.  Returns 0 if it cannot communicate with the radio.
	//             Channel can be 0-125 and sets the exact frequency of the radio between 2400 - 2525 MHz.
    // readData  = Loads a received data packet or ACK packet into the specified data parameter.
    // powerDown = Power down the radio.  It only draws 900 nA in this state.  The radio will be powered back on when one of the 
    //             'hasData' or 'send' methods is called.
    // printDetails = For debugging, it prints most radio registers using the serial object provided in the constructor.
    uint8_t init(uint8_t radioId, uint8_t cePin, uint8_t csnPin, Bitrates bitrate = BITRATE2MBPS, uint8_t channel = 100); 
    void readData(void* data);
    void powerDown();
    void printDetails();

    // Methods for transmitters.
    // send       = Sends a data packet and waits for success or failure.  If NO_ACK is specified, no acknowledgment is required.
    // hasAckData = Checks to see if an ACK data packet was received and returns its length.
    uint8_t send(uint8_t toRadioId, void* data, uint8_t length, SendType sendType = REQUIRE_ACK);
    uint8_t hasAckData();

    // Methods for receivers.
    // hasData    = Checks to see if a data packet has been received and returns its length.
    // addAckData = Queues an acknowledgment packet for sending back to a transmitter.  Whenever the transmitter sends the 
    //              next data packet, it will get this ACK packet back in the response.  The radio will store up to 3 ACK packets
    //              but you can clear this buffer if you like using the 'removeExistingAcks' parameter.
    uint8_t hasData(uint8_t usingInterrupts = 0);
    void addAckData(void* data, uint8_t length, uint8_t removeExistingAcks = 0); 
    
    // Methods when using the radio's IRQ pin for interrupts.
    // startSend    = Start sending a data packet without waiting for it to complete.
    // whatHappened = Use this inside the interrupt handler to see what caused the interrupt.
    // hasDataISR   = Same as hasData(1) and is just for clarity.  It will greatly speed up the receive bitrate when CE and CSN 
    //                share the same pins.
    void startSend(uint8_t toRadioId, void* data, uint8_t length, SendType sendType = REQUIRE_ACK); 
    void whatHappened(uint8_t& tx_ok, uint8_t& tx_fail, uint8_t& rx_ready); 
    uint8_t hasDataISR(); 
    
    private:
    
    enum SpiTransferType { READ_OPERATION, WRITE_OPERATION };

    Stream* _serial;
    uint8_t _cePin, _csnPin, _enableInterruptFlagsReset;
    uint16_t _transmissionRetryWaitMicros, _allowedDataCheckIntervalMicros;
    uint64_t _microsSinceLastDataCheck;
    
    uint8_t getPipeOfFirstRxFifoPacket();
    uint8_t getRxFifoPacketLength();
    void prepForTransmission(uint8_t toRadioId, SendType sendType);
    uint8_t readRegister(uint8_t regName);
    void readRegister(uint8_t regName, void* data, uint8_t length);
    void writeRegister(uint8_t regName, uint8_t data);
    void writeRegister(uint8_t regName, void* data, uint8_t length);
    void spiTransfer(SpiTransferType transferType, uint8_t regName, void* data, uint8_t length);
    uint8_t usiTransfer(uint8_t data);    
    void printRegister(char* name, uint8_t regName);
};

#endif


