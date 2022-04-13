# Device Operations

Methods to setup the device, get device information and change device options.

## Initialization

### begin()
This method is called to initialize the TMF882X library and connect to the TMF882X device. This method should be called before accessing the device.

The I2C address of the is an optional argument. If not provided, the default address is used.

During the initialization process, the device is opened and the runtime firmware loaded onto the device. The TMF882X device is them placed into "APP" mode. 

```c++
bool begin(TwoWire &wirePort, uint8_t address)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `wirePort` | `TwoWire` | **optional**. The Wire port. If not provided, the default port is used|
| `address` | `uint8_t` | **optional**. I2C Address. If not provided, the default address is used|
| return value | `bool` | ```true``` on success, ```false``` on startup failure |

### loadFirmware()
To operate the TMF882X device, runtime firmware must be loaded. At startup, this library loads a default firmware version on library initialization.

This method allows the library user to set the firmware version on the device if a newer version is available from AMS.

```C++ 
bool loadFirmware(const unsigned char *firmwareBinImage, unsigned long length)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `firmwareBinImage` | `const unsigned char` | The firmware binary image |
| `length` | `unsigned long` | The length of the image array |
| return value | `bool` | ```true``` on success, ```false``` on failure |

### isConnected()
Called to determine if a TMF882X device, at the provided i2c address is connected.

```C++ 
bool isConnected()
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| return value | `bool` | ```true``` if connected, ```false``` if not |

### setI2CAddress()
Called to change the I2C address of the connected device.

```C++ 
bool setI2CAddress(uint8_t address)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| return value | `bool` | ```true``` on success, ```false``` on failure |

### getApplicationVersion()
Returns the version of the "Application" software running on the connected TMF882X device. See the TMF882X data sheet for more information regarding application software

```C++ 
bool getApplicationVersion(char *Version, uint8_t vlen)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `Version` | `char*` |   Pointer to a character array to receive the version data |
| `vlen` | `uint8_t` | The length of the array pointed to by Version |
| return value | `bool` | ```true``` on success, ```false``` on failure |

### getDeviceUniqueID()
Returns the unique ID of the connected TMF882X.

!!! note
    This method uses an ID structure as defined by the AMS TMF882X SDK to
    store the ID value.

```C++ 
bool getDeviceUniqueID(struct tmf882x_mode_app_dev_UID &devUID)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `deviceUID` | `struct tmf882x_mode_app_dev_UID` |   The TMF882X UID structure to store the ID into. |
| return value | `bool` | ```true``` on success, ```false``` on failure |


## Debug and Development

### setDebug()
Set the debug state fo the SDK. To use the full debug capabilities of the SDK, debug should be enabled before calling init/begin() on the library

```C++ 
void setDebug(bool bEnable)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `bEnable` | `bool` |   To enable or disable debug mode in the SDK|

### getDebug()
Returns the current debug setting of the library

```C++ 
bool getDebug(void)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| return value | `bool` | ```true``` Debug mode enabled, ```false``` Debug Mode disabled |

### setInfoMessages()
Enable/Disable the output of info messages from the AMS SDK.

```C++ 
void setInfoMessages(bool bEnable)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `bEnable` | `bool` |   To enable or disable info message output from the SDK|

### setMessageLevel()
Used to set the message level of the system.

The value passed in should be one, or a combination of the following flags.

| FLAG |  Description |
| :------------ | :---------- | 
| TMF882X_MSG_INFO      | Output Info messages|
| TMF882X_MSG_DEBUG     | Output Debug messages|
| TMF882X_MSG_ERROR     | Output Error messages|
| TMF882X_MSG_ALL       | Output all messages|
| TMF882X_MSG_NONE      | Disable all message output the output of info messages from the AMS SDK.|

```C++ 
void setMessageLevel(uint8_t msg)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `msg` | `uint8_t` |   Message type flag(s)|

### getMessageLevel()
Return the current message settings. See setMessageLevel() description for possible values

```C++ 
uint8_t getMessageLevel(void)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| return value | `uint8_t` |   The current message level settings|

### setOutputDevice()
This method is called to provide an output Serial device that the is used to output messages from the underlying AMS SDK.

This is needed when debug or info messages are enabled in the library

```C++ 
void setOutputDevice(Stream& theStream)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| `theStream` | `Stream` |   The output stream device - normally a Serial port. |

### getTMF882XContext()
Returns the context structure used by this library when accessing the underlying TMF882X SDK.

With this structure,  users of this library can make direct calls to the interface functions of the TMF882X SDK.

!!! warning
    Calling the TMF882X SDK functions directly could impact the operation of this library. Use this option with caution.

```C++ 
tmf882x_tof &getTMF882XContext(void)
```

| Parameter | Type | Description |
| :------------ | :---------- | :---------------------------------------------- |
| return value | `tmf882x_tof` |   The TMF882X Context being used by this library. |