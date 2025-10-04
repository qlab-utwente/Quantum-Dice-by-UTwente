//version control see Version.h

#warning "Compile with Pin Numbering By GPIO (legacy)"
#warning "Tested with Arduino ESP32 Boards version 2.0.13"

#include "Version.h"
#include "diceConfig.h"
#include "defines.h"
#include "ImageLibrary/ImageLibrary.h"
#include "ScreenStateDefs.h"
#include "IMUhelpers.h"
#include "Screenfunctions.h"
#include "ESPNowHelpers.h"
#include "handyHelpers.h"
#include "StateMachine.h"

StateMachine stateMachine;

#define UPDATE_INTERVAL 100  //loop functions
unsigned long previousMillisWatchDog = 0;

void setup() {
  //give me some power
  pinMode(REGULATOR_PIN, OUTPUT);
  digitalWrite(REGULATOR_PIN, LOW);

  initSerial();
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  debug("version:");
  debugln(VERSION);
  Serial.print(" MISO: ");
  Serial.print(MISO);
  Serial.print(" MOSI: ");
  Serial.print(MOSI);
  Serial.print(" SCK: ");
  Serial.print(SCK);
  Serial.print(" CS: ");
  Serial.println(SS);

  delay(1000);

  IMUSensor *imuSensor;
  imuSensor = new BNO055IMUSensor();
  imuSensor->init();
  imuSensor->update();
  imuSensor->reset();

  stateMachine.setImuSensor(imuSensor);

  ESPinit();
  initDisplays();
  initRandomGenerators();
  debugln("setup ready!");
  stateMachine.begin();  // Initialize the state machine
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    //button.loop();
    stateMachine.update();
    sendMeasurements(diceNumberSelf, upSideSelf, measureAxisSelf);
  }
}
