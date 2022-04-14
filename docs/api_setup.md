# Device Setup

Methods used to setup and configure the device before taking data samples. 

## Configuration 

### getTMF882XConfig()

The underlying TMF882X SDK uses a "configuration structure" to read and modify the configuration of the connected device. This method retrieves the current configuration of the device.

When changing a value of the configuration, the following steps are taken:
* Retrieve the current configuration using `getTMF882XConfig()`
* Change the value in the configuration structure
* Updating the device configuration using the method `setTMF882XConfig()`

The configuration is transferred as a structure of type `struct tmf882x_mode_app_config`. This structure is defined by the TMF882X SDK, and contains the following fields:

| Type | Struct Field | Description |
| :--- | :--- | :--- |
| uint16_t | report_period_ms | Result reporting period in milliseconds |
| uint16_t | kilo_iterations | Iterations * 1024 for measurements |
| uint16_t | low_threshold | Low distance threshold setting triggering interrupts |
| uint16_t | high_threshold | High distance threshold setting triggering interrupts |
| uint32_t | zone_mask | Zone mask for disabling interrupts for certain channels |
| uint8_t | persistence | Persistence setting for generating interrupts |
| uint8_t | confidence_threshold | Confidence threshold for generating interrupts |
| uint8_t | gpio_0 | GPIO_0 config settings |
| uint8_t | gpio_1 | GPIO_1 config settings |
| uint8_t | power_cfg |  Power configuration settings |
| uint8_t | spad_map_id | Spad map identifier |
| uint32_t | alg_setting | Algorithm setting configuration |
| uint8_t | histogram_dump | Histogram dump configuration |
| uint8_t | spread_spectrum | Spread Spectrum configuration |
| uint8_t | i2c_slave_addr | I2C slave address configuration |
| uint16_t | oscillator_trim | Sensor Oscillator trim value |

The details and specific values for the above fields are detailed in the TMF882X datasheet.

```c++
 bool getTMF882XConfig(struct tmf882x_mode_app_config tofConfig)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| tofConfig | `struct tmf882x_mode_app_config` | A configuration structure that the current configuration data is copied to|
| return value| `bool` | `true` on success, `false` on an error |


### setTMF882XConfig()

Call this method to set the current configuration page in the connected TMF882X device. A `tmf882x_mode_app_config` structure with the desired values is passed into this method. Details of the structure fields are listed in the description of the `getTMF882XConfig()` method.

When changing a value of the configuration, the following steps are taken:
* Retrieve the current configuration using `getTMF882XConfig()`
* Change the value in the configuration structure
* Updating the device configuration using the method `setTMF882XConfig()`

```c++
bool setTMF882XConfig(struct tmf882x_mode_app_config tofConfig)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| tofConfig | `struct tmf882x_mode_app_config` | A configuration structure that has the desired settings for the device|
| return value| `bool` | `true` on success, `false` on an error |

## Calibration 

### factoryCalibration()

Used to run a factory calibration on the connected TMF882X device. The results of the calibration are returned in the passed in calibration structure.

Consult the TMF882X datasheet for details on performing a factory calibration

```c++
bool factoryCalibration(struct tmf882x_mode_app_calib tofCalib);
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| tofCalib | `struct tmf882x_mode_app_calib` | The results of the calibration process |
| return value| `bool` | `true` on success, `false` on an error |

### setCalibration()

Used to set the calibration data on the connected TMF882X device.

Consult the TMF882X datasheet for details on calibration data.

```c++
bool setCalibration(struct tmf882x_mode_app_calib tofCalib)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| tofCalib | `struct tmf882x_mode_app_calib` | The calibration data to set |
| return value| `bool` | `true` on success, `false` on an error |

### getCalibration()

Used to get the calibration data on the connected TMF882X device.

Consult the TMF882X datasheet for details on calibration data.

```c++
bool setCalibration(struct tmf882x_mode_app_calib tofCalib)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| tofCalib | `struct tmf882x_mode_app_calib` | The calibration data |
| return value| `bool` | `true` on success, `false` on an error |

## SPAD Settings


### getCurrentSPADMap()

This method returns the ID of the current SPAD Map in use on the device. The TMF882X uses a _SPAD Map_ to control the viewing geometry of the sample data retrieved from the sensor. Consult the TMF882X datasheet for SPAD Map details as available ID values. 

```c++
uint8_t getCurrentSPADMap(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| return value| `uint8_t` | `0` on error, otherwise the ID of the SPAD Map|

### setCurrentSPADMap()

This method sets the current SPAD map in the connected device. The SPAD Map is specified by its ID. Available SPAD Maps and their associated IDs are listed in the TMF882X datasheet.

```c++
bool setCurrentSPADMap(uint8_t idSPAD)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| idSPAD| `uint8_t` | The ID of the SPAD to make current on the device |
| return value| `bool` | `true` on success, `false` on an error |

### getSPADConfig()

Retrieve the current SPAD configuration from the connected device. This configuration is returned as a `tmf882x_mode_app_spad_config` structure. 


The SPAD configuration structure is defined as follows:
```C++
struct tmf882x_mode_app_spad_config {
    struct tmf882x_mode_app_single_spad_config {
        int8_t xoff_q1;
        int8_t yoff_q1;
        uint8_t xsize;
        uint8_t ysize;
        uint8_t spad_mask[TMF8X2X_COM_MAX_SPAD_SIZE];
        uint8_t spad_map[TMF8X2X_COM_MAX_SPAD_SIZE];
    } spad_configs[TMF8X2X_MAX_CONFIGURATIONS];
    uint32_t num_spad_configs;
};
```
The fields are defined as:

| Type | Struct Field | Description |
| :--- | :--- | :--- |
| int8_t | xoff_q1 | X-direction offset in Q1 format |
| int8_t | yoff_q1 | Y-direction offset in Q1 format |
| uint8_t | xsize | Size of spad map in X-direction |
| uint8_t | ysize | Size of spad map in Y-direction |
| uint8_t | spad_mask | Spad enable mask configuration (1 enable, 0 disable)|
| uint8_t | spad_map | Spad channel mapping for measurement (channels 1 - 9) |
| uint32_t | num_spad_configs | he number of spad configurations |

The details and specific values for the above fields are detailed in the TMF882X datasheet.

```c++
bool getSPADConfig(struct tmf882x_mode_app_spad_config tofSpad)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| tofSpad| `struct tmf882x_mode_app_spad_config` | Struct to hold the current SPAD configuration values. |
| return value| `bool` | `true` on success, `false` on an error |

### setSPADConfig()

Set the current SPAD configuration in the connected device. This configuration is set using a `tmf882x_mode_app_spad_config` structure. This method is used to set custom SPAD Maps in the TMF882X.

See the descriptor of `getSPADConfig()` for details of the `tmf882x_mode_app_spad_config`.

```c++
bool setSPADConfig(struct tmf882x_mode_app_spad_config tofSpad)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| tofSpad| `struct tmf882x_mode_app_spad_config` | The config values for the on device SPAD settings. |
| return value| `bool` | `true` on success, `false` on an error |