/*
TODO - Fix Header


*/
// Example 02 - using a callback to detect messages.

#include "SparkFun_TMF882X_Library.h"

SparkFun_TMF882X  myTMF882X;

#define kNumberOfSamplesToTake  6

int nSample =0;
// Define our measurement callback function

void onMeasurementCallback(TMF882XMeasurement_t *measurement){

	nSample++;
	Serial.print("Sample Number: ");
	Serial.println(nSample);
	myTMF882X.printMeasurement(measurement);
	Serial.println();
}

void setup(){


	Serial.begin(115200);
	Serial.println("");


	if(!myTMF882X.begin()){
		Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
		while(1);
	}

	myTMF882X.setMeasurementHandler(onMeasurementCallback);
}

void loop()
{
	delay(2000);

	// get a measurment
	// Have the sensor take 4 measurements, the results are sent to the above callback

	Serial.println("---------------------------------------------------------");	
	Serial.println("Start Taking Measurements");
	nSample=0;

	myTMF882X.takeMeasurements(kNumberOfSamplesToTake);
	
	Serial.println("---------------------------------------------------------\n\n");	

}
