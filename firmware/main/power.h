#ifndef POWER_H
#define POWER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// On new PCBs (v4.0+), this function will configure the GPIO power pins for the I2C devices and the
// screens. After configuring, the I2C devices will be enabled and the screens will remain disabled.
//
// On old PCBs, this function will configure the SHUTDOWN pin and will set it to LOW, which will
// stop the power board from cutting the power to the dice.
void power_prepare();

// This function will enable the power to the screens.
//
// This function only works on new PCBs (v4.0+).
void power_on_screens();

// This function will disable the power to the screens.
//
// This function only works on new PCBs (v4.0+).
void power_off_screens();

// On new PCBs (v4.0+), this function will disable the power to the I2C devices and the screens and
// will then wait until it can enter deep sleep.
//
// On old PCBs, this function will set the SHUTDOWN pin to HIGH, which will cause the power board to
// cut the power to the dice.
void power_shutdown();

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POWER_H
