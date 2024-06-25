#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

void setup() {
  Serial.begin(9600);
  
  // initialize the OLED object
  if(!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  oled.setTextColor(WHITE);
  
}

void loop() {

  ClearOLED();
  WriteOLED(1, 0, 0, "Chupa Gouveia");
  DisplayOLED();
  delay(2000);
  ClearOLED();
  WriteOLED(2,0,0,"Seleciona");
  WriteOLED(2,0,16,"Jogo:");
  DisplayOLED();
  delay(2000);

  DrawLineAnimation(127, 1);
  delay(2000);
  DeDrawLineAnimation(128, 1);
  delay(2000);
}

//*****************************************************
//
// OLED
//
//*****************************************************

void SetupOLED(){
  // initialize the OLED object
  if(!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  } else{
    Serial.println("OLED Inicializado com Sucesso");
  }
}

void ClearOLED(){
  oled.clearDisplay();
}

void WriteOLED(int textSize, int x, int y, String text){
  oled.setTextSize(textSize);
  oled.setCursor(x,y);
  oled.println(text);
}

void DisplayOLED(){
  oled.display();
}

void DrawLineAnimation(int progress, int speed) {
  for (int i = 0; i < progress; i++) {
    DrawLine(i);
    delay(speed);
  }
}

void DrawLine(int xMax){
  for (int i = 18; i <= 32; i++) {
    oled.writeLine(0, i, xMax, i, WHITE);
  }
  DisplayOLED();
}

void DeDrawLineAnimation(int progress, int speed) {
  for (int i = 0; i < progress; i++) {
    DeDrawLine(127-i);
    delay(speed);
  }
}

void DeDrawLine(int xMin){
  for (int i = 18; i <= 32; i++) {
    oled.writeLine(xMin, i, 127, i, BLACK);
  }
  DisplayOLED();
}
