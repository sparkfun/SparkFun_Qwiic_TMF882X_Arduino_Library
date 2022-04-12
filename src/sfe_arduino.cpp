// sfe_arduino_c.cpp
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
//
//////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_arduino.cpp
//
// This file implements the C interface functions that the underlying TMF SDK/Library uses for platform
// specific functionalty. The platform is Arduino in this scenario.
//
// The implementation of the functions call Arduino C++ code. The functions are called from C, so
// all function signatures are C, and annotated as such ("extern C") in the header file.
//

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "qwiic_tmf882x.h"
#include "sfe_arduino.h"
#include <Arduino.h>
#include <Wire.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stash for our output device -- for messages...etc.
//
// Use a baseclass of Serial - Stream - do store the device.
static Stream* s_outputDevice = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_millis()
//
// Wrapper around Arduino function millis() - keeps Arduino space isolated from AMS code. Used
// in  platform_shim.h - tof_get_timespec()

unsigned long sfe_millis(void)
{
    return millis();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_msleep()
//
// Sleep for a number of milli-seconds
//
// Wrapper around Arduino function delay() - keeps Arduino space isolated from AMS code. Used
// in  sfe_shim.h - tof_sleep()

void sfe_msleep(uint32_t msec)
{
    delay(msec);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_msleep()
//
// Sleep for a number of micro-seconds
//
// Wrapper around Arduino function delay() - keeps Arduino space isolated from AMS code. Used
// in  sfe_shim.h - tof_sleep()

void sfe_usleep(uint32_t usec)
{
    // We're passed in microsecs, but we'll use Arduino::delay, which uses milli-secs, so this
    // will lose some fine resolution...which is fine
    int tick = usec / 1000;

    sfe_msleep(tick < 3 ? 3 : tick);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_set_output_device()
//
// Function to set the output device used in this file for dumping out text messages
// 
void sfe_set_output_device(void* theDevice)
{
    if (!theDevice)
        return;

    s_outputDevice = (Stream*)theDevice;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_output()
//
// Outputs a string to the provided output device. Expects a format string and arg list

#define kOutputBufferSize 100

void sfe_output(const char* fmt, va_list args)
{
    if (!fmt || !s_outputDevice)
        return;

    char szBuffer[kOutputBufferSize];
    vsnprintf(szBuffer, kOutputBufferSize, fmt, args);

    s_outputDevice->println(szBuffer);
}
