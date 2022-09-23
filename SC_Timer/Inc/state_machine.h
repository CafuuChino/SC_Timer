//
// Created by CafuuChino on 2022/9/17.
//

#ifndef SC_TIMER_STATE_MACHINE_H
#define SC_TIMER_STATE_MACHINE_H
#include <stdint.h>
#define STATEMACHINE_DEFAULT 0
#define STATEMACHINE_HL_PROFILE 1
#define STATEMACHINE_HL_CH 2
#define STATEMACHINE_HL_TIME1 3
#define STATEMACHINE_HL_TIME2 4
#define STATEMACHINE_HL_TRIG 5

#define DISPLAY_MODE_ABS 0

extern uint8_t statemachine_state;
extern uint8_t statemachine_update;
void statemachine();

#endif //SC_TIMER_STATE_MACHINE_H
