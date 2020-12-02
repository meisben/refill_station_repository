/*
   ~~~~~Prerequisites~~~~~

   See Github Readme

   ~~~~~Details~~~~~~~~~~~~

   Authors: Ben Money-Coomes
   Date: 2/12/20
   Purpose: Utility program for saving mass set points (and if desired, scaling factor) to an arduino/microcontroller EEPROM so they can be read for production code
   References: See Github Readme

   ~~~~Version Control~~~~~

   v1.0 - [Checkpoint] This version is working and printing values to the serial monitor and LCD screen
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Library Includes.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <LiquidCrystal.h>
#include <EEPROM.h> // for EEPROM

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Definitions                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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

// EEPROM ------------------------------------
// Starting position in the address space
#define  SCALING_FACTOR_START_ADDRESS 0
#define  MASS_SETPOINT_START_ADDRESS 5

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Global Variables                                                               *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// For deciding if scaling factor will be stored
const bool writeScalingFactor = true;
const float scalingFactor = 1588.48; //1588.48 for mk2_varC1

// For storing the values relating to the masses in an array
int massValues[] = {180, 400, 840};

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

// Custom LCD screen characters
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
  Start of main program                                                          *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void setup() {
  //start serial
  Serial.begin(9600);
  Serial.println(F("Program starting -> writing values to EEPROM"));

  // Set up the LCD ------
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.createChar(4, hour_glass);
  lcd.createChar(5, tick_symbol);

  // Write Scaling Factor value if desired
  if(writeScalingFactor){
    lcdPrint("  Writing scale","factor to EEPROM",false);
    lcd.setCursor(0,0);
    lcd.write(byte(4));

    Serial.print(F("Writing scaling factor with value of: "));
    Serial.println(scalingFactor);
    Serial.println(F(""));

    EEPROM.put(SCALING_FACTOR_START_ADDRESS, scalingFactor);
    delay(1000);
  }
   
  // Write EEPROM Values
  lcdPrint("  Writing set-","points to EEPROM",false);
  lcd.setCursor(0,0);
  lcd.write(byte(4));

  //Write the mass set points to the EEPROM, each int takes 2 bytes in the address space  
  for (byte i = 0; i < (sizeof(massValues) / sizeof(massValues[0])); i++) {
    Serial.print(F("Writing value of: "));
    Serial.print(massValues[i]);
    Serial.print(F("g for the volume: "));
    Serial.println(massStrings[i]);
    EEPROM.put(MASS_SETPOINT_START_ADDRESS+(2*i), massValues[i]);
  }
  
  //Test read for each position
  Serial.println(F("Reading written values ...")); 
  float temp_float_val;
  int temp_int_val;

  EEPROM.get(SCALING_FACTOR_START_ADDRESS, temp_float_val);
  Serial.print(F("Scaling factor read value: "));
  Serial.println(temp_float_val);
  
  for (byte i = 0; i < (sizeof(massValues) / sizeof(massValues[0])); i++) {
    Serial.print(F("Reading set-point value for: "));
    Serial.println(massStrings[i]);
    
    EEPROM.get(MASS_SETPOINT_START_ADDRESS+(2*i), temp_int_val);
    Serial.print(F(" Which has value(g) of: "));
    Serial.println(temp_int_val);
  }

  //end program
  Serial.println(F("End of program"));
  lcdPrint("  All complete", "Program ended",false);
  lcd.setCursor(0,0);
  lcd.write(byte(5));
}

void loop() {
  // Nothing required to be done in the loop! this program runs once only :) 
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Functions                                                                      *
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
