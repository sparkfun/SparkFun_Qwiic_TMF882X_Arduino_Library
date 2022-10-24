
// qwiic_tmf882x.h
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
//    https://github.com/sparkfun/SparkFun_Qwiic_TMF882X_Arduino_Library
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

#pragma once

// The AMS supplied library/sdk interface
#include "inc/tmf882x.h"
#include "tmf882x_interface.h"

#include "qwiic_i2c.h"

// Default I2C address for the device
#define kDefaultTMF882XAddress 0x41

#define kDefaultSampleDelayMS 500

// Flags for enable/disable output messages from the underlying SDK

#define TMF882X_MSG_INFO 0x01
#define TMF882X_MSG_DEBUG 0x02
#define TMF882X_MSG_ERROR 0x04
#define TMF882X_MSG_ALL 0x07
#define TMF882X_MSG_NONE 0x00

//////////////////////////////////////////////////////////////////////////////
// Messaage Callback Types
//
// The underlying SDK passes information back to the callee using a callback
// message handler pattern. There are four types of messages:
//
//  - Measurement results
//  - Device statistics
//  - Histogram results
//  - Error messages
//
// This library has methods that allow the user to set callback functions for
// each of the above message types, as well as a general message handler.
//
// The spcific message callbacks are helpful/easier to understand. The general
// callback handler function requires the user to impelment logic to determine
// the message type and take actions.
//
// For each of the callbacks functions, we define a type
//
// Measurement Handler Type
typedef void (*TMF882XMeasurementHandler)(struct tmf882x_msg_meas_results *);

// Histogram Handler type
typedef void (*TMF882XHistogramHandler)(struct tmf882x_msg_histogram *);

// Stats handler
typedef void (*TMF882XStatsHandler)(struct tmf882x_msg_meas_stats *);

// Error handler
typedef void (*TMF882XErrorHandler)(struct tmf882x_msg_error *);

// General Message Handler
typedef void (*TMF882XMessageHandler)(struct tmf882x_msg *);

class QwDevTMF882X
{

  public:
    // Default noop constructor
    QwDevTMF882X()
        : _isInitialized{false}, _sampleDelayMS{kDefaultSampleDelayMS}, _outputSettings{TMF882X_MSG_NONE},
          _debug{false}, _measurementHandlerCB{nullptr}, _histogramHandlerCB{nullptr}, _statsHandlerCB{nullptr},
          _errorHandlerCB{nullptr}, _messageHandlerCB{nullptr}, _i2cBus{nullptr}, _i2cAddress{0} {};

    ///////////////////////////////////////////////////////////////////////
    // init()
    //
    // Called to init the system. Connects to the device and sets it up for 
    // operation

    bool init();

    ///////////////////////////////////////////////////////////////////////
    // isConnected()
    //
    // Called to determine if a TMF882X device, at the provided i2c address
    // is connected.
    //
    //  Parameter   Description
    //  ---------   -----------------------------
    //  retval      true if device is connected, false if not connected

    bool isConnected(); // Checks if sensor ack's the I2C request

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

    bool setI2CAddress(uint8_t address);

    ///////////////////////////////////////////////////////////////////////
    // getI2CAddress()
    //
    // Returns the i2c address for the connected device
    //
    // Called after the device has been initialized.
    //
    //  Parameter   Description
    //  ---------   -----------------------------
    //  retval      The device i2c address. 0 if the device isn't conntected

    uint8_t getI2CAddress(void)
    {
        return _i2cAddress;
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
    //  pVersion    Pointer to a character array to recieve the version data
    //  vlen        Length of the array pointed to be pVersion
    //  retval      true on success, false on failure

    bool getApplicationVersion(char *pVersion, uint8_t vlen);

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

    bool getDeviceUniqueID(struct tmf882x_mode_app_dev_UID &uid);

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

    bool loadFirmware(const unsigned char *firmwareBinImage, unsigned long length);

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

    void setMeasurementHandler(TMF882XMeasurementHandler handler);

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

    void setHistogramHandler(TMF882XHistogramHandler handler);

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

    void setStatsHandler(TMF882XStatsHandler handler);

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

    void setErrorHandler(TMF882XErrorHandler handler);

    ///////////////////////////////////////////////////////////////////////
    // setMessageHandler()
    //
    // Data values from the TMF882X are sent from the AMS SDK using a
    // callback/message pattern.
    //
    // Call this method with a function to call any message is sent from the
    // AMS sdk. The function passed into this method can be used to handle all
    // messages instead of using the other handlers for specific message types
    //
    //  Parameter   Description
    //  ---------   -----------------------------
    //  handler     The function to call when any message is sent from the SDK.

    void setMessageHandler(TMF882XMessageHandler handler);

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
    //  - The specified timeout value expires
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

    int startMeasuring(uint32_t reqMeasurements = 0, uint32_t timeout = 0);

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

    int startMeasuring(struct tmf882x_msg_meas_results &results, uint32_t timeout = 0);

    ///////////////////////////////////////////////////////////////////////
    // stopMeasuring()
    //
    // Called to stop the device measuring loop. Normally called in a message
    // handler function.
    //
    //
    //  Parameter         Description
    //  ---------         -----------------------------
    //  None

    void stopMeasuring(void);

    ///////////////////////////////////////////////////////////////////////
    // factoryCalibration()
    //
    // Used to run a factory calibration on the connected TMF882X device.
    //
    // The results of the calibration are returned in the passed in calibration
    // structure.
    //
    // Consult the TMF882X datasheet for details on performing a factory
    // calibration
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  tofCalib     The results of the calibration process
    //  retval       True on success, false on error

    bool factoryCalibration(struct tmf882x_mode_app_calib &tofCalib);

    ///////////////////////////////////////////////////////////////////////
    // setCalibration()
    //
    // Used to set the calibration data on the connected TMF882X device
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  tofCalib     The calibration data to set
    //  retval       True on success, false on error

    bool setCalibration(struct tmf882x_mode_app_calib &tofCalib);

    ///////////////////////////////////////////////////////////////////////
    // getCalibration()
    //
    // Used to get the calibration data on the connected TMF882X device
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  tofCalib     Struct to hold the calibration data.
    //  retval       True on success, false on error

    bool getCalibration(struct tmf882x_mode_app_calib &tofCalib);

    //////////////////////////////////////////////////////////////////////////////////
    // setSampleDelay()
    //
    // Set the delay used in the libraries sample loop used when processing
    // samples/reading from the device.
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  delay        The delay to use, in milli-seconds

    void setSampleDelay(uint16_t delay)
    {
        if (delay)
            _sampleDelayMS = delay;
    };

    //////////////////////////////////////////////////////////////////////////////////
    // getSampleDelay()
    //
    // The current value of the library processing loop delay. The value is in
    // milliseconds.
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  retval       The current delay value - in milli-seconds

    uint16_t getSampleDelay(void)
    {
        return _sampleDelayMS;
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

    bool getTMF882XConfig(struct tmf882x_mode_app_config &tofConfig);

    //////////////////////////////////////////////////////////////////////////////////
    // setTMF882XConfig()
    //
    // Set the current configuration settings on the connected TMF882X
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  tofConfig    The config values to set on the TMF882X.
    //  retval       True on success, false on error

    bool setTMF882XConfig(struct tmf882x_mode_app_config &tofConfig);

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

    uint8_t getCurrentSPADMap(void);

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

    bool setCurrentSPADMap(uint8_t idSPAD);

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

    bool getSPADConfig(struct tmf882x_mode_app_spad_config &tofSpad);

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

    bool setSPADConfig(struct tmf882x_mode_app_spad_config &tofSpad);

    //////////////////////////////////////////////////////////////////////////////////
    // getTMF882XContext()
    //
    // Returns the context structure used by this library when accessing the
    // underlying TMF882X SDK.
    //
    // With this structure,  users of this library can make direct calls to
    // the interface functions of the TMF882X SDK.
    //
    // Note:
    //  Calling the TMF882X SDK functions directly could impact the operation of
    //  this library. Use this option with caution.
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  retval       The TMF882X Context used by this library.

    tmf882x_tof &getTMF882XContext(void)
    {
        return _TOF;
    }

    //////////////////////////////////////////////////////////////////////////////////
    // setDebug()
    //
    // Set the debug state fo the SDK. To use the full debug capabilities of the SDK,
    // debug should be enabled before calling init/begin() on the library
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  bEnable      To enable or disable debug mode in the SDK

    void setDebug(bool bEnable)
    {
        _debug = bEnable;

        if (_debug)
            _outputSettings |= TMF882X_MSG_DEBUG;
        else
            _outputSettings &= ~TMF882X_MSG_DEBUG;
    }

    //////////////////////////////////////////////////////////////////////////////////
    // getDebug()
    //
    // Returns the current debug setting of the library
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  retval       True if in Debug mode, false if not.

    bool getDebug(void)
    {
        return _debug;
    }

    //////////////////////////////////////////////////////////////////////////////////
    // setInfoMessages()
    //
    // Enable/Disable the output of info messages from the AMS SDK.
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  bEnable      To enable or disable info messages in the SDK

    void setInfoMessages(bool bEnable)
    {
        if (bEnable)
            _outputSettings |= TMF882X_MSG_INFO;
        else
            _outputSettings &= ~TMF882X_MSG_INFO;
    }

    //////////////////////////////////////////////////////////////////////////////////
    // setMessageLevel()
    //
    // Used to set the message level of the system.
    //
    // The value passed in should be one, or a combination of the following
    // flags.
    //
    //    TMF882X_MSG_INFO      - Output Info messages
    //    TMF882X_MSG_DEBUG     - Output Debug messages
    //    TMF882X_MSG_ERROR     - Output Error messages
    //    TMF882X_MSG_ALL       - Output all messages
    //    TMF882X_MSG_NONE      - Disable all message output
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  msg          Message type flag(s)

    void setMessageLevel(uint8_t msg)
    {
        _outputSettings = msg;
    }

    //////////////////////////////////////////////////////////////////////////////////
    // getMessageLevel()
    //
    // Return the current message settings. See setMessageLevel() description for
    // possible values
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  retval       The current message level settings

    uint8_t getMessageLevel(void)
    {
        return _outputSettings;
    }

    //////////////////////////////////////////////////////////////////////////////////
    // Methods that are called from our "shim relay". They are public so the SDk/SHIM
    // functions can call into the library.
    //////////////////////////////////////////////////////////////////////////////////
    // sdkMessageHandler()
    //
    // Called from the SDK sfe_shim implementation to relay messages from the SDK
    // to this library.
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  msg          The message from the SDK
    //  retval       0 on success, -1 on error

    int32_t sdkMessageHandler(struct tmf882x_msg *msg);

    //////////////////////////////////////////////////////////////////////////////////
    // writeRegisterRegion()
    //
    // Called from the SDK sfe_shim implementation to write data on the I2C bus
    // to the specified register on the device
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  reg          register to write to
    //  data         Data to write
    //  length       Length of the data to write
    //  retval       -1 = error, 0 = success

    int32_t writeRegisterRegion(uint8_t reg, uint8_t *data, uint16_t length);

    //////////////////////////////////////////////////////////////////////////////////
    // readRegisterRegion()
    //
    // Called from the SDK sfe_shim implementation to read data
    // from the specified register on the device
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  reg          register to read from
    //  data         Array to store data in
    //  length       Length of the data to read
    //  retval       -1 = error, 0 = success

    int32_t readRegisterRegion(uint8_t reg, uint8_t *data, uint16_t length);

    //////////////////////////////////////////////////////////////////////////////////
    // setCommunicationBus()
    //
    // Called to set the Communication Bus object to use
    //
    //  Parameter    Description
    //  ---------    -----------------------------
    //  theBus       The Bus object to use
    //  idBus        The bus ID for the target device.
    //

    void setCommunicationBus(sfe_TMF882X::QwI2C &theBus, uint8_t idBus);

  private:
    // The internal method to initialize the device
    bool initializeTMF882x(void);

    // The actual measurment loop method
    int measurementLoop(uint16_t nMeasurements, uint32_t timeout);

    // Library initialized flag
    bool _isInitialized;

    // Delay for the read sample loop
    uint16_t _sampleDelayMS;

    // for managing message output levels
    uint8_t _outputSettings;
    bool _debug;

    // Callback function pointers
    TMF882XMeasurementHandler _measurementHandlerCB;
    TMF882XHistogramHandler _histogramHandlerCB;
    TMF882XStatsHandler _statsHandlerCB;
    TMF882XErrorHandler _errorHandlerCB;
    TMF882XMessageHandler _messageHandlerCB;

    // I2C  things
    sfe_TMF882X::QwI2C *_i2cBus;      // pointer to our i2c bus object
    uint8_t _i2cAddress; // address of the device

    // Structure/state for the underlying TOF SDK
    tmf882x_tof _TOF;

    // for processing messages from SDK
    uint16_t _nMeasurements;

    // Cache of our last measurement taken
    struct tmf882x_msg_meas_results *_lastMeasurement;

    // Flag to indicate to the system to stop measurements
    bool _stopMeasuring;

};
