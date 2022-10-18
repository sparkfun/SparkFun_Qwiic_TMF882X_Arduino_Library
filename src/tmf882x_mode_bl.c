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

/***** tmf882x_mode_bl.c *****/
#include "inc/tmf882x.h"
#include "inc/tmf882x_mode.h"
#include "inc/tmf882x_mode_bl.h"
#include "inc/intel_hex_interpreter.h"
#include "inc/tmf882x_host_interface.h"
#include "tmf882x_interface.h"

#define TMF882X_BL_MODE_TAG        0x80

#define TMF882X_BL_ENCRYPT_FLAG    1
#define BL_CMD_WAIT_MSEC           1
#define BL_VALID_CHKSUM            0xFF
#define BL_DEFAULT_SALT            0x29
#define BL_DEFAULT_BIN_START_ADDR  0x20000000
#define verify_mode(mode) \
({ \
    struct tmf882x_mode *__mode = mode; \
    (TMF882X_BL_MODE_TAG == __mode->ops->tag); \
})


static inline struct tmf882x_mode *to_parent(struct tmf882x_mode_bl *bl)
{
    return &bl->mode;
}

static inline void * priv(struct tmf882x_mode_bl *bl)
{
    return tmf882x_mode_priv(to_parent(bl));
}

static int32_t tmf882x_mode_bl_open(struct tmf882x_mode *self)
{
    if (!self) return -1;
    if (verify_mode(self)) {
        tof_info(tmf882x_mode_priv(self), "%s", __func__);
        return 0;
    }
    return -1;
}

static void tmf882x_mode_bl_close(struct tmf882x_mode *self)
{
    if (!self) return;
}

static int32_t is_bl_cmd_busy(struct tmf882x_mode_bl *bl)
{
    uint8_t status = BL_STAT_CMD_BUSY;
    if (!verify_mode(&bl->mode)) return -1;
    tof_get_register(priv(bl), BL_REG_CMD_STATUS, &status);
    return BL_IS_CMD_BUSY(status);
}

static uint8_t * get_bl_cmd_buf(struct tmf882x_mode_bl *bl)
{
    if (!verify_mode(&bl->mode)) return NULL;
    return bl->bl_command.anon_cmd.data;
}

static uint8_t * get_bl_rsp_buf(struct tmf882x_mode_bl *bl)
{
    if (!verify_mode(&bl->mode)) return NULL;
    return bl->bl_response.anon_resp.data;
}

static uint8_t tmf882x_calc_chksum(const uint8_t *data, int32_t size)
{
    uint32_t sum = 0;
    int32_t idx = 0;
    for (; idx < size; idx++) {
        sum += data[idx];
    }
    return (uint8_t) ~sum; /* 1's complement of lowest byte */
}

int32_t tmf882x_mode_bl_read_status(struct tmf882x_mode_bl *bl,
                                int32_t num_retries)
{
    int32_t error = 0;
    uint8_t *rbuf = get_bl_rsp_buf(bl);
    uint8_t *status = &bl->bl_response.short_resp.status;
    uint8_t *rdata_size = &bl->bl_response.short_resp.size;
    uint8_t chksum;
    if (!verify_mode(&bl->mode)) return -1;
    if (num_retries < 0)
        num_retries = 5;
    do {
        num_retries -= 1;
        error = tof_i2c_read(priv(bl), BL_REG_CMD_STATUS,
                rbuf, BL_MSG_HEADER_SIZE);
        if (error)
            continue;
        if (BL_IS_CMD_BUSY(*status)) {
            /* CMD is still executing, wait and retry */
            tof_usleep(priv(bl), BL_CMD_WAIT_MSEC*1000);
            if (num_retries <= 0) {
                tof_info(priv(bl), "bl mode is busy: %#04x", *status);
                error = -1;
            }
            continue;
        }
        /* if we have reached here, the command has either succeeded or failed */
        if ( *rdata_size > 0 ) {
            /* read in data part and csum */
            error = tof_i2c_read(priv(bl), BL_REG_CMD_STATUS,
                    rbuf, BL_CALC_RSP_SIZE(*rdata_size));
            if (error)
                continue;
            chksum = (uint8_t) ~tmf882x_calc_chksum(rbuf, BL_CALC_RSP_SIZE(*rdata_size));
            if ((chksum != BL_VALID_CHKSUM) && (num_retries <= 0)) {
                tof_err(priv(bl),
                        "Checksum verification of Response failed: %#04x", chksum);
                return -1;
            }
            /* all done, break and return */
            return 0;
        }
    } while ((error == 0) && (num_retries > 0));
    tof_dbg(priv(bl), "bl mode wait for response: \'%d\'", error);
    return error;
}

int32_t tmf882x_mode_bl_send_rcv_cmd(struct tmf882x_mode_bl *bl)
{
    int32_t error = -1;
    uint8_t *wbuf = get_bl_cmd_buf(bl);
    uint8_t wsize = BL_CALC_CMD_SIZE(wbuf[1]);
    if (!verify_mode(&bl->mode)) return -1;
    if (is_bl_cmd_busy(bl))
        return error;

    error = tof_i2c_write(priv(bl), BL_REG_CMD_STATUS, wbuf, wsize);
    if (error)
        return error;

    return tmf882x_mode_bl_read_status(bl, 5);
}

int32_t tmf882x_mode_bl_send_cmd(struct tmf882x_mode_bl *bl)
{
    int32_t error = -1;
    uint8_t *wbuf = get_bl_cmd_buf(bl);
    uint8_t wsize = BL_CALC_CMD_SIZE(wbuf[1]);
    if (!verify_mode(&bl->mode)) return -1;
    if (is_bl_cmd_busy(bl))
        return error;

    error = tof_i2c_write(priv(bl), BL_REG_CMD_STATUS, wbuf, wsize);
    if (error)
        return error;

    return error;
}

int32_t tmf882x_mode_bl_short_cmd(struct tmf882x_mode_bl *bl,
                         int32_t cmd_e)
{
    struct tmf882x_mode_bl_short_cmd *cmd = &(bl->bl_command.short_cmd);
    if (!verify_mode(&bl->mode)) return -1;
    cmd->command = cmd_e;
    cmd->size = 0;
    cmd->chksum = tmf882x_calc_chksum(get_bl_cmd_buf(bl),
                                      BL_CALC_CHKSUM_SIZE(cmd->size));

    return tmf882x_mode_bl_send_rcv_cmd(bl);
}

int32_t tmf882x_mode_bl_reset(struct tmf882x_mode_bl *bl)
{
    int32_t error;
    struct tmf882x_mode_bl_short_cmd *cmd = &(bl->bl_command.short_cmd);
    if (!verify_mode(&bl->mode)) return -1;
    cmd->command = BL_CMD_RST;
    cmd->size = 0;
    cmd->chksum = tmf882x_calc_chksum(get_bl_cmd_buf(bl),
                                      BL_CALC_CHKSUM_SIZE(cmd->size));

    error = tmf882x_mode_bl_send_cmd(bl);
    if (error)
        return error;
    return 0;
}

int32_t tmf882x_mode_bl_ram_remap(struct tmf882x_mode_bl *bl)
{
    int32_t error;
    struct tmf882x_mode_bl_short_cmd *cmd = &(bl->bl_command.short_cmd);
    if (!verify_mode(&bl->mode)) return -1;
    cmd->command = BL_CMD_RAMREMAP_RST;
    cmd->size = 0;
    cmd->chksum = tmf882x_calc_chksum(get_bl_cmd_buf(bl),
                                      BL_CALC_CHKSUM_SIZE(cmd->size));

    error = tmf882x_mode_bl_send_cmd(bl);
    if (error)
        return error;
    return 0;
}

int32_t tmf882x_mode_bl_rom_remap(struct tmf882x_mode_bl *bl)
{
    int32_t error;
    struct tmf882x_mode_bl_short_cmd *cmd = &(bl->bl_command.short_cmd);
    if (!verify_mode(&bl->mode)) return -1;
    tof_info(priv(bl), "Switching to application in ROM");
    cmd->command = BL_CMD_ROMREMAP_RST;
    cmd->size = 0;
    cmd->chksum = tmf882x_calc_chksum(get_bl_cmd_buf(bl),
            BL_CALC_CHKSUM_SIZE(cmd->size));

    error = tmf882x_mode_bl_send_cmd(bl);
    if (error)
        return error;
    return 0;
}

int32_t tmf882x_mode_bl_addr_ram(struct tmf882x_mode_bl *bl, int32_t addr)
{
    struct tmf882x_mode_bl_addr_ram_cmd *cmd = &(bl->bl_command.addr_ram_cmd);
    if (!verify_mode(&bl->mode)) return -1;
    cmd->command = BL_CMD_RAM_ADDR;
    cmd->size = 2;
    cmd->addr_lsb  = addr & 0xff;
    cmd->addr_msb  = (addr >> 8) & 0xff;
    cmd->chksum = tmf882x_calc_chksum(get_bl_cmd_buf(bl),
            BL_CALC_CHKSUM_SIZE(cmd->size));

    return tmf882x_mode_bl_send_rcv_cmd(bl);
}

int32_t tmf882x_mode_bl_read_ram(struct tmf882x_mode_bl *bl,
                        uint8_t * rbuf, int32_t len)
{
    int32_t error = 0;
    int32_t rc;
    struct tmf882x_mode_bl_read_ram_cmd *cmd = &(bl->bl_command.read_ram_cmd);
    struct tmf882x_mode_bl_read_ram_resp *rsp = &(bl->bl_response.read_ram_resp);
    int32_t num = 0;
    if (!verify_mode(&bl->mode)) return -1;
    do {
        cmd->command = BL_CMD_RD_RAM;
        cmd->size = 1;
        cmd->num_bytes = ((len - num) >= BL_MAX_DATA_SZ) ?
            BL_MAX_DATA_SZ : (uint8_t) (len - num);
        cmd->chksum = tmf882x_calc_chksum(get_bl_cmd_buf(bl),
                BL_CALC_CHKSUM_SIZE(cmd->size));
        rc = tmf882x_mode_bl_send_rcv_cmd(bl);
        if (!rc) {
            /* command was successful, lets copy a batch of data over */
            if (rbuf)
                memcpy((rbuf + num), rsp->data, rsp->size);
            num += cmd->num_bytes;
        }
        error = error ? error : (rc ? rc : 0);
    } while ((num < len) && !rc);
    return error;
}

int32_t tmf882x_mode_bl_write_ram(struct tmf882x_mode_bl *bl,
                              const uint8_t *buf, int32_t len)
{
    struct tmf882x_mode_bl_write_ram_cmd *cmd = &(bl->bl_command.write_ram_cmd);
    int32_t idx = 0;
    int32_t num = 0;
    uint8_t chunk_bytes = 0;
    int32_t error = 0;
    int32_t rc;
    if (!verify_mode(&bl->mode)) return -1;
    do {
        cmd->command = BL_CMD_WR_RAM;
        chunk_bytes = ((len - num) > BL_MAX_DATA_SZ) ?
            BL_MAX_DATA_SZ : (uint8_t) (len - num);
        cmd->size = chunk_bytes;
        for(idx = 0; idx < cmd->size; idx++) {
            cmd->data[idx] = buf[num + idx];
        }
        /* add chksum to end */
        cmd->data[(uint8_t)cmd->size] =
            tmf882x_calc_chksum(get_bl_cmd_buf(bl),
                    BL_CALC_CHKSUM_SIZE(cmd->size));
        rc = tmf882x_mode_bl_send_rcv_cmd(bl);
        if (!rc)
            num += chunk_bytes;
        error = error ? error : (rc ? rc : 0);
    } while ((num < len) && !rc);
    return error;
}

int32_t tmf882x_mode_bl_upload_init(struct tmf882x_mode_bl *bl,
                           uint8_t salt)
{
    struct tmf882x_mode_bl_upload_init_cmd *cmd = &(bl->bl_command.upload_init_cmd);
    if (!verify_mode(&bl->mode)) return -1;
    cmd->command = BL_CMD_UPLOAD_INIT;
    cmd->size = 1;
    cmd->seed = salt;
    cmd->chksum = tmf882x_calc_chksum(get_bl_cmd_buf(bl),
            BL_CALC_CHKSUM_SIZE(cmd->size));

    return tmf882x_mode_bl_send_rcv_cmd(bl);
}

static int32_t hex_fwdl(struct tmf882x_mode_bl *bl, const uint8_t *buf, size_t len)
{
    int32_t size = 0;
    int32_t error;
    uint32_t patch_size = 0;
    uint32_t addr = 0;
    uint8_t bin[BL_MAX_DATA_SZ];
    tof_info(priv(bl), "Starting HEX fwdl");
    ihexi_init(&bl->hex, buf, len);

    while ( (size = ihexi_get_next_bin(&bl->hex, bin, sizeof(bin), &addr)) ) {

        if (size < 0) {
            tmf882x_dump_i2c_regs(to_parent(bl));
            tof_err(priv(bl),
                    "%s: Ram patch failed: %d", __func__, size);
            return size;
        }

        // add up patch size
        patch_size += size;

        error = tmf882x_mode_bl_addr_ram(bl, addr);
        if (error) {
            tmf882x_dump_i2c_regs(to_parent(bl));
            tof_info(priv(bl), "Error setting start addr %#x: \'%d\'",
                     addr, error);
            return error;
        }

        error = tmf882x_mode_bl_write_ram(bl, bin, size);
        if (error) {
            tof_info(priv(bl), "Error writing RAM: \'%d\'", error);
            tmf882x_dump_i2c_regs(to_parent(bl));
            return error;
        }
    }

    tof_info(priv(bl), "%s: patch size: %u B", __func__, patch_size);

    // If EOF is reached, issue RAM_REMAP command
    if ( ihexi_is_eof(&bl->hex) ) {
        error = tmf882x_mode_bl_ram_remap(bl);
        if (error) {
            tmf882x_dump_i2c_regs(to_parent(bl));
            tof_info(priv(bl), "Error RAM REMAPRESET command: \'%d\'", error);
            return error;
        }
        // FWDL is successful
        return 0;
    }

    // If we reached here, we have completed a *PARTIAL* Firmware Download, and
    // the client must continue to download the rest of the firmware until the
    // EOF record is reached
    return -1;
}

static int32_t bin_fwdl(struct tmf882x_mode_bl *bl, const uint8_t *buf, size_t len)
{
    int32_t error = 0;

    tof_info(priv(bl), "Starting BIN fwdl");

    error = tmf882x_mode_bl_addr_ram(bl, BL_DEFAULT_BIN_START_ADDR);
    if (error) {
        tof_info(priv(bl), "Error setting start addr: \'%d\'", error);
        return error;
    }

    error = tmf882x_mode_bl_write_ram(bl, buf, len);
    if (error) {
        tof_info(priv(bl), "Error writing RAM: \'%d\'", error);
        return error;
    }

    error = tmf882x_mode_bl_ram_remap(bl);
    if (error) {
        tof_info(priv(bl), "Error RAM REMAPRESET command: \'%d\'", error);
        return error;
    }

    return 0;
}

static int32_t tmf882x_mode_bl_app_switch(struct tmf882x_mode *self, uint32_t mode)
{
    struct tmf882x_mode_bl *bl;
    int32_t rc;

    if (mode == TMF882X_MODE_BOOTLOADER) return 0;
    // curently only supports switching to one mode through ROM
    if (mode != TMF882X_MODE_APP) return -1;
    if (!verify_mode(self)) return -1;
    bl = member_of(self, struct tmf882x_mode_bl, mode);

    // try a rom-remap reset to switch applications
    rc = tmf882x_mode_bl_rom_remap(bl);

    // close the bootloader because we are switching apps
    tmf882x_mode_bl_close(self);

    return rc;
}

static int32_t tmf882x_mode_bl_fwdl(struct tmf882x_mode *self, int32_t fwdl_type, const uint8_t *buf, size_t len)
{
    int32_t rc = 0;
    struct tmf882x_mode_bl *bl;

    if (!verify_mode(self)) return -1;
    bl = member_of(self, struct tmf882x_mode_bl, mode);

    if (TMF882X_BL_ENCRYPT_FLAG) {
        rc = tmf882x_mode_bl_upload_init(bl, BL_DEFAULT_SALT);
        if (rc) {
            tof_info(priv(bl), "Error setting upload salt: \'%d\'", rc);
            return rc;
        }
    }

    switch (fwdl_type) {
        case FWDL_TYPE_HEX:
            rc = hex_fwdl(bl, buf, len);
            break;
        case FWDL_TYPE_BIN:
            rc = bin_fwdl(bl, buf, len);
            break;
        default:
            tof_err(priv(bl), "Error invalid fwdl_type: \'%u\'", fwdl_type);
            return rc;
    }

    if (0 == rc)
        // close the bootloader because we are switching apps
        tmf882x_mode_bl_close(self);

    return rc;
}

static const struct mode_vtable ops = {
    .tag = TMF882X_BL_MODE_TAG,
    .open = tmf882x_mode_bl_open,
    .fwdl = tmf882x_mode_bl_fwdl,
    .mode_switch = tmf882x_mode_bl_app_switch,
    .close = tmf882x_mode_bl_close,
};

void tmf882x_mode_bl_init(struct tmf882x_mode_bl *bl, void *priv)
{
    if (!bl) return;
    tmf882x_mode_init(to_parent(bl), &ops, priv);
}

