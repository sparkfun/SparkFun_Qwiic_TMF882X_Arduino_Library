/*
TODO - Fix Header
*/

#include <SparkFun_TMF882X_Library.h>



static struct tmf882x_msg_meas_results myResults;

// Include header, which defines a static SPAD map for this demo.
#include "Example-10_CustomSPADMap.h"

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
	Serial.println("Setting a custom SPAD Map.");
	Serial.println();

	// First set some config parameters to support the spad map
	struct tmf882x_mode_app_config tofConfig;
	if (!myTMF882X.getTMF882XConfig(tofConfig)) {
		Serial.println("Error - unable to get device configuration.");
		while(1){}
	}
    
    // Change the APP configuration
    //  - set the reporting period to 500 milliseconds
    //  - ID of the spad map.
    tofConfig.report_period_ms = 500;
    tofConfig.spad_map_id = 15;

	if (!myTMF882X.setTMF882XConfig(tofConfig)) {
		Serial.println("Error - unable to set device configuration.");
		while(1){}
	}

	// Set the SPAD Map
	if (!myTMF882X.setSPADConfig(spadConfig)) {
		Serial.println("Error - Setting SPAD config failed.");
		while(1){}
	}

	Serial.println("New SPAD Map set in device");
	

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
