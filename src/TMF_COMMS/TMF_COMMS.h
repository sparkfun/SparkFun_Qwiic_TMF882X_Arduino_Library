#ifndef _TMF_COMMS_
#define _TMF_COMMS_

#include <Arduino.h>
#include <Wire.h>
#define MAX_BUFFER_LENGTH 32
#define DEF_TMF_ADDR 0x41

class TMF_COMMS
{  
  public:
    
    TMF_COMMS(); 

    int32_t commsBegin(uint8_t address = DEF_TMF_ADDR, TwoWire &commsWirePort = Wire); 

    int32_t write_i2c_block(uint8_t, uint8_t, const uint8_t* data, uint32_t);
    int32_t read_i2c_block(uint8_t, uint8_t, uint8_t* data, uint32_t);
		int32_t overBufLenI2CRead(uint8_t, uint8_t, uint8_t* data, uint32_t);

    uint8_t _address = DEF_TMF_ADDR;
		
    TwoWire *_i2cPort = &Wire;
};

extern "C" int32_t write_i2c_block(uint8_t, uint8_t, const uint8_t* data, uint32_t);
extern "C" int32_t read_i2c_block(uint8_t, uint8_t, uint8_t* data, uint32_t);
extern "C" void tmf_delay(unsigned long);

#endif //TMF_COMMS

