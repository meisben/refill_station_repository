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
   v1.2 - [Checkpoint] Script to calculate scaling factor for the scale. Will be saved seperately!
   v1.3 - [Checkpoint] Functionality for automatic pumping is working!
   v1.4 - Adding an additional timer to try and get rid of the error where MD_UISwitch sees multiple button presses ... Solved ! ! :)
   v1.41 - Checking on bug where you place a container for the second time, I should just printout two values .... the setPointMass, and the calibratedMassReading ... it's seems like it's not a bug but I added the debug flag with extra printing anway
   v1.42 - Adding function to print to the LCD screen to simplify the code
   v1.43 - Adding text to better describe for the user what to do, adding a state machine function, adding a function to keep track of whether a container is placed ... 98% working ! , I just need to ad actual masses of fluid !
   v1.44 - Squishing cancel button bug.
   v1.45 - [Checkpoint]improve squishing of cancel button bug by moving functionality directly into the button function and making all ouputs low as soon as cancel button is pressed ! Fully working program ! Happy with this for prototype :) 
   v1.51 - minor adjustments to timing
   v1.52 - Adding actual fluid masses from measured experimental shampoo results (see spreadsheet). Corrected hardware bug for pin 4 which is shared with LCD keypad. This must be pin 12 or another unused pin.
   v2.00 - [Checkpoint, production] 
   v2.10 - Adjusting masses in garden with water to get correct weights
   v2.11 - Adjusting code to variant B -> to accomodate TAL220 strain gauge and D1Robot keypad
   v2.12 - Squishing bug where eroneous button presses after pumping were carried forward to the next bottle (added line "buttonPressActive = false; // reset status to ignore any erroneous button presses and wait for next button press" to case 0 of the state machine)

*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Library Includes.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <HX711.h> // for loadcell // https://github.com/bogde/HX711
#include <LiquidCrystal.h> // for LCD screen
#include <MD_UISwitch.h> // for reistor ladder buttons on LCD shield
#include <arduino-timer.h> // to create timers // https://github.com/contrem/arduino-timer

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Definitions                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
//Timeer definitions ------------------------

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
  Global Variables                                                        *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// For keeping track of the program state
int currentProgramState = 0;
bool firstTimeInState = false;

// For calibration
float scalingFactor = 223.61;

// For pumping trigger
const int outputPinPump1 =  12; // The output pin assigned to this pump, note this is normally pin '12', but 'LED_BUILTIN' can be usefully used for debugging! Note that it can't share the same pins as the LCD shield ! 
bool outputPinPump1_trigger = false; // Used to call the pump trigger
bool outputPinPump1_triggerActive = false;  // Used to confirm record that it has been implemented
bool outputPinPump_emergencyStop = false; // Used to cancel pumping

// For storing the masses of interest (from the 'hx711_calibration_spreadsheet.xls')
int testMass = 330; // A good test object is 150g chocolate bars, yum ;) 
int quarterLitreMass = 180; //This is adjusted to account for shampoo density, values are from the calibration sheet. // was 215
int halfLitreMass = 400; //was 440
int litreMass = 840; //was 890

// For storing the container/bottle mass, and the mass to pump
int containerMass; //used to record the current mass of the container used for receiving liquid
int massToPump = 0; //used to store the current mass to pump

// For storing global signals
bool cancelSignal = false;

// The expected mass of the lightest receptacle and the boolean which keeps track of whether a container is present on the scale
int expectedContainerMass = 20;
bool containerPresent = false;

// To hold the current LCD shield pushbutton state
MD_UISwitch::keyResult_t buttonState;
char lastButtonPressed = 'Z'; // initialised to an unused char
bool buttonPressActive = false;

// For debugging
bool debug_verbose = false; //if set to true, extra text is printed out
bool debug_scaleValue = false; //if set to true, the scale reading is printed out 500ms

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Start of main program                                                          *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void setup()
{
  //Setup the input and output pins for the pumps -----
  pinMode(outputPinPump1, OUTPUT);
  digitalWrite(outputPinPump1, LOW);   // turn the output off
  
  // Start serial communications -----
  Serial.begin(9600);

  // Set up the LCD ------
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcdPrint("<-- Start up -->", "zeroing scale...", false);

  // Start communicating with the loadcell -----
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.tare(); // reset the scale to 0, so that the reference mass is set
  scale.set_scale(scalingFactor); // Set scale calibration factor (this is calibrated using the calibration script)

  // Start the switch -----
  pushbuttonSwitch.begin();
  pushbuttonSwitch.enableLongPress(false);

  //Setup the timer -----
  timer.every(800, stateMachine); //manage the state of the program,to be called every 500 millis (0.5 second)
  timer.every(800, managePumptriggers); //manage the pump on/off statuses, to be called every 500 millis (0.5 second)
  if (debug_scaleValue) {
    timer.every(500, printScaleValueToSerial);
  }

  // Program intialiseation messages for User -----

  // Print a serial mesage with instructions
  Serial.println(F("Program commencing..."));
  Serial.println(F("Note: That scale is zeroed during program start up. The scale must have no objects placed on it at startup."));
  Serial.println(F(""));
  Serial.println(F("Now place fluid receptacle and then press 'Left' to start pumping"));
  Serial.println(F(""));

  // Print a ready message to the LCD.
  delay(500); //delay to allow user to read startup message, then print ready message
  lcdPrint("Ready...", "place receptacle", false);

}

void loop()
{
  readLCDShieldButtons();
  timer.tick();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Functions                                                                      *
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
        case 'S': //select button (designated as the emergency stop button)
          emergencyStop();
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


// To command the pump to pump a volume of fluid
void pumpFluidVolume(int myMassToPump) {

  containerMass = scale.get_units(3); //the average of 5 readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale
  if (debug_verbose) {
    Serial.print(F("Container mass recorded as ="));
    Serial.print(containerMass);
    Serial.println(F(" g"));
  }
  if (containerMass <= 20) {
    Serial.println(F("Error: no container placed on scale"));
  }
  else {
    outputPinPump1_trigger = true;
    massToPump = myMassToPump + containerMass;
    if (debug_verbose) {
      Serial.print(F("Total mass to pump ="));
      Serial.print(massToPump);
      Serial.println(F(" g"));
    }

  }
}

// To manage the trigger start/stop of the pump
bool managePumptriggers() {

  if (outputPinPump1_trigger == true) {

    if (!outputPinPump1_triggerActive) {
      digitalWrite(outputPinPump1, HIGH);
      outputPinPump1_triggerActive = true; // turn the output on
      Serial.println(F("Pumping started"));
    }
    else {
      bool stopSignal = false;
      bool stopSignalMassExceeded = isMassExceeded(scale.get_units(1), massToPump); // returns true if pumping volume is exceeded
      bool stopSignalCancelButton = manageCancelButton(); //returns true if cancel button is pressed
      stopSignal = (stopSignalMassExceeded || stopSignalCancelButton); // stopSignal takes an logical 'OR' value

      if (stopSignal == true) {
        digitalWrite(outputPinPump1, LOW);   // turn the output off
        outputPinPump1_triggerActive = false;
        outputPinPump1_trigger = false;
        Serial.println(F("Pumping stopped"));
      }
    }
  }

  return true; // important, to ensure timer continues to repeat this function!
}

// To check if the mass being pumped has exceeded the set point
bool isMassExceeded(float currentMass, float massToExceed) {
  if (currentMass >= massToExceed) {
    Serial.println(F("Mass exceeded set point"));
    return true;
  }
  else {
    return false;
  }
}

// To enable the cancel button functionality
bool manageCancelButton() {
  if (cancelSignal == true) {
    cancelSignal = false;
    Serial.println(F("Cancel button pressed"));
    return true;
  }
  else {
    return false;
  }
}

// To manage whether a receptacle is detected on the scale and whether it has been removed. Protect against fluid spills. Allow correct text to be shown to user
bool isContainerPresent() {

  float currentMass = scale.get_units(3); //the average of 5 readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale

  if (currentMass >= expectedContainerMass) {
    return true;
  }
  else {
    return false;
  }
}

void printScaleValueToSerial() {
  Serial.print(F("Scale value (g): "));
  Serial.println(scale.get_units(2)); //prints to serial constantly for debugging purposes, can be graphed!
}


// this is effectively a state machine, it will also make it easier to write funtionality into a menu later
bool stateMachine() {

  switch (currentProgramState) {
    case 0:
      // Container not on the scale
      // -->> continue to check if container is placed on scale then move to next program state
      if (isContainerPresent()) {
        currentProgramState = 1;
        buttonPressActive = false; // reset status to ignore any erroneous button presses and wait for next button press
        Serial.println(F("Press button to select fluid quantity. 'Left':0.25L, 'Up':0.5L, 'Down':1L"));
        lcdPrint("Press L:0.25,", "U:0.5, D:1 litre", false);
      }
      break;

    case 1:
      // Container is placed, waiting for pumping command
      // -->> continue to check if pumping command button press is issued, then move to next program state
      // -->> continue to check if container is removed from the scale in error, then go backto previous step

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            cancelSignal = false;
            pumpFluidVolume(quarterLitreMass);
            Serial.print(F("pumpFluidVolume activated with mass = "));
            Serial.print(quarterLitreMass);
            Serial.println(F(" g"));
            break;
          case 'U': //up button
            cancelSignal = false;
            pumpFluidVolume(halfLitreMass);
            Serial.print(F("pumpFluidVolume activated with mass = "));
            Serial.print(halfLitreMass);
            Serial.println(F(" g"));
            break;
          case 'D': //down button
            cancelSignal = false;
            pumpFluidVolume(litreMass);
            Serial.print(F("pumpFluidVolume activated with mass = "));
            Serial.print(litreMass);
            Serial.println(F(" g"));
            break;
          case 'R': //down button
            cancelSignal = false;
            pumpFluidVolume(testMass);
            Serial.print(F("pumpFluidVolume activated with mass = "));
            Serial.print(testMass);
            Serial.println(F(" g"));
            break;
        }
      }

      if (outputPinPump1_trigger) {
        currentProgramState = 2;
        lcdPrint("Pumping! Press", "'S' to cancel", false);
        break;
      }
      if (!isContainerPresent()) {
        // if container is removed
        currentProgramState = 0;
        lcdPrint("Ready...", "place receptacle", false);
        break;
      }
      break;

    case 2:
      // Pumping command is activated
      // -->> continue to check if pumping is finished, then move to next program state
      // -->> continue to check if cancel key is pressed, then move to next program state. Note that because cancel key is important this functionality is implemented directly in the button press function!
      // -->> continue to check if container is removed from the scale in error, then stop pumping

      if (!outputPinPump1_trigger) {
        // if pumping ends
        currentProgramState = 3;
        firstTimeInState = true;
        break;
      }
      if (!isContainerPresent()) {
        // if container is removed
        cancelSignal = true; // act as a cancel button to stop pumping
        Serial.println(F("Container removed, cancelling pumping!"));
        currentProgramState = 0;
        lcdPrint("Ready...", "place receptacle", false);
        break;
      }

      // Note that emergency stop functionality is implemented directly in the button function for quick implementation! 

      break;

    case 3:
      // // Pumping is finished
      // -->> waiting for container to be removed
      if(firstTimeInState){
        lcdPrint("Finished! remove", "receptacle", false);
        firstTimeInState = false;
      }
      
      if (!isContainerPresent()) {
        // if container is removed
        currentProgramState = 0;
        lcdPrint("Ready...", "place receptacle", false);
      }
      break;

    default:
      // statements
      break;
  }

  return true; // to repeat with the timer
}


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

void emergencyStop() {
digitalWrite(outputPinPump1, LOW);   // turn the outputs off
cancelSignal = true; // act as a cancel button to stop pumping
Serial.println(F("Emergency Stop!"));
}
