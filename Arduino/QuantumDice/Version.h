// version.h
#ifndef VERSION_H
#define VERSION_H

//Version control
//#define VERSION "001" //copy of quantum_dice_v400. First setup. Role definition
//#define VERSION "002" //copy of quantum_dice_v400. Restart. Role definition
//#define VERSION "003" //copy of quantum_dice_v400. Restart. Role definition
//#define VERSION "004" //entanglement colors in diceConfig. Same diceNumber after entanglement
//#define VERSION "005" //incorporate getAxis() into tumble check; change of voltage_indicator function; CS control via digitalWrite
//#define VERSION "051" //restore calibration data added, remove of LSM6DS3TRC option; gravity stability check, added declaring digital pins for cs
//#define VERSION "060" //implementation of IMU classes
//#define VERSION "061" //repair LowVoltage screen
//#define VERSION "062" //speed up setup and new randomgenerator functions
//#define VERSION "065" //merging NANO and DEVKIT
//#define VERSION "070" //implementation new espnow class from M with esp32 core 3.2.1
//#define VERSION "071" //bugfixing implementation new espnow class from M with esp32 core 3.3.0
//#define VERSION "075" //config file in eeprom
//#define VERSION "1.0.0" //release version
#define VERSION "1.0.1B" //added: when in entanglement mode (yellow 1to60) and one dice is measured, the other will have the white 1to6 screen
#endif // VERSION_H
