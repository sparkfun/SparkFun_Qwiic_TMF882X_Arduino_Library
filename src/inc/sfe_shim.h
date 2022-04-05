

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
