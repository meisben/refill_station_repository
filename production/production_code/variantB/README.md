# Variant B Production Code

## Production code details
Follow the instructions in the table below

### Production code base versions
These are the base 'reference' versions of the code. Updates happen here, are tested on this code, and are manually proliferated to the child versions of the code. Clearly this isn't efficient if the use of this software grows, but it works for prototyping..!

| Refill Station Type | Production code assignment         | Latest Version | Tested? |  Bug list  |
| :-----------------: | ---------------------------------- | :------------: | :-----: | :--------: |
|         Mk1         | variantB_production_refill_station |     v2.12      |   Yes   | None known |

## Production code child versions

| Refill Station UID | Production code assignment          | Latest Version | Tested? | Scale factor used |                                                                  Differences from base version                                                                  |
| :----------------: | ----------------------------------- | :------------: | :-----: | :---------------: | :-------------------------------------------------------------------------------------------------------------------------------------------------------------: |
|       Mk1_1        | variantB2_production_refill_station |     v2.12      |   No    |      112.157      | [A] Scale factor (for hx711 and strain gauge) [B] "int expectedContainerMass = 50;" (solves creep/drift of scale)  [C] alternate button resitance ladder values |
|       Mk1_2        | variantB2_production_refill_station |     v2.12      |   Yes   |      223.61       |                                                                          No difference                                                                          |
|       Mk1_3        | variantB3_production_refill_station |     v2.12      |   Yes   |        214        |                        [A] Scale factor (for hx711 and strain gauge) [B] "int expectedContainerMass = 50;" (solves creep/drift of scale)                        |


## Prerequisites
The following arduino libraries are required to be installed on your system for the full code to work

|        Library        | Tested version |           Authors           |                            URL                             |
| :-------------------: | :------------: | :-------------------------: | :--------------------------------------------------------: |
|    Liquid Crystal     |     v1.07      |   Hans-Christoph Steiner    | [Link](https://github.com/arduino-libraries/LiquidCrystal) |
| HX711 Arduino Library |     v0.74      | Bogdan Necula, Andreas Motl |           [Link](https://github.com/bogde/HX711)           |
|     arduino timer     |     v2.1.0     |      Michael Conteras       |      [Link](https://github.com/contrem/arduino-timer)      |
|      MD_UISwitch      |     v2.1.0     |        MajicDesigns         |    [Link](https://github.com/MajicDesigns/MD_UISwitch)     |

[//]: # (This is the right format for a comment)