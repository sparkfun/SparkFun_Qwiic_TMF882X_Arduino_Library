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
  return  ( begin(&wirePort) ? true : false );
}

bool QwiicTMF882X::beginSpi(uint8_t userCsPin, uint32_t spiPortSpeed, SPIClass &spiPort)
{

 return ( beginSpi(userPin, &spiPort)  ? true : false ) //spi from COMMS lib - change name to remove confusion and conlict

}



