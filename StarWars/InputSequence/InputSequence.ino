/**
* Input Sequence
* 
* Alastair Aitchison (c) 2017
* 
* This puzzle requires the user to enter a set of inputs in the correct order. 
* i.e. press a sequence of buttons. 
* The number of inputs and the length of sequence are customisable.
*/

// DEFINES
// Provides debugging information over serial connection
#define DEBUG

// CONSTANTS
// Define the number of possible "inputs" - i.e. the number of switches, buttons etc. that the player can press
const byte numInputs = 5;
// What pins are those buttons connected to? (other wire should go to ground)
const byte inputPins[numInputs] = {6, 5, 4, 3, 2};
// Define the number of steps in the sequence that the player must follow
const byte numSteps = 5;
// The correct sequence of inputs required to solve the puzzle.
const byte steps[numSteps] = {0, 1, 2, 3, 4}; // i.e. press button #2 once, then button #3 twice, then button #0, then button #1.
// These pins are used to light up LEDs to show the player's progress, so one output pin per step in the puzzle.
const byte ledPins[numSteps] = {12, 11, 10, 9, 8};
// This pin will be driven LOW to release a lock when puzzle is solved
const byte lockPin = A0;

// GLOBALS
// Assume the default state of each switch is HIGH. 
bool lastInputState[] = {HIGH, HIGH, HIGH, HIGH};
// What step of the sequence is the player currently on?
int currentStep = 0;

// Switches can "bounce" when they open/close, generating a flurry of false readings
// To prevent this, we'll add a short delay between each time an input value
// is read.
// The last time the input switch was toggled
unsigned long lastDebounceTime = 0;
// The amount of time (in ms) to wait before reading again 
unsigned long debounceDelay = 50;

void StartupLEDSequence() {
  // Abusing inputpins temporarily as output pins for audio
  for(int i=0; i< numInputs; i++){
    pinMode(inputPins[i], OUTPUT);
    digitalWrite(inputPins[i], HIGH); // no sound
  }

  for (int j=0; j<4; j++) {
    for(int i=0; i< numSteps; i++){
      digitalWrite(ledPins[i], HIGH);
      digitalWrite(inputPins[2], LOW);
    }
    delay(500);
    for(int i=0; i< numSteps; i++){
      digitalWrite(ledPins[i], LOW);
      digitalWrite(inputPins[2], HIGH);
    }
    delay(500);
  }
}

// Setup function runs once when first starting (or resetting) the board
void setup() {
  #ifdef DEBUG
    // Open communications on serial port
    Serial.begin(9600);
    Serial.println(F("Serial communication started"));
  #endif

  delay(500); // get the Arduino and other hardware to rest first

  // Initialise the LED pins that show progress through the sequence 
  for(int i=0; i< numSteps; i++){
    pinMode(ledPins[i], OUTPUT);
  }

  // Initialise the input pins that have switches attached
  for(int i=0; i< numInputs; i++){
    pinMode(inputPins[i], INPUT_PULLUP);
  }
  
  StartupLEDSequence(); // Just a visual cue to indicate puzzle is starting up
  
  // Set the lock pin as output and secure the lock
  pinMode(lockPin, OUTPUT);
  digitalWrite(lockPin, HIGH);

}

// The main program loop runs continuously
void loop() {

  // Check that we've waited at least "debounceDelay" since last input
  if ( (millis() - lastDebounceTime) > debounceDelay) {
  
    // Loop through all the inputs
    for(int i=0; i<numInputs; i++){
      int currentInputState = digitalRead(inputPins[i]);

      // If the input has changed, reset the debounce timer
      if(currentInputState != lastInputState[i]) {
        lastDebounceTime = millis();    
      }
        
      // If the input is currently being pressed (and wasn't before)
      // Note that since the input pins are configured as INPUT_PULLUP,
      // they read as LOW when pressed and HIGH when not.
      if(currentInputState == LOW && lastInputState[i] == HIGH) {
        // Was this the correct input for this step of the sequence?
        if(steps[currentStep] == i) {
          currentStep++;
          
          #ifdef DEBUG
            Serial.print(F("Correct input! Onto step #"));
            Serial.println(currentStep);
          #endif
        }
        // Incorrect input!
        else {
          currentStep = 0;
          Serial.println(F("Incorrect input! Back to the beginning!"));
        }
      }
      
      // Update the stored value for this input
      lastInputState[i] = currentInputState;
    }
  }

  // Check whether the puzzle has been solved
  if(currentStep == numSteps){
    onSolve();
  }

  // Turn on the number of LEDs corresponding to the current step
  for(int i=0; i<numSteps; i++){
    digitalWrite(ledPins[i], (i < currentStep ? HIGH : LOW));
  }
}

// Takes action when the puzzle becomes solved
void onSolve(){

  #ifdef DEBUG
    // Print a message
    Serial.println(F("Puzzle Solved!"));
  #endif 

  // Release the lock
  digitalWrite(lockPin, LOW);

  // Loop forever
  while(true){

    // Flash LEDs
    for(int i=0; i<numSteps; i++){
      digitalWrite(ledPins[i], HIGH);
      delay(100);  
    }
    delay(100);  
    for(int i=0; i<numSteps; i++){
      digitalWrite(ledPins[i], LOW);
      delay(100);  
    }
  }
}
