//////////////////////////////////////////////////////////////////////////////
//
//  This is a library written for the SparkFun AMS TMF882X ToF breakout/qwiic board
//
//  SparkFun sells these at its website: www.sparkfun.com
//  Do you like this library? Help support SparkFun. Buy a board!
//
//  https://www.sparkfun.com/products/<TODO>
//
//  Written by <TODO>
//
//
//  https://github.com/sparkfun/<TODO>
//
//  License
//     <TODO>
//
//////////////////////////////////////////////////////////////////////////////


#include "qwiic_tmf882x.h"
#include "tmf882x_interface.h"
#include "inc/tmf882x_host_interface.h"

// Arduino things
#include <Arduino.h>


//  AMS library things
static tmf882x_tof tof={0};

static platform_ctx ctx = {
    NULL, //char* i2ccdev
    0, // i2c_addr
    0, //debug
    0, //curr_num_measurements
    0, //mode_8x8
    &tof //struct above
};

//////////////////////////////////////////////////////////////////////////////
// begin()
//
// Initializes the I2C bus, and the underlying sensor library. 
//
// Return Value: false on error, true on success
//

bool Qwiic_TMF882X::begin(TwoWire &wirePort, uint8_t deviceAddress)
{

    _i2cPort = &wirePort;
    _i2cPort->begin(); //This resets any setClock() the user may have done

    _deviceAddress = deviceAddress;

    // set address in the TMF882X context 
    ctx.i2c_addr = deviceAddress;

    // Call our C level i2c init routine  - this is used to provide an I2C 
    // interface to the AMS supplied library. 

    if(sfe_i2c_init(deviceAddress, (void*)_i2cPort))
        return false; 

    // init the application mode
    if(platform_wrapper_init_device(&ctx, 0, 0))
        return false;


    if(platform_wrapper_cfg_device(&ctx))
        return false;

    // TODO - not sure if this needed, but send in our factory calibration
    if(platform_wrapper_factory_calibration(&ctx, &calibration_data))
        return false;

    return true;
}
//////////////////////////////////////////////////////////////////////////////
// isConnected()
//
// Is the device connected to the i2c bus
//
// Return Value: false on not connected, true if it is connected 

bool Qwiic_TMF882X::isConnected()
{

    _i2cPort->beginTransmission((uint8_t)_deviceAddress);

    return _i2cPort->endTransmission() == 0;

}
//////////////////////////////////////////////////////////////////////////////
// getMeasurement()

bool Qwiic_TMF882X::getMeasurement(TMF882XMeasurement_t *results){

    platform_wrapper_start_measurements(&ctx, 1, NULL);

    TMF882XMeasurement_t *lastSample = platform_wrapper_get_last_measurement();

    if(!lastSample)
        return false;

    if(results)
        memcpy(results, lastSample, sizeof(TMF882XMeasurement_t));

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// printMeasurement()

void Qwiic_TMF882X::printMeasurement(TMF882XMeasurement_t *measurment){

    if(!measurment)
        return;

    platform_wrapper_print_measurment(&ctx, measurment);
}