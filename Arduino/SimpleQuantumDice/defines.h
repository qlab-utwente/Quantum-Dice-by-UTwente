#ifndef DEFINES_H_
#define DEFINES_H_


#define DEBUG 1

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

#define MAXBATERYVOLTAGE 4.00 //under load 4.2 -> 4.0
#define MINBATERYVOLTAGE 3.40
#define BATTERYSTABTIME 1000 //waiting time to stabilize battery voltage after diceState change
#define MOVINGTHRESHOLD 0.7 //maximum acceleration magnitude to detect nonMoving

#define REGULATOR_PIN GPIO_NUM_18 //pin D9
#define BUTTON_PIN GPIO_NUM_14


#endif /* DEFINES_H_ */