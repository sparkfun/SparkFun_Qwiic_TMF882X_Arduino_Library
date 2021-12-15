/*
  This is a library for...
  By: Elias Santistevan
  Date: 
  License: This code is public domain but you buy me a beer if you use this and 
  we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!
 */

#include "QwiicTMF882X.h"

QwiicTMF882X::QwiicTMF882X(){} //Constructor for SPI
QwiicTMF882X::QwiicTMF882X(uint8_t address){  _address = address; } //Constructor for I2C

bool QwiicTMF882X::begin( TwoWire &wirePort )
{
  return  ( commsBegin(&wirePort) ? true : false );
}

bool QwiicTMF882X::beginSpi(uint8_t userPin, uint32_t spiPortSpeed, SPIClass &spiPort)
{

 return ( commsBeginSpi(userPin, &spiPort)  ? true : false ) //spi from COMMS lib - change name to remove confusion and conlict

}

uint16_t QwiicTMF882X::getAppID(){

  uint16_t tempReg[2]; 
  uint16_t id; 

  COMMS_STATUS_t result = readMultiRegister(TMF882X_APPID_MAJOR, tempReg)
  if( result != COMMS_SUCCESS ) 
    return static_cast<uint16_t>(TMF882X_GEN_ERROR); 

  id = tempReg[0] << 8; 
  id |= tempReg[1]; 

  return id; 
  
}

//uint8_t QwiicTMF882X::someFunc(){
  
//}
