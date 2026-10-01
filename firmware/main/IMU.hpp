#ifndef IMU_HPP
#define IMU_HPP

#include <cstdint>
#include <cmath>

#include "Adafruit_BNO055.h"
#include "Adafruit_LSM6DS3.h"

/**
 * An enum that represents the chip of the IMU.
 */
enum class IMUChip : uint8_t {
	/**
	 * The chip is unknown.
	 */
	UNKNOWN,

	/**
	 * The chip is the BNO055.
	 */
	BNO055,

	/**
	 * The chip is the LSM6DS3.
	 */
	LSM6DS3
};

/**
 * An enum that represents the axes of the IMU.
 */
enum class IMUAxis : uint8_t {
	/**
	 * The axis is unknown.
	 */
	UNKNOWN,

	/**
	 * No axis.
	 */
	NONE,

	/**
	 * The X axis.
	 */
	X_AXIS,

	/**
	 * The Y axis.
	 */
	Y_AXIS,

	/**
	 * The Z axis.
	 */
	Z_AXIS
};

/**
 * An enum that represents several orientations of the IMU where a face of the die is pointing up.
 */
enum class IMUOrientation : uint8_t {
	/**
	 * The orientation is unknown.
	 */
	UNKNOWN,

	/**
	 * The die is tilted.
	 *
	 * No face of the die is pointing up.
	 */
	TILTED,

	/**
	 * The Z+ face of the die is pointing up.
	 */
	Z_POS,

	/**
	 * The Z- face of the die is pointing up.
	 */
	Z_NEG,

	/**
	 * The Y+ face of the die is pointing up.
	 */
	Y_POS,

	/**
	 * The Y- face of the die is pointing up.
	 */
	Y_NEG,

	/**
	 * The X+ face of the die is pointing up.
	 */
	X_POS,

	/**
	 * The X- face of the die is pointing up.
	 */
	X_NEG
};

class IMUClass {
public:
	/**
	 * Returns the IMU chip present.
	 *
	 * @return The IMU chip present.
	 */
	constexpr IMUChip getChip() const noexcept {
		return this->chip;
	}

	/**
	 * Returns the axis the die is currently aligned with.
	 *
	 * @return The axis the die is currently aligned with.
	 */
	constexpr IMUAxis getAxis() const noexcept {
		return this->axis;
	}

	/**
	 * Returns the current orientation of the die.
	 *
	 * @return The current orientation of the die.
	 */
	constexpr IMUOrientation getOrientation() const noexcept {
		return this->orientation;
	}

	void init();
	void update();
	inline bool moving() const __attribute__((always_inline)) { return this->_isMoving; }
	inline bool stable() const __attribute__((always_inline)) { return !this->_isMoving && (this->_stableCounter >= this->_stableCountRequired); }
	inline bool onTable() const __attribute__((always_inline)) { return this->orientation != IMUOrientation::UNKNOWN && this->orientation != IMUOrientation::TILTED; }
	inline float gyroX() const __attribute__((always_inline)) { return this->_gyroX; }
	inline float gyroY() const __attribute__((always_inline)) { return this->_gyroY; }
	inline float gyroZ() const __attribute__((always_inline)) { return this->_gyroZ; }
	inline float accelX() const __attribute__((always_inline)) { return this->_accelX; }
	inline float accelY() const __attribute__((always_inline)) { return this->_accelY; }
	inline float accelZ() const __attribute__((always_inline)) { return this->_accelZ; }
	inline float accelMagnitude() const __attribute__((always_inline)) { return this->_accelMag; }
	inline float gravityX() const __attribute__((always_inline)) { return this->_gravityX; }
	inline float gravityY() const __attribute__((always_inline)) { return this->_gravityY; }
	inline float gravityZ() const __attribute__((always_inline)) { return this->_gravityZ; }
	void resetTumbleDetection();
	inline bool tumbled() const __attribute__((always_inline)) { return this->_tumbled; }
	inline void setMotionThreshold(float motionThreshold) __attribute__((always_inline)) { this->_motionThreshold = motionThreshold; }
	inline void setStableThreshold(float stableThreshold) __attribute__((always_inline)) { this->_stableThreshold = stableThreshold; }
	inline void setStableCountRequired(int stableCountRequired) __attribute__((always_inline)) { this->_stableCountRequired = stableCountRequired; }
	inline void setTumbleThreshold(float tumbleThreshold) __attribute__((always_inline)) { this->_tumbleThreshold = tumbleThreshold; this->_tumbled = false; }
	inline void setOrientationThresholds(float flatGravityMin, float flatGravityMax, float flatOtherAxisMax) {
		this->_flatGravityMin = flatGravityMin;
		this->_flatGravityMax = flatGravityMax;
		this->_flatOtherAxisMax = flatOtherAxisMax;
	}

	IMUClass(const IMUClass &) = delete;
	IMUClass &operator=(const IMUClass &) = delete;

	static IMUClass &getSingleton();

private:
	IMUClass() = default;
	~IMUClass() = default;

	Adafruit_BNO055 _bno;
	Adafruit_LSM6DS3 _lsm;

	uint64_t _lastMicros = 0;
	float _accelX = NAN, _accelY = NAN, _accelZ = NAN, _accelMag = NAN;
	float _gyroX = NAN, _gyroY = NAN, _gyroZ = NAN;
	float _gravityX = NAN, _gravityY = NAN, _gravityZ = NAN;
	float _upX = NAN, _upY = NAN, _upZ = NAN;
	float _upStartX = NAN, _upStartY = NAN, _upStartZ = NAN;
	float _biasAccelX = 0.0f, _biasAccelY = 0.0f, _biasAccelZ = 0.0f;
	float _biasGyroX = 0.0f, _biasGyroY = 0.0f, _biasGyroZ = 0.0f;
	float _motionThreshold = 1.0f, _stableThreshold = 0.5f, _tumbleThreshold = 0.707f;
	float _flatGravityMin = 9.2f, _flatGravityMax = 10.5f, _flatOtherAxisMax = 3.5f;
	float _quaternion[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	IMUOrientation orientation = IMUOrientation::UNKNOWN;
	IMUAxis axis = IMUAxis::UNKNOWN;
	IMUChip chip = IMUChip::UNKNOWN;
	int _stableCounter = 0, _stableCountRequired = 10;
	bool _isMoving = false;
	bool _tumbled = false, _tumbleReferenceSet = false;

	void updateCalibration(sensors_event_t *accel, sensors_event_t *gyro);
	void updateMahony(sensors_event_t *accel, sensors_event_t *gyro, float deltaTime);
	void calculateGravity(float *x, float *y, float *z);
};

/**
 * Returns a string representation of the given IMU chip.
 *
 * @return A string representation of the given IMU chip.
 */
const char *toString(const IMUChip chip) noexcept;

/**
 * Returns a string representation of the given IMU axis.
 *
 * @return A string representation of the given IMU axis.
 */
const char *toString(const IMUAxis axis) noexcept;

/**
 * Returns a string representation of the given IMU orientation.
 *
 * @return A string representation of the given IMU orientation.
 */
const char *toString(const IMUOrientation orientation) noexcept;

#define IMU IMUClass::getSingleton()

#endif // IMU_HPP
