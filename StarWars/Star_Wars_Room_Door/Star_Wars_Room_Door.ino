// Code from https://www.instructables.com/Arduino-Security-Panel-System-With-Using-Keypad-an/

#include <Keypad.h>
#include <LiquidCrystal.h>

#define redLED 12 //define the LED pins
#define greenLED 11
#define Relay1 9  // HIGH == door OPEN
#define Relay2 10


LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

#define Password_Lenght 7 // Give enough room for six chars + NULL char

int pos = 0;    // variable to store the servo position

char Data[Password_Lenght]; // 6 is the number of chars it can hold + the null char = 7
char Master[Password_Lenght] = "052577";
byte data_count = 0, master_count = 0;
bool Pass_is_good;
char customKey;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
int door = 0;
bool once=true;

byte rowPins[ROWS] = {1, 2, 3, 4}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 6, 7, 8}; //connect to the column pinouts of the keypad

Keypad customKeypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS); //initialize an instance of class NewKeypad

void setup()
{
  lcd.begin(16, 2);
  lcd.print("  Door is reset");
  lcd.setCursor(0, 1);
  lcd.print("  --Stand By--");
  delay(3000);
  lcd.clear();
  pinMode(redLED, OUTPUT);  //set the LED as an output
  pinMode(greenLED, OUTPUT);
  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);

  // Force door open
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);
  digitalWrite(Relay1, HIGH);
  digitalWrite(Relay2, HIGH);
  lcd.print(" DO NOT TOUCH !!");
}

void loop()
{
  if (door == 0)
  {
    customKey = customKeypad.getKey();

    if (customKey == 'D')
    {
      lcd.clear();
      lcd.print("  Door Closing");
      delay(3000);
      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, LOW);
      digitalWrite(Relay1, LOW);
      digitalWrite(Relay2, LOW);
      door = 1;
    }
  }
  else if (door == 1) 
  {
    Open();
  }
  else // door == 2 
  {
      if (once) {
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("  Door is Open");
         delay(500);
         lcd.clear();
         delay(500);
//         once = false;
      }
  }
}

void clearData()
{
  while (data_count != 0)
  { // This can be used for any array size,
    Data[data_count--] = 0; //clear array for new data
  }
  return;
}


void Open()
{
  lcd.setCursor(0, 0);
  lcd.print(" Enter Passcode");
  
  customKey = customKeypad.getKey();
  if (customKey) // makes sure a key is actually pressed, equal to (customKey != NO_KEY)
  {
    Data[data_count] = customKey; // store char into data array
    lcd.setCursor(data_count, 1); // move cursor to show each new char
    lcd.print(Data[data_count]); // print char at said cursor
    data_count++; // increment data array by 1 to store new char, also keep track of the number of chars entered
  }

  if (data_count == Password_Lenght - 1) // if the array index is equal to the number of expected chars, compare data to master
  {
    if (!strcmp(Data, Master)) // equal to (strcmp(Data, Master) == 0)
    {
      lcd.clear();
      lcd.print("  Door is Open");
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, HIGH);
      digitalWrite(Relay1, HIGH);
      digitalWrite(Relay2, HIGH);
      door = 2;
    }
    else
    {
      lcd.clear();
      lcd.print(" Wrong Passcode");
      delay(2000);
      lcd.clear();
      door = 1;
    }
    clearData();
  }
}
