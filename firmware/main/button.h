#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void button_start(bool pullup);
bool button_is_pressed();
bool button_poll_press_short();
bool button_poll_press_double();
bool button_poll_press_long();

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // BUTTON_H
