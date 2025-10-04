#ifndef IMUHELPERS_H_
#define IMUHELPERS_H_

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <EEPROM.h>

#define LOWERBOUND 9.0  //g-values boundaries for axis detection
#define UPPERBOUND 10.50
#define TWOPI 6.2831853072

class IMUSensor {
public:
  virtual ~IMUSensor() {}

  // Sensor specific functions
  virtual void init() {}
  virtual void update() {}

  // Independent functions
  void reset();
  //void measureBias();
  bool tumbled(float minRotation);
  bool isMoving();
  bool isNotMoving() {
    return !isMoving();
  }

  float getXGravity() const {
    return _xGravity;
  }
  float getYGravity() const {
    return _yGravity;
  }
  float getZGravity() const {
    return _zGravity;
  }

protected:
  virtual void processData(sensors_event_t *event) {}
  void updateUpVector(double deltaTime);

protected:
  const float threshold = 0.5; //maximum acceleration to indicate stable
  const unsigned long stableTime = 200;  //ms)

  unsigned long _prevMicros;
  unsigned long _lastMovementTime;

  double _xUp, _yUp, _zUp;
  double _xUpStart, _yUpStart, _zUpStart;
 // double _xGyroBias, _yGyroBias, _zGyroBias;

  float _xGyro, _yGyro, _zGyro;
  float _xGravity, _yGravity, _zGravity;
  float _ax, _ay, _az, _magnitude;
  // float _xRotationMagnitude, _yRotationMagnitude, _zRotationMagnitude;

  bool _isMoving;
};

#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

class BNO055IMUSensor : public IMUSensor {
public:
  void init() override;
  void update() override;
  void processData(sensors_event_t *event) override;

private:
  Adafruit_BNO055 _accGyro;
  void restoreCalibrationData();
  void displaySensorOffsets(const adafruit_bno055_offsets_t &calibData);
  void displayCalStatus(void);
};

#endif /* IMUHELPERS_H_ */
