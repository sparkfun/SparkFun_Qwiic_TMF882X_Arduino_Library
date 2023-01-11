// qwiic_i2c.h
//
// This is a library written for SparkFun Qwiic TMF882X boards
//
// SparkFun sells these bpards at its website: www.sparkfun.com
//
// Do you like this library? Help support SparkFun. Buy a board!
//
//  SparkFun Qwiic dToF Imager - TMF8820        https://www.sparkfun.com/products/19036
//  SparkFun Qwiic Mini dToF Imager - TMF8820   https://www.sparkfun.com/products/19218
//  SparkFun Qwiic Mini dToF Imager - TMF8821   https://www.sparkfun.com/products/19451
//  SparkFun Qwiic dToF Imager - TMF8821        https://www.sparkfun.com/products/19037
//
// Written by Kirk Benell @ SparkFun Electronics, April 2022
//
// This library provides an abstract interface to the underlying TMF882X
// SDK that is provided by AMS.
//
// Repository:
//     https://github.com/sparkfun/SparkFun_Qwiic_TMF882X_Arduino_Library
//
//
// SparkFun code, firmware, and software is released under the MIT
// License(http://opensource.org/licenses/MIT).
//
// SPDX-License-Identifier: MIT
//
//    The MIT License (MIT)
//
//    Copyright (c) 2022 SparkFun Electronics
//    Permission is hereby granted, free of charge, to any person obtaining a
//    copy of this software and associated documentation files (the "Software"),
//    to deal in the Software without restriction, including without limitation
//    the rights to use, copy, modify, merge, publish, distribute, sublicense,
//    and/or sell copies of the Software, and to permit persons to whom the
//    Software is furnished to do so, subject to the following conditions: The
//    above copyright notice and this permission notice shall be included in all
//    copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
//    "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//    NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//    PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Header for I2C driver object

#pragma once

// Simple object to encapsulate basic I2C operations.
//
// This is following a pattern for future implementations
//
// This class is focused on Arduino..

#include "Arduino.h"
#include <Wire.h>

namespace sfe_TMF882X {


class QwI2C {

public:
    QwI2C(void);

    bool init(void);
    bool init(TwoWire& wirePort, bool bInit=false);

    // see if a device exists
    bool ping(uint8_t address);

    bool writeRegisterByte(uint8_t address, uint8_t offset, uint8_t data);

    // Write a block of bytes to the device --
    int writeRegisterRegion(uint8_t address, uint8_t offset, uint8_t* data, uint16_t length);

    int readRegisterRegion(uint8_t addr, uint8_t reg, uint8_t* data, uint16_t numBytes);

private:
    TwoWire* _i2cPort;
};

};