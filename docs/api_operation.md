# Operation

Methods used to read data from the connected TMF882X device.

## Data Callback Functions

The TMF882X SDK returns data via a message structure passed to a user provided C function. This library allows the user to register callback functions that are called during the measurement operation. 

The user has the option to register a generic message callback function, or specific functions based on message type. 

### setMessageHandler()

Call this method with a function that is called when any message is sent from the AMS sdk. The function passed into this method can be used to handle all messages instead of using the other handlers for specific message types.

The passed in function should be of type `TMF882XMessageHandler`, which is defined as:

```C++
typedef void (*TMF882XMessageHandler)(struct tmf882x_msg *msg);
```

This function accepts a parameter of type tmf882x_msg, which is defined as:

```C++
struct tmf882x_msg {
    union {
        struct tmf882x_msg_header       hdr;
        struct tmf882x_msg_error        err_msg;
        struct tmf882x_msg_histogram    hist_msg;
        struct tmf882x_msg_meas_results meas_result_msg;
        struct tmf882x_msg_meas_stats   meas_stat_msg;
        uint8_t msg_buf[TMF882X_MAX_MSG_SIZE];
    };
};
```
The type of the message is defined in `msg->hdr.msg_id`, and is one of the following types:

| Type | Description |
| :---- | :---- |
| ID_MEAS_RESULTS | The results of a measurement - `meas_result_msg` is valid. |
| ID_MEAS_STATS | The measurement statistics - `meas_stat_msg` is valid. |
| ID_HISTOGRAM | The histogram of a measurement - `hist_msg` is valid. |
| ID_ERROR | An error occurred - `err_msg` is valid. |

```c++
void setMessageHandler(TMF882XMessageHandler handler)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| handler | `TMF882XMessageHandler` | The message handler callback C function |

### setMeasurementHandler()

Call this method with a function that is called when measurement data is sent from the AMS sdk.

The passed in function should be of type `TMF882XMeasurementHandler`, which is defined as:

```C++
typedef void (*TMF882XMeasurementHandler)(struct tmf882x_msg_meas_results *message);
```

This function accepts a parameter of type tmf882x_msg_meas_results, which is defined as:

```C++
struct tmf882x_msg_meas_results {
    struct tmf882x_msg_header hdr;
    uint32_t result_num;         /* increments with every new bunch of results */
    uint32_t temperature;        /* temperature */
    uint32_t ambient_light;      /* ambient light level */
    uint32_t photon_count;       /* photon count */
    uint32_t ref_photon_count;   /* reference photon count */
    uint32_t sys_ticks;          /* system ticks */
    uint32_t valid_results;      /* number of valid results */
    uint32_t num_results;        /* number of results */
    struct tmf882x_meas_result results[TMF882X_MAX_MEAS_RESULTS];
};
```
The fields of the message structure are defined as:

| field | Description |
| :---- | :---- |
| result_num | This is the result number reported by the device. |
| temperature | This is the temperature reported by the device (in Celsius) |
| ambient_light | This is the ambient light level reported by the device |
| photon_count | This is the photon count reported by the device |
| ref_photon_count | This is the reference channel photon count reported by the device |
| sys_ticks | This is the system tick counter (5MHz counter) reported by the device. This is used by the core driver to perform clock compensation correction on the measurement results. |
| valid_results | This is the number of targets reported by the device |
| num_results | This is the number of non-zero targets counted by the core driver |
| results | This is the list of measurement targets @ref struct tmf882x_meas_result |

```c++
void setMeasurementHandler(TMF882XMeasurementHandler handler)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| handler | `TMF882XMeasurementHandler` | The message handler callback C function |

### setHistogramHandler()

Call this method with a function that is called when histogram data is sent from the AMS sdk.

The passed in function should be of type `TMF882XHistogramHandler`, which is defined as:

```C++
typedef void (*TMF882XHistogramHandler)(struct tmf882x_msg_histogram * message);
```

This function accepts a parameter of type tmf882x_msg_histogram, which is defined as:

```C++
struct tmf882x_msg_histogram {
    struct tmf882x_msg_header hdr;
    uint32_t capture_num;       /* matches the value of 'result_num' from measure result messages*/
    uint32_t sub_capture;       /* sub-capture measurement nubmer for time multiplexed measurements*/
    uint32_t histogram_type;    /* RAW, ELEC_CAL, etc */
    uint32_t num_tdc;           /* Number of histogram channels in this message */
    uint32_t num_bins;          /* length of histogram(s) for each channel */
    uint32_t bins[TMF882X_HIST_NUM_TDC][TMF882X_HIST_NUM_BINS];
};
```
The fields of the message structure are defined as:

| field | Description |
| :---- | :---- |
| capture_num | This is the capture number that this set of histograms is associated with. The capture_num will match the  `struct tmf882x_msg_meas_results::result_num` |
| sub_capture | This is the time-multiplexed sub-capture index of this set of histograms. For non-time-multiplexed measurements this value is always zero. |
| histogram_type | This is the histogram type identifier code `enum tmf882x_histogram_type` |
| num_tdc | This is the number of TDC histograms being published |
| num_bins | This is the number of bins in the histograms being published |
| bins | These are the histogram bin values for each TDC. There are two channels per TDC, the first channel histogram occupies bins `[0 : TMF882X_HIST_NUM_BINS/2 - 1]`, the 2nd channel occupies bins `[TMF882X_HIST_NUM_BINS/2 : @ref TMF882X_HIST_NUM_BINS - 1]` |

```c++
void setHistogramHandler(TMF882XHistogramHandler handler)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| handler | `TMF882XHistogramHandler` | The message handler callback C function |


### setStatsHandler()

Call this method with a function that is called when measurement statistics data is sent from the AMS sdk.

The passed in function should be of type `TMF882XStatsHandler`, which is defined as:

```C++
typedef void (*TMF882XStatsHandler)(struct tmf882x_msg_meas_stats *message);
```

This function accepts a parameter of type tmf882x_msg_meas_stats, which is defined as:

```C++
struct tmf882x_msg_meas_stats {
    struct tmf882x_msg_header hdr;
    uint32_t capture_num;       /* matches the value of 'result_num' from measure result messages*/
    uint32_t sub_capture;       /* sub-capture measurement nubmer for time multiplexed measurements*/
    uint32_t tdcif_status;
    uint32_t iterations_configured;
    uint32_t remaining_iterations;
    uint32_t accumulated_hits;
    uint32_t raw_hits[TMF882X_HIST_NUM_TDC];
    uint32_t saturation_cnt[TMF882X_HIST_NUM_TDC];
};
```
The fields of the message structure are defined as:

| field | Description |
| :---- | :---- |
| capture_num | This is the capture number that this set of histograms is associated with. The capture_num will match the  `struct tmf882x_msg_meas_results::result_num` |
| sub_capture | This is the time-multiplexed sub-capture index of this set of histograms. For non-time-multiplexed measurements this value is always zero. |
| tdcif_status | This is the tdcif status reported by the device |
| iterations_configured | This is the iterations configured reported by the device |
| remaining_iterations | This is the remaining iterations reported by the device |
| accumulated_hits | This is the accumulated hits reported by the device |
| raw_hits | This is the raw hits reported by the device for each TDC |
| saturation_cnt | This is the saturation count reported by the device for each TDCd |

```c++
void setStatsHandler(TMF882XStatsHandler handler)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| handler | `TMF882XStatsHandler` | The message handler callback C function |

### setErrorHandler()

Call this method with a function that is called when error information is sent from the AMS sdk.

The passed in function should be of type `TMF882XErrorHandler`, which is defined as:

```C++
typedef void (*TMF882XErrorHandler)(struct tmf882x_msg_error *message);
```

This function accepts a parameter of type tmf882x_msg_error, which is defined as:

```C++
struct tmf882x_msg_error {
    struct tmf882x_msg_header hdr;
    uint32_t err_code;
};
```
The fields of the message structure are defined as:

| field | Description |
| :---- | :---- |
| err_code | This is the error code identifier |

```c++
void setErrorHandler(TMF882XErrorHandler handler)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| handler | `TMF882XErrorHandler` | The message handler callback C function |

## Measurement Methods

### startMeasuring()

Start measuring distance/data on the TMF882X device. This method returns after one measurement is performed.

Measurement data is returned in the provided measurement struct.

```c++
int startMeasuring(struct tmf882x_msg_meas_results results, uint32_t timeout = 0)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| results| `struct tmf882x_msg_meas_results` | The results of the measurement |
| timeout| `uint32_t` | [OPTIONAL] The time, in milliseconds, to take measurements. A value of zero [default value] indicates no timeout set |
| return value| `int` | The number of measurements taken (1), or -1 on error. |

### startMeasuring()

Start measuring distance/data on the TMF882X device. This method won't return until the measurement activity ends.

Measurement data is passed to the library user via a callback function, which is set using one of the set<type>Handler() methods on this object.

Measurements continue, until one of the following conditions occurs:

* The specified number of measurements took place (set via reqMeasurements)
* The specified timeout value expires
* The stopMeasuring() method was called in a Handler function.

This method won't start measuring if a measurement limit isn't set, a timeout isn't set and no callback handlers are not set.

This method returns the number of measurements taken

```c++
int startMeasuring(uint32_t reqMeasurements = 0, uint32_t timeout = 0)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| reqMeasurements| `uint32_t` | [OPTIONAL] The number of measurements desired. A value of zero [the default value] indicates no limit |
| timeout| `uint32_t` | [OPTIONAL] The time, in milliseconds, to take measurements. A value of zero [default value] indicates no timeout set |
| return value| `int` | The number of measurements taken, or -1 on error. |

### stopMeasuring()

Called to stop the device measuring loop. Normally called in a message handler function.

```c++
void stopMeasuring(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| None|  |  |


### setSampleDelay()

Set the delay used in the libraries sample loop used when processing samples/reading from the device.

```c++
void setSampleDelay(uint16_t delay)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| delay| `uint16_t` | The delay to use, in milli-seconds. |

### getSampleDelay()

Returns the current value of the library processing loop delay. The value is ib milliseconds..

```c++
uint16_t getSampleDelay(void)
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| return value| `uint16_t` | The current delay, in milli-seconds. |