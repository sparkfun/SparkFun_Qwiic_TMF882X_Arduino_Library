/*
TODO - Fix Header


*/
// Example 04 - end using a timeout

#include "SparkFun_TMF882X_Library.h"

SparkFun_TMF882X  myTMF882X;

#define SAMPLE_TIMEOUT_MS  3000

// Define our measurement callback function

void onMeasurementCallback(struct tmf882x_msg_meas_results *myResults){

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


	if(!myTMF882X.begin()){
		Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
		while(1);
	}

	// set our call back function
	myTMF882X.setMeasurementHandler(onMeasurementCallback);

	// Set our delay between samples  - 1 second - note it's in ms
	myTMF882X.setSampleDelay(700);
}

void loop()
{
	delay(2000);

	// get a measurment
	// Have the sensor take 4 measurements, the results are sent to the above callback

	Serial.println("---------------------------------------------------------");	
	Serial.print("Taking Samples over a period of: "); 
	Serial.print(SAMPLE_TIMEOUT_MS);
	Serial.println(" MS");
	Serial.println();
		
	// If number of desired samples is 0, the system loops forever - until timeout 
	// is hit, or stopped in the callback function.

	int nSamples = myTMF882X.startMeasuring(0, SAMPLE_TIMEOUT_MS);

	Serial.print("Took "); 
	Serial.print(nSamples);
	Serial.println(" data samples.");
	Serial.println();
	
	Serial.println("---------------------------------------------------------\n\n");	

}
