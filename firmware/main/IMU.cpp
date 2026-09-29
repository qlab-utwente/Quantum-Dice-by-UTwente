#include "IMU.hpp"
#include "defines.hpp"
#include "Wire.h"

#include "Quaternion.hpp"
#include "Vector3.hpp"

IMUClass &IMUClass::getSingleton() {
	static IMUClass instance;
	return instance;
}

void IMUClass::init() {
	// Try to start both, continue when one responds.
	while (true) {
		if (this->_bno.begin(OPERATION_MODE_ACCGYRO)) {
			this->_isBNO055 = true;
			this->_bno.setExtCrystalUse(true);
			break;
		}

		if (this->_lsm.begin_I2C()) {
			this->_isBNO055 = false;
			break;
		}
	}

	// Wait until sensible reading.
	while (true) {
		sensors_event_t accel, gyro, temp;

		// Read the data from the chips.
		if (this->_isBNO055) {
			this->_bno.getEvent(&accel, Adafruit_BNO055::VECTOR_ACCELEROMETER);
			this->_bno.getEvent(&gyro, Adafruit_BNO055::VECTOR_GYROSCOPE);
		} else {
			this->_lsm.getEvent(&accel, &gyro, &temp);
		}

		// Swap X and Z axis.
		float _temp = accel.acceleration.x;
		accel.acceleration.x = accel.acceleration.z;
		accel.acceleration.z = _temp;

		_temp = gyro.gyro.x;
		gyro.gyro.x = gyro.gyro.z;
		gyro.gyro.z = _temp;

		// Are the readings sensible?
		float accMag = sqrtf(accel.acceleration.x * accel.acceleration.x + accel.acceleration.y * accel.acceleration.y + accel.acceleration.z * accel.acceleration.z);
		float gyroMag = sqrtf(gyro.gyro.x * gyro.gyro.x + gyro.gyro.y * gyro.gyro.y + gyro.gyro.z * gyro.gyro.z);

		if (accMag > 9.2F && accMag < 10.5F && gyroMag < 0.25F) {
			break;
		}
	}
}

void IMUClass::update() {
	static uint64_t last = micros();
	uint64_t now = micros();
	float deltaTime = static_cast<float>(now - last) * 1e-6f;
	last = now;
	sensors_event_t accel, gyro, temp;

	// Read the data from the chips.
		if (this->_isBNO055) {
			this->_bno.getEvent(&accel, Adafruit_BNO055::VECTOR_ACCELEROMETER);
			this->_bno.getEvent(&gyro, Adafruit_BNO055::VECTOR_GYROSCOPE);
		} else {
			this->_lsm.getEvent(&accel, &gyro, &temp);
		}

		// Swap X and Z axis.
		float _temp = accel.acceleration.x;
		accel.acceleration.x = -accel.acceleration.z;
		accel.acceleration.z = -_temp;

		_temp = gyro.gyro.x;
		gyro.gyro.x = -gyro.gyro.z;
		gyro.gyro.z = -_temp;

	// Update the calibration values with this data.
	this->updateCalibration(&accel, &gyro);

	// Calibrate the data, remove the bias.
	accel.acceleration.x -= this->_biasAccelX;
	accel.acceleration.y -= this->_biasAccelY;
	accel.acceleration.z -= this->_biasAccelZ;
	gyro.gyro.x -= this->_biasGyroX;
	gyro.gyro.y -= this->_biasGyroY;
	gyro.gyro.z -= this->_biasGyroZ;

	// Update the mahony fusion.
	this->updateMahony(&accel, &gyro, deltaTime);

	// Calculate the gravity vector.
	float gravityX = NAN, gravityY = NAN, gravityZ = NAN;
	this->calculateGravity(&gravityX, &gravityY, &gravityZ);

	// Calculate linear acceleration.
	float linAccelX = accel.acceleration.x - gravityX;
	float linAccelY = accel.acceleration.y - gravityY;
	float linAccelZ = accel.acceleration.z - gravityZ;
	float linAccelMag = sqrtf(linAccelX * linAccelX + linAccelY * linAccelY + linAccelZ * linAccelZ);

	// Update variables.
	this->_accelX = linAccelX;
	this->_accelY = linAccelY;
	this->_accelZ = linAccelZ;
	this->_accelMag = linAccelMag;
	this->_gyroX = gyro.gyro.x;
	this->_gyroY = gyro.gyro.y;
	this->_gyroZ = gyro.gyro.z;
	this->_gravityX = gravityX;
	this->_gravityY = gravityY;
	this->_gravityZ = gravityZ;

	// Calculate gyroscope magnitude.
	float gyroMag = sqrtf(this->_gyroX * this->_gyroX + this->_gyroY * this->_gyroY + this->_gyroZ * this->_gyroZ);

	// Motion detection logic.
	if (gyroMag > this->_motionThreshold) {
		if (!this->_isMoving) {
			debugln("NOW MOVING");
		}
		// Significant motion.
		this->_isMoving = true;
		this->_stableCounter = 0;
	} else if (gyroMag < this->_stableThreshold) {
		// No motion.
		this->_stableCounter++;

		if (this->_isMoving && this->_stableCounter >= this->_stableCountRequired) {
			debugln("NO LONGER MOVING");
			this->_isMoving = false;
		}
	} else {
		// Little motion.
		if (this->_stableCounter > 0) {
			this->_stableCounter--;
		}
	}

	// Update axis.
	static IMUAxis lastAxis = IMUAxis::UNKNOWN;
	if (abs(this->_gravityX) > this->_flatGravityMin && abs(this->_gravityX) < this->_flatGravityMax && abs(this->_gravityY) < this->_flatOtherAxisMax && abs(this->_gravityZ) < this->_flatOtherAxisMax) {
		this->axis = IMUAxis::X_AXIS;
	} else if (abs(this->_gravityY) > this->_flatGravityMin && abs(this->_gravityY) < this->_flatGravityMax && abs(this->_gravityX) < this->_flatOtherAxisMax && abs(this->_gravityZ) < this->_flatOtherAxisMax) {
		this->axis = IMUAxis::Y_AXIS;
	} else if (abs(this->_gravityZ) > this->_flatGravityMin && abs(this->_gravityZ) < this->_flatGravityMax && abs(this->_gravityX) < this->_flatOtherAxisMax && abs(this->_gravityY) < this->_flatOtherAxisMax) {
		this->axis = IMUAxis::Z_AXIS;
	} else {
		this->axis = IMUAxis::NONE;
	}
	if (this->axis != lastAxis) {
		lastAxis = this->axis;
		debugf("IMU New Axis: %s\n", toString(lastAxis));
	}

	// Update orientation.
	static IMUOrientation lastOrientation = IMUOrientation::UNKNOWN;
	switch (this->axis) {
		case IMUAxis::X_AXIS:
			this->orientation = (this->_gravityX < 0.0F) ? IMUOrientation::X_POS : IMUOrientation::X_NEG;
			break;

		case IMUAxis::Y_AXIS:
			this->orientation = (this->_gravityY < 0.0F) ? IMUOrientation::Y_POS : IMUOrientation::Y_NEG;
			break;

		case IMUAxis::Z_AXIS:
			this->orientation = (this->_gravityZ < 0.0F) ? IMUOrientation::Z_POS : IMUOrientation::Z_NEG;
			break;

		case IMUAxis::NONE:
			this->orientation = IMUOrientation::TILTED;
			break;

		default:
			this->orientation = IMUOrientation::UNKNOWN;
			break;
	}
	if (this->orientation != lastOrientation) {
		lastOrientation = this->orientation;
		debugf("IMU New Orientation: %s\n", toString(lastOrientation));
	}

	// Update up vector.
	float gravityMag = sqrtf(this->_gravityX * this->_gravityX + this->_gravityY * this->_gravityY + this->_gravityZ * this->_gravityZ);
	if (gravityMag > 0.0f) {
		this->_upX = -(this->_gravityX / gravityMag);
		this->_upY = -(this->_gravityY / gravityMag);
		this->_upZ = -(this->_gravityZ / gravityMag);
	} else {
		this->_upX = NAN;
		this->_upY = NAN;
		this->_upZ = NAN;
	}

	// Tumble detection logic.
	if (this->_tumbleReferenceSet) {
		// Calculate dot product between current and initial up vectors
		float upMag = sqrtf(this->_upX * this->_upX + this->_upY * this->_upY + this->_upZ * this->_upZ);
		float upStartMag = sqrtf(this->_upStartX * this->_upStartX + this->_upStartY * this->_upStartY + this->_upStartZ * this->_upStartZ);
		float dotProduct = (this->_upX * this->_upStartX + this->_upY * this->_upStartY + this->_upZ * this->_upStartZ) / (upMag * upStartMag);

		// Check if tumbled beyond threshold.
		if (dotProduct < this->_tumbleThreshold) {
			this->_tumbled = true;
		}
	}
}

void IMUClass::resetTumbleDetection() {
	float _gravityMag = sqrtf(this->_gravityX * this->_gravityX + this->_gravityY * this->_gravityY + this->_gravityZ * this->_gravityZ);

	if (_gravityMag > 0.1f) {
		this->_upStartX = -(this->_gravityX / _gravityMag);
		this->_upStartY = -(this->_gravityY / _gravityMag);
		this->_upStartZ = -(this->_gravityZ / _gravityMag);

		// Initialize current up vector to same as start
		this->_upX = this->_upStartX;
		this->_upY = this->_upStartY;
		this->_upZ = this->_upStartZ;

		// Clear detection flags
		this->_tumbled = false;
		this->_tumbleReferenceSet = true;
    } else {
		this->_upStartX = NAN;
		this->_upStartY = NAN;
		this->_upStartZ = NAN;
		this->_upX = NAN;
		this->_upY = NAN;
		this->_upZ = NAN;
		this->_tumbled = false;
		this->_tumbleReferenceSet = false;
	}
}

void IMUClass::updateCalibration(sensors_event_t *accel, sensors_event_t *gyro) {
	constexpr size_t SAMPLE_COUNT = 21;
	static Vector3 gyroSamples[SAMPLE_COUNT];
	static size_t writeIndex = 0;
	static bool filledOnce = false;

	//
	gyroSamples[writeIndex] = Vector3(gyro->gyro.x, gyro->gyro.y, gyro->gyro.z);
	writeIndex++;
	if (writeIndex >= SAMPLE_COUNT) {
		writeIndex = 0;
		filledOnce = true;
	}

	//
	if (filledOnce) {
		// Calculate average.
		Vector3 average;
		for (size_t index = 0; index < SAMPLE_COUNT; index++) {
			average += gyroSamples[index];
		}
		average *=(1.0F / static_cast<float>(SAMPLE_COUNT));

		// Calculate variance.
		Vector3 variance;
		for (size_t index = 0; index < SAMPLE_COUNT; index++) {
			Vector3 difference = average - gyroSamples[index];
			variance += Vector3(
				difference.x * difference.x,
				difference.y * difference.y,
				difference.z * difference.z
			);
		}
		variance *= (1.0F / static_cast<float>(SAMPLE_COUNT - 1));
		float combined_variance = variance.magnitude();

		//
		if (combined_variance < 1E-5F) {
			_biasGyroX = average.x;
			_biasGyroY = average.y;
			_biasGyroZ = average.z;
		}
	}
}

void IMUClass::updateMahony(sensors_event_t *accel, sensors_event_t *gyro, float deltaTime) {
	constexpr float _kp = 15.0f, _ki = 0.0f;
	float recipNorm;
    float vx, vy, vz;
    float ex, ey, ez;  //error terms
    float qa, qb, qc;
    static float ix = 0.0, iy = 0.0, iz = 0.0;  //integral feedback terms
    float tmp;
	float accelX = accel->acceleration.x, accelY = accel->acceleration.y, accelZ = accel->acceleration.z;
	float gyroX = gyro->gyro.x, gyroY = gyro->gyro.y, gyroZ = gyro->gyro.z;

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
    recipNorm = 1.0F / sqrtf(_quaternion[0] * _quaternion[0] + _quaternion[1] * _quaternion[1] + _quaternion[2] * _quaternion[2] + _quaternion[3] * _quaternion[3]);
    _quaternion[0] = _quaternion[0] * recipNorm;
    _quaternion[1] = _quaternion[1] * recipNorm;
    _quaternion[2] = _quaternion[2] * recipNorm;
    _quaternion[3] = _quaternion[3] * recipNorm;
}

void IMUClass::calculateGravity(float *x, float *y, float *z) {
	constexpr float GRAVITY = 9.81F;

	Quaternion mahonyQuaternion = Quaternion(this->_quaternion[0], this->_quaternion[1], this->_quaternion[2], this->_quaternion[3]);
	Vector3 gravityZ = mahonyQuaternion.rotateVector(Vector3(0.0F, 0.0F, GRAVITY));
	Vector3 gravityY = mahonyQuaternion.rotateVector(Vector3(0.0F, GRAVITY, 0.0F));
	Vector3 gravityX = mahonyQuaternion.rotateVector(Vector3(GRAVITY, 0.0F, 0.0F));

	*x = gravityX.z;
	*y = gravityY.z;
	*z = gravityZ.z;
}

const char *toString(const IMUAxis axis) noexcept {
	switch (axis) {
		case IMUAxis::UNKNOWN: return "UNKNOWN";
		case IMUAxis::NONE: return "NONE";
		case IMUAxis::X_AXIS: return "X-AXIS";
		case IMUAxis::Y_AXIS: return "Y-AXIS";
		case IMUAxis::Z_AXIS: return "Z-AXIS";
	}
}

const char *toString(const IMUOrientation orientation) noexcept {
	switch (orientation) {
		case IMUOrientation::UNKNOWN: return "UNKNOWN";
		case IMUOrientation::TILTED: return "TILTED";
		case IMUOrientation::Z_POS: return "Z+ UP";
		case IMUOrientation::Z_NEG: return "Z- UP";
		case IMUOrientation::Y_POS: return "Y+ UP";
		case IMUOrientation::Y_NEG: return "Y- UP";
		case IMUOrientation::X_POS: return "X+ UP";
		case IMUOrientation::X_NEG: return "X- UP";
	}
}
