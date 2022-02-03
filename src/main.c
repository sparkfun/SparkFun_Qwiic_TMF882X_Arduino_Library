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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/limits.h>
#include "mcu_tmf882x_config.h"
#include "tmf882x_interface.h"
#include "platform_wrapper.h"

#define TMF882X_I2C_ADDR (0x41)
#define MAX_FWDL_FILESIZE (256 << 10) // hopefully 256 KB is large enough

static struct tmf882x_tof tof = {0};
static struct platform_ctx ctx = {
    .debug = 0,
    .i2c_addr = TMF882X_I2C_ADDR,
    .tof = &tof,
};

static int32_t read_factory_calibration_file(const char *filename,
                                             struct tmf882x_mode_app_calib *calib)
{
    FILE *fac_cal_file;
    char * line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    uint32_t count = 0;
    int32_t error = 0;

    if (!filename || !calib)
        return -1;

    fac_cal_file = fopen(filename, "r");

    if (!fac_cal_file)
        return -1;

    while ((read = getline(&line, &len, fac_cal_file) != -1)) {
        error = sscanf(line, "%hhx", &calib->data[calib->calib_len++]);
        if (error != 1)
            return -1;
        if (calib->calib_len >= sizeof(calib->data))
            break; // we have filled the factory calibration buffer
    }

    printf("Factory calibration data: \n");
    for (uint32_t i = 0; i < calib->calib_len; i++) {
        printf("0x%02x ", calib->data[i]);
    }
    printf("\n");

    fclose(fac_cal_file);
    if (line)
        free(line);

    return calib->calib_len;
}

static int32_t factory_calib_save_to_file(struct tmf882x_mode_app_calib *calib,
                                          const char *output_filename)
{
    int32_t flags = 0;
    int32_t error = 0;
    FILE *fac_cal_file;

    fac_cal_file = fopen(output_filename, "w");

    if (!fac_cal_file)
        return -1;

    for (int i = 0; i < calib->calib_len; i++) {
        fprintf(fac_cal_file, "0x%02x\n", calib->data[i]);
    }
    fclose(fac_cal_file);
    printf("Factory calibration complete, saved to: '%s'\n", output_filename);

    return 0;
}

void catch_sigint(int parm)
{
    platform_wrapper_power_off();
    exit(0);
}

int main(int argc, char * argv[])
{
    int c;
    int32_t rc = 0;
    uint32_t exit_num_results = 0;
    char firmware_file[PATH_MAX] = "";
    char i2cdev_path[PATH_MAX] = {0};
    bool do_factory_calib = false;
    struct tmf882x_mode_app_calib tof_calib = {0};
    uint32_t fac_cal_len = 0;
    char fac_cal_fname[PATH_MAX] = {0};
    char *fwdl_file_contents;
    uint32_t fwdl_file_size;
    int fwdl_fd;

    signal(SIGINT, catch_sigint);

    while ((c = getopt (argc, argv, "hc:dbn:vz:y:")) != -1) {
        switch (c) {
            case 'h':
                printf("\n\n");
                printf("Help: ./%s [options] [firmware_file]\n",
                        MCU_TMF882X_PROJ_NAME);
                printf("\n\n");
                printf("\tfirmware_file: The firmware patch to download to "
                       "the device (optional)\n");
                printf("\n\n");
                printf("Options:\n");
                printf("\t-h: Display this help screen\n");
                printf("\t-b: Use 8x8 mode (if HW supports)\n");
                printf("\t-d: Display debug output from core driver\n");
                printf("\t-c <input_fac_calib_file>: Factory calibration file "
                       "with newline-separated hexadecimal values\n");
                printf("\t-n <num_results>: Read <num_results> and then exit\n");
                printf("\t-v: Display version info and exit\n");
                printf("\t-y <i2cdev>: Use specified i2c device file (default: '/dev/i2c-1')\n");
                printf("\t-z <output_fac_calib_file>: Perform factory "
                       " calibration and print to <output_fac_calib_file>\n");
                printf("\n\n");
                printf("NOTE: If you are tying to run this MCU driver application on\n"
                       "      an EVM platform, make sure to first disable the EVM demo\n"
                       "      application by running these commands:\n"
                       "\t\t\'pkill commanager\'\n""\t\'rmmod tmf882x\'\n");
                printf("\n\n");
                exit(0);
                break;
            case 'c':
                sscanf(optarg, "%s", fac_cal_fname);
                printf("Input factory calibration file: \'%s\'\n",
                       fac_cal_fname);
                rc = read_factory_calibration_file(fac_cal_fname,
                                                   &tof_calib);
                if (rc < 0) {
                    printf("Error reading Factory calibration file\n");
                    exit(rc);
                }
                fac_cal_len = rc;
                break;
            case 'b':
                ctx.mode_8x8 = 1;
                break;
            case 'd':
                ctx.debug += 1;
                break;
            case 'n':
                sscanf(optarg, "%u", &exit_num_results);
                break;
            case 'v':
                printf("mcu driver tmf882x test tool: '%s' Version: %u.%u.%u.%u\n",
                        MCU_TMF882X_PROJ_NAME,
                        MCU_TMF882X_VERSION_MAJOR,
                        MCU_TMF882X_VERSION_MINOR,
                        TMF882X_MAJ_MODULE_VER,
                        TMF882X_MIN_MODULE_VER
                        );
                exit(0);
                break;
            case 'y':
                sscanf(optarg, "%s", i2cdev_path);
                printf("I2C dev filepath: \'%s\'\n", i2cdev_path);
                ctx.i2cdev = i2cdev_path;
                break;
            case 'z':
                sscanf(optarg, "%s", fac_cal_fname);
                printf("Output factory calibration file: \'%s\'\n",
                       fac_cal_fname);
                do_factory_calib = true;
                break;
            default:
                fprintf(stderr, "Error parsing cmdline args.\n");
                exit(1);
        }
    }

    if (argc - optind) {
        sscanf(argv[optind], "%s", firmware_file);
        printf("Firmware file: %s\n", firmware_file);
    }

    // Power on device
    platform_wrapper_power_off();
    rc = platform_wrapper_power_on();
    if (rc) {
        fprintf(stderr, "Error asserting CE pin\n");
        exit(rc);
    }

    if (firmware_file && strlen(firmware_file)) {
        fwdl_fd = open(firmware_file, O_RDONLY);
        if (!fwdl_fd) {
            fprintf(stderr, "Error opening FWDL file: '%s'\n", firmware_file);
            exit(-1);
        }

        // allocate memory to read fwdl file contents
        fwdl_file_contents = (char *)malloc(MAX_FWDL_FILESIZE);
        if (!fwdl_file_contents) {
            fprintf(stderr, "Error not enough memory to read: '%s'\n", firmware_file);
            close(fwdl_fd);
            exit(1);
        }

        fwdl_file_size = read(fwdl_fd, fwdl_file_contents, MAX_FWDL_FILESIZE);
        if (fwdl_file_size < 1) {
            fprintf(stderr, "Error (%d) reading FWDL file '%s'\n",
                    fwdl_file_size, firmware_file);
            free(fwdl_file_contents);
            close(fwdl_fd);
            exit(-1);
        }

    } else {

        // no firmware hex file input by user
        fwdl_file_contents = NULL;
        fwdl_file_size = 0;

    }

    // Init application mode on device
    if (platform_wrapper_init_device(&ctx, fwdl_file_contents, fwdl_file_size)) {
        fprintf(stderr, "Error loading application mode\n");
        free(fwdl_file_contents);
        close(fwdl_fd);
        exit(-1);
    }

    free(fwdl_file_contents);
    close(fwdl_fd);

    rc = platform_wrapper_cfg_device(&ctx);
    if (rc) {
        fprintf(stderr, "Error configuring device: %d\n", rc);
        exit(-1);
    }

    if (do_factory_calib) {
        rc = platform_wrapper_factory_calibration(&ctx, &tof_calib);
        if (rc) {
            fprintf(stderr, "Error performing factory calibration: %d\n", rc);
        }
        //dump to output factory calib file
        rc = factory_calib_save_to_file(&tof_calib, fac_cal_fname);
        exit(rc);
    } else {
        // Function only returns if a specified number of measurements are given
        platform_wrapper_start_measurements(&ctx, exit_num_results, &tof_calib);
    }

    platform_wrapper_power_off();
    exit(0);
}

