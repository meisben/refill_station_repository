/*
   ~~~~~Prerequisites~~~~~

   TBC

   ~~~~~Details~~~~~~~~~~~~

   Authors: Ben Money-Coomes
   Date: 7/7/20
   Purpose: Test buttons using MD_UISwitch library - > monitor_button_resitive_ladder_a2d_input
   References: See OneNote

   ~~~~Version Control~~~~~

   v1.0 - [Checkpoint] This version is working and printing the analog values of the button to the serial monitor

   ----Notes---
   For the LCD Keypad shield from ebay the anaolgue input values at pin A0 are as follows:
   No button: 1023
   Select: 640
   Left: 410
   Up: 100
   Down: 257
   Right: 0

*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   Library Includes.                                                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <MD_UISwitch.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Definitions                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int sensorPin = A0;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor



void setup() {
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:


  // read the value from the sensor:
  sensorValue = analogRead(sensorPin);

  Serial.print("Value: ");
  Serial.println(sensorValue);

  // stop the program for for <sensorValue> milliseconds:
  delay(100);

}
