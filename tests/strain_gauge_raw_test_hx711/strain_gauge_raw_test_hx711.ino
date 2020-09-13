/*
   ~~~~~Prerequisites~~~~~

   - Installation of HX711 library
   - Installation of LiquidCrystal library

   ~~~~~Details~~~~~~~~~~~~

   Authors: Ben Money-Coomes
   Date: 13/9/20
   Purpose: Strain gauge raw readings are printed on the serial montior, and if and LCD screen is connected to the LCD
   
   ~~~~Version Control~~~~~

   v1.0 - [Checkpoint] This version is working and printing values to the serial monitor
   v1.1 - [Checkpoint] Moving to Github repository for refill station
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Library Includes.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <HX711.h>
#include <LiquidCrystal.h>


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

// HX711 strain gauge -----------------------------
// HX711  definitions
HX711 scale;
long int values = 0;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// Note that when using the LCD Keypad Shield then pin 2 is the 3rd pin from the rhs and pin 3 is the 4th pin from the rhs

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Start of main program                                                          *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void setup()
{
  Serial.begin(9600);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);


  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Strain gauge raw value:");
}

void loop()
{
  //values = scale.read();
  //Serial.println(values);
  Serial.print("Value: ");
  Serial.println(scale.read());


  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the stain gauge raw value:
  lcd.print(scale.read());

  delay(500);
}
