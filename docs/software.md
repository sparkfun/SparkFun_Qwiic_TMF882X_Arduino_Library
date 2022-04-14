# Software Setup


## Installation

The SparkFun Qwiic TMF882X Arduino Library is available within in the Arduino library manager, which is launched via the `Sketch > Include Libraries > Manage Libraries …` menu option in the Arduino IDE. Just search for ***SparkFun Qwiic TMF882X Library***

!!! note
    This guide assumes you are using the latest version of the Arduino IDE on your desktop. The following resources available at [SparkFun](https://www.sparkfun.com) provide the details on setting up and configuring Arduino to use this library.

    - [Installing the Arduino IDE](https://learn.sparkfun.com/tutorials/installing-arduino-ide)
    - [Installing Board Definitions in the Arduino IDE](https://learn.sparkfun.com/tutorials/installing-board-definitions-in-the-arduino-ide)
    - [Installing an Arduino Library](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)

General Use Pattern
---------
After installing this library in your local Arduino environment, begin with a standard Arduino sketch, and include the header file for this library.
```C++
// Include the SparkFun Qwiic TMF882X Library
#include <SparkFun_TMF882X_Library.h>
```
The next step is to declare the object for the SparkFun Qwiic TMF882X device used. Like most Arduino sketches, this is done at a global scope (after the include file declaration), not within the ```setup()``` or ```loop()``` functions. 

For all supported boards, the same object is used: `SparkFun_TMF882X`.
```C++
SparkFun_TMF882X  myTMF882X;
```
In the ```setup()``` function of this sketch, like all of the SparkFun qwiic libraries, the device is initialized by calling the ```begin()``` method. This method returns a value of ```true``` on success, or ```false``` on failure. 
```C++
void setup()
{
    Serial.begin(115200);

    if(!myTMF882X.begin()){
        Serial.println("Device failed to initialize");
        while(1);  // halt execution
    }
    Serial.println("Device is initialized");
    
}
```
Now that device is initialized, the `loop()` function can call the library to retrieve a single data sample from the connected TMF882X device.

Taken from the libraries Example 1, once a measurement is taken, the relevant data is output to the Serial device.

```C++
void loop()
{
    delay(2000);

    // get a Measurement
    if(myTMF882X.startMeasuring(myResults))
    {
        // print out results
        Serial.println("Measurement:");
        Serial.print("     Result Number: "); 
            Serial.print(myResults.result_num);
        Serial.print("  Number of Results: "); 
            Serial.println(myResults.num_results);       

        for (int i = 0; i < myResults.num_results; ++i) 
        {
            Serial.print("       conf: "); 
                Serial.print(myResults.results[i].confidence);
            Serial.print(" distance mm: "); 
                Serial.print(myResults.results[i].distance_mm);
            Serial.print(" channel: "); 
                Serial.print(myResults.results[i].channel);
            Serial.print(" sub_capture: "); 
                Serial.println(myResults.results[i].sub_capture);   

        }
        Serial.print("     photon: "); 
            Serial.print(myResults.photon_count);    
        Serial.print(" ref photon: "); 
            Serial.print(myResults.ref_photon_count);
        Serial.print(" ALS: "); 
            Serial.println(myResults.ambient_light); 
        Serial.println();

    }

}
```

## Advanced Data Collection

The TMF882X is unique when compared to our other qwiic libraries. Instead of data being returned directly from a method, measured data is passed back to the user via a callback function. This is how the underlying TMF882X SDK framework is designed and works well for interactive sensors like the TMF882X. 

To receive data, a callback function is defined. For this example, we define a callback that prints out the detected data.

```C++
void onMeasurementCallback(struct tmf882x_msg_meas_results *myResults)
{
    nSample++;

    Serial.print("Sample Number: ");
    Serial.println(nSample);

    // print out results
    // Not Shown here - This is similar to the output shown in the
    // previous example.
}
```
In the `setup()` function of this example after the device is initialized, the defined callback function is registered with the TMF882 object.

```C++
void setup()
{
    Serial.begin(115200);

    if(!myTMF882X.begin()){
        Serial.println("Device failed to initialize");
        while(1);  // halt execution
    }
    Serial.println("Device is initialized");
    
    // set our callback function in the library.
    myTMF882X.setMeasurementHandler(onMeasurementCallback);
}
```

For this example, in the `loop()` function, a measurement session is started, requesting that four samples be taken before returning from the method call. When data is detected by the TMF882X, the `onMeasurementCallback()` function, is called with the detected results. Once four measurements are taken, the measurement is finished and the TMF882X sensing session is stopped. 

```C++
void loop()
{
    delay(2000);

    // get a measurement
    // Have the sensor take 4 measurements. 
    // 
    // As measurements are taken, the results are sent to the above function, 
    // "onMeasurementCallback()"

    nSample=0; // simple counter
    myTMF882X.startMeasuring(4);

}

```

Library Provided Examples
--------
The SparkFun Qwiic TMF882X Arduino Library, includes a wide variety of examples. These are available from the Examples menu of the Arduino IDE, and in the [`examples`](https://github.com/sparkfun/SparkFun_Qwiic_TMF882X_Arduino_Library/blob/main/examples)folder of this repository. 

For a detailed description of the examples, see the Examples section of the documentation.


