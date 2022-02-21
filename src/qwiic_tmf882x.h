
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

// The AMS supplied library/sdk interface
#include "inc/tmf882x.h"

// Arduino things - this needs to change once moved to framework
#include <Wire.h>

// Default I2C address for the device
#define kDefaultTMF882XAddress 0x41

// define a type for the results -- just alias the underlying measurment struct - easier to type
typedef struct tmf882x_msg_meas_results TMF882XMeasurement_t;

class Qwiic_TMF882X
{

public:

    // Default noop constructor
    Qwiic_TMF882X(){};

    bool begin(TwoWire &wirePort = Wire, uint8_t deviceAddress = kDefaultTMF882XAddress);
    bool isConnected(); //Checks if sensor ack's the I2C request


    bool getMeasurement(TMF882XMeasurement_t * results);
    void printMeasurement(TMF882XMeasurement_t * results);

private:
    TwoWire *_i2cPort;
    uint8_t  _deviceAddress;

};
