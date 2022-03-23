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

int32_t sfe_queue_msg(struct platform_ctx *ctx, struct tmf882x_msg *msg);

// I2C communication API

int sfe_i2c_init(uint8_t addresss, void *wireI2CPort);
int sfe_write_i2c_block(uint8_t addr, uint8_t reg, const uint8_t* data, uint32_t numBytes);
int sfe_read_i2c_block(uint8_t addr, uint8_t reg, uint8_t* data, uint32_t numBytes);

#ifdef __cplusplus
}
#endif
#endif