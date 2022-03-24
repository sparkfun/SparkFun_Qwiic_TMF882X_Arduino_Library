/*
TODO - Fix Header
*/


#include "SparkFun_TMF882X_Library.h"

SparkFun_TMF882X  myTMF882X;

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

}

void loop()
{
	delay(2000);

    // get a myResultsurment
    if(myTMF882X.startMeasuring(myResults))
    {
    	// print out results
    	Serial.println("Measurement:");
    	Serial.print("Result Number: "); Serial.print(myResults.result_num);
    	Serial.print(" Number of Results: "); Serial.println(myResults.num_results);    	

	    for (uint32_t i = 0; i < myResults.num_results; ++i) {
	    	Serial.print("    conf: "); Serial.print(myResults.results[i].confidence);
	    	Serial.print(" distance mm: "); Serial.print(myResults.results[i].distance_mm);
	    	Serial.print(" channel: "); Serial.print(myResults.results[i].channel);
	    	Serial.print(" sub_capture: "); Serial.println(myResults.results[i].sub_capture);	

	    }
	   	Serial.print(" photon: "); Serial.print(myResults.photon_count);	
	   	Serial.print(" ref photon: "); Serial.print(myResults.ref_photon_count);
	   	Serial.print(" ALS: "); Serial.println(myResults.ambient_light); Serial.println();

    }

}
