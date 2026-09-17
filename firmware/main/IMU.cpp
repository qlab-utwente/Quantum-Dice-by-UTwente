#include "IMU.hpp"
#include "utility/imumaths.h"
#include "sensor_processing_lib.h"
#include "defines.hpp"
#include "Wire.h"

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
			this->applyAxisRemap();
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

			// Swap X and Z axis.
			// Invert the Z axis.
			float temp = accel.acceleration.x;
			accel.acceleration.x = accel.acceleration.z;
			accel.acceleration.z = -temp;

			temp = gyro.gyro.x;
			gyro.gyro.x = gyro.gyro.z;
			gyro.gyro.z = -temp;
		}

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

		// Swap X and Z axis.
		// Invert the Z axis.
		float temp = accel.acceleration.x;
		accel.acceleration.x = accel.acceleration.z;
		accel.acceleration.z = -temp;

		temp = gyro.gyro.x;
		gyro.gyro.x = gyro.gyro.z;
		gyro.gyro.z = -temp;
	}

	// Update the calibration values with this data.
	this->updateCalibration(&accel, &gyro);

	// Calibrate the data, remove the bias.
	accel.acceleration.x -= this->_biasAccelX;
	accel.acceleration.y -= this->_biasAccelY;
	accel.acceleration.z -= this->_biasAccelZ;
	gyro.gyro.x -= this->_biasGyroX;
	gyro.gyro.y -= this->_biasGyroY;
	gyro.gyro.z -= this->_biasGyroZ;
	debugf(
		"ACC: (%.2f, %.2f, %.2f)\n",
		accel.acceleration.x,
		accel.acceleration.y,
		accel.acceleration.z
	);
	debugf(
		"GYRO: (%.2f, %.2f, %.2f)\n",
		gyro.gyro.x,
		gyro.gyro.y,
		gyro.gyro.z
	);

	// Update the mahony fusion.
	this->updateMahony(&accel, &gyro, deltaTime);

	// Calculate the gravity vector.
	float gravityX = NAN, gravityY = NAN, gravityZ = NAN;
	this->calculateGravity(&gravityX, &gravityY, &gravityZ);
	debugf(
		"GRAVITY: (%.2f, %.2f, %.2f)\n",
		gravityX,
		gravityY,
		gravityZ
	);

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

	// Update orientation.
	static IMUOrientation lastOrientation = IMUOrientation::UNKNOWN;
	this->_orientation = this->detectOrientation();
	if (this->_orientation != lastOrientation) {
		lastOrientation = this->_orientation;
		debugf("NEW ORIENTATION: %s\n", this->getOrientationString());
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

const char *IMUClass::getOrientationString() const {
	switch (this->_orientation) {
		case IMUOrientation::TILTED: return "TILTED (not aligned)";
		case IMUOrientation::Z_POS: return "Z+ UP (Vertical - Normal)";
		case IMUOrientation::Z_NEG: return "Z- UP (Vertical - Inverted)";
		case IMUOrientation::Y_POS: return "Y+ UP";
		case IMUOrientation::Y_NEG: return "Y- UP";
		case IMUOrientation::X_POS: return "X+ UP";
		case IMUOrientation::X_NEG: return "X- UP";
		default: return "UNKNOWN";
	}
}

void IMUClass::getCalibration(uint8_t *system, uint8_t *gyro, uint8_t *accel, uint8_t *mag) {
	if (this->_isBNO055) {
		this->_bno.getCalibration(system, gyro, accel, mag);
	}
}

bool IMUClass::isCalibrated() {
	if (this->_isBNO055) {
		uint8_t system, gyro, accel, mag;
		this->getCalibration(&system, &gyro, &accel, &mag);
		return (system >= 2 && gyro >= 2 && accel >= 2 && mag >= 2);
	} else {
		return true;
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

float IMUClass::getTumbleAngle() const {
	if (!this->_tumbleReferenceSet) {
		return 0.0f;  // No reference set
	}

	// Calculate dot product between current and initial up vectors
	float upMag = sqrtf(this->_upX * this->_upX + this->_upY * this->_upY + this->_upZ * this->_upZ);
	float upStartMag = sqrtf(this->_upStartX * this->_upStartX + this->_upStartY * this->_upStartY + this->_upStartZ * this->_upStartZ);
	float dotProduct = (this->_upX * this->_upStartX + this->_upY * this->_upStartY + this->_upZ * this->_upStartZ) / (upMag * upStartMag);

	// Convert to angle in degrees
	float angleRadians = acosf(dotProduct);
	return angleRadians * 57.2958f; // 180 / PI
}

void IMUClass::setAxisRemap(uint8_t config, uint8_t sign) {
	this->_axisRemapConfig = config;
	this->_axisRemapSign = sign;
	this->applyAxisRemap();
}

void IMUClass::getAxisRemap(uint8_t *config, uint8_t *sign) const {
	*config = this->_axisRemapConfig;
	*sign = this->_axisRemapSign;
}

void IMUClass::applyAxisRemap() {
	constexpr uint8_t BNO055_OPR_MODE_ADDR = 0x3D;
	constexpr uint8_t BNO055_AXIS_MAP_CONFIG_ADDR = 0x41;
	constexpr uint8_t BNO055_AXIS_MAP_SIGN_ADDR = 0x42;

	if (this->_isBNO055) {
		// Must be in CONFIG mode to change axis remap
		this->writeRegisterBNO(BNO055_OPR_MODE_ADDR, OPERATION_MODE_CONFIG);
		delay(25);

		// Write custom axis remap configuration
		this->writeRegisterBNO(BNO055_AXIS_MAP_CONFIG_ADDR, _axisRemapConfig);
		delay(10);

		// Write custom axis sign configuration
		this->writeRegisterBNO(BNO055_AXIS_MAP_SIGN_ADDR, _axisRemapSign);
		delay(10);

		// Switch to NDOF mode (all sensors + fusion)
		this->writeRegisterBNO(BNO055_OPR_MODE_ADDR, OPERATION_MODE_ACCGYRO);
		delay(25);
	}
}

IMUOrientation IMUClass::detectOrientation() const {
	// Note: Accelerometer reads NEGATIVE when axis points UP (gravity pulls down)
	// and POSITIVE when axis points DOWN (accelerating toward ground)
	//
	// Check which axis is aligned with gravity
	bool xAligned = (abs(this->_gravityX) > this->_flatGravityMin && abs(this->_gravityX) < this->_flatGravityMax);
	bool yAligned = (abs(this->_gravityY) > this->_flatGravityMin && abs(this->_gravityY) < this->_flatGravityMax);
	bool zAligned = (abs(this->_gravityZ) > this->_flatGravityMin && abs(this->_gravityZ) < this->_flatGravityMax);

    // Z-axis aligned (physical X+ up = normal vertical)
    if (zAligned && abs(this->_gravityX) < this->_flatOtherAxisMax && abs(this->_gravityY) < this->_flatOtherAxisMax) {
        return (this->_gravityZ < 0) ? IMUOrientation::Z_POS : IMUOrientation::Z_NEG;
    }

    // X-axis aligned (tilted toward physical Z direction)
    if (xAligned && abs(this->_gravityY) < this->_flatOtherAxisMax && abs(this->_gravityZ) < this->_flatOtherAxisMax) {
        return (this->_gravityX < 0) ? IMUOrientation::X_POS : IMUOrientation::X_NEG;
    }

    // Y-axis aligned (tilted sideways)
    if (yAligned && abs(this->_gravityX) < this->_flatOtherAxisMax && abs(this->_gravityZ) < this->_flatOtherAxisMax) {
        return (this->_gravityY < 0) ? IMUOrientation::Y_POS : IMUOrientation::Y_NEG;
    }

    // Not aligned with any axis
    return IMUOrientation::TILTED;
}

void IMUClass::updateCalibration(sensors_event_t *accel, sensors_event_t *gyro) {
	constexpr size_t SAMPLE_COUNT = 21;
	static vector_ijk gyroSamples[SAMPLE_COUNT];
	static size_t writeIndex = 0;
	static bool filledOnce = false;

	//
	gyroSamples[writeIndex] = { gyro->gyro.x, gyro->gyro.y, gyro->gyro.z };
	writeIndex++;
	if (writeIndex >= SAMPLE_COUNT) {
		writeIndex = 0;
		filledOnce = true;
	}

	//
	if (filledOnce) {
		// Calculate average.
		vector_ijk average = { 0.0F, 0.0F, 0.0F };
		for (size_t index = 0; index < SAMPLE_COUNT; index++) {
			average = vector_3d_sum(average, gyroSamples[index]);
		}
		average = vector_3d_scale(average, 1.0F / static_cast<float>(SAMPLE_COUNT));

		// Calculate variance.
		vector_ijk variance = { 0.0F, 0.0F, 0.0F };
		for (size_t index = 0; index < SAMPLE_COUNT; index++) {
			vector_ijk difference = vector_3d_difference(average, gyroSamples[index]);
			variance = vector_3d_sum(variance, {
				difference.a * difference.a,
				difference.b * difference.b,
				difference.c * difference.c
			});
		}
		variance = vector_3d_scale(variance, 1.0F / static_cast<float>(SAMPLE_COUNT - 1));
		float combined_variance = sqrtf(variance.a * variance.a + variance.b * variance.b + variance.c * variance.c);

		//
		if (combined_variance < 1E-5F) {
			_biasGyroX = average.a;
			_biasGyroY = average.b;
			_biasGyroZ = average.c;
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
    recipNorm = 1.0F / sqrt(_quaternion[0] * _quaternion[0] + _quaternion[1] * _quaternion[1] + _quaternion[2] * _quaternion[2] + _quaternion[3] * _quaternion[3]);
    _quaternion[0] = _quaternion[0] * recipNorm;
    _quaternion[1] = _quaternion[1] * recipNorm;
    _quaternion[2] = _quaternion[2] * recipNorm;
    _quaternion[3] = _quaternion[3] * recipNorm;
}

void IMUClass::calculateGravity(float *x, float *y, float *z) {
	constexpr float GRAVITY = 9.81f;

	Quaternion mahonyQuaternion = quaternion_initialize(this->_quaternion[0], this->_quaternion[1], this->_quaternion[2], this->_quaternion[3]);
	vector_ijk gravityZ = quaternion_rotate_vector({ 0.0f, 0.0f, GRAVITY }, mahonyQuaternion);
	vector_ijk gravityY = quaternion_rotate_vector({ 0.0f, GRAVITY, 0.0f }, mahonyQuaternion);
	vector_ijk gravityX = quaternion_rotate_vector({ GRAVITY, 0.0f, 0.0f }, mahonyQuaternion);

	*x = gravityX.c;
	*y = gravityY.c;
	*z = gravityZ.c;
}

void IMUClass::writeRegisterBNO(uint8_t registerAddr, uint8_t value) {
	Wire.beginTransmission(0x28);
	Wire.write(registerAddr);
	Wire.write(value);
	Wire.endTransmission();
	delay(2);
}

uint8_t IMUClass::readRegisterBNO(uint8_t registerAddr) const {
	Wire.beginTransmission(0x28);
	Wire.write(registerAddr);
	Wire.endTransmission();
	Wire.requestFrom(0x28, 1);
	return Wire.read();
}
