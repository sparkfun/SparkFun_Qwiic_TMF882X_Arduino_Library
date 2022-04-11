
#include "inc/tmf882x_mode_app_ioctl.h"

#ifndef TOF_FACTORY_CAL_H_
#define TOF_FACTORY_CAL_H_

#define CAL_DATA_SIZE	188U

static struct tmf882x_mode_app_calib calibration_data = {
    .data = {
    		0,
	},
	.calib_len = CAL_DATA_SIZE,
};

#endif /* TOF_FACTORY_CAL_H_ */
