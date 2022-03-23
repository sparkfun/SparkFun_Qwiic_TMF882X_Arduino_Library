/*
TODO - Fix Header
*/


#include "SparkFun_TMF882X_Library.h"

SparkFun_TMF882X  myTMF882X;

static TMF882XMeasurement_t myResults;

void setup(){

	delay(500);
	Serial.begin(115200);
	Serial.println("");


	if(!myTMF882X.begin()){
		Serial.println("Error - The TMF882X failed to initialize - is the board connected?");
		while(1);
	}
}

void loop()
{
	delay(2000);

    // get a measurment
    if(myTMF882X.startMeasuring(myResults))
    {
    	// print out results
    	Serial.println();
   		myTMF882X.printMeasurement(myResults);
    }

}
