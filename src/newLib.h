#ifndef _HEADER_PROTECTION_NAME_
#define _HEADER_PROTECTION_NAME_

#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>

#define 
#define 
#define 

enum {
};

class Class_Name
{  
  public:
    
    // Public Variables
    
    //Function declarations
    Class_Name(); // SPI Constructor
    Class_Name(uint8_t address); // I2C Constructor

    bool begin( TwoWire &wirePort );
    bool beginSpi(uint8_t userCsPin, SPIClass &spiPort);

    bool begin(TwoWire &wirePort = Wire); // begin function
    bool beginSpi(SPIClass &spiPort = SPI); 

  private:
    
    // Private Variables
    uint8_t _address;

    //SPI settings class sets the speed, chip select, and bit order
    SPISettings mySpi; 

    // This generic function handles I2C write commands for modifying individual
    // bits in an eight bit register. Paramaters include the register's address, a mask 
    // for bits that are ignored, the bits to write, and the bits' starting
    // position.
    void writeRegister(uint8_t _wReg, uint8_t _mask, uint8_t _bits, uint8_t _startPosition);

    // This generic function does a basic I-squared-C write transaction at the
    // given address, and writes the given _command argument. 
    void _writeCommand(uint8_t _command);

    // This generic function reads an eight bit register. It takes the register's
    // address as its' parameter. 
    uint8_t readRegister(uint8_t _reg);

    // This generic function does a basic I-squared-C read transaction at the given
    // addres, taking the number of reads as argument. 
    uint8_t _readCommand(uint8_t _numReads);


    TwoWire *_i2cPort;
    SPIClass *_spiPort;
};
#endif
