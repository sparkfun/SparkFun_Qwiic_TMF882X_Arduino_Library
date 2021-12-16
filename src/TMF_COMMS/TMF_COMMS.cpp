/*
  This library defines basic communication functions for the product. 
  By: Elias Santistevan
  Date: 1/01/2021
  License: This code is public domain but you buy me a beer if you use this and 
  we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
 */

#include "TMF_COMMS.h"

//TMF_COMMS::TMF_COMMS() {}  

bool TMF_COMMS::commsBegin( uint8_t address, TwoWire &commsWirePort ) 
{
  _address = address;
  _i2cPort = &commsWirePort;
  _i2cPort->beginTransmission(_address);
  uint8_t _ret = _i2cPort->endTransmission();
  return (_ret ? false : true);  

}

bool TMF_COMMS::commsBeginSpi(uint8_t userPin, uint32_t spiPortSpeed, SPIClass &commsSpiPort)  
{
  _i2cPort = NULL;

  _spiPort = &commsSpiPort;
  _spiPortSpeed = spiPortSpeed;
  _cs = userPin;
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  if( _spiPortSpeed > MAX_SPI_SPEED )
    _spiPortSpeed = MAX_SPI_SPEED; 

  _spiPort->begin();


#ifdef ESP32 
  commsSPISettings = SPISettings(spiPortSpeed, SPI_MSBFIRST, userPin);
#else
  commsSPISettings = SPISettings(spiPortSpeed, MSBFIRST, userPin);
#endif

  return true; 
}

//*************************************************************************
// Write Functions 
//*************************************************************************

COMMS_STATUS_t TMF_COMMS::writeRegister(uint8_t reg, uint8_t data)
{
  
  if( _i2cPort == NULL ) {
    _spiPort->beginTransaction(commsSPISettings); 
    digitalWrite(_cs, LOW);
    _spiPort->transfer(reg); 
    _spiPort->transfer(data);
    digitalWrite(_cs, HIGH); 
    _spiPort->endTransaction();
    return COMMS_SUCCESS;
  }
  else { 
    _i2cPort->beginTransmission(_address); 
    _i2cPort->write(reg); 
    _i2cPort->write(data); 
    uint8_t retVal = _i2cPort->endTransmission(); 
    return (retVal ? COMMS_I2C_ERROR : COMMS_SUCCESS);
  }
}

COMMS_STATUS_t TMF_COMMS::writeMultiRegister(uint8_t reg, uint8_t data[], uint8_t numBytes)
{
  
  if( _i2cPort == NULL ) {
    _spiPort->beginTransaction(commsSPISettings); 
    digitalWrite(_cs, LOW); 
    _spiPort->transfer(reg);
    digitalWrite(_cs, HIGH); 
    _spiPort->endTransaction();
    return COMMS_SUCCESS;
  }

  else { 
    _i2cPort->beginTransmission(_address); 
    _i2cPort->write(reg); 
    _i2cPort->write(data, numBytes); 
    uint8_t retVal = _i2cPort->endTransmission(); 
    return (retVal ? COMMS_I2C_ERROR : COMMS_SUCCESS);
  }
}

// A write, but when you want to preserve what's in the registers. 
COMMS_STATUS_t TMF_COMMS::updateRegister(uint8_t reg, uint8_t mask, uint8_t bits, uint8_t startPosition)
{
  
  uint8_t tempData; 
  COMMS_STATUS_t result;

  if( _i2cPort == NULL ) {
    result = readRegister(reg, &tempData);
    if( result != COMMS_SUCCESS )
      return result; 

    tempData &= mask; 
    tempData |= (bits << startPosition); 
    _spiPort->beginTransaction(commsSPISettings); 
    digitalWrite(_cs, LOW); 
    _spiPort->transfer(reg);
    digitalWrite(_cs, HIGH); 
    _spiPort->endTransaction();
    return COMMS_SUCCESS;
  }

  else { 
    result = readRegister(reg, &tempData); 
    if( result != COMMS_SUCCESS )
      return result; 

    tempData &= mask; 
    tempData |= (bits << startPosition);  
    _i2cPort->beginTransmission(_address);
    _i2cPort->write(reg); 
    _i2cPort->write(tempData); 
    uint8_t retVal = _i2cPort->endTransmission(); 
    return (retVal ? COMMS_I2C_ERROR : COMMS_SUCCESS);
  }
}


//***********************************************************
// Read Functions 
//***********************************************************


// This generic function reads an eight bit register. It takes the register's
// address as its' parameter. 
COMMS_STATUS_t TMF_COMMS::readRegister(uint8_t reg, uint8_t *data)
{

  if( _i2cPort == NULL ) {
    _spiPort->beginTransaction(commsSPISettings); 
    digitalWrite(_cs, LOW); 
    _spiPort->transfer(reg |= SPI_READ);  
    data = _spiPort->transfer(0x00); 
    _spiPort->endTransaction();
    return COMMS_SUCCESS; 
  }

  else {
    _i2cPort->beginTransmission(_address); 
    _i2cPort->write(reg); 
    uint8_t i2cError = _i2cPort->endTransmission(); 
    if( i2cError )
      return COMMS_I2C_ERROR;

    _i2cPort->requestFrom(static_cast<uint8_t>(_address), static_cast<uint8_t>(1)); // Read the register, only ever once. 
    *data = _i2cPort->read();
    return COMMS_SUCCESS;
  }
}

//Sends a request to read a number of registers
COMMS_STATUS_t TMF_COMMS::readMultiRegisters(uint8_t reg, uint8_t data[], uint16_t numBytes)
{
	
	if( _i2cPort == NULL ) {
		_spiPort->beginTransaction(commsSPISettings);
		digitalWrite(_cs, LOW);
    _spiPort->transfer(reg | SPI_READ);
		for(size_t i = 0; i < numBytes; i++) {
			data[i] = _spiPort->transfer(0x00); //Assuming this will initiate auto-increment behavior
		}
		digitalWrite(_cs, HIGH);
		_spiPort->endTransaction();
		return COMMS_SUCCESS;
	}

	else {

    if( numBytes > MAX_BUFFER_LENGTH ){
      COMMS_STATUS_t returnError;
      returnError = overBufLenI2CRead(reg, data, numBytes);
      return returnError;
    }

		_i2cPort->beginTransmission(_address);
		_i2cPort->write(reg);
    uint8_t i2cResult = _i2cPort->endTransmission(false);
    if( i2cResult != 0 )
      return COMMS_I2C_ERROR; //Error: Sensor did not ack

		i2cResult = _i2cPort->requestFrom(static_cast<uint8_t>(_address), static_cast<uint8_t>(numBytes), static_cast<uint8_t>(true));
    if( i2cResult == 0 ) 
      return COMMS_I2C_ERROR;
		for(size_t i = 0; i < numBytes; i++) {
			data[i] = _i2cPort->read();
		}
    return COMMS_SUCCESS;
	}
}

// This function is used when more than 32 bytes (TwoWire maximum buffer
// length) of data are requested.
COMMS_STATUS_t TMF_COMMS::overBufLenI2CRead(uint8_t reg, uint8_t data[], uint16_t numBytes)
{
  uint8_t resizedRead; 
  uint8_t i2cResult; 
  size_t index = 0;

  _i2cPort->beginTransmission(_address);
  _i2cPort->write(reg); 
  i2cResult = _i2cPort->endTransmission(false);
  if( i2cResult != 0 )
    return COMMS_I2C_ERROR; 

  while(numBytes > 0) 
  {
    resizedRead =  numBytes > MAX_BUFFER_LENGTH ? MAX_BUFFER_LENGTH : numBytes;
		i2cResult = _i2cPort->requestFrom(static_cast<uint8_t>(_address), resizedRead, static_cast<uint8_t>(false)); 
    if( i2cResult == 0 )
      return COMMS_I2C_ERROR;

		for(size_t i = 0; i < resizedRead; i++) {
			data[index] = _i2cPort->read();
      index++;
    }	
    numBytes = numBytes - MAX_BUFFER_LENGTH; // end condition
  }
  return COMMS_SUCCESS;
}

