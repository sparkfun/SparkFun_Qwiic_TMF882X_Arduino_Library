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
#include "inc/sfe_arduino_c.h"


#define kTMF882XCalInterations 4000
//////////////////////////////////////////////////////////////////////////////
// init_tmf882x()
//
// Called to initialize the TMF device. 
//
// returns false if the init failes.

bool QwDevTMF882X::init_tmf882x(void){


    // call SDK routines for initial init. Note: for callback pointer, we give it this object. 

    tmf882x_init(&_tof, (void*)this);

    // open the driver
    if(tmf882x_open(&_tof)) // error
        return false; 

    // switch device to application mode. 
    if(tmf882x_mode_switch(&_tof, TMF882X_MODE_APP))  // failed to enter app mode
        return false; 

    // now verify we are running in app mode
    if(tmf882x_get_mode(&_tof) != TMF882X_MODE_APP) // failed to open up into application mode....
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// init()
//
// Init the system
//
// Return Value: false on error, true on success
//

bool QwDevTMF882X::init(void){
    

    if(_isInit)
        return true;

    //  do we have a bus yet? is the device connected?
    if(!_i2cBus || !_i2c_address || !_i2cBus->ping(_i2c_address))
        return false;

    // crank up the tmf
    if(!init_tmf882x())
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

bool QwDevTMF882X::setFactoryCalibration(struct tmf882x_mode_app_calib *tof_calib){


    if(!_isInit)
        return false; 

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

// >>>> TODO <<<<: Add a timeout parameter?

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
    if(_nMeasurements == 0 && !_msgHandlerCB)
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
// stopMeasuring()

void QwDevTMF882X::stopMeasuring(void){
    _stopMeasuring=true;
}    
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// sdk_msg_handler()
//
// "Internal" method used to process messages from the SDK

bool QwDevTMF882X::_sdk_msg_handler(struct tmf882x_msg *msg){

    if(!msg || !_isInit)
        return false;

    if(msg->hdr.msg_id == ID_MEAS_RESULTS){
        _nMeasurements++;
        _lastMeasurment = &msg->meas_result_msg;        

        if(_msgHandlerCB)
            _msgHandlerCB(_lastMeasurment);
    }

    return true;
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

bool QwDevTMF882X::isConnected(void){

    if(!_isInit)
        return false;

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
// I2C relays for the underlying SDK
int32_t QwDevTMF882X::writeRegisterRegion(uint8_t offset, uint8_t *data, uint16_t length){

    if(!_isInit)
        return -1;

    return _i2cBus->writeRegisterRegion(_i2c_address, offset, data, length);

}
    
int32_t QwDevTMF882X::readRegisterRegion(uint8_t offset, uint8_t* data, uint16_t length){


    if(!_isInit)
        return -1;

    return _i2cBus->readRegisterRegion(_i2c_address, offset, data, length);

}
