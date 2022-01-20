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

//#include "QwiicTMF882X.h"
#include "tmf882x_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "mcu_tmf882x_config.h"
#include "inc\platform_wrapper.h"
#include <Wire.h>
#include "SdFat.h"

#define TMF882X_I2C_ADDR 0x41
#define MAX_FWDL_FILESIZE 256 << 10 // hopefully 256 KB is large enough
#define PWR_EN_PIN 10

#define SD_FAT_TYPE 0
#define SPI_CLOCK SD_SCK_MHZ(50)

#ifdef CS
const uint8_t SD_CS_PIN = CS;
#else
const uint8_t SD_CS_PIN = 10;
#endif

const char* calibFile= "file_config.txt";

SdFat sd;
File fwdl_fd;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)

static struct tmf882x_tof tof = {0};
static struct platform_ctx ctx = {
    .debug = 0,
    .i2c_addr = TMF882X_I2C_ADDR,
    .tof = &tof
};


void setup(){

	Wire.begin();

	Serial.begin(115200);

	if (!sd.begin(SD_CONFIG))
		sd.initErrorHalt(&Serial);

}

void loop()
{
    int32_t rc = 0;
    uint32_t exit_num_results = 0;
    bool do_factory_calib = false;
    struct tmf882x_mode_app_calib tof_calib = {0};
    uint32_t fac_cal_len = 0;
    char *fwdl_file_contents;
    uint32_t fwdl_file_size;
		long optSelect = Serial.parseInt(); 
    
		switch (optSelect) {
				case 1:
						Serial.print("\n\n");
						Serial.print("Options:\n");
						Serial.print("\t-1: Display this help screen\n");
						Serial.print("\t-3: Display debug output from core driver\n");
						Serial.print("\t-4 <input_fac_calib_file>: Factory calibration file "
									 "with newline-separated hexadecimal values\n");
						Serial.print("\t-6 <num_results>: Read <num_results> and then exit\n");
						Serial.print("\t-z <output_fac_calib_file>: Perform factory "
									 " calibration and print to <output_fac_calib_file>\n");
						Serial.print("\n\n");
						Serial.print("NOTE: If you are tying to run this MCU driver application on\n"
									 "      an EVM platform, make sure to first disable the EVM demo\n"
									 "      application by running these commands:\n"
									 "\t\t\'pkill commanager\'\n""\t\'rmmod tmf882x\'\n");
						Serial.print("\n\n");
						break;
				case 2:
						if( sd.exists(calibFile) ){

							Serial.print("Input factory calibration file: \'%s\'\n"),
							rc = read_factory_calibration_file(calibFile, &tof_calib);
							if (rc < 0) {
									Serial.print("Error reading Factory calibration file: ");
									Serial.println(rc);
							}

						}

						fac_cal_len = rc;
						break;
				case 4:
						ctx.debug += 1;
						break;
				case 5:
						break;
				case 6:
						Serial.print("Output factory calibration file: ");
						Serial.println(calibFile);
						do_factory_calib = true;
						break;
				default:
						Serial.print("Error parsing cmdline args.\n");
		}
		
    // Power on device
    platform_wrapper_power_off(PWR_EN_PIN);
    platform_wrapper_power_on(PWR_EN_PIN);

    if (calibFile && strlen(calibFile)) {

        if(!fwdl_fd.open(calibFile, O_RDONLY)){
            Serial.println("Error opening FWDL file");
        }

        fwdl_file_contents = (char *)malloc(MAX_FWDL_FILESIZE);
        if (!fwdl_file_contents) {
            Serial.println("Error not enough memory to read");
            fwdl_fd.close();
        }

        fwdl_file_size = fwdl_fd.read(fwdl_file_contents, MAX_FWDL_FILESIZE);
        if (fwdl_file_size == -1) {
            Serial.println("Error reading FWDL file");
						free(fwdl_file_contents);
            fwdl_fd.close();
        }
    } 

		else {

        // no firmware hex file input by user
        fwdl_file_contents = NULL;
        fwdl_file_size = 0;

    }

    // Init application mode on device
    if (platform_wrapper_init_device(&ctx, fwdl_file_contents, fwdl_file_size)) {
        Serial.print("Error loading application mode\n");
        free(fwdl_file_contents);
        fwdl_fd.close();
    }

    free(fwdl_file_contents);
    fwdl_fd.close();

    rc = platform_wrapper_cfg_device(&ctx);

    if (rc) {
        Serial.print("Error configuring device: ");
				Serial.println(rc);
    }

    if (do_factory_calib) {
        rc = platform_wrapper_factory_calibration(&ctx, &tof_calib);
        if (rc) {
            Serial.print("Error performing factory calibration:");
            Serial.println(rc);
        }
        //dump to output factory calib file
        rc = factory_calib_save_to_file(&tof_calib, calibFile);
				Serial.println(rc);
    } 
		else {
        // Function only returns if a specified number of measurements are given
        platform_wrapper_start_measurements(&ctx, exit_num_results, &tof_calib);
    }

    platform_wrapper_power_off(PWR_EN_PIN);
}

