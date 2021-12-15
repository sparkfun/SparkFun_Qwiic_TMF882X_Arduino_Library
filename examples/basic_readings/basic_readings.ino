#include <Wire.h>
#include "QwiicTMF882X.h"

QwiicTMF882X mydToF(0x41); 

void setup(){

  Serial.begin(115200);
  while(!Serial) { delay(100) };
  Serial.print("Nice.");

  Wire.begin();
  myDToF.begin();

}

void loop(){
}
