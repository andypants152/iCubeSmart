# iCubeSmart
Open Source iCubeSmart 3D8RGB LED Cube Firmware


## About

I started by trying to use [Sliicy's 8x8x8-LED Project](https://github.com/Sliicy/8x8x8-LED/) 
However my motherboard appears to be a different version than Sliicy.

From the issues on his github, someone else seemed to have the issue. 
Their "fix" was flagged as a virus, and the issue closed without real resolution 🙄

Fortunately! my cube does use the same GD32F103RET6 microcontroller.
Initially I followed Sciily's 3D8RGB Build instructions, but have it working in vs code now as well.


## Setup VS Code

1) Install [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)

2) Install [Arduino CLI](https://arduino.github.io/arduino-cli/)

3) Install [Arduino Community Edition](https://marketplace.visualstudio.com/items?itemName=vscode-arduino.vscode-arduino-community)

4) Configure the extension settings
    Since I'm on windows this was:
    Additional URLS: https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
    Path: C:\Program Files\Arduino CLI
    commandPath: arduino-cli.exe

5) Create/Open the ino

6) Arduino Board Configuration:
    In the extensions board manager set:
        Board: Generic STM32F1 Series
        Board Part Number: Generic F103RETx
        Upload method: STM32CubeProgrammer (Serial)

7) Select the COM port

8) Select Arduino: Upload with the cube with boot0:1 boot1:0. Only the blue LED should light on the yellow board if it is ready to flash the main memory.

9) Open the Serial Monitor at baud 115200 to see the live status of the buttons.


## Current Issues

I've not had much luck deciphering the schematics provided by iCubeSmart and have been finding keys1-7 by trial and error. I got key6 working and key5 stopped 😖

The current ino here will show the status of key1-4,6

No Lights work. some flash at power up? 

I'm waiting for either the orignal firmware or a better schematic from iCubeSmart.