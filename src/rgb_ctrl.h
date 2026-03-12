#pragma once
#include <Adafruit_NeoPixel.h>
static constexpr int BRIGHTNESS = 255;


Adafruit_NeoPixel matrix = Adafruit_NeoPixel(NUMPIXELS, RGB_LED, NEO_GRB + NEO_KHZ800);

void InitRGB(){
  matrix.setBrightness(BRIGHTNESS);
  matrix.begin();
  matrix.show();
}

void colorWipe(uint32_t c, uint8_t wait) 
{
  for(uint16_t i=0; i<matrix.numPixels(); i++) {
    matrix.setPixelColor(i, c);
    matrix.show();
    delay(wait);
  }
}


void RGBALLoff(){
  colorWipe(matrix.Color(0, 0, 0), 0);
}


void setSingleLED(uint16_t LEDnum, uint32_t c)
{
  matrix.setPixelColor(LEDnum, c);
  matrix.show();
}

void RGBoff(){
  setSingleLED(0, matrix.Color(0, 0, 0));
  setSingleLED(1, matrix.Color(0, 0, 0));
}

void RGBcolor(byte Rinput, byte Ginput, byte Binput){
  setSingleLED(0, matrix.Color(Rinput, Ginput, Binput));
  setSingleLED(1, matrix.Color(Rinput, Ginput, Binput));
}


void ctrlAllLED(int totalNum, int inputR, int inputG, int inputB){
  for(int i = 0; i<totalNum; i++){
    setSingleLED(i, matrix.Color(inputR, inputG, inputB));
    delay(1);
  }
}

