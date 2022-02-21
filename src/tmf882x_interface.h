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

/** @file tmf882x_interface.h
 *
 *  TMF882X Core Driver interface
 */

#ifndef __TMF882X_INTERFACE_H
#define __TMF882X_INTERFACE_H

#include "inc/tmf882x.h"
#include "inc/tmf882x_mode.h"
#include "inc/tmf882x_mode_bl.h"
#include "inc/tmf882x_mode_app.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TMF882X_MAJ_MODULE_VER      3
#define TMF882X_MIN_MODULE_VER      7

#define QUOTE(x)                    #x
#define STRINGIFY(x)                QUOTE(x)
/** @brief TMF882X DCB driver module version string */
#define TMF882X_MODULE_VER          STRINGIFY(TMF882X_MAJ_MODULE_VER) "." \
                                    STRINGIFY(TMF882X_MIN_MODULE_VER)

/** @brief Timeout for FWDL (firmware download) */
#define TOF_FWDL_TIMEOUT_MSEC       30000

/**
 * @enum tmf882x_mode_t
 * @brief Supported application modes
 */
typedef enum tmf882x_mode_t {
    TMF882X_MODE_BOOTLOADER = 0x80, /**< Bootloader mode */
    TMF882X_MODE_APP        = 0x03, /**< Application mode */
} tmf882x_mode_t;

/**
 * @enum tmf882x_fwdl_type_t
 * @brief FWDL type when performing FWDL
 */
typedef enum tmf882x_fwdl_type_t {
    FWDL_TYPE_BIN,
    FWDL_TYPE_HEX,
} tmf882x_fwdl_type_t;

/**
 * @enum tmf882x_regs
 * @brief core register mapping common to all modes
 */
enum tmf882x_regs {
    TMF882X_APP_ID        = 0x00, /**< Application ID register */
    TMF882X_STAT          = 0xE0, /**< CPU Status register */
    TMF882X_INT_STAT      = 0xE1, /**< IRQ Status register */
    TMF882X_INT_EN        = 0xE2, /**< IRQ Enable register */
    TMF882X_ID            = 0xE3, /**< Chip ID register */
    TMF882X_REV_ID        = 0xE4, /**< Chip Revision register */
};

/**
 * @struct tmf882x_tof
 * @brief
 *      TMF882X DCB context handle
 * @var tmf882x_tof::bl
 *      This member holds the bootloader state context
 * @var tmf882x_tof::app
 *      This member holds the application state context
 * @var tmf882x_tof::state
 *      This member holds the base state context
 */
struct tmf882x_tof {

    union {
        struct tmf882x_mode_bl    bl;
        struct tmf882x_mode_app   app;
        struct tmf882x_mode       state;
    };

};

/************************************/
/***** Core Interface functions *****/
/************************************/

/**
 * @brief
 *      Initialize tof structure, *must be called* before using any other
 *      interface function.
 * @param[in] tof
 *      pointer to tof dcb interface context to initialize
 * @param[in] priv
 *      Private context to pass back to client for callback platform functions
 * @warning
 *      This function must be called before using any other interface function
 * @note
 *      This function performs no I/O with the device
 */
extern void tmf882x_init(struct tmf882x_tof *tof, void *priv);

/**
 * @brief
 *      Open the firmware core driver interface. No effect if already open,
 *      error if a different application interface is already open.
 * @param[in] tof
 *      tof dcb interface context
 * @warning
 *      All other interface functions should not be called without calling
 *      tmf882x_open() first (except @ref tmf882x_init).
 * @return 0 for sucess, otherwise failure (driver remains closed on failure)
 */
extern int32_t tmf882x_open(struct tmf882x_tof * tof);

/**
 * @brief
 *      Download new firmware. The new mode is automatically opened on success.
 * @param[in] tof
 *      tof dcb interface context
 * @param[in] buf
 *      Firmware data buffer
 * @param[in] len
 *      size of firmware data buffer
 * @note This function will try to re-open() the device after FWDL
 * @note This function supports partial Firmware Downloads when using intel
 *       hex record format. The return value will be negative and the device
 *       will not be re-opened until the EOF hex record is passed in.
 * @return 0 for sucess, otherwise failure
 */
extern int32_t tmf882x_fwdl(struct tmf882x_tof *tof, tmf882x_fwdl_type_t fwdl_type,
                            const uint8_t *buf, size_t len);

/**
 * @brief
 *      Perform an application mode switch operation on the current running
 *      application mode. The new mode is automatically opened on success.
 * @param[in] tof
 *      tof dcb interface context
 * @param[in] mode
 *      Requested @ref tmf882x_mode_t mode to switch to
 * @note
 *      Not all modes may support switching to any other mode
 * @note This function will try to re-open() the device after mode switch
 * @return 0 for sucess, otherwise failure
 */
extern int32_t tmf882x_mode_switch(struct tmf882x_tof *tof, tmf882x_mode_t mode);

/**
 * @brief
 *      Start measurements with current configuration
 * @param[in] tof
 *      tof dcb interface context
 * @return 0 for sucess, otherwise failure
 */
extern int32_t tmf882x_start(struct tmf882x_tof * tof);

/**
 * @brief
 *      Check for interrupt conditions and handle accordingly. Any output
 *      data is passed through shim platform layer in the form of one or more
 *      @ref tmf882x_msg
 * @param[in] tof
 *      tof dcb interface context
 * @return 0 for sucess, otherwise failure
 */
extern int32_t tmf882x_process_irq(struct tmf882x_tof * tof);

/**
 * @brief
 *      Stop measurements
 * @param[in] tof
 *      tof dcb interface context
 * @return 0 for sucess, otherwise failure
 */
extern int32_t tmf882x_stop(struct tmf882x_tof * tof);

/**
 * @brief
 *      Perform an IO Control command
 * @param[in] tof
 *      Tof dcb interface context
 * @param[in] cmd
 *      Mode-dependent command code
 * @param[in] input
 *      An input argument passed to the ioctl command. See
 *      @ref tmf882x_mode_app_ioctl.h for list of available ioctl commands
 *      and their respective argument(s). The type of input is dependent
 *      on the IOCTL command code. This argument may be NULL for commands
 *      that take no input.
 * @param[out] output
 *      An output argument returned from the ioctl command. See
 *      @ref tmf882x_mode_app_ioctl.h for list of available ioctl commands
 *      and their respective argument(s). The type of output is dependent
 *      on the IOCTL command code. This argument may be NULL for commands
 *      that return no data.
 * @return 0 for sucess, otherwise failure
 */
extern int32_t tmf882x_ioctl(struct tmf882x_tof *tof, uint32_t cmd,
                             const void *input, void *output);

/**
 * @brief
 *      Close the core driver firmware interface, the inverse operation
 *      of @ref tmf882x_open(). Device will be put into STANDBY power state.
 *      @ref tmf882x_open() must be called again to perform any operations
 *      on the device.
 * @param[in] tof
 *      tof dcb interface context
 * @return 0 for sucess, otherwise failure
 */
extern void tmf882x_close(struct tmf882x_tof *tof);

/**
 * @brief
 *      Get the base mode handle.
 * @param[in] tof
 *      tof dcb interface context
 * @return pointer to current tmf882x_mode
 */
static inline struct tmf882x_mode * tmf882x_mode_hndl(struct tmf882x_tof *tof)
{
    return &tof->state;
}

/**
 * @brief
 *      Return the current mode
 * @param[in] tof
 *      pointer to tof dcb interface context
 * @return current mode as @ref tmf882x_mode_t
 */
extern tmf882x_mode_t tmf882x_get_mode(struct tmf882x_tof *tof);

/**
 * @brief
 *      Enable debug logging of the DCB
 * @param[in] tof
 *      pointer to tof dcb interface context
 * @param[in] flag
 *      Non-zero value enables debug logging, 0 disables debug logging
 */
extern void tmf882x_set_debug(struct tmf882x_tof *tof, bool flag);

/**
 * @brief
 *      Fill buffer with version string of the current open firmware mode
 * @param[in] tof
 *      pointer to tof dcb interface context
 * @param[in] ver
 *      Buffer to be filled with version string
 * @param[in] len
 *      length of buffer
 * @return
 *      number of characters copied to buffer, negative if an error occurred
 */
extern int32_t tmf882x_get_firmware_ver(struct tmf882x_tof *tof, char *ver, size_t len);

/**
 * @brief
 *      Fill buffer with device revision string
 * @param[in] tof
 *      pointer to tof dcb interface context
 * @param[in] rev_buf
 *      Buffer to be filled with version string
 * @param[in] len
 *      length of buffer
 * @return
 *      number of characters copied to buffer
 */
extern int32_t tmf882x_get_device_revision(struct tmf882x_tof *tof, char *rev_buf, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* __TMF882X_INTERFACE_H */
