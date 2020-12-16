/*
   ~~~~~Prerequisites~~~~~

   As per refill production code

   ~~~~~Details~~~~~~~~~~~~

   Authors: Ben Money-Coomes
   Date: 1/11/20
   Purpose: Mock up for the refill station physical calibration functionality for the 500ml volume of fluid
   References: N/A

   ~~~~Version Control~~~~~

   v1.0 - [Checkpoint] All functionality is working
   v1.1 - EEPROM command changed to EEPROM.put(eeAddress, object), and EEPROM.get() to squish bug happening with writing float with EEPROM.update() and EEPROM.read()
   v1.2 - Adding tare command inside first loop on first program run (because for some reason when the power is plugged in, then the tare subtracts a value in the setup) [I think it's solved without major change]
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Library Includes.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <HX711.h>
#include <LiquidCrystal.h>
#include <MD_UISwitch.h>
#include <EEPROM.h> // for EEPROM
#include <arduino-timer.h> // to create timers // https://github.com/contrem/arduino-timer

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Definitions                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
//Timer definitions ------------------------

auto timer = timer_create_default(); // create a timer with default settings

//MD_UISwitch defintions----------------------

const uint8_t ANALOG_SWITCH_PIN = A0;       // switches connected to this pin

// These key values work for most LCD shields
// Can be found using button_test_v1.0.ino script in drafts folder
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
MD_UISwitch_Analog pushbuttonSwitch(ANALOG_SWITCH_PIN, kt, ARRAY_SIZE(kt));

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
   Global Variables.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int currentProgramState = 2; // 1-> just print 'raw values minus tare', change to 2 -> for 'calibrated values'
float scalingFactor = 1.00;
float knownMass = 450;

// To hold the current LCD shield pushbutton state
MD_UISwitch::keyResult_t buttonState;
char lastButtonPressed = 'Z'; // initialised to an unused char
bool buttonPressActive = false;

// To check if it's the first time the program is running
bool firstProgramLoop = true; // workaround for incosistent tareing when power is plugged in

// For debugging
bool debug_verbose = false; //if set to true, extra text is printed out (not yet implemented)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Main Program                                                                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void setup() {
  //start serial
  Serial.begin(9600);

  // Set up the LCD ------
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Read the scaling factor and print it as information
  EEPROM.get(0, scalingFactor); // Read the last scalingFactor from the EEPROM
  lcdPrint("Scaling Factor", "", false);
  lcd.setCursor(0, 1); // used to print to LCD because function doesn't accept a non char array
  lcd.print(scalingFactor);
  Serial.print(F("<-- Start up --> | Scaling Factor = "));
  Serial.println(scalingFactor);
  delay(2000);

  // Start the switch -----
  pushbuttonSwitch.begin();
  pushbuttonSwitch.enableLongPress(false);

  // Zero the scale and set scaling factor -----
  lcdPrint("<-- Start up -->", "zeroing scale...", false);

  // Start communicating with the loadcell -----
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(scalingFactor); // Set scale calibration factor (this is calibrated using the calibration script)
  scale.tare(); // reset the scale to 0, so that the reference mass is set

  //Setup the timer -----
  timer.every(500, stateMachine); //Run the state machine

  // Program intialiseation messages for User -----
  // Print a ready message to the LCD.
  // Initial menu displayed over serial print
  Serial.println(F("---Button Menu----"));
  Serial.println(F("L-> Print raw value"));
  Serial.println(F("U-> Print raw value minus tare"));
  Serial.println(F("D-> Print calibrated value"));
  Serial.println(F("R-> Procedure to calculate scale factor and save this to EEPROM"));
  Serial.println(F("------------------"));
  Serial.println(F(""));
  delay(2000); // //delay to allow user to read startup message, then print ready message

}

void loop() {
  if(firstProgramLoop){
        delay(1000);
//        scale.tare(); // reset the scale to 0, so that the reference mass is set
        firstProgramLoop = false;     
      }
  readLCDShieldButtons();
  timer.tick();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Main Functions                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool readLCDShieldButtons() {
  buttonState = pushbuttonSwitch.read(); // check button statue

  switch (buttonState)
  {
    //    Note: uncomment for additional functionality!

    //    case MD_UISwitch::KEY_NULL:                                    break;
    //    case MD_UISwitch::KEY_UP:                                      break;
    //    case MD_UISwitch::KEY_DOWN:                                    break;
    case MD_UISwitch::KEY_PRESS:           //Serial.println("KEY_PRESS");
      buttonPressActive = true;
      lastButtonPressed = (char)pushbuttonSwitch.getKey(); //global variable
      switch (lastButtonPressed) // if you need a default button behaviour enter it here
      { 
        case 'L': //select button 
          currentProgramState = 0;
          break;
        case 'U': //select button 
          currentProgramState = 1;
          break;
        case 'D': //select button 
          currentProgramState = 2;
          break;
        case 'R': //select button 
          currentProgramState = 3;
          break;
      }
      break;
      //    case MD_UISwitch::KEY_DPRESS:    Serial.println("KEY_LONGPRESS"); break;
      //    case MD_UISwitch::KEY_LONGPRESS: Serial.println("KEY_LONGPRESS"); break;
      //    case MD_UISwitch::KEY_RPTPRESS:  Serial.println("KEY_RPTPRESS");  break;
      //    default:                         Serial.println("KEY_UNKNWN");    break;
  }

  return true; // important, to ensure timer continues to repeat this function!
}

bool stateMachine(){
  // A state machine structure

  switch (currentProgramState) {
    case 0:
      // Print raw value
      showRawResult();
      break;

    case 1:
      // Print raw value minus tare (default mode)
      showRawTareResult();
      break;

    case 2:
      // Print calibrated results
      showCalibratedResult();
      break;

    case 3:
      // Calculate scaling factor, save it to EEPROM and calibrate the scale
      lcdPrint("Add 450g mass", "s-> calc sf", false);
      Serial.println(F("The container must already have been placed when the board is started"));
      Serial.println(F("Now place 450g of mass and press the 'S' key"));   

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
          
            long lastRawResult = scale.get_value(10); // the average of 10 readings from the ADC minus the tare weight, set with tare()
            float scalingFactor =  lastRawResult / knownMass;
            
            Serial.print(F("Calculated Scaling Factor = "));
            Serial.println(scalingFactor);

            // Print to the LCD keypad shield
            lcdPrint("Scaling factor", "", false);
            lcd.setCursor(0, 1); // used to print to LCD because function doesn't accept a non char array
            lcd.print(scalingFactor);

            // Set the scale scaling factor and write it to the EEPROM at address 0
            scale.set_scale(scalingFactor); //set the scaling factor for the scale
            EEPROM.put(0, scalingFactor);

            delay(3000);

            // Moving to read calibrated result
            Serial.print(F(""));
            Serial.print(F("Reading calibrated result"));
            lcdPrint("SF set", "continuing ... ", false);

            delay(3000);
            currentProgramState = 2; // move to print calibrated results
                       
            break;
        }
        
      }


      buttonPressActive = false; // this allow the keypad to catch a true next button press because it is running at a faster read cycle
      break;

  }
  return true; // to repeat with the timer
}

void showRawResult(){
  // Print the raw analogue value (no calibration) and without tare adjustment
  long lastResult = scale.read();

  // Print to the LCD keypad shield
  lcdPrint("Raw Value", "", false);
  lcd.setCursor(0, 1); // used to print to LCD because function doesn't accept a non char array
  lcd.print(lastResult);

  // Print to the serial monitor
  Serial.println(lastResult);
}


void showRawTareResult(){
  // Print the raw analogue value (no calibration) minus the tare weight
  long lastResult = scale.get_value(1);

  // Print to the LCD keypad shield
  lcdPrint("Raw Tare Val", "", false);
  lcd.setCursor(0, 1);
  lcd.print(lastResult);

  // Print to the serial monitor
  Serial.println(lastResult);
}

void showCalibratedResult(){
  //Print the calibrated result (based on scale factor)
  float lastResult = scale.get_units(1);

  // Print to the LCD keypad shield
  lcdPrint("Calib Val (grams)", "", false);
  lcd.setCursor(0, 1); // used to print to LCD because function doesn't accept a non char array
  lcd.print(lastResult);

  // Print to the serial monitor
  Serial.println(lastResult);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Helper Functions                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void lcdPrint(char *lcdString1, char *lcdString2, bool autoscrollStatus) {
  lcd.clear();
  if (autoscrollStatus) {
    lcd.autoscroll();
  }
  else {
    lcd.noAutoscroll();
  }

  lcd.setCursor(0, 0);
  lcd.print(lcdString1);
  lcd.setCursor(0, 1);
  lcd.print(lcdString2);
}
