// sfe_shim.cpp
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
////////////////////////////////////////////////////////////////////////////////////////
// 
// The AMS SDK architecture defines a "shim" pattern that is used to provide 
// the SDK functionality in a platform independent manner. This file
// implements the needed interface functions for Arduino.
//
// Note - This is a C++ file, but the functions implemented are C interfaces.
//        This is needed because the AMS SDK is in C, but Arduino is C++

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "inc/sfe_shim.h"
#include "qwiic_tmf882x.h"
#include "sfe_arduino.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_info()
//
// Output an info message. Note, this routine takes a users defined (void *), a C format 
// string and a list of args to support the output.
//
// The void * pointer is our libraries QwDevTMF882X object

void tof_info(void* pTarget, const char* fmt, ...)
{
    // Is our library outputting info messages?
    if (!fmt || !(((QwDevTMF882X*)pTarget)->getMessageLevel() & TMF882X_MSG_INFO))
        return;

    // Grab our args, send to our generic output routine
    va_list ap;
    va_start(ap, fmt);
    sfe_output(fmt, ap);
    va_end(ap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_dbg()
//
// Output an debug message. Note, this routine takes a users defined (void *), a C format 
// string and a list of args to support the output.
//
// The void * pointer is our libraries QwDevTMF882X object

void tof_dbg(void* pTarget, const char* fmt, ...)
{
    // Is our library outputting debug messages?    
    if (!fmt || !(((QwDevTMF882X*)pTarget)->getMessageLevel() & TMF882X_MSG_DEBUG))
        return;

    // Grab our args, send to our generic output routine
    va_list ap;
    va_start(ap, fmt);
    sfe_output(fmt, ap);
    va_end(ap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_err()
//
// Output an error message. Note, this routine takes a users defined (void *), a C format 
// string and a list of args to support the output.
//
// The void * pointer is our libraries QwDevTMF882X object

void tof_err(void* pTarget, const char* fmt, ...)
{
    // Is our library outputting error messages?        
    if (!fmt || !(((QwDevTMF882X*)pTarget)->getMessageLevel() & TMF882X_MSG_ERROR))
        return;

    // Grab our args, send to our generic output routine
    va_list ap;
    va_start(ap, fmt);
    sfe_output(fmt, ap);
    va_end(ap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_i2c_read()
//
// Used to read a block of data from the specified register from I2C device

int32_t tof_i2c_read(void* pTarget, uint8_t reg, uint8_t* buf, int32_t len)
{
    // Just relay up to our library object

    return ((QwDevTMF882X*)pTarget)->readRegisterRegion(reg, buf, len);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_i2c_write()
//
// Used to write a block of data to the specified register on the I2C device

int32_t tof_i2c_write(void* pTarget, uint8_t reg, const uint8_t* buf, int32_t len)
{
    // Just relay up to our library object

    return ((QwDevTMF882X*)pTarget)->writeRegisterRegion(reg, (uint8_t*)buf, len);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_set_register()
//
// Send a byte value to the specified register

int32_t tof_set_register(void* pTarget, uint8_t reg, uint8_t val)
{
    // use our generic routine in this interface

    return tof_i2c_write(pTarget, reg, &val, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_get_register()
//
// Get a byte from the specified register

int32_t tof_get_register(void* pTarget, uint8_t reg, uint8_t* val)
{
    // use our generic routine in this interface    
    return tof_i2c_read(pTarget, reg, val, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_queue_msg()
//
// The SDK sends results and error via messages, calling this function to relay the message
// struct to the SDK user. 
//
// Send this message to our library object (which is the void * passed in)

int32_t tof_queue_msg(void* pTarget, struct tmf882x_msg* msg)
{
    // relay up to our main library object

    return ((QwDevTMF882X*)pTarget)->sdkMessageHandler(msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_usleep()
//
// Called to sleep for a number of micro secs

void tof_usleep(void* pTarget, uint32_t usec)
{
    sfe_usleep(usec);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// tof_get_timespec()
//
// Used to get a number of elapsed milli secs in a timespec struct.

void tof_get_timespec(struct timespec* ts)
{
    ts->tv_sec = sfe_millis();
    ts->tv_nsec = 0;
}
