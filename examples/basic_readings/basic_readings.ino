#include <Wire.h>
#include "QwiicTMF882X.h"

QwiicTMF882X mydToF; 

void setup(){

  Serial.begin(115200);
  while(!Serial) { delay(100); }

  Wire.begin();
  if ( mydToF.begin() ) 
    Serial.println("Wooomp Weeee Wooooo."); 

  Serial.print("0x");
  Serial.println(mydToF.getAppID(), HEX); 

  Serial.print("0x");
  Serial.println(mydToF.getDeviceID(), HEX); 

  Serial.print("0x");
  Serial.println(mydToF.getAppStatus(), HEX); 

  Serial.print("0x");
  Serial.println(mydToF.getMeasStat(), HEX); 

  mydToF.setCommand(START_MEAS);
	delay(10);
  Serial.print("0x");
  Serial.println(mydToF.getAppID(), HEX); 
	while(1);
}

void loop(){
  Serial.print("T in Celsius: ");
  Serial.println(mydToF.getTemp());
  delay(100);
}
