#ifndef DEFINES_H_
#define DEFINES_H_

#define VERSION "2.0.0"

#define DEBUG 1
#if DEBUG == 1

#define debug(...) Serial.print("[D] "); Serial.print(__VA_ARGS__)
#define debugln(...) Serial.print("[DEBUG]\t"); Serial.print(__VA_ARGS__); Serial.println()
#define debugf(fmt, ...) \
    Serial.print("[D] "); \
    Serial.printf(fmt, __VA_ARGS__)

#else
#define debug(x)
#define debugln(x)
#define debugf(fmt, ...)
#endif

#define info(...) Serial.print("[L] "); Serial.print(__VA_ARGS__)
#define infoln(...) Serial.print("[LOG]\t"); Serial.print(__VA_ARGS__); Serial.println()
#define infof(fmt, ...) \
    Serial.print("[L] "); \
    Serial.printf(fmt, __VA_ARGS__)

#define warn(...) Serial.print("[W] "); Serial.print(__VA_ARGS__)
#define warnln(...) Serial.print("[WARN]\t"); Serial.print(__VA_ARGS__); Serial.println()
#define warnf(fmt, ...) \
    Serial.print("[W] "); \
    Serial.printf(fmt, __VA_ARGS__)

#define error(...) Serial.print("[E] "); Serial.print(__VA_ARGS__)
#define errorln(...) Serial.print("[ERROR]\t"); Serial.print(__VA_ARGS__); Serial.println()
#define errorf(fmt, ...) \
    Serial.print("[E] "); \
    Serial.printf(fmt, __VA_ARGS__)

#define MAXBATERYVOLTAGE 4.00 // under load 4.2 -> 4.0
#define MINBATERYVOLTAGE 3.40
#define BATTERYSTABTIME \
    1000 // waiting time to stabilize battery voltage after diceState change
#define MOVINGTHRESHOLD \
    0.7                      // maximum acceleration magnitude to detect
                             // nonMoving

#define REGULATOR_PIN GPIO_NUM_18 // pin D9
#define BUTTON_PIN GPIO_NUM_14

#endif /* DEFINES_H_ */
