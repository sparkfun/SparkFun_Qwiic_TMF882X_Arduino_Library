/*
TODO - Fix Header
*/


#include <SparkFun_TMF882X_Library.h>

// Inlcude the firmware header file - this is from the TMF882X SDK, and ships
// with the SparkFun library. This header defines the symbols:
//
//	tof_bin_image   		- the Firmware image
//	tof_bin_image_length	- length/size of the firmware

#include <tof_bin_image.h>

SparkFun_TMF882X  myTMF882X;


static struct tmf882x_msg_meas_results myResults;

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

	Serial.println();
	// The library loads the firmware into the TMF882X device on startup (call to begin), 
	// which is normal. 
	//
	// This example shows how to upload new Firmware, if it's been released.


    if (!myTMF882X.loadFirmware(tof_bin_image, tof_bin_image_length))
    	Serial.println("ERROR - Failure to load new firmware into the TMF882X.");
    else
    	Serial.println("The new firmware was loaded successfully into the TMF882X.");
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
