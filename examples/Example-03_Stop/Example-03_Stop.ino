/*
TODO - Fix Header


*/
// Example 03 - using a callback to detect messages and calling stopMeasuring() to stop the effort.

#include "SparkFun_TMF882X_Library.h"

SparkFun_TMF882X  myTMF882X;

#define NUMBER_OF_SAMPLES_TO_TAKE  4

int nSample =0;
// Define our measurement callback function

void onMeasurementCallback(TMF882XMeasurement_t *measurement){

	nSample++;
	Serial.print("Sample Number: ");
	Serial.println(nSample);
	myTMF882X.printMeasurement(measurement);
	Serial.println();

	// If we are at our limit, stop taking measurments

	if(nSample == NUMBER_OF_SAMPLES_TO_TAKE){
		myTMF882X.stopMeasuring();
		Serial.println();
		Serial.println("-----------------------------------------------");
		Serial.println("Measurement Goal Hit - stopping measurements.");
		Serial.println("-----------------------------------------------");
		Serial.println();		
	}
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
	myTMF882X.startMeasuring();
	
	Serial.println("---------------------------------------------------------\n\n");	

}
