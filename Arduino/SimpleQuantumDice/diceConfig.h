#ifndef DICECONFIG_H_
#define DICECONFIG_H_

/*
Run quantum_dice_diagnosis.ino sketch for both dices. Use Serial monitor to get the mac adress. Copy the output line and paste below for each dice A or B.
Use a 4 letter arbitrary code to identify the dice set.
*/
// List of all dice sets
#define DICE_SET_S000 0  //test set of Aernout
#define DICE_SET_S001 1  //black test set of Bart
#define DICE_SET_S002 2  //white test set of Bart
#define DICE_SET_S003 3  //TPU-set colored Pepijn
#define DICE_SET_S004 4  //TPU-set colored Lennard

//select one of the above
#define SELECTED_DICE_SET DICE_SET_S004

// Configuration for each dice set
#if SELECTED_DICE_SET == DICE_SET_S000
#define DICE_ID "TEST"
#define SMD
inline uint8_t deviceA_mac[6] = { 0xEC, 0xDA, 0x3B, 0x54, 0xAC, 0xD0 };  // MAC address of device A
inline uint8_t deviceB_mac[6] = { 0x74, 0x4D, 0xBD, 0x7E, 0x26, 0x48 };  // MAC address of device B
//background color of display. Select  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORNAG, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
//IMU sensor in use
#define BNO055
//#define LSM6DS3TRC
#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

#elif SELECTED_DICE_SET == DICE_SET_S001
#define DICE_ID "BLCK"
#define HDR
inline uint8_t deviceA_mac[6] = { 0xDC, 0xDA, 0x0C, 0x21, 0x06, 0xD8 };  // MAC address of device A
inline uint8_t deviceB_mac[6] = { 0x74, 0x4D, 0xBD, 0xA0, 0x3D, 0xE0 };  // MAC address of device B
//background color of display
// use one of the following colors:  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORNAG, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
//IMU sensor in use
#define BNO055
//#define LSM6DS3TRC
#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

#elif SELECTED_DICE_SET == DICE_SET_S002
#define DICE_ID "WHITE"
#define HDR
inline uint8_t deviceA_mac[6] = { 0x74, 0x4D, 0xBD, 0xA0, 0x88, 0xEC };  // MAC address of device A
inline uint8_t deviceB_mac[6] = { 0x74, 0x4D, 0xBD, 0xA1, 0x2C, 0x8C };  // MAC address of device B
//background color of display
// use one of the following colors:  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORNAG, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
//IMU sensor in use
#define BNO055
//#define LSM6DS3TRC
#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

// Configuration for each dice set
#elif SELECTED_DICE_SET == DICE_SET_S003
#define DICE_ID "COLOR7"
#define SMD
inline uint8_t deviceA_mac[6] = { 0x3C, 0x84, 0x27, 0xC3, 0x19, 0x44 };
inline uint8_t deviceB_mac[6] = { 0x3C, 0x84, 0x27, 0xC3, 0x18, 0x4 };

//background color of display. Select  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORNAG, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
//IMU sensor in use
#define BNO055
//#define LSM6DS3TRC
#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive

// Configuration for each dice set
#elif SELECTED_DICE_SET == DICE_SET_S004
#define DICE_ID "COLOR7"
#define SMD
inline uint8_t deviceA_mac[6] = { 0x3C, 0x84, 0x27, 0xC2, 0xE4, 0x60 };
inline uint8_t deviceB_mac[6] = { 0x3C, 0x84, 0x27, 0xC2, 0xFE, 0xD8 };

//background color of display. Select  BLACK, BLUE, RED, GREEN, CYAN, MAGNETA, YELLOW, WHITE, ORNAG, GREY, BORDEAUX, DINOGREEN, WHITE
#define X_BACKGROUND GC9A01A_BLACK
#define Y_BACKGROUND GC9A01A_BLACK
#define Z_BACKGROUND GC9A01A_BLACK
//IMU sensor in use
#define BNO055
//#define LSM6DS3TRC
#define RSSILIMIT -35       //RSSI value to detect close by for entanglement. Less negative is less sensitive


#else
#error "Unknown device selected!"
#endif


//other constants to adapt
#define RANDOMSWITCHPOINT 50            //if randomValue < RANDOMSWITCHPOINT -> low, else high
#define TUMBLECONSTANT 0.2              //number of tumbles to detect tumbling over x/y/z axis
#define DEEPSLEEPTIMEOUT 5 * 60 * 1000  //deepsleep timeout; no movement after DEEPSLEEPTIMEOUT -> Deep sleep


#endif /* DICECONFIG_H_ */
