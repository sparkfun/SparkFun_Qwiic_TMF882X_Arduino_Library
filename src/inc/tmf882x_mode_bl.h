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
 * OWNER OR CONTRIBUTORS BE LIAblE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */
/** @file tmf882x_mode_bl.h
 *
 *  TMF882X Bootloader mode interface
 */


#ifndef __TMF882X_MODE_BL_H
#define __TMF882X_MODE_BL_H

#include "tmf882x_mode.h"
#include "intel_hex_interpreter.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tmf882x_mode_bl;
/** @brief
 *      Return pointer to @ref tmf882x_mode_bl from pointer to
 *      @ref tmf882x_mode
 */
#define mode_to_bl(x) \
    ((struct tmf882x_mode_bl *) \
     (member_of(x, struct tmf882x_mode_bl, mode)))

#define BL_CMD_SIZE                1
#define BL_DATA_LEN_SIZE           1
#define BL_CHKSUM_SIZE             1


/**
 * SparkFun Changes/Additions March 2022
 * 
 * For MCU's that don't support an I2C transfer buffer > 128,
 * the BL_NUM_DATA value must be reduced below the default of
 * 128. This system uses this value to chunk over firmware udpates,
 * which include a checksum.
 * 
 * So for  the ESP32 and the nrf52840, this size is reduced
 */ 
     

#ifdef ESP32
#define BL_NUM_DATA                120      

// The nrf52840 - this #define is from the Arduino biuld setup
#elif NRF52840_XXAA
#define BL_NUM_DATA                30 

// Default
#else
#define BL_NUM_DATA                128    

#endif

#define BL_MAX_DATA_SZ             (BL_NUM_DATA*sizeof(uint8_t))

#define BL_MSG_HEADER_SIZE          (BL_CMD_SIZE + \
                                     BL_DATA_LEN_SIZE)
#define BL_MSG_FOOTER_SIZE          BL_CHKSUM_SIZE
#define BL_MSG_CMD_MAX_SIZE         (BL_MSG_HEADER_SIZE   + \
                                     BL_MAX_DATA_SZ + \
                                     BL_MSG_FOOTER_SIZE)
#define BL_CALC_CHKSUM_SIZE(sz)     ((sz) + \
                                     BL_MSG_HEADER_SIZE)
#define BL_CALC_CMD_SIZE(sz)        (BL_CALC_CHKSUM_SIZE(sz) + \
                                     BL_MSG_FOOTER_SIZE)
#define BL_CALC_RSP_SIZE(sz)        ((sz) + \
                                     BL_MSG_HEADER_SIZE + \
                                     BL_MSG_FOOTER_SIZE)

/**
 *  @enum tmf882x_mode_bl_regs
 *  @brief
 *      register map specific to bootloader mode
 */
enum tmf882x_mode_bl_regs {
    BL_REG_CMD_STATUS    = 0x08,
    BL_REG_DATA_SIZE     = 0x09,
    BL_REG_DATA_0        = 0x0A,
    BL_REG_DATA_127      = 0x89,
    BL_REG_CHKSUM        = 0x8A,
};

/**
 *  @enum tmf882x_mode_bl_cmd
 *  @brief
 *      all bootloader mode commands
 */
enum tmf882x_mode_bl_cmd {
    BL_CMD_RST            = 0x10,
    BL_CMD_RAMREMAP_RST   = 0x11,
    BL_CMD_ROMREMAP_RST   = 0x12,
    BL_CMD_EXT_FLASHRST   = 0x13,
    BL_CMD_UPLOAD_INIT    = 0x14,
    BL_CMD_ADDR_EXT_FLASH = 0x15,
    BL_CMD_BIST           = 0x2C,
    BL_CMD_RD_RAM         = 0x40,
    BL_CMD_WR_RAM         = 0x41,
    BL_CMD_RAM_ADDR       = 0x43,
};

/**
 *  @enum tmf882x_mode_bl_cmd_stat
 *  @brief
 *      all bootloader mode command status return codes
 */
enum tmf882x_mode_bl_cmd_stat {
    BL_STAT_READY          = 0x0,
    BL_STAT_ERR_SIZE       = 0x1,
    BL_STAT_ERR_CSUM       = 0x2,
    BL_STAT_ERR_RES        = 0x3,
    BL_STAT_ERR_APP        = 0x4,
    BL_STAT_ERR_TIMEOUT    = 0x5,
    BL_STAT_ERR_LOCK       = 0x6,
    BL_STAT_ERR_RANGE      = 0x7,
    BL_STAT_ERR_MORE       = 0x8,
    BL_STAT_ERROR1         = 0x9,
    BL_STAT_ERROR2         = 0xA,
    BL_STAT_ERROR3         = 0xB,
    BL_STAT_ERROR4         = 0xC,
    BL_STAT_ERROR5         = 0xD,
    BL_STAT_ERROR6         = 0xE,
    BL_STAT_ERROR7         = 0xF,
    BL_STAT_CMD_BUSY       = 0x10,
    MAX_BL_STAT,
};

/**
 *  @brief
 *      Returns non-zero if the CMD_STAT value indicates the bootloader
 *      is busy performing a command
 */
#define BL_IS_CMD_BUSY(x)          ((x) >= BL_STAT_CMD_BUSY)

/*****************************************************************************
 *
 *
 *  START Boot Loader structures
 *
 *
 * ***************************************************************************
 */

/*****************************************************************************/
/***** Bootloader command responses *****/
/*****************************************************************************/
struct tmf882x_mode_bl_short_resp {
    uint8_t status;
    uint8_t size;
    uint8_t reserved[BL_MSG_CMD_MAX_SIZE - 3];
    uint8_t chksum;
};

struct tmf882x_mode_bl_read_ram_resp {
    uint8_t status;
    uint8_t size;
    uint8_t data[BL_MSG_CMD_MAX_SIZE + 1]; /* chksum at flexible position */
};

struct tmf882x_anon_resp {
    uint8_t data[BL_MSG_CMD_MAX_SIZE];
};

union tmf882x_mode_bl_response {
    struct tmf882x_anon_resp        anon_resp;
    struct tmf882x_mode_bl_read_ram_resp read_ram_resp;
    struct tmf882x_mode_bl_short_resp    short_resp;
};

/*****************************************************************************/
/***** Bootloader commands *****/
/*****************************************************************************/
struct tmf882x_mode_bl_short_cmd {
    uint8_t command;
    uint8_t size;
    uint8_t chksum;
    uint8_t reserved[BL_MAX_DATA_SZ];
};

struct tmf882x_mode_bl_upload_init_cmd {
    uint8_t command;
    uint8_t size;
    uint8_t seed;
    uint8_t chksum;
    uint8_t reserved[BL_MAX_DATA_SZ - 1];
};

struct tmf882x_mode_bl_read_ram_cmd {
    uint8_t command;
    uint8_t size;
    uint8_t num_bytes;
    uint8_t chksum;
    uint8_t reserved[BL_MAX_DATA_SZ - 1];
};

struct tmf882x_mode_bl_write_ram_cmd {
    uint8_t command;
    uint8_t size;
    uint8_t data[BL_MAX_DATA_SZ + 1]; /*chksum in flexible position */
};

struct tmf882x_mode_bl_addr_ram_cmd {
    uint8_t command;
    uint8_t size;
    uint8_t addr_lsb;
    uint8_t addr_msb;
    uint8_t chksum;
    uint8_t reserved[BL_MAX_DATA_SZ - 2];
};

struct tmf882x_mode_bl_anon_cmd {
    uint8_t data[BL_MSG_CMD_MAX_SIZE];
};

union tmf882x_mode_bl_command {
    struct tmf882x_mode_bl_anon_cmd        anon_cmd;
    struct tmf882x_mode_bl_addr_ram_cmd    addr_ram_cmd;
    struct tmf882x_mode_bl_write_ram_cmd   write_ram_cmd;
    struct tmf882x_mode_bl_read_ram_cmd    read_ram_cmd;
    struct tmf882x_mode_bl_upload_init_cmd upload_init_cmd;
    struct tmf882x_mode_bl_short_cmd       short_cmd;
};

/**
 * This is the Bootloader mode context structure
 */
struct tmf882x_mode_bl {
    /** This member is the Base mode context */
    struct tmf882x_mode mode;
    /** This member is the Intel Hex Interpreter context */
    struct intel_hex_interpreter hex;
    /** This member is the bootloader command */
    union tmf882x_mode_bl_command  bl_command;
    /** This member is the bootloader command response */
    union tmf882x_mode_bl_response bl_response;
};

/*****************************************************************************
 *
 *
 *  END Boot Loader structures
 *
 *
 * ***************************************************************************
 */
/*
 * ************************************************************************
 *
 *  Function Declarations
 *
 * ***********************************************************************
 */

/**
 * Initialize a @ref tmf882x_mode_bl context structure
 * @param[in] bl
 *      pointer to bl mode context
 * @param[in] priv
 *      User-private context to pass back through callback functions
 * @note
 *      Direct calls to this function should not be made, use the
 *      @ref tmf882x_tof interface instead
 */
extern void tmf882x_mode_bl_init(struct tmf882x_mode_bl *bl, void *priv);

#ifdef __cplusplus
}
#endif
#endif
