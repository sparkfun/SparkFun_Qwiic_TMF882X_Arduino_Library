

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "tmf882x.h"

#ifdef __cplusplus
extern "C" {
#endif


#define tof_err(p, fmt, ...) \
({ \
    fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
})

#define tof_info(p, fmt, ...) \
({ \
    printf(fmt "\n", ##__VA_ARGS__); \
})

#define tof_dbg(p, fmt, ...) \
({ \
    printf(fmt "\n", ##__VA_ARGS__); \
})

int32_t tof_i2c_read(void *pTarget, uint8_t reg, uint8_t *buf, int32_t len);


int32_t tof_i2c_write(void *pTarget, uint8_t reg, const uint8_t *buf, int32_t len);


int32_t tof_set_register(void *pTarget, uint8_t reg, uint8_t val);

int32_t tof_get_register(void *pTarget, uint8_t reg, uint8_t *val);

int32_t tof_queue_msg(void *pTarget, struct tmf882x_msg *msg);

void tof_usleep(void *pTarget, uint32_t usec);

void tof_get_timespec(struct timespec *ts);

#ifdef __cplusplus
}
#endif

