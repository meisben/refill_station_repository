/*
   ~~~~~Prerequisites~~~~~

   See Github Readme

   ~~~~~Details~~~~~~~~~~~~

   Authors: Ben Money-Coomes
   Date: 2/10/20
   Purpose: Production code baseline for prototype refill station (version B2). For usage see Github readme
   References: See Github Readme

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
   v2.11a - Ajusting to variant B2 (no changes)
   v2.12 - Squishing bug where eroneous button presses after pumping were carried forward to the next bottle (added line "buttonPressActive = false; // reset status to ignore any erroneous button presses and wait for next button press" to case 0 of the state machine)
   v2.12_draftv1 - Looking to implement a low pass filter to see if it improves the pumping variation performance
   v_C_2.3 - [Checkpoint] note: the up button is still problematic
   v_C_2.4 - working on changing button flow
   v_C_2.5 - [Checkpoint] everything is working
   v_C_2.6 - working on writing Mass Set Points to the EEPROM using new states --> all working in v2
   v_C_2.6 - working on removing bug caused by ocassional erroneous big negative HX711 readings
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Library Includes.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <HX711.h> // for loadcell // https://github.com/bogde/HX711
#include <LiquidCrystal.h> // for LCD screen
#include <MD_UISwitch.h> // for reistor ladder buttons on LCD shield
#include <arduino-timer.h> // to create timers // https://github.com/contrem/arduino-timer
#include "g_h_filter_class.h" // allows filtering of the mass sensor readings
#include <EEPROM.h> // for EEPROM

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Definitions                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
//Start address for EEPROM -----------------
#define  SCALING_FACTOR_START_ADDRESS 0
#define  MASS_SETPOINT_START_ADDRESS  5 //Starting position in the address space
 
//Timer definitions ------------------------

auto timer = timer_create_default(); // create a timer with default settings

//MD_UISwitch defintions----------------------

const uint8_t ANALOG_SWITCH_PIN = A0;       // switches connected to this pin

// These key values work for most LCD shields
// Can be found using button_test_v1.0.ino script in drafts folder
MD_UISwitch_Analog::uiAnalogKeys_t kt[] =
{
  { 5, 5, 'R' },  // Right
  { 100, 50, 'U' },  // Up
  { 257, 15, 'D' },  // Down
  { 410, 15, 'L' },  // Left
  { 640, 15, 'S' },  // Select
};

//// These key values work for the "D1 Robot" LCD shields (at least one of them!)
//MD_UISwitch_Analog::uiAnalogKeys_t kt[] =
//{
//  { 5, 5, 'R' },  // Right
//  { 131, 50, 'U' },  // Up
//  { 307, 15, 'D' },  // Down
//  { 480, 15, 'L' },  // Left
//  { 720, 15, 'S' },  // Select
//};


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
//long int values = 0;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// Note that when using the LCD Keypad Shield then pin 2 is the 3rd pin from the rhs and pin 3 is the 4th pin from the rhs

// G-H filter constants -----------------------------

#define GH_FILTER_ACTIVE 0 // --> 1 if active, 0 if inactive
#define G_CONSTANT 0.3
#define H_CONSTANT 0.2

// Error limit - for scale error catching ----------------------------------
const bool scaleErrorCatching = true; //if true error catching is active and erroneous readings are ignored
#define NEGATIVE_ERROR_LIMIT -50 //readings below this value are considered an error, if the refill station sees a value below this in state 1, then it will prompt the user to reset the scale before continuing
#define NEGATIVE_ERROR_TOLLERANCE_DURING_PUMPING -10 //readings below this error during this pumping are considered an error (in the massExceeded function and ignored) because the mass should be increasing approximately linearly. A seperate function still checks if the container has been removed
float lastMassValue;

// LCD characters ----------------------------------

const byte up_arrow[8] = {
  B00000,
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
};

const byte down_arrow[8] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
};

const byte right_arrow[8] = {
  B00000,
  B00100,
  B00010,
  B11111,
  B00010,
  B00100,
  B00000,
};

const byte left_arrow[8] = {
  B00000,
  B00100,
  B01000,
  B11111,
  B01000,
  B00100,
  B00000,
};

const byte hour_glass[8] = {
  B11111,
  B11111,
  B01110,
  B00100,
  B01110,
  B11111,
  B11111,
};

const byte tick_symbol[8] = {
  B00001,
  B00011,
  B00011,
  B10110,
  B11110,
  B01100,
  B00000,
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Global Variables                                                               *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// For keeping track of the program state
int currentProgramState = 0;
bool firstTimeInState = false;

// For calibration
float scalingFactor = 1588.48; //note that this is changed by default on startup by reading custom values from the EEPROM (usually set by utility program pre-production) this is a safety value only

// For pumping trigger
const int outputPinPump1 =  12; // The output pin assigned to this pump, note this is normally pin '12', but 'LED_BUILTIN' can be usefully used for debugging! Note that it can't share the same pins as the LCD shield ! 
bool outputPinPump1_trigger = false; // Used to call the pump trigger
bool outputPinPump1_triggerActive = false;  // Used to confirm record that it has been implemented
bool outputPinPump_emergencyStop = false; // Used to cancel pumping

// For storing the masses of interest
byte massSelectionIndex = 1; // Used to index the massSelection arrays, this defines the default (corresponds to 0.5L)
int massValues[] = {180, 400, 840}; //note these are changed by default on startup by reading custom values from the EEPROM (usually set by utility program pre-production)

// For storing the text relating to the masses in an array
char massText1[] = " 250ml ";
char massText2[] = " 500ml ";
char massText3[] = "1000ml ";

char * massStrings[] =
{
  massText1,
  massText2,
  massText3
};

// For storing the container/bottle mass, and the mass to pump
int containerMass; //used to record the current mass of the container used for receiving liquid
int massToPump = 0; //used to store the current mass to pump

// For storing global signals
bool cancelSignal = false;

// The expected mass of the lightest receptacle and the boolean which keeps track of whether a container is present on the scale
int expectedContainerMass = 40;
bool containerPresent = false;

// To hold the current LCD shield pushbutton state
MD_UISwitch::keyResult_t buttonState;
char lastButtonPressed = 'Z'; // initialised to an unused char
bool buttonPressActive = false;

// Initialise g-h filter
g_h_filter massFilter(200, 0, 300, G_CONSTANT, H_CONSTANT); //create g-h filter for tracking system state (mass of fluid)

// For debugging
const bool debug_verbose = false; //if set to true, extra text is printed out
const bool debug_scaleValue = false; //if set to true, the scale reading is printed out 500ms

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
  // Create custom characters
  lcd.createChar(0, up_arrow);
  lcd.createChar(1, down_arrow);
  lcd.createChar(2, right_arrow);
  lcd.createChar(3, left_arrow);
  lcd.createChar(4, hour_glass);
  lcd.createChar(5, tick_symbol);
  // inform user we are zeroing the scale
  lcdPrint("    Start up    ", "zeroing scale...", false);
  lcd.setCursor(0,0);
  lcd.write(byte(4)); 

  // information on GH filter
  if(GH_FILTER_ACTIVE == 1){
    Serial.println(F("GH filter active"));
  }
  else{
    Serial.println(F("GH filter inactive"));    
  }

  // Print a serial mesage with instructions
  Serial.println(F("Program commencing..."));
  Serial.println(F("Note: That scale is zeroed during program start up. The scale must have no objects placed on it at startup."));

  // Read scaling factor from the EEPROM
  EEPROM.get(SCALING_FACTOR_START_ADDRESS, scalingFactor);
  Serial.print(F("ScalingFactor value (from EEPROM): "));
  Serial.println(scalingFactor);

  // Start communicating with the loadcell -----
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.tare(); // reset the scale to 0, so that the reference mass is set
  scale.set_scale(scalingFactor); // Set scale calibration factor (this is calibrated using the calibration script)

  // Read mass set points from the EEPROM
  int temp_val; // local variable
  for (byte i = 0; i < (sizeof(massValues) / sizeof(massValues[0])); i++) {
    Serial.print(F("Reading mass set-point value for: "));
    Serial.println(massStrings[i]);
    
    EEPROM.get(MASS_SETPOINT_START_ADDRESS+(2*i), temp_val); // read the value
    massValues[i] = temp_val; // assign it
    Serial.print(F(" Which has value(g) of: ")); // print it to the serial monitor
    Serial.println(massValues[i]);
  }

  // Start the switch -----
  pushbuttonSwitch.begin();
  pushbuttonSwitch.enableLongPress(false);

  //Setup the timer -----
  timer.every(800, stateMachine); //manage the state of the program,to be called every 500 millis (0.8 second)
  timer.every(25, managePumptriggers); //manage the pump on/off statuses, to be called every 500 millis (0.1 second)
  if (debug_scaleValue) {
    timer.every(500, printScaleValueToSerial);
  }

  // Program intialiseation messages for User -----  
  Serial.println(F(""));
  Serial.println(F("Now place fluid receptacle and then press 'right' to start pumping"));
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

  containerMass = scale.get_units(3); //the average of x readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale
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
      lastMassValue = scale.get_units(3); //initialise this value
      digitalWrite(outputPinPump1, HIGH);
      outputPinPump1_triggerActive = true; // turn the output on
      Serial.println(F("Pumping started"));
      massFilter.reset(scale.get_units(1), 0, 300, G_CONSTANT, H_CONSTANT);
    }
    else {
      bool stopSignal = false;
      float myCurrentMass = scale.get_units(1);

      // the below if statement is active if the G-H filter is active, and the scale is not returning a erroneous negative reading
      if(GH_FILTER_ACTIVE==1 && !isScaleReadingErroneousDuringPumping(myCurrentMass,lastMassValue)){
        myCurrentMass = massFilter.return_value(scale.get_units(1));        
      }
      
      lastMassValue = myCurrentMass; //store the mass reading for error checking
      
      if (debug_verbose) {
        Serial.print("filter/scale output myCurrentMass = ");
        Serial.println(myCurrentMass);  
      }
      bool stopSignalMassExceeded = isMassExceeded(myCurrentMass, massToPump); // returns true if pumping volume is exceeded
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

  float thisCurrentMass = scale.get_units(1); //the average of x readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale
  
  if (thisCurrentMass <= expectedContainerMass && !isScaleReadingErroneous(thisCurrentMass)) {
    if (debug_verbose) {
      Serial.print(F("  No container present, mass is: "));
      Serial.println(thisCurrentMass);
    }
    return false;
  }
  else {
    return true;
  }
}

bool isScaleReadingErroneous(float myReading){
  if(scaleErrorCatching){
    if(myReading < NEGATIVE_ERROR_LIMIT){
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }
}

bool isScaleReadingErroneousDuringPumping(float myReading, float myLastReading){
  if(scaleErrorCatching){

    if(myReading < myLastReading + NEGATIVE_ERROR_TOLLERANCE_DURING_PUMPING){
      return true;
    }
    else{
      return false;
    }    
  }
  else{
    return false;
  }
}

void printScaleValueToSerial() {
  Serial.print(F("Scale value (g): "));
  Serial.println(scale.get_units(1)); //prints to serial constantly for debugging purposes, can be graphed!
}


// this is effectively a state machine, it will also make it easier to write funtionality into a menu later
bool stateMachine() {

  switch (currentProgramState) {
    case 0:
      // Container not on the scale
      // -->> continue to check if container is placed on scale then move to next program state
      // -->> continue to check if the 'right' button is pressed, in which case initiate Mass Set Point mode
      if (isContainerPresent()) {
        currentProgramState = 1;
        buttonPressActive = false; // reset status to ignore any erroneous button presses and wait for next button press (v2.12 edit)
        Serial.println(F("Select volume to pump using up, down buttons. Then press right key"));
        lcdShowMassSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass (default last selected)
      }

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = 30;
            Serial.println(F("-- Set point Mode --"));
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Set-Point Mode");
            lcd.setCursor(0,1);
            lcd.write(byte(3)); 
            lcd.setCursor(3,1);
            lcd.print("Continue?");
            lcd.setCursor(15,1);
            lcd.write(byte(2)); 
            break;
        }
      }
      
      break;

    case 1:
      // Container is placed, waiting for selection command
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if container is removed from the scale in error, then go backto previous step
      // -->> continue to check if up or down buttons are placed, then change the display and the selected mass

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            //Do nothing
            break;
          case 'L': //left button
            //Do nothing
            break;
          case 'U': //up button
            if(massSelectionIndex == 2){
              massSelectionIndex = 0;
            }
            else{
              massSelectionIndex = massSelectionIndex + 1; //keeps mass index in range of menu size
            }
            lcdShowMassSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'D': //down button
            if(massSelectionIndex == 0){
              massSelectionIndex = ((sizeof(massValues)/2) - 1);
            }
            else{
              massSelectionIndex = massSelectionIndex - 1; //keeps mass index in range of menu size
            }
            lcdShowMassSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'R': //right button
            currentProgramState = 2;
            Serial.println(F("Check nozzle is inserted in receptacle, confirm with right key"));
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Check nozzle is");
            lcd.setCursor(0,1);
            lcd.write(byte(3)); 
            lcd.setCursor(3,1);
            lcd.print("inserted?");
            lcd.setCursor(15,1);
            lcd.write(byte(2)); 
            break;
        }
      }

      if (!isContainerPresent()) {
        // if container is removed
        currentProgramState = 0;
        lcdPrint("Ready...", "place receptacle", false);
        break;
      }
      break;

    case 2:
      // Container is placed, waiting for pumping command
      // -->> continue to check if pumping command button press is issued, then move to next program state
      // -->> continue to check if back button is pressed, then move to previous program state
      // -->> continue to check if container is removed from the scale in error, then go backto previous step

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = 1;
            lcdShowMassSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            Serial.print(F("pumpFluidVolume activated with volume, mass = "));
            Serial.print(massStrings[massSelectionIndex]);
            Serial.print(F(", "));
            Serial.print(massValues[massSelectionIndex]);
            Serial.println(F(" g"));
            cancelSignal = false;
            pumpFluidVolume(massValues[massSelectionIndex]);
            break;
        }
      }
      
      if (outputPinPump1_trigger) {
        currentProgramState = 3;
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

    case 3:
      // Pumping command is activated
      // -->> continue to check if pumping is finished, then move to next program state
      // -->> continue to check if cancel key is pressed, then move to next program state. Note that because cancel key is important this functionality is implemented directly in the button press function!
      // -->> continue to check if container is removed from the scale in error, then stop pumping

      if (!outputPinPump1_trigger) {
        // if pumping ends
        currentProgramState = 4;
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

    case 4:
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

    // --------
    // The cases above this line deal with the core functionality
    // The cases below deal with set point mode
    // -------

    case 30:
      // Mass set point mode title being displayed
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = 0;
            lcdPrint("Ready...", "place receptacle", false);
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = 31;
            // describe set point mode
            lcdPrint("For set-point", "recalibration ", false);
            lcd.setCursor(15,1);
            lcd.write(byte(2)); 
            break;
        }
      }

      break;

    case 31:
      // Mass set point mode description being displayed
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = 0;
            lcdPrint("Ready...", "place receptacle", false);
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = 32;
            lcdShowSetPointSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass set-point
            break;
        }
      }

      break;

      case 32:
      // Select which setPoint to adjust
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = 0;
            lcdPrint("Ready...", "place receptacle", false);
            break;
          case 'U': //up button
            if(massSelectionIndex == 2){
              massSelectionIndex = 0;
            }
            else{
              massSelectionIndex = massSelectionIndex + 1; //keeps mass index in range of menu size
            }
            lcdShowSetPointSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'D': //down button
            if(massSelectionIndex == 0){
              massSelectionIndex = ((sizeof(massValues)/2) - 1);
            }
            else{
              massSelectionIndex = massSelectionIndex - 1; //keeps mass index in range of menu size
            }
            lcdShowSetPointSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'R': //right button
            currentProgramState = 33;
            // ask: Is container placed on scale?
            lcdPrint("Is container on","scale?",false);
            lcd.setCursor(15,1);
            lcd.write(byte(2)); 
            Serial.println(F("Check container is on scale"));
            break;
        }
      }

      break;

      case 33:
      // 'Is container placed on scale message' displayed
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = 0;
            lcdPrint("Ready...", "place receptacle", false);
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = 34;
            // move to recording container mass
            break;
        }
      }

      break;

      case 34:
      // record container mass, then move to next state
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      Serial.println(F("Recording container mass"));
      lcdPrint("  Recording mass", "of container...", false);
      lcd.setCursor(0,0);
      lcd.write(byte(4)); 
      containerMass = scale.get_units(3); //the average of x readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale
      if (debug_verbose) {
        Serial.print(F("Container mass recorded as ="));
        Serial.print(containerMass);
        Serial.println(F(" g"));
      }
      if (containerMass <= 20) {
        Serial.println(F("Error: no container placed on scale"));
      }
      delay(1000);
      
      currentProgramState = 35;
      lcdPrint("Fill container", "to set point", false);
      lcd.setCursor(15,1);
      lcd.write(byte(2)); 
      break;

      case 35:
      // Display message asks 'is container filled to set point?'
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = 0;
            lcdPrint("Ready...", "place receptacle", false);
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = 36;
            // move to recording set point mass
            break;
        }
      }
      break;

      case 36:
      // record setpoint mass, then move to end state
      // -->> record the set-point mass
      // -->> save it to the correct indexed EEPROM address

      lcdPrint("  Recording mass", "for set-point", false);
      lcd.setCursor(0,0);
      lcd.write(byte(4)); 
      massValues[massSelectionIndex] = scale.get_units(3) - containerMass; //the average of x readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale
      EEPROM.put(MASS_SETPOINT_START_ADDRESS+(2*massSelectionIndex), massValues[massSelectionIndex]); // Write the value to EEPRM
      
      if(debug_verbose){
        Serial.print(F("Writing value of: "));
        Serial.print(massValues[massSelectionIndex]);
        Serial.print(F("g for the volume: "));
        Serial.println(massStrings[massSelectionIndex]);
      }
      delay(1000);

      lcdPrint("Complete! set-", "point recorded", false);
      delay(2000);

      Serial.print(F("Done! Remove container!"));
      currentProgramState = 37;
      lcdPrint("  Done!", "Remove container!", false);
      lcd.setCursor(0,0);
      lcd.write(byte(5)); 
      break;

      case 37:
      // Wait for container to be removed and then move to the home state
      // -->> continue to check if container is removed (scale reading is close to zero)
      // -->> otherwise remain looping in this state
      
      if (abs(scale.get_units(3))<30) {
        // move to previous state
        currentProgramState = 0;
        lcdPrint("Ready...", "place receptacle", false);
        Serial.print(F("Place container to start refill"));
      }
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

void lcdShowMassSelectionMenu(byte myIndex){
  // for the indexes 0-> corresponds to 250ml, 1-> corresponds to 500ml, 2-> corresponds to 1L
  lcdPrint("Select volume", "", false);
  lcd.setCursor(0,1);
  lcd.print(massStrings[myIndex]);
  lcd.write(byte(1));
  lcd.write(byte(0));
  lcd.setCursor(15, 1);
  lcd.write(byte(2)); 
}

void lcdShowSetPointSelectionMenu(byte myIndex){
  // for the indexes 0-> corresponds to 250ml, 1-> corresponds to 500ml, 2-> corresponds to 1L
  lcdPrint("Select set-point", "", false);
  lcd.setCursor(0,1);
  lcd.print(massStrings[myIndex]);
  lcd.write(byte(1));
  lcd.write(byte(0));
  lcd.setCursor(15, 1);
  lcd.write(byte(2)); 
}
