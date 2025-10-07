
//version control see Version.h
#warning "Compile with Pin Numbering By GPIO (legacy)"
#warning "ESP version 3.3.0 ,board esp32/Arduino Nano ESP32 or esp32/ESP32S3 Dev Module

#include "Version.h"
#include "diceConfig.h"
#include "defines.h"
#include "ImageLibrary/ImageLibrary.h"
#include "ScreenStateDefs.h"
#include "IMUhelpers.h"
#include "Screenfunctions.h"
#include "handyHelpers.h"
#include "StateMachine.h"

StateMachine stateMachine;

#define UPDATE_INTERVAL 50  //loop functions
unsigned long previousMillisWatchDog = 0;

void setup() {
  //make sure power switch keeps on
  pinMode(REGULATOR_PIN, OUTPUT);
  digitalWrite(REGULATOR_PIN, LOW);

  initSerial();  //  delay(1000); included

  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  debug("version:");
  debug(VERSION);
  debug(" - diceID:");
  debugln(DICE_ID);
#if defined(NANO)
  debugln("nano board");
#endif
#if defined(DEVKIT)
  debugln("devkit board");
#endif

  initDisplays();
  //init display and show something during setup
  displayQLab(ALL);

  IMUSensor *imuSensor;
  imuSensor = new BNO055IMUSensor();

  imuSensor->init();
  imuSensor->update();
  imuSensor->reset();

  stateMachine.setImuSensor(imuSensor);

  initButton();

  initRandomGenerators();
  stateMachine.begin();  // Initialize the state machine

  debugln("setup ready with new IMU-class!");
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    button.loop();
    stateMachine.update();
    //   refreshScreens();
    //   sendWatchDog(); //sendWatchDog removed. Is called at every onEntry function after the states are set. More efficient.
  }
}
