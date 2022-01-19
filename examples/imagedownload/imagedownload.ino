#include <Wire.h>
#include "QwiicTMF882X.h"

// 0x08 -> 0x14 0x01 0x29 0xC1
// Poll 0x08 ---> 0x00, 0x00, 0x0FF

uint8_t downloadInit[] = {0x14, 0x01, 0x29, 0xC1};

uint8_t setAddress[] = {0x43,0x02,0x00,0x00,0xBA};

// Magic numbers pulled directly from the HOST USER GUIDE pg. 12
uint8_t writeRAM[] = {0x41,0x20,0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x76,
											0x75, 0x74, 0x73, 0x72, 0x71, 0x70,0x5F,0x5E,0x5D,0x5C,0x5B,
											0x5A,0x59,0x58, 0x57,0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0x50, 0xAE};

uint8_t numVals = 3;
uint8_t successVals[numVals] = NULL; 

QwiicTMF882X mydToF; 

#define PWR_EN 6

void setup(){

  Serial.begin(115200);
  while(!Serial) { delay(100); }

  Wire.begin();
  if ( mydToF.begin() ) 
    Serial.println("Wooomp Weeee Wooooo."); 

	// Enable the chip in software - EN pin already pulled high by default
	mydToF.enableApp();

	while( mydToF.getDeviceStatus() != 0x41 )
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println();

	// Looking for bootloader APP ID 0x08, minor rev 0x29
	Serial.print("AppID (Should be 0x0829): 0x");
	Serial.println(mydToF.getAppID(), HEX);

	// Download INIT command
	if( !mydToF.setImage(*downloadInit) ){
		Serial.print("Error.");
		while(1);
	}
	
	delay(100);
	mydToF.getCommandStat(successVals, numVals);

	for( int i = 0; i < numVals; i++ ){

		Serial.print(successVals[i]);
	}
	

	if( !mydToF.setImage(*setAddress) ){
		Serial.print("Error.");
		while(1);
	}
	
	delay(100);
	mydToF.getCommandStat(successVals, numVals);

	for( int i = 0; i < numVals; i++ ){

		Serial.print(successVals[i]);
	}

	Serial.println("Ready.");



	if( !mydToF.setImage(*writeRAM) ){
		Serial.print("Error.");
		while(1);
	}
	
	delay(100);
	mydToF.getCommandStat(successVals, numVals);

	for( int i = 0; i < numVals; i++ ){

		Serial.print(successVals[i]);
	}

	// Issue Reset
	if( !mydToF.issueReset() ){
		Serial.println("Did not Reset Properly.");
		while(1);
	}

	// Suggests 2.5ms in USER GUIDE. 
	delay(5);

	// Looking for bootloader APP ID 0x03, minor rev 0x00
	Serial.print("AppID (Should be 0x0300): 0x");
	Serial.println(mydToF.getAppID(), HEX);

	Serial.println("Ready.");
	while(1);
}

void loop(){
}
