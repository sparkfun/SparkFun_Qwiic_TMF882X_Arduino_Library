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

/** @file tmf882x_mode_app.h
 *
 *  TMF882X Application mode interface
 */


#ifndef __TMF882X_MODE_APP_H
#define __TMF882X_MODE_APP_H

#include <stdbool.h>
#include "tmf882x.h"
#include "tmf882x_host_interface.h"
#include "tmf882x_mode_app_protocol.h"
#include "tmf882x_mode_app_ioctl.h"
#include "tmf882x_clock_correction.h"
#include "tmf882x_mode.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tmf882x_mode_app;
/** @brief
 *      Return pointer to @ref tmf882x_mode_app from pointer to
 *      @ref tmf882x_mode
 */
#define mode_to_app(x) \
    ((struct tmf882x_mode_app *) \
     (member_of(x, struct tmf882x_mode_app, mode)))

/** @brief
 *      Max i2c payload message size
 */
#ifdef CONFIG_TMF882X_NO_HISTOGRAM_SUPPORT
#define APP_MAX_MSG_SIZE        TMF8X2X_COM_HEADER_PLUS_PAYLOAD
#else
#define APP_MAX_MSG_SIZE        ((TMF882X_HIST_NUM_BINS * \
                                  TMF882X_HIST_NUM_TDC * \
                                  TMF882X_BYTES_PER_BIN))
#endif

/**
 *  @enum tmf882x_mode_app_pckt_indices
 *  @brief
 *      Indices for parsing i2c messages from the application
 */
enum tmf882x_mode_app_pckt_indices {
    APP_COM_RID_IDX        = 0,
    APP_COM_TID_IDX        = 1,
    APP_COM_SIZE_LSB_IDX   = 2,
    APP_COM_SIZE_MSB_IDX   = 3,

    /**< In multipacket msg 'DATA' is shifted down to make room for
     *   subpacket header */
    APP_COM_MULTI_NUM_IDX    = 4,
    APP_COM_MULTI_SIZE_IDX   = 5,
    APP_COM_MULTI_CFG_ID_IDX = 6,

    /**< In multipacket msg 'DATA' is shifted down to make room for
     *   subpacket header */
    APP_COM_DATA_IDX         = 4,
    APP_COM_MULTI_DATA_IDX   = 7,
};

/*****************************************************************************
 *
 *
 *  START app structures
 *
 *
 * ***************************************************************************
 */

/**
 * @struct tmf882x_mode_app_i2c_msg
 * @brief
 *      App mode i2c message
 * @var tmf882x_mode_app_i2c_msg::rid
 *      This member is the return message id for receiving messages,
 * @var tmf882x_mode_app_i2c_msg::cmd
 *      This member is the command message id for sending messages
 * @var tmf882x_mode_app_i2c_msg::tid
 *      This member is the monotonically-increasing Transaction ID
 * @var tmf882x_mode_app_i2c_msg::size
 *      This member is the size of the data buffer in the message
 * @var tmf882x_mode_app_i2c_msg::cfg_id
 *      This member is the subcapture configuration ID or breakpoint number
 * @var tmf882x_mode_app_i2c_msg::pckt_size
 *      This member is the subpacket payload size
 * @var tmf882x_mode_app_i2c_msg::pckt_num
 *      This member is the subpacket number in the total message
 * @var tmf882x_mode_app_i2c_msg::buf
 *      This member is the message data buffer
 */
struct tmf882x_mode_app_i2c_msg {
    uint8_t rid;
    uint8_t cmd;
    uint8_t tid;
    uint16_t size;
    uint8_t cfg_id;
    uint8_t pckt_size;
    uint8_t pckt_num;
    uint8_t buf[APP_MAX_MSG_SIZE];
};

/**
 * @struct tmf882x_mode_app
 * @brief
 *      This is the Application mode context structure
 * @var tmf882x_mode_app::mode
 *      This member is the @ref tmf882x_mode base class structure
 * @var tmf882x_mode_app::volat_data
 *      This member is all of the volatile data within the application mode
 *      context structure
 * @var tmf882x_mode_app::volat_data::is_open
 *      This member is whether the application mode is open
 * @var tmf882x_mode_app::volat_data::is_measuring
 *      This member is whether the application mode is currently measuring
 * @var tmf882x_mode_app::volat_data::clk_corr_enabled
 *      This member is whether the application mode is compensating results for
 *      clock skew
 * @var tmf882x_mode_app::volat_data::irq
 *      This member is the cached IRQ status while servicing device interrupts
 * @var tmf882x_mode_app::volat_data::cr
 *      This member tracks the clock correction data @ref struct tmf882x_clk_corr
 * @var tmf882x_mode_app::volat_data::msg
 *      This member is the @ref tmf882x_msg for return data output
 *      from the device
 * @var tmf882x_mode_app::volat_data::i2c_msg
 *      This member is the @ref tmf882x_mode_app_i2c_msg for sending/receiving
 *      i2c messages from the application mode
 * @var tmf882x_mode_app::volat_data::results
 *      This member is the result message format from the application mode
 * @var tmf882x_mode_app::volat_data::capture_num
 *      This member is the monotonically-increasing capture number for each
 *      result
 * @var tmf882x_mode_app::volat_data::cfg
 *      This member is the @ref tmf882x_mode_app_config configuration used
 *      for writing/reading configuration from the application mode. Two
 *      configuration structure tables are supported by the device
 * @var tmf882x_mode_app::volat_data::uid
 *      Buffer for reading out the Device UID
 * @var tmf882x_mode_app::volat_data::timestamp
 *      This member is the cached previous timestamp used in clock correction
 */


struct tmf882x_mode_app {

    struct tmf882x_mode mode;

    // Volatile data of application
    struct volat_data {

        // true if open
        bool is_open;

        // TRUE if measuring
        bool is_measuring;

        // TRUE if correcting for clock skew
        bool clk_corr_enabled;

        // cached IRQ status
        uint32_t irq;

        // clock correction
        struct tmf882x_clk_corr clk_cr;

        // output data from app module
        struct tmf882x_msg msg;

        // input/output from Chip
        struct tmf882x_mode_app_i2c_msg i2c_msg;

        // Capture number follows the result number from results
        uint32_t capture_num;

        // Application config type
        struct tmf882x_mode_app_config cfg;

        // Device UID
        uint8_t uid[sizeof(uint32_t)];

        // cached timestamp used for clock correction
        struct timespec timestamp;

    } volat_data;

};

/*****************************************************************************
 *
 *
 *  END app structures
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
 * @brief
 *      initialize a @ref tmf882x_mode_app context structure
 * @param[in] app
 *      pointer to app mode context
 * @param[in] priv
 *      User-private context to pass back through callback functions
 * @note
 *      Direct calls to this function should not be made, use the
 *      @ref tmf882x_tof interface instead
 */
extern void tmf882x_mode_app_init(struct tmf882x_mode_app *app, void *priv);

#ifdef __cplusplus
}
#endif
#endif
