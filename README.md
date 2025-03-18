# iCubeSmart
Open Source iCubeSmart 3D8RGB LED Cube Firmware


## About

I started by trying to use [Sliicy's 8x8x8-LED Project](https://github.com/Sliicy/8x8x8-LED/) 
However my motherboard is a later  version than Sliicy, that supports up to 32 layers instead of 8.
iCubeSmart does not have the original "bootloader" or source code available; just a dotnet app for control of the firmware shipped with the kit. [Kit on Amazon](https://a.co/d/c1vPxGl)

## Setup VS Code

1) Install [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)

2) Install [Arduino CLI](https://arduino.github.io/arduino-cli/)

3) Install [Arduino Community Edition](https://marketplace.visualstudio.com/items?itemName=vscode-arduino.vscode-arduino-community)

4) Configure Settings
    Since I'm on windows The extension needs these settings:
    **Additional URLS**: https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
    
    **Path**: C:\Program Files\Arduino CLI
    
    **commandPath**: arduino-cli.exe

    and to get intellisense working:

    **c_cpp.intelliSenseEngine**: Tag Parser

5) Open the ino, this will trigger the andrino extension to analyze the code.

6) Arduino Board Configuration: *if you pull this repo the .vscode folder should pull these settings
    In the extensions board manager set:
        Board: Generic STM32F1 Series
        Board Part Number: Generic F103RETx
        Upload method: STM32CubeProgrammer (Serial)

7) Select the COM port

8) Select Arduino: Upload with the cube with boot0:1 boot1:0. Only the blue LED should light on the yellow board if it is ready to flash the main memory.

The cube should now light the 7 colors available in the corners of the cube!

#WIP