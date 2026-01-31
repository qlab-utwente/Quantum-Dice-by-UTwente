/*
   IMUhelpers.h - Polymorphic IMU sensor interface

Architecture:
- IMUSensor: Abstract base class defining the interface for any IMU sensor
- BNO055IMUSensor: Concrete implementation for BNO055 sensor

Usage in main code:
IMUSensor *imuSensor;
imuSensor = new BNO055IMUSensor();
imuSensor->init();

Usage in StateMachine:
void StateMachine::setImuSensor(IMUSensor *imuSensor) {
_imuSensor = imuSensor;
}

void StateMachine::update() {
_imuSensor->update();
if (_imuSensor->moving()) {
// handle motion
}
}
*/

#ifndef IMUHELPERS_H_
#define IMUHELPERS_H_

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// ============================================
// ORIENTATION ENUMERATION
// ============================================

enum IMU_Orientation: uint8_t
{
    ORIENTATION_UNKNOWN,
    ORIENTATION_Z_UP,   // Normal vertical position
    ORIENTATION_Z_DOWN, // Upside down
    ORIENTATION_X_UP,   // Tilted
    ORIENTATION_X_DOWN, // Tilted opposite
    ORIENTATION_Y_UP,   // Tilted sideways
    ORIENTATION_Y_DOWN, // Tilted opposite sideways
    ORIENTATION_TILTED  // Not aligned with any axis
};

// ============================================
// ABSTRACT BASE CLASS: IMUSensor
// ============================================

class IMUSensor
{
    public:
        virtual ~IMUSensor() = default;

        // ============================================
        // CORE SENSOR INTERFACE
        // ============================================

        // Initialize sensor
        // Returns true if successful, false if sensor not detected
        virtual auto init() -> bool = 0;

        // Must be called regularly to update sensor state
        virtual void update() = 0;

        // ============================================
        // MOTION DETECTION
        // ============================================

        // Check if sensor is currently moving
        virtual auto moving() -> bool = 0;

        // Check if sensor is stable (not moving)
        virtual auto stable() -> bool = 0;

        // ============================================
        // ORIENTATION DETECTION
        // ============================================

        // Check if sensor is flat on one of its faces (on table)
        virtual auto on_table() -> bool = 0;

        // Get current orientation
        virtual auto orientation() -> IMU_Orientation = 0;

        // Get orientation as human-readable string
        virtual auto getOrientationString() -> String = 0;

        // ============================================
        // GYROSCOPE
        // ============================================

        // Get gyroscope readings in deg/s (as output by BNO055)
        // NOTE: These return raw sensor values in degrees per second
        // Conversion to rad/s happens internally in updateUpVector()
        virtual auto gyroX() -> float = 0;
        virtual auto gyroY() -> float = 0;
        virtual auto gyroZ() -> float = 0;

        // ============================================
        // ACCELEROMETER
        // ============================================

        // Get accelerometer readings in m/s²
        virtual auto accelX() -> float = 0;
        virtual auto accelY() -> float = 0;
        virtual auto accelZ() -> float = 0;

        // Get total acceleration magnitude
        virtual auto getAccelMagnitude() -> float = 0;

        // Get acceleration change since last update
        virtual auto getAccelChange() -> float = 0;

        // ===========================================
        // GRAVITY VECTOR
        // ===========================================

        virtual auto getGravityX() -> float = 0;
        virtual auto getGravityY() -> float = 0;
        virtual auto getGravityZ() -> float = 0;

        // ============================================
        // CALIBRATION
        // ============================================

        // Get calibration status (0-3 for each, 3 = fully calibrated)
        virtual void getCalibration(uint8_t *system, uint8_t *gyro, uint8_t *accel, uint8_t *mag) = 0;

        // Check if sensor is calibrated
        virtual auto isCalibrated() -> bool = 0;

        // ============================================
        // TUMBLE DETECTION
        // ============================================

        // Reset tumble detection - captures current "up" direction as reference
        // Uses gravity vector to determine initial orientation
        // Can be called with sensor in ANY orientation
        virtual void resetTumbleDetection() = 0;

        // Check if sensor has tumbled beyond threshold since last reset
        // Uses rotation matrices to track orientation change
        // Insensitive to rotation around the vertical axis (spinning on table)
        // Only detects actual rolling motion
        virtual auto tumbled() -> bool = 0;

        // Get the rotation angle in degrees since reference was set
        // Calculated from dot product between current and initial up vectors
        virtual auto getTumbleAngle() -> float = 0;

        // Set tumble detection threshold (cosine of angle, default: 0.707 = 45°)
        // Common values: 0.866 (30°), 0.707 (45°), 0.5 (60°), 0.0 (90°)
        // Threshold represents cos(angle), where angle is the rotation threshold
        virtual void setTumbleThreshold(float threshold) = 0;

        // ============================================
        // DEBUG FUNCTIONS
        // ============================================

        // Get the current dot product (for debugging)
        // Returns value between -1.0 and 1.0
        virtual auto getDebugDotProduct() -> float = 0;

        // Get the current "up" vector (for debugging)
        virtual void getDebugUpVector(float *x, float *y, float *z) = 0;

        // Get the reference "up" vector (for debugging)
        virtual void getDebugUpStart(float *x, float *y, float *z) = 0;

        // Print comprehensive debug info to Serial
        virtual void printDebugInfo() = 0;

        // ============================================
        // CONFIGURATION & TUNING
        // ============================================

        // Set motion detection threshold (default: 0.5 m/s²)
        virtual void setMotionThreshold(float threshold) = 0;

        // Set stability threshold (default: 0.15 m/s²)
        virtual void setStableThreshold(float threshold) = 0;

        // Set number of stable samples required (default: 5)
        virtual void setStableCount(int count) = 0;

        // Set orientation detection thresholds
        virtual void setOrientationThresholds(float minGravity, float maxGravity, float maxOtherAxis) = 0;

        // ============================================
        // AXIS REMAPPING (BNO055 specific, but in interface for flexibility)
        // ============================================

        // Set custom axis remapping
        virtual void setAxisRemap(uint8_t config, uint8_t sign) = 0;

        // Get current axis remap configuration
        virtual void getAxisRemap(uint8_t *config, uint8_t *sign) = 0;
};

// ============================================
// CONCRETE CLASS: BNO055IMUSensor
// ============================================

class BNO055IMUSensor : public IMUSensor
{
    public:
        // Constructor
        BNO055IMUSensor();

        // ============================================
        // IMPLEMENTATION OF IMUSensor INTERFACE
        // ============================================

        auto init() -> bool override;
        void update() override;

        auto moving() -> bool override;
        auto stable() -> bool override;

        auto on_table() -> bool override;
        auto orientation() -> IMU_Orientation override;
        auto getOrientationString() -> String override;

        auto gyroX() -> float override;
        auto gyroY() -> float override;
        auto gyroZ() -> float override;

        auto accelX() -> float override;
        auto accelY() -> float override;
        auto accelZ() -> float override;
        auto getAccelMagnitude() -> float override;
        auto getAccelChange() -> float override;

        auto getGravityX() -> float override;
        auto getGravityY() -> float override;
        auto getGravityZ() -> float override;

        void getCalibration(uint8_t *system, uint8_t *gyro, uint8_t *accel, uint8_t *mag) override;
        auto isCalibrated() -> bool override;

        void resetTumbleDetection() override;
        auto tumbled() -> bool override;
        auto getTumbleAngle() -> float override;
        void setTumbleThreshold(float threshold) override;

        auto getDebugDotProduct() -> float override;
        void getDebugUpVector(float *x, float *y, float *z) override;
        void getDebugUpStart(float *x, float *y, float *z) override;
        void printDebugInfo() override;

        void setMotionThreshold(float threshold) override;
        void setStableThreshold(float threshold) override;
        void setStableCount(int count) override;
        void setOrientationThresholds(float minGravity, float maxGravity, float maxOtherAxis) override;

        void setAxisRemap(uint8_t config, uint8_t sign) override;
        void getAxisRemap(uint8_t *config, uint8_t *sign) override;

    private:
        // BNO055 sensor object
        Adafruit_BNO055 _bno;

        // Sensor readings
        imu::Vector<3> _accel;
        imu::Vector<3> _gravity;
        imu::Vector<3> _gyro;

        // Motion detection state
        float _prevAccelMag;
        float _currentAccelMag;
        float _accelChange;
        bool _isMoving;
        int _stableCounter;

        // Orientation state
        IMU_Orientation _currentOrientation;

        // Thresholds (tunable)
        float _motionThreshold;
        float _stableThreshold;
        int _stableCountRequired;
        float _flatGravityMin;
        float _flatGravityMax;
        float _flatOtherAxisMax;

        // Axis remap configuration
        uint8_t _axisRemapConfig;
        uint8_t _axisRemapSign;

        // Tumble detection (rotation matrix-based)
        float _xUp; // Current "up" vector (updated by rotation matrices)
        float _yUp;
        float _zUp;
        float _xUpStart; // Initial reference "up" vector
        float _yUpStart;
        float _zUpStart;
        unsigned long _prevMicros; // For delta time calculation
        float _tumbleThreshold;    // Threshold as cosine of angle (default: 0.707 = 45°)
        bool _tumbleDetected;
        bool _tumbleReferenceSet;
        bool _firstUpdateAfterReset; // Flag to skip first update with bad deltaTime

        // BNO055 Register addresses
        static const uint8_t BNO055_OPR_MODE_ADDR = 0x3D;
        static const uint8_t BNO055_AXIS_MAP_CONFIG_ADDR = 0x41;
        static const uint8_t BNO055_AXIS_MAP_SIGN_ADDR = 0x42;

        // Internal helper functions
        auto detectOrientation() -> IMU_Orientation;
        void applyAxisRemap();
        auto readRegister(uint8_t reg) -> uint8_t;
        void writeRegister(uint8_t reg, uint8_t value);
        void updateUpVector(float deltaTime); // Apply rotation matrices
};

#endif /* IMUHELPERS_H_ */
