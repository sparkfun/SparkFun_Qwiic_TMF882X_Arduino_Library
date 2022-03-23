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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "qwiic_tmf882x.h"
#include "mcu_tmf882x_config.h"
#include "inc/sfe_arduino_c.h"
#include "tof_factory_cal.h"

#include "inc/tmf882x_host_interface.h"

// Arduino things
#include <Arduino.h>


#define kTMF882XCalInterations 4000


//////////////////////////////////////////////////////////////////////////////
// init_tmf882x()
//
// Called to initialize the TMF device. 
//
// returns false if the init failes.

bool QwDevTMF882X::init_tmf882x(void){

    int32_t rc = 0;
    char app_ver[32] = {0};

    tmf882x_init(&_tof, this);

    // Open the driver
    if (tmf882x_open(&_tof)) {
        fprintf(stderr, "%s Error opening driver\n", __func__);
        return false;
    }


    if(tof_bin_image && tof_bin_image_length) {

       // printf("Using builtin fw image start addr: 0x%08x size: 0x%08x\n",
        //       tof_bin_image_start, tof_bin_image_length);

        // Do a mode switch to the bootloader (bootloader mode necessary for FWDL)
        if (tmf882x_mode_switch(&_tof, TMF882X_MODE_BOOTLOADER)) {
            fprintf(stderr, "%s mode switch failed\n", __func__);
            tmf882x_dump_i2c_regs(tmf882x_mode_hndl(&_tof));
            return false;
        }

        rc = tmf882x_fwdl(&_tof, FWDL_TYPE_BIN, tof_bin_image, tof_bin_image_length);
        if (rc) {
            fprintf(stderr, "Error (%d) performing FWDL with built-in firmware\n", rc);
            return false;
        }

    }
    // else use builtin ROM image
    else {

        // Mode switch while in bootloader mode tries to load the requested
        //  mode from ROM/Flash/etc since FWDL failed / is not available
        if (tmf882x_mode_switch(&_tof, TMF882X_MODE_APP)) {
            fprintf(stderr, "%s ROM switch to APP mode failed\n", __func__);
            tmf882x_dump_i2c_regs(tmf882x_mode_hndl(&_tof));
            return false;
        }

    }

    // Make sure we are running application mode
    if  (tmf882x_get_mode(&_tof) != TMF882X_MODE_APP) {
        fprintf(stderr, "%s failed to open APP mode\n", __func__);
        return false;
    }

    return true;
    
}

//////////////////////////////////////////////////////////////////////////////
bool QwDevTMF882X::setFactoryCalibration(struct tmf882x_mode_app_calib *tof_calib){


    //if(!_isInit)
      //  return false; 

    struct tmf882x_mode_app_config tof_cfg;

    int32_t error;

    if(!tof_calib) 
        return false;

    if(tmf882x_ioctl(&_tof, IOCAPP_GET_CFG, NULL, &tof_cfg))
        return false;

    // Change the iterations for Factory Calibration
    tof_cfg.kilo_iterations = kTMF882XCalInterations;

    if(tmf882x_ioctl(&_tof, IOCAPP_SET_CFG, &tof_cfg, NULL))
        return false;


    if(tmf882x_ioctl( &_tof, IOCAPP_DO_FACCAL, NULL, tof_calib))
        return false;

    return true;

}
//////////////////////////////////////////////////////////////////////////////
// init()
//
// init the system
//
// Return Value: false on error, true on success
//

bool QwDevTMF882X::init(void)
{

    if(_isInit)
        return true;

    //  do we have a bus yet? is the device connected?
    if(!_i2cBus || !_i2c_address || !_i2cBus->ping(_i2c_address))
        return false;

    // init the application mode
    if(!init_tmf882x())
        return false;

    // TODO - do this here? Leave to user
    if(!setFactoryCalibration(&calibration_data))
        return false;


    _isInit = true;

    return true;
}

bool QwDevTMF882X::getAppVersion(char * pVersion, uint8_t vlen){

    // verify we are in app mode
    if(tmf882x_get_mode(&_tof) != TMF882X_MODE_APP) // failed to open up into application mode....
        return false;

    if(!tmf882x_get_firmware_ver(&_tof, pVersion, vlen))
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////////

bool QwDevTMF882X::setCalibration(struct tmf882x_mode_app_calib *tof_calib){


    if(!_isInit)
        return false; 


    if(!tof_calib || !tof_calib->calib_len) 
        return false;

    if(tmf882x_ioctl(&_tof, IOCAPP_SET_CALIB, tof_calib, NULL))
        return false;

    return true;

}
//////////////////////////////////////////////////////////////////////////////
// startMeasuring()

bool QwDevTMF882X::startMeasuring(TMF882XMeasurement_t &results){

    if(!_isInit)
        return false;

    // Will start measuring, requesting only 1 measurment.

    if(!start_measuring(1))
        return false;

    if(!_lastMeasurment)
        return false;

    memcpy(&results, _lastMeasurment, sizeof(TMF882XMeasurement_t));

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// startMeasuring
//
// Take N number of measurements. The user should get the results via a callback
// that was set.
//
// If N is 0, this will r
bool QwDevTMF882X::startMeasuring(uint32_t reqMeasurments){

    if(!_isInit)
        return false;

    // Will start measuring, requesting only 1 measurment.

    if(!start_measuring(reqMeasurments))
        return false;

    return true;
}
//////////////////////////////////////////////////////////////////////////////
// stopMeasuring()

void QwDevTMF882X::stopMeasuring(void){
    _stopMeasuring=true;
}    
//////////////////////////////////////////////////////////////////////////////
// start_measurements()
//
// Take N number of measurements. If N is 0, run forever (until stop is called)

bool QwDevTMF882X::start_measuring(uint8_t reqMeasurements){

    if(!_isInit)
        return false;

    _stopMeasuring = false;

    _lastMeasurment = nullptr;
    _nMeasurements=0; // internal counter

    // if you want to measure forever, you need CB
    if(reqMeasurements == 0 && !_msgHandlerCB)
        return false;

    if(tmf882x_start(&_tof))
        return false;

    uint8_t _nTaken = 0;

    // Measurment loop
    do{

        // collecton/process pump for SDK
        if(tmf882x_process_irq(&_tof)) // something went wrong 
            break;

        if(_stopMeasuring) // caller set the stop flag
            break;

        // reached our limit/goal
        if(reqMeasurements && _nMeasurements == reqMeasurements)
            break;

        // yield 
        sfe_usleep(20000); // micro sec poll period (5 millis) TODO: needs better abstraction

    }while(true);

    tmf882x_stop(&_tof);

    return true;

}

//////////////////////////////////////////////////////////////////////////////
// sdk_msg_handler()
//
// "Internal" method used to process messages from the SDK

int32_t QwDevTMF882X::_sdk_msg_handler(struct tmf882x_msg *msg){

    if(!msg || !_isInit)
        return false;

    if(msg->hdr.msg_id == ID_MEAS_RESULTS){
        _nMeasurements++;
        _lastMeasurment = &msg->meas_result_msg;        

        if(_msgHandlerCB)
            _msgHandlerCB(_lastMeasurment);
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////////
// printMeasurement()

void QwDevTMF882X::printMeasurement(TMF882XMeasurement_t *meas){

    if(!meas)
        return;

    printf("result_num: %u num_results: %u\n", meas->result_num, meas->num_results);

    for (uint32_t i = 0; i < meas->num_results; ++i) {

        printf("conf: %u distance_mm: %u channel: %u sub_capture: %u\n",
               meas->results[i].confidence,
               meas->results[i].distance_mm,
               meas->results[i].channel,
               meas->results[i].sub_capture);
    }
    printf("photon: %u ref_photon: %u ALS: %u\n",
           meas->photon_count, meas->ref_photon_count, meas->ambient_light);

}

//////////////////////////////////////////////////////////////////////////////
// isConnected()
//
// Is the device connected to the i2c bus
//
// Return Value: false on not connected, true if it is connected 

bool QwDevTMF882X::isConnected()
{

    return _i2cBus->ping(_i2c_address);
}


//////////////////////////////////////////////////////////////////////////////
// setMeasurementHandler()
//
// Set a callback function that is called when a measurement is detected.

void QwDevTMF882X::setMeasurementHandler(TMF882XMeasurementHandler_t handler){

    if(handler)
        _msgHandlerCB = handler;

}
////////////////////////////////////////////////////////////////////////////////////
// set_comm_bus()
//
// Method to set the bus object that is used to communicate with the device
//
// TODO -  In the *future*, generalize to match SDK

void QwDevTMF882X::set_comm_bus(QwI2C &theBus, uint8_t id_bus){

    _i2cBus = &theBus;
    _i2c_address = id_bus;
}
//////////////////////////////////////////////////////////////////////////////
// I2C relays for the underlying SDK
int32_t QwDevTMF882X::writeRegisterRegion(uint8_t offset, uint8_t *data, uint16_t length){

    return _i2cBus->writeRegisterRegion(_i2c_address, offset, data, length);

}
    
int32_t QwDevTMF882X::readRegisterRegion(uint8_t offset, uint8_t* data, uint16_t length){

    return _i2cBus->readRegisterRegion(_i2c_address, offset, data, length);

}
