# Variant C Production Code

## Production code details
Follow the instructions in the table below

This version of the code reads prototype specific settings (HX711 scale factor, custom mass set points) from the EEPROM on the arduino

| Refill Station Type | Production code assignment           | Latest Version | Tested? |  Bug list  |
| :-----------------: | ------------------------------------ | :------------: | :-----: | :--------: |
|         Mk2         | variantC14_production_refill_station |   vC_14.5.0    |   Yes   | None known |


## Prerequisites
The following arduino libraries are required to be installed on your system for the full code to work

|        Library        | Tested version |           Authors           |                            URL                             |
| :-------------------: | :------------: | :-------------------------: | :--------------------------------------------------------: |
|    Liquid Crystal     |     v1.07      |   Hans-Christoph Steiner    | [Link](https://github.com/arduino-libraries/LiquidCrystal) |
| HX711 Arduino Library |     v0.74      | Bogdan Necula, Andreas Motl |           [Link](https://github.com/bogde/HX711)           |
|     arduino timer     |     v2.1.0     |      Michael Conteras       |      [Link](https://github.com/contrem/arduino-timer)      |
|      MD_UISwitch      |     v2.1.0     |        MajicDesigns         |    [Link](https://github.com/MajicDesigns/MD_UISwitch)     |

[//]: # (This is the right format for a comment)