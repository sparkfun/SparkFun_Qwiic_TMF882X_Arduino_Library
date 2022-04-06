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

#define kTMF882XCalInterations 4000

//////////////////////////////////////////////////////////////////////////////
// initializeTMF882x()
//
// Called to initialize the TMF device.
//
// returns false if the init fails.

bool QwDevTMF882X::initializeTMF882x(void)
{
    // init the underlying SDK
    tmf882x_init(&m_TOF, this);

    // was debug mode set
    if (m_debug)
        tmf882x_set_debug(&m_TOF, true);

    // Open the driver
    if (tmf882x_open(&m_TOF)) {

        tof_err((void*)this, "ERROR - Unable to open the TMF882X");
        return false;
    }

    // Load the firmware image that is part of the TMF882X SDK
    if (!loadFirmware(tof_bin_image, tof_bin_image_length)) {

        // Fallback:
        //    Firmware upload failed. See if the device can move to app
        //    mode using the onboard image.
        if (tmf882x_mode_switch(&m_TOF, TMF882X_MODE_APP))
            return false;
    }

    // Make sure we are running application mode
    if (tmf882x_get_mode(&m_TOF) != TMF882X_MODE_APP) {

        tof_err((void*)this, "ERROR - The TMF882X failed to enter APP mode.");
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// loadFirmware()
//
bool QwDevTMF882X::loadFirmware(const unsigned char* firmwareBinImage, unsigned long length)
{

    if (!firmwareBinImage || !length)
        return false;

    // Do a mode switch to the bootloader (bootloader mode necessary for FWDL)
    if (tmf882x_mode_switch(&m_TOF, TMF882X_MODE_BOOTLOADER)) {
        tof_err((void*)this, "ERROR - Switch to TMF882X Switch to Bootloader failed");
        return false;
    }

    if (tmf882x_fwdl(&m_TOF, FWDL_TYPE_BIN, firmwareBinImage, length)) {
        tof_err((void*)this, "ERROR - Upload of firmware image failed");
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// setFactoryCalibration()

bool QwDevTMF882X::setFactoryCalibration(struct tmf882x_mode_app_calib* tof_calib)
{

    struct tmf882x_mode_app_config tofConfig;

    int32_t error;

    if (!tof_calib)
        return false;

    if (tmf882x_ioctl(&m_TOF, IOCAPP_GET_CFG, NULL, &tofConfig))
        return false;

    // Change the iterations for Factory Calibration
    tofConfig.kilo_iterations = kTMF882XCalInterations;

    if (tmf882x_ioctl(&m_TOF, IOCAPP_SET_CFG, &tofConfig, NULL))
        return false;

    if (tmf882x_ioctl(&m_TOF, IOCAPP_DO_FACCAL, NULL, tof_calib))
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

    if (m_isInitialized)
        return true;

    //  do we have a bus yet? is the device connected?
    if (!m_i2cBus || !m_i2cAddress || !m_i2cBus->ping(m_i2cAddress))
        return false;

    // init the application mode
    if (!initializeTMF882x())
        return false;

    // TODO - do this here? Leave to user
    if (!setFactoryCalibration(&calibration_data))
        return false;

    m_isInitialized = true;

    return true;
}

bool QwDevTMF882X::applicationVersion(char* pVersion, uint8_t vlen)
{

    // verify we are in app mode
    if (tmf882x_get_mode(&m_TOF) != TMF882X_MODE_APP) // failed to open up into application mode....
        return false;

    if (!tmf882x_get_firmware_ver(&m_TOF, pVersion, vlen))
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////////

bool QwDevTMF882X::setCalibration(struct tmf882x_mode_app_calib* tof_calib)
{

    if (!m_isInitialized)
        return false;

    if (!tof_calib || !tof_calib->calib_len)
        return false;

    if (tmf882x_ioctl(&m_TOF, IOCAPP_SET_CALIB, tof_calib, NULL))
        return false;

    return true;
}
//////////////////////////////////////////////////////////////////////////////
//  >> TO DO <<
//
//////////////////////////////////////////////////////////////////////////////
// startMeasuring()

int QwDevTMF882X::startMeasuring(TMF882XMeasurement_t& results, uint32_t timeout)
{

    if (!m_isInitialized)
        return -1;

    // Will start measuring, requesting only 1 measurement.

    if (!measurementLoop(1, timeout))
        return -1;

    if (!m_lastMeasurement) {
        memset(&results, 0, sizeof(TMF882XMeasurement_t));
        return -1;
    }

    memcpy(&results, m_lastMeasurement, sizeof(TMF882XMeasurement_t));

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

    if (!m_isInitialized)
        return -1;

    // Start the measurement loop - it returns the number of samples taken

    return measurementLoop(reqMeasurements, timeout);
}
//////////////////////////////////////////////////////////////////////////////
// stopMeasuring()

void QwDevTMF882X::stopMeasuring(void)
{
    m_stopMeasuring = true;
}
//////////////////////////////////////////////////////////////////////////////
// start_measurements()
//
// Take N number of measurements. If N is 0, run forever (until stop is called)

int QwDevTMF882X::measurementLoop(uint16_t reqMeasurements, uint32_t timeout)
{

    if (!m_isInitialized)
        return -1;

    // Setup for the measurement internval
    m_stopMeasuring = false;
    m_lastMeasurement = nullptr;
    m_nMeasurements = 0; // internal counter

    // if you want to measure forever, you need CB function, or a timeout set
    if (reqMeasurements == 0 && !(m_messageHandlerCB || timeout))
        return -1;

    if (tmf882x_start(&m_TOF))
        return -1;

    // Do we have a timeout on this?
    uint32_t startTime = 0;

    if (timeout)
        startTime = sfe_millis();

    // Measurment loop
    do {

        // collecton/process pump for SDK
        if (tmf882x_process_irq(&m_TOF)) // something went wrong
            break;

        if (m_stopMeasuring) // caller set the stop flag
            break;

        // reached our limit/goal
        if (reqMeasurements && m_nMeasurements == reqMeasurements)
            break;

        // if we have a timeout, check
        if (timeout && sfe_millis() - startTime >= timeout)
            break;

        // yield
        sfe_msleep(m_sampleDelayMS); // milli sec poll period

    } while (true);

    tmf882x_stop(&m_TOF);

    return m_nMeasurements;
}

//////////////////////////////////////////////////////////////////////////////
// sdk_msg_handler()
//
// "Internal" method used to process messages from the SDK

int32_t QwDevTMF882X::sdkMessageHandler(struct tmf882x_msg* msg)
{
    if (!msg || !m_isInitialized)
        return false;

    if (msg->hdr.msg_id == ID_MEAS_RESULTS) {
        m_nMeasurements++;
        m_lastMeasurement = &msg->meas_result_msg;

        if (m_messageHandlerCB)
            m_messageHandlerCB(m_lastMeasurement);
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
    return (m_i2cBus && m_i2cAddress ? m_i2cBus->ping(m_i2cAddress) : false);
}

//////////////////////////////////////////////////////////////////////////////
// setMeasurementHandler()
//
// Set a callback function that is called when a measurement is detected.

void QwDevTMF882X::setMeasurementHandler(TMF882XMeasurementHandler_t handler)
{
    if (handler)
        m_messageHandlerCB = handler;
}

////////////////////////////////////////////////////////////////////////////////////
// setTMF882XConfig()
//
// Sets the passed in configuration struct values in the device.
//
// Returns true on success, false on failure
bool QwDevTMF882X::getTMF882XConfig(struct tmf882x_mode_app_config& tofConfig)
{
    if (!m_isInitialized)
        return false;

    // Get the config struct from the underlying SDK
    if (tmf882x_ioctl(&m_TOF, IOCAPP_GET_CFG, NULL, &tofConfig))
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// setTMF882XConfig()
//
// Sets the passed in configuration struct values in the device.
//
// Returns true on success, false on failure

bool QwDevTMF882X::setTMF882XConfig(struct tmf882x_mode_app_config& tofConfig)
{
    if (!m_isInitialized)
        return false;

    // Set the config in the dvice
    if (tmf882x_ioctl(&m_TOF, IOCAPP_SET_CFG, &tofConfig, NULL))
        return false;
    return true;
}
////////////////////////////////////////////////////////////////////////////////////
// setCommBus()
//
// Method to set the bus object that is used to communicate with the device
//
// TODO -  In the *future*, generalize to match SDK

void QwDevTMF882X::setCommBus(QwI2C& theBus, uint8_t id_bus)
{
    m_i2cBus = &theBus;
    m_i2cAddress = id_bus;
}

//////////////////////////////////////////////////////////////////////////////
//
// I2C relay methods used to support the "shim" architecture of the AMS TMF882X
// C SDK.
//
int32_t QwDevTMF882X::writeRegisterRegion(uint8_t offset, uint8_t* data, uint16_t length)
{
    return m_i2cBus->writeRegisterRegion(m_i2cAddress, offset, data, length);
}

int32_t QwDevTMF882X::readRegisterRegion(uint8_t offset, uint8_t* data, uint16_t length)
{
    return m_i2cBus->readRegisterRegion(m_i2cAddress, offset, data, length);
}
