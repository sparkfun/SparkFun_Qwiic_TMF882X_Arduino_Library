#include <Wire.h>
#include "QwiicTMF882X.h"

QwiicTMF882X mydToF; 

#define PWR_EN 6

void setup(){

  Serial.begin(115200);
  while(!Serial) { delay(100); }

  Wire.begin();
  if ( mydToF.begin() ) 
    Serial.println("Wooomp Weeee Wooooo."); 

	mydToF.enableApp();

	while( mydToF.getDeviceStatus() != 0x41 )
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println();

	Serial.print("0x");
	Serial.println(mydToF.getAppID(), HEX);

	mydToF.powerSelect(0x02);

	mydToF.setCommand(0x11);
	delay(5);

	Serial.print("0x");
	Serial.println(mydToF.getAppID(), HEX);

	Serial.print("Status (Should read 0x22): 0x");
	Serial.println(mydToF.getDeviceStatus(), HEX);


	delay(500);

	Serial.print("ID (Should read 0x0300): 0x");
	Serial.println(mydToF.getAppID(), HEX);

	Serial.println("Ready.");
	while(1);
}

void loop(){
  Serial.print("T in Celsius: ");
  Serial.println(mydToF.getTemp());
  delay(100);
}
