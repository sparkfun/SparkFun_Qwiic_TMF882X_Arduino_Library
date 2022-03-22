
//////////////////////////////////////////////////////////////////////////////
//
//  This is a library written for the SparkFun AMS TMF882X ToF breakout/qwiic board
//
//  SparkFun sells these at its website: www.sparkfun.com
//  Do you like this library? Help support SparkFun. Buy a board!
//
//  https://www.sparkfun.com/products/<TODO>
//
//  Written by <TODO>
//
//
//  https://github.com/sparkfun/<TODO>
//
//  License
//     <TODO>
//
//////////////////////////////////////////////////////////////////////////////

#pragma once 

// This is really just a think wrapper class that provides an Arduino interface to
// the underlying C++/C implementation

// Include our implementation class
#include "qwiic_tmf882x.h"

// Arduino things
#include <Arduino.h>
#include <Wire.h>



class SparkFun_TMF882X : public QwDevTMF882X
{

public:

    // Default noop constructor
    SparkFun_TMF882X(){};

    bool begin(TwoWire &wirePort = Wire, uint8_t deviceAddress = kDefaultTMF882XAddress){

        _i2cBus.init(wirePort);

        set_comm_bus(_i2cBus, deviceAddress);

        return this->QwDevTMF882X::init();
    };

private:

    QwI2C _i2cBus;
};
