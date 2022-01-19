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

/** @file tmf882x_mode_app_ioctl.h
 *
 *  TMF882X APP mode ioctl definitions. See @ref tmf882x_ioctl() for how to pass
 *  input and output parameters to IOCTL driver mode functions.
 *
 *  _IOCTL_R    = IOCTL where driver returns data to the client through output param
 *  _IOCTL_W    = IOCTL where client writes data to the driver through input param
 *  _IOCTL_RW   = IOCTL data is both written by the client though the input param
 *                  and data is returned through the output param
 *  _IOCTL_N    = IOCTL where no data is written or returned
 */

#ifndef __TMF882X_MODE_APP_IOCTL_H
#define __TMF882X_MODE_APP_IOCTL_H

#include "tmf882x_mode.h"
#include "tmf882x_mode_app_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************
 * APP mode IOCTL definitions
 *******************************/

/** @brief APP mode IOCTL ID */
#define TMF882X_IOCTL_APP_MODE          0x02U
#define VERIFY_IOCAPP(nr) (_IOCTL_MODE(nr) == TMF882X_IOCTL_APP_MODE)

/**
 * @brief These are the command code numberings in the ioctl bit fields.
 *          Do not use these directly, use the IOCTL bitmasks IOCAPP_*.
 */
enum _tmf882x_mode_app_iocnr{
    APP_SET_CFG,
    APP_GET_CFG,
    APP_SET_SPADCFG,
    APP_GET_SPADCFG,
    APP_SET_CALIB,
    APP_GET_CALIB,
    APP_DO_FACCAL,
    APP_IS_MEAS,
    APP_DEV_UID,
    APP_IS_CLKADJ,
    APP_SET_CLKADJ,
    APP_SET_8X8MODE,
    APP_IS_8X8MODE,
    NUM_APP_IOCTL
};

/**
 * @struct tmf882x_mode_app_config
 * @brief
 *      This is the Application mode config structure that holds all
 *      configuration parameters for the application
 * @var tmf882x_mode_app_config::report_period_ms
 *      Result reporting period in milliseconds
 * @var tmf882x_mode_app_config::kilo_iterations
 *      Iterations * 1024 for measurements
 * @var tmf882x_mode_app_config::low_threshold
 *      Low distance threshold setting triggering interrupts
 * @var tmf882x_mode_app_config::high_threshold
 *      High distance threshold setting triggering interrupts
 * @var tmf882x_mode_app_config::zone_mask
 *      Zone mask for disabling interrupts for certain channels
 * @var tmf882x_mode_app_config::persistence
 *      Persistence setting for generating interrupts
 * @var tmf882x_mode_app_config::confidence_threshold
 *      Confidence threshold for generating interrupts
 * @var tmf882x_mode_app_config::gpio_0
 *      GPIO_0 config settings
 * @var tmf882x_mode_app_config::gpio_1
 *      GPIO_1 config settings
 * @var tmf882x_mode_app_config::power_cfg
 *      Power configuration settings
 * @var tmf882x_mode_app_config::spad_map_id
 *      Spad map identifier
 * @var tmf882x_mode_app_config::alg_setting
 *      Algorithm setting configuration
 * @var tmf882x_mode_app_config::histogram_dump
 *      Histogram dump configuration
 * @var tmf882x_mode_app_config::spread_spectrum
 *      Spread Spectrum configuration
 * @var tmf882x_mode_app_config::i2c_slave_addr
 *      I2C slave address configuration
 * @var tmf882x_mode_app_config::oscillator_trim
 *      Sensor Oscillator trim value
 */
struct tmf882x_mode_app_config {
    uint16_t report_period_ms;
    uint16_t kilo_iterations;
    uint16_t low_threshold;
    uint16_t high_threshold;
    uint32_t zone_mask;
    uint8_t persistence;
    uint8_t confidence_threshold;
    uint8_t gpio_0;
    uint8_t gpio_1;
    uint8_t power_cfg;
    uint8_t spad_map_id;
    uint32_t alg_setting;
    uint8_t histogram_dump;
    uint8_t spread_spectrum;
    uint8_t i2c_slave_addr;
    uint16_t oscillator_trim;
};

/**
 * @brief
 *      IOCTL command code to Write a configuration to the application mode
 * @param[in] input type: struct tmf882x_mode_app_config *
 * @param[out] output type: none
 * @return zero for success, fail otherwise
 */
#define IOCAPP_SET_CFG   _IOCTL_W( TMF882X_IOCTL_APP_MODE, APP_SET_CFG, struct tmf882x_mode_app_config )

/**
 * @brief
 *      IOCTL command code to Read a configuration from the application mode
 * @param[in] input type: none
 * @param[out] output type: struct tmf882x_mode_app_config *
 * @return zero for success, fail otherwise
 */
#define IOCAPP_GET_CFG   _IOCTL_R( TMF882X_IOCTL_APP_MODE, APP_GET_CFG, struct tmf882x_mode_app_config )

/**
 * @struct tmf882x_mode_app_spad_config
 * @brief
 *      This is the Application mode spad config structure that holds the
 *      complete spad configuration for the application
 * @var tmf882x_mode_app_spad_config::tmf882x_mode_app_single_spad_config::xoff_q1
 *      X-direction offset in Q1 format
 * @var tmf882x_mode_app_spad_config::tmf882x_mode_app_single_spad_config::yoff_q1
 *      Y-direction offset in Q1 format
 * @var tmf882x_mode_app_spad_config::tmf882x_mode_app_single_spad_config::xsize
 *      Size of spad map in X-direction
 * @var tmf882x_mode_app_spad_config::tmf882x_mode_app_single_spad_config::ysize
 *      Size of spad map in Y-direction
 * @var tmf882x_mode_app_spad_config::tmf882x_mode_app_single_spad_config::spad_mask
 *      Spad enable mask configuration (1 enable, 0 disable)
 * @var tmf882x_mode_app_spad_config::tmf882x_mode_app_single_spad_config::spad_map
 *      Spad channel mapping for measurement (channels 1 - 9)
 * @var tmf882x_mode_app_spad_config::spad_configs
 *      The list of spad configurations
 * @var tmf882x_mode_app_spad_config::num_spad_configs
 *      The number of spad configurations in @ref tmf882x_mode_app_spad_configs::spad_configs
 * @note The spad enable mask and map size should be 'ysize' * 'xsize' in
 *       length
 */

struct tmf882x_mode_app_spad_config {
    struct tmf882x_mode_app_single_spad_config {
        int8_t xoff_q1;
        int8_t yoff_q1;
        uint8_t xsize;
        uint8_t ysize;
        uint8_t spad_mask[TMF8X2X_COM_MAX_SPAD_SIZE];
        uint8_t spad_map[TMF8X2X_COM_MAX_SPAD_SIZE];
    } spad_configs[TMF8X2X_MAX_CONFIGURATIONS];
    uint32_t num_spad_configs;
};

/**
 * @brief
 *      IOCTL command code to Write a spad configuration to the application mode
 * @param[in] input type: struct tmf882x_mode_app_spad_config *
 * @param[out] output type: none
 * @return zero for success, fail otherwise
 */
#define IOCAPP_SET_SPADCFG   _IOCTL_W( TMF882X_IOCTL_APP_MODE, \
                                       APP_SET_SPADCFG, \
                                       struct tmf882x_mode_app_spad_config )

/**
 * @brief
 *      IOCTL command code to Read the spad configuration from the application mode
 * @param[in] input type: none
 * @param[out] output type: struct tmf882x_mode_app_spad_config *
 * @return zero for success, fail otherwise
 */
#define IOCAPP_GET_SPADCFG   _IOCTL_R( TMF882X_IOCTL_APP_MODE, \
                                       APP_GET_SPADCFG, \
                                       struct tmf882x_mode_app_spad_config )

#define TMF882X_MAX_CALIB_SIZE  (188 * 4) // 4x to handle 8x8
/**
 * @struct tmf882x_mode_app_calib
 * @brief
 *      This is the Application mode calibration structure.
 * @var tmf882x_mode_app_calib::data
 *      Calibration data buffer
 * @var tmf882x_mode_app_calib::calib_len
 *      Length of calibration data in calibration data buffer
 */
struct tmf882x_mode_app_calib {
    uint8_t data[TMF882X_MAX_CALIB_SIZE];
    uint32_t calib_len;
};

/**
 * @brief
 *      IOCTL command code to Write the calibration data to the application mode
 * @param[in] input type: struct tmf882x_mode_app_calib *
 * @param[out] output type: none
 * @return zero for success, fail otherwise
 */
#define IOCAPP_SET_CALIB     _IOCTL_W( TMF882X_IOCTL_APP_MODE, \
                                       APP_SET_CALIB, \
                                       struct tmf882x_mode_app_calib )

/**
 * @brief
 *      IOCTL command code to Read the current calibration data from the application mode
 * @param[in] input type: none
 * @param[out] output type: struct tmf882x_mode_app_calib *
 * @return zero for success, fail otherwise
 */
#define IOCAPP_GET_CALIB     _IOCTL_R( TMF882X_IOCTL_APP_MODE, \
                                       APP_GET_CALIB, \
                                       struct tmf882x_mode_app_calib )

/**
 * @brief
 *      IOCTL command code to Perform Factory Calibration and get the new calibration data
 * @param[in] input type: none
 * @param[out] output type: struct tmf882x_mode_app_calib *
 * @return zero for success, fail otherwise
 */
#define IOCAPP_DO_FACCAL     _IOCTL_R( TMF882X_IOCTL_APP_MODE, \
                                       APP_DO_FACCAL, \
                                       struct tmf882x_mode_app_calib )

/**
 * @brief
 *      IOCTL command code to Return whether the application mode is currently measuring
 * @param[in] input type: none
 * @param[out] output type: bool *
 * @return zero for success, fail otherwise
 */
#define IOCAPP_IS_MEAS     _IOCTL_R( TMF882X_IOCTL_APP_MODE, \
                                     APP_IS_MEAS, \
                                     bool )

/**
 * @struct tmf882x_mode_app_dev_UID
 * @brief
 *      This is the Application mode structure to hold the device Unique ID
 * @var tmf882x_mode_app_dev_UID::uid
 *      Unique Identifier (UID) string
 */
struct tmf882x_mode_app_dev_UID {
    char uid[32];
};

/**
 * @brief
 *      IOCTL command code to Retrieve the device Unique Identifier (UID)
 * @param[in] input type: none
 * @param[out] output type: struct tmf882x_mode_app_dev_UID *
 * @return zero for success, fail otherwise
 */
#define IOCAPP_DEV_UID     _IOCTL_R( TMF882X_IOCTL_APP_MODE, \
                                     APP_DEV_UID, \
                                     struct tmf882x_mode_app_dev_UID )

/**
 * @brief
 *      IOCTL command code to Read the clock compensation enable state
 * @param[in] input type: none
 * @param[out] output type: bool *
 * @return zero for success, fail otherwise
 */
#define IOCAPP_IS_CLKADJ     _IOCTL_R( TMF882X_IOCTL_APP_MODE, \
                                       APP_IS_CLKADJ, \
                                       bool )

/**
 * @brief
 *      IOCTL command code to Set the clock compensation enable state
 * @param[in] input type: bool *
 * @param[out] output type: none
 * @return zero for success, fail otherwise
 */
#define IOCAPP_SET_CLKADJ     _IOCTL_W( TMF882X_IOCTL_APP_MODE, \
                                        APP_SET_CLKADJ, \
                                        bool )

/**
 * @brief
 *      IOCTL command code to Set the 8x8 operating mode (TMF8828)
 * @param[in] input type: bool *
 * @param[out] output type: none
 * @return zero for success, fail otherwise
 * @warn Note that changing to/from 8x8 mode will reset the device
 *       configuration to its default.
 */
#define IOCAPP_SET_8X8MODE     _IOCTL_W( TMF882X_IOCTL_APP_MODE, \
                                         APP_SET_8X8MODE, \
                                         bool )

/**
 * @brief
 *      IOCTL command code to Read the 8x8 operating mode state (TMF8828)
 * @param[in] input type: none
 * @param[out] output type: bool *
 * @return zero for success, fail otherwise
 */
#define IOCAPP_IS_8X8MODE     _IOCTL_R( TMF882X_IOCTL_APP_MODE, \
                                        APP_IS_8X8MODE, \
                                        bool )

#ifdef __cplusplus
}
#endif
#endif
