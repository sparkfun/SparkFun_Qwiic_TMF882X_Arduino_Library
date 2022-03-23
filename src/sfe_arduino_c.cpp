

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <Arduino.h>
#include <Wire.h>
#include "inc/sfe_arduino_c.h"
#include "qwiic_tmf882x.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_arduino_c.cpp
//
// This file implements the C interface functions that the underlying TMF SDK/Library uses for platform
// specific functionalty. The platform is Arduino in this scenario.
//
// The implementation of the functions call Arduino C++ code. The functions are called from C, so 
// all function signatures are C, and annotated as such ("extern C") in the header file.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_millis()
//
// Wrapper around Arduino function millis() - keeps Arduino space isolated from AMS code. Used
// in  platform_shim.h - tof_get_timespec()

unsigned long sfe_millis(void){

	return millis();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_usleep()
//
// Wrapper around Arduino function delay() - keeps Arduino space isolated from AMS code. Used
// in  platform_shim.h - tof_sleep()

void sfe_usleep(uint32_t usec){

	// We're passed in microsecs, but we'll use Arduino::delay, which uses milli-secs, so this 
	// will lose some fine resolution...which is fine
	delay(usec/1000);
}

#ifdef DEAD_DEAD_DEAD__
int32_t sfe_queue_msg(struct platform_ctx *ctx, struct tmf882x_msg *msg){

	return ((Qwiic_TMF882X*)ctx->_extra)->_sdk_msg_handler(msg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I2C Things
//
// This exposes a simple C interface, but uses Arduino C++ Wire API for I2c
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Chunk size for large buffer reads
#define kMaxI2CBufferLength 32

// Variables for the i2c comms 
uint8_t tmf_address;    // address of the device

TwoWire * i2cPort;      // pointer to I2C bus port

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_i2c_init()
//
// C function to init the i2c routines for the C interface. 
//
// The Arduino Wire Port is passed in as a void *, since this implements a straight C interface
//
int sfe_i2c_init(uint8_t addresss, void *wireI2CPort){

	// Stash the i2c vars, ping the device 

	tmf_address = addresss;
	i2cPort = (TwoWire*)wireI2CPort;
	
	i2cPort->beginTransmission(tmf_address);

	return i2cPort->endTransmission() ? -1 : 0; // -1 = error, 0 = success
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_write_i2c_block()
//
// Writes a block of data to the i2c device
//
int sfe_write_i2c_block(struct platform_ctx *ctx, uint8_t reg, const uint8_t* data, uint32_t numBytes)
{

	return ((Qwiic_TMF882X*)ctx->_extra)->writeRegisterRegion(reg, (uint8_t*)data, numBytes);
/*
	i2cPort->beginTransmission(tmf_address); 
	i2cPort->write(reg); 
	i2cPort->write(data, (int)numBytes); 

	return i2cPort->endTransmission() ? -1 : 0;  // -1 = error, 0 = success
*/
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_read_i2c_block()
//
// Reads a block of data from an i2c register on the devices.
//
// For large buffers, the data is chuncked over KMaxI2CBufferLength at a time
//
//
int sfe_read_i2c_block(struct platform_ctx *ctx, uint8_t reg, uint8_t* data, uint32_t numBytes)
{

	return ((Qwiic_TMF882X*)ctx->_extra)->readRegisterRegion(reg, data, numBytes);
	/*
 	uint8_t nChunk; 
  	uint8_t nReturned; 

  	i2cPort->beginTransmission(tmf_address);
  	i2cPort->write(reg); 
  	
  	if(i2cPort->endTransmission(false) != 0)
  	 	return -1; // error with the end transmission

  	// Chunk in the data from the bus. This allows efficient data transfer if 
  	// the number of bytes requested is greater than kMaxI2CBufferLenth
  	while(numBytes > 0){
    	
		int i;

		// We're chunking in data - keeping the max chunk to kMaxI2CBufferLength
		nChunk =  numBytes > kMaxI2CBufferLength ? kMaxI2CBufferLength : numBytes;

		// Grab the chunk data - note, if reading last data chunk (nChunk == numBytes),
		// send stop as "true" to release the i2c bus
		nReturned = i2cPort->requestFrom(tmf_address, nChunk, nChunk == numBytes); 

		// No data returned, no dice    	
		if(nReturned == 0 )
			return -1; // error 

		// Copy the retrieved data chunk to the current index in the data segment
		for(i = 0; i < nReturned; i++) 
			*data++ = i2cPort->read();

		// Decrement the amount of data recieved from the overall data request amount 
		numBytes = numBytes - nReturned; 
  	}
  	return 0;  // Success
  	*/
}
#endif