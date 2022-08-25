#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include <Arduino.h>

#include "registers.h"

extern int16_t reg[N_REGISTERS];

bool CommTimedOut(long current_millis);
void OnUdpReceived(long current_millis, uint8_t data[], int length,
                   uint8_t retData[], int* retSize);

#endif  // COMMUNICATION_H_