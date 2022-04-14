/*

  Example-06_Config.ino


  This example shows how to get and set the TMF882X device configuration structure. Details
  of this structure are in the TMF882X datasheet.

  The example also shows how to retrieve the unique ID of the device attached.

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


#include "SparkFun_TMF882X_Library.h"   //http://librarymanager/All#SparkFun_Qwiic_TMPF882X

SparkFun_TMF882X  myTMF882X;

// Declare a TMF882X device configuration structure
static struct tmf882x_mode_app_config tofConfig = { 0 };


static struct tmf882x_msg_meas_results myResults;

void setup()
{

    delay(1000);
    Serial.begin(115200);
    Serial.println("");

    Serial.println("In setup");
    Serial.println("==============================");

    if(!myTMF882X.begin())
    {
        Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
        while(1);
    }else
        Serial.println("TMF882X started.");

    // Use the configuration settings of the TMF882X to change the device reporting
    // period to 500 ms

    bool bConfigSet = false;
    if (myTMF882X.getTMF882XConfig(tofConfig))
    {
        // Change the report period
        tofConfig.report_period_ms = 500;

        bConfigSet = myTMF882X.setTMF882XConfig(tofConfig);
    }

    // printout a message 
    Serial.print("The update of the TMF882X Report Period ");
    Serial.println( (bConfigSet ? "Succeeded" : "Failed") );

    // Get the unique identifier from our connected device.
    struct tmf882x_mode_app_dev_UID devUID;

    if (myTMF882X.getDeviceUniqueID(devUID))
    {
        Serial.print("Connected Device Unique ID: ");
        Serial.println(devUID.uid);
    }
    Serial.println();
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
