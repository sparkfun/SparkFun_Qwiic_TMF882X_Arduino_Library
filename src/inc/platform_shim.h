/*
 *****************************************************************************
 * Copyright by ams AG                                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED FOR USE ONLY IN CONJUNCTION WITH AMS PRODUCTS.  *
 * USE OF THE SOFTWARE IN CONJUNCTION WITH NON-AMS-PRODUCTS IS EXPLICITLY    *
 * EXCLUDED.                                                                 *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */

/***** platform_shim.h *****/

#ifndef __PLATFORM_SHIM_H
#define __PLATFORM_SHIM_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "platform_wrapper.h"
#include "sfe_arduino_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define tof_err(p, fmt, ...) \
({ \
    struct platform_ctx *__ctx = (struct platform_ctx*)p; \
    fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
})

#define tof_info(p, fmt, ...) \
({ \
    struct platform_ctx *__ctx = (struct platform_ctx*)p; \
    if (__ctx->debug) \
        printf(fmt "\n", ##__VA_ARGS__); \
})

#define tof_dbg(p, fmt, ...) \
({ \
    struct platform_ctx *__ctx = (struct platform_ctx*)p; \
    if (__ctx->debug > 1) \
        printf(fmt "\n", ##__VA_ARGS__); \
})

static inline int32_t tof_i2c_read(struct platform_ctx *ctx, uint8_t reg,
                                   uint8_t *buf, int32_t len)
{
    return sfe_read_i2c_block(ctx, reg, buf, len);
   // return platform_wrapper_read_i2c_block(ctx, reg, buf, len);
}

static inline int32_t tof_i2c_write(struct platform_ctx *ctx, uint8_t reg,
                                    const uint8_t *buf, int32_t len)
{
     return sfe_write_i2c_block(ctx, reg, buf, len);
    //return platform_wrapper_write_i2c_block(ctx, reg, buf, len);
}

static inline int32_t tof_set_register(struct platform_ctx *ctx, uint8_t reg,
                                       uint8_t val)
{
    return tof_i2c_write(ctx, reg, &val, 1);
}

static inline int32_t tof_get_register(struct platform_ctx *ctx, uint8_t reg,
                                       uint8_t *val)
{
    return tof_i2c_read(ctx, reg, val, 1);
}

static inline int32_t tof_queue_msg(struct platform_ctx *ctx, struct tmf882x_msg *msg)
{
    return sfe_queue_msg(ctx, msg);
}

static inline void tof_usleep(struct platform_ctx *ctx, uint32_t usec)
{
    sfe_usleep(usec);
    
}

static inline void tof_get_timespec(struct timespec *ts)
{
    ts->tv_sec = sfe_millis();
    ts->tv_nsec = 0;

}

#ifdef __cplusplus
}
#endif

#endif
