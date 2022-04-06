
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

// This is really just a thin wrapper class that provides an Arduino interface to
// the underlying C++/C implementation

// Include our implementation class
#include "qwiic_tmf882x.h"
#include "sfe_arduino_c.h"

// Arduino things
#include <Wire.h>

class SparkFun_TMF882X : public QwDevTMF882X {

public:
    // Default noop constructor
    SparkFun_TMF882X() {};

    bool begin(TwoWire& wirePort = Wire, uint8_t deviceAddress = kDefaultTMF882XAddress)
    {
        // Setup  I2C object and pass into the superclass
        m_i2cBus.init(wirePort);
        setCommBus(m_i2cBus, deviceAddress);

        // Initialize the system - return results
        return this->QwDevTMF882X::init();
    };

    void setOutputDevice(Stream& theStream)
    {
        sfe_set_output_device((void*)&theStream);
    }

private:
    QwI2C m_i2cBus;
};
