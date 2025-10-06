#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <EEPROM.h>

/* Set the delay between fresh samples */
#define BNO055_SAMPLERATE_DELAY_MS (100)

// Check I2C device address and correct line below (by default address is 0x29 or 0x28)
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

/**************************************************************************/
/*
    Displays some basic information on this sensor from the unified
    sensor API sensor_t type (see Adafruit_Sensor for more information)
*/
/**************************************************************************/
void displaySensorDetails(void) {
  sensor_t sensor;
  bno.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print("Sensor:       ");
  Serial.println(sensor.name);
  Serial.print("Driver Ver:   ");
  Serial.println(sensor.version);
  Serial.print("Unique ID:    ");
  Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    ");
  Serial.print(sensor.max_value);
  Serial.println(" xxx");
  Serial.print("Min Value:    ");
  Serial.print(sensor.min_value);
  Serial.println(" xxx");
  Serial.print("Resolution:   ");
  Serial.print(sensor.resolution);
  Serial.println(" xxx");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

/**************************************************************************/
/*
    Display some basic info about the sensor status
*/
/**************************************************************************/
void displaySensorStatus(void) {
  /* Get the system status values (mostly for debugging purposes) */
  uint8_t system_status, self_test_results, system_error;
  system_status = self_test_results = system_error = 0;
  bno.getSystemStatus(&system_status, &self_test_results, &system_error);

  /* Display the results in the Serial Monitor */
  Serial.println("");
  Serial.print("System Status: 0x");
  Serial.println(system_status, HEX);
  Serial.print("Self Test:     0x");
  Serial.println(self_test_results, HEX);
  Serial.print("System Error:  0x");
  Serial.println(system_error, HEX);
  Serial.println("");
  delay(500);
}

/**************************************************************************/
/*
    Display sensor calibration status
*/
/**************************************************************************/
void displayCalStatus(void) {
  /* Get the four calibration values (0..3) */
  /* Any sensor data reporting 0 should be ignored, */
  /* 3 means 'fully calibrated" */
  uint8_t system, gyro, accel, mag;
  system = gyro = accel = mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag);

  /* The data should be ignored until the system calibration is > 0 */
  Serial.print("\t");
  if (!system) {
    Serial.print("! ");
  }

  /* Display the individual values */
  Serial.print("Sys:");
  Serial.print(system, DEC);
  Serial.print(" G:");
  Serial.print(gyro, DEC);
  Serial.print(" A:");
  Serial.print(accel, DEC);
  Serial.print(" M:");
  Serial.print(mag, DEC);
}

/**************************************************************************/
/*
    Display the raw calibration offset and radius data
*/
/**************************************************************************/
void displaySensorOffsets(const adafruit_bno055_offsets_t& calibData) {
  Serial.print("Accelerometer: ");
  Serial.print(calibData.accel_offset_x);
  Serial.print(" ");
  Serial.print(calibData.accel_offset_y);
  Serial.print(" ");
  Serial.print(calibData.accel_offset_z);
  Serial.print(" ");

  Serial.print("\nGyro: ");
  Serial.print(calibData.gyro_offset_x);
  Serial.print(" ");
  Serial.print(calibData.gyro_offset_y);
  Serial.print(" ");
  Serial.print(calibData.gyro_offset_z);
  Serial.print(" ");

  Serial.print("\nMag: ");
  Serial.print(calibData.mag_offset_x);
  Serial.print(" ");
  Serial.print(calibData.mag_offset_y);
  Serial.print(" ");
  Serial.print(calibData.mag_offset_z);
  Serial.print(" ");

  Serial.print("\nAccel Radius: ");
  Serial.print(calibData.accel_radius);

  Serial.print("\nMag Radius: ");
  Serial.print(calibData.mag_radius);
}

/**************************************************************************/
/*
    Perform calibration routine and save to EEPROM
*/
/**************************************************************************/
void performCalibration() {
  Serial.println("\n=== Starting Calibration Process ===");

  // // Reset the sensor to clear existing calibration
  // Serial.println("Resetting sensor to clear calibration...");
  // if (!bno.begin()) {
  //   Serial.println("ERROR: Failed to reinitialize sensor!");
  //   return;
  // }

  // // Reconfigure the crystal setting
  // bno.setExtCrystalUse(true);
  // delay(1000);

  // Serial.println("Sensor reset complete. Starting calibration...");
  Serial.println("Please Calibrate Sensor:");
  Serial.println("- Move the sensor around in a figure-8 pattern for magnetometer");
  Serial.println("- Rotate the sensor slowly around all axes for gyroscope");
  Serial.println("- Place the sensor in different orientations for accelerometer");
  Serial.println("");

  sensors_event_t event;
  sensor_t sensor;

  while (!bno.isFullyCalibrated()) {
    bno.getEvent(&event);

    Serial.print("X: ");
    Serial.print(event.orientation.x, 4);
    Serial.print("\tY: ");
    Serial.print(event.orientation.y, 4);
    Serial.print("\tZ: ");
    Serial.print(event.orientation.z, 4);

    /* Display calibration status */
    displayCalStatus();

    /* New line for the next sample */
    Serial.println("");

    /* Wait the specified delay before requesting new data */
    delay(BNO055_SAMPLERATE_DELAY_MS);
  }

  Serial.println("\nFully calibrated!");
  Serial.println("--------------------------------");
  Serial.println("Calibration Results: ");

  // Get the new calibration data
  adafruit_bno055_offsets_t newCalib;
  bno.getSensorOffsets(newCalib);
  displaySensorOffsets(newCalib);

  Serial.println("\n\nStoring calibration data to EEPROM...");

  // Store the sensor ID and calibration data
  int eeAddress = 0;
  bno.getSensor(&sensor);
  long bnoID = sensor.sensor_id;

  EEPROM.put(eeAddress, bnoID);
  eeAddress += sizeof(long);
  EEPROM.put(eeAddress, newCalib);
  EEPROM.commit();  // CRITICAL: Commit changes to flash memory

  Serial.println("Data stored to EEPROM.");

  // Verify the write was successful
  Serial.println("\nVerifying EEPROM write...");
  long verifyID;
  adafruit_bno055_offsets_t verifyCalib;

  eeAddress = 0;
  EEPROM.get(eeAddress, verifyID);
  eeAddress += sizeof(long);
  EEPROM.get(eeAddress, verifyCalib);

  if (verifyID == bnoID && verifyCalib.accel_offset_x == newCalib.accel_offset_x && verifyCalib.accel_offset_y == newCalib.accel_offset_y && verifyCalib.accel_offset_z == newCalib.accel_offset_z) {
    Serial.println("EEPROM write verification PASSED");
  } else {
    Serial.println("EEPROM write verification FAILED");
  }

  Serial.println("\n================================");
  Serial.println("Calibration complete!");
  Serial.println("Type 'C' to recalibrate anytime");
  Serial.println("================================\n");
}

/**************************************************************************/
/*
    Arduino setup function (automatically called at startup)
*/
/**************************************************************************/
void setup(void) {
  Serial.begin(115200);
  EEPROM.begin(512);  // Initialize EEPROM with sufficient size
  delay(1000);
  Serial.println("Orientation Sensor Test");
  Serial.println("");

  /* Initialise the sensor */
  if (!bno.begin()) {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while (1)
      ;
  }

  int eeAddress = 0;
  long bnoID;
  bool foundCalib = false;

  EEPROM.get(eeAddress, bnoID);

  adafruit_bno055_offsets_t calibrationData;
  sensor_t sensor;

  /*
   *  Look for the sensor's unique ID at the beginning of EEPROM.
   *  This isn't foolproof, but it's better than nothing.
   */
  bno.getSensor(&sensor);
  if (bnoID != sensor.sensor_id) {
    Serial.println("\nNo Calibration Data for this sensor exists in EEPROM");
    foundCalib = false;
    delay(500);
  } else {
    Serial.println("\nFound Calibration for this sensor in EEPROM.");
    eeAddress += sizeof(long);
    EEPROM.get(eeAddress, calibrationData);

    displaySensorOffsets(calibrationData);

    Serial.println("\n\nRestoring Calibration data to the BNO055...");
    bno.setSensorOffsets(calibrationData);

    Serial.println("\n\nCalibration data loaded into BNO055");
    foundCalib = true;
  }

  delay(1000);

  /* Display some basic information on this sensor */
  displaySensorDetails();

  /* Optional: Display current status */
  displaySensorStatus();

  /* Crystal must be configured AFTER loading calibration data into BNO055. */
  bno.setExtCrystalUse(true);

  // Only start calibration process if no calibration data was found
  if (!foundCalib) {
    performCalibration();
  } else {
    Serial.println("\n=== Using Stored Calibration ===");
    Serial.println("Sensor is ready to use with stored calibration data.");

    // Optional: Move sensor slightly to recalibrate magnetometer
    // as it can drift over time
    Serial.println("\nNote: You may want to move the sensor slightly");
    Serial.println("to recalibrate the magnetometer if needed.");
    Serial.println("\n>>> Type 'C' in the Serial Monitor to start recalibration <<<\n");
  }

  Serial.println("\n================================");
  Serial.println("Setup complete. Starting main loop...");
  Serial.println("================================\n");
  delay(500);
}

void loop() {
  // Check for serial input to trigger recalibration
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    // Convert to uppercase
    if (inChar >= 'a' && inChar <= 'z') {
      inChar = inChar - 32;
    }

    if (inChar == 'C') {
      Serial.println("\n\n*** RECALIBRATION REQUESTED ***");
      Serial.println("Clearing any buffered serial data...");

      // Clear the serial buffer
      while (Serial.available() > 0) {
        Serial.read();
      }

      // Perform the calibration
      performCalibration();
    }
  }

  // Get sensor events
  sensors_event_t orientationData, angVelocityData, linearAccelData, magnetometerData, accelerometerData, gravityData;
  bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  bno.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
  bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
  bno.getEvent(&magnetometerData, Adafruit_BNO055::VECTOR_MAGNETOMETER);
  bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  bno.getEvent(&gravityData, Adafruit_BNO055::VECTOR_GRAVITY);

  printEvent(&orientationData);
  printEvent(&angVelocityData);
  printEvent(&linearAccelData);
  printEvent(&magnetometerData);
  printEvent(&accelerometerData);
  printEvent(&gravityData);

  /* Optional: Display calibration status */
  displayCalStatus();

  /* New line for the next sample */
  Serial.println("");

  /* Wait the specified delay before requesting new data */
  delay(BNO055_SAMPLERATE_DELAY_MS);
}

void printEvent(sensors_event_t* event) {
  double x = -1000000, y = -1000000, z = -1000000, mag = 0;  //dumb values, easy to spot problem
  if (event->type == SENSOR_TYPE_ACCELEROMETER) {
    Serial.print("Accl:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
  } else if (event->type == SENSOR_TYPE_ORIENTATION) {
    Serial.print("Orient:");
    x = event->orientation.x;
    y = event->orientation.y;
    z = event->orientation.z;
  } else if (event->type == SENSOR_TYPE_MAGNETIC_FIELD) {
    Serial.print("Mag:");
    x = event->magnetic.x;
    y = event->magnetic.y;
    z = event->magnetic.z;
  } else if (event->type == SENSOR_TYPE_GYROSCOPE) {
    Serial.print("Gyro:");
    x = event->gyro.x;
    y = event->gyro.y;
    z = event->gyro.z;
  } else if (event->type == SENSOR_TYPE_ROTATION_VECTOR) {
    Serial.print("Rot:");
    x = event->gyro.x;
    y = event->gyro.y;
    z = event->gyro.z;
  } else if (event->type == SENSOR_TYPE_LINEAR_ACCELERATION) {
    Serial.print("Linear:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
    mag = sqrt(x * x + y * y + z * z);
    Serial.print(mag);
    if (mag > 0.4) Serial.print(" ALARM");
  } else if (event->type == SENSOR_TYPE_GRAVITY) {
    Serial.print("Gravity:");
    x = event->acceleration.x;
    y = event->acceleration.y;
    z = event->acceleration.z;
  } else {
    Serial.print("Unk:");
  }

  Serial.print("\tx= ");
  Serial.print(x);
  Serial.print(" |\ty= ");
  Serial.print(y);
  Serial.print(" |\tz= ");
  Serial.println(z);
}