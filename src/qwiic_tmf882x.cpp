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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcu_tmf882x_config.h"
#include "qwiic_tmf882x.h"
#include "sfe_arduino_c.h"
#include "tof_factory_cal.h"

#include "inc/tmf882x_host_interface.h"

//////////////////////////////////////////////////////////////////////////////
// initializeTMF882x()
//
// Called to initialize the TMF device.
//
// returns false if the init fails.

bool QwDevTMF882X::initializeTMF882x(void)
{
    // init the underlying SDK
    tmf882x_init(&_TOF, this);

    // was debug mode set
    if (_debug)
        tmf882x_set_debug(&_TOF, true);

    // Open the driver
    if (tmf882x_open(&_TOF))
    {

        tof_err((void *)this, "ERROR - Unable to open the TMF882X");
        return false;
    }

    // Load the firmware image that is part of the TMF882X SDK
    if (!loadFirmware(tof_bin_image, tof_bin_image_length))
    {

        // Fallback:
        //    Firmware upload failed. See if the device can move to app
        //    mode using the onboard image.
        if (tmf882x_mode_switch(&_TOF, TMF882X_MODE_APP))
            return false;
    }

    // Make sure we are running application mode
    if (tmf882x_get_mode(&_TOF) != TMF882X_MODE_APP)
    {

        tof_err((void *)this, "ERROR - The TMF882X failed to enter APP mode.");
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// loadFirmware()
//
bool QwDevTMF882X::loadFirmware(const unsigned char *firmwareBinImage, unsigned long length)
{

    if (!firmwareBinImage || !length)
        return false;

    // Do a mode switch to the bootloader (bootloader mode necessary for FWDL)
    if (tmf882x_mode_switch(&_TOF, TMF882X_MODE_BOOTLOADER))
    {
        tof_err((void *)this, "ERROR - Switch to TMF882X Switch to Bootloader failed");
        return false;
    }

    if (tmf882x_fwdl(&_TOF, FWDL_TYPE_BIN, firmwareBinImage, length))
    {
        tof_err((void *)this, "ERROR - Upload of firmware image failed");
        return false;
    }

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

    if (_isInitialized)
        return true;

    //  do we have a bus yet? is the device connected?
    if (!_i2cBus || !_i2cAddress || !_i2cBus->ping(_i2cAddress))
        return false;

    // init the application mode
    if (!initializeTMF882x())
        return false;

    // Set Calibration data? TODO

    _isInitialized = true;

    return true;
}
//////////////////////////////////////////////////////////////////////////////
//
bool QwDevTMF882X::getApplicationVersion(char *pVersion, uint8_t vlen)
{

    // verify we are in app mode
    if (tmf882x_get_mode(&_TOF) != TMF882X_MODE_APP) // failed to open up into application mode....
        return false;

    if (!tmf882x_get_firmware_ver(&_TOF, pVersion, vlen))
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// getDeviceUniqueID()
//
// Return the devices unique ID

bool QwDevTMF882X::getDeviceUniqueID(struct tmf882x_mode_app_dev_UID &devUID)
{

    if (!_isInitialized)
        return false;

    // Get the UID from the device
    if (tmf882x_ioctl(&_TOF, IOCAPP_DEV_UID, NULL, &devUID))
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////////

bool QwDevTMF882X::setCalibration(struct tmf882x_mode_app_calib &tofCalib)
{

    if (!_isInitialized)
        return false;

    if (!tofCalib.calib_len)
        return false;

    if (tmf882x_ioctl(&_TOF, IOCAPP_SET_CALIB, &tofCalib, NULL))
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////////

bool QwDevTMF882X::getCalibration(struct tmf882x_mode_app_calib &tofCalib)
{

    if (!_isInitialized)
        return false;

    if (tmf882x_ioctl(&_TOF, IOCAPP_GET_CALIB, NULL, &tofCalib))
        return false;

    return true;
}
//////////////////////////////////////////////////////////////////////////////
// factoryCalibration()

bool QwDevTMF882X::factoryCalibration(struct tmf882x_mode_app_calib &tofCalib)
{

    // Perform the factory calibration -- this returns the calibration data
    if (tmf882x_ioctl(&_TOF, IOCAPP_DO_FACCAL, NULL, &tofCalib))
        return false;

    return true;
}
//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
// startMeasuring()

int QwDevTMF882X::startMeasuring(struct tmf882x_msg_meas_results &results, uint32_t timeout)
{

    if (!_isInitialized)
        return -1;

    // Will start measuring, requesting only 1 measurement.

    if (!measurementLoop(1, timeout))
        return -1;

    if (!_lastMeasurement)
    {
        memset(&results, 0, sizeof(tmf882x_msg_meas_results));
        return -1;
    }

    memcpy(&results, _lastMeasurement, sizeof(tmf882x_msg_meas_results));

    return 1;
}

//////////////////////////////////////////////////////////////////////////////
// startMeasuring
//
// Take N number of measurements. The user should get the results via a callback
// that was set.
//
// If N is 0, this will r
int QwDevTMF882X::startMeasuring(uint32_t reqMeasurements, uint32_t timeout)
{

    if (!_isInitialized)
        return -1;

    // Start the measurement loop - it returns the number of samples taken

    return measurementLoop(reqMeasurements, timeout);
}
//////////////////////////////////////////////////////////////////////////////
// stopMeasuring()

void QwDevTMF882X::stopMeasuring(void)
{
    _stopMeasuring = true;
}
//////////////////////////////////////////////////////////////////////////////
// start_measurements()
//
// Take N number of measurements. If N is 0, run forever (until stop is called)

int QwDevTMF882X::measurementLoop(uint16_t reqMeasurements, uint32_t timeout)
{

    if (!_isInitialized)
        return -1;

    // Setup for the measurement internval
    _stopMeasuring = false;
    _lastMeasurement = nullptr;
    _nMeasurements = 0; // internal counter

    // if you want to measure forever, you need CB function, or a timeout set
    if (reqMeasurements == 0 && !(_measurementHandlerCB || timeout))
        return -1;

    if (tmf882x_start(&_TOF))
        return -1;

    // Do we have a timeout on this?
    uint32_t startTime = 0;

    if (timeout)
        startTime = sfe_millis();

    // Measurment loop
    do
    {

        // data collection/process pump for SDK
        if (tmf882x_process_irq(&_TOF)) // something went wrong
            break;

        if (_stopMeasuring) // caller set the stop flag
            break;

        // reached our limit/goal
        if (reqMeasurements && _nMeasurements == reqMeasurements)
            break;

        // if we have a timeout, check
        if (timeout && sfe_millis() - startTime >= timeout)
            break;

        // yield
        sfe_msleep(_sampleDelayMS); // milli sec poll period

    } while (true);

    tmf882x_stop(&_TOF);

    return _nMeasurements;
}

//////////////////////////////////////////////////////////////////////////////
// sdk_msg_handler()
//
// "Internal" method used to process messages from the SDK

int32_t QwDevTMF882X::sdkMessageHandler(struct tmf882x_msg *msg)
{
    if (!msg || !_isInitialized)
        return false;

    // Do we have a general handler set
    if(_messageHandlerCB)
        _messageHandlerCB(msg);

    // Check the message type - call type handler methods if we have one
    switch (msg->hdr.msg_id)
    {

    case ID_MEAS_RESULTS:

        _nMeasurements++;
        _lastMeasurement = &msg->meas_result_msg;

        if (_measurementHandlerCB)
            _measurementHandlerCB(_lastMeasurement);
        break;

    case ID_HISTOGRAM:

        if (_histogramHandlerCB)
            _histogramHandlerCB(&msg->hist_msg);
        break;

    case ID_MEAS_STATS:

        if (_statsHandlerCB)
            _statsHandlerCB(&msg->meas_stat_msg);
        break;
    
    case ID_ERROR:

        if (_errorHandlerCB)
            _errorHandlerCB(&msg->err_msg);
        break;    

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// isConnected()
//
// Is the device connected to the i2c bus
//
// Return Value: false on not connected, true if it is connected

bool QwDevTMF882X::isConnected()
{
    return (_i2cBus && _i2cAddress ? _i2cBus->ping(_i2cAddress) : false);
}

//////////////////////////////////////////////////////////////////////////////
// setMeasurementHandler()
//
// Set a callback function that is called when a measurement is detected.

void QwDevTMF882X::setMeasurementHandler(TMF882XMeasurementHandler handler)
{
    if (handler)
        _measurementHandlerCB = handler;
}

//////////////////////////////////////////////////////////////////////////////
// setHistogramHandler()
//
// Set a callback function that is called when a histogram is detected.

void QwDevTMF882X::setHistogramHandler(TMF882XHistogramHandler handler)
{
    if (handler)
        _histogramHandlerCB = handler;
}

//////////////////////////////////////////////////////////////////////////////
// setStatsHandler()
//
// Set a callback function that is called when a measurement statistics message
// is detected

void QwDevTMF882X::setStatsHandler(TMF882XStatsHandler handler)
{
    if (handler)
        _statsHandlerCB = handler;
}

//////////////////////////////////////////////////////////////////////////////
// setErrorHandler()
//
// Set a callback function that is called when a error message is detected.

void QwDevTMF882X::setErrorHandler(TMF882XErrorHandler handler)
{
    if (handler)
        _errorHandlerCB = handler;
}

//////////////////////////////////////////////////////////////////////////////
// setMessageHandler()
//
// Set a callback function that is called when any message is sent.

void QwDevTMF882X::setMessageHandler(TMF882XMessageHandler handler)
{
    if (handler)
        _messageHandlerCB = handler;
}

////////////////////////////////////////////////////////////////////////////////////
// getTMF882XConfig()
//
// Fills in the passed in config struct with the configuration of the connected
// TMF882X device
//
// Returns true on success, false on failure

bool QwDevTMF882X::getTMF882XConfig(struct tmf882x_mode_app_config &tofConfig)
{
    if (!_isInitialized)
        return false;

    // Get the config struct from the underlying SDK
    if (tmf882x_ioctl(&_TOF, IOCAPP_GET_CFG, NULL, &tofConfig))
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// setTMF882XConfig()
//
// Sets the passed in configuration struct values in the device.
//
// Returns true on success, false on failure

bool QwDevTMF882X::setTMF882XConfig(struct tmf882x_mode_app_config &tofConfig)
{
    if (!_isInitialized)
        return false;

    // Set the config in the dvice
    if (tmf882x_ioctl(&_TOF, IOCAPP_SET_CFG, &tofConfig, NULL))
        return false;
    return true;
}
////////////////////////////////////////////////////////////////////////////////////
// getSPADConfig()
//
// Fills in the passed in SPAD config struct with the SPAD configuration of
// the connected TMF882X device
//
// Returns true on success, false on failure

bool QwDevTMF882X::getSPADConfig(struct tmf882x_mode_app_spad_config &spadConfig)
{
    if (!_isInitialized)
        return false;

    // Get the config struct from the underlying SDK
    if (tmf882x_ioctl(&_TOF, IOCAPP_GET_SPADCFG, NULL, &spadConfig))
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// setSPADConfig()
//
// Sets the passed in SPAD configuration struct values in the device.
//
// Returns true on success, false on failure

bool QwDevTMF882X::setSPADConfig(struct tmf882x_mode_app_spad_config &spadConfig)
{
    if (!_isInitialized)
        return false;

    // Set the config in the dvice
    if (tmf882x_ioctl(&_TOF, IOCAPP_SET_SPADCFG, &spadConfig, NULL))
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// setCommBus()
//
// Method to set the bus object that is used to communicate with the device
//
// TODO -  In the *future*, generalize to match SDK

void QwDevTMF882X::setCommBus(QwI2C &theBus, uint8_t id_bus)
{
    _i2cBus = &theBus;
    _i2cAddress = id_bus;
}

//////////////////////////////////////////////////////////////////////////////
//
// I2C relay methods used to support the "shim" architecture of the AMS TMF882X
// C SDK.
//
int32_t QwDevTMF882X::writeRegisterRegion(uint8_t offset, uint8_t *data, uint16_t length)
{
    return _i2cBus->writeRegisterRegion(_i2cAddress, offset, data, length);
}

int32_t QwDevTMF882X::readRegisterRegion(uint8_t offset, uint8_t *data, uint16_t length)
{
    return _i2cBus->readRegisterRegion(_i2cAddress, offset, data, length);
}
