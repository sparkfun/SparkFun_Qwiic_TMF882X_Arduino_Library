/*

  Example-11_Histogram.ino

  This example shows how to enable and recieve raw histogram data from the 
  connected TMF882X device

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

#define NUMBER_OF_SAMPLES_TO_TAKE  4

int nSample = 0;

// For our histogram printout 
#define MAX_BIN_LEN 128

// Define our histogram callback function

void onHistogramCallback(struct tmf882x_msg_histogram *myHistogram)
{

    nSample++;

    Serial.print("Histogram Number: ");
    Serial.println(nSample);

    uint8_t zone_count = 0;
    for (int tdc_idx = 0; tdc_idx < myHistogram->num_tdc; ++tdc_idx) 
    {
        // Histogram tag for zones, #HLONG01,#HLONG02....
        Serial.println();
        Serial.print("#HLONG");
        Serial.print(zone_count++);

        for (int bin_idx = 0; bin_idx < myHistogram->num_bins; ++bin_idx) {

            Serial.print((unsigned long)myHistogram->bins[tdc_idx][bin_idx]);

            if ((bin_idx + 1) == MAX_BIN_LEN) 
            {
                Serial.println();
                Serial.print("#HLONG");
                Serial.print(zone_count++);

            } else if ((bin_idx + 1) % MAX_BIN_LEN != 0)
                Serial.print(",");
        }
    }
    Serial.println();
}

void setup()
{

    delay(500);
    Serial.begin(115200);
    Serial.println("");


    if(!myTMF882X.begin())
    {
        Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
        while(1){}
    }

    // set our call back function that handles histograms
    myTMF882X.setHistogramHandler(onHistogramCallback);

    // Set our delay between samples  - 1 second - note it's in ms
    myTMF882X.setSampleDelay(1000);

    // First config parameter to enable output of histogram data.

    struct tmf882x_mode_app_config tofConfig;
    if (!myTMF882X.getTMF882XConfig(tofConfig)) 
    {
        Serial.println("Error - unable to get device configuration.");
        while(1){}
    }
    
    // Change the APP configuration
    //  - set the reporting period to 500 milliseconds
    //  - Enable Histogram mode
    tofConfig.report_period_ms = 500;
    tofConfig.histogram_dump = 1;

    if (!myTMF882X.setTMF882XConfig(tofConfig)) 
    {
        Serial.println("Error - unable to set device configuration.");
        while(1){}
    }
}

void loop()
{
    delay(2000);

    // get a measurment
    // Have the sensor take 4 measurements, the results are sent to the above callback

    Serial.println("---------------------------------------------------------");    
    Serial.print("Taking "); 
    Serial.print(NUMBER_OF_SAMPLES_TO_TAKE);
    Serial.println(" data samples."); 
    Serial.println();

    nSample=0;
    myTMF882X.startMeasuring(NUMBER_OF_SAMPLES_TO_TAKE);
    
    Serial.println("---------------------------------------------------------\n\n");    

}
