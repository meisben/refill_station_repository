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

| Test ID        | What is tested?       |Associated test code  | Version         | Stage  | Condition | Test steps | Pass criteria
| :-----------:|-------------|:-------------:| ----| ----:| ----| ----| ----|
| Test A1 | Arduino board   | Example blink code (from arduino IDE) | N/A |After wiring to arduino | N/A | Execute code | Built in LED flashes
| Test B1 | Hook up wires: (a) on pin 12, (b) matching ground    | pin_test_external_LED.ino | v2.0 | After wiring to arduino   | Pin #12 hookup wires are connected to LED tester | Execute code | LED tester flashes
| Test C1 | Hook up wires: for arduino power source (a) live, (b) matching ground | pin_test_external_LED.ino | v2.0  | After wiring to arduino | Pin #12 hookup wires are connected to LED tester | Execute code | LED tester flashes
| Test D1 | (a) HX711, (b) strain gauage assembly | strain_gauge_raw_test_hx711.ino | v1.1 | After asembling hx711 and strain gauge | hx711 and strain gauage should be connected and wired into frame. LCD screen can be connected | Execute code | Raw readings are distinct for each weight (TBC)
| Test E1 | LED shield | TBC | vTBC | After testing steps above | Pin #12 hookup wires are connected to LED tester | Execute code | LED tester flashes


## Prerequisites
All arduino libraries must be installed as per instructions in the main readme (root of this repository)