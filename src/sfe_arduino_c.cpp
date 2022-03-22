

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <Arduino.h>
#include <Wire.h>
#include "inc/sfe_arduino_c.h"

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

unsigned long sfe_millis(void){

	return millis();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_usleep()
//
// Wrapper around Arduino function delay() - keeps Arduino space isolated from AMS code. Used
// in  platform_shim.h - tof_sleep()

void sfe_usleep(uint32_t usec){

	// We're passed in microsecs, but we'll use Arduino::delay, which uses milli-secs, so this 
	// will lose some fine resolution...which is fine
	delay(usec/1000);
}

