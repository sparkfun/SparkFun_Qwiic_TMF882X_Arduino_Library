/*
  This library defines basic communication functions for the product. 
  By: Elias Santistevan
  Date: 1/01/2021
  License: This code is public domain but you buy me a beer if you use this and 
  we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
 */

#include "TMF_COMMS.h"


bool TMF_COMMS::commsBegin( uint8_t address, TwoWire &commsWirePort ) 
{
  _address = address;
  _i2cPort = &commsWirePort;
  _i2cPort->beginTransmission(_address);
  uint8_t _ret = _i2cPort->endTransmission();
  return (_ret ? false : true);  

}

//*************************************************************************
// Write Functions 
//*************************************************************************

int32_t TMF_COMMS::writeMultiRegister(uint8_t addr, uint8_t reg, const uint8_t data[], uint32_t numBytes)
{
  
	_i2cPort->beginTransmission(_address); 
	_i2cPort->write(reg); 
	_i2cPort->write(data, numBytes); 
	uint8_t retVal = _i2cPort->endTransmission(); 
	return (retVal ? -1 : 0);

}


//***********************************************************
// Read Functions 
//***********************************************************

//Sends a request to read a number of registers
int32_t TMF_COMMS::readMultiRegisters(uint8_t addr, uint8_t reg, const uint8_t* data, uint32_t numBytes)
{

	if( numBytes > MAX_BUFFER_LENGTH ){
		int32_t returnError;
		returnError = overBufLenI2CRead(addr, reg, data, numBytes);
		return returnError;
	}

	_i2cPort->beginTransmission(_address);
	_i2cPort->write(reg);
	uint8_t i2cResult = _i2cPort->endTransmission(false);
	if( i2cResult != 0 )
		return -1; //Error: Sensor did not ack

	i2cResult = _i2cPort->requestFrom(static_cast<uint8_t>(_address), static_cast<uint8_t>(numBytes), static_cast<uint8_t>(true));
	if( i2cResult == 0 ) 
		return -1;
	for(size_t i = 0; i < numBytes; i++) {
		data = _i2cPort->read();
	}

	return 0;

}

// This function is used when more than 32 bytes (TwoWire maximum buffer
// length) of data are requested.
int32_t TMF_COMMS::overBufLenI2CRead(uint8_t addr, uint8_t reg, const uint8_t* data, uint32_t numBytes)
{
  uint8_t resizedRead; 
  uint8_t i2cResult; 
  size_t index = 0;

  _i2cPort->beginTransmission(_address);
  _i2cPort->write(reg); 
  i2cResult = _i2cPort->endTransmission(false);
  if( i2cResult != 0 )
    return -1; 

  while(numBytes > 0) 
  {
    resizedRead =  numBytes > MAX_BUFFER_LENGTH ? MAX_BUFFER_LENGTH : numBytes;
		i2cResult = _i2cPort->requestFrom(static_cast<uint8_t>(_address), resizedRead, static_cast<uint8_t>(false)); 
    if( i2cResult == 0 )
      return -1;

		for(size_t i = 0; i < resizedRead; i++) {
			data = _i2cPort->read();
      index++;
    }	
    numBytes = numBytes - MAX_BUFFER_LENGTH; // end condition
  }
  return 0;
}

