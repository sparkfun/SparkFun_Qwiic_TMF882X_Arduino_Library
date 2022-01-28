#ifndef _TMF_COMMS_
#define _TMF_COMMS_

// #ifdef __cpluplus
#include <Arduino.h>
#include <Wire.h>
#define MAX_BUFFER_LENGTH 32

class TMF_COMMS
{  
  public:
    
//    TMF_COMMS(); 

    bool commsBegin(uint8_t address, TwoWire &commsWirePort = Wire); 

    int32_t write_i2c_block(uint8_t, uint8_t, const uint8_t* data, uint32_t);
    int32_t read_i2c_block(uint8_t, uint8_t, uint8_t* data, uint32_t);
		int32_t overBufLenI2CRead(uint8_t, uint8_t, uint8_t* data, uint32_t);

    uint8_t _address;

    TwoWire *_i2cPort;
};

extern "C" int32_t write_i2c_block(uint8_t, uint8_t, const uint8_t* data, uint32_t);
extern "C" int32_t read_i2c_block(uint8_t, uint8_t, uint8_t* data, uint32_t);
// #endif//__cplusplus
#endif //TMF_COMMS

