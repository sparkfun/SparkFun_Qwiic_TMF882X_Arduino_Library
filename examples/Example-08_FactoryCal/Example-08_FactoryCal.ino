/*
TODO - Fix Header
*/


#include <SparkFun_TMF882X_Library.h>

SparkFun_TMF882X  myTMF882X;

void setup(){

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
	if (!myTMF882X.getTMF882XConfig(tofConfig)) {
		Serial.println("Error - unable to get device configuration.");
		while(1){}
	}
    
    // Change the APP configuration
    //  - set the reporting period to 500 milliseconds
    //  - set the iterations to 4,000,000 (4M) to perform factory calibration
    tofConfig.report_period_ms = 500;
    tofConfig.kilo_iterations = 4000;

	if (!myTMF882X.setTMF882XConfig(tofConfig)) {
		Serial.println("Error - unable to set device configuration.");
		while(1){}
	}

	struct tmf882x_mode_app_calib factoryCal;

	// Peform the calibration
	if (!myTMF882X.factoryCalibration(factoryCal)) {
		Serial.println("Error - Factory Calibration Failed.");
		while(1){}
	}

	// Output the calibration
	Serial.println("Calibration Complete"); 
	Serial.println();
	Serial.print("Calibration Data Length: ");
	Serial.println(factoryCal.calib_len);

	Serial.println("Calibration Data:");
	for (int i=0; i < factoryCal.calib_len; i++){
		Serial.print("   "); Serial.print(factoryCal.data[i]);

		if ( (i + 1) % 16 == 0 )
			Serial.println();
	}
	Serial.println();

}

void loop()
{
	delay(2000);
}
