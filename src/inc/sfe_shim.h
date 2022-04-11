// sfe_shim.h
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

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "tmf882x.h"

// The AVR system/tool environment for Arduino doesn't include a timespec declaration, but
// the SDK needs one. It's a simple struct, so declare it here.
//
// This gets it to compile, but if you're looking to run this on an uno, this
// library, with the SDK and embedded firmware it contains won't fit

#if defined(__AVR__)
struct timespec {
    time_t tv_sec; /* seconds */
    long tv_nsec; /* and nanoseconds */
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

// The specifics for each function are detailed in the AMS SDK.
//
// See sfe_shim.cpp for the SparkFun implementation

void tof_dbg(void* pTarget, const char* fmt, ...);

void tof_info(void* pTarget, const char* fmt, ...);

void tof_err(void* pTarget, const char* fmt, ...);

int32_t tof_i2c_read(void* pTarget, uint8_t reg, uint8_t* buf, int32_t len);

int32_t tof_i2c_write(void* pTarget, uint8_t reg, const uint8_t* buf, int32_t len);

int32_t tof_set_register(void* pTarget, uint8_t reg, uint8_t val);

int32_t tof_get_register(void* pTarget, uint8_t reg, uint8_t* val);

int32_t tof_queue_msg(void* pTarget, struct tmf882x_msg* msg);

void tof_usleep(void* pTarget, uint32_t usec);

void tof_get_timespec(struct timespec* ts);

#ifdef __cplusplus
}
#endif
