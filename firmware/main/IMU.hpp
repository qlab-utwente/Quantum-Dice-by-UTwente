#ifndef IMU_HPP
#define IMU_HPP

#include <cstdint>
#include <cmath>

#include "Adafruit_BNO055.h"
#include "Adafruit_LSM6DS3.h"

enum class IMUOrientation {
	UNKNOWN,
	TILTED,
	Z_POS,
	Z_NEG,
	Y_POS,
	Y_NEG,
	X_POS,
	X_NEG
};

class IMUClass {
public:
	void init();
	void update();
	inline bool moving() const __attribute__((always_inline)) { return this->_isMoving; }
	inline bool stable() const __attribute__((always_inline)) { return !this->_isMoving && (this->_stableCounter >= this->_stableCountRequired); }
	inline bool onTable() const __attribute__((always_inline)) { return this->_orientation != IMUOrientation::UNKNOWN && this->_orientation != IMUOrientation::TILTED; }
	inline IMUOrientation getOrientation() const __attribute__((always_inline)) { return this->_orientation; }
	const char *getOrientationString() const;
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
	void getCalibration(uint8_t *system, uint8_t *gyro, uint8_t *accel, uint8_t *mag);
	bool isCalibrated();
	void resetTumbleDetection();
	inline bool tumbled() const __attribute__((always_inline)) { return this->_tumbled; }
	float getTumbleAngle() const;
	inline void setMotionThreshold(float motionThreshold) __attribute__((always_inline)) { this->_motionThreshold = motionThreshold; }
	inline void setStableThreshold(float stableThreshold) __attribute__((always_inline)) { this->_stableThreshold = stableThreshold; }
	inline void setStableCountRequired(int stableCountRequired) __attribute__((always_inline)) { this->_stableCountRequired = stableCountRequired; }
	inline void setTumbleThreshold(float tumbleThreshold) __attribute__((always_inline)) { this->_tumbleThreshold = tumbleThreshold; this->_tumbled = false; }
	inline void setOrientationThresholds(float flatGravityMin, float flatGravityMax, float flatOtherAxisMax) {
		this->_flatGravityMin = flatGravityMin;
		this->_flatGravityMax = flatGravityMax;
		this->_flatOtherAxisMax = flatOtherAxisMax;
	}
	IMUOrientation detectOrientation() const;

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
	IMUOrientation _orientation = IMUOrientation::UNKNOWN;
	int _stableCounter = 0, _stableCountRequired = 10;
	bool _isMoving = false;
	bool _tumbled = false, _tumbleReferenceSet = false;
	bool _isBNO055 = false;

	void updateCalibration(sensors_event_t *accel, sensors_event_t *gyro);
	void updateMahony(sensors_event_t *accel, sensors_event_t *gyro, float deltaTime);
	void calculateGravity(float *x, float *y, float *z);
};

#define IMU IMUClass::getSingleton()

#endif // IMU_HPP
