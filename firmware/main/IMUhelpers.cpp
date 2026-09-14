#include "IMUhelpers.hpp"
#include "defines.hpp"
#include <cmath>

// ============================================
// BNO055IMUSensor IMPLEMENTATION
// ============================================

// Constructor
BNO055IMUSensor::BNO055IMUSensor() : _bno(55), _prevAccelMag(0), _currentAccelMag(0), _accelChange(0), _isMoving(false), _stableCounter(0), _currentOrientation(ORIENTATION_UNKNOWN), _motionThreshold(0.5), _stableThreshold(0.15), _stableCountRequired(10), _flatGravityMin(9.2), _flatGravityMax(10.5), _flatOtherAxisMax(3.5), _axisRemapConfig(0x06), _axisRemapSign(0x01), _xUp(0.0), _yUp(0.0), _zUp(1.0), _xUpStart(0.0), _yUpStart(0.0), _zUpStart(1.0), _prevMicros(0), _tumbleThreshold(0.707), _tumbleDetected(false), _tumbleReferenceSet(false), _firstUpdateAfterReset(false) {}

// ============================================
// CORE FUNCTIONS
// ============================================

auto BNO055IMUSensor::init() -> bool {
    // Initialize I2C and BNO055
    debug("Initializing BNO055... ");

    if (!_bno.begin()) {
        errorln("FAILED! Sensor not detected.");
        return false;  // Sensor not detected
    }

    debugln("BNO055 detected.");
    delay(100);

    // Apply custom axis remapping
    debug("Applying axis remapping... ");
    applyAxisRemap();
    debugln("done.");

    // Use external crystal for better accuracy
    _bno.setExtCrystalUse(true);

    delay(100);

    // Wait for sensor to produce sensible readings
    // This is especially important with ESP32 and I2C initialization
    debug("Waiting for stable readings... ");

    unsigned long startTime = millis();
    const unsigned long timeout = 5000;  // 5 second timeout
    bool sensibleReading = false;
    int attempts = 0;

    while (!sensibleReading && (millis() - startTime < timeout)) {
        // Read acceleration
        _accel = _bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
        _gravity = _bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
        float mag = sqrt(_gravity.x()*_gravity.x() + _gravity.y()*_gravity.y() + _gravity.z()*_gravity.z());

        // Check if reading is sensible (close to gravity, not zero or wildly off)
        // Valid range: 7-12 m/s² (allows for some movement during init)
        if (mag > 7.0 && mag < 12.0) {
            sensibleReading = true;
            _prevAccelMag = mag;
            _currentAccelMag = mag;
            debug("OK (");
            debug(mag, 2);
            debug(" m/s² after ");
            debug(attempts);
            debugln(" attempts)");
        } else {
            attempts++;
            if (attempts % 10 == 0) {
                debug(".");
            }
            delay(50);  // Wait a bit before next reading
        }
    }

    if (!sensibleReading) {
        debugln("\nFAILED! Timeout - sensor not producing valid readings.");
        debugln("Check connections and try again.");
        return false;  // Timeout - sensor not producing valid readings
    }

    // Do a few more updates to stabilize the baseline
    debug("Stabilizing baseline... ");
    for (int i = 0; i < 5; i++) {
        _accel = _bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
        _gyro = _bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
        _gravity = _bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
        _currentAccelMag = sqrt(_accel.x()*_accel.x() + _accel.y()*_accel.y() + _accel.z()*_accel.z());
        _prevAccelMag = _currentAccelMag;
        delay(20);
    }
    debugln("done.");
    debugln("✓ BNO055 initialization complete!");

    return true;  // Success
}

void BNO055IMUSensor::update() {
    // Calculate delta time for rotation matrices
    unsigned long currentMicros = micros();
    float deltaTime = (currentMicros - _prevMicros) * 1e-6;  // Convert to seconds
    _prevMicros = currentMicros;

    // Read sensor data
    _accel = _bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
    _gravity = _bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
    _gyro = _bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);

    // Calculate acceleration magnitude
    _currentAccelMag = sqrt(_accel.x()*_accel.x() + _accel.y()*_accel.y() + _accel.z()*_accel.z());
    float _gyroMag = sqrt(_gyro.x()*_gyro.x() + _gyro.y()*_gyro.y() + _gyro.z()*_gyro.z());

    // Motion detection logic
    if (_gyroMag > _motionThreshold) {
        // Significant change = motion detected
        _isMoving = true;
        _stableCounter = 0;
    }
    else if (_gyroMag < _stableThreshold) {
        // Very little change = potentially stable
        _stableCounter++;

        if (_isMoving && _stableCounter >= _stableCountRequired) {
            // Been stable long enough
            _isMoving = false;
        }
    }
    else {
        // In between - small movements
        if (_stableCounter > 0) {
            _stableCounter--;  // Slowly decay stable counter
        }
    }

    // Detect orientation
    _currentOrientation = detectOrientation();

    // Update up vector using rotation matrices (if reference is set)
    if (_tumbleReferenceSet) {
        // Skip first update after reset to avoid bad deltaTime
        if (_firstUpdateAfterReset) {
            _firstUpdateAfterReset = false;
            _prevMicros = currentMicros;  // Reset timing
        }
        else if (deltaTime > 0.0 && deltaTime < 1.0) {
            updateUpVector(deltaTime);

            // Check for tumble by comparing current up vector with initial reference
            float dotProduct = (_xUp * _xUpStart) + (_yUp * _yUpStart) + (_zUp * _zUpStart);

            // Clamp to valid range for acos
            dotProduct = constrain(dotProduct, -1.0, 1.0);

            // Check if tumbled beyond threshold
            if (dotProduct < _tumbleThreshold) {
                _tumbleDetected = true;
            }
        }
    }

    // Update previous magnitude for next iteration
    _prevAccelMag = _currentAccelMag;
}

// ============================================
// MOTION DETECTION
// ============================================

auto BNO055IMUSensor::moving() -> bool {
    return _isMoving;
}

auto BNO055IMUSensor::stable() -> bool {
    return !_isMoving && (_stableCounter >= _stableCountRequired);
}

// ============================================
// ORIENTATION DETECTION
// ============================================

auto BNO055IMUSensor::on_table() -> bool {
    // Check if orientation is one of the flat positions (not tilted or unknown)
    return (_currentOrientation != ORIENTATION_UNKNOWN &&
            _currentOrientation != ORIENTATION_TILTED);
}

auto BNO055IMUSensor::orientation() -> IMU_Orientation {
    return _currentOrientation;
}

auto BNO055IMUSensor::getOrientationString() -> const char * {
    switch (_currentOrientation) {
        case ORIENTATION_Z_UP:
            return "Z+ UP (Vertical - Normal)";
        case ORIENTATION_Z_DOWN:
            return "Z- UP (Vertical - Inverted)";
        case ORIENTATION_X_UP:
            return "X+ UP";
        case ORIENTATION_X_DOWN:
            return "X- UP";
        case ORIENTATION_Y_UP:
            return "Y+ UP";
        case ORIENTATION_Y_DOWN:
            return "Y- UP";
        case ORIENTATION_TILTED:
            return "TILTED (not aligned)";
        default:
            return "UNKNOWN";
    }
}

// ============================================
// GYROSCOPE
// ============================================

auto BNO055IMUSensor::gyroX() -> float {
    return _gyro.x();
}

auto BNO055IMUSensor::gyroY() -> float {
    return _gyro.y();
}

auto BNO055IMUSensor::gyroZ() -> float {
    return _gyro.z();
}

// ============================================
// ACCELEROMETER
// ============================================

auto BNO055IMUSensor::accelX() -> float {
    return _accel.x();
}

auto BNO055IMUSensor::accelY() -> float {
    return _accel.y();
}

auto BNO055IMUSensor::accelZ() -> float {
    return _accel.z();
}

auto BNO055IMUSensor::getAccelMagnitude() -> float {
    return _currentAccelMag;
}

auto BNO055IMUSensor::getAccelChange() -> float {
    return _accelChange;
}

// ============================================
// GRAVITY
// ===========================================

auto BNO055IMUSensor::getGravityX() -> float {
    return _gravity.x();
}

auto BNO055IMUSensor::getGravityY() -> float {
    return _gravity.y();
}

auto BNO055IMUSensor::getGravityZ() -> float {
    return _gravity.z();
}

// ============================================
// CALIBRATION
// ============================================

void BNO055IMUSensor::getCalibration(uint8_t* system, uint8_t* gyro, uint8_t* accel, uint8_t* mag) {
    _bno.getCalibration(system, gyro, accel, mag);
}

auto BNO055IMUSensor::isCalibrated() -> bool {
    uint8_t system = 0;
    uint8_t gyro = 0;
    uint8_t accel = 0;
    uint8_t mag = 0;
    _bno.getCalibration(&system, &gyro, &accel, &mag);
    return (system >= 2 && gyro >= 2 && accel >= 2 && mag >= 2);
}

// ============================================
// TUMBLE DETECTION
// ============================================

void BNO055IMUSensor::resetTumbleDetection() {
    // Get current acceleration (gravity) vector
    _gravity = _bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);

    // Calculate magnitude
    float mag = sqrt(_gravity.x()*_gravity.x() + _gravity.y()*_gravity.y() + _gravity.z()*_gravity.z());

    // Normalize to unit vector (invert because gravity points down, we want "up")
    // Avoid division by zero
    if (mag > 0.1) {
        _xUpStart = -_gravity.x() / mag;
        _yUpStart = -_gravity.y() / mag;
        _zUpStart = -_gravity.z() / mag;

        // Initialize current up vector to same as start
        _xUp = _xUpStart;
        _yUp = _yUpStart;
        _zUp = _zUpStart;

        // Reset timing
        _prevMicros = micros();

        // Clear detection flags
        _tumbleDetected = false;
        _tumbleReferenceSet = true;
        _firstUpdateAfterReset = true;  // Skip first update to avoid bad deltaTime
    }
}

auto BNO055IMUSensor::tumbled() -> bool {
    return _tumbleDetected;
}

auto BNO055IMUSensor::getTumbleAngle() -> float {
    if (!_tumbleReferenceSet) {
        return 0.0;  // No reference set
    }

    // Calculate dot product between current and initial up vectors
    float dotProduct = (_xUp * _xUpStart) + (_yUp * _yUpStart) + (_zUp * _zUpStart);

    // Clamp to valid range for acos
    dotProduct = constrain(dotProduct, -1.0, 1.0);

    // Convert to angle in degrees
    double angleRadians = acos(dotProduct);
    double angleDegrees = angleRadians * 57.2958;  // 180/PI

    return (float) angleDegrees;
}

// ============================================
// CONFIGURATION & TUNING
// ============================================

void BNO055IMUSensor::setMotionThreshold(float threshold) {
    _motionThreshold = threshold;
}

void BNO055IMUSensor::setStableThreshold(float threshold) {
    _stableThreshold = threshold;
}

void BNO055IMUSensor::setStableCount(int count) {
    _stableCountRequired = count;
}

void BNO055IMUSensor::setTumbleThreshold(float threshold) {
    _tumbleThreshold = threshold;  // Threshold is cosine of angle
                                   // Reset tumble detection when threshold changes
    _tumbleDetected = false;
}

void BNO055IMUSensor::setOrientationThresholds(float minGravity, float maxGravity, float maxOtherAxis) {
    _flatGravityMin = minGravity;
    _flatGravityMax = maxGravity;
    _flatOtherAxisMax = maxOtherAxis;
}

// ============================================
// AXIS REMAPPING
// ============================================

void BNO055IMUSensor::setAxisRemap(uint8_t config, uint8_t sign) {
    _axisRemapConfig = config;
    _axisRemapSign = sign;
    applyAxisRemap();
}

void BNO055IMUSensor::getAxisRemap(uint8_t* config, uint8_t* sign) {
    *config = readRegister(BNO055_AXIS_MAP_CONFIG_ADDR);
    *sign = readRegister(BNO055_AXIS_MAP_SIGN_ADDR);
}

// ============================================
// PRIVATE HELPER FUNCTIONS
// ============================================

auto BNO055IMUSensor::detectOrientation() -> IMU_Orientation {
    float x = _gravity.x();
    float y = _gravity.y();
    float z = _gravity.z();

    // Note: Accelerometer reads NEGATIVE when axis points UP (gravity pulls down)
    // and POSITIVE when axis points DOWN (accelerating toward ground)

    // Check which axis is aligned with gravity
    bool xDown = (abs(x) > _flatGravityMin && abs(x) < _flatGravityMax);
    bool yDown = (abs(y) > _flatGravityMin && abs(y) < _flatGravityMax);
    bool zDown = (abs(z) > _flatGravityMin && abs(z) < _flatGravityMax);

    // Z-axis aligned (physical X+ up = normal vertical)
    if (zDown && abs(x) < _flatOtherAxisMax && abs(y) < _flatOtherAxisMax) {
        return (z < 0) ? ORIENTATION_Z_UP : ORIENTATION_Z_DOWN;
    }

    // X-axis aligned (tilted toward physical Z direction)
    if (xDown && abs(y) < _flatOtherAxisMax && abs(z) < _flatOtherAxisMax) {
        return (x < 0) ? ORIENTATION_X_UP : ORIENTATION_X_DOWN;
    }

    // Y-axis aligned (tilted sideways)
    if (yDown && abs(x) < _flatOtherAxisMax && abs(z) < _flatOtherAxisMax) {
        return (y < 0) ? ORIENTATION_Y_UP : ORIENTATION_Y_DOWN;
    }

    // Not aligned with any axis
    return ORIENTATION_TILTED;
}

void BNO055IMUSensor::applyAxisRemap() {
    // Must be in CONFIG mode to change axis remap
    writeRegister(BNO055_OPR_MODE_ADDR, 0x00);
    delay(25);

    // Write custom axis remap configuration
    writeRegister(BNO055_AXIS_MAP_CONFIG_ADDR, _axisRemapConfig);
    delay(10);

    // Write custom axis sign configuration
    writeRegister(BNO055_AXIS_MAP_SIGN_ADDR, _axisRemapSign);
    delay(10);

    // Switch to NDOF mode (all sensors + fusion)
    writeRegister(BNO055_OPR_MODE_ADDR, 0x0C);
    delay(25);
}

auto BNO055IMUSensor::readRegister(uint8_t reg) -> uint8_t {
    uint8_t value = 0;
    Wire.beginTransmission(BNO055_ADDRESS_A);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(BNO055_ADDRESS_A, (uint8_t)1);
    value = Wire.read();
    return value;
}

void BNO055IMUSensor::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(BNO055_ADDRESS_A);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
    delay(2);
}

// ============================================
// TUMBLE DETECTION - ROTATION MATRIX UPDATE
// ============================================

void BNO055IMUSensor::updateUpVector(float deltaTime) {
    // BNO055 outputs gyroscope in DEGREES per second, not radians!
    // Convert to radians per second, then calculate rotation angles

    float xRot = _gyro.x() * DEG_TO_RAD * deltaTime;
    float yRot = _gyro.y() * DEG_TO_RAD * deltaTime;
    float zRot = _gyro.z() * DEG_TO_RAD * deltaTime;

    // Apply rotation matrices sequentially: X, then Y, then Z
    // This updates the current "up" vector based on the rotation

    // X-axis rotation matrix
    // Rotates around X-axis, affects Y and Z components
    float xUp = _xUp;
    float yUp = (_yUp * cos(xRot)) - (_zUp * sin(xRot));
    float zUp = (_yUp * sin(xRot)) + (_zUp * cos(xRot));

    _xUp = xUp;
    _yUp = yUp;
    _zUp = zUp;

    // Y-axis rotation matrix
    // Rotates around Y-axis, affects X and Z components
    xUp = (_xUp * cos(yRot)) + (_zUp * sin(yRot));
    yUp = _yUp;
    zUp = (-_xUp * sin(yRot)) + (_zUp * cos(yRot));

    _xUp = xUp;
    _yUp = yUp;
    _zUp = zUp;

    // Z-axis rotation matrix
    // Rotates around Z-axis, affects X and Y components
    xUp = (_xUp * cos(zRot)) - (_yUp * sin(zRot));
    yUp = (_xUp * sin(zRot)) + (_yUp * cos(zRot));
    zUp = _zUp;

    _xUp = xUp;
    _yUp = yUp;
    _zUp = zUp;

    // Calculate magnitude before normalization
    float magnitude = sqrt((_xUp * _xUp) + (_yUp * _yUp) + (_zUp * _zUp));

    // CRITICAL: Renormalize the up vector to prevent drift
    // Floating-point errors accumulate, causing magnitude to drift from 1.0
    // This would make dot product calculations unreliable
    if (magnitude > 0.01) {  // Avoid division by zero
        _xUp /= magnitude;
        _yUp /= magnitude;
        _zUp /= magnitude;
    }
}

// ============================================
// DEBUG FUNCTIONS
// ============================================

auto BNO055IMUSensor::getDebugDotProduct() -> float {
    if (!_tumbleReferenceSet) {
        return 1.0;  // No reference set, return 1.0
    }

    float dotProduct = (_xUp * _xUpStart) + (_yUp * _yUpStart) + (_zUp * _zUpStart);
    return constrain(dotProduct, -1.0, 1.0);
}

void BNO055IMUSensor::getDebugUpVector(float* x, float* y, float* z) {
    *x = _xUp;
    *y = _yUp;
    *z = _zUp;
}

void BNO055IMUSensor::getDebugUpStart(float* x, float* y, float* z) {
    *x = _xUpStart;
    *y = _yUpStart;
    *z = _zUpStart;
}

void BNO055IMUSensor::printDebugInfo() {
    info("UpStart:(");
    info(_xUpStart, 4);
    info(", ");
    info(_yUpStart, 4);
    info(", ");
    info(_zUpStart, 4);
    info(") | Up:(");
    info(_xUp, 4);
    info(", ");
    info(_yUp, 4);
    info(", ");
    info(_zUp, 4);
    info(") | Gyro:(");
    info(_gyro.x(), 4);
    info(", ");
    info(_gyro.y(), 4);
    info(", ");
    info(_gyro.z(), 4);
    info(") | Dot:");
    info(getDebugDotProduct(), 4);
    info(" | Angle:");
    info(getTumbleAngle(), 2);
    infoln("°");
}


// ============================================
// LSM6DS3TRCIMUSensor IMPLEMENTATION
// ============================================

// Constructor
LSM6DS3TRCIMUSensor::LSM6DS3TRCIMUSensor()
  : _lsm(),
    _prevAccelMag(0),
    _currentAccelMag(0),
    _accelChange(0),
    _isMoving(false),
    _stableCounter(0),
    _currentOrientation(ORIENTATION_UNKNOWN),
    _motionThreshold(1.0),
    _stableThreshold(0.5),
    _stableCountRequired(10),
    _flatGravityMin(9.2),
    _flatGravityMax(10.5),
    _flatOtherAxisMax(3.5),
    _axisRemapConfig(0x06),
    _axisRemapSign(0x01),
    _xUp(0.0), _yUp(0.0), _zUp(1.0),
    _xUpStart(0.0), _yUpStart(0.0), _zUpStart(1.0),
    _prevMicros(0),
    _tumbleThreshold(0.707),
    _tumbleDetected(false),
    _tumbleReferenceSet(false),
    _firstUpdateAfterReset(false) {}

// ============================================
// CORE FUNCTIONS
// ============================================

auto LSM6DS3TRCIMUSensor::init() -> bool {
    // Initialize I2C and LSM6DS3
    debugln("Initializing LSM6DS3... ");

    if (!_lsm.begin_I2C()) {
        errorln("FAILED! Sensor not detected.");
        return false;  // Sensor not detected
    }

    debugln("LSM6DS3 detected.");

    // Wait for sensor to produce sensible readings
    // This is especially important with ESP32 and I2C initialization
    debugln("Waiting for stable readings... ");

    _prevMicros = 0;
    unsigned long startTime = micros();
    constexpr unsigned long microsToSecond = 1000 * 1000;
    constexpr unsigned long timeout = 5 * microsToSecond;
    constexpr unsigned long delayBetweenAttempts = 50;
    int attempts = 1;

    while (true) {
        unsigned long now = micros();
        float deltaTime = static_cast<float>(now - _prevMicros) / static_cast<float>(microsToSecond);
        _prevMicros = now;

        if (now - startTime > timeout) {
            debugln("\nFAILED! Timeout - sensor not producing valid readings.");
            debugln("Check connections and try again.");
            return false;
        }

        // Get the sensor readings.
        sensors_event_t acceleration;
        sensors_event_t gyroscope;
        sensors_event_t temperature;
        _lsm.getEvent(&acceleration, &gyroscope, &temperature);

        // Calculate new values.
        float accel[3] = { acceleration.acceleration.x, acceleration.acceleration.y, acceleration.acceleration.z };
        for (uint8_t i = 0; i < 3; i++) {
            accel[i] = (accel[i] - _accelCalib[i]) * _accelCalib[i + 3];
        }

        float gyro[3] = { gyroscope.gyro.x, gyroscope.gyro.y, gyroscope.gyro.z };
        for (uint8_t i = 0; i < 3; i++) {
            gyro[i] = gyro[i] - _gyroCalib[i];
        }

        // Update Mahony.
        _mahonyUpdate(accel[0], accel[1], accel[2], gyro[0], gyro[1], gyro[2], deltaTime);

        // Calculate gravity vector.
        Quaternion mahonyQuaternion = quaternion_initialize(_quaternion[0], _quaternion[1], _quaternion[2], _quaternion[3]);
        vector_ijk gravity_z = quaternion_rotate_vector({ 0.0F, 0.0F, 9.81F }, mahonyQuaternion);
        vector_ijk gravity_y = quaternion_rotate_vector({ 0.0F, 9.81F, 0.0F }, mahonyQuaternion);
        vector_ijk gravity_x = quaternion_rotate_vector({ 9.81F, 0.0F, 0.0F }, mahonyQuaternion);
        vector_ijk gravity = {
            -gravity_x.c,
             gravity_y.c,
            -gravity_z.c
        };

        // Calculate linear acceleration.
        float linAccel[3] = {
            accel[0] - gravity.a,
            accel[1] - gravity.b,
            accel[2] - gravity.c
        };

        // Store the result.
        _accel = imu::Vector<3>(linAccel[0], linAccel[1], linAccel[2]);
        _gyro = imu::Vector<3>(gyro[0], gyro[1], gyro[2]);
        _gravity = imu::Vector<3>(gravity.a, gravity.b, gravity.c);

        // Is the reading stable?
        if (attempts > 10) {
            break;
        }

        // Wait a bit before next attempt.
        attempts++;
        delay(delayBetweenAttempts);
    }

    debugln("done.");
    debugln("✓ LSM6DS3 initialization complete!");
    return true;  // Success
}

void LSM6DS3TRCIMUSensor::update() {
    // Calculate delta time for rotation matrices
    unsigned long now = micros();
    float deltaTime = static_cast<float>(now - _prevMicros) * 1e-6F;  // Convert to seconds
    _prevMicros = now;

    // Get the sensor readings.
    sensors_event_t acceleration;
    sensors_event_t gyroscope;
    sensors_event_t temperature;
    _lsm.getEvent(&acceleration, &gyroscope, &temperature);

    // Calculate new values.
    float accel[3] = { acceleration.acceleration.x, acceleration.acceleration.y, acceleration.acceleration.z };
    for (uint8_t i = 0; i < 3; i++) {
        accel[i] = (accel[i] - _accelCalib[i]) * _accelCalib[i + 3];
    }

	// Swap X and Z.
	// Invert the Z-axis.
	float temp = accel[0];
	accel[0] = accel[2];
	accel[2] = -temp;

    float gyro[3] = { gyroscope.gyro.x, gyroscope.gyro.y, gyroscope.gyro.z };
    for (uint8_t i = 0; i < 3; i++) {
        gyro[i] = gyro[i] - _gyroCalib[i];
    }

    // Update Mahony.
    _mahonyUpdate(accel[0], accel[1], accel[2], gyro[0], gyro[1], gyro[2], deltaTime);

    // Calculate gravity vector.
    Quaternion mahonyQuaternion = quaternion_initialize(_quaternion[0], _quaternion[1], _quaternion[2], _quaternion[3]);
    vector_ijk gravity_z = quaternion_rotate_vector({ 0.0F, 0.0F, 9.81F }, mahonyQuaternion);
    vector_ijk gravity_y = quaternion_rotate_vector({ 0.0F, 9.81F, 0.0F }, mahonyQuaternion);
    vector_ijk gravity_x = quaternion_rotate_vector({ 9.81F, 0.0F, 0.0F }, mahonyQuaternion);
    vector_ijk gravity = {
        gravity_x.c,
        gravity_y.c,
        gravity_z.c
    };

    // Calculate linear acceleration.
    float linAccel[3] = {
        accel[0] - gravity.a,
        accel[1] - gravity.b,
        accel[2] - gravity.c
    };

    // Store the result.
    _accel = imu::Vector<3>(linAccel[0], linAccel[1], linAccel[2]);
    _gyro = imu::Vector<3>(gyro[0], gyro[1], gyro[2]);
    _gravity = imu::Vector<3>(gravity.a, gravity.b, gravity.c);

    // Calculate acceleration magnitude
    _currentAccelMag = sqrt(_accel.x()*_accel.x() + _accel.y()*_accel.y() + _accel.z()*_accel.z());
    float _gyroMag = sqrt(_gyro.x()*_gyro.x() + _gyro.y()*_gyro.y() + _gyro.z()*_gyro.z());

    // Motion detection logic
    if (_gyroMag > _motionThreshold) {
        // Significant change = motion detected
        _isMoving = true;
        _stableCounter = 0;
    }
    else if (_gyroMag < _stableThreshold) {
        // Very little change = potentially stable
        _stableCounter++;

        if (_isMoving && _stableCounter >= _stableCountRequired) {
            // Been stable long enough
            _isMoving = false;
        }
    }
    else {
        // In between - small movements
        if (_stableCounter > 0) {
            _stableCounter--;  // Slowly decay stable counter
        }
    }

    // Detect orientation
    static IMU_Orientation prevOrientation = IMU_Orientation::ORIENTATION_UNKNOWN;
    _currentOrientation = detectOrientation();

    if (_currentOrientation != prevOrientation) {
        prevOrientation = _currentOrientation;
        debugf("Orientation: %s\n", getOrientationString());
    }

    // Update up vector using rotation matrices (if reference is set)
    if (_tumbleReferenceSet) {
        // Skip first update after reset to avoid bad deltaTime
        if (_firstUpdateAfterReset) {
            _firstUpdateAfterReset = false;
            _prevMicros = now;  // Reset timing
        }
        else if (deltaTime > 0.0 && deltaTime < 1.0) {
            updateUpVector(deltaTime);

            // Check for tumble by comparing current up vector with initial reference
            float dotProduct = (_xUp * _xUpStart) + (_yUp * _yUpStart) + (_zUp * _zUpStart);

            // Clamp to valid range for acos
            dotProduct = constrain(dotProduct, -1.0, 1.0);

            // Check if tumbled beyond threshold
            if (dotProduct < _tumbleThreshold) {
                _tumbleDetected = true;
            }
        }
    }

    // Update previous magnitude for next iteration
    _prevAccelMag = _currentAccelMag;
}

// ============================================
// MOTION DETECTION
// ============================================

auto LSM6DS3TRCIMUSensor::moving() -> bool {
    return _isMoving;
}

auto LSM6DS3TRCIMUSensor::stable() -> bool {
    return !_isMoving && (_stableCounter >= _stableCountRequired);
}

// ============================================
// ORIENTATION DETECTION
// ============================================

auto LSM6DS3TRCIMUSensor::on_table() -> bool {
    // Check if orientation is one of the flat positions (not tilted or unknown)
    return (_currentOrientation != ORIENTATION_UNKNOWN &&
            _currentOrientation != ORIENTATION_TILTED);
}

auto LSM6DS3TRCIMUSensor::orientation() -> IMU_Orientation {
    return _currentOrientation;
}

auto LSM6DS3TRCIMUSensor::getOrientationString() -> const char * {
    switch (_currentOrientation) {
        case ORIENTATION_Z_UP:
            return "Z+ UP (Vertical - Normal)";
        case ORIENTATION_Z_DOWN:
            return "Z- UP (Vertical - Inverted)";
        case ORIENTATION_X_UP:
            return "X+ UP";
        case ORIENTATION_X_DOWN:
            return "X- UP";
        case ORIENTATION_Y_UP:
            return "Y+ UP";
        case ORIENTATION_Y_DOWN:
            return "Y- UP";
        case ORIENTATION_TILTED:
            return "TILTED (not aligned)";
        default:
            return "UNKNOWN";
    }
}

// ============================================
// GYROSCOPE
// ============================================

auto LSM6DS3TRCIMUSensor::gyroX() -> float {
    return _gyro.x();
}

auto LSM6DS3TRCIMUSensor::gyroY() -> float {
    return _gyro.y();
}

auto LSM6DS3TRCIMUSensor::gyroZ() -> float {
    return _gyro.z();
}

// ============================================
// ACCELEROMETER
// ============================================

auto LSM6DS3TRCIMUSensor::accelX() -> float {
    return _accel.x();
}

auto LSM6DS3TRCIMUSensor::accelY() -> float {
    return _accel.y();
}

auto LSM6DS3TRCIMUSensor::accelZ() -> float {
    return _accel.z();
}

auto LSM6DS3TRCIMUSensor::getAccelMagnitude() -> float {
    return _currentAccelMag;
}

auto LSM6DS3TRCIMUSensor::getAccelChange() -> float {
    return _accelChange;
}

// ============================================
// GRAVITY
// ===========================================

auto LSM6DS3TRCIMUSensor::getGravityX() -> float {
    return _gravity.x();
}

auto LSM6DS3TRCIMUSensor::getGravityY() -> float {
    return _gravity.y();
}

auto LSM6DS3TRCIMUSensor::getGravityZ() -> float {
    return _gravity.z();
}

// ============================================
// CALIBRATION
// ============================================

void LSM6DS3TRCIMUSensor::getCalibration(uint8_t* system, uint8_t* gyro, uint8_t* accel, uint8_t* mag) {}

auto LSM6DS3TRCIMUSensor::isCalibrated() -> bool {
    return true;
}

// ============================================
// TUMBLE DETECTION
// ============================================

void LSM6DS3TRCIMUSensor::resetTumbleDetection() {
    // Calculate magnitude
    float mag = sqrt(_gravity.x()*_gravity.x() + _gravity.y()*_gravity.y() + _gravity.z()*_gravity.z());

    // Normalize to unit vector (invert because gravity points down, we want "up")
    // Avoid division by zero
    if (mag > 0.1) {
        _xUpStart = -_gravity.x() / mag;
        _yUpStart = -_gravity.y() / mag;
        _zUpStart = -_gravity.z() / mag;

        // Initialize current up vector to same as start
        _xUp = _xUpStart;
        _yUp = _yUpStart;
        _zUp = _zUpStart;

        // Clear detection flags
        _tumbleDetected = false;
        _tumbleReferenceSet = true;
    }
}

auto LSM6DS3TRCIMUSensor::tumbled() -> bool {
    return _tumbleDetected;
}

auto LSM6DS3TRCIMUSensor::getTumbleAngle() -> float {
    if (!_tumbleReferenceSet) {
        return 0.0;  // No reference set
    }

    // Calculate dot product between current and initial up vectors
    float dotProduct = (_xUp * _xUpStart) + (_yUp * _yUpStart) + (_zUp * _zUpStart);

    // Clamp to valid range for acos
    dotProduct = constrain(dotProduct, -1.0, 1.0);

    // Convert to angle in degrees
    double angleRadians = acos(dotProduct);
    double angleDegrees = angleRadians * 57.2958;  // 180/PI

    return (float) angleDegrees;
}

// ============================================
// CONFIGURATION & TUNING
// ============================================

void LSM6DS3TRCIMUSensor::setMotionThreshold(float threshold) {
    _motionThreshold = threshold;
}

void LSM6DS3TRCIMUSensor::setStableThreshold(float threshold) {
    _stableThreshold = threshold;
}

void LSM6DS3TRCIMUSensor::setStableCount(int count) {
    _stableCountRequired = count;
}

void LSM6DS3TRCIMUSensor::setTumbleThreshold(float threshold) {
    _tumbleThreshold = threshold;  // Threshold is cosine of angle
                                   // Reset tumble detection when threshold changes
    _tumbleDetected = false;
}

void LSM6DS3TRCIMUSensor::setOrientationThresholds(float minGravity, float maxGravity, float maxOtherAxis) {
    _flatGravityMin = minGravity;
    _flatGravityMax = maxGravity;
    _flatOtherAxisMax = maxOtherAxis;
}

// ============================================
// AXIS REMAPPING
// ============================================

void LSM6DS3TRCIMUSensor::setAxisRemap(uint8_t config, uint8_t sign) {
    _axisRemapConfig = config;
    _axisRemapSign = sign;
    //applyAxisRemap();
}

void LSM6DS3TRCIMUSensor::getAxisRemap(uint8_t* config, uint8_t* sign) {
    //*config = readRegister(BNO055_AXIS_MAP_CONFIG_ADDR);
    //*sign = readRegister(BNO055_AXIS_MAP_SIGN_ADDR);
}

// ============================================
// PRIVATE HELPER FUNCTIONS
// ============================================

auto LSM6DS3TRCIMUSensor::detectOrientation() -> IMU_Orientation {
    float x = _gravity.x();
    float y = _gravity.y();
    float z = _gravity.z();

    // Note: Accelerometer reads NEGATIVE when axis points UP (gravity pulls down)
    // and POSITIVE when axis points DOWN (accelerating toward ground)

    // Check which axis is aligned with gravity
    bool xDown = (abs(x) > _flatGravityMin && abs(x) < _flatGravityMax);
    bool yDown = (abs(y) > _flatGravityMin && abs(y) < _flatGravityMax);
    bool zDown = (abs(z) > _flatGravityMin && abs(z) < _flatGravityMax);

    // Z-axis aligned (physical X+ up = normal vertical)
    if (zDown && abs(x) < _flatOtherAxisMax && abs(y) < _flatOtherAxisMax) {
        return (z < 0) ? ORIENTATION_Z_UP : ORIENTATION_Z_DOWN;
    }

    // X-axis aligned (tilted toward physical Z direction)
    if (xDown && abs(y) < _flatOtherAxisMax && abs(z) < _flatOtherAxisMax) {
        return (x < 0) ? ORIENTATION_X_UP : ORIENTATION_X_DOWN;
    }

    // Y-axis aligned (tilted sideways)
    if (yDown && abs(x) < _flatOtherAxisMax && abs(z) < _flatOtherAxisMax) {
        return (y < 0) ? ORIENTATION_Y_UP : ORIENTATION_Y_DOWN;
    }

    // Not aligned with any axis
    return ORIENTATION_TILTED;
}

// ============================================
// TUMBLE DETECTION - ROTATION MATRIX UPDATE
// ============================================

void LSM6DS3TRCIMUSensor::updateUpVector(float deltaTime) {
    float magnitude = sqrt(_gravity.x()*_gravity.x() + _gravity.y()*_gravity.y() + _gravity.z()*_gravity.z());

    if (magnitude > 0.01) {
        _xUp = -_gravity.x() / magnitude;
        _yUp = -_gravity.y() / magnitude;
        _zUp = -_gravity.z() / magnitude;
    }
}

// ============================================
// DEBUG FUNCTIONS
// ============================================

auto LSM6DS3TRCIMUSensor::getDebugDotProduct() -> float {
    if (!_tumbleReferenceSet) {
        return 1.0;  // No reference set, return 1.0
    }

    float dotProduct = (_xUp * _xUpStart) + (_yUp * _yUpStart) + (_zUp * _zUpStart);
    return constrain(dotProduct, -1.0, 1.0);
}

void LSM6DS3TRCIMUSensor::getDebugUpVector(float* x, float* y, float* z) {
    *x = _xUp;
    *y = _yUp;
    *z = _zUp;
}

void LSM6DS3TRCIMUSensor::getDebugUpStart(float* x, float* y, float* z) {
    *x = _xUpStart;
    *y = _yUpStart;
    *z = _zUpStart;
}

void LSM6DS3TRCIMUSensor::printDebugInfo() {
    info("UpStart:(");
    info(_xUpStart, 4);
    info(", ");
    info(_yUpStart, 4);
    info(", ");
    info(_zUpStart, 4);
    info(") | Up:(");
    info(_xUp, 4);
    info(", ");
    info(_yUp, 4);
    info(", ");
    info(_zUp, 4);
    info(") | Gyro:(");
    info(_gyro.x(), 4);
    info(", ");
    info(_gyro.y(), 4);
    info(", ");
    info(_gyro.z(), 4);
    info(") | Dot:");
    info(getDebugDotProduct(), 4);
    info(" | Angle:");
    info(getTumbleAngle(), 2);
    infoln("°");
}

void LSM6DS3TRCIMUSensor::_mahonyUpdate(float accelX, float accelY, float accelZ, float gyroX, float gyroY, float gyroZ, float deltaTime) {
    float recipNorm;
    float vx, vy, vz;
    float ex, ey, ez;  //error terms
    float qa, qb, qc;
    static float ix = 0.0, iy = 0.0, iz = 0.0;  //integral feedback terms
    float tmp;

    // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
    tmp = accelX * accelX + accelY * accelY + accelZ * accelZ;

    // ignore accelerometer if false (tested OK, SJR)
    if (tmp > 0.0F)
    {
        // Normalise accelerometer (assumed to measure the direction of gravity in body frame)
        recipNorm = 1.0F / sqrt(tmp);
        accelX *= recipNorm;
        accelY *= recipNorm;
        accelZ *= recipNorm;

        // Estimated direction of gravity in the body frame (factor of two divided out)
        vx = _quaternion[1] * _quaternion[3] - _quaternion[0] * _quaternion[2];
        vy = _quaternion[0] * _quaternion[1] + _quaternion[2] * _quaternion[3];
        vz = _quaternion[0] * _quaternion[0] - 0.5F + _quaternion[3] * _quaternion[3];

        // Error is cross product between estimated and measured direction of gravity in body frame
        // (half the actual magnitude)
        ex = (accelY * vz - accelZ * vy);
        ey = (accelZ * vx - accelX * vz);
        ez = (accelX * vy - accelY * vx);

        // Compute and apply to gyro term the integral feedback, if enabled
        if (_ki > 0.0F) {
            ix += _ki * ex * deltaTime;  // integral error scaled by Ki
            iy += _ki * ey * deltaTime;
            iz += _ki * ez * deltaTime;
            gyroX += ix;  // apply integral feedback
            gyroY += iy;
            gyroZ += iz;
        }

        // Apply proportional feedback to gyro term
        gyroX += _kp * ex;
        gyroY += _kp * ey;
        gyroZ += _kp * ez;
    }

    // Integrate rate of change of quaternion, given by gyro term
    // rate of change = current orientation quaternion (qmult) gyro rate

    deltaTime = 0.5F * deltaTime;
    gyroX *= deltaTime;   // pre-multiply common factors
    gyroY *= deltaTime;
    gyroZ *= deltaTime;
    qa = _quaternion[0];
    qb = _quaternion[1];
    qc = _quaternion[2];

    //add qmult*delta_t to current orientation
    _quaternion[0] += (-qb * gyroX - qc * gyroY - _quaternion[3] * gyroZ);
    _quaternion[1] += (qa * gyroX + qc * gyroZ - _quaternion[3] * gyroY);
    _quaternion[2] += (qa * gyroY - qb * gyroZ + _quaternion[3] * gyroX);
    _quaternion[3] += (qa * gyroZ + qb * gyroY - qc * gyroX);

    // Normalise quaternion
    recipNorm = 1.0F / sqrt(_quaternion[0] * _quaternion[0] + _quaternion[1] * _quaternion[1] + _quaternion[2] * _quaternion[2] + _quaternion[3] * _quaternion[3]);
    _quaternion[0] = _quaternion[0] * recipNorm;
    _quaternion[1] = _quaternion[1] * recipNorm;
    _quaternion[2] = _quaternion[2] * recipNorm;
    _quaternion[3] = _quaternion[3] * recipNorm;
}
