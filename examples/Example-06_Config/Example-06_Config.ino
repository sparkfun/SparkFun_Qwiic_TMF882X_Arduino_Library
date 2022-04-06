/*
TODO - Fix Header
*/


#include "SparkFun_TMF882X_Library.h"

SparkFun_TMF882X  myTMF882X;

// Declare a TMF882X device configuration structure
static struct tmf882x_mode_app_config tofConfig = { 0 };


static TMF882XMeasurement_t myResults;

void setup(){

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
	Serial.println( (bConfigSet ? "Succeeded" : "Failed"));

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

	    for (uint32_t i = 0; i < myResults.num_results; ++i) {
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
