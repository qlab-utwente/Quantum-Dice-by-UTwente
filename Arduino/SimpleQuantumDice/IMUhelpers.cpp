#include "Adafruit_Sensor.h"
#include "Arduino.h"
#include "IMUhelpers.h"

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

  // Serial.print("Rotation ");
  // Serial.print(deltaTime, 6);
  // Serial.print(", ");
  // Serial.print(xRot);
  // Serial.print(", ");
  // Serial.print(yRot);
  // Serial.print(", ");
  // Serial.println(zRot);

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

  // Serial.print("Up (");
  // Serial.print(_xUp);
  // Serial.print(", ");
  // Serial.print(_yUp);
  // Serial.print(", ");
  // Serial.print(_zUp);
  // Serial.println(")");
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
}

bool IMUSensor::tumbled(float minRotation) {
  // Serial.print("Start Up (");
  // Serial.print(_xUpStart, 2);
  // Serial.print(", ");
  // Serial.print(_yUpStart, 2);
  // Serial.print(", ");
  // Serial.print(_zUpStart, 2);
  // Serial.print(", ");
  //Serial.println(")");


  //Serial.print("Up (");
  // Serial.print(_xUp, 2);
  // Serial.print(", ");
  // Serial.print(_yUp, 2);
  // Serial.print(", ");
  // Serial.print(_zUp, 2);
  // Serial.print(", ");
  //Serial.println(")");
  double dotProduct = _xUp * _xUpStart + _yUp * _yUpStart + _zUp * _zUpStart;

  // Clamp dot product to valid range for acos [-1, 1]
  dotProduct = constrain(dotProduct, -1.0, 1.0);
  double rotation = acos(dotProduct) / TWOPI;  //inverse cos of dot product between vectors, normalised (2PI=1.0). Max rotation: 0.5
  // Serial.print(dotProduct, 6);
  // Serial.print(", ");
  // Serial.print(rotation, 6);
  // Serial.println();

    if (abs(rotation) > (double)minRotation) {
    return true;
  }

  // unsigned long currentMillis = millis();

  // _xRotationMagnitude += _xGyro / TWOPI * (currentMillis - _prevMillis) / 1000.0;
  // _yRotationMagnitude += _yGyro / TWOPI * (currentMillis - _prevMillis) / 1000.0;
  // _zRotationMagnitude += _zGyro / TWOPI * (currentMillis - _prevMillis) / 1000.0;

  // if (abs(_xRotationMagnitude) > minRotation || abs(_yRotationMagnitude) > minRotation || abs(_zRotationMagnitude) > minRotation)
  // {
  //   Serial.println("set Rotation magnitude to zero");
  //   _xRotationMagnitude = 0.0;
  //   _yRotationMagnitude = 0.0;
  //   _zRotationMagnitude = 0.0;
  //   return true;
  // }

  // _prevMillis = currentMillis;
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
  EEPROM.begin(512);  // Initialize EEPROM for ESP32
  while (!_accGyro.begin()) {
    Serial.println("BNO device not detected at default I2C address");
    delay(100);
  }
  Serial.println("BNO device found!");

  // Try to restore calibration data from EEPROM
  restoreCalibrationData();

  // Add sensor config (update frequency and range)
  // Set external crystal use (must be done after loading calibration)
  _accGyro.setExtCrystalUse(true);

  // Wait for valid gravity data before calling reset
  Serial.println("Waiting for valid gravity data...");
  int attempts = 0;
  float gravityMagnitude = 0.0;

  do {
    delay(100);
    update();  // Get sensor reading
    attempts++;

    gravityMagnitude = sqrt(_xGravity * _xGravity + _yGravity * _yGravity + _zGravity * _zGravity);

    Serial.print("Attempt ");
    Serial.print(attempts);
    Serial.print(" - Gravity: (");
    Serial.print(_xGravity, 2);
    Serial.print(", ");
    Serial.print(_yGravity, 2);
    Serial.print(", ");
    Serial.print(_zGravity, 2);
    Serial.print(") Magnitude: ");
    Serial.println(gravityMagnitude, 2);

  } while (gravityMagnitude < 8.0 && attempts < 100);  // Wait for reasonable gravity magnitude (~9.8 m/s²)

  if (gravityMagnitude >= 8.0) {
    reset();  // Now we have valid gravity data
    Serial.println("Up vector initialized successfully");
  } else {
    Serial.println("Warning: Failed to get valid gravity data after 100 attempts!");
    // Set a default up vector as fallback
    _xUp = 0.0;
    _yUp = 0.0;
    _zUp = 1.0;
    _xUpStart = 0.0;
    _yUpStart = 0.0;
    _zUpStart = 1.0;
  }

  Serial.println("IMU initialization complete");
  //add sensor config (update frequency and range);
}

void BNO055IMUSensor::restoreCalibrationData() {
  int eeAddress = 0;
  long bnoID;
  bool foundCalib = false;

  // Get stored sensor ID from EEPROM
  EEPROM.get(eeAddress, bnoID);

  // Get current sensor info
  sensor_t sensor;
  AccGyro.getSensor(&sensor);

  Serial.print("Current sensor ID: ");
  Serial.println(sensor.sensor_id);
  Serial.print("EEPROM stored ID: ");
  Serial.println(bnoID);

  // Check if we have calibration data for this sensor
  if (bnoID != sensor.sensor_id) {
    Serial.println("No calibration data found in EEPROM for this sensor");
    Serial.println("Run calibration sketch first to store calibration data");
  } else {
    Serial.println("Found calibration data in EEPROM");

    // Read calibration data
    eeAddress += sizeof(long);
    adafruit_bno055_offsets_t calibrationData;
    EEPROM.get(eeAddress, calibrationData);

    // Display what we're loading
    Serial.println("Loading calibration offsets:");
    displaySensorOffsets(calibrationData);

    // Apply calibration data to sensor
    AccGyro.setSensorOffsets(calibrationData);

    Serial.println("Calibration data restored successfully");
    foundCalib = true;
  }

  // Optional: Display current calibration status
  delay(100);  // Give sensor time to settle
  displayCalStatus();
}

void BNO055IMUSensor::displaySensorOffsets(const adafruit_bno055_offsets_t &calibData) {
  Serial.print("Accel: ");
  Serial.print(calibData.accel_offset_x);
  Serial.print(" ");
  Serial.print(calibData.accel_offset_y);
  Serial.print(" ");
  Serial.print(calibData.accel_offset_z);
  Serial.print(" ");

  Serial.print(" | Gyro: ");
  Serial.print(calibData.gyro_offset_x);
  Serial.print(" ");
  Serial.print(calibData.gyro_offset_y);
  Serial.print(" ");
  Serial.print(calibData.gyro_offset_z);
  Serial.print(" ");

  Serial.print(" | Mag: ");
  Serial.print(calibData.mag_offset_x);
  Serial.print(" ");
  Serial.print(calibData.mag_offset_y);
  Serial.print(" ");
  Serial.print(calibData.mag_offset_z);
  Serial.print(" ");

  Serial.print(" | Radii: A=");
  Serial.print(calibData.accel_radius);
  Serial.print(" M=");
  Serial.println(calibData.mag_radius);
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
    Serial.print(" [!] System not calibrated - data should be ignored");
  }
  Serial.println();
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
      break;

    case SENSOR_TYPE_GYROSCOPE:
      _xGyro = event->gyro.x;
      _yGyro = event->gyro.y;
      _zGyro = event->gyro.z;
      // Serial.print("Gyro read: ");
      // Serial.print(_xGyro, 4);
      // Serial.print(", ");
      // Serial.print(_yGyro, 4);
      // Serial.print(", ");
      // Serial.print(_zGyro, 4);
      // Serial.println();
      break;

    case SENSOR_TYPE_GRAVITY:
      _xGravity = event->acceleration.x;
      _yGravity = event->acceleration.y;
      _zGravity = event->acceleration.z;
      // Serial.print("Gravity read: ");
      // Serial.print(_xGravity, 4);
      // Serial.print(", ");
      // Serial.print(_yGravity, 4);
      // Serial.print(", ");
      // Serial.print(_zGravity, 4);
      // Serial.println();
      break;
  }
}

