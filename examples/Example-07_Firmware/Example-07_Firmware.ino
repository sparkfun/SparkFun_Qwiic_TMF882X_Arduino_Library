/*

  Example-07_Firmware.ino

  The TMF882X device requires firmware be loaded at runtime to operate correctly.

  The SparkFun Qwiic TMF882X library automatically downloads the firmware to the 
  connected device when the begin() method is called. 

  This example shows how to load firmware on the connected TMF882X using the library. 
  This is helpful if a new firmware version is released for the TMF882X, and has yet
  to be included in this Arduino Library. 

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

// Inlcude the firmware header file - this is from the TMF882X SDK, and ships
// with the SparkFun library. This header defines the symbols:
//
//  tof_bin_image           - the Firmware image
//  tof_bin_image_length    - length/size of the firmware

#include <tof_bin_image.h>

SparkFun_TMF882X  myTMF882X;

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

    Serial.println();
    // The library loads the firmware into the TMF882X device on startup (call to begin), 
    // which is normal. 
    //
    // This example shows how to upload new Firmware, if it's been released.

    if (!myTMF882X.loadFirmware(tof_bin_image, tof_bin_image_length))
        Serial.println("ERROR - Failure to load new firmware into the TMF882X.");
    else
        Serial.println("The new firmware was loaded successfully into the TMF882X.");
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
