
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
        : _isInitialized{false}, _sampleDelayMS{kDefaultSampleDelayMS},
          _outputSettings{TMF882X_MSG_NONE}, _debug{false}, _measurementHandlerCB{nullptr},
          _histogramHandlerCB{nullptr}, _statsHandlerCB{nullptr}, _errorHandlerCB{nullptr},
          _messageHandlerCB{nullptr}, _i2cBus{nullptr}, _i2cAddress{0} {};

    bool init();
    bool isConnected(); // Checks if sensor ack's the I2C request

    bool getApplicationVersion(char *pVersion, uint8_t vlen);

    bool getDeviceUniqueID(struct tmf882x_mode_app_dev_UID &);

    bool loadFirmware(const unsigned char *firmwareBinImage, unsigned long length);

    void setMeasurementHandler(TMF882XMeasurementHandler handler);
    void setHistogramHandler(TMF882XHistogramHandler handler);
    void setStatsHandler(TMF882XStatsHandler handler);
    void setErrorHandler(TMF882XErrorHandler handler);
    void setMessageHandler(TMF882XMessageHandler handler);            

    //////////////////////////////////////////////////////////////////////////////////
    //
    int startMeasuring(uint32_t reqMeasurements = 0, uint32_t timeout = 0);
    int startMeasuring(struct tmf882x_msg_meas_results &results, uint32_t timeout = 0);

    //////////////////////////////////////////////////////////////////////////////////
    //
    void stopMeasuring(void);

    //////////////////////////////////////////////////////////////////////////////////
    //
    bool factoryCalibration(struct tmf882x_mode_app_calib &tof_calib);

    //////////////////////////////////////////////////////////////////////////////////
    //

    bool setCalibration(struct tmf882x_mode_app_calib &tof_calib);

    bool getCalibration(struct tmf882x_mode_app_calib &tof_calib);

    //////////////////////////////////////////////////////////////////////////////////
    // setSampleDelay()
    //
    // Set the delay used in the libraries sample loop used when processing
    // samples/reading from the device.
    //
    // The value is in milli-seconds
    void setSampleDelay(uint16_t delay)
    {
        if (delay)
            _sampleDelayMS = delay;
    };

    //////////////////////////////////////////////////////////////////////////////////
    // sampleDelay()
    //
    // The current value of the library processing loop delay. The value is in
    //  milliseconds.

    uint16_t getSampleDelay(void)
    {
        return _sampleDelayMS;
    }

    //////////////////////////////////////////////////////////////////////////////////
    // TMF882X Configuration
    //
    // Methods to access and set the underlying TMF882X configuration structure

    bool getTMF882XConfig(struct tmf882x_mode_app_config &);

    bool setTMF882XConfig(struct tmf882x_mode_app_config &);

    //////////////////////////////////////////////////////////////////////////////////
    // TMF882X SPAD methods - used to get and set SPAD config values
    //

    bool getSPADConfig(struct tmf882x_mode_app_spad_config &);

    bool setSPADConfig(struct tmf882x_mode_app_spad_config &);

    //////////////////////////////////////////////////////////////////////////////////
    // Methods that are called from our "shim relay". They are public, but not really
    int32_t sdkMessageHandler(struct tmf882x_msg *msg);

    // access to the underlying i2c calls - used by the underlying SDK
    int32_t writeRegisterRegion(uint8_t offset, uint8_t *data, uint16_t length);
    int32_t readRegisterRegion(uint8_t reg, uint8_t *data, uint16_t numBytes);

    // method to set the communication bus this object should use
    void setCommBus(QwI2C &theBus, uint8_t id_bus);

    void setDebug(bool bEnable)
    {
        _debug = bEnable;
        
        if (_debug)
            _outputSettings |= TMF882X_MSG_DEBUG;
        else
            _outputSettings &= ~TMF882X_MSG_DEBUG;
    }

    bool getDebug(void)
    {
        return _debug;
    }

    void setInfoMessages(bool bEnable)
    {
        if (bEnable)
            _outputSettings |= TMF882X_MSG_INFO;
        else
            _outputSettings &= ~TMF882X_MSG_INFO;
    }

    void setMessageLevel(uint8_t msg)
    {
        _outputSettings = msg;
    }

    uint8_t getMessageLevel(void)
    {
        return _outputSettings;
    }

  private:
    bool initializeTMF882x(void);
    int measurementLoop(uint16_t nMeasurements, uint32_t timeout);

    // Library initialized flag
    bool _isInitialized;

    // I2C  things
    QwI2C * _i2cBus;      // pointer to our i2c bus object
    uint8_t _i2cAddress; // address of the device

    // Delay for the read sample loop
    uint16_t _sampleDelayMS;

    // Structure/state for the underlying TOF SDK
    tmf882x_tof _TOF;

    // for processing messages from SDK
    uint16_t _nMeasurements;

    // Cache of our last measurement taking
    struct tmf882x_msg_meas_results *_lastMeasurement;

    // Flag to indicate to the system to stop measurements
    bool _stopMeasuring;

    // Callbacks
    //
    // Callback function pointer - measurement
    TMF882XMeasurementHandler _measurementHandlerCB;
    TMF882XHistogramHandler _histogramHandlerCB;
    TMF882XStatsHandler _statsHandlerCB;    
    TMF882XErrorHandler _errorHandlerCB;
    TMF882XMessageHandler _messageHandlerCB;

    // for managing message output levels
    uint8_t _outputSettings;
    bool _debug;
};
