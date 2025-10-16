#include "Adafruit_Sensor.h"
#include "Arduino.h"
#include "defines.h"
#include "IMUhelpers.h"
#include "handyHelpers.h"  // Include for EEPROM address definitions

//***************************** IMU independant functions
void IMUSensor::updateUpVector(double deltaTime) {
  // Calculate the new Up
  double xRot = _xGyro * deltaTime;  //rotation angle over the last deltaTime
  double yRot = _yGyro * deltaTime;
  double zRot = _zGyro * deltaTime;

  // // Exponential moving average filter (https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average)
  // xRot = ALPHA * xRot + (1 - ALPHA) * _xRotPrev;
  // yRot = ALPHA * yRot + (1 - ALPHA) * _yRotPrev;
  // zRot = ALPHA * zRot + (1 - ALPHA) * _zRotPrev;
  //
  // _xRotPrev = xRot;
  // _yRotPrev = yRot;
  // _zRotPrev = zRot;

  // debug("Rotation ");
  // debug(deltaTime);
  // debug(", ");
  // debug(xRot);
  // debug(", ");
  // debug(yRot);
  // debug(", ");
  // debugln(zRot);

  // x-axis rotation matrix
  double xUp = _xUp;
  double yUp = _yUp * cos(xRot) - _zUp * sin(xRot);
  double zUp = _yUp * sin(xRot) + _zUp * cos(xRot);

  _xUp = xUp;
  _yUp = yUp;
  _zUp = zUp;

  // y-axis
  xUp = _xUp * cos(yRot) + _zUp * sin(yRot);
  yUp = _yUp;
  zUp = -_xUp * sin(yRot) + _zUp * cos(yRot);

  _xUp = xUp;
  _yUp = yUp;
  _zUp = zUp;

  // z-axis
  xUp = _xUp * cos(zRot) - _yUp * sin(zRot);
  yUp = _xUp * sin(zRot) + _yUp * cos(zRot);
  zUp = _zUp;

  _xUp = xUp;
  _yUp = yUp;
  _zUp = zUp;

  // debug("Up (");
  // debug(_xUp);
  // debug(", ");
  // debug(_yUp);
  // debug(", ");
  // debug(_zUp);
  // debugln(")");
}

void IMUSensor::reset() {
  _prevMicros = micros();

  float inverseMagnitude = 1.0 / sqrt((_xGravity * _xGravity + _yGravity * _yGravity + _zGravity * _zGravity));

  _xUpStart = -_xGravity * inverseMagnitude;  //unit vector
  _yUpStart = -_yGravity * inverseMagnitude;
  _zUpStart = -_zGravity * inverseMagnitude;

  _xUp = _xUpStart;
  _yUp = _yUpStart;
  _zUp = _zUpStart;

  debug("Reset (");
  debug(_xUpStart);
  debug(", ");
  debug(_yUpStart);
  debug(", ");
  debug(_zUpStart);
  debug(", ");
  debugln(")");
}

bool IMUSensor::tumbled(float minRotation) {
  // debug("Start Up (");
  // debug(_xUpStart, 2);
  // debug(", ");
  // debug(_yUpStart, 2);
  // debug(", ");
  // debug(_zUpStart, 2);
  // debug(", ");
  // //debugln(")");

  // //debug("Up (");
  // debug(_xUp, 2);
  // debug(", ");
  // debug(_yUp, 2);
  // debug(", ");
  // debug(_zUp, 2);
  // debug(", ");
  //debugln(")");
  double dotProduct = _xUp * _xUpStart + _yUp * _yUpStart + _zUp * _zUpStart;

  // Clamp dot product to valid range for acos [-1, 1]
  dotProduct = constrain(dotProduct, -1.0, 1.0);
  double rotation = acos(dotProduct) / TWOPI;  //inverse cos of dot product between vectors, normalised (2PI=1.0). Max rotation: 0.5

  if (abs(rotation) > (double)minRotation) {
    debug("Start Up (");
    debug(_xUpStart);
    debug(", ");
    debug(_yUpStart);
    debug(", ");
    debug(_zUpStart);
    debug(", ");
    debugln(")");

    debug("Up (");
    debug(_xUp);
    debug(", ");
    debug(_yUp);
    debug(", ");
    debug(_zUp);
    debug(", ");
    debugln(")");
    debug("Rotation: ");
    debug(rotation);
    debugln();
    reset();
    return true;
  }
  return false;
}

bool IMUSensor::isMoving() {
  // Check if magnitude is below the threshold. If it was moving, set it to false and keep the timestamp of that moment
  if (_magnitude < threshold) {
    if (_isMoving) {
      _lastMovementTime = millis();
      _isMoving = false;
    }
  } else {
    _isMoving = true;
  }
  if (!_isMoving && (millis() - _lastMovementTime > stableTime)) {
    return false;
  } else {
    return true;
  }
}

Adafruit_BNO055 AccGyro = Adafruit_BNO055(55, 0x28, &Wire);
sensors_event_t angVelocityData, linearAccelData, gravityData;

void BNO055IMUSensor::init() {
  Wire.begin();
  // Note: EEPROM is already initialized by initEEPROM() in handyHelpers
  // Don't call EEPROM.begin() here again
  
  while (!_accGyro.begin()) {
    debugln("BNO device not detected at default I2C address");
    delay(100);
  }
  debugln("BNO device found!");

  // Try to restore calibration data from EEPROM
  restoreCalibrationData();

  // Set external crystal use (must be done after loading calibration)
  _accGyro.setExtCrystalUse(true);

  // Wait for valid gravity data before calling reset
  debugln("Waiting for valid gravity data...");
  int attempts = 0;
  float gravityMagnitude = 0.0;

  do {
    delay(100);
    update();  // Get sensor reading
    attempts++;

    gravityMagnitude = sqrt(_xGravity * _xGravity + _yGravity * _yGravity + _zGravity * _zGravity);

    debug("Attempt ");
    debug(attempts);
    debug(" - Gravity: (");
    debug(_xGravity);
    debug(", ");
    debug(_yGravity);
    debug(", ");
    debug(_zGravity);
    debug(") Magnitude: ");
    debugln(gravityMagnitude);

  } while (gravityMagnitude < 8.0 && attempts < 100);

  if (gravityMagnitude >= 8.0) {
    reset();
    debugln("Up vector initialized successfully");
  } else {
    debugln("Warning: Failed to get valid gravity data after 100 attempts!");
    // Set a default up vector as fallback
    _xUp = 0.0;
    _yUp = 0.0;
    _zUp = 1.0;
    _xUpStart = 0.0;
    _yUpStart = 0.0;
    _zUpStart = 1.0;
  }

  debugln("IMU initialization complete");
}

void BNO055IMUSensor::restoreCalibrationData() {
  long bnoID;
  bool foundCalib = false;

  // Get stored sensor ID from EEPROM using the new address
  EEPROM.get(EEPROM_BNO_SENSOR_ID_ADDR, bnoID);

  // Get current sensor info
  sensor_t sensor;
  _accGyro.getSensor(&sensor);

  debug("Current sensor ID: ");
  debugln(sensor.sensor_id);
  debug("EEPROM stored ID: ");
  debugln(bnoID);

  // Check if we have calibration data for this sensor
  if (bnoID != sensor.sensor_id) {
    debugln("No calibration data found in EEPROM for this sensor");
    debugln("Run calibration sketch first to store calibration data");
  } else {
    debugln("Found calibration data in EEPROM");

    // Read calibration data from the new address
    adafruit_bno055_offsets_t calibrationData;
    EEPROM.get(EEPROM_BNO_CALIBRATION_ADDR, calibrationData);

    // Display what we're loading
    debugln("Loading calibration offsets:");
    displaySensorOffsets(calibrationData);

    // Apply calibration data to sensor
    _accGyro.setSensorOffsets(calibrationData);

    debugln("Calibration data restored successfully");
    foundCalib = true;
  }

  // Optional: Display current calibration status
  delay(100);  // Give sensor time to settle
  displayCalStatus();
}

void BNO055IMUSensor::displaySensorOffsets(const adafruit_bno055_offsets_t &calibData) {
  debug("Accel: ");
  debug(calibData.accel_offset_x);
  debug(" ");
  debug(calibData.accel_offset_y);
  debug(" ");
  debug(calibData.accel_offset_z);
  debug(" ");

  debug(" | Gyro: ");
  debug(calibData.gyro_offset_x);
  debug(" ");
  debug(calibData.gyro_offset_y);
  debug(" ");
  debug(calibData.gyro_offset_z);
  debug(" ");

  debug(" | Mag: ");
  debug(calibData.mag_offset_x);
  debug(" ");
  debug(calibData.mag_offset_y);
  debug(" ");
  debug(calibData.mag_offset_z);
  debug(" ");

  debug(" | Radii: A=");
  debug(calibData.accel_radius);
  debug(" M=");
  debugln(calibData.mag_radius);
}

void BNO055IMUSensor::displayCalStatus(void) {
  /* Get the four calibration values (0..3) */
  /* Any sensor data reporting 0 should be ignored, */
  /* 3 means 'fully calibrated" */
  uint8_t system, gyro, accel, mag;
  system = gyro = accel = mag = 0;
  AccGyro.getCalibration(&system, &gyro, &accel, &mag);

  /* Display the individual values */
  Serial.print("Calibration Status - Sys:");
  Serial.print(system, DEC);
  Serial.print(" G:");
  Serial.print(gyro, DEC);
  Serial.print(" A:");
  Serial.print(accel, DEC);
  Serial.print(" M:");
  Serial.print(mag, DEC);

  if (system == 0) {
    debug(" [!] System not calibrated - data should be ignored");
  }
  debugln();
}

void BNO055IMUSensor::update() {
  unsigned long currentMicros = micros();
  double deltaTime = (currentMicros - _prevMicros) * 1e-6;
  sensors_event_t angVelocityData, linearAccelData, gravityData;
  _accGyro.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
  _accGyro.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
  _accGyro.getEvent(&gravityData, Adafruit_BNO055::VECTOR_GRAVITY);
  processData(&angVelocityData);
  processData(&linearAccelData);
  processData(&gravityData);
  updateUpVector(deltaTime);
  _prevMicros = currentMicros;
}

void BNO055IMUSensor::processData(sensors_event_t *event) {
  switch (event->type) {
    case SENSOR_TYPE_LINEAR_ACCELERATION:
      _ax = event->acceleration.x;
      _ay = event->acceleration.y;
      _az = event->acceleration.z;

      // Calculate the magnitude of the linear acceleration
      _magnitude = sqrt(_ax * _ax + _ay * _ay + _az * _az);
      // debug("Acc read: ");
      // debug(_ax);
      // debug(", ");
      // debug(_ay);
      // debug(", ");
      // debug(_az);
      // debugln();
      break;

    case SENSOR_TYPE_GYROSCOPE:
      _xGyro = event->gyro.x;
      _yGyro = event->gyro.y;
      _zGyro = event->gyro.z;
      // debug("Gyro read: ");
      // debug(_xGyro, 4);
      // debug(", ");
      // debug(_yGyro, 4);
      // debug(", ");
      // debug(_zGyro, 4);
      // debugln();
      break;

    case SENSOR_TYPE_GRAVITY:
      _xGravity = event->acceleration.x;
      _yGravity = event->acceleration.y;
      _zGravity = event->acceleration.z;
      // debug("Gravity read: ");
      // debug(_xGravity, 4);
      // debug(", ");
      // debug(_yGravity, 4);
      // debug(", ");
      // debug(_zGravity, 4);
      // debugln();
      break;
  }
}
