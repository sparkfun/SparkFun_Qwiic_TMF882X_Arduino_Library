////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sfe_arduino_c.h
//
// This header file implements the C interface functions that the underlying TMF SDK/Library uses for platform
// specific functionalty. The platform is Arduino in this scenario.
//
// The implementation of the functions call Arduino C++ code. The functions are called from C, so 
// all function signatures are C, and annotated as such ("extern C") in this header file.
//

#ifndef _SFE_ARDUINO_C_H_
#define _SFE_ARDUINO_C_H_


#define kDefaultTMFAddress 0x41

#ifdef __cplusplus
extern "C" {
#endif

// Utility routines needed for the underling sdk
unsigned long sfe_millis(void);
void sfe_usleep(uint32_t usec);
void sfe_msleep(uint32_t msec);

#ifdef __cplusplus
}
#endif
#endif