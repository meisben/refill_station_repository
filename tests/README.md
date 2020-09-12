# Testing of the assembly and sub assemblies

This folder contains the testing information for the refill station

## Contents

* Folder contents
* Test Equipment required
* Test Method
* Prerequisites

## Folder contents

| Folder name        | Contents           | Comment  |
| :-----------: |:-------------:| :----:|
| test_code      | Code for each test | N/A |
| images | Images for Readme      |    N/A |

## Test equipment required

| Test equipment name        | Consists of           | Comment  |
| :-----------: |:-------------:| :----:|
| LED tester      | 1 x mini breadboard, 1 x LED, 1 x resistor(TBC ohms) | N/A |


## Test method
Follow the instructions in the table below

| Test ID        | What is tested?       |Associated test code           | Stage  | Condition
| :-----------:|-------------|:-------------:| ----| ----|
| Test A1 | Arduino board   | Example blink code (from arduino IDE) | After wiring to arduino | N/A
| Test B1 | Hook up wire on pin 12   | pin_test_led_v2.ino  | After wiring to arduino   | Pin #12 hookup wires are connected to LED tester |
| Test C1 | Arduino power source hookup wires | pin_test_led_v2.ino | After wiring to arduino | Pin #12 hookup wires are connected to LED tester |
| Test D1 | HX711 | hx711_strain_gauge_v1.1.ino | After asembling hx711 and strain gauge | hx711 and strain gauage should be connected and wired into frame |
| Test E1 | LED shield | TBC | After testing steps above | Pin #12 hookup wires are connected to LED tester |


## Prerequisites
All arduino libraries must be installed as per instructions in the main readme (root of this repository)