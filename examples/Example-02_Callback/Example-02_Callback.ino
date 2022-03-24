/*
TODO - Fix Header


*/
// Example 02 - using a callback to detect messages.

#include "SparkFun_TMF882X_Library.h"

SparkFun_TMF882X  myTMF882X;

#define NUMBER_OF_SAMPLES_TO_TAKE  4

int nSample =0;
// Define our measurement callback function

void onMeasurementCallback(TMF882XMeasurement_t *myResults){

	nSample++;

	Serial.print("Sample Number: ");
	Serial.println(nSample);

	// print out results
    Serial.println("Measurement:");
    Serial.print("Result Number: "); Serial.print(myResults->result_num);
    Serial.print(" Number of Results: "); Serial.println(myResults->num_results);    	

	for(uint32_t i = 0; i < myResults->num_results; ++i) {
	    Serial.print("    conf: "); Serial.print(myResults->results[i].confidence);
	    Serial.print(" distance mm: "); Serial.print(myResults->results[i].distance_mm);
	    Serial.print(" channel: "); Serial.print(myResults->results[i].channel);
	    Serial.print(" sub_capture: "); Serial.println(myResults->results[i].sub_capture);	

	}
	Serial.print(" photon: "); Serial.print(myResults->photon_count);	
	Serial.print(" ref photon: "); Serial.print(myResults->ref_photon_count);
	Serial.print(" ALS: "); Serial.println(myResults->ambient_light); Serial.println();

}

void setup(){

	delay(500);
	Serial.begin(115200);
	Serial.println("");


	if(!myTMF882X.begin())
	{
		Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
		while(1);
	}

	// set our call back function
	myTMF882X.setMeasurementHandler(onMeasurementCallback);

	// Set our delay between samples  - 1 second - note it's in ms
	myTMF882X.setSampleDelay(1000);
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
