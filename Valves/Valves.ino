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
/**
 * RVG 12NOV22 added code for addressing LED Matrix using the NEO Matrix library
 */

// INCLUDES
// Library for controlling LED strips, available from https://github.com/FastLED/FastLED
#include "FastLED.h"

// DEFINES
// Flag to toggle debugging output
// #define DEBUG
// #define ALLOW_UNSOLVE

#define SNDUP 0
#define SNDDOWN 1
#define SNDSOLVED 4

// There are only *9* LEDs in each strip representing the numbers 1-9.
// No need for 10 LEDs, because for the digit 0 all the LEDs are simply turned off!
#define numLEDsInEachStrip 9

// CONSTANTS
// The input pins to which the potentiometers is connected. NOTE - Must use analog (Ax) input pins!
const byte potPins[] = {A0, A1, A2};
// The total number of valves
const byte numPots = 3;
// The level to which each valve must be set to solve the puzzle
const int solution[numPots] = {9, 2, 5};
// This digital pin will be driven LOW to release a lock when puzzle is solved
const int hystThreshold = 30; 
// sound pins. These just set digital pins to high/low for a certain short period to trigger soundboard
const byte sndUpPin = 2;
const byte sndDownPin = 3;
const byte sndSolvedPin = 6;
const int soundTriggerPeriod = 100; // ms
// Relay pin for triggering powering on...anything
const byte relayPin = 7;
// Specific colors for the different strips
const uint8_t stripHue[numPots] = {HUE_ORANGE, HUE_RED, HUE_GREEN};
// NOTE: the pins to which the LED strips are connected is hardcoded in setup, not here.
const int keyCorrectTimeout = 1000; // ms

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
enum PuzzleState {Initialising, Running, KeyCorrect, Solved};
PuzzleState puzzleState = Initialising;
// timer variables 
// Timer for audio LOW signal. This eliminates using blocking delay() function
unsigned long soundTimerStart;
bool soundTimerArmed = false;
// Timer for period between key correct and puzzle solved
unsigned long solvedTimerStart;

/////////////////////////////////
// NEO Matrix includes & defines
/////////////////////////////////
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define MATRIX_PIN A3
// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_GRBW    Pixels are wired for GRBW bitstream (RGB+W NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, MATRIX_PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

///////////////////////
// NEOMatrix variables
///////////////////////
// red, green blue
const uint16_t matrixColors[] = {
  matrix.Color(255, 0, 0),
  matrix.Color(0, 255, 0),
  matrix.Color(0, 0, 255)
};

int matrixPosCnt = matrix.width();
char matrixUnsolvedText[] = "Computers offline";
char matrixSolvedText[] = "Computers online [NESW]";
char matrixOutStr[50]; // space for 50-char text
byte matrixColIdx = 0;
int matrixCharWidth = 6; // width of font in pixels

// We need the arrow symbols from the ASCI table that is used by the Adafruit NeoMatrix library,
// actually the adafruit GFX library contains the functions for drawing texts
// ASCII codes used from https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
// arrow up   (^): 24 (0x18)
// arrow down (v): 25 (0x19)
// arrow right(>): 26 (0x1A)
// arrow left (<): 27 (0x1B)
// The array below contains the sequence of 4 arrows (access code) that you want displayed (eg <^^v)
int arrowCode[4] = { 0x1B, 0x18, 0x18, 0x19 };
// The next array indicates the positions in the SolvedText that contain the placeholders
// (1st character in string has index 0!)
int arrowPlaceholder[4] = { 18, 19, 20, 21 };

/**
 * Initialisation
 */
void setup(){
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);

  delay(500);
  randomSeed(analogRead(7)); 
  
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
 
  // Set the relay pin as output and set to LOW
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Setup the sound pins
  pinMode(sndUpPin, OUTPUT);
  digitalWrite(sndUpPin, HIGH);
  pinMode(sndDownPin, OUTPUT);
  digitalWrite(sndDownPin, HIGH);
  pinMode(sndSolvedPin, OUTPUT);
  digitalWrite(sndSolvedPin, HIGH);

  // Initialise some random noise values to add variety to the LED colours
  //noiseOffset[0] = random16();
  //noiseOffset[1] = random16();
  //noiseOffset[2] = random16();

  // NEO Matrix setup
  // this is a bit of a hack
  // several arrow symbols from the ASCII table are written
  // over the placeholders in the SolvedText array 
  for (int i=0; i<4; i++) {
    matrixSolvedText[arrowPlaceholder[i]] = (char)arrowCode[i];
  }
  strcpy(matrixOutStr, matrixUnsolvedText);
  matrixColIdx = 0;
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(matrixColors[matrixColIdx]);

  // Set the puzzle into the running state
  puzzleState = Running;
}

/**
 * Returns true if the input key is correct, false otherwise
 */
unsigned long mainTimer = millis();
bool checkIfKeyCorrect(){
//  // TEST TEST TEST
//  #define ALLOW_UNSOLVE
//  static bool correct = false;
//  
//  if (millis() - mainTimer > 10000) { // simulate correct key after 10s
//    correct = !correct;
//    mainTimer = millis();
//  }
//  return correct;
//  // /TEST
  
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
  
  // TODO: Play sound indicating puzzle is solved
  
  delay(100); // was 1000, don't know why
  // Trigger the Relay
  digitalWrite(relayPin, HIGH);
  triggerSound(SNDSOLVED, 0);

  // set text on matrix to the solved text & color
  matrixColIdx = 1;
  strcpy(matrixOutStr, matrixSolvedText);
  
  puzzleState = Solved;
}

/**
 * Called when the puzzle (which previously had been solved) becomes unsolved
 */
void onUnsolve() {
  #ifdef DEBUG
    Serial.println("Puzzle has just become unsolved!");
  #endif

  // Shut off the relay
  digitalWrite(relayPin, LOW);

  // set text on matrix to the unsolved text & color
  matrixColIdx = 0;
  strcpy(matrixOutStr, matrixUnsolvedText);
  
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

void triggerSound(int soundType, int value) {
  byte soundPin;
  switch(soundType) {
    case SNDUP:
      soundPin = sndUpPin;
      break;
    case SNDDOWN:
      soundPin = sndDownPin;
      break;
    case SNDSOLVED:
      soundPin = sndSolvedPin;
      break;
    default:
      return; // no sound
  }
  digitalWrite(soundPin, LOW);
  soundTimerStart = millis();
  soundTimerArmed = true;
}

/*
 * This function uses a timer to determine if the soundpins should be set back to high
 * in order for sounds to be played only once.
 * It uses the state of the puzzle, because the solved sound should play continuously
 */
void checkSoundOutputs() {
  if (soundTimerArmed && (millis() - soundTimerStart) > soundTriggerPeriod) {
    Serial.println("checkSoundOutputs: resetting UP/DOWN audio pins HIGH");
    digitalWrite(sndUpPin, HIGH);
    digitalWrite(sndDownPin, HIGH);
    if (puzzleState != Solved) { // set SolvedPin to HIGH if not solved
      Serial.println("                   resetting SOLVED audio pin HIGH");
      digitalWrite(sndSolvedPin, HIGH);
    }
    soundTimerArmed = false;
  }
}

/**
 *  Read the input from the potentiometers and store in the currentReadings array
 */
void getInput() {
  int scaledValue;
  // Read the value from the pots
  for(int i=0; i<numPots; i++){
    #ifdef DEBUG
      Serial.print("Valve ");
      Serial.print(i);
    #endif
    // Get the "raw" input, which is a value from 0-1023
    int rawValue = analogRead(potPins[i]);
    
    //rawValue += random(-10, 10); // for testing with hysteresis
    //rawValue = constrain(rawValue, 0, 1023);

    // Scale the value to the number of LEDs in each strip
    // take into account hysteresis for determining up or down movement of scaled value
    int hystLowRawValue  = constrain(rawValue - hystThreshold, 0, 1023);
    int hystHighRawValue = constrain(rawValue + hystThreshold, 0, 1023);
    int scaledValueDown = map2(hystHighRawValue, 0, 1023, 0, numLEDsInEachStrip);
    int scaledValueUp   = map2(hystLowRawValue, 0, 1023, 0, numLEDsInEachStrip);
    // To ensure we don't get any dodgy values, constrain the output range too
    scaledValueDown = constrain(scaledValueDown, 0, numLEDsInEachStrip);
    scaledValueUp   = constrain(scaledValueUp, 0, numLEDsInEachStrip);
    if (scaledValueDown < currentReadings[i]) { // Change below hysteresis threshold
      scaledValue = scaledValueDown;
    } else if (scaledValueUp > currentReadings[i]) { // Change above hysteresis threshold
      scaledValue = scaledValueUp;
    } else {
      // no change of output/scaled value
      scaledValue = currentReadings[i];
    }

    // Play sound depending on change in value
    if (scaledValue > currentReadings[i]) {
      triggerSound(SNDUP, scaledValue);
    } else if (scaledValue < currentReadings[i]) {
      triggerSound(SNDDOWN, scaledValue);
    }

    // Store the scaled value in the currentReadings array
    currentReadings[i] = scaledValue;
    
    // Print some debug information
    #ifdef DEBUG
    EVERY_N_MILLISECONDS(50) {
      Serial.print(" raw:");
      Serial.print(rawValue);
      Serial.print(", scaled:");
      Serial.println(scaledValue);
    }
    #endif
  }
}

/**
 * Set the LEDs in the chosen style
 */
void setDisplay(int style = 0) {

  // Fill the noise array with values
  // The first parameter determines the speed and the second the scale of the generated noise. Try playing with them!
  fillnoise8(3, 128);
  
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
            stripHue[i] + (int)(noise[i][x]>>2),
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

void updateMatrix() {
  EVERY_N_MILLISECONDS(40) { // 100 for slow movement (debug)
#ifdef DEBUG
    Serial.print("text: ");
    Serial.print(matrixOutStr);
    Serial.print("  colidx: ");
    Serial.print(matrixColIdx);
    Serial.print("  len: ");
    Serial.print(strlen(matrixOutStr));
    Serial.print("  width: ");
    Serial.print(matrixCharWidth);
    Serial.print("  <calc>: ");
    Serial.print((strlen(matrixOutStr) + 1)*matrixCharWidth);   
    Serial.print("  poscnt: ");
    Serial.println(matrixPosCnt);    
#endif

    matrix.fillScreen(0);
    matrix.setCursor(matrixPosCnt, 0);
    matrix.print(matrixOutStr);
    int len = strlen(matrixOutStr);
    if (--matrixPosCnt < -(len + 1)*matrixCharWidth) {
      matrixPosCnt = matrix.width();
    }
    matrix.setTextColor(matrixColors[matrixColIdx]);
    matrix.show();
  }
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
      if (checkIfKeyCorrect()) {
        Serial.println("Key is correct. Waiting...");
        solvedTimerStart = millis();
        //onSolve();
        puzzleState = KeyCorrect;
      }
      break;

    case Solved:
      setDisplay(0);
      #ifdef ALLOW_UNSOLVE
        getInput();
        setDisplay(0);
        if (!checkIfPuzzleSolved()){
          onUnsolve();
        }
      #endif 
      break;

    case KeyCorrect:
      // in this state the Key is constantly checked. 
      // If key is correct during a certain time period, the state transfers to solved.
      // This state prevents the puzzle being solved by accident
      getInput();
      setDisplay(1);
      if (checkIfKeyCorrect()) { 
        if (millis() - solvedTimerStart > keyCorrectTimeout) { // key stayed correct during timeout
          //Serial.println("Key timeout successful. Transitioning to Solved state.");
          onSolve();
        }
      } else {
        puzzleState = Running; // back to normal state if key was changed during Timeout
      }
      break; 
  }

  // NEO Matrix loop code
  updateMatrix();

  // check sound pin output states
  checkSoundOutputs();
}
