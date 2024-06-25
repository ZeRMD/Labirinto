#include <Wire.h>
#include "Grove_LED_Matrix_Driver_HT16K33.h"

#define addr0 0x70

Matrix_8x8 matrix0;

void setup() {
  
  Wire.begin();

  matrix0.init(addr0);
  matrix0.setBrightness(0);
  matrix0.setBlinkRate(BLINK_OFF);
  matrix0.setDisplayOrientation(0);

  matrix0.clear();
  matrix0.display();
}

void loop() {
  // put your main code here, to run repeatedly:
  matrix0.writeString("A FUNCIONAR", 1000, ACTION_SCROLLING);
  matrix0.display();
}
