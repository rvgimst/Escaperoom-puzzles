/*********
 Very simple LED all white full brightness
*********/

// FastLED includes
#include <FastLED.h>

// FastLED defines
#define NUM_LEDS 12 
#define DATA_PIN A0
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

// FastLED parameters
CRGB leds[NUM_LEDS];
int LEDBrightness = 255 ; 

void setup(){
  // Serial port for debugging purposes
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);
  
  // Init LED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(LEDBrightness); // set master brightness control
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
}
  
void loop() {
  for (int i=0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255,255,255); //::White;
  }
  
  FastLED.show();  
}
