/**
 * "Valves" Puzzle
 *
 * To solve this puzzle, players must adjust a series of "valves" (potentiometers) to adjust the level 
 * of a set of corresponding pipes, as indicated by LED strips. 
 * When all pipes have been set to the correct level, the puzzle is solved and a relay is triggered, which
 * can release a maglock, activate a siren, light a lamp etc. etc.
 *
 * Demonstrates:
 * - LED Strips
 * - Potentiometers
 * 
 * Updated 20190915 : Changed pin numbers from 3/4/5 to 10/9/8 to reflect wiring used in video
 */

// INCLUDES
// Library for controlling LED strips, available from https://github.com/FastLED/FastLED
#include "FastLED.h"

// DEFINES
// Flag to toggle debugging output
// #define DEBUG

// There are only *9* LEDs in each strip representing the numbers 1-9.
// No need for 10 LEDs, because for the digit 0 all the LEDs are simply turned off!
#define numLEDsInEachStrip 9

// CONSTANTS
// The input pins to which the potentiometers is connected. NOTE - Must use analog (Ax) input pins!
const byte potPins[] = {A0, A1, A2};
// The total number of valves
const byte numPots = 3;
// The level to which each valve must be set to solve the puzzle
const int solution[numPots] = {5, 2, 9};
// This digital pin will be driven LOW to release a lock when puzzle is solved
const byte lockPin = A5;
// NOTE: the pins to which the LED strips are connected is hardcoded in setup, not here.

// GLOBALS
// This array will record the current reading of each input valve
int currentReadings[numPots] = {};
// An multidimensional LED array - one element for each LED, arranged into separate arrays for each potentiometer
CRGB leds[numPots][numLEDsInEachStrip];
// To make the LED colours more interesting, we'll add some random noise offset
static uint16_t noiseOffset[3];
// This is the array that we keep our computed noise values in
uint8_t noise[numPots][numLEDsInEachStrip];
// Track state of overall puzzle
enum PuzzleState {Initialising, Running, Solved};
PuzzleState puzzleState = Initialising;

/**
 * Returns true if the puzzle has been solved, false otherwise
 */
bool checkIfPuzzleSolved(){
  for(int i=0; i<numPots; i++) {
    if(currentReadings[i] != solution[i]) {
      return false;
    }
  }
  return true;
}

/**
 * Called when the puzzle is solved
 */
void onSolve() {
  #ifdef DEBUG
    Serial.println("Puzzle has just been solved!");
  #endif
  
  // Release the lock
  digitalWrite(lockPin, LOW);
  
  puzzleState = Solved;
}

/**
 * Called when the puzzle (which previously had been solved) becomes unsolved
 */
void onUnsolve() {
  #ifdef DEBUG
    Serial.println("Puzzle has just become unsolved!");
  #endif

  // Lock the lock again
  digitalWrite(lockPin, HIGH);
  
  puzzleState = Running;
}

/**
 * Initialisation
 */
void setup(){
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);

  // Set the linear pot pins as input
  for(int i=0; i<numPots; i++){
    // Set the pin for the pot
    pinMode(potPins[i], INPUT);
  }

  // Create and assign the arrays of LEDs on the pins corresponding to each pot
  // NOTE: The template initialisation function used by the FastLED library requires pin numbers to be constants defined at compile-time.
  // Therefore we can't loop over an array of pin numbers as we do with the pots, and instead manually call addLeds() several times with hardcoded pin values
  FastLED.addLeds<NEOPIXEL, 10>(leds[0], numLEDsInEachStrip);
  FastLED.addLeds<NEOPIXEL, 9>(leds[1], numLEDsInEachStrip);
  FastLED.addLeds<NEOPIXEL, 8>(leds[2], numLEDsInEachStrip);
 
  // Set the lock pin as output and secure the lock
  pinMode(lockPin, OUTPUT);
  digitalWrite(lockPin, HIGH);

  // Initialise some random noise values to add variety to the LED colours
  //noiseOffset[0] = random16();
  //noiseOffset[1] = random16();
  //noiseOffset[2] = random16();

  // Set the puzzle into the running state
  puzzleState = Running;
}


// Create an array of "noise" - one value for each LED
void fillnoise8(uint16_t speed, uint16_t scale) {
  for(int i = 0; i < numPots; i++) {
    for(int j = 0; j < numLEDsInEachStrip; j++) {
      noise[i][j] = inoise8(noiseOffset[0] + (i*scale), noiseOffset[1] + (j*scale), noiseOffset[2]);
    }
  }
  noiseOffset[2] += speed;
}

// The Arduino map() function produces incorrect distribution for integers. See https://github.com/arduino/Arduino/issues/2466
// So, create a fixed function instead:
long map2(long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min+1) / (in_max - in_min+1) + out_min;
}

/**
 *  Read the input from the potentiometers and store in the currentReadings array
 */
void getInput() {
  // Read the value from the pots
  for(int i=0; i<numPots; i++){
    // Get the "raw" input, which is a value from 0-1023
    int rawValue = analogRead(potPins[i]);
    // Scale the value to the number of LEDs in each strip
    int scaledValue = map2(rawValue, 0, 1023, 0, numLEDsInEachStrip);
    // To ensure we don't get any dodgy values, constrain the output range too
    scaledValue = constrain(scaledValue, 0, numLEDsInEachStrip);
    // Store the scaled value in the currentReadings array
    currentReadings[i] = scaledValue;
    // Print some debug information
    #ifdef DEBUG
      Serial.print("Valve ");
      Serial.print(i);
      Serial.print(" raw:");
      Serial.print(rawValue);
      Serial.print(", scaled:");
      Serial.println(scaledValue);
      delay(50);
    #endif
  }
}

/**
 * Set the LEDs in the chosen style
 */
void setDisplay(int style = 0) {

  // Fill the noise array with values
  // The first parameter determines the speed and the second the scale of the generated noise. Try playing with them!
  fillnoise8(5, 255);
  
  // Clear the LEDs
  FastLED.clear();

  // Loop over each input
  for(int i=0; i<numPots; i++){
    // Now count up from 0 to the reading for this LED
    for(int x=0; x<currentReadings[i]; x++){
      // Colour the LEDs according to the chosen style
      switch(style){
        case 0:
          // Noisy (i.e. random) colours
          leds[i][x] = CHSV(noise[i][x], 255, 160);
          break;
        case 1:
          // Blue hue but with some variation in hue and brightness
          leds[i][x] = CHSV(
            150 + (int)(noise[i][x]>>2),
            255,
            constrain(16 + (2<<x) + (int)(noise[i][x]>>1), 0, 255)
          );
          break;
        default:
          // Block solid colour
          leds[i][x] = CRGB::Blue;
          break;
      }
    }
    // Colour the top LED of each line
    if(currentReadings[i] > 0) {
      leds[i][currentReadings[i]-1] = CRGB(255,255,255);
    }
  }

  // Having assigned the colour, show the LEDs
  FastLED.show();
}

/**
 * Main program loop runs indefinitely
 */
void loop(){

  // Switch action based on the current state of the puzzle
  switch(puzzleState) {
    case Initialising:
      puzzleState = Running;
      break;
      
    case Running:
      getInput();
      setDisplay(1);
      if(checkIfPuzzleSolved()){ 
        onSolve();
      }
      break;
      
    case Solved:
    setDisplay(0);
    /*
      getInput();
      setDisplay(0);
      if (!checkIfPuzzleSolved()){
        onUnsolve();
      }
      */
      break;
  }
}
