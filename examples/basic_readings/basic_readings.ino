#include <Wire.h>
#include "QwiicTMF882X.h"

QwiicTMF882X mydToF; 

void setup(){

  Serial.begin(115200);
  while(!Serial) { delay(100); }

  Wire.begin();
  if ( mydToF.begin() ) 
    Serial.println("Wooomp Weeee Wooooo."); 

  Serial.println(mydToF.getDeviceID(), HEX); 
  while(1);

}

void loop(){
}
