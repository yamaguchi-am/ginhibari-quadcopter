#ifndef HAL_H_
#define HAL_H_

enum class DisplayState { READY, CALIBRATING };

void SetupPwm();
void OutputPwm(int id, int duty);
int GetBattAD();
void ShowLED(DisplayState state);

#endif  // HAL_H_