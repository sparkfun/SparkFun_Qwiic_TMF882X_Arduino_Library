/*
  This is a library for...
  By: Elias Santistevan
  Date: 
  License: This code is public domain but you buy me a beer if you use this and 
  we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!
 */

#include "QwiicTMF882X.h"

QwiicTMF882X::QwiicTMF882X(){} 

bool QwiicTMF882X::begin(uint8_t address,  TwoWire &wirePort )
{
  return  ( commsBegin(address, wirePort) ? true : false );
}

bool QwiicTMF882X::beginSpi(uint8_t userPin, uint32_t spiPortSpeed, SPIClass &spiPort)
{

 return ( commsBeginSpi(userPin, spiPortSpeed, spiPort)  ? true : false ); //spi from COMMS lib - change name to remove confusion and conlict

}

uint16_t QwiicTMF882X::getAppID(){

  uint8_t tempReg[2]; 
  uint16_t id; 

  COMMS_STATUS_t result = readMultiRegisters(TMF882X_APPID_MAJOR, tempReg, 2);
  if( result != COMMS_SUCCESS ) 
    return static_cast<uint16_t>(TMF882X_GEN_ERROR); 

  id = tempReg[0] << 8; 
  id |= tempReg[1]; 

  return id; 
  
}

//uint8_t QwiicTMF882X::someFunc(){
  
//}
