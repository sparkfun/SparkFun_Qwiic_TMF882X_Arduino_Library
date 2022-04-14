/*

  Example-09_SPADMap.ino

  The Optical performance of the TMF882X is controled by a SPAD (Single Photon
  Avalanche Photodiode) Map. 

  SPAD Maps are set using a SPAD Map ID, which are detailed in the TMF882X datasheet.

  This example shows how to determine the current SPAD Map on the device and change 
  it to a desired map.

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

#include <SparkFun_TMF882X_Library.h>    //http://librarymanager/All#SparkFun_Qwiic_TMPF882X

static struct tmf882x_msg_meas_results myResults;


SparkFun_TMF882X  myTMF882X;

// What SPAD map to change to

#define NEW_SPAD_MAP 2

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
        while(1){}
    }else
        Serial.println("TMF882X started.");


    // Let's change the SPAD map in use on this device.
    //
    // Get the current SPAD Map ID
    int spadMap =  myTMF882X.getCurrentSPADMap();
    Serial.println();
    Serial.print("Current SPAD Map ID: ");
    Serial.println(spadMap);

    // Now switch
    Serial.println("Switching SPAD Map to ID 2 - 3x3 Macro 1 off center");
    Serial.println();

    if (!myTMF882X.setCurrentSPADMap(NEW_SPAD_MAP))
    {
        Serial.println("Error -  Failed to set the SPAD Map - halting");
        while(1){}
    }

    // Let's make sure it worked
    spadMap =  myTMF882X.getCurrentSPADMap();

    if(spadMap != NEW_SPAD_MAP)
    {
        Serial.println("Error -  Failed to set the SPAD Map - halting");
        while(1){}
    }

    Serial.print("The new SPAD Map ID: ");
    Serial.println(spadMap);

    // Now set some config parameters to support the spad map
    struct tmf882x_mode_app_config tofConfig;

    if (!myTMF882X.getTMF882XConfig(tofConfig)) 
    {
        Serial.println("Error - unable to get device configuration.");
        while(1){}
    }
    
    // Change the APP configuration
    //  - set the reporting period to 500 milliseconds
    tofConfig.report_period_ms = 500;

    if (!myTMF882X.setTMF882XConfig(tofConfig)) 
    {
        Serial.println("Error - unable to set device configuration.");
        while(1){}
    }

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

        for (int i = 0; i < myResults.num_results; ++i) 
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
