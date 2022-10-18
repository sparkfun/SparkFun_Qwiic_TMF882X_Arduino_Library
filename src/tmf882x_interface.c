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

/***** tmf882x_interface.c *****/

#include "tmf882x_interface.h"
#include "inc/tmf882x_host_interface.h"
#include "inc/tmf882x_mode.h"
#include "inc/tmf882x_mode_app.h"
#include "inc/tmf882x_mode_bl.h"

static inline void * to_priv(struct tmf882x_mode *self)
{
    return tmf882x_mode_priv(self);
}

static void initialize_state_from_mode(struct tmf882x_tof *tof, tmf882x_mode_t mode)
{
    switch (mode) {
        case TMF882X_MODE_APP:
            tmf882x_mode_app_init(&tof->app, to_priv(&tof->state));
            break;
        case TMF882X_MODE_BOOTLOADER:
            tmf882x_mode_bl_init(&tof->bl, to_priv(&tof->state));
            break;
    }
}

static void mode_reinit(struct tmf882x_tof *tof)
{
    void * priv;
    int32_t debug;
    if (!tof) return;

    debug = tof->state.debug;   // keep current debug setting

    priv = tmf882x_mode_priv(&tof->state);

    tmf882x_init(tof, priv);
    tmf882x_set_debug(tof, (bool)debug);
    // close base mode
    if ( tof && tof->state.ops->close )
        tof->state.ops->close(&tof->state);
}

void tmf882x_init(struct tmf882x_tof *tof, void *priv)
{
    if(tof) {
        memset(tof, 0, sizeof(struct tmf882x_tof));
        tmf882x_mode_init(&tof->state, NULL, priv);
    }
}

int32_t tmf882x_open(struct tmf882x_tof * tof)
{
    tmf882x_mode_t mode;
    if (!tof) return -1;

    if (!tof->state.ops->tag) {
        // if closed, open base mode first
        if (tof->state.ops->open &&
                tof->state.ops->open(&tof->state)) {
            tof_err(to_priv(&tof->state), "Error opening device");
            tmf882x_init(tof, to_priv(&tof->state));
            return -1;
        }

        mode = tmf882x_get_mode(tof);

        // Try to initialize mode context with requested mode and open()
        initialize_state_from_mode(tof, mode);
    }

    if (tof->state.ops->open &&
        tof->state.ops->open(&tof->state)) {
        tof_err(to_priv(&tof->state), "Error opening mode");
        tmf882x_init(tof, to_priv(&tof->state));
        return -1;
    }

    return 0;
}

void tmf882x_close(struct tmf882x_tof * tof)
{
    // close mode
    if ( tof && tof->state.ops->close )
        tof->state.ops->close(&tof->state);

    mode_reinit(tof);

    // try to go to standby after ToF device is closed
    (void) tmf882x_mode_standby_operation(&tof->state, TOF_STANDBY);
}

void tmf882x_set_debug(struct tmf882x_tof *tof, bool flag)
{
    tmf882x_mode_set_debug(&tof->state, flag);
}

int32_t tmf882x_fwdl(struct tmf882x_tof *tof, tmf882x_fwdl_type_t fwdl_type,
                 const uint8_t *buf, size_t len)
{
    if (tof) {
        if (!tof->state.ops->fwdl ||
            tof->state.ops->fwdl(&tof->state, fwdl_type, buf, len)) {
            return -1;
        } else {
            // current mode is close()'d because we are starting a new mode
            mode_reinit(tof);
            return tmf882x_open(tof);
        }
    }
    return -1;
}

int32_t tmf882x_mode_switch(struct tmf882x_tof *tof, tmf882x_mode_t mode)
{
    if (tof) {
        // if running requested mode, exit
        if  (tmf882x_get_mode(tof) == mode)
            return 0;

        if (!tof->state.ops->mode_switch ||
            tof->state.ops->mode_switch(&tof->state, mode)) {
            tof_err(to_priv(&tof->state), "Error, mode switch failed");
            return -1;
        } else {
            // current mode is close()'d because we are starting a new mode
            mode_reinit(tof);
            if(tmf882x_open(tof)) {
                tof_err(to_priv(&tof->state), "Error, reopening after mode "
                        "switch");
                return -1;
            }
            return 0;
        }
    }
    return -1;
}

inline int32_t tmf882x_process_irq(struct tmf882x_tof * tof)
{
    if (tof && tof->state.ops->process_irq) {
        return tof->state.ops->process_irq(&tof->state);
    }

    return -1;
}

inline int32_t tmf882x_start(struct tmf882x_tof * tof)
{
    if (tof && tof->state.ops->start) {
        return tof->state.ops->start(&tof->state);
    }

    return -1;
}

inline int32_t tmf882x_stop(struct tmf882x_tof * tof)
{
    if (tof && tof->state.ops->stop) {
        return tof->state.ops->stop(&tof->state);
    }

    return -1;
}

int32_t tmf882x_ioctl (struct tmf882x_tof *tof, uint32_t cmd,
                       const void *input, void *output)
{
    int32_t rc = -1;

    if (tof && tof->state.ops->ioctl) {
        if (tof->state.debug)
            tof_info(to_priv(&tof->state),
                     "%s: dir: 0x%x mode: 0x%x isize: 0x%x osize: 0x%x cmd: 0x%x",
                     __func__, _IOCTL_DIR(cmd), _IOCTL_MODE(cmd), _IOCTL_ISIZE(cmd),
                     _IOCTL_OSIZE(cmd), _IOCTL_NR(cmd));
        rc = tof->state.ops->ioctl(&tof->state, cmd, input, output);
    }

    return rc;
}

inline tmf882x_mode_t tmf882x_get_mode(struct tmf882x_tof *tof)
{
    return tmf882x_mode(&tof->state);
}

int32_t tmf882x_get_firmware_ver(struct tmf882x_tof *tof, char *ver, size_t len)
{
    return tmf882x_mode_version(&tof->state, ver, len);
}

int32_t tmf882x_get_device_revision(struct tmf882x_tof *tof, char *rev_buf, size_t len)
{
    uint8_t buf[2] = {0};
    int32_t error = 0;

    if (!tof || !rev_buf) return -1;

    error = tof_i2c_read(to_priv(&tof->state), TMF882X_ID, buf, sizeof(buf));
    if (error) {
        tof_err(to_priv(&tof->state), "Error reading device revision");
        return -1;
    }

    return snprintf(rev_buf, len, "%u.%u", buf[0], buf[1]);
}
