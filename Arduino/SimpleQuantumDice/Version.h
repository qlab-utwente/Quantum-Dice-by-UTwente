// version.h
#ifndef VERSION_H
#define VERSION_H

//Version control
//#define VERSION "3.4" //add of blink function, add of coupleforentang state, initentangle is just show time
//#define VERSION "3.5" //add sisterstate into watchdog, remove 2nd FSM, updated waitForThrow state to go to initEntangled state.
//#define VERSION "3.6" //new stability functions
//#define VERSION "3.7" //bno055 instead of BNO085
//#define VERSION "3.8" //new power pcb with on/off button switch. Deep sleep is removed
//#define VERSION "0.1" //dobbelsteen, based on version 3.8
//#define VERSION "0.2" //added second IMU. Change bno to LSnogwat IMU. 
//#define VERSION "0.3" //rolling instead of moving; Separate getAxis function (not used) Bugfix BNO sensor
//#define VERSION "0.4" //replace TFT_espi to Adafruit GFX
//#define VERSION "0.5" //IMU calibration restore and clean up
//#define VERSION "0.51" //use digitalwrite to select screens; gravity calibration, change voltage indicator
#define VERSION "0.52" //tumbling detector from IMU
#endif // VERSION_H