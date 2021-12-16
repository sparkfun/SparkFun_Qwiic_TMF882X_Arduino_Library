#ifndef _TMF_COMMS_
#define _TMF_COMMS_

#include <Wire.h>
#include <SPI.h>

#define MAX_SPI_SPEED 10000000
#define MAX_BUFFER_LENGTH 32

#define SPI_READ 0x80

typedef enum COMMS_STATUS_t {

  COMMS_SUCCESS = 0x00,
  COMMS_I2C_ERROR = 0xFF

};

enum {
};

class TMF_COMMS
{  
  public:
    
//    TMF_COMMS(); 

    bool commsBegin(uint8_t address, TwoWire &commsWirePort = Wire); 
    bool commsBeginSpi(uint8_t userPin, uint32_t spiPortSpeed = MAX_SPI_SPEED, SPIClass &commsSpiPort = SPI);

    COMMS_STATUS_t writeRegister(uint8_t, uint8_t);
    COMMS_STATUS_t writeMultiRegister(uint8_t, uint8_t data[], uint8_t);
    COMMS_STATUS_t updateRegister(uint8_t reg, uint8_t mask, uint8_t bits, uint8_t);
    COMMS_STATUS_t readRegister(uint8_t, uint8_t*);
    COMMS_STATUS_t readMultiRegisters(uint8_t, uint8_t data[], uint16_t);
    COMMS_STATUS_t overBufLenI2CRead(uint8_t, uint8_t data[], uint16_t);

    uint8_t _address;
    uint8_t _cs;
    uint32_t _spiPortSpeed;

    SPISettings commsSPISettings; 

    TwoWire *_i2cPort;
    SPIClass *_spiPort;
};
#endif
