//
// Created by CafuuChino on 2022/4/25.
//

#ifndef SC_TIMER_SC_TIMER_CORE_H
#define SC_TIMER_SC_TIMER_CORE_H

#include "main.h"



#define FLASH_DATA_ADDR 0x08007C00

#define PROFILE_NUM 1
#define OUTPUT_CHANNEL 22

typedef struct GPIO_Package_def {
    GPIO_TypeDef *GPIO_Type;
    uint16_t GPIO_Num;
} GPIO_Package;

extern volatile uint16_t itr_time_count;
extern volatile uint8_t itr_exec_count;
extern volatile uint8_t cdc_RX_enable;
extern uint8_t *cdc_cmd_ptr;
extern uint32_t cdc_cmd_len;
extern uint16_t trig_mode;
extern uint16_t profile_data[PROFILE_NUM][OUTPUT_CHANNEL*2];
extern GPIO_TypeDef *GPIO_Bank_List[OUTPUT_CHANNEL];
extern uint16_t GPIO_Pin_List[OUTPUT_CHANNEL];
void main_setup();
void main_loop();
void start_running(uint8_t profile_index);

#endif //SC_TIMER_SC_TIMER_CORE_H
