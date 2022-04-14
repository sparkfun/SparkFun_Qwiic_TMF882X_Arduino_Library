/*

  Example-04_Timeout.ino

  The TMF882X Arduino library uses the TMF882X Software Development Kit (SDK) from
  AMS to interface with the sensor. This SDK returns results by calling a provided
  function and passing in a message structure.

  This example shows how to create and regsiter a callback function to recieve results
  from the library. 

  The library will continue to take measurments until the specified timeout value is 
  reached.

  Supported Boards:
  
   SparkFun Qwiic dToF Imager - TMF8820        https://www.sparkfun.com/products/19036
   SparkFun Qwiic Mini dToF Imager - TMF8820   https://www.sparkfun.com/products/19218
   SparkFun Qwiic Mini dToF Imager - TMF8821   https://www.sparkfun.com/products/19451
   SparkFun Qwiic dToF Imager - TMF8821        https://www.sparkfun.com/products/19037
   
  Written by Kirk Benell @ SparkFun Electronics, April 2022

  Repository:
     https://github.com/sparkfun/SparkFun_Qwiic_TMF882X_Arduino_Library

  Documentation:
     https://sparkfun.github.io/SparkFun_Qwiic_TMF882X_Arduino_Library/

  SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
*/


#include "SparkFun_TMF882X_Library.h"    //http://librarymanager/All#SparkFun_Qwiic_TMPF882X

SparkFun_TMF882X  myTMF882X;

// How long to take samples 

#define SAMPLE_TIMEOUT_MS  3000

// Define our measurement callback function

void onMeasurementCallback(struct tmf882x_msg_meas_results *myResults)
{

    // print out results
    Serial.println("Measurement:");
    Serial.print("Result Number: "); Serial.print(myResults->result_num);
    Serial.print(" Number of Results: "); Serial.println(myResults->num_results);       

    for(uint32_t i = 0; i < myResults->num_results; ++i) 
    {
        Serial.print("    conf: "); Serial.print(myResults->results[i].confidence);
        Serial.print(" distance mm: "); Serial.print(myResults->results[i].distance_mm);
        Serial.print(" channel: "); Serial.print(myResults->results[i].channel);
        Serial.print(" sub_capture: "); Serial.println(myResults->results[i].sub_capture);  

    }
    Serial.print(" photon: "); Serial.print(myResults->photon_count);   
    Serial.print(" ref photon: "); Serial.print(myResults->ref_photon_count);
    Serial.print(" ALS: "); Serial.println(myResults->ambient_light); Serial.println();
}

void setup()
{

    delay(500);
    Serial.begin(115200);
    Serial.println("");


    if(!myTMF882X.begin())
    {
        Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
        while(1);
    }

    // set our call back function
    myTMF882X.setMeasurementHandler(onMeasurementCallback);

    // Set our delay between samples  - 1 second - note it's in ms
    myTMF882X.setSampleDelay(700);
}

void loop()
{
    delay(2000);

    Serial.println("---------------------------------------------------------");    
    Serial.print("Taking Samples over a period of: "); 
    Serial.print(SAMPLE_TIMEOUT_MS);
    Serial.println(" MS");
    Serial.println();
        
    // If number of desired samples is 0, the system loops forever - until timeout 
    // is hit, or stopped in the callback function.

    int nSamples = myTMF882X.startMeasuring(0, SAMPLE_TIMEOUT_MS);

    Serial.print("Took "); 
    Serial.print(nSamples);
    Serial.println(" data samples.");
    Serial.println();
    
    Serial.println("---------------------------------------------------------\n\n");    

}
