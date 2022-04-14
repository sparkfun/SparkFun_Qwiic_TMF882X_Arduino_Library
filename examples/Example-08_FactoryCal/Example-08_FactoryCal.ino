/*

  Example-08_FactoryCal.ino

  This example shows how to peform a Factory Calibration on the connected 
  TMF882X device. Details on the calibration and it's use are contained in
  the TMF882X datasheet.

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

SparkFun_TMF882X  myTMF882X;

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


    Serial.println();
    Serial.println("Performing a Factory Calibration.");
    Serial.println();
    // Perform a factory calibration of the connected device.

    // First set some config parameters to support the calibration
    struct tmf882x_mode_app_config tofConfig;
    if (!myTMF882X.getTMF882XConfig(tofConfig)) 
    {
        Serial.println("Error - unable to get device configuration.");
        while(1){}
    }
    
    // Change the APP configuration
    //  - set the reporting period to 500 milliseconds
    //  - set the iterations to 4,000,000 (4M) to perform factory calibration
    tofConfig.report_period_ms = 500;
    tofConfig.kilo_iterations = 4000;

    if (!myTMF882X.setTMF882XConfig(tofConfig)) 
    {
        Serial.println("Error - unable to set device configuration.");
        while(1){}
    }

    struct tmf882x_mode_app_calib factoryCal;

    // Peform the calibration
    if (!myTMF882X.factoryCalibration(factoryCal)) 
    {
        Serial.println("Error - Factory Calibration Failed.");
        while(1){}
    }

    // Output the calibration
    Serial.println("Calibration Complete"); 
    Serial.println();
    Serial.print("Calibration Data Length: ");
    Serial.println(factoryCal.calib_len);

    Serial.println("Calibration Data:");
    for (int i = 0; i < factoryCal.calib_len; i++)
    {
        Serial.print("   "); Serial.print(factoryCal.data[i]);

        if ( (i + 1) % 16 == 0 )
            Serial.println();
    }
    Serial.println();

}

void loop()
{
    delay(2000);

    // Nothing - just here for the calibration example
}
