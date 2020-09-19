/*
   ~~~~~Prerequisites~~~~~

   TBC

   ~~~~~Details~~~~~~~~~~~~

   Authors: Ben Money-Coomes
   Date: 7/7/20
   Purpose: Strain gauge functionality for displaying values from the refill strain gauge
   References: See OneNote

   ~~~~Version Control~~~~~

   v1.0 - [Checkpoint] This version is working and printing values to the serial monitor
   v1.1 - Working on investigating the functionality of the HX711 loadcell, also printing a value to the LCD
   v1.2 - [Checkpoint] Script to calculate scaling factor for the scale. Will be saved seperately! ->> as hx711_calibration_script_v1.0
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Library Includes.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <HX711.h>
#include <LiquidCrystal.h>
#include <MD_UISwitch.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Definitions                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//MD_UISwitch defintions

const uint8_t ANALOG_SWITCH_PIN = A0;       // switches connected to this pin

//// These key values work for most LCD shields
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

// HX711 strain gauge -----------------------------
// HX711  definitions
HX711 scale;
long int values = 0;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// Note that when using the LCD Keypad Shield then pin 2 is the 3rd pin from the rhs and pin 3 is the 4th pin from the rhs

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Variables                                                        *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

float knownMass = 450; // in grams
float lastRawResult;
float scalingFactor = 1;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Start of main program                                                          *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void setup()
{
  // Start serial communications
  Serial.begin(9600);

  // Start communicating with the loadcell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Start the switch
  S.begin();
  S.enableLongPress(false);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Print a serial mesage with instructions
  Serial.println(F("Remove all objects from the scale/loadcell, and then press the 'select' key to tare/zero the scale"));
  Serial.println(F(""));
  
  // Print a message to the LCD.
  lcd.print("Remove objects,");
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  lcd.print("'S' -> to tare");
}

void loop()
{
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
       
        case 'S': //select button
          tareScale_reset();
          break;

        case 'L': //left button
          showRawResult();
          break;

        case 'U': //up button
          calcScalingFactorFromLastRawReading();
          break;

        case 'D': //down button
          showCalibratedResult();
          break;

      }


    case MD_UISwitch::KEY_DPRESS:                                   break;
    case MD_UISwitch::KEY_LONGPRESS:                                break;
    case MD_UISwitch::KEY_RPTPRESS:                                 break;
    default:                         Serial.print("KEY_UNKNWN");    break;
  }

}

void tareScale_reset(){
  scale.set_scale(); //as per readme, sets scalling factor to 1 (no scaling)
  scale.tare(); // reset the scale to 0

  Serial.println(F("Scale has been zeroed..."));
  Serial.println(F("Place a known mass on the scale, then press the 'left' button to read a raw value from the Analog Digital Converter (ADC)"));
  Serial.println(F(""));

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scale zeroed");
  lcd.setCursor(0, 1);
  lcd.print("...");
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place known mass");
  lcd.setCursor(0, 1);
  lcd.print("& press 'L'");
}

void showRawResult(){
  lastRawResult = scale.get_value(10); // the average of 10 readings from the ADC minus the tare weight, set with tare()

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Raw result =");
  lcd.setCursor(0, 1);
  lcd.print(lastRawResult);

  Serial.print(F("Raw result = "));
  Serial.println(lastRawResult);
  Serial.println(F("Record this value to a spreadsheet to calculate the linear scaling factor"));
  Serial.println(F("Plot it against a known mass in kg"));
  Serial.println(F(""));
  Serial.println(F("Alternatively enter the known masss as a variable in this program,")); 
  Serial.println(F("Then press 'up' button to calculate the scalign factor")); 
  Serial.println(F(""));
}

void calcScalingFactorFromLastRawReading(){
  scalingFactor =  lastRawResult / knownMass;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ScalingFactor=");
  lcd.setCursor(0, 1);
  lcd.print(scalingFactor);

  Serial.print("Calculated Scaling Factor = ");
  Serial.println(scalingFactor);
  Serial.println("Compare this value to a spreadsheet calculated linear scaling factor to check for acuracy");
  Serial.println(F(""));
  Serial.println(F("Press the 'down' button to read a calibrated value"));
  Serial.println(F(""));

  scale.set_scale(scalingFactor);
}

void showCalibratedResult(){
  float lastCalibratedResult;
  lastCalibratedResult = scale.get_units(10); //the average of 5 readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calib Result =");
  lcd.setCursor(0, 1);
  lcd.print(lastCalibratedResult);
  lcd.print(" g");

  Serial.print("Calibrated result = ");
  Serial.print(lastCalibratedResult);
  Serial.print(" g");
  Serial.println("");
  
}
