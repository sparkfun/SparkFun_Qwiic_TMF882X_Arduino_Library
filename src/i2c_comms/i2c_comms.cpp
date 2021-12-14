/*
  This is a library for...
  By: Elias Santistevan
  Date: 
  License: This code is public domain but you buy me a beer if you use this and 
  we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!
 */

#include "TMF_COMMS.h"

TMF_COMMS::TMF_COMMS()   
TMF_COMMS::TMF_COMMS(uint8_t address) :  _address(address) 

bool TMF_COMMS::begin( TwoWire &wirePort ) : _i2cPort(&wirePort)
{
  
  _i2cPort->beginTransmission(_address);
  uint8_t _ret = _i2cPort->endTransmission();
  return (_ret ? false : true);  

}

bool TMF_COMMS::beginSpi(uint8_t userPin, uint32_t spiPortSpeed, SPIClass &spiPort) : _cs(userPin), _spiPort(&spiPort)
{
  _i2cPort = NULL;

  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  _spiPort.begin();

#ifdef ESP32 
  mySpi = SPISettings(spiPortSpeed, SPI_MSBFIRST, userCsPin);
#else
  mySpi = SPISettings(spiPortSpeed, MSBFIRST, userCsPin);
#endif

  return true; 
}

COMMS_STATUS_t TMF_COMMS::_writeRegister(uint8_t reg, uint8_t dataToWrite)
{
  
  if(_i2cPort == NULL) {
    _spiPort->beginTransaction(tmfSPISettings); 
    digitalWrite(_cs, LOW);
    _spiPort->transfer(reg); 
    _spiPort->transfer(_spiWrite);
    digitalWrite(_cs, HIGH); 
    _spiPort->endTransaction();
  }
  else { 
    _i2cPort->beginTransmission(_address); 
    _i2cPort->write(reg); 
    _i2cPort->write(dataToWrite); 
    uint8_t retVal = _i2cPort->endTransmission(); 
    return (retval ? COMMS_I2C_ERROR : COMMS_SUCCESS))
  }
}

COMMS_STATUS_t TMF_COMMS::_writeMultiRegister(uint8_t reg, uint8_t dataToWrite[])
{
  
  if(_i2cPort == NULL) {
    _spiPort->beginTransaction(tmfSPISettings); 
    digitalWrite(_cs, LOW); 
    _spiPort->transfer(reg);
    _spiPort->transfer(_spiWrite);
    digitalWrite(_cs, HIGH); 
    _spiPort->endTransaction();
  }
  else { 
    _i2cPort->beginTransmission(_address); 
    _i2cPort->write(reg); 
    _i2cPort->write(dataToWrite); 
    uint8_t retVal = _i2cPort->endTransmission(); 
    return (retval ? COMMS_I2C_ERROR : COMMS_SUCCESS))
  }
}

// A write, but when you want to preserve what's in the registers. 
COMMS_STATUS_t TMF_COMMS::_updateRegister(uint8_t reg, uint8_t mask, uint8_t bits, uint8_t startPosition)
{
  
  if(_i2cPort == NULL) {
    _spiWrite = readRegister(reg);
    _spiWrite &= mask; 
    _spiWrite |= (bits << startPosition); 
    _spiPort->beginTransaction(tmfSPISettings); 
    digitalWrite(_cs, LOW); 
    _spiPort->transfer(reg);
    _spiPort->transfer(_spiWrite);
    digitalWrite(_cs, HIGH); 
    _spiPort->endTransaction();
  }
  else { 
    uint8_t _i2cWrite = readRegister(reg); 
    _i2cWrite &= mask; 
    _i2cWrite |= (bits << startPosition);  
    _i2cPort->beginTransmission(_address);
    _i2cPort->write(reg); 
    _i2cPort->write(_i2cWrite); 
    uint8_t retVal = _i2cPort->endTransmission(); 
    return (retval ? COMMS_I2C_ERROR : COMMS_SUCCESS)
  }
}

//Sends a request to read a number of registers
COMMS_STATUS_t TMF_COMMS::readMultipleRegisters(uint8_t reg, uint8_t dataBuffer[], uint16_t numBytes)
{
	
	if( _i2cPort == NULL ) {
		_spiPort->beginTransaction(kxSPISettings);
		digitalWrite(_cs, LOW);
		reg |= SPI_READ;
    _spiPort->transfer(reg);
		dataBuffer[0] = _spiPort->transfer(0x00); //first byte on transfer of address and read bit
		for(size_t i = 1; i < numBytes; i++) {
			dataBuffer[i] = _spiPort->transfer(0x00); //Assuming this will initiate auto-increment behavior
		}
		digitalWrite(_cs, HIGH);
		_spiPort->endTransaction();
		return COMMS_SUCCESS;
	}
	else {

    if( numBytes > MAX_BUFFER_LENGTH ){
      COMMS_STATUS_t returnError;
      returnError = overBufLenI2CRead(reg, dataBuffer, numBytes);
      return returnError;
    }

		_i2cPort->beginTransmission(_deviceAddress);
		_i2cPort->write(reg);
    uint8_t i2cResult = _i2cPort->endTransmission(false);
    if( i2cResult != 0 )
      return COMMS_I2C_ERROR; //Error: Sensor did not ack

		i2cResult = _i2cPort->requestFrom(_deviceAddress, uint8_t(numBytes), uint8_t(true));
    if( i2cResult == 0 ) 
      return COMMS_I2C_ERROR;
		for(size_t i = 0; i < numBytes; i++) {
			dataBuffer[i] = _i2cPort->read();
		}
    return COMMS_SUCCESS;
	}
}
// This function is used when more than 32 bytes (TwoWire maximum buffer
// length) of data are requested.
COMMS_STATUS_t TMF_COMMS::overBufLenI2CRead(uint8_t reg, uint8_t dataBuffer[], uint16_t numBytes)
{
  uint8_t resizedRead; 
  uint8_t i2cResult; 
  size_t arrayPlaceHolder = 0;

  _i2cPort->beginTransmission(_deviceAddress);
  _i2cPort->write(reg); 
  i2cResult = _i2cPort->endTransmission(false);
  if( i2cResult != 0 )
    return COMMS_I2C_ERROR; //Error: Sensor did not ack

  while(numBytes > 0) 
  {
    if( numBytes > MAX_BUFFER_LENGTH )
      resizedRead = MAX_BUFFER_LENGTH; 
    else
      resizedRead = numBytes; 

		i2cResult = _i2cPort->requestFrom(_deviceAddress, resizedRead, uint8_t(false)); //false = repeated start
    if( i2cResult == 0 )
      return COMMS_I2C_ERROR;
		for(size_t i = 0; i < resizedRead; i++) {
			dataBuffer[arrayPlaceHolder] = _i2cPort->read();
      arrayPlaceHolder++;
    }	
    numBytes = numBytes - MAX_BUFFER_LENGTH; // end condition
  }
  return COMMS_SUCCESS;
}

// This generic function reads an eight bit register. It takes the register's
// address as its' parameter. 
uint8_t TMF_COMMS::_readRegister(uint8_t _reg)
{

  if(_i2cPort == NULL) {
    _spiPort->beginTransaction(mySpi); 
    digitalWrite(_cs, LOW); // Start communication.
    _spiPort->transfer(_reg |= SPI_READ_M);  // Register OR'ed with SPI read command. 
    _regValue = _spiPort->transfer(0); // Get data from register.  
    _spiPort->endTransaction();
    return(_regValue); 
  }
  else {
    _i2cPort->beginTransmission(_address); 
    _i2cPort->write(_reg); // Moves pointer to register.
    _i2cPort->endTransmission(false); // 'False' here sends a restart message so that bus is not released
    _i2cPort->requestFrom(_address, 1); // Read the register, only ever once. 
    _regValue = _i2cPort->read();
    return(_regValue);
  }
}

