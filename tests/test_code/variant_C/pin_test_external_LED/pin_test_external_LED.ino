/*
   ~~~~~Details~~~~~~~~~~~~
   Authors: Ben Money-Coomes
   Date: 13/9/20
   Purpose: Blink LED connected to arduino pin 12, as well as built in LED (modified version of blink sketch)

    ~~~~Version Control~~~~~
   v2.0 - [Checkpoint] Moving to github repository for version control

*/

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(12,OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(12, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(12, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
