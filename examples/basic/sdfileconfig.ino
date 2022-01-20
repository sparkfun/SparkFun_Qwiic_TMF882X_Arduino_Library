
#include "SdFat.h"

#define SD_FAT_TYPE 0
#define SPI_CLOCK SD_SCK_MHZ(50)

File fac_cal_file;
File root; 


static int32_t read_factory_calibration_file(const char *filename,
                                             struct tmf882x_mode_app_calib *calib)
{
    char * line = NULL;
    size_t len = 0;
    uint32_t count = 0;
    int32_t error = 0;

    fac_cal_file.open(filename, "r");

		fac_cal_file.read(filename);
    while( fac_cal_file.read(&line, &len, fac_cal_file) ) {
        error = sscanf(line, "%hhx", &calib->data[calib->calib_len++]);
        if (error != 1)
            return -1;
        if (calib->calib_len >= sizeof(calib->data))
            break; // we have filled the factory calibration buffer
    }

    Serial.print("Factory calibration data: \n");
    for (uint32_t i = 0; i < calib->calib_len; i++) {
				Serial.print("0x ");	
        Serial.print(calib->data[i]);
				Serial.print(" "); 
    }
    Serial.println();

    fac_cal_file.close();
    if (line)
        free(line);

    return calib->calib_len;
}


static int32_t factory_calib_save_to_file(struct tmf882x_mode_app_calib *calib,
                                          const char *output_filename)
{
    int32_t flags = 0;
    int32_t error = 0;

    fac_cal_file.open(output_filename, "w");

    for (int i = 0; i < calib->calib_len; i++) {
				Serial.println("0x ");
				Serial.println(calib->data[i]);

    }
    fac_cal_file.close();
    Serial.print("Factory calibration complete, saved to: ");
		Serial.println(output_filename); 

    return 0;
}
