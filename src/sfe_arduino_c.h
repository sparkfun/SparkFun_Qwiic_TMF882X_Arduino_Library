////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_arduino_c.h
//
// This header file implements the C interface functions that the underlying TMF SDK/Library uses for platform
// specific functionalty. The platform is Arduino in this scenario.
//
// The implementation of the functions call Arduino C++ code. The functions are called from C, so
// all function signatures are C, and annotated as such ("extern C") in this header file.
//

#pragma once


#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Utility routines needed for the underling sdk
unsigned long sfe_millis(void);
void sfe_usleep(uint32_t usec);
void sfe_msleep(uint32_t msec);
void sfe_output(const char* fmt, va_list args);
void sfe_set_output_device(void*);

#ifdef __cplusplus
}
#endif

