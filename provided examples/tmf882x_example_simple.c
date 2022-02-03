/*
 * tmf882x_example.c
 *
 *  Created on: Aug 18, 2020
 *      Author: orya
 */

#include <stdint.h>
#include "board.h"
#include "fsl_gpio.h"
#include "fsl_i2c.h"
#include "fsl_utick.h"
#include "fsl_debug_console.h"
#include "tmf882x.h"
#include "tmf882x_interface.h"
#include "tof_bin_image.h"
#include "tof_factory_cal.h"

#define TMF882X_DEMO_MAJ_VER      1
#define TMF882X_DEMO_MIN_VER      6

#define QUOTE(x)                  #x
#define STRINGIFY(x)              QUOTE(x)
#define TMF882X_DEMO_VER          STRINGIFY(TMF882X_DEMO_MAJ_VER) "." \
                                  STRINGIFY(TMF882X_DEMO_MIN_VER)

#define EXAMPLE_SIMPLE_MAX_CH     9
#define TMF882X_I2C_ADDR          0x41
#define PERIODIC_UTICK_CNT        601
#define POLL_PERIOD_MS            20000
#define DEBOUNCE_PERIOD_MS        50000
#define SW_DEBOUNCE_CNT           (((DEBOUNCE_PERIOD_MS) / (POLL_PERIOD_MS)) + 1)
// Currently the CE port is connected to the BLUE LED GPIO on the devkit board
#define CE_GPIO_PORT              BOARD_LED_BLUE_GPIO_PORT
#define CE_GPIO_PIN               BOARD_LED_BLUE_GPIO_PIN

/***************************************************
 *     0: only output results/data from device
 *     1: output info level messages from core driver
 *     2+: output debug level messages from core driver
 ***************************************************/
const uint32_t g_core_drv_logging = 0;

// microsecond counter used for timing needed by the tmf882x core driver
static volatile uint32_t usec_cnt = 0;
// TMF882x core driver context
static struct tmf882x_tof tof;
// TMF882x core driver APP mode-specific config structure
static struct tmf882x_mode_app_config tofcfg;

/*
 * This function provides the core driver a way to retrieve relative
 * timestamps at microsecond resolution.
 */
uint32_t get_uticks(void) {
    uint32_t ticks;
    DisableIRQ(UTICK0_IRQn);
    ticks = usec_cnt;
    EnableIRQ(UTICK0_IRQn);
    return ticks;
}

/*
 * This function is called by the UTICK driver core periodically
 */
static void periodic_cb(void) {
    /* Periodically update our usec counter, this is not a true 1MHz counter,
     * but it is close enough for the needs of the demo. The true resolution
     * of the usec counter is = 1MHz / PERIODIC_UTICK_CNT
     */
    uint32_t temp = usec_cnt;
    temp += PERIODIC_UTICK_CNT;
    usec_cnt = temp;
}

static void power_on_tmf882x(void) {
    IOCON_PinMuxSet(IOCON, CE_GPIO_PORT, CE_GPIO_PIN, IOCON_DIGITAL_EN);
    GPIO_PinInit(GPIO, CE_GPIO_PORT, CE_GPIO_PIN, &(gpio_pin_config_t ) {
                    kGPIO_DigitalOutput, 1U });
}

/*
 * When the demo is measuring light the board LED GREEN
 */
static void update_board_led(bool measuring) {
    if (measuring)
        LED_GREEN_ON();
    else
        LED_GREEN_OFF();
}

/*
 * Simple debounce routine to check if the user hit the 'ISP' button
 * on the board.
 */
static bool user_pressed_measure_toggle_button(uint32_t *gpio_history) {
    // return debounced 0->1 transition of ISP switch
    if (!gpio_history)
        return false;
    uint32_t gpio_hist = *gpio_history;
    gpio_hist = (gpio_hist << 1)
            | GPIO_PinRead(GPIO, BOARD_SW1_GPIO_PORT, BOARD_SW1_GPIO_PIN);
    *gpio_history = gpio_hist;
    return ((gpio_hist & ((1 << SW_DEBOUNCE_CNT) - 1))
            == ((1 << (SW_DEBOUNCE_CNT - 1)) - 1));
}

/*
 * Log measure results to the UART as they come from the device.
 * See "tmf882x.h" for sensor output data type definitions.
 */
static void log_result(struct tmf882x_msg_meas_results *result_msg) {
    uint32_t distances_cm[TMF882X_MAX_MEAS_RESULTS] = { 0 };

    if (!result_msg)
        return;

    for (uint32_t hit = 0, idx = 0; hit < result_msg->num_results; ++hit) {
        idx = result_msg->results[hit].channel - 1;    // channel '0' is the reference channel
        idx += result_msg->results[hit].sub_capture * EXAMPLE_SIMPLE_MAX_CH;
        distances_cm[idx] = result_msg->results[hit].distance_mm / 10;
        
        /*
         * To print the confidence level of the result reported
         * PRINTF("conf: %u ", result_msg->results[hit].confidence);
         */
    }

    // Print distance results of the example channels
    for (uint32_t idx = 0; idx < EXAMPLE_SIMPLE_MAX_CH; ++idx) {
        PRINTF("%4.0u", distances_cm[idx]);
    }

    PRINTF("\r\n");
    return;
}

/*
 * Simple microsecond delay routine
 */
void ams_delay_us(uint32_t usec) {
    uint32_t start = get_uticks();
    while (usec > (get_uticks() - start)) {
        __asm("NOP");
        __asm("NOP");
        __asm("NOP");
    }
}

/**************************************************************************
 *
 * Callback function to handle output data from the TMF882x core driver
 *  - all output data after processing IRQs is of type 'struct tmf882x_msg'
 *  - See "tmf882x.h" for output data message type definitions
 *
 *************************************************************************/
int32_t tmf882x_mcu_handle_msg(void *ctx, struct tmf882x_msg *msg) {
    if (!msg)
        return -1;

    switch (msg->hdr.msg_id) {
    case ID_MEAS_RESULTS:
        log_result(&msg->meas_result_msg);
        break;
    default:
        break;
    }

    return 0;
}

/*
 * Demo run routine, never returns
 */
void tmf882x_example_run(void) {
    // Temporary variable for storing version info
    char ver[16] = { 0 };
    bool is_measuring = false;

    // Seed initial user-button history
    uint32_t sw_history = -GPIO_PinRead(GPIO, BOARD_SW1_GPIO_PORT,
            BOARD_SW1_GPIO_PIN);

    PRINTF("\r\n\r\n\r\n\r\n\r\n");
    PRINTF("\t*************************************************\r\n");
    PRINTF("\t***      Starting TMF882X Example Simple      ***\r\n");
    PRINTF("\t*************************************************\r\n\r\n");

    // Start the UTICK periodic interrupt
    UTICK_SetTick(UTICK0, kUTICK_Repeat, PERIODIC_UTICK_CNT, periodic_cb);

    PRINTF("TMF882X Example Simple version: %s\r\n", TMF882X_DEMO_VER);
    PRINTF("TMF882X Core Driver version: %s\r\n", TMF882X_MODULE_VER);

    // Assert the CE pin on the TMF882X to turn on the device
    power_on_tmf882x();

    /*************************************************************************
     *
     * Initialize the TMF882X core driver.
     *  - 'struct tmf882x_tof' is the core driver context structure
     *  - performs no I/O, only data initialization
     *
     *************************************************************************/
    tmf882x_init(&tof, NULL);

    /**************************************************************************
     *
     * Enable debug logging in the TMF882X core driver
     *
     *************************************************************************/
    if (g_core_drv_logging > 1)
        tmf882x_set_debug(&tof, true);

    /**************************************************************************
     *
     * Open the TMF882X core driver
     *  - perform chip initialization
     *  - perform mode-specific initialization
     *
     *************************************************************************/
    while (tmf882x_open(&tof)) {
        PRINTF("Error opening core ToF driver, retrying...\r\n");
        ams_delay_us(3000000);
    }

    /**************************************************************************
     *
     * Switch from the current mode to the Bootloader mode
     *     - Must be in the Bootloader mode to perform Firmware Download (FWDL)
     *     - All modes support switching to the Bootloader mode
     *
     *************************************************************************/
    while (tmf882x_mode_switch(&tof, TMF882X_MODE_BOOTLOADER)) {
        PRINTF("Error switching to Bootloader for FWDL, retrying...\r\n");
        ams_delay_us(3000000);
    }

    /**************************************************************************
     *
     * Perform FWDL
     *     - FWDL supports "bin" download or "intel hex format" download
     *
     *************************************************************************/
    while (tmf882x_fwdl(&tof, FWDL_TYPE_BIN, tof_bin_image,
            tof_bin_image_length)) {
        PRINTF("Error during FWDL, retrying...\r\n");
        ams_delay_us(3000000);
    }

    /**************************************************************************
     *
     * Retrieve the current mode Firmware version information
     *
     *************************************************************************/
    (void) tmf882x_get_firmware_ver(&tof, ver, sizeof(ver));
    PRINTF("TMF882X FW: APP version: %s\r\n", ver);

    /**************************************************************************
     *
     * Retrieve the current APP mode configuration data
     *  - 'struct tmf882x_mode_app_config' is the APP mode config structure
     *  - IOCAPP_GET_CFG is the ioctl command code used to get the APP config
     *
     *************************************************************************/
    while (tmf882x_ioctl(&tof, IOCAPP_GET_CFG, NULL, &tofcfg)) {
        PRINTF("Error retrieving APP config, retrying...\r\n");
        ams_delay_us(3000000);
    }

    /**************************************************************************
     *
     * Change the APP configuration
     *  - set the reporting period to 500 milliseconds
     *
     *************************************************************************/
    tofcfg.report_period_ms = 500;

    /**************************************************************************
     *
     * Commit the changed APP mode configuration data
     *  - IOCAPP_SET_CFG is the ioctl command code used to set the APP config
     *
     *************************************************************************/
    while (tmf882x_ioctl(&tof, IOCAPP_SET_CFG, &tofcfg, NULL)) {
        PRINTF("Error setting APP config, retrying...\r\n");
        ams_delay_us(3000000);
    }

    /**************************************************************************
     *
     * Write the APP factory calibration data
     *  - IOCAPP_SET_CALIB is the ioctl command code used to set the APP calibration
     *
     *************************************************************************/
    while (tmf882x_ioctl(&tof, IOCAPP_SET_CALIB, &calibration_data, NULL)) {
        PRINTF("Error setting APP calib data, retrying...\r\n");
        ams_delay_us(3000000);
    }

    PRINTF("\r\n\r\nPress 'ISP' button to start (GREEN LED ON) or stop "
           "(GREEN LED OFF) ToF measurement.\r\n\r\n");

    /**************************************************************************
     *
     * The demo runs forever polling the TMF882X device for measurement results
     *  - the poll period is POLL_PERIOD_MS
     *  - 'tmf882x_process_irq()' checks and clears the device interrupt flags
     *      and reads out any available data
     *  - check the 'ISP' user button to start/stop measurements
     *  - log measurement results to the debug serial port
     *
     *************************************************************************/
    while (1) {

        // delay for polling
        ams_delay_us(POLL_PERIOD_MS);

        // check & clear device IRQ status, read out available data
        tmf882x_process_irq(&tof);

        // start/stop measurements with the 'ISP' board switch
        if (user_pressed_measure_toggle_button(&sw_history)) {

            if (is_measuring) {

                // Stop the measurements of TMF882X
                (void) tmf882x_stop(&tof);
                PRINTF("\r\n\r\n\t<- Stopping Measurement\r\n\r\n");

            } else {

                // Start the measurements of TMF882X
                (void) tmf882x_start(&tof);
                PRINTF("\r\n\r\n\tStarting Measurement ->\r\n\r\n");

            }

            // Retrieve the current measurement state and update the board LED
            (void) tmf882x_ioctl(&tof, IOCAPP_IS_MEAS, NULL, &is_measuring);
            update_board_led(is_measuring);
        }

    }

    // never return from demo
}
