// qwiic_tmf882x.cpp
//
// This is a library written for SparkFun Qwiic TMF882X boards
//
// SparkFun sells these bpards at its website: www.sparkfun.com
//
// Do you like this library? Help support SparkFun. Buy a board!
//
//  SparkFun Qwiic dToF Imager - TMF8820        https://www.sparkfun.com/products/19036
//  SparkFun Qwiic Mini dToF Imager - TMF8820   https://www.sparkfun.com/products/19218
//  SparkFun Qwiic Mini dToF Imager - TMF8821   https://www.sparkfun.com/products/19451
//  SparkFun Qwiic dToF Imager - TMF8821        https://www.sparkfun.com/products/19037
//
// Written by Kirk Benell @ SparkFun Electronics, April 2022
//
// This library provides an abstract interface to the underlying TMF882X
// SDK that is provided by AMS.
//
// Repository:
//     https://github.com/sparkfun/SparkFun_Qwiic_TMF882X_Arduino_Library
//
//
// SparkFun code, firmware, and software is released under the MIT
// License(http://opensource.org/licenses/MIT).
//
// SPDX-License-Identifier: MIT
//
//    The MIT License (MIT)
//
//    Copyright (c) 2022 SparkFun Electronics
//    Permission is hereby granted, free of charge, to any person obtaining a
//    copy of this software and associated documentation files (the "Software"),
//    to deal in the Software without restriction, including without limitation
//    the rights to use, copy, modify, merge, publish, distribute, sublicense,
//    and/or sell copies of the Software, and to permit persons to whom the
//    Software is furnished to do so, subject to the following conditions: The
//    above copyright notice and this permission notice shall be included in all
//    copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
//    "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//    NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//    PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcu_tmf882x_config.h"
#include "qwiic_tmf882x.h"
#include "sfe_arduino.h"

#include "inc/tmf882x_host_interface.h"

//////////////////////////////////////////////////////////////////////////////
// initializeTMF882x()
//
// Private/internal method that is called to initialize the TMF device.
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

    // Load the firmware image that is part of the TMF882X SDK. Without
    // firware, the device won't work
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

///////////////////////////////////////////////////////////////////////
// loadFirmware()
//
// To operate the TMF882X device, runtime firmware must be loaded.
//
// This library loads a default firmware version on library initialization.
//
// This method allows the library user to set the firmware version on the
// device if a newer version is available from AMS
//
//  Parameter         Description
//  ---------         -----------------------------
//  firmwareBinImage  Array that contains the firmware binary image
//  length            The length of the firmware array
//  retval            true on success, false on failure

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

    // Load the fireware.
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

    if (_isInitialized)  // already init'd
        return true;

    //  do we have a bus yet? is the device connected?
    if (!_i2cBus || !_i2cAddress || !_i2cBus->ping(_i2cAddress))
        return false;

    // init the application mode
    if (!initializeTMF882x())
        return false;

    _isInitialized = true;

    return true;
}

///////////////////////////////////////////////////////////////////////
// isConnected()
//
// Called to determine if a TMF882X device, at the provided i2c address
// is connected.
//
//  Parameter   Description
//  ---------   -----------------------------
//  retval      true if device is connected, false if not connected

bool QwDevTMF882X::isConnected()
{
    return (_i2cBus && _i2cAddress ? _i2cBus->ping(_i2cAddress) : false);
}

///////////////////////////////////////////////////////////////////////
// setI2CAddress()
//
// Set/Change the address of the connected device.
//
// Called after the device has been initialized.
//
//  Parameter   Description
//  ---------   -----------------------------
//  address     The new address for the device
//  retval      true on success, false on failure

bool QwDevTMF882X::setI2CAddress(uint8_t address)
{
    // Initialized? Is the address legal?
    if (!_isInitialized || address < 0x08 || address > 0x77)
        return false;

    // is the address the same as already set?
    if (address == _i2cAddress)
        return true;

    // Okay, go time -- get the config
    struct tmf882x_mode_app_config tofConfig;

    // Get the config struct from the underlying SDK
    if (tmf882x_ioctl(&_TOF, IOCAPP_GET_CFG, NULL, &tofConfig))
        return false;

    // change the address in the config
    tofConfig.i2c_slave_addr = address;

    // set it.
    if (tmf882x_ioctl(&_TOF, IOCAPP_SET_CFG, &tofConfig, NULL))
        return false;

    // Now tell the device to switch to the address in the config page.
    uint8_t cmdCode = TMF8X2X_COM_CMD_STAT__cmd_stat__CMD_I2C_SLAVE_ADDRESS;
    if (_i2cBus->writeRegisterRegion(_i2cAddress, TMF8X2X_COM_CMD_STAT, &cmdCode, sizeof(uint8_t)))
        return false;

    // Potential TODO - check status register... .. need to test.

    // If we are here, the address should of been changed
    _i2cAddress = address;
    return true;
}

///////////////////////////////////////////////////////////////////////
// getApplicationVersion()
//
// Returns the version of the "Application" software running on the
// connected TMF882X device. See the TMF882X data sheet for more
// information regarding application software
//
//  Parameter   Description
//  ---------   -----------------------------
//  pVersion    Pointer to a character array to receive the version data
//  vlen        Length of the array pointed to be pVersion
//  retval      true on success, false on failure

bool QwDevTMF882X::getApplicationVersion(char *pVersion, uint8_t vlen)
{

    if (!_isInitialized)
        return false;

    // verify we are in app mode
    if (tmf882x_get_mode(&_TOF) != TMF882X_MODE_APP) // failed to open up into application mode....
        return false;

    if (!tmf882x_get_firmware_ver(&_TOF, pVersion, vlen))
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////
// getDeviceUniqueID()
//
// Returns the unique ID of the connected TMF882X.
//
// Note:  Uses a ID structure as defined by the AMS TMF882X SDK to
//        store the ID value
//
//  Parameter   Description
//  ---------   -----------------------------
//  uid         The TMF882X UID structure to store the ID into.
//  retval      true on success, false on failure

bool QwDevTMF882X::getDeviceUniqueID(struct tmf882x_mode_app_dev_UID &devUID)
{

    if (!_isInitialized)
        return false;

    // Get the UID from the device
    if (tmf882x_ioctl(&_TOF, IOCAPP_DEV_UID, NULL, &devUID))
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////
// setCalibration()
//
// Used to set the calibration data on the connected TMF882X device
//
//  Parameter    Description
//  ---------    -----------------------------
//  tofCalib     The calibration data to set
//  retval       True on success, false on error

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

///////////////////////////////////////////////////////////////////////
// getCalibration()
//
// Used to get the calibration data on the connected TMF882X device
//
//  Parameter    Description
//  ---------    -----------------------------
//  tofCalib     Struct to hold the calibration data.
//  retval       True on success, false on error

bool QwDevTMF882X::getCalibration(struct tmf882x_mode_app_calib &tofCalib)
{

    if (!_isInitialized)
        return false;

    if (tmf882x_ioctl(&_TOF, IOCAPP_GET_CALIB, NULL, &tofCalib))
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////
// factoryCalibration()
//
// Used to run a factory calibrary on the connected TMF882X device.
//
// The results of the calibration are returned in the pass in calibration
// structure.
//
// Consult the TMF882X datasheet for details on performing a factory
// calibration
//
//  Parameter    Description
//  ---------    -----------------------------
//  tofCalib     The results of the calibration process
//  retval       True on success, false on error

bool QwDevTMF882X::factoryCalibration(struct tmf882x_mode_app_calib &tofCalib)
{
    if (!_isInitialized)
        return false;

    // Perform the factory calibration -- this returns the calibration data
    if (tmf882x_ioctl(&_TOF, IOCAPP_DO_FACCAL, NULL, &tofCalib))
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////
// startMeasuring()
//
// Start measuring distance/data on the TMF882X device. This method
// returns after one measurement is performed.
//
// Measurement data is returned in the provided measurement struct.
//
//  Parameter         Description
//  ---------         -----------------------------
//  results           The results of the mesurement
//  timeout           The time, in milliseconds, to take measurements. A
//                    value of zero indicates no timeout set.
//  retval            The number of measurements taken (1), or -1 on error.

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

    // copy over results to output struct
    memcpy(&results, _lastMeasurement, sizeof(tmf882x_msg_meas_results));

    return 1;
}

///////////////////////////////////////////////////////////////////////
// startMeasuring()
//
// Start measuring distance/data on the TMF882X device. This method
// won't return until the measurement activity ends.
//
// Measurement data is passed to the library user via a callback function,
// which is set using one of the set<type>Handler() methods on this object.
//
// Measurements continue, until one of the following conditions occurs:
//
//  - The specified number of measurements took place (set via reqMeasurements)
//  - The specififed timeout value expires
//  - The stopMeasuring() method was called in a Handler function.
//
// This method won't start measuring if a measurement limit isn't set, a timeout
// isn't set and no callback handlers are not set.
//
// This method returns the number of measurements taken
//
//  Parameter         Description
//  ---------         -----------------------------
//  reqMeasurements   The number of measurements desired. A value of zero
//                    indicates no limit.
//  timeout           The time, in milliseconds, to take measurements. A
//                    value of zero indicates no timeout set.
//  retval            The number of measurements taken, or -1 on error

int QwDevTMF882X::startMeasuring(uint32_t reqMeasurements, uint32_t timeout)
{

    if (!_isInitialized)
        return -1;

    // Start the measurement loop - it returns the number of samples taken

    return measurementLoop(reqMeasurements, timeout);
}

///////////////////////////////////////////////////////////////////////
// stopMeasuring()
//
// Called to stop the device measuring loop. Normally called in a message
// handler function.
//
//  Parameter         Description
//  ---------         -----------------------------
//  None

void QwDevTMF882X::stopMeasuring(void)
{
    _stopMeasuring = true;
}

//////////////////////////////////////////////////////////////////////////////
// measurementLoop()
//
// Internal, private method. Peforms/manages the actual processing loop for the
// TMF882X.
//
// Looks for the following stop conditions:
//      - Reach the desired number of measurements
//      - Reach a timeout value
//      - A stop condition was set by calling stopMeasuring()
//
// Each measurement loop has a delay, set by the setSampleDelay() method.
//
// Note:
//     If reqMeasurements and timeout are both 0, a callback method must be set
//     so the loop can be stopped using stopMeasuring(). Otherwise, this could loop forever.
//
//  Parameter           Description
//  ---------           -----------------------------
//  reqMeasurements     The desired number of measurements. If 0, no limit
//  timeout             The timeout value for taking measurments. If 0, not limit
//  retval              The number of measurements taken

int QwDevTMF882X::measurementLoop(uint16_t reqMeasurements, uint32_t timeout)
{

    if (!_isInitialized)
        return -1;

    // Setup for the measurement internval
    _stopMeasuring = false;
    _lastMeasurement = nullptr;
    _nMeasurements = 0; // internal counter

    // if you want to measure forever, you need CB function, or a timeout set
    if (reqMeasurements == 0 && !(_measurementHandlerCB || _histogramHandlerCB || _messageHandlerCB || timeout))
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
//
// The methods in the sfe_shim call this, passing in the message structure from
// the SDK.
//
// This method processes the message and dispatches to registered callback functions

int32_t QwDevTMF882X::sdkMessageHandler(struct tmf882x_msg *msg)
{
    if (!msg || !_isInitialized)
        return false;

    // Do we have a general handler set
    if (_messageHandlerCB)
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

///////////////////////////////////////////////////////////////////////
// setMeasurementHandler()
//
// Data values from the TMF882X are sent from the AMS SDK using a
// callback/message pattern.
//
// Call this method with a function to call when measurement data is
// sent from the AMS sdk.
//
//  Parameter   Description
//  ---------   -----------------------------
//  handler     The function to call when measurement data is sent from the SDK

void QwDevTMF882X::setMeasurementHandler(TMF882XMeasurementHandler handler)
{
    if (handler)
        _measurementHandlerCB = handler;
}

///////////////////////////////////////////////////////////////////////
// setHistogramHandler()
//
// Data values from the TMF882X are sent from the AMS SDK using a
// callback/message pattern.
//
// Call this method with a function to call when histogram data is
// sent from the AMS sdk.
//
//  Parameter   Description
//  ---------   -----------------------------
//  handler     The function to call when histogram data is sent from the SDK

void QwDevTMF882X::setHistogramHandler(TMF882XHistogramHandler handler)
{
    if (handler)
        _histogramHandlerCB = handler;
}

///////////////////////////////////////////////////////////////////////
// setStatsHandler()
//
// Data values from the TMF882X are sent from the AMS SDK using a
// callback/message pattern.
//
// Call this method with a function to call when measurement stats data is
// sent from the AMS sdk.
//
//  Parameter   Description
//  ---------   -----------------------------
//  handler     The function to call when measurement stats data is sent from the SDK

void QwDevTMF882X::setStatsHandler(TMF882XStatsHandler handler)
{
    if (handler)
        _statsHandlerCB = handler;
}

///////////////////////////////////////////////////////////////////////
// setErrorHandler()
//
// Data values from the TMF882X are sent from the AMS SDK using a
// callback/message pattern.
//
// Call this method with a function to call when error information is
// sent from the AMS sdk.
//
//  Parameter   Description
//  ---------   -----------------------------
//  handler     The function to call when error information is sent from the SDK

void QwDevTMF882X::setErrorHandler(TMF882XErrorHandler handler)
{
    if (handler)
        _errorHandlerCB = handler;
}

///////////////////////////////////////////////////////////////////////
// setMessageHandler()
//
// Data values from the TMF882X are sent from the AMS SDK using a
// callback/message pattern.
//
// Call this methods with a function to call any message is sent from the
// AMS sdk. The function passed into this method can be used to handle all
// messages instead of using the other handlers for specific message types
//
//  Parameter   Description
//  ---------   -----------------------------
//  handler     The function to call when any message is sent from the SDK.

void QwDevTMF882X::setMessageHandler(TMF882XMessageHandler handler)
{
    if (handler)
        _messageHandlerCB = handler;
}

//////////////////////////////////////////////////////////////////////////////////
// getTMF882XConfig()
//
// Get the current configuration settings on the connected TMF882X
//
//  Parameter    Description
//  ---------    -----------------------------
//  tofConfig    Struct to hold the current configuration values.
//  retval       True on success, false on error

bool QwDevTMF882X::getTMF882XConfig(struct tmf882x_mode_app_config &tofConfig)
{
    if (!_isInitialized)
        return false;

    // Get the config struct from the underlying SDK
    if (tmf882x_ioctl(&_TOF, IOCAPP_GET_CFG, NULL, &tofConfig))
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////////////
// setTMF882XConfig()
//
// Set the current configuration settings on the connected TMF882X
//
//  Parameter    Description
//  ---------    -----------------------------
//  tofConfig    The config values to set on the TMF882X.
//  retval       True on success, false on error

bool QwDevTMF882X::setTMF882XConfig(struct tmf882x_mode_app_config &tofConfig)
{
    if (!_isInitialized)
        return false;

    // Set the config in the dvice
    if (tmf882x_ioctl(&_TOF, IOCAPP_SET_CFG, &tofConfig, NULL))
        return false;
    return true;
}

//////////////////////////////////////////////////////////////////////////////////
// getCurrentSPAD()
//
// Returns the ID of the current SPAD Map in use on the device.
//
// See the datatsheet for ID values
//
//  Parameter    Description
//  ---------    -----------------------------
//  retval       0 on error, > 0  on success - the SPAD id.

uint8_t QwDevTMF882X::getCurrentSPADMap(void)
{
    if (!_isInitialized)
        return 0;

    struct tmf882x_mode_app_config tofConfig;

    if (!getTMF882XConfig(tofConfig))
        return 0;

    return tofConfig.spad_map_id;
}

//////////////////////////////////////////////////////////////////////////////////
// setCurrentSPAD()
//
// Set the current SPAD MAP in use by the device.
//
// See the datatsheet for ID values
//
//  Parameter    Description
//  ---------    -----------------------------
//  idSPAD       The ID of the SPAD Map to use
//  retval       true on success, false on error

bool QwDevTMF882X::setCurrentSPADMap(uint8_t idSPAD)
{
    if (!_isInitialized)
        return false;

    // set map using the device config
    struct tmf882x_mode_app_config tofConfig;

    if (!getTMF882XConfig(tofConfig))
        return false;

    tofConfig.spad_map_id = idSPAD;

    return setTMF882XConfig(tofConfig);
}

//////////////////////////////////////////////////////////////////////////////////
// getSPADConfig()
//
// Get the current SPAD configuration on the connected TMF882X
//
// See the TMF882X datasheet to fully understand the concept of SPAD
//
//  Parameter    Description
//  ---------    -----------------------------
//  tofSpad      Struct to hold the current SPAD configuration values.
//  retval       True on success, false on error

bool QwDevTMF882X::getSPADConfig(struct tmf882x_mode_app_spad_config &spadConfig)
{
    if (!_isInitialized)
        return false;

    // Get the config struct from the underlying SDK
    if (tmf882x_ioctl(&_TOF, IOCAPP_GET_SPADCFG, NULL, &spadConfig))
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////////////
// setSPADConfig()
//
// Set the current SPAD configuration on the connected TMF882X. Used to set
// a custom SPAD.
//
// To set a pre-defined SPAD, use the spad filed in the TMF882X Config structure.
//
// See the TMF882X datasheet to fully understand the concept of SPAD
//
//  Parameter    Description
//  ---------    -----------------------------
//  tofSpad      The config values for the on device SPAD settings
//  retval       True on success, false on error

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
// setCommunicationBus()
//
// Method to set the bus object that is used to communicate with the device
//
//  Parameter    Description
//  ---------    -----------------------------
//  theBus       The communication bus object
//  idBus        The id/address of the device on the bus

void QwDevTMF882X::setCommunicationBus(sfe_TMF882X::QwI2C &theBus, uint8_t idBus)
{
    _i2cBus = &theBus;
    _i2cAddress = idBus;
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
