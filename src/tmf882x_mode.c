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

/***** tmf882x_mode.c *****/
#include "tmf882x_interface.h"
#include "inc/tmf882x_mode.h"
#include "inc/tmf882x_host_interface.h"

#define TMF882X_MODE_TAG            0

#define TMF882X_STARTUP_WAIT_USEC   10000
#define TMF882X_WAIT_USEC           3000
#define TMF882X_MAX_RETRY           10
#define TMF882X_STAT_CPU_SLEEP(x)   (!((x) & 0x1))
#define TMF882X_STAT_CPU_READY(x)   (!!((x) & 0x40))
#define TMF882X_STAT_CPU_BUSY(x)    (!((x) & 0x40))


static inline void * to_priv(struct tmf882x_mode *self)
{
    return self->priv;
}

static int32_t tmf882x_wait_for_cpu_ready(struct tmf882x_mode *self)
{
    int32_t retry = 0;
    int32_t cpu_try_reset = 0;
    int32_t error;
    uint8_t status;

    if (!self) return -1;
    while (retry++ < TMF882X_MAX_RETRY) {
        error = tof_get_register(to_priv(self), TMF882X_STAT, &status);
        if (error) {
            tof_err(to_priv(self), "CPU status read failed, attempt %d: %d", retry, error);
            tmf882x_mode_standby_operation(self, TOF_WAKEUP);
            continue;
        }
        if (TMF882X_STAT_CPU_READY(status)) {
            tof_dbg(to_priv(self), "ToF chip CPU is ready");
            return 0;
        } else if (TMF882X_STAT_CPU_SLEEP(status)) {
            tof_info(to_priv(self), "ToF chip in standby state, waking up");
            tmf882x_mode_standby_operation(self, TOF_WAKEUP);
            tof_usleep(to_priv(self), TMF882X_WAIT_USEC);
            error = -1;
            continue;
        } else if (TMF882X_STAT_CPU_BUSY(status) &&
                   (retry >= TMF882X_MAX_RETRY)) {
            if (!cpu_try_reset) {
                tof_info(to_priv(self), "Forcing boot monitor reset");
                // force boot monitor reset
                (void) tmf882x_mode_cpu_reset(self, 0x1);
                retry = 0;
                cpu_try_reset = 1;
                continue;
            }
            tmf882x_dump_i2c_regs(self);
            return -1;
        }
        tof_usleep(to_priv(self), TMF882X_WAIT_USEC);
    }
    return error;
}

static int32_t tmf882x_wait_for_cpu_startup(struct tmf882x_mode *self)
{
    int32_t rc = 0;
    if (!self) return -1;
    tof_usleep(to_priv(self), TMF882X_STARTUP_WAIT_USEC);
    rc = tmf882x_wait_for_cpu_ready(self);
    // wait again for patch application to override any settings
    tof_usleep(to_priv(self), TMF882X_STARTUP_WAIT_USEC);
    return rc;
}

int32_t tmf882x_mode_set_powerup_bootmatrix(struct tmf882x_mode *self,
                                            uint32_t powerup_bitfield)
{
    uint8_t cpu_stat;
    if (!self) return -1;
    if (tof_get_register(to_priv(self), TMF882X_STAT, &cpu_stat)) {
        return -1;
    }
    // set bits 4:5 to powerup matrix bit field
    cpu_stat &= ~(0x3 << 4);
    cpu_stat |= (powerup_bitfield << 4);

    return tof_set_register(to_priv(self), TMF882X_STAT, cpu_stat);
}

int32_t tmf882x_mode_cpu_reset(struct tmf882x_mode *self,
                               uint32_t powerup_bitfield)
{
    uint8_t cpu_stat;
    if (!self) return -1;
    // ignore i2c errors and always try to do the reset
    (void) tmf882x_mode_set_powerup_bootmatrix(self, powerup_bitfield);
    (void) tof_get_register(to_priv(self), 0xEC, &cpu_stat);
    // switch off PLL
    cpu_stat &= ~0x40;
    tof_set_register(to_priv(self), 0xEC, cpu_stat);
    return tof_set_register(to_priv(self), 0xF0, 0x80);
}

int32_t tmf882x_mode_standby_operation(struct tmf882x_mode *self, tmf882x_pwr_mode_t mode)
{
    uint8_t oper = !!mode;
    uint8_t cpu_stat;
    uint32_t rc = 0;
    if (!self) return -1;
    if (tof_get_register(to_priv(self), TMF882X_STAT, &cpu_stat)) {
        return -1;
    }
    // Standby operation is bit0 of cpu_stat register
    cpu_stat &= ~0x01;
    cpu_stat |= oper;
    rc = tof_set_register(to_priv(self), TMF882X_STAT, cpu_stat);
    if (mode == TOF_STANDBY)
        tof_usleep(to_priv(self), 5000); // wait for device to go to standby
    else
        tof_usleep(to_priv(self), 2000); // wait for device to wakeup
    return rc;
}

inline void * tmf882x_mode_priv(struct tmf882x_mode *self)
{
    return self->priv;
}

inline uint8_t tmf882x_mode(struct tmf882x_mode *self)
{
    return self->info_rec.record.app_id;
}

inline uint8_t tmf882x_mode_maj_ver(struct tmf882x_mode *self)
{
    // Major version is currently = Info Record Application ID
    return tmf882x_mode(self);
}

static void tmf882x_mode_close(struct tmf882x_mode *self)
{
    if (!self) return;
}

static int32_t tmf882x_mode_open(struct tmf882x_mode *self)
{
    int32_t error;

    if (!self) return -1;

    tof_info(to_priv(self), "%s", __func__);
    error = tmf882x_wait_for_cpu_startup(self);
    if (error) {
        tof_err(to_priv(self), "Error waiting for CPU startup.");
        return error;
    }

    error = tof_i2c_read(to_priv(self), TMF882X_APP_ID,
                         self->info_rec.data,
                         sizeof(self->info_rec.record));
    if (error) {
        tof_err(to_priv(self), "read record failed: %d", error);
        return error;
    }
    tof_info(to_priv(self),
             "Read info record - Running mode: %#x.",
             tmf882x_mode(self));
    tof_info(to_priv(self), "mode version: %u.%u.%u.%u",
                    tmf882x_mode_maj_ver(self),
                    self->info_rec.record.min_ver,
                    self->info_rec.record.build_ver,
                    self->info_rec.record.patch_ver);

    return 0;
}

void tmf882x_dump_data(struct tmf882x_mode *self, const uint8_t *dbuf, size_t len)
{
    uint32_t per_line = 4;
    uint32_t idx = 0;
    uint32_t per_line_idx = 0;
    uint32_t cnt = 0;
    uint8_t buf[80] = {0};

    if (!self | (len <= 0) | !dbuf) return;

    for (idx = 0; idx < len; idx += per_line) {
        cnt += snprintf((char *)buf, sizeof(buf) - cnt,
                        "buffer dump [%02x]: ", idx);
        for (per_line_idx = 0;
             (per_line_idx < per_line) && ((idx + per_line_idx) < len);
             per_line_idx++) {
            cnt += snprintf((char *)(buf + cnt), sizeof(buf) - cnt, "%02x ",
                            dbuf[idx+per_line_idx]);
        }
        cnt = 0;
        tof_info(to_priv(self), "%s", buf);
    }
}

void tmf882x_dump_i2c_regs(struct tmf882x_mode * self)
{
    uint8_t buf[MAX_REGS] = {0};
    if (!self) return;
    tof_info(to_priv(self), "%s", __func__);
    (void) tof_i2c_read(to_priv(self), 0, buf, sizeof(buf));
    tmf882x_dump_data(self, buf, sizeof(buf));
}

inline void tmf882x_mode_set_debug(struct tmf882x_mode *self, int32_t flag)
{
    self->debug = !!flag;
}

int32_t tmf882x_mode_version(struct tmf882x_mode *self, char *ver, size_t len)
{
    if (!self || !ver) return -1;
    return snprintf(ver, len, "%u.%u.%u.%u",
                    tmf882x_mode_maj_ver(self),
                    self->info_rec.record.min_ver,
                    self->info_rec.record.build_ver,
                    self->info_rec.record.patch_ver);
}

static const struct mode_vtable base_ops = {
    .tag = TMF882X_MODE_TAG,
    .open = tmf882x_mode_open,
    .close = tmf882x_mode_close,
};

void tmf882x_mode_init(struct tmf882x_mode *self,
                       struct mode_vtable const * ops,
                       void * priv)
{
    if (!ops) {
        self->ops = &base_ops;
    } else {
        self->ops = ops;
    }
    self->priv = priv;
}

