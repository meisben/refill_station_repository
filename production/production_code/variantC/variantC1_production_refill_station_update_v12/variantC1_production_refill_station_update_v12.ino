/*
   ~~~~~Prerequisites~~~~~

   See Github Readme

   ~~~~~Details~~~~~~~~~~~~

   Authors: Ben Money-Coomes
   Date: 5/10/20
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
   v_C_2.7 - [Checkpoint] Working version, no major bugs. Need to (1) add compensating factor for pump/fluid intertia, and (2) Periodically prompt user to tare scale if zero drifts
   v_C_2.7 - working on (1) add compensating factor for pump/fluid intertia
   v_C_2.8 - [Checkpoint] Added compensating factor for fluid inertia, but discovered bug with extreme high erroneous value from scale
   v_C_2.9 - working on removing extreme high erroneous value from scale (I think i've sold it), There is also a bug where you zero the scale, when you take it off it says ready to pump -> should enter state where scale needs re-zeroing
   v_C_3.0 - removed tare error from scale (removed by taring it after a 5s delay to allow any power fluctuations to pass)
   v_C_3.1 - working on removing the bug:  where you zero the scale with a bottle on it, when you take it off it says ready to pump -> should enter state where scale needs re-zeroing
   v_C_3.2 - [Checkpoint] Everything is working
   v_C_3.3 - working on adding functionality so that if you hold down a button then things will keep pumping until it is released, I'm probably going to use the 'right' button for this as when pressed the button pin voltage should be around zero. Currently pump pin is changed to LED Pin on line 224!
   v_C_3.4 - This is working ! Just need to add actual pin turn on to manual mode. And then sort out the menu order. State machine has been updated to best practice. Also removing totally the printLCD function will free up a LOT of memory!
   v_C_3.5 - Working on the issues above (printLCD -> removed) (Output pin -> added)
   v_C_3.6 - Working on the issues above 
   v_C_3.7 - [Checkpoint] Everything is working, stable version
   v_C_3.8 - Adding a custom value to the setpoints with title 'Custom' which will have intial value same as 250 ml
   v_C_3.9 - [Checkpoint] Everything is working, stable version
   v_C_4.0 - [Checkpoint] Working on removing bugs, which (1) erroneouly cause menus to be entered into sometimes because 'buttonActive' isn't reset, (2) adjusting fudge factor for setpoint to reduce this by half, (3) the set point mode and manual mode should return to menu if the left button is pressed
   v_C_4.1 - In the above list of issued (2) has been solved. (1) and (3) have also been solved :) ! 

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
//MD_UISwitch_Analog::uiAnalogKeys_t kt[] =
//{
//  { 5, 5, 'R' },  // Right
//  { 100, 50, 'U' },  // Up
//  { 257, 15, 'D' },  // Down
//  { 410, 15, 'L' },  // Left
//  { 640, 15, 'S' },  // Select
//};

//// These key values work for the "D1 Robot" LCD shields (at least one of them!)
MD_UISwitch_Analog::uiAnalogKeys_t kt[] =
{
  { 5, 5, 'R' },  // Right
  { 131, 20, 'U' },  // Up
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
//long int values = 0;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// Note that when using the LCD Keypad Shield then pin 2 is the 3rd pin from the rhs and pin 3 is the 4th pin from the rhs

// G-H filter constants -----------------------------

#define GH_FILTER_ACTIVE 0 // --> 1 if active, 0 if inactive
#define G_CONSTANT 0.6
#define H_CONSTANT 0.2

// Error limit - for scale error catching ----------------------------------
const bool scaleErrorCatching = true; //if true error catching is active and erroneous readings are ignored
#define NEGATIVE_ERROR_LIMIT -50 //readings below this value are considered an error, if the refill station sees a value below this in state 1, then it will prompt the user to reset the scale before continuing
#define SCALE_ZERO_ERROR_TOLLERANCE 15 //readings just above or above this value when the program is in state 0 should be considered an error
#define POSITIVE_ERROR_TOLLERANCE_DURING_PUMPING 500 // readings above this relative number during pumping are considered an error (in the massExceeded function) and ignored. This is because the mass should be increasing approximately linearly. 
#define NEGATIVE_ERROR_TOLLERANCE_DURING_PUMPING -10 //readings below this relative number during this pumping are considered an error (in the massExceeded function) and ignored. This is because the mass should be increasing approximately linearly. A seperate function still checks if the container has been removed
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

const byte warning_mark[8] = {
  B11111,
  B11011,
  B11011,
  B11011,
  B11111,
  B11011,
  B11111,
};

const byte info_sign[8] = {
  B11111,
  B11011,
  B11111,
  B11011,
  B11011,
  B11011,
  B11111,
};

const byte g_top_left[8] = {
  B11111,
  B11111,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
};

const byte g_top_right[8] = {
  B11111,
  B11111,
  B00011,
  B00000,
  B00000,
  B00000,
  B00000,
};

const byte g_bottom_left[8] = {
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11111,
  B11111,
};

const byte g_bottom_right[8] = {
  B00000,
  B00111,
  B00111,
  B00011,
  B00011,
  B11111,
  B11111,
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Global Variables                                                               *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// For keeping track of the program state
// the possible states of the state-machine
typedef enum { STATE_noContainer,
               STATE_WaitForSelectionCommand,
               STATE_WaitForPumpCommand,
               STATE_AutoPumpingCommandActive,
               STATE_AutoPumpingFinished,
               STATE_MenuTitle,
               STATE_MenuScreen,
               STATE_ManualContainerCheck,
               STATE_ManualPumping,
               STATE_ManualContainerRemovalCheck,
               STATE_SetPointTitle,
               STATE_SetPointDescription,
               STATE_SetPointMenu,
               STATE_SetPointContainerCheck,
               STATE_SetPointRecordContainerMass,
               STATE_SetPointFillCheck,
               STATE_SetPointRecordFluidMass,
               STATE_SetPointContainerRemoval,
               STATE_ErrorContainerRemoved,
             } states;

states currentProgramState = STATE_noContainer;
bool firstTimeInState = false;

// For calibration
float scalingFactor = 1588.48; //note that this is changed by default on startup by reading custom values from the EEPROM (usually set by utility program pre-production) this is a safety value only

// For pumping trigger
const int outputPinPump1 =  12; // The output pin assigned to this pump, note this is normally pin '12', but 'LED_BUILTIN' can be usefully used for debugging! Note that it can't share the same pins as the LCD shield !
//const int outputPinPump1 =  LED_BUILTIN;
bool outputPinPump1_trigger = false; // Used to call the pump trigger
bool outputPinPump1_triggerActive = false;  // Used to confirm record that it has been implemented
bool outputPinPump_emergencyStop = false; // Used to cancel pumping

// For parameters relating to the mode menu
byte modeSelectionIndex = 0; // Used to index the modeSelection arrays, this defines the default (corresponds to manual Mode)

// For storing the text relating to the modes in an array
const char modeText1[] = "Manual mode";
const char modeText2[] = "Set point mode";

const char * modeStrings[] =
{
  modeText1,
  modeText2,
};

// For parameters relating to the manual pumping mode
bool manualPumpingStarted = false;

// For storing the masses of interest
byte massSelectionIndex = 1; // Used to index the massSelection arrays, this defines the default (corresponds to 0.5L)
int massValues[] = {180, 400, 840, 180}; //note these are changed by default on startup by reading custom values from the EEPROM (usually set by utility program pre-production)
const int fluidInertiaCompensatingFactor = 14; // compensates for the fluid inertia (n.b. used to be set at 15, adjusted to 9 as a test for accuracy. If this is too low the bottle may overfill)

// For storing the text relating to the masses in an array
char massText1[] = " 250ml ";
char massText2[] = " 500ml ";
char massText3[] = "1000ml ";
char massText4[] = "Custom ";

char * massStrings[] =
{
  massText1,
  massText2,
  massText3,
  massText4
};

// For storing the container/bottle mass, and the mass to pump
int containerMass; //used to record the current mass of the container used for receiving liquid
int massToPump = 0; //used to store the current mass to pump

// For storing global signals
bool cancelSignal = false;

// The expected mass of the lightest receptacle and the boolean which keeps track of whether a container is present on the scale
const int expectedContainerMass = 40;
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

  // Set up the LCD ------
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.clear();

  // Start serial communications -----
  Serial.begin(9600);

  // Create custom characters for boot screen
  lcd.createChar(0, g_top_left);
  lcd.createChar(1, g_top_right);
  lcd.createChar(2, g_bottom_left);
  lcd.createChar(3, g_bottom_right);

  // Show the boot screen
  lcdShowGreenGreenBootScreen();
  delay(2000);

  // Create custom characters
  lcd.createChar(0, up_arrow);
  lcd.createChar(1, down_arrow);
  lcd.createChar(2, right_arrow);
  lcd.createChar(3, left_arrow);
  lcd.createChar(4, hour_glass);
  lcd.createChar(5, tick_symbol);
  lcd.createChar(6, warning_mark);
  lcd.createChar(7, info_sign);

  // inform user we are zeroing the scale
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(4));
  lcd.setCursor(4, 0);
  lcd.print(F("Start up"));
  lcd.setCursor(0, 1);
  lcd.print(F("zeroing scale..."));

  // information on GH filter
  if (GH_FILTER_ACTIVE == 1) {
    Serial.println(F("GH filter active"));
  }
  else {
    Serial.println(F("GH filter inactive"));
  }

  // Print a serial mesage with instructions
  Serial.println(F("Program commencing..."));
  Serial.println(F("Note: That scale is zeroed during program start up. The scale must have no objects placed on it at startup."));

  // Read scaling factor from the EEPROM
  EEPROM.get(SCALING_FACTOR_START_ADDRESS, scalingFactor);
  Serial.print(F("ScalingFactor value (from EEPROM): "));
  Serial.println(scalingFactor);

  // Read mass set points from the EEPROM
  int temp_val; // local variable
  for (byte i = 0; i < (sizeof(massValues) / sizeof(massValues[0])); i++) {
    Serial.print(F("Reading mass set-point value for: "));
    Serial.println(massStrings[i]);

    EEPROM.get(MASS_SETPOINT_START_ADDRESS + (2 * i), temp_val); // read the value
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

  // Start communicating with the loadcell -----
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(scalingFactor); // Set scale calibration factor (this is calibrated using the calibration script)
  // Delay to allow any voltage fluctuations to sort themselves out
  delay(3000);
  scale.tare(); // reset the scale to 0, so that the reference mass is set

  // Print a ready message to the LCD.
  delay(500); //delay to allow user to read startup message, then print ready message
  lcdPrintStateNoContainer();

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
          //          Serial.println(F("Debug:S pressed"));
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
      if (debug_verbose) {
        Serial.print(F("  scale output myCurrentMass = "));
        Serial.println(myCurrentMass);
      }

      // the below if statement is active if the G-H filter is active, and the scale is not returning a erroneous negative reading
      if (GH_FILTER_ACTIVE == 1 && !isScaleReadingErroneousDuringPumping(myCurrentMass, lastMassValue)) {
        float myFilterMass = massFilter.return_value(myCurrentMass);
        if (debug_verbose) {
          Serial.print(F("    filter output myFilterMass = "));
          Serial.println(myFilterMass);
        }
        myCurrentMass = myFilterMass;
      }

      bool stopSignalMassExceeded = false;

      // Check if reading is erroneous, and if not see if the mass has been exceeded
      if (!isScaleReadingErroneousDuringPumping(myCurrentMass, lastMassValue)) {
        stopSignalMassExceeded = isMassExceeded(myCurrentMass, massToPump); // returns true if pumping volume is exceeded
      }

      if (!isScaleReadingErroneousDuringPumping(myCurrentMass, lastMassValue)) {
        lastMassValue = myCurrentMass; //store the mass reading for error checking
      }

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

// To manage whether a reading when a container is removed is incorrectly interpreted as a container being placed
bool isContainerRemoved() {
  float thisCurrentMass = scale.get_units(5); //the average of x readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale
  if (debug_verbose) {
    Serial.print(F("isContainerRemoved mass: "));
    Serial.println(thisCurrentMass);
  }

  if (thisCurrentMass <= (-1 * SCALE_ZERO_ERROR_TOLLERANCE)) {
    return true;
  }
  else {
    return false;
  }

}

bool isScaleReadingErroneous(float myReading) {
  if (scaleErrorCatching) {
    if (myReading < NEGATIVE_ERROR_LIMIT) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

bool isScaleReadingErroneousDuringPumping(float myReading, float myLastReading) {
  if (scaleErrorCatching) {

    if (myReading < myLastReading + NEGATIVE_ERROR_TOLLERANCE_DURING_PUMPING || myReading > myLastReading + POSITIVE_ERROR_TOLLERANCE_DURING_PUMPING) {
      //      Serial.println("Reading erroneous");
      return true;
    }
    else {
      //      Serial.println("Reading not erroneous");
      return false;
    }
  }
  else {
    //    Serial.println("Reading not erroneous");
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
    case STATE_noContainer:
    {
      // Container not on the scale
      // -->> continue to check if container is placed on scale then move to next program state
      // -->> continue to check if the 'right' button is pressed, in which case initiate Mass Set Point mode
      if (isContainerPresent()) {
        // -->> first check if the reading was valid, to avoid a state where a removed container is interpreted as a placed container
        if (isContainerRemoved()) {
          currentProgramState = STATE_ErrorContainerRemoved;
          Serial.println(F("Error detected, container removed. Pls reset board"));
          lcd.clear();
          lcd.setCursor(2,0);
          lcd.print(F("Error detected"));
          lcd.setCursor(2,1);
          lcd.print(F("Removed item"));
          lcd.setCursor(0, 0);
          lcd.write(byte(6));
        }
        else {
          currentProgramState = STATE_WaitForSelectionCommand;
          buttonPressActive = false; // reset status to ignore any erroneous button presses and wait for next button press (v2.12 edit)
          Serial.println(F("Select volume to pump using up, down buttons. Then press right key"));
          lcdShowMassSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass (default last selected)
        }
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
            currentProgramState = STATE_MenuTitle;
            lcd.clear();
            lcd.setCursor(7,0);
            lcd.print(F("Menu"));
            lcd.setCursor(3,1);
            lcd.print(F("Continue?"));
            lcd.setCursor(5, 0);
            lcd.write(byte(7));
            lcd.setCursor(0, 1);
            lcd.write(byte(3));
            lcd.setCursor(15, 1);
            lcd.write(byte(2));
            break;
        }
      }

      break;
    }

    case STATE_WaitForSelectionCommand:
    {
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
            if (massSelectionIndex == 3) {
              massSelectionIndex = 0;
            }
            else {
              massSelectionIndex = massSelectionIndex + 1; //keeps mass index in range of menu size
            }
            lcdShowMassSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'D': //down button
            if (massSelectionIndex == 0) {
              massSelectionIndex = ((sizeof(massValues) / 2) - 1);
            }
            else {
              massSelectionIndex = massSelectionIndex - 1; //keeps mass index in range of menu size
            }
            lcdShowMassSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'R': //right button
            currentProgramState = STATE_WaitForPumpCommand;
            Serial.println(F("Check nozzle is inserted in receptacle, confirm with right key"));
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("Check nozzle is"));
            lcd.setCursor(0, 1);
            lcd.write(byte(3));
            lcd.setCursor(3, 1);
            lcd.print(F("inserted?"));
            lcd.setCursor(15, 1);
            lcd.write(byte(2));
            break;
        }
      }

      if (!isContainerPresent()) {
        // if container is removed
        currentProgramState = STATE_noContainer;
        lcdPrintStateNoContainer();
        break;
      }
      break;
    }

    case STATE_WaitForPumpCommand:
    {
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
            currentProgramState = STATE_WaitForSelectionCommand;
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
        currentProgramState = STATE_AutoPumpingCommandActive;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("Pumping! Press"));
        lcd.setCursor(0,1);
        lcd.print(F("'S' to cancel"));
        break;
      }
      if (!isContainerPresent()) {
        // if container is removed
        currentProgramState = STATE_noContainer;
        lcdPrintStateNoContainer();
        break;
      }
      break;
    }

    case STATE_AutoPumpingCommandActive:
    {
      // Pumping command is activated
      // -->> continue to check if pumping is finished, then move to next program state
      // -->> continue to check if cancel key is pressed, then move to next program state. Note that because cancel key is important this functionality is implemented directly in the button press function!
      // -->> continue to check if container is removed from the scale in error, then stop pumping

      if (!outputPinPump1_trigger) {
        // if pumping ends
        currentProgramState = STATE_AutoPumpingFinished;
        firstTimeInState = true;
        break;
      }
      if (!isContainerPresent()) {
        // if container is removed
        cancelSignal = true; // act as a cancel button to stop pumping
        Serial.println(F("Container removed, cancelling pumping!"));
        currentProgramState = STATE_noContainer;
        lcdPrintStateNoContainer();
        break;
      }

      // Note that emergency stop functionality is implemented directly in the button function for quick implementation!

      break;
    }

    case STATE_AutoPumpingFinished:
    {
      // // Pumping is finished
      // -->> waiting for container to be removed
      if (firstTimeInState) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("Finished! remove"));
        lcd.setCursor(0,1);
        lcd.print(F("receptacle"));
        firstTimeInState = false;
      }

      if (!isContainerPresent()) {
        // if container is removed
        currentProgramState = STATE_noContainer;
        buttonPressActive = false; // reset status to wait for next button press
        lcdPrintStateNoContainer();
      }
      break;

    default:
      // statements
      break;
    }

    // --------
    // The cases above this line deal with the core functionality
    // The cases below deal with the mode menu
    // -------

    case STATE_MenuTitle:
    {
      // Menu title being displayed
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = STATE_noContainer;
            lcdPrintStateNoContainer();
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = STATE_MenuScreen;
            buttonPressActive = false; // reset status to wait for next button press
            lcdShowModeSelectionMenu(modeSelectionIndex); //show menu screen for selecting the mass set-point
            break;
        }
      }

      break;
    }

    case STATE_MenuScreen:
    {
      // Mode menu being displayed
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = STATE_noContainer;
            lcdPrintStateNoContainer();
            break;
          case 'U': //up button
            if (modeSelectionIndex == 1) {
              modeSelectionIndex = 0;
            }
            else {
              modeSelectionIndex = modeSelectionIndex + 1; //keeps mode index in range of menu size
            }
            lcdShowModeSelectionMenu(modeSelectionIndex); //show menu screen for selecting the mode
            break;
          case 'D': //down button
            if (modeSelectionIndex == 0) {
              modeSelectionIndex = ((sizeof(modeStrings) / 2) - 1);
            }
            else {
              modeSelectionIndex = modeSelectionIndex - 1; //keeps mode index in range of menu size
            }
            lcdShowModeSelectionMenu(modeSelectionIndex); //show menu screen for selecting the mode
            break;
          case 'R': //right button
            if (modeSelectionIndex == 0) {
              //Move to manual pumping check screen
              currentProgramState = STATE_ManualContainerCheck;
              lcd.clear();
              lcd.setCursor(2,0);
              lcd.print(F("Is container"));
              lcd.setCursor(3,1);
              lcd.print(F("on scale?"));
              lcd.setCursor(0, 1);
              lcd.write(byte(3));
              lcd.setCursor(15, 1);
              lcd.write(byte(2));
            }
            else {
              //Move to set point mode
              currentProgramState = STATE_SetPointTitle;
              Serial.println(F("-- Set point Mode --"));
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print(F("Set-Point Mode"));
              lcd.setCursor(0, 1);
              lcd.write(byte(3));
              lcd.setCursor(3, 1);
              lcd.print(F("Continue?"));
              lcd.setCursor(15, 1);
              lcd.write(byte(2));
            }
            break;
        }
      }

      break;
    }

    // --------
    // The cases above this line deal with the mode menu
    // The cases below deal with manual pumping mode
    // -------

    case STATE_ManualContainerCheck:
    {
      // Manual pumping mode continer check screen
      // -->> continue to check if right (next) button press is issued, then move to the next state
      // -->> continue to check if left button is pressed, then move to previous state

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = STATE_MenuScreen;
            buttonPressActive = false; // reset status to wait for next button press
            lcdShowModeSelectionMenu(modeSelectionIndex); //show menu screen for selecting the mass set-point
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = STATE_ManualPumping;
            //Move to manual pumping mode
            lcd.clear();
            lcd.setCursor(2,0);
            lcd.print(F("Hold RIGHT"));
            lcd.setCursor(0,1);
            lcd.print(F("button to pump"));
            lcd.setCursor(0, 0);
            lcd.write(byte(7));
            buttonPressActive = false; // reset status to wait for next button press
            break;
        }
      }

      break;
    }

    case STATE_ManualPumping:
    {
      // Manual pumping mode
      // -->> continue to check if right (next) button press is issued, then keep pumping
      // -->> continue to check if container is removed then display pumped mass and return to state 0

      int buttonValue = analogRead(ANALOG_SWITCH_PIN);

      if (buttonValue <= 5) {
        // if right button is pressed
        if (!manualPumpingStarted) {
          manualPumpingStarted = true;
          // start pumping and write text to screen
          digitalWrite(outputPinPump1, HIGH);   // turn the output off
          lcd.clear();
          lcd.setCursor(2,0);
          lcd.print(F("To stop pump"));
          lcd.setCursor(0,1);
          lcd.print(F("release button"));
          lcd.setCursor(0, 0);
          lcd.write(byte(7));
        }
        else {
          // do nothing because pump is already running
        }
      }
      else {
        // if right button is not pressed or relased
        if (manualPumpingStarted) {
          manualPumpingStarted = false;
          // stop pumping and write text to screen
          digitalWrite(outputPinPump1, LOW);   // turn the output off
          lcd.clear();
          lcd.setCursor(2,0);
          lcd.print(F("Press LEFT"));
          lcd.setCursor(0,1);
          lcd.print(F("button to finish"));
          lcd.setCursor(0, 0);
          lcd.write(byte(7));
        }
      }

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to next program state
            currentProgramState = STATE_ManualContainerRemovalCheck;
            lcd.clear();
            lcd.setCursor(2,0);
            lcd.print(F("Is container"));
            lcd.setCursor(3,1);
            lcd.print(F("removed?"));
            lcd.setCursor(0, 0);
            lcd.write(byte(7));
            lcd.setCursor(0, 1);
            lcd.write(byte(3));
            lcd.setCursor(15, 1);
            lcd.write(byte(2));
            buttonPressActive = false; // reset status to wait for next button press
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            break;
        }
      }
      
      break;
    }
    
    case STATE_ManualContainerRemovalCheck:
    {
      // Manual pumping mode continer removal check screen
      // -->> continue to check if right (next) button press is issued, then keep pumping
      // -->> continue to check if container is removed then display pumped mass and return to state zero

      if (buttonPressActive) {
        buttonPressActive = false; // reset status to wait for next button press

        switch (lastButtonPressed) {
          case 'S': //select button
            break;
          case 'L': //left button
            // move to previous state
            currentProgramState = STATE_ManualPumping;
            //Move to manual pumping mode
            lcd.clear();
            lcd.setCursor(2,0);
            lcd.print(F("Hold RIGHT"));
            lcd.setCursor(0,1);
            lcd.print(F("button to pump"));
            lcd.setCursor(0, 0);
            lcd.write(byte(7));
            buttonPressActive = false; // reset status to wait for next button press
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            // move to home state
            currentProgramState = STATE_noContainer;
            buttonPressActive = false; // reset status to wait for next button press
            lcdPrintStateNoContainer();
            break;
        }
      }
      break;
    }

    // --------
    // The cases above this line deal with the manual pumping mode
    // The cases below deal with set point mode
    // -------

    case STATE_SetPointTitle:
    {
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
            currentProgramState = STATE_MenuScreen;
            buttonPressActive = false; // reset status to wait for next button press
            lcdShowModeSelectionMenu(modeSelectionIndex); //show menu screen for selecting the mass set-point
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = STATE_SetPointDescription;
            // describe set point mode
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(F("For set-point"));
            lcd.setCursor(0,1);
            lcd.print(F("recalibration"));
            lcd.setCursor(15, 1);
            lcd.write(byte(2));
            buttonPressActive = false; // reset status to wait for next button press
            break;
        }
      }

      break;
    }

    case STATE_SetPointDescription:
    {
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
            currentProgramState = STATE_noContainer;
            buttonPressActive = false; // reset status to wait for next button press
            lcdPrintStateNoContainer();
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = STATE_SetPointMenu;
            buttonPressActive = false; // reset status to wait for next button press
            lcdShowSetPointSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass set-point
            break;
        }
      }

      break;
    }

    case STATE_SetPointMenu:
    {
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
            currentProgramState = STATE_noContainer;
            buttonPressActive = false; // reset status to wait for next button press
            lcdPrintStateNoContainer();
            break;
          case 'U': //up button
            if (massSelectionIndex == 3) {
              massSelectionIndex = 0;
            }
            else {
              massSelectionIndex = massSelectionIndex + 1; //keeps mass index in range of menu size
            }
            lcdShowSetPointSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'D': //down button
            if (massSelectionIndex == 0) {
              massSelectionIndex = ((sizeof(massValues) / 2) - 1);
            }
            else {
              massSelectionIndex = massSelectionIndex - 1; //keeps mass index in range of menu size
            }
            lcdShowSetPointSelectionMenu(massSelectionIndex); //show menu screen for selecting the mass
            break;
          case 'R': //right button
            currentProgramState = STATE_SetPointContainerCheck;
            // ask: Is container placed on scale?
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(F("Is container on"));
            lcd.setCursor(0,1);
            lcd.print(F("scale?"));
            lcd.setCursor(15, 1);
            lcd.write(byte(2));
            Serial.println(F("Check container is on scale"));
            buttonPressActive = false; // reset status to wait for next button press
            break;
        }
      }

      break;
    }

    case STATE_SetPointContainerCheck:
    {
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
            currentProgramState = STATE_noContainer;
            buttonPressActive = false; // reset status to wait for next button press
            lcdPrintStateNoContainer();
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = STATE_SetPointRecordContainerMass;
            buttonPressActive = false; // reset status to wait for next button press
            // move to recording container mass
            break;
        }
      }

      break;
    }

    case STATE_SetPointRecordContainerMass:
    {
      // record container mass, then move to next state
      // -->> continue to check if right (next) button press is issued, then move to next program state
      // -->> continue to check if left  (back) button press is issued, then move to previous program state

      Serial.println(F("Recording container mass"));
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print(F("Recording mass"));
      lcd.setCursor(0,1);
      lcd.print(F("of container..."));
      lcd.setCursor(0, 0);
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

      currentProgramState = STATE_SetPointFillCheck;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("Fill container"));
      lcd.setCursor(0,1);
      lcd.print(F("to set point"));
      lcd.setCursor(15, 1);
      lcd.write(byte(2));
      break;
    }

    case STATE_SetPointFillCheck:
    {
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
            currentProgramState = STATE_noContainer;
            buttonPressActive = false; // reset status to wait for next button press
            lcdPrintStateNoContainer();
            break;
          case 'U': //up button
            break;
          case 'D': //down button
            break;
          case 'R': //right button
            currentProgramState = STATE_SetPointRecordFluidMass;
            buttonPressActive = false; // reset status to wait for next button press
            // move to recording set point mass
            break;
        }
      }
      break;
    }

    case STATE_SetPointRecordFluidMass:
    {
      // record setpoint mass, then move to end state
      // -->> record the set-point mass
      // -->> save it to the correct indexed EEPROM address

      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print(F("Recording mass"));
      lcd.setCursor(0,1);
      lcd.print(F("for set-point"));
      lcd.setCursor(0, 0);
      lcd.write(byte(4));
      massValues[massSelectionIndex] = scale.get_units(3) - containerMass - fluidInertiaCompensatingFactor; //the average of x readings from the ADC minus tare weight, divided by the SCALE parameter set with set_scale
      EEPROM.put(MASS_SETPOINT_START_ADDRESS + (2 * massSelectionIndex), massValues[massSelectionIndex]); // Write the value to EEPRM

      if (debug_verbose) {
        Serial.print(F("Writing value of: "));
        Serial.print(massValues[massSelectionIndex]);
        Serial.print(F("g for the volume: "));
        Serial.println(massStrings[massSelectionIndex]);
      }
      delay(1000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(F("Complete! set-"));
      lcd.setCursor(0,1);
      lcd.print(F("point recorded"));
      delay(2000);

      Serial.print(F("Done! Remove container!"));
      currentProgramState = STATE_SetPointContainerRemoval;
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print(F("Done!"));
      lcd.setCursor(0,1);
      lcd.print(F("Remove container!"));
      lcd.setCursor(0, 0);
      lcd.write(byte(5));
      break;
    }

    case STATE_SetPointContainerRemoval:
    {
      // Wait for container to be removed and then move to the home state
      // -->> continue to check if container is removed (scale reading is close to zero)
      // -->> otherwise remain looping in this state

      if (abs(scale.get_units(3)) < 30) {
        // move to previous state
        currentProgramState = STATE_noContainer;
        buttonPressActive = false; // reset status to wait for next button press
        lcdPrintStateNoContainer();
        Serial.print(F("Place container to start refill"));
      }
      break;
    }


    case STATE_ErrorContainerRemoved:
    {
      // In case container is removed, or error is detected and user needs to manually reset board
      // -->> print error message after some time to allow error code to be displayed
      // -->> Remain in this state indefinitely until the board is reset

      delay(2000);
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print(F("Clear scale"));
      lcd.setCursor(2,1);
      lcd.print(F("Press reset"));
      lcd.setCursor(0, 0);
      lcd.write(byte(6));
      break;
    }

  }

  return true; // to repeat with the timer
}

void lcdPrintStateNoContainer(){
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print(F("Ready..."));
  lcd.setCursor(0,1);
  lcd.print(F("Place container"));
  lcd.setCursor(0, 0);
  lcd.write(byte(5));
}

void emergencyStop() {
  digitalWrite(outputPinPump1, LOW);   // turn the outputs off
  cancelSignal = true; // act as a cancel button to stop pumping
  Serial.println(F("Emergency Stop!"));
}

void lcdShowMassSelectionMenu(byte myIndex) {
  // for the indexes 0-> corresponds to 250ml, 1-> corresponds to 500ml, 2-> corresponds to 1L
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Select volume"));
  lcd.setCursor(0, 1);
  lcd.print(massStrings[myIndex]);
  lcd.write(byte(1));
  lcd.write(byte(0));
  lcd.setCursor(15, 1);
  lcd.write(byte(2));
}

void lcdShowSetPointSelectionMenu(byte myIndex) {
  // for the indexes 0-> corresponds to 250ml, 1-> corresponds to 500ml, 2-> corresponds to 1L, 3-> correponds to 'Custom'
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Select set-point"));
  lcd.setCursor(0, 1);
  lcd.print(massStrings[myIndex]);
  lcd.write(byte(1));
  lcd.write(byte(0));
  lcd.setCursor(15, 1);
  lcd.write(byte(2));
}

void lcdShowModeSelectionMenu(byte myIndex) {
  // for the indexes 0-> corresponds to Manual Mode, 1-> corresponds to Set Point Mode
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print(modeStrings[0]);
  lcd.setCursor(2, 1);
  lcd.print(modeStrings[1]);
  if (myIndex == 0) {
    lcd.setCursor(0, 0);
  }
  else {
    lcd.setCursor(0, 1);
  }
  lcd.write(byte(2));
}

void lcdShowGreenGreenBootScreen(){
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.write(byte(0));
  lcd.write(byte(1));
  lcd.setCursor(1, 1);
  lcd.write(byte(2));
  lcd.write(byte(3));
  lcd.print(F("reen"));

  lcd.setCursor(8, 0);
  lcd.write(byte(0));
  lcd.write(byte(1));
  lcd.setCursor(8, 1);
  lcd.write(byte(2));
  lcd.write(byte(3));
  lcd.print(F("reen"));
}
