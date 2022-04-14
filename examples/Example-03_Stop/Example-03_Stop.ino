/*

  Example-03_Stop.ino

  The TMF882X Arduino library uses the TMF882X Software Development Kit (SDK) from
  AMS to interface with the sensor. This SDK returns results by calling a provided
  function and passing in a message structure.

  This examples shows how to setup a callback, but use the stopMeasuring() 
  to stop operation of the TMF882X.

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


#include "SparkFun_TMF882X_Library.h"  //http://librarymanager/All#SparkFun_Qwiic_TMPF882X

SparkFun_TMF882X  myTMF882X;

// Define how many measurments/samples to take before calling "stopMeasuring()"

#define NUMBER_OF_SAMPLES_TO_TAKE  4

int nSample = 0;

// Define our measurement callback function. The Library calls this when a 
// measurment is taken.

void onMeasurementCallback(struct tmf882x_msg_meas_results *myResults)
{

    nSample++;
    Serial.print("Sample Number: ");
    Serial.println(nSample);

    // print out results
    Serial.println("Measurement:");
    Serial.print("Result Number: "); Serial.print(myResults->result_num);
    Serial.print(" Number of Results: "); Serial.println(myResults->num_results);       

    for(int i = 0; i < myResults->num_results; ++i) 
    {
        Serial.print("    conf: "); Serial.print(myResults->results[i].confidence);
        Serial.print(" distance mm: "); Serial.print(myResults->results[i].distance_mm);
        Serial.print(" channel: "); Serial.print(myResults->results[i].channel);
        Serial.print(" sub_capture: "); Serial.println(myResults->results[i].sub_capture);  

    }
    Serial.print(" photon: "); Serial.print(myResults->photon_count);   
    Serial.print(" ref photon: "); Serial.print(myResults->ref_photon_count);
    Serial.print(" ALS: "); Serial.println(myResults->ambient_light); Serial.println();

    // If we are at our limit, stop taking measurments

    if(nSample == NUMBER_OF_SAMPLES_TO_TAKE)
    {
        myTMF882X.stopMeasuring();
    
        Serial.println();
        Serial.println("-----------------------------------------------");
        Serial.println("Measurement Goal Hit - stopping measurements.");
        Serial.println("-----------------------------------------------");
        Serial.println();       
    }
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
    myTMF882X.setSampleDelay(1000);
}

void loop()
{
    delay(2000);

    Serial.println("---------------------------------------------------------");    
    Serial.print("Taking "); 
    Serial.print(NUMBER_OF_SAMPLES_TO_TAKE);
    Serial.println(" data samples.");
    Serial.println();

    nSample = 0;

    // Start taking samples. Note - no stop condition is specified. This is handled
    // in the measurement callback function above.

    myTMF882X.startMeasuring();
    
    Serial.println("---------------------------------------------------------\n\n");    

}
