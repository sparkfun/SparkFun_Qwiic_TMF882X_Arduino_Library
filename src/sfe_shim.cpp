

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>


#include "inc/sfe_arduino_c.h"
#include "inc/sfe_shim.h"
#include "qwiic_tmf882x.h"


int32_t tof_i2c_read(void * pTarget, uint8_t reg, uint8_t *buf, int32_t len){

    return ((QwDevTMF882X*)pTarget)->readRegisterRegion(reg, buf, len);
}

int32_t tof_i2c_write(void *pTarget, uint8_t reg, const uint8_t *buf, int32_t len){

    return ((QwDevTMF882X*)pTarget)->writeRegisterRegion(reg, (uint8_t*)buf, len);    
}

int32_t tof_set_register(void *pTarget, uint8_t reg, uint8_t val){

    return tof_i2c_write(pTarget, reg, &val, 1);
}

int32_t tof_get_register(void * pTarget, uint8_t reg, uint8_t *val){

    return tof_i2c_read(pTarget, reg, val, 1);

}

int32_t tof_queue_msg(void *pTarget, struct tmf882x_msg *msg)
{
    return ((QwDevTMF882X*)pTarget)->_sdk_msg_handler(msg);
}

void tof_usleep(void *pTarget, uint32_t usec)
{
    sfe_usleep(usec);
    
}

void tof_get_timespec(struct timespec *ts)
{
    ts->tv_sec = sfe_millis();
    ts->tv_nsec = 0;

}
