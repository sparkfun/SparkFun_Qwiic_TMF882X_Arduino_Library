#ifndef _TMF_COMMS_
#define _TMF_COMMS_

#include <Wire.h>
#include <SPI.h>

#define TMF_DEF_ADDR 0x41
#define MAX_SPI_SPEED 10000000
#define MAX_BUFFER_LENGTH 32

#define SPI_READ 0x80

enum {
};

class TMF_COMMS
{  
  public:
    
    
    TMF_COMMS(); // SPI Constructor
    TMF_COMMS(uint8_t address = TMF_DEF_ADDR); // I2C Constructor

    bool begin(TwoWire &wirePort = Wire); 
    bool beginSpi(uint8_t userPin, uint32_t spiPortSpeed = MAX_SPI_SPEED, SPIClass &spiPort = SPI);

  private:
    
    COMMS_STATUS_t _writeRegister(uint8_t, uint8_t);
    COMMS_STATUS_t _writeMultiRegister(uint8_t, uint8_t data[]);
    COMMS_STATUS_t _updateRegister(uint8_t reg, uint8_t mask, uint8_t bits, uint8_t);
    COMMS_STATUS_t _readRegister(uint8_t, uint8_t*);
    COMMS_STATUS_t readMultipleRegisters(uint8_t, uint8_t data[], uint16_t);
    COMMS_STATUS_t overBufLenI2CRead(uint8_t, uint8_t data[], uint16_t);

    uint8_t _address;
    uint8_t _cs;
    uint32_t _spiPortSpeed;

    SPISettings commsSPISettings; 

    TwoWire *_i2cPort;
    SPIClass *_spiPort;
};
#endif
