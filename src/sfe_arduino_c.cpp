

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "qwiic_tmf882x.h"
#include "sfe_arduino_c.h"
#include <Arduino.h>
#include <Wire.h>

// Stash for our output device -- for messages...etc.
//
// Use a baseclass of Serial - Stream - do store the device.
static Stream* s_outputDevice = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_arduino_c.cpp
//
// This file implements the C interface functions that the underlying TMF SDK/Library uses for platform
// specific functionalty. The platform is Arduino in this scenario.
//
// The implementation of the functions call Arduino C++ code. The functions are called from C, so
// all function signatures are C, and annotated as such ("extern C") in the header file.
//
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
// sfe_usleep()
//
// Wrapper around Arduino function delay() - keeps Arduino space isolated from AMS code. Used
// in  platform_shim.h - tof_sleep()

void sfe_msleep(uint32_t msec)
{
    delay(msec);
}

void sfe_usleep(uint32_t usec)
{
    // We're passed in microsecs, but we'll use Arduino::delay, which uses milli-secs, so this
    // will lose some fine resolution...which is fine
    int tick = usec / 1000;

    sfe_msleep(tick < 3 ? 3 : tick);
}

void sfe_set_output_device(void* theDevice)
{
    if (!theDevice)
        return;

    s_outputDevice = (Stream*)theDevice;
}
#define kOutputBufferSize 100

void sfe_output(const char* fmt, va_list args)
{
    if (!fmt || !s_outputDevice)
        return;

    char szBuffer[kOutputBufferSize];
    vsnprintf(szBuffer, kOutputBufferSize, fmt, args);

    s_outputDevice->println(szBuffer);
}
