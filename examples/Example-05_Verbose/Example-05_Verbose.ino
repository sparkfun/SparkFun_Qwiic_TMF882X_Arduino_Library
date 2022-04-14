/*

  Example-05_Verbose.ino

  The TMF882X Arduino library uses the TMF882X Software Development Kit (SDK) from
  AMS to interface with the sensor. 

  The AMS SDK is able to print out informational messages during normal operation, as 
  well as debug messages. This example shows how to enable those messages and direct 
  them to a Serial device for output.

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

static struct tmf882x_msg_meas_results myResults;

void setup()
{

    delay(1000);
    Serial.begin(115200);
    Serial.println("");

    Serial.println("In setup");
    Serial.println("==============================");

    // The underlying TMF882X SDK can output a wide variety of information during 
    // normal operation. It's very verbose.
    //
    // Enable this output as part of this demo.
    //
    // Pass in our output device - Serial

    myTMF882X.setOutputDevice(Serial);

    // Enable Info messages
    myTMF882X.setInfoMessages(true);

    // Enable Debug mode. Set this before calling begin to get initialization debug 
    // information.

    myTMF882X.setDebug(true);

    if(!myTMF882X.begin())
    {
        Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
        while(1);
    }else
        Serial.println("TMF882X started.");


}

void loop()
{
    delay(2000);

    // get a myResultsurment
    if(myTMF882X.startMeasuring(myResults))
    {
        // print out results
        Serial.println("Measurement:");
        Serial.print("     Result Number: "); Serial.print(myResults.result_num);
        Serial.print("  Number of Results: "); Serial.println(myResults.num_results);       

        for (uint32_t i = 0; i < myResults.num_results; ++i) 
        {
            Serial.print("       conf: "); Serial.print(myResults.results[i].confidence);
            Serial.print(" distance mm: "); Serial.print(myResults.results[i].distance_mm);
            Serial.print(" channel: "); Serial.print(myResults.results[i].channel);
            Serial.print(" sub_capture: "); Serial.println(myResults.results[i].sub_capture);   

        }
        Serial.print("     photon: "); Serial.print(myResults.photon_count);    
        Serial.print(" ref photon: "); Serial.print(myResults.ref_photon_count);
        Serial.print(" ALS: "); Serial.println(myResults.ambient_light); Serial.println();

    }

}
