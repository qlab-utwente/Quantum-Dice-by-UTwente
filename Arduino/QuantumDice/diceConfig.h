#ifndef DICECONFIG_H_
#define DICECONFIG_H_

/*
macAddresses via Tools-Get Board Info or run getMacAddress.ino sketch
*/
// List of all dice sets
#define DICE_SET_S000 0  //TEST1
#define DICE_SET_S001 1  //BART1
#define DICE_SET_S002 2  //BART2
#define DICE_SET_S003 3  //SQD1
#define DICE_SET_S004 4  //non existent
#define DICE_SET_S005 5  //TELEP

//select one of the above
#define SELECTED_DICE_SET DICE_SET_S000

// Configuration for each dice set

//**********************************************//
//ESP32 SMD v3.2 n16r8,
#if SELECTED_DICE_SET == DICE_SET_S000
#define DICE_ID "TEST1"
#define SMD //default SMD. Optional HDR for ancient processor boards
#define DEVKIT //default DEVKIT. Optional NANO for processorPCB based on Arduino ESP32 nano board

inline uint8_t deviceA_mac[6] = { 0xD0, 0xCF, 0x13, 0x36, 0x40, 0x88 }; // MAC address of device A
inline uint8_t deviceB1_mac[6] = {0xD0, 0xCF, 0x13, 0x33, 0x58, 0x5C };  // MAC address of device B
inline uint8_t deviceB2_mac[6] = { 0xDC, 0xDA, 0xC, 0x21, 0x2, 0x44 };  // DUMMY. Replace with actual mac adress. Use 3rd dice with the teleportation experiment

//background color of display. Select  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORNAG, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
#define ENTANG_AB1_COLOR GC9A01A_YELLOW
#define ENTANG_AB2_COLOR GC9A01A_GREEN

#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

//**********************************************//
//Black set of Bart (nano)
#elif SELECTED_DICE_SET == DICE_SET_S001
#define DICE_ID "BART1"
#define HDR
#define NANO //default DEVKIT

inline uint8_t deviceA_mac[6] = { 0xDC, 0xDA, 0x0C, 0x21, 0x06, 0xD8 };  // MAC address of device A
inline uint8_t deviceB1_mac[6] = { 0x74, 0x4D, 0xBD, 0xA0, 0x3D, 0xE0 };  // MAC address of device B
inline uint8_t deviceB2_mac[6] = { 0xDC, 0xDA, 0xC, 0x21, 0x2, 0x44 };  // DUMMY. Replace with actual mac adress
//background color of display
// use one of the following colors:  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORANGE, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK

#define ENTANG_AB1_COLOR GC9A01A_YELLOW
#define ENTANG_AB2_COLOR GC9A01A_GREEN

#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

//**********************************************//
//White set of Bart
#elif SELECTED_DICE_SET == DICE_SET_S002
#define DICE_ID "BART2"
#define SMD
#define NANO //default DEVKIT

inline uint8_t deviceA_mac[6] = { 0x3C, 0x84, 0x27, 0xC2, 0xE4, 0x60 };
inline uint8_t deviceB1_mac[6] = { 0x3C, 0x84, 0x27, 0xC2, 0xFE, 0xD8 };
inline uint8_t deviceB2_mac[6] = { 0xDC, 0xDA, 0xC, 0x21, 0x2, 0x44 };  // DUMMY. Replace with actual mac adress

// #define HDR
// inline uint8_t deviceA_mac[6] = { 0x74, 0x4D, 0xBD, 0xA0, 0x88, 0xEC };  // MAC address of device A
// inline uint8_t deviceB1_mac[6] = { 0x74, 0x4D, 0xBD, 0xA1, 0x2C, 0x8C };  // MAC address of device B
// inline uint8_t deviceB2_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
//background color of display
// use one of the following colors:  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORNAG, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
#define ENTANG_AB1_COLOR GC9A01A_YELLOW
#define ENTANG_AB2_COLOR GC9A01A_GREEN

#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

//**********************************************//
// set of Pepijn
#elif SELECTED_DICE_SET == DICE_SET_S003 //set Pepijn
#define DICE_ID "SQD1"
#define SMD
#define NANO //default DEVKIT

inline uint8_t deviceA_mac[6] = { 0x3C, 0x84, 0x27, 0xC3, 0x19, 0x44 };
inline uint8_t deviceB1_mac[6] = { 0x3C, 0x84, 0x27, 0xC3, 0x18, 0x4 };
inline uint8_t deviceB2_mac[6] = { 0xDC, 0xDA, 0xC, 0x21, 0x2, 0x44 };  // DUMMY. Replace with actual mac adress

//background color of display. Select  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORNAG, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
#define ENTANG_AB1_COLOR GC9A01A_YELLOW
#define ENTANG_AB2_COLOR GC9A01A_GREEN

#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

//**********************************************//
//teleportation triple black
#elif SELECTED_DICE_SET == DICE_SET_S005
#define DICE_ID "TELEP"
#define SMD
#define NANO //default DEVKIT

inline uint8_t deviceB1_mac[6] = { 0x3C, 0x84, 0x27, 0xC3, 0xCD, 0x60 };  // MAC address of device B1 SMD
inline uint8_t deviceB2_mac[6] = { 0xDC, 0xDA, 0xC, 0x21, 0x2, 0x44 };  // MAC address of device B2 SMD
inline uint8_t deviceA_mac[6] = { 0x3C, 0x84, 0x27, 0xC2, 0xCC, 0x54 };  //SMD

//force the dices to always produce 7, no matter which axis is measured. Uncomment to ignore
#define ALWAYS_SEVEN

// use one of the following colors:  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORANGE, GREY, BORDEAUX, DINOGREEN, WHITE
//background color of display
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
#define ENTANG_AB1_COLOR GC9A01A_YELLOW
#define ENTANG_AB2_COLOR GC9A01A_GREEN

#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

//**********************************************//


#else
#error "Unknown device selected!"
#endif

//**********************************************//

//other constants to adapt
#define RANDOMSWITCHPOINT 50            //if randomValue < RANDOMSWITCHPOINT -> low, else high
#define TUMBLECONSTANT 0.2              //number of tumbles to detect tumbling over x/y/z axis
#define DEEPSLEEPTIMEOUT 5 * 60 * 1000  //deepsleep timeout; no movement after DEEPSLEEPTIMEOUT -> Deep sleep


#endif /* DICECONFIG_H_ */
