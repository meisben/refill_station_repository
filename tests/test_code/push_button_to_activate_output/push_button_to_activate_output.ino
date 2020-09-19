/*
   ~~~~~Prerequisites~~~~~

   TBC

   ~~~~~Details~~~~~~~~~~~~

   Authors: Ben Money-Coomes
   Date: 7/7/20
   Purpose: Test buttons using MD_UISwitch library. Uses LCD shield for arduino. Activates output on pin 12
   References: See OneNote

   ~~~~Version Control~~~~~

   v1.0 - [Checkpoint] This version is working and printing the analog values of the button to the serial monitor
   v2.0 - [Checkpoint] This version is using the MD_UISwitch object and is working ! :)
   v3.0 - [Checkpoint] This version lights up the built in LED when the 'switch button' is pressed and is working!
   v3.1 - Select key press is now handled and detected in a tidy seperate function !
   v3.2 - [Checkpoint] When select button is pressed LED lights for short period
   v3.3 - changing LED to pin number 4
   v4.0 - [Checkpoint] When left button is pressed pin is active, when select button is pressed pin is inactive. Uses LCD shield.
   v4.1 - Adding values for button a2d ouput for alternative lcd shield.
   ----Notes---
   For the LCD Keypad shield from ebay the anaolgue input values at pin A0 are as follows:
   No button: 1023
   Select: 640
   Left: 410
   Up: 100
   Down: 257
   Right: 0

   Method is from the MD_UISwitch example

*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Library Includes.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <MD_UISwitch.h>
#include <LiquidCrystal.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Definitions                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//MD_UISwitch defintions

const uint8_t ANALOG_SWITCH_PIN = A0;       // switches connected to this pin

// These key values work for most LCD shields
//MD_UISwitch_Analog::uiAnalogKeys_t kt[] =
//{
//  { 0, 0, 'R' },  // Right
//  { 100, 15, 'U' },  // Up
//  { 257, 15, 'D' },  // Down
//  { 410, 15, 'L' },  // Left
//  { 640, 15, 'S' },  // Select
//};


// These key values work for the "D1 Robot" LCD shields (at least one of them!)

MD_UISwitch_Analog::uiAnalogKeys_t kt[] =
{
  { 0, 0, 'R' },  // Right
  { 131, 15, 'U' },  // Up
  { 307, 15, 'D' },  // Down
  { 480, 15, 'L' },  // Left
  { 720, 15, 'S' },  // Select
};

// Initialise Switch
MD_UISwitch_Analog S(ANALOG_SWITCH_PIN, kt, ARRAY_SIZE(kt));


// LCD Display -----------------------------
// LCD display definitions
#define  LCD_ROWS  2
#define  LCD_COLS  16

// LCD pin definitions - change to suit your shield
#define  LCD_RS    8
#define  LCD_ENA   9
#define  LCD_D4    4
#define  LCD_D5    LCD_D4+1
#define  LCD_D6    LCD_D4+2
#define  LCD_D7    LCD_D4+3

static LiquidCrystal lcd(LCD_RS, LCD_ENA, LCD_D4, LCD_D5, LCD_D6, LCD_D7);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Variables                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool pinFourStatus;

void setup() {

  // Start serial communications
  Serial.begin(9600);

  // Start the switch
  S.begin();
  S.enableLongPress(false);

  //Built in LED
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(12, OUTPUT);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  lcd.print("Pin four: low");
  lcd.setCursor(0, 1);
  lcd.print("Press 'left'");

  // initialise pinFourStatus as false;
  pinFourStatus = false;
  powerOffPinFour();

}

void loop() {

  actionButtonState(S.read());

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Functions                                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void actionButtonState(MD_UISwitch::keyResult_t state)
{
  if (state == MD_UISwitch::KEY_NULL)
    return;

  switch (state)
  {
    case MD_UISwitch::KEY_NULL:                                    break;
    case MD_UISwitch::KEY_UP:                                      break;
    case MD_UISwitch::KEY_DOWN:                                    break;
    case MD_UISwitch::KEY_PRESS:
      char key = (char)S.getKey();
      switch (key) {
        case 'L':
          if (pinFourStatus == false) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Pin 12: high");
            lcd.setCursor(0, 1);
            lcd.print("Press 'select'");
            Serial.println("Pin 12 active");
            powerOnPinFour();
            pinFourStatus = true;
            break;
          }

        case 'S':
          if (pinFourStatus == true) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Pin 12: low");
            lcd.setCursor(0, 1);
            lcd.print("Press 'left'");
            Serial.println("Press 'left'");
            powerOffPinFour();
            pinFourStatus = false;
            break;
          }

      }


    case MD_UISwitch::KEY_DPRESS:                                   break;
    case MD_UISwitch::KEY_LONGPRESS:                                break;
    case MD_UISwitch::KEY_RPTPRESS:                                 break;
    default:                         Serial.print("KEY_UNKNWN");    break;
  }

}


void powerOnPinFour()
{
  digitalWrite(12, HIGH);   // turn the pin on (HIGH is the voltage level)
}

void powerOffPinFour()
{
  digitalWrite(12, LOW);    // turn the pin off by making the voltage LOW
}
