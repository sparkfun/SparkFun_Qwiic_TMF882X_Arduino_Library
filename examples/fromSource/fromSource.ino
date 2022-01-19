#include <Wire.h>
#include "QwiicTMF882X.h"
#include "source\tof_bin_image.h"
#include "source\tof_factory_cal.h"
#include "source\tmf882x.h"
#include "source\tmf882x_mode.h"

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


void setup(){

	Wire.begin();
	Serial.begin(115200);

	while(!Serial){delay(10);}

	tmf882x_init(&tof, NULL);

	char ver[16] = { 0 };

	while (tmf882x_open(&tof)) {
			Serial.print("Error opening core ToF driver, retrying...\r\n");
			delay(3000000);
	}


	while (tmf882x_mode_switch(&tof, TMF882X_MODE_BOOTLOADER)) {
			Serial.print("Error switching to Bootloader for FWDL, retrying...\r\n");
			delay(3000000);
	}

	/**************************************************************************
	 *
	 * Perform FWDL
	 *     - FWDL supports "bin" download or "intel hex format" download
	 *
	 *************************************************************************/
	while (tmf882x_fwdl(&tof, FWDL_TYPE_BIN, tof_bin_image,
					tof_bin_image_length)) {
			Serial.print("Error during FWDL, retrying...\r\n");
			delay(3000000);
	}

	/**************************************************************************
	 *
	 * Retrieve the current mode Firmware version information
	 *
	 *************************************************************************/
	(void) tmf882x_get_firmware_ver(&tof, ver, sizeof(ver));
	Serial.print("TMF882X FW: APP version: %s\r\n");

  while (tmf882x_ioctl(&tof, IOCAPP_GET_CFG, NULL, &tofcfg)) {
        Serial.print("Error retrieving APP config, retrying...\r\n");
        delay(3000000);
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
        Serial.print("Error setting APP config, retrying...\r\n");
        delay(3000000);
    }

    /**************************************************************************
     *
     * Write the APP factory calibration data
     *  - IOCAPP_SET_CALIB is the ioctl command code used to set the APP calibration
     *
     *************************************************************************/
    while (tmf882x_ioctl(&tof, IOCAPP_SET_CALIB, &calibration_data, NULL)) {
        Serial.print("Error setting APP calib data, retrying...\r\n");
        delay(3000000);
    }

    Serial.print("\r\n\r\nPress 'ISP' button to start (GREEN LED ON) or stop "
           "(GREEN LED OFF) ToF measurement.\r\n\r\n");
}



void loop(){


}
