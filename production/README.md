# Testing of the assembly and sub assemblies

This folder contains the production code and associated information for the refill station

## Contents

* Folder contents
* Production code details
* Prerequisites

## Folder contents

|   Folder name   |                                        Contents                                        | Comment |
| :-------------: | :------------------------------------------------------------------------------------: | :-----: |
| production_code | Code for each production varient of the refill station. Details are in the table below |   N/A   |
|     images      |                                   Images for Readme                                    |   N/A   |


## Production code details
Follow the instructions in the table below

### Production code base versions
These are the 'reference' versions of the code. Updates happen here and are manually proliferated to the child versions of the code. Clearly this isn't efficient if the use of this software proliferates, but it works for prototyping.

| Refill Station Type | Production code assignment         | Latest Version | Tested? |                                                     Bug list                                                      |
| :-----------------: | ---------------------------------- | :------------: | :-----: | :---------------------------------------------------------------------------------------------------------------: |
|         Mk1         | variantB_production_refill_station |     v2.11      |   Yes   | (1) When buttons are pressed after 'finish pumping' then pumping will immediately start when new bottle is placed |

## Production code child versions

| Refill Station UID | Production code assignment          | Latest Version | Tested? |
| :----------------: | ----------------------------------- | :------------: | :-----: |
|       Mk1_2        | variantB2_production_refill_station |     v2.11      |   Yes   |
|       Mk1_3        | variantB3_production_refill_station |     v2.11      |   Yes   |




## Prerequisites
All arduino libraries must be installed as per instructions in the main readme (root of this repository)